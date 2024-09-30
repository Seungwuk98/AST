#ifndef AST_CONTEXT_H
#define AST_CONTEXT_H

#include "ast/ASTKindProperty.h"
#include <memory>

namespace ast {

class ASTSet;
class ASTContextImpl;
class ASTSetRegistry;

class ASTContext {
public:
  using AllocSetFn = std::unique_ptr<ASTSet> (*)(ASTContext *);

  ASTContext();
  ASTContext(ASTSetRegistry &registry);
  ~ASTContext();

  ASTContext(const ASTContext &) = delete;
  ASTContext &operator=(const ASTContext &) = delete;
  ASTContext(ASTContext &&) = delete;
  ASTContext &operator=(ASTContext &&) = delete;

  void RegisterAST(ID id, ASTKindProperty &&property);

  template <typename Class, typename... Args> Class *Alloc(Args &&...args);

  ASTKindProperty *GetASTKindProperty(ID id);

  template <typename Set> ASTSet *GetOrRegisterASTSet();

private:
  void *allocImpl(std::size_t size, std::size_t align,
                  void (*destructor)(void *));

  void *getOrRegisterASTSetImpl(ID id, AllocSetFn fn);

  ASTContextImpl *impl;
};

template <typename Class, typename... Args>
Class *ASTContext::Alloc(Args &&...args) {
  void *ptr = allocImpl(
      sizeof(Class), alignof(Class),
      +[](void *ptr) { static_cast<Class *>(ptr)->~Class(); });
  return new (ptr) Class(std::forward<Args>(args)...);
}

template <typename Set> ASTSet *ASTContext::GetOrRegisterASTSet() {
  void *set = getOrRegisterASTSetImpl(
      ID::get<Set>(), +[](ASTContext *ctx) -> std::unique_ptr<ASTSet> {
        return std::make_unique<Set>(ctx);
      });
  return static_cast<Set *>(set);
}

} // namespace ast

#endif // AST_CONTEXT_H
