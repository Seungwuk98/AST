#include "ast/ASTWalker.h"
#include "ast/AST.h"

namespace ast {

WalkResult ASTWalker::Walk(AST ast) {
  if (auto iter = visited.find(ast.getImplAsVoidPointer());
      iter != visited.end()) {
    return iter->second;
  }

  if (order == WalkOrder::PostOrder) {
    auto childrenResult = walkChildren(ast);
    if (childrenResult.isInterrupt()) {
      visited.try_emplace(ast.getImplAsVoidPointer(), WalkResult::interrupt());
      return WalkResult::interrupt();
    }
  }

  for (const auto &fn : functions) {
    auto result = fn(ast);
    if (result.isInterrupt()) {
      visited.try_emplace(ast.getImplAsVoidPointer(), WalkResult::interrupt());
      return WalkResult::interrupt();
    }
    if (result.isSkip()) {
      visited.try_emplace(ast.getImplAsVoidPointer(), WalkResult::skip());
      return WalkResult::skip();
    }
  }

  if (order == WalkOrder::PreOrder) {
    auto childrenResult = walkChildren(ast);
    if (childrenResult.isInterrupt()) {
      visited.try_emplace(ast.getImplAsVoidPointer(), WalkResult::interrupt());
      return WalkResult::interrupt();
    }
  }

  visited.try_emplace(ast.getImplAsVoidPointer(), WalkResult::success());
  return WalkResult::success();
}

} // namespace ast
