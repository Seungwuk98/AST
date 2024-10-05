#include "TableGenEmitter.h"
#include "CXXComponent.h"
#include "CXXType.h"
#include "TableGenModel.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/TableGen/Record.h"

namespace ast::tblgen {

bool ASTDeclGenMain(llvm::raw_ostream &OS, llvm::RecordKeeper &Records) {
  TableGenEmitter E(OS, Records);
  std::vector<llvm::Record *> astRecords =
      Records.getAllDerivedDefinitions("AST");

  std::vector<std::unique_ptr<ASTDeclModel>> astDeclModels;
  astDeclModels.reserve(astRecords.size());

  for (auto *R : astRecords) {
    auto dataModelOpt = DataModel::create(&E, R);
    if (!dataModelOpt)
      return true; /// exit code 1

    auto astDeclModel = ASTDeclModel::create(*dataModelOpt);
    if (!astDeclModel)
      return true; /// exit code 1

    astDeclModels.emplace_back(std::move(astDeclModel));
  }

  cxx::ComponentPrinter printer(OS);

  /// Print AST Names
  {
    cxx::ComponentPrinter::DefineScope scope(printer, "AST_TABLEGEN_ID");
    for (const auto &astDeclModel : astDeclModels)
      printer.OS() << llvm::formatv("AST_TABLEGEN_ID({0})\n",
                                    astDeclModel->getClassName());
  }

  /// Print comma joined AST Names
  {
    cxx::ComponentPrinter::DefineScope scope(printer, "AST_TABLEGEN_ID_COMMA");
    for (auto I = astDeclModels.begin(), E = astDeclModels.end(); I != E;) {
      if (I != astDeclModels.begin())
        printer.OS() << ", ";
      printer.OS() << (*I)->getNamespaceName() << "::" << (*I)->getClassName()
                   << '\n';
    }
  }

  /// Print AST Declarations
  {
    cxx::ComponentPrinter::DefineScope scope(printer, "AST_TABLEGEN_DECL");
    for (const auto &astDeclModel : astDeclModels) {
      cxx::ComponentPrinter::NamespaceScope namespaceScope(
          printer, astDeclModel->getNamespaceName());
      astDeclModel->getForwardClassDecl()->print(printer);
      astDeclModel->getForwardClassImplDecl()->print(printer.PrintLine());
      astDeclModel->getClassImplDecl()->print(printer.PrintLine());
      astDeclModel->getClassDecl()->print(printer.PrintLine());
    }

    // print declaring type id
    for (const auto &astDeclModel : astDeclModels) {
      printer.OS() << "DECLARE_TYPE_ID(" << astDeclModel->getNamespaceName()
                   << "::" << astDeclModel->getClassName() << ")\n";
    }
  }

  return false;
}

bool ASTDefGenMain(llvm::raw_ostream &OS, llvm::RecordKeeper &Records) {
  return false;
}

TableGenEmitter::TableGenEmitter(llvm::raw_ostream &os,
                                 llvm::RecordKeeper &records)
    : os(os), records(records), context(new TableGenContext) {
  astType = cxx::RawType::create(context, "::ast::AST", {});
  astImplType = cxx::RawType::create(context, "::ast::ASTImpl", {});
  astContextType = cxx::RawType::create(context, "::ast::ASTContext", {});
  astSetType = cxx::RawType::create(context, "::ast::ASTSet", {});
  voidType = cxx::RawType::create(context, "void", {});
  autoType = cxx::RawType::create(context, "auto", {});
  constAutoRefType = cxx::createConstReferenceType(context, autoType);
  astBuilderType = cxx::RawType::create(context, "::ast::ASTBuilder", {});
  astPrinterRef = cxx::ReferenceType::create(
      context, cxx::RawType::create(context, "::ast::ASTPrinter", {}));
}
TableGenEmitter::~TableGenEmitter() { delete context; }

std::tuple<llvm::SmallVector<llvm::StringRef>,
           llvm::SmallVector<TableGenEmitter::TypePair>, bool>
TableGenEmitter::getTypePairs(const llvm::DagInit *dag) {
  assert(dag->getOperator()->getAsString() == "ins");
  llvm::SmallVector<llvm::StringRef> paramNames;
  llvm::SmallVector<TypePair> typePairs;
  paramNames.reserve(dag->getNumArgs());
  typePairs.reserve(dag->getNumArgs());

  for (auto idx = 0u; idx < dag->getNumArgs(); ++idx) {
    llvm::Init *arg = dag->getArg(idx);
    const auto &[typePair, success] = getTypePair(arg);
    if (!success)
      return {{}, {}, false};

    llvm::StringRef paramName = dag->getArgNameStr(idx);
    if (paramName.empty())
      llvm::PrintFatalError("Parameter name is empty");
    paramNames.emplace_back(paramName);
    typePairs.emplace_back(typePair);
  }

  return {paramNames, typePairs, true};
}

std::pair<TableGenEmitter::TypePair, bool>
TableGenEmitter::getTypePair(const llvm::Init *init) {
  if (const auto *stringInit = llvm::dyn_cast<llvm::StringInit>(init)) {
    auto *retType = cxx::RawType::create(context, stringInit->getValue(), {});
    return {std::make_pair(retType, retType), true};
  }

  if (const auto *defInit = llvm::dyn_cast<llvm::DefInit>(init))
    return getTypePairByDefInit(defInit);

  llvm::PrintFatalError(llvm::formatv(
      "Unsupported init type for convert c++ type: {0}", init->getAsString()));
}

std::pair<TableGenEmitter::TypePair, bool>
TableGenEmitter::getTypePairByDefInit(const llvm::DefInit *defInit) {
  auto *record = defInit->getDef();
  assert(record->isSubClassOf("DataFormat"));

  llvm::StringRef paramTypeStr = record->getValueAsString("paramType");
  std::optional<llvm::StringRef> viewTypeStrOpt =
      record->getValueAsOptionalString("viewType");

  if (record->isSubClassOf("String")) {
    auto *paramType = cxx::RawType::create(context, paramTypeStr, {});
    assert(viewTypeStrOpt);
    auto *viewType = cxx::RawType::create(context, *viewTypeStrOpt, {});
    return {std::make_pair(paramType, viewType), true};
  }

  if (record->isSubClassOf("Vector") || record->isSubClassOf("SmallVector") ||
      record->isSubClassOf("Optional")) {
    const auto &[typePair, success] =
        getTypePair(record->getValueInit("elementType"));
    if (!success)
      return {std::make_pair(nullptr, nullptr), false};
    auto *elementType = typePair.first;
    llvm::SmallVector<const cxx::Type *> typeArgs{elementType};

    cxx::Type *paramType =
        cxx::RawType::create(context, paramTypeStr, typeArgs);
    const cxx::Type *viewType;

    if (viewTypeStrOpt)
      viewType = cxx::RawType::create(context, *viewTypeStrOpt, typeArgs);
    else
      viewType = cxx::createConstReferenceType(context, viewType);
    return {std::make_pair(paramType, viewType), true};
  }

  if (record->isSubClassOf("Tuple") || record->isSubClassOf("Variant")) {
    llvm::ListInit *elementInits = record->getValueAsListInit("elementTypes");
    llvm::SmallVector<const cxx::Type *> typeArgs;
    typeArgs.reserve(elementInits->size());
    for (auto idx = 0u; idx < elementInits->size(); ++idx) {
      const auto &[typePair, success] =
          getTypePair(elementInits->getElement(idx));
      if (!success)
        return {std::make_pair(nullptr, nullptr), false};
      typeArgs.emplace_back(typePair.first);
    }

    const cxx::Type *paramType =
        cxx::RawType::create(context, paramTypeStr, typeArgs);
    assert(!viewTypeStrOpt);
    const cxx::Type *viewType =
        cxx::createConstReferenceType(context, paramType);
    return {std::make_pair(paramType, viewType), true};
  }

  if (record->isSubClassOf("UserDefineType")) {
    bool viewConstRef = record->getValueAsBit("viewConstRef");
    assert(viewConstRef && viewTypeStrOpt);
    const cxx::Type *paramType =
        cxx::RawType::create(context, paramTypeStr, {});
    const cxx::Type *viewType;
    if (viewTypeStrOpt)
      viewType = cxx::RawType::create(context, *viewTypeStrOpt, {});
    else if (viewConstRef)
      viewType = cxx::createConstReferenceType(context, paramType);
    else
      viewType = paramType;
    return {std::make_pair(paramType, viewType), true};
  }

  if (record->isSubClassOf("Pair")) {
    llvm::Init *firstTypeInit = record->getValueInit("firstType");
    llvm::Init *secondTypeInit = record->getValueInit("secondType");
    const auto &[firstTypePair, firstSuccess] = getTypePair(firstTypeInit);
    if (!firstSuccess)
      return {std::make_pair(nullptr, nullptr), false};
    const auto &[secondTypePair, secondSuccess] = getTypePair(secondTypeInit);
    if (!secondSuccess)
      return {std::make_pair(nullptr, nullptr), false};

    llvm::SmallVector<const cxx::Type *> typeArgs{firstTypePair.first,
                                                  secondTypePair.first};
    const cxx::Type *paramType =
        cxx::RawType::create(context, paramTypeStr, typeArgs);
    assert(!viewTypeStrOpt);
    const cxx::Type *viewType =
        cxx::createConstReferenceType(context, paramType);
    return {std::make_pair(paramType, viewType), true};
  }

  llvm::PrintFatalError(
      llvm::formatv("Unsupported record type: {0}", record->getName()));
}

} // namespace ast::tblgen
