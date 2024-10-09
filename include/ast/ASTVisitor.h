#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include "ast/AST.h"
#include <functional>
#include <type_traits>

namespace ast {

class Visitor {
public:
  void visit(AST ast) { visitFn(ast, *this); }

protected:
  Visitor(const std::function<void(AST, Visitor &)> &visitFn)
      : visitFn(visitFn) {}

private:
  std::function<void(AST, Visitor &)> visitFn;
};

#define VISITOR_VTBL_THRESHOLD 100 // 100 is an arbitrary number

template <typename ConcreteType, typename Enable = void, typename... VisitASTs>
class VisitorBaseImpl;

template <typename ConcreteType, typename... VisitASTs>
class VisitorBaseImpl<
    ConcreteType,
    std::enable_if_t<(sizeof...(VisitASTs) > VISITOR_VTBL_THRESHOLD)>,
    VisitASTs...> : public Visitor {
protected:
  using Visitor::Visitor;

  void visitImpl(AST ast, ConcreteType &visitor) {
    static llvm::DenseMap<ID, void *> vtbl = {
        {ID::get<VisitASTs>(),
         reinterpret_cast<void *>(+[](AST ast, ConcreteType &visitor) {
           visitor.visit(ast.cast<VisitASTs>());
         })}...};

    if (auto iter = vtbl.find(ast.getID()); iter != vtbl.end()) {
      auto visitFn =
          reinterpret_cast<void (*)(AST, ConcreteType &)>(iter->second);
      visitFn(ast, visitor);
    } else
      llvm_unreachable("No visit method for the given AST");
  }
};

template <typename ConcreteType, typename... VisitASTs>
class VisitorBaseImpl<
    ConcreteType,
    std::enable_if_t<(sizeof...(VisitASTs) <= VISITOR_VTBL_THRESHOLD)>,
    VisitASTs...> : public Visitor {
protected:
  using Visitor::Visitor;

  void visitImpl(AST ast, ConcreteType &visitor) {
    visitImplHelp<VisitASTs...>(ast, visitor);
  }

  template <typename VisitAST, typename... Others>
  void visitImplHelp(AST ast, ConcreteType &visitor) {
    if (auto visitAST = ast.dyn_cast<VisitAST>())
      visitor.visit(visitAST);
    else {
      if constexpr (sizeof...(Others) > 0)
        visitImplHelp<Others...>(ast, visitor);
      else
        llvm_unreachable("No visit method for the given AST");
    }
  }
};

template <typename ConcreteType, typename... VisitASTs>
class VisitorBase : public VisitorBaseImpl<ConcreteType, void, VisitASTs...> {
  using Impl = VisitorBaseImpl<ConcreteType, void, VisitASTs...>;

public:
  VisitorBase() : Impl(createVisitorFn()) {}

private:
  auto createVisitorFn() {
    return [this](AST ast, Visitor &visitor) {
      auto &concreteVisitor = static_cast<ConcreteType &>(visitor);
      Impl::visitImpl(ast, concreteVisitor);
    };
  }
};

} // namespace ast

#endif // AST_VISITOR_H
