#ifndef AST_TABLEGEN_EMITTER_H
#define AST_TABLEGEN_EMITTER_H

#include "CXXType.h"
#include "TableGenContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"

namespace ast::tblgen {

class ASTDeclModel;
class ASTDefModel;

class TableGenEmitter {
public:
  TableGenEmitter(llvm::raw_ostream &os, llvm::RecordKeeper &records);
  ~TableGenEmitter();

  llvm::raw_ostream &OS() { return os; }
  llvm::RecordKeeper &Records() { return records; }
  TableGenContext *getContext() { return context; }

  llvm::SmallVector<std::unique_ptr<ASTDeclModel>> GetASTDeclModels();
  llvm::SmallVector<std::unique_ptr<ASTDefModel>> GetASTDefModels();

  using TypePair = std::pair<const cxx::Type *, const cxx::Type *>;

  std::tuple<llvm::SmallVector<llvm::StringRef>, llvm::SmallVector<TypePair>,
             bool>
  getTypePairs(const llvm::DagInit *dag);
  std::pair<TypePair, bool> getTypePair(const llvm::Init *init);
  std::pair<TypePair, bool> getTypePairByDefInit(const llvm::DefInit *defInit);

  const cxx::Type *getASTType() const { return astType; }
  const cxx::Type *getASTImplType() const { return astImplType; }
  const cxx::Type *getASTContextType() const { return astContextType; }
  const cxx::Type *getASTContextPointerType() const {
    return astContextPointerType;
  }
  const cxx::Type *getASTSetType() const { return astSetType; }
  const cxx::Type *getASTPrinterRef() const { return astPrinterRef; }
  const cxx::Type *getVoidType() const { return voidType; }
  const cxx::Type *getAutoType() const { return autoType; }
  const cxx::Type *getConstAutoRefType() const { return constAutoRefType; }
  const cxx::Type *getASTBuilderType() const { return astBuilderType; }
  const cxx::Type *getllmvSMRangeType() const { return llvmSMRangeType; }

private:
  llvm::raw_ostream &os;
  llvm::RecordKeeper &records;
  TableGenContext *context;

  const cxx::Type *astType;
  const cxx::Type *astImplType;
  const cxx::Type *astContextType;
  const cxx::Type *astContextPointerType;
  const cxx::Type *astSetType;
  const cxx::Type *astPrinterRef;
  const cxx::Type *voidType;
  const cxx::Type *autoType;
  const cxx::Type *constAutoRefType;
  const cxx::Type *astBuilderType;
  const cxx::Type *llvmSMRangeType;
};

} // namespace ast::tblgen

#endif // AST_TABLEGEN_EMITTER_H
