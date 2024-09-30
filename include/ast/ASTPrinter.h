#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "llvm/Support/raw_ostream.h"

namespace ast {

class ASTPrinter {
public:
  explicit ASTPrinter(llvm::raw_ostream &os) : os(os), indentLevel(0) {}

  llvm::raw_ostream &OS() { return os; }
  llvm::raw_ostream &Line() {
    return os << '\n' << std::string(indentLevel, ' ');
  }

  struct IndentScope {
    explicit IndentScope(ASTPrinter &printer, std::size_t indentLevel)
        : printer(printer), savedIndentLevel(printer.indentLevel) {
      printer.indentLevel = indentLevel;
    }

    ~IndentScope() { printer.indentLevel = savedIndentLevel; }

  private:
    ASTPrinter &printer;
    std::size_t savedIndentLevel;
  };

  struct AddIndentScope {
  public:
    explicit AddIndentScope(ASTPrinter &printer, std::size_t amount)
        : scope(printer, printer.indentLevel + amount) {}

  private:
    IndentScope scope;
  };

private:
  llvm::raw_ostream &os;
  std::size_t indentLevel;
};

} // namespace ast

#endif // AST_PRINTER_H
