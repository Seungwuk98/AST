#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestAST.h"
#include "doctest/doctest.h"

TEST_SUITE("AST_TEST_SUITE") {}

namespace ast::test {

TEST_CASE("AST Creation Test" * doctest::test_suite("AST_TEST_SUITE")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("TestAST1 test") {
    auto testAST1 = TestAST1::create(&ctx, 1, 2);
    CHECK_EQ(testAST1.getValue1(), 1);
    CHECK_EQ(testAST1.getValue2(), 2);

    CHECK_EQ(testAST1.toString(), "TestAST1(1, 2)");
  }

  SUBCASE("TestIf test") {
    auto testAST1 = TestAST1::create(&ctx, 1, 2);
    auto testAST2 = TestAST1::create(&ctx, 3, 4);
    auto testAST3 = TestAST1::create(&ctx, 5, 6);
    auto testIf = TestIf::create(&ctx, testAST1, testAST2, testAST3);

    CHECK_EQ(testIf.toString(), R"(TestIf(TestAST1(1, 2)) {
  TestAST1(3, 4)
} else {
  TestAST1(5, 6)
})");
  }
}

TEST_CASE("AST Equality Test" * doctest::test_suite("AST_TEST_SUITE")) {
  ASTContext ctx;
  ctx.GetOrRegisterASTSet<TestASTSet>();

  SUBCASE("TestAST1 equality test") {
    auto testAST1 = TestAST1::create(&ctx, 1, 2);
    auto testAST2 = TestAST1::create(&ctx, 1, 2);
    auto testAST3 = TestAST1::create(&ctx, 3, 4);

    CHECK(testAST1.isEqual(testAST2));
    CHECK_FALSE(testAST1.isEqual(testAST3));
  }

  SUBCASE("TestIf equality test") {
    auto testAST1 = TestAST1::create(&ctx, 1, 2);
    auto testAST2 = TestAST1::create(&ctx, 3, 4);
    auto testAST3 = TestAST1::create(&ctx, 5, 6);
    auto testIf1 = TestIf::create(&ctx, testAST1, testAST2, testAST3);
    auto testIf2 = TestIf::create(&ctx, testAST1, testAST2, testAST3);
    auto testIf3 = TestIf::create(&ctx, testAST1, testAST3, testAST2);

    CHECK(testIf1.isEqual(testIf2));
    CHECK_FALSE(testIf1.isEqual(testIf3));
  }
}

} // namespace ast::test
