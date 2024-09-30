#ifndef AST_KIND_PROPERTY_H
#define AST_KIND_PROPERTY_H

#include "ast/ASTPrinter.h"
#include "ast/ASTTypeID.h"
#include <functional>

namespace ast {

class AST;
class ASTBuilder;
class ASTKindProperty {
public:
  using ChildrenWalkFn = std::function<void(AST, std::function<void(AST)>)>;
  using EqualFn = std::function<bool(AST, AST)>;
  using PrintFn = std::function<void(AST, ASTPrinter &)>;

  ID getID() const { return id; }

  const auto &getChildrenWalkFn() const { return childrenWalkFn; }
  const auto &getEqualFn() const { return equalFn; }
  const auto &getPrintFn() const { return printFn; }

private:
  friend class ::ast::ASTBuilder;

  template <typename Class> static ASTKindProperty get() {
    return ASTKindProperty(ID::get<Class>(), Class::getChildrenWalkFn(),
                           Class::getEqualFn(), Class::getPrintFn());
  }

  ASTKindProperty(ID id, ChildrenWalkFn childrenWalkFn, EqualFn equalFn,
                  PrintFn printFn)
      : id(id), childrenWalkFn(std::move(childrenWalkFn)),
        equalFn(std::move(equalFn)), printFn(std::move(printFn)) {}

  const ID id;
  const ChildrenWalkFn childrenWalkFn;
  const EqualFn equalFn;
  const PrintFn printFn;
};

} // namespace ast

#endif // AST_KIND_PROPERTY_H
