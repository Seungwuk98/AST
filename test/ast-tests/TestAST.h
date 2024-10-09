#ifndef TEST_AST_H
#define TEST_AST_H

#include "ast/AST.h"
#include "ast/ASTSet.h"
#include "ast/ASTTypeID.h"

namespace ast::test {

class TestASTSet final : public ASTSet {
public:
  using ASTSet::ASTSet;

  void RegisterSet() override;

  llvm::StringRef getASTSetName() const override { return "TestAST"; }
};

class TestAST1Impl : public ASTImpl {
public:
  int getValue1() const { return value1; }
  int getValue2() const { return value2; }
  void setValue1(int value) { value1 = value; }
  void setValue2(int value) { value2 = value; }

private:
  friend class ::ast::ASTBuilder;
  friend class ::ast::ASTContext;
  TestAST1Impl(int value1, int value2) : value1(value1), value2(value2) {}

  static TestAST1Impl *create(ASTContext *ctx, int value1, int value2) {
    return ctx->Alloc<TestAST1Impl>(value1, value2);
  }

  int value1;
  int value2;
};

class TestAST1 : public AST::Base<TestAST1, AST, TestAST1Impl> {
public:
  using Base::Base;

  static TestAST1 create(llvm::SMRange loc, ASTContext *ctx, int value1,
                         int value2);

  int getValue1() const { return getImpl()->getValue1(); }
  int getValue2() const { return getImpl()->getValue2(); }
  void setValue1(int value) { getImpl()->setValue1(value); }
  void setValue2(int value) { getImpl()->setValue2(value); }

  const auto traversalOrder() const {
    return std::tuple(getValue1(), getValue2());
  }

  static void print(TestAST1 ast, ASTPrinter &printer);
};

class TestIfImpl : public ASTImpl {
public:
  AST getCondition() const { return condition; }
  AST getThenBranch() const { return thenBranch; }
  AST getElseBranch() const { return elseBranch; }

private:
  friend class ::ast::ASTBuilder;
  friend class ::ast::ASTContext;
  TestIfImpl(AST condition, AST thenBranch, AST elseBranch)
      : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}

  static TestIfImpl *create(ASTContext *ctx, AST condition, AST thenBranch,
                            AST elseBranch) {
    return ctx->Alloc<TestIfImpl>(condition, thenBranch, elseBranch);
  }

  AST condition;
  AST thenBranch;
  AST elseBranch;
};

class TestIf : public AST::Base<TestIf, AST, TestIfImpl> {
public:
  using Base::Base;

  static TestIf create(llvm::SMRange loc, ASTContext *ctx, AST condition,
                       AST thenBranch, AST elseBranch);

  AST getCondition() const { return getImpl()->getCondition(); }
  AST getThenBranch() const { return getImpl()->getThenBranch(); }
  AST getElseBranch() const { return getImpl()->getElseBranch(); }

  const auto traversalOrder() const {
    return std::tuple(getCondition(), getThenBranch(), getElseBranch());
  }

  static void print(TestIf ast, ASTPrinter &printer);
};

} // namespace ast::test

DECLARE_TYPE_ID(ast::test::TestASTSet)
DECLARE_TYPE_ID(ast::test::TestAST1)
DECLARE_TYPE_ID(ast::test::TestIf)

#endif // TEST_AST_H
