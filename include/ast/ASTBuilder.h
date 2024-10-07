#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "ast/ASTContext.h"

namespace ast {

class ASTImpl;
struct ASTBuilder {
  template <typename Class, typename... Args>
  static Class create(ASTContext *ctx, Args &&...args) {
    using ImplTy = typename Class::ImplTy;

    auto *kindProperty = ctx->GetASTKindProperty(ID::get<Class>());
    assert(kindProperty && "AST kind property not registered");
    ImplTy *impl;
    if constexpr (std::is_same_v<ASTImpl, ImplTy>) {
      impl = ctx->Alloc<ImplTy>();
    } else {
      impl = ImplTy::create(ctx, std::forward<Args>(args)...);
    }
    impl->setProperty(kindProperty);

    return Class(impl);
  }

  template <typename... Class> static void registerAST(ASTContext *ctx) {
    (ctx->RegisterAST(ID::get<Class>(), ASTKindProperty::get<Class>()), ...);
  }

  template <typename Class>
  static ASTKindProperty *getASTKindProperty(ASTContext *ctx) {
    return ctx->GetASTKindProperty(ID::get<Class>());
  }
};

} // namespace ast

#endif // AST_BUILDER_H
