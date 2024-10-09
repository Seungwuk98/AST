#include "ast/AST.h"
#include "ast/ASTVisitor.h"

namespace ast {

void AST::accept(Visitor &visitor) const { visitor.visit(*this); }

std::string AST::toString() const {
  std::string str;
  llvm::raw_string_ostream os(str);
  print(os);
  return os.str();
}

void AST::print(llvm::raw_ostream &os) const {
  ASTPrinter printer(os);
  print(printer);
}

void AST::print(ASTPrinter &printer) const {
  getASTKindProperty().getPrintFn()(*this, printer);
}

void AST::dump() const { print(llvm::errs()); }

} // namespace ast
