#ifndef AST_WALKER_H
#define AST_WALKER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include <functional>

namespace ast {

class AST;

class WalkResult {
public:
  enum class Action {
    Success,
    Skip,
    Interrupt,
  };

  constexpr WalkResult() : action(Action::Success) {}
  constexpr WalkResult(Action action) : action(action) {}

  static constexpr WalkResult success() { return WalkResult(Action::Success); }
  static constexpr WalkResult skip() { return WalkResult(Action::Skip); }
  static constexpr WalkResult interrupt() {
    return WalkResult(Action::Interrupt);
  }

  constexpr bool isSuccess() const { return action == Action::Success; }
  constexpr bool isSkip() const { return action == Action::Skip; }
  constexpr bool isInterrupt() const { return action == Action::Interrupt; }

private:
  Action action;
};

enum class WalkOrder {
  PreOrder,
  PostOrder,
};

class ASTWalker {
public:
  ASTWalker(WalkOrder order) : order(order) {}

  template <typename Fn> void addFn(Fn &&fn) {
    functions.emplace_back(std::forward<Fn>(fn));
  }

  WalkResult Walk(AST ast);

private:
  WalkResult walkChildren(AST ast);

  WalkOrder order;
  llvm::SmallVector<std::function<WalkResult(AST)>> functions;
  llvm::DenseMap<void *, WalkResult> visited;
};

} // namespace ast

#endif // AST_WALKER_H
