#ifndef TEST_AST_VISITOR_H
#define TEST_AST_VISITOR_H

#include "TestAST2.h"
#include "ast/ASTVisitor.h"

namespace ast::test {

class TestASTVisitor : public VisitorBase<TestASTVisitor
#define AST_TABLEGEN_ID(ID) , ID
#include "TestAST2.hpp.inc"
                                          > {
public:
  TestASTVisitor(llvm::raw_ostream &OS) : OS(OS) {}

  void visit(Integer integer) {
    OS << "visit Integer : " << integer.getValue() << '\n';
  }

  void visit(TestFor testFor) {
    OS << "visit TestFor\n";
    testFor.getFromE().accept(*this);
    testFor.getToE().accept(*this);
    testFor.getStepE().accept(*this);
    testFor.getBodyE().accept(*this);
  }

private:
  llvm::raw_ostream &OS;
};

} // namespace ast::test

#endif // TEST_AST_VISITOR_H
