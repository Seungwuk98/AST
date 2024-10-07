#ifndef AST_TABLEGEN_MODEL_H
#define AST_TABLEGEN_MODEL_H

#include "CXXComponent.h"
#include "TableGenContext.h"
#include "TableGenEmitter.h"
#include "llvm/TableGen/Record.h"

namespace ast::tblgen {

struct DataModel {
  TableGenEmitter *Emitter;

  llvm::StringRef SetName;
  llvm::StringRef ASTName;
  llvm::StringRef Namespace;
  llvm::StringRef Description;
  llvm::StringRef Parent;
  llvm::StringRef ExtraClassDeclaration;
  llvm::StringRef ExtraClassDefinition;

  llvm::SmallVector<llvm::StringRef> TreeMemberParamNames;
  llvm::SmallVector<TableGenEmitter::TypePair> TreeMemberTypePairs;
  llvm::SmallVector<llvm::StringRef> TagParamNames;
  llvm::SmallVector<TableGenEmitter::TypePair> TagTypePairs;

  static std::optional<DataModel> create(TableGenEmitter *emitter,
                                         llvm::Record *record);
};

class ASTDeclModel {
public:
  llvm::StringRef getClassName() const { return className; }
  llvm::StringRef getClassImplName() const { return classImplName; }
  llvm::StringRef getNamespaceName() const { return namespaceName; }
  llvm::StringRef getDescription() const { return description; }

  cxx::Class *getForwardClassDecl() const { return forwardClassDecl; }
  cxx::Class *getForwardClassImplDecl() const { return forwadClassImplDecl; }
  cxx::Class *getClassDecl() const { return classDecl; }
  cxx::Class *getClassImplDecl() const { return classImplDecl; }

  static std::unique_ptr<ASTDeclModel> create(const DataModel &model);

private:
  ASTDeclModel(llvm::StringRef className, llvm::StringRef classImplName,
               llvm::StringRef namespaceName, llvm::StringRef description,
               cxx::Class *forwardClassDecl, cxx::Class *forwadClassImplDecl,
               cxx::Class *classDecl, cxx::Class *classImplDecl)
      : className(className), classImplName(classImplName),
        namespaceName(namespaceName), description(description),
        forwardClassDecl(forwardClassDecl),
        forwadClassImplDecl(forwadClassImplDecl), classDecl(classDecl),
        classImplDecl(classImplDecl) {}

  std::string className;
  std::string classImplName;
  std::string namespaceName;
  std::string description;
  cxx::Class *forwardClassDecl;
  cxx::Class *forwadClassImplDecl;
  cxx::Class *classDecl;
  cxx::Class *classImplDecl;
};

class ASTDefModel {
public:
  llvm::StringRef getClassName() const { return className; }
  llvm::StringRef getClassImplName() const { return classImplName; }
  llvm::StringRef getNamespaceName() const { return namespaceName; }
  llvm::StringRef getDescription() const { return description; }
  llvm::StringRef getExtraClassDefinition() const {
    return extraClassDefinition;
  }

  cxx::Function *getASTImplCreateFunction() const {
    return astImplCreateFunction;
  }
  cxx::ClassConstructor *getASTImplConstructor() const {
    return astImplConstructor;
  }
  cxx::Function *getASTCreateFunction() const { return astCreateFunction; }

  void print(llvm::raw_ostream &OS) const;

  static std::unique_ptr<ASTDefModel> create(const DataModel &model);

private:
  ASTDefModel(llvm::StringRef className, llvm::StringRef classImplName,
              llvm::StringRef namespaceName, llvm::StringRef description,
              llvm::StringRef extraClassDefinition,
              cxx::Function *astImplCreateFunction,
              cxx::ClassConstructor *astImplConstructor,
              cxx::Function *astCreateFunction)
      : className(className), classImplName(classImplName),
        namespaceName(namespaceName), description(description),
        extraClassDefinition(extraClassDefinition),
        astImplCreateFunction(astImplCreateFunction),
        astImplConstructor(astImplConstructor),
        astCreateFunction(astCreateFunction) {}

  std::string className;
  std::string classImplName;
  std::string namespaceName;
  std::string description;
  std::string extraClassDefinition;
  cxx::Function *astImplCreateFunction;
  cxx::ClassConstructor *astImplConstructor;
  cxx::Function *astCreateFunction;
};

} // namespace ast::tblgen

#endif // AST_TABLEGEN_MODEL_H
