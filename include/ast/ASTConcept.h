#ifndef AST_CONCEPT_H
#define AST_CONCEPT_H

namespace ast {

template <typename T>
concept HasTraversalOrder = requires(T obj) {
  { obj.traversalOrder() };
};

} // namespace ast

#endif // AST_CONCEPT_H
