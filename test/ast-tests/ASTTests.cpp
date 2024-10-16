#include "TestAST.h"
#include "TestAST2.h"
#include "TestASTVisitor.h"
#include "ast/ASTContext.h"
#include "llvm/Support/raw_ostream.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

TEST_SUITE("ast test suite") {}

namespace ast::test {

TEST_CASE("AST Creation Test" * doctest::test_suite("ast test suite")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("TestAST1 test") {
    auto testAST1 = TestAST1::create({}, &ctx, 1, 2);
    CHECK_EQ(testAST1.getValue1(), 1);
    CHECK_EQ(testAST1.getValue2(), 2);

    CHECK_EQ(testAST1.toString(), "TestAST1(1, 2)");
  }

  SUBCASE("TestIf test") {
    auto testAST1 = TestAST1::create({}, &ctx, 1, 2);
    auto testAST2 = TestAST1::create({}, &ctx, 3, 4);
    auto testAST3 = TestAST1::create({}, &ctx, 5, 6);
    auto testIf = TestIf::create({}, &ctx, testAST1, testAST2, testAST3);

    CHECK_EQ(testIf.toString(), R"(TestIf(TestAST1(1, 2)) {
  TestAST1(3, 4)
} else {
  TestAST1(5, 6)
})");
  }
}

TEST_CASE("AST Walk Test" * doctest::test_suite("ast test suite")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("Simple walk test") {
    auto testAST1 = TestAST1::create({}, &ctx, 1, 2);
    auto testAST2 = TestAST1::create({}, &ctx, 3, 4);
    auto testAST3 = TestAST1::create({}, &ctx, 5, 6);
    auto testIf = TestIf::create({}, &ctx, testAST1, testAST2, testAST3);

    llvm::SmallVector<AST> asts;
    auto walkResult = testIf.walk([&asts](AST ast) {
      asts.push_back(ast);
      return WalkResult::success();
    });

    CHECK(walkResult.isSuccess());
    CHECK_EQ(asts.size(), 4);
    CHECK_EQ(asts[0], testAST1);
    CHECK_EQ(asts[1], testAST2);
    CHECK_EQ(asts[2], testAST3);
    CHECK_EQ(asts[3], testIf);
  }
}

TEST_CASE("AST Equality Test" * doctest::test_suite("ast test suite")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("TestAST1 equality test") {
    auto testAST1 = TestAST1::create({}, &ctx, 1, 2);
    auto testAST2 = TestAST1::create({}, &ctx, 1, 2);
    auto testAST3 = TestAST1::create({}, &ctx, 3, 4);

    CHECK(testAST1.isEqual(testAST2));
    CHECK_FALSE(testAST1.isEqual(testAST3));
  }

  SUBCASE("TestIf equality test") {
    auto testAST1 = TestAST1::create({}, &ctx, 1, 2);
    auto testAST2 = TestAST1::create({}, &ctx, 3, 4);
    auto testAST3 = TestAST1::create({}, &ctx, 5, 6);
    auto testIf1 = TestIf::create({}, &ctx, testAST1, testAST2, testAST3);
    auto testIf2 = TestIf::create({}, &ctx, testAST1, testAST2, testAST3);
    auto testIf3 = TestIf::create({}, &ctx, testAST1, testAST3, testAST2);

    CHECK(testIf1.isEqual(testIf2));
    CHECK_FALSE(testIf1.isEqual(testIf3));
  }
}

TEST_CASE("TableGen AST" * doctest::test_suite("ast test suite")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("TestFor") {
    auto one = Integer::create({}, &ctx, 1);
    auto two = Integer::create({}, &ctx, 2);
    auto three = Integer::create({}, &ctx, 3);
    auto four = Integer::create({}, &ctx, 4);
    auto testFor = TestFor::create({}, &ctx, "iter", one, two, three, four);
    testFor.setHasBraceTag(true);

    CHECK_EQ(testFor.toString(), R"(for (iter from 1 to 2 step 3) {
  4
})");
  }
}

TEST_CASE("AST Visotor Test" * doctest::test_suite("ast test suite")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("TestAST1 visitor test") {
    auto one = Integer::create({}, &ctx, 1);
    auto two = Integer::create({}, &ctx, 2);
    auto three = Integer::create({}, &ctx, 3);
    auto four = Integer::create({}, &ctx, 4);
    auto testFor = TestFor::create({}, &ctx, "iter", one, two, three, four);

    std::string result;
    llvm::raw_string_ostream ss(result);
    TestASTVisitor visitor(ss);
    testFor.accept(visitor);

    CHECK_EQ(result, R"(visit TestFor
visit Integer : 1
visit Integer : 2
visit Integer : 3
visit Integer : 4
)");
  }
}

} // namespace ast::test
