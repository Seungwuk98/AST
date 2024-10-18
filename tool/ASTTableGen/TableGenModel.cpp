#include "TableGenModel.h"
#include "CXXComponent.h"
#include "CXXType.h"
#include "TableGenContext.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <numeric>

namespace ast::tblgen {

std::optional<DataModel> DataModel::create(TableGenEmitter *emitter,
                                           llvm::Record *record) {
  assert(record->isSubClassOf("AST"));
  llvm::StringRef name = record->getName();

  const auto &[setName, astName] = name.split('_');
  assert(!setName.empty() && !astName.empty());

  std::string astImplName = (astName + "Impl").str();

  llvm::StringRef namespaceName = record->getValueAsString("namespace");
  llvm::StringRef description = record->getValueAsString("description");
  llvm::StringRef parentName = record->getValueAsString("parent");

  llvm::StringRef extraClassDecl =
      record->getValueAsString("extraClassDeclaration");
  llvm::StringRef extraClassDef =
      record->getValueAsString("extraClassDefinition");

  llvm::DagInit *treeMember = record->getValueAsDag("treeMember");
  llvm::DagInit *tag = record->getValueAsDag("tag");

  const auto &[paramNames, typePairs, success] =
      emitter->getTypePairs(treeMember);
  if (!success)
    return std::nullopt;

  const auto &[tagParamNames, tagTypePairs, tagSuccess] =
      emitter->getTypePairs(tag);
  if (!tagSuccess)
    return std::nullopt;

  return DataModel{
      .Emitter = emitter,
      .SetName = setName,
      .ASTName = astName,
      .Namespace = namespaceName,
      .Description = description,
      .Parent = parentName,
      .ExtraClassDeclaration = extraClassDecl,
      .ExtraClassDefinition = extraClassDef,
      .TreeMemberParamNames = paramNames,
      .TreeMemberTypePairs = typePairs,
      .TagParamNames = tagParamNames,
      .TagTypePairs = tagTypePairs,
  };
}

static cxx::Type *
createTupleType(TableGenContext *context,
                llvm::ArrayRef<const cxx::Type *> elementTypes) {
  return cxx::RawType::create(
      context, "std::tuple",
      llvm::SmallVector<const cxx::Type *>(elementTypes));
}

static std::string getGetterName(llvm::StringRef name) {
  std::string getterName;
  getterName.reserve(name.size() + 4);
  llvm::raw_string_ostream ss(getterName);
  ss << "get" << llvm::toUpper(name[0]) << name.drop_front();
  return getterName;
}

static std::string getSetterName(llvm::StringRef name) {
  std::string setterName;
  setterName.reserve(name.size() + 4);
  llvm::raw_string_ostream ss(setterName);
  ss << "set" << llvm::toUpper(name[0]) << name.drop_front();
  return setterName;
}

static cxx::Class::Method *
createTreeMemberGetterMethod(TableGenContext *context, std::size_t idx,
                             llvm::StringRef memberName,
                             const cxx::Type *viewType) {
  std::string getterName = getGetterName(memberName);

  auto getterBody = llvm::formatv("return std::get<{0}>(astTreeMember);", idx);
  cxx::Class::Method::InstanceAttribute getterAttr{.IsConst = true,
                                                   .Body = {getterBody.str()}};

  return cxx::Class::Method::create(context, viewType, getterName, std::nullopt,
                                    getterAttr);
}

static cxx::Class::Method *
createTagMemberGetterMethod(TableGenContext *context, std::size_t idx,
                            llvm::StringRef tagName,
                            const cxx::Type *viewType) {
  std::string getterName;
  getterName.reserve(tagName.size() + 7);
  llvm::raw_string_ostream ss(getterName);
  ss << "get" << llvm::toUpper(tagName[0]) << tagName.drop_front() << "Tag";

  auto getterBody = llvm::formatv("return std::get<{0}>(astTag);", idx);
  cxx::Class::Method::InstanceAttribute getterAttr{.IsConst = true,
                                                   .Body = {getterBody.str()}};

  return cxx::Class::Method::create(context, viewType, getterName, std::nullopt,
                                    getterAttr);
}

static std::string cast2ParamTypeExpr(llvm::StringRef arg,
                                      const cxx::Type *paramType) {
  return llvm::formatv("({0})({1})", paramType->toString(), arg).str();
}

static cxx::Class::Method *
createTagMemberSetterMethod(TableGenEmitter *emitter, std::size_t idx,
                            llvm::StringRef tagName, const cxx::Type *paramType,
                            const cxx::Type *viewType) {
  std::string setterName;
  setterName.reserve(tagName.size());
  llvm::raw_string_ostream ss(setterName);
  ss << "set" << llvm::toUpper(tagName[0]) << tagName.drop_front() << "Tag";

  auto setterBody = llvm::formatv("std::get<{0}>(astTag) = {1};", idx,
                                  cast2ParamTypeExpr(tagName, paramType));
  cxx::Class::Method::InstanceAttribute setterAttr{.IsConst = false,
                                                   .Body = {setterBody.str()}};
  return cxx::Class::Method::create(emitter->getContext(),
                                    emitter->getVoidType(), setterName,
                                    {{tagName.str(), viewType}}, setterAttr);
}

std::unique_ptr<ASTDeclModel> ASTDeclModel::create(const DataModel &model) {
  TableGenEmitter *emitter = model.Emitter;

  std::string astImplName = (model.ASTName + "Impl").str();

  cxx::Type *astType =
      cxx::RawType::create(emitter->getContext(),
                           (model.Namespace + "::" + model.ASTName).str(), {});
  cxx::Type *astImplType = cxx::RawType::create(
      emitter->getContext(), (model.Namespace + "::" + astImplName).str(), {});
  cxx::Type *astImplTypePointer =
      cxx::PointerType::create(emitter->getContext(), astImplType);

  //==---------------------------------------------------------------------==//
  /// AST Impl
  //==---------------------------------------------------------------------==//

  /// tree member declaration as std::tuple
  auto hasTreeMember = !model.TreeMemberParamNames.empty();
  cxx::Class::Field *treeMemberDecl = nullptr;
  llvm::SmallVector<cxx::Class::Method *> treeMemberGetters;
  llvm::SmallVector<const cxx::Type *> treeMemberElementTypes;
  llvm::SmallVector<const cxx::Type *> treeMemberViewTypes;

  if (hasTreeMember) {
    treeMemberElementTypes.reserve(model.TreeMemberTypePairs.size());
    treeMemberViewTypes.reserve(model.TreeMemberTypePairs.size());

    for (const auto &[paramType, viewType] : model.TreeMemberTypePairs) {
      treeMemberElementTypes.emplace_back(paramType);
      treeMemberViewTypes.emplace_back(viewType);
    }

    cxx::Type *treeMemberType =
        createTupleType(emitter->getContext(), treeMemberElementTypes);

    treeMemberDecl = cxx::Class::Field::create(
        emitter->getContext(), false, {"astTreeMember", treeMemberType});

    /// tree member getter
    treeMemberGetters.reserve(model.TreeMemberTypePairs.size());

    for (const auto &[idx, paramName, paramType] :
         llvm::enumerate(model.TreeMemberParamNames, treeMemberViewTypes)) {
      auto *getter = createTreeMemberGetterMethod(emitter->getContext(), idx,
                                                  paramName, paramType);
      treeMemberGetters.emplace_back(getter);
    }
  }

  /// tag declaration
  auto hasTag = !model.TagParamNames.empty();
  cxx::Class::Field *tagDecl = nullptr;
  llvm::SmallVector<cxx::Class::Method *> tagGetters;
  llvm::SmallVector<cxx::Class::Method *> tagSetters;
  llvm::SmallVector<const cxx::Type *> tagElementTypes;
  llvm::SmallVector<const cxx::Type *> tagViewTypes;

  if (hasTag) {
    tagElementTypes.reserve(model.TagTypePairs.size());
    tagViewTypes.reserve(model.TagTypePairs.size());

    for (const auto &[paramType, viewType] : model.TagTypePairs) {
      tagElementTypes.emplace_back(paramType);
      tagViewTypes.emplace_back(viewType);
    }

    cxx::Type *tagType =
        createTupleType(emitter->getContext(), tagElementTypes);
    tagDecl = cxx::Class::Field::create(emitter->getContext(), false,
                                        {"astTag", tagType});

    /// tag getter & setter
    tagGetters.reserve(model.TagTypePairs.size());
    tagSetters.reserve(model.TagTypePairs.size());

    for (const auto &[idx, paramName, paramType, viewType] :
         llvm::enumerate(model.TagParamNames, tagElementTypes, tagViewTypes)) {
      auto *getter = createTagMemberGetterMethod(emitter->getContext(), idx,
                                                 paramName, viewType);
      auto *setter = createTagMemberSetterMethod(emitter, idx, paramName,
                                                 paramType, viewType);
      tagGetters.emplace_back(getter);
      tagSetters.emplace_back(setter);
    }
  }

  /// traversal order
  cxx::Class::Method *traversalOrderMethod = nullptr;
  if (hasTreeMember) {
    traversalOrderMethod = cxx::Class::Method::create(
        emitter->getContext(), emitter->getConstAutoRefType(), "traversalOrder",
        std::nullopt,
        cxx::Class::Method::InstanceAttribute{
            .IsConst = true, .Body = {"return astTreeMember;"}});
  }

  /// friend class
  cxx::Class::Friend *friendASTContext = cxx::Class::Friend::create(
      emitter->getContext(), emitter->getASTContextType());
  cxx::Class::Friend *friendASTBuilder = cxx::Class::Friend::create(
      emitter->getContext(), emitter->getASTBuilderType());
  cxx::Class::Friend *friendAST =
      cxx::Class::Friend::create(emitter->getContext(), astType);

  /// constructor
  llvm::SmallVector<cxx::DeclPair> params;
  params.reserve(model.TreeMemberParamNames.size());

  for (const auto &[paramName, viewType] :
       llvm::zip(model.TreeMemberParamNames, treeMemberViewTypes)) {
    params.emplace_back(paramName, viewType);
  }
  cxx::Class::Constructor *constructor = cxx::Class::Constructor::create(
      emitter->getContext(), astImplName, params, std::nullopt);

  /// create function
  llvm::SmallVector<cxx::DeclPair> createParams{
      {"context", emitter->getASTContextPointerType()}};
  createParams.append(params.begin(), params.end());

  cxx::Class::Method *createMethod = cxx::Class::Method::create(
      emitter->getContext(), astImplTypePointer, "create", createParams,
      cxx::Class::Method::StaticAttribute{});

  /// private block
  llvm::SmallVector<cxx::Class::ClassMember> privateMembers;
  privateMembers.emplace_back(friendASTContext);
  privateMembers.emplace_back(friendASTBuilder);
  privateMembers.emplace_back(friendAST);
  privateMembers.emplace_back(constructor);
  privateMembers.emplace_back(createMethod);
  if (hasTreeMember)
    privateMembers.emplace_back(treeMemberDecl);
  if (hasTag)
    privateMembers.emplace_back(tagDecl);

  cxx::Class::Block privateBlock{.Access = cxx::Class::AccessModifier::Private,
                                 .Members = privateMembers};

  /// public block
  llvm::SmallVector<cxx::Class::ClassMember> publicMembers;
  if (hasTreeMember) {
    publicMembers.emplace_back(traversalOrderMethod);
    publicMembers.append(treeMemberGetters.begin(), treeMemberGetters.end());
  }
  if (hasTag) {
    publicMembers.append(tagGetters.begin(), tagGetters.end());
    publicMembers.append(tagSetters.begin(), tagSetters.end());
  }
  cxx::Class::Block publicBlock{.Access = cxx::Class::AccessModifier::Public,
                                .Members = publicMembers};

  /// class definition
  cxx::Class::ClassImplementation impl{
      .Blocks =
          {
              publicBlock,
              privateBlock,
          },
      .Bases = {{true, emitter->getASTImplType()}}};

  cxx::Class *astImplClass =
      cxx::Class::create(emitter->getContext(),
                         cxx::Class::StructOrClass::Class, astImplName, impl);
  /// class declaration
  cxx::Class *astImplClassDecl = cxx::Class::create(
      emitter->getContext(), cxx::Class::StructOrClass::Class, astImplName,
      std::nullopt);

  //==---------------------------------------------------------------------==//
  /// AST
  //==---------------------------------------------------------------------==//

  /// using Base::Base
  cxx::Using::Member usingBaseConstructor{
      .Namespaces = {"Base"},
      .Name = {"Base"},
  };
  cxx::Using *usingBase =
      cxx::Using::create(emitter->getContext(), {usingBaseConstructor});

  /// tree getter
  llvm::SmallVector<cxx::Class::Method *> astTreeMemberGetters;
  if (hasTreeMember) {
    astTreeMemberGetters.reserve(model.TreeMemberParamNames.size());

    for (const auto &[idx, paramName, viewType] :
         llvm::enumerate(model.TreeMemberParamNames, treeMemberViewTypes)) {
      auto getterName = getGetterName(paramName);
      auto getterBody =
          llvm::formatv("return getImpl()->get{0}{1}();",
                        llvm::toUpper(paramName[0]), paramName.drop_front());
      cxx::Class::Method::InstanceAttribute getterAttr{
          .IsConst = true, .Body = {getterBody.str()}};
      cxx::Class::Method *getter =
          cxx::Class::Method::create(emitter->getContext(), viewType,
                                     getterName, std::nullopt, getterAttr);
      astTreeMemberGetters.emplace_back(getter);
    }
  }
  /// tag getter & setter
  llvm::SmallVector<cxx::Class::Method *> astTagGetters;
  llvm::SmallVector<cxx::Class::Method *> astTagSetters;

  if (hasTag) {
    astTagGetters.reserve(model.TreeMemberTypePairs.size());
    astTagSetters.reserve(model.TreeMemberTypePairs.size());

    for (const auto &[idx, paramName, paramType, viewType] :
         llvm::enumerate(model.TagParamNames, tagElementTypes, tagViewTypes)) {
      auto getterName = getGetterName(paramName) + "Tag";
      auto setterName = getSetterName(paramName) + "Tag";

      auto getterBody =
          llvm::formatv("return getImpl()->get{0}{1}Tag();",
                        llvm::toUpper(paramName[0]), paramName.drop_front());
      cxx::Class::Method::InstanceAttribute getterAttr{
          .IsConst = true, .Body = {getterBody.str()}};
      cxx::Class::Method *getter =
          cxx::Class::Method::create(emitter->getContext(), viewType,
                                     getterName, std::nullopt, getterAttr);

      auto setterBody = llvm::formatv("getImpl()->set{0}{1}Tag({2});",
                                      llvm::toUpper(paramName[0]),
                                      paramName.drop_front(), paramName);
      cxx::Class::Method::InstanceAttribute setterAttr{
          .IsConst = false, .Body = {setterBody.str()}};
      cxx::Class::Method *setter = cxx::Class::Method::create(
          emitter->getContext(), emitter->getVoidType(), setterName,
          {{paramName.str(), viewType}}, setterAttr);
      astTagGetters.emplace_back(getter);
      astTagSetters.emplace_back(setter);
    }
  }

  cxx::Class::Method *astTraversalOrderMethod = nullptr;
  if (hasTreeMember) {
    /// traversal order
    astTraversalOrderMethod = cxx::Class::Method::create(
        emitter->getContext(), emitter->getConstAutoRefType(), "traversalOrder",
        {},
        cxx::Class::Method::InstanceAttribute{
            .IsConst = true, .Body = {"return getImpl()->traversalOrder();"}});
  }

  /// print method
  cxx::Class::Method *astPrintMethod = cxx::Class::Method::create(
      emitter->getContext(), emitter->getVoidType(), "print",
      {{"ast", astType}, {"printer", emitter->getASTPrinterRef()}},
      cxx::Class::Method::StaticAttribute{});

  /// create function
  llvm::SmallVector<cxx::DeclPair> astCreateParam{
      {"loc", emitter->getllmvSMRangeType()},
      {"context", emitter->getASTContextPointerType()}};
  astCreateParam.append(params.begin(), params.end());
  cxx::Class::Method *astCreateFunc = cxx::Class::Method::create(
      emitter->getContext(), astType, "create", astCreateParam,
      cxx::Class::Method::StaticAttribute{});

  /// public block
  llvm::SmallVector<cxx::Class::ClassMember> astPublicMembers;

  astPublicMembers.emplace_back(usingBase);
  if (hasTreeMember) {
    astPublicMembers.append(astTreeMemberGetters.begin(),
                            astTreeMemberGetters.end());
  }
  if (hasTag) {
    astPublicMembers.append(astTagGetters.begin(), astTagGetters.end());
    astPublicMembers.append(astTagSetters.begin(), astTagSetters.end());
  }
  if (hasTreeMember)
    astPublicMembers.emplace_back(astTraversalOrderMethod);

  astPublicMembers.emplace_back(astPrintMethod);
  astPublicMembers.emplace_back(astCreateFunc);

  /// extra class declaration
  if (!model.ExtraClassDeclaration.empty()) {
    cxx::Class::RawCode *extraClassDeclaration = cxx::Class::RawCode::create(
        emitter->getContext(), model.ExtraClassDeclaration);
    astPublicMembers.emplace_back(extraClassDeclaration);
  }

  cxx::Class::Block astPublicBlock{.Access = cxx::Class::AccessModifier::Public,
                                   .Members = astPublicMembers};

  /// class base
  cxx::Type *classBase = cxx::RawType::create(
      emitter->getContext(), "::ast::AST::Base",
      llvm::SmallVector<const cxx::Type *>{
          astType,
          cxx::RawType::create(emitter->getContext(), model.Parent, {}),
          astImplType});

  cxx::Class::ClassImplementation astDefinition{
      .Blocks = {astPublicBlock},
      .Bases = {{true, classBase}},
  };
  /// class definition
  cxx::Class *astClass = cxx::Class::create(emitter->getContext(),
                                            cxx::Class::StructOrClass::Class,
                                            model.ASTName, astDefinition);
  cxx::Class *astClassDecl = cxx::Class::create(
      emitter->getContext(), cxx::Class::StructOrClass::Class, model.ASTName,
      std::nullopt);

  return std::unique_ptr<ASTDeclModel>(new ASTDeclModel(
      model.ASTName, astImplName, model.Namespace, model.Description,
      astClassDecl, astImplClassDecl, astClass, astImplClass));
}

std::unique_ptr<ASTDefModel> ASTDefModel::create(const DataModel &model) {
  TableGenEmitter *emitter = model.Emitter;

  std::string astImplName = (model.ASTName + "Impl").str();

  cxx::Type *astType =
      cxx::RawType::create(emitter->getContext(), model.ASTName, {});
  cxx::Type *astImplType = cxx::RawType::create(
      emitter->getContext(), (model.ASTName + "Impl").str(), {});
  cxx::Type *astImplTypePointer =
      cxx::PointerType::create(emitter->getContext(), astImplType);

  llvm::SmallVector<const cxx::Type *> treeParamTypes;
  llvm::SmallVector<const cxx::Type *> treeViewTypes;
  treeParamTypes.reserve(model.TreeMemberParamNames.size());
  treeViewTypes.reserve(model.TreeMemberParamNames.size());

  for (const auto &[paramType, viewType] : model.TreeMemberTypePairs) {
    treeParamTypes.emplace_back(paramType);
    treeViewTypes.emplace_back(viewType);
  }

  llvm::SmallVector<cxx::DeclPair> param;
  param.reserve(model.TreeMemberParamNames.size());
  for (const auto &[paramName, viewType] :
       llvm::zip(model.TreeMemberParamNames, treeViewTypes)) {
    param.emplace_back(paramName, viewType);
  }

  /// ast create function
  llvm::SmallVector<cxx::DeclPair> createParam{
      {"loc", emitter->getllmvSMRangeType()},
      {"context", emitter->getASTContextPointerType()}};

  createParam.append(param.begin(), param.end());

  std::string arguments;
  llvm::raw_string_ostream ss(arguments);
  for (const auto &paramName : model.TreeMemberParamNames)
    ss << ", " << paramName;

  auto createBody =
      llvm::formatv("return Base::create(loc, context{0});", arguments);

  auto *astCreateFunc = cxx::Function::create(
      emitter->getContext(), std::nullopt, cxx::Function::Access::None, astType,
      llvm::SmallVector<std::string>{model.ASTName.str()}, "create",
      createParam, cxx::BodyCode{createBody.str()});

  /// ast impl create function
  llvm::SmallVector<cxx::DeclPair> implCreateParam{
      {"context", emitter->getASTContextPointerType()}};
  implCreateParam.append(param.begin(), param.end());

  auto implCreateBody =
      llvm::formatv("return context->Alloc<{0}>({1});", astImplName,
                    llvm::join(model.TreeMemberParamNames, ", "));
  auto *astImplCreateFunc = cxx::Function::create(
      emitter->getContext(), std::nullopt, cxx::Function::Access::None,
      astImplTypePointer, llvm::SmallVector<std::string>{astImplName}, "create",
      implCreateParam, cxx::BodyCode{implCreateBody.str()});

  /// ast impl constructor
  llvm::SmallVector<std::pair<std::string, std::string>> initializerList;
  std::string initializeExpr;
  llvm::raw_string_ostream initExprStream(initializeExpr);

  for (const auto &[idx, paramName, paramType] :
       llvm::enumerate(model.TreeMemberParamNames, treeParamTypes)) {
    if (idx != 0)
      initExprStream << ", ";
    initExprStream << cast2ParamTypeExpr(paramName, paramType);
  }
  initializerList.emplace_back("astTreeMember", initializeExpr);

  cxx::Class::Constructor::Implement constructorImplement{
      .Initializers = initializerList,
      .Body = {},
  };
  auto *astImplConstructor =
      cxx::ClassConstructor::create(emitter->getContext(), std::nullopt,
                                    astImplName, param, constructorImplement);

  return std::unique_ptr<ASTDefModel>(
      new ASTDefModel(model.ASTName, astImplName, model.Namespace,
                      model.Description, model.ExtraClassDefinition,
                      astImplCreateFunc, astImplConstructor, astCreateFunc));
}

} // namespace ast::tblgen
