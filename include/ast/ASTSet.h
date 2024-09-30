#ifndef AST_SET_H
#define AST_SET_H

#include "ast/ASTContext.h"
#include "llvm/ADT/StringRef.h"

namespace ast {

class ASTSet {
public:
  ASTSet(ASTContext *ctx) : ctx(ctx) {}

  virtual ~ASTSet() = default;

  virtual void RegisterSet() = 0;

  virtual llvm::StringRef getASTSetName() const = 0;

  ASTContext *getContext() const { return ctx; }

private:
  ASTContext *ctx;
};

} // namespace ast

#endif // AST_SET_H
