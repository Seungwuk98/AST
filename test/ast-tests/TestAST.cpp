#include "TestAST.h"
#include "TestAST2.h"
#include "ast/ASTBuilder.h"
#include "ast/ASTTypeID.h"

DEFINE_TYPE_ID(ast::test::TestASTSet)
DEFINE_TYPE_ID(ast::test::TestAST1)
DEFINE_TYPE_ID(ast::test::TestIf)

namespace ast::test {

void TestASTSet::RegisterSet() {
  ASTBuilder::registerAST<TestAST1>(getContext());
  ASTBuilder::registerAST<TestIf>(getContext());
  ASTBuilder::registerAST<
#define AST_TABLEGEN_ID_COMMA
#include "TestAST2.hpp.inc"
      >(getContext());
}

TestAST1 TestAST1::create(ASTContext *ctx, int value1, int value2) {
  return Base::create(ctx, value1, value2);
}

void TestAST1::print(TestAST1 ast, ASTPrinter &printer) {
  printer.OS() << "TestAST1(" << ast.getValue1() << ", " << ast.getValue2()
               << ")";
}

TestIf TestIf::create(ASTContext *ctx, AST condition, AST thenBranch,
                      AST elseBranch) {
  return Base::create(ctx, condition, thenBranch, elseBranch);
}

void TestIf::print(TestIf ast, ASTPrinter &printer) {
  printer.OS() << "TestIf(";
  ast.getCondition().print(printer);
  printer.OS() << ") {";
  {
    ASTPrinter::AddIndentScope scope(printer, 2);
    printer.Line();
    ast.getThenBranch().print(printer);
  }
  printer.Line() << "} else {";
  {
    ASTPrinter::AddIndentScope scope(printer, 2);
    printer.Line();
    ast.getElseBranch().print(printer);
  }
  printer.Line() << "}";
}

} // namespace ast::test
