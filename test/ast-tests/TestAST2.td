#ifndef TEST_AST2_TD
#define TEST_AST2_TD

include "ast/AST.td"

def TestASTSet_Integer : AST {
  let namespace = "ast::test";

  let treeMember = (ins U64 : $value);
}

def TestASTSet_TestFor : AST {
  let namespace = "ast::test";

  let treeMember = (ins String
                    : $iterName, ASTType
                    : $fromE, ASTType
                    : $toE, ASTType
                    : $stepE, ASTType
                    : $bodyE);
  let tag = (ins Bool : $hasBrace);
}

#endif // TEST_AST2_ID
