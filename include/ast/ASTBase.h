#ifndef AST_BASE_H
#define AST_BASE_H

#include "ast/ASTBuilder.h"
#include "ast/ASTContext.h"
#include "ast/ASTDataHandler.h"
#include "ast/ASTPrinter.h"
#include "ast/ASTTypeID.h"
#include <type_traits>

namespace ast {
class AST;

template <typename ConcreteType, typename ParentType, typename ImplType,
          typename BaseType = AST>
class ASTBase : public ParentType {
public:
  using ParentType::ParentType;
  using Base = ASTBase<ConcreteType, ParentType, ImplType, BaseType>;
  using ImplTy = ImplType;

  ImplTy *getImpl() const { return static_cast<ImplTy *>(BaseType::getImpl()); }

  template <typename... Args>
  static ConcreteType create(ASTContext *ctx, Args &&...args) {
    return ASTBuilder::create<ConcreteType>(ctx, std::forward<Args>(args)...);
  }

  template <typename T>
    requires std::is_convertible_v<T, AST>
  static bool classof(const T ast) {
    return ast.getID() == ID::get<ConcreteType>();
  }

  static const auto getChildrenWalkFn() {
    return [](BaseType ast, std::function<void(AST)> fn) {
      auto concreteAST = ast.template cast<ConcreteType>();
      const auto &traversalData = concreteAST.traversalOrder();
      detail::ASTDataHandler<
          std::remove_cvref_t<decltype(traversalData)>>::walk(traversalData,
                                                              fn);
    };
  }

  static const auto getEqualFn() {
    return [](BaseType left, BaseType right) {
      if (left.getID() != right.getID())
        return false;
      auto leftConcrete = left.template cast<ConcreteType>();
      auto leftMember = leftConcrete.traversalOrder();
      auto rightConcrete = right.template cast<ConcreteType>();
      auto rightMember = rightConcrete.traversalOrder();
      return detail::ASTDataHandler<
          std::remove_cvref_t<decltype(leftMember)>>::isEqual(leftMember,
                                                              rightMember);
    };
  }

  static const auto getPrintFn() {
    return [](BaseType ast, ASTPrinter &printer) {
      ConcreteType::print(ast.template cast<ConcreteType>(), printer);
    };
  }

private:
};

} // namespace ast

#endif // AST_BASE_H
