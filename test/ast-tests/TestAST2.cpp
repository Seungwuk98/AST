#include "TestAST2.h"
#include "ast/ASTPrinter.h"

#define AST_TABLEGEN_DEF
#include "TestAST2.cpp.inc"

namespace ast::test {

void TestFor::print(TestFor testFor, ASTPrinter &printer) {
  printer.OS() << "for (" << testFor.getIterName() << " from ";
  testFor.getFromE().print(printer);
  printer.OS() << " to ";
  testFor.getToE().print(printer);
  printer.OS() << " step ";
  testFor.getStepE().print(printer);
  printer.OS() << ") {";
  {
    ASTPrinter::AddIndentScope scope(printer, 2);
    printer.Line();
    testFor.getBodyE().print(printer);
  }
  printer.Line() << "}";
}

void Integer::print(Integer integer, ASTPrinter &printer) {
  printer.OS() << integer.getValue();
}

} // namespace ast::test
