#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"

namespace ast::tblgen {

extern bool ASTDeclGenMain(llvm::raw_ostream &OS, llvm::RecordKeeper &Records);
extern bool ASTDefGenMain(llvm::raw_ostream &OS, llvm::RecordKeeper &Records);

} // namespace ast::tblgen

enum class ASTTableGenBackend {
  ASTDeclGen,
  ASTDefGen,
};

static llvm::cl::opt<ASTTableGenBackend> Backend(
    llvm::cl::desc("Choose AST tablegen backend:"),
    llvm::cl::values(clEnumValN(ASTTableGenBackend::ASTDeclGen, "ast-decl-gen",
                                "Generate AST declarations"),
                     clEnumValN(ASTTableGenBackend::ASTDefGen, "ast-def-gen",
                                "Generate AST definitions")),
    llvm::cl::init(ASTTableGenBackend::ASTDeclGen), llvm::cl::Required);

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv, "AST tablegen\n");

  std::function<llvm::TableGenMainFn> MainFn = nullptr;

  switch (Backend) {
  case ASTTableGenBackend::ASTDeclGen:
    MainFn = ast::tblgen::ASTDeclGenMain;
    break;
  case ASTTableGenBackend::ASTDefGen:
    MainFn = ast::tblgen::ASTDefGenMain;
    break;
  };

  return llvm::TableGenMain(argv[0], MainFn);
}
