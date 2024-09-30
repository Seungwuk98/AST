#ifndef AST_H
#define AST_H

#include "ast/ASTBase.h"
#include "ast/ASTKindProperty.h"
#include "ast/ASTPrinter.h"
#include "llvm/Support/Casting.h"

namespace ast {
class ASTBuilder;

class ASTImpl {
public:
  ASTKindProperty *getProperty() const { return property; }

private:
  friend class ::ast::ASTBuilder;
  void setProperty(ASTKindProperty *property) { this->property = property; }

  ASTKindProperty *property{nullptr};
};

class AST {
public:
  template <typename ConcreteType, typename ParentType, typename ImplType>
  using Base = ASTBase<ConcreteType, ParentType, ImplType, AST>;

  AST() : impl(nullptr) {}
  AST(const ASTImpl *impl) : impl(const_cast<ASTImpl *>(impl)) {}

  bool operator==(const AST &other) const { return impl == other.impl; }
  bool operator!=(const AST &other) const { return impl != other.impl; }
  operator bool() const { return impl != nullptr; }

  ASTImpl *getImpl() const { return impl; }
  void *getImplAsVoidPointer() const { return static_cast<void *>(impl); }

  template <typename U> bool isa() const { return llvm::isa<U>(*this); }
  template <typename U> U cast() const { return llvm::cast<U>(*this); }
  template <typename U> U cast_if_present() const {
    return llvm::cast_if_present<U>(*this);
  }
  template <typename U> U dyn_cast() const { return llvm::dyn_cast<U>(*this); }
  template <typename U> U dyn_cast_if_present() const {
    return llvm::dyn_cast_if_present<U>(*this);
  }

  ASTKindProperty &getASTKindProperty() const { return *impl->getProperty(); }

  ID getID() const { return impl->getProperty()->getID(); }

  template <typename Fn> void walkChildren(Fn &&fn) const {
    getASTKindProperty().getChildrenWalkFn()(*this, std::forward<Fn>(fn));
  }

  bool isEqual(const AST other) const {
    return getASTKindProperty().getEqualFn()(*this, other);
  }

  std::string toString() const;
  void print(llvm::raw_ostream &os) const;
  void print(ASTPrinter &printer) const;
  void dump() const;

private:
  ASTImpl *impl;
};

} // namespace ast

namespace llvm {

template <typename To, typename From>
struct CastInfo<
    To, From,
    std::enable_if_t<std::is_same_v<ast::AST, std::remove_const_t<From>> ||
                     std::is_base_of_v<ast::AST, From>>>
    : NullableValueCastFailed<To>,
      DefaultDoCastIfPossible<To, From, CastInfo<To, From>> {
  static inline bool isPossible(ast::AST ast) {
    if constexpr (std::is_base_of_v<To, From>) {
      (void)ast;
      return true;
    } else {
      return To::classof(ast);
    }
  }

  static inline To doCast(ast::AST ast) { return To(ast.getImpl()); }
};

} // namespace llvm

#endif // AST_H
