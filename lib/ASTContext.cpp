#include "ast/ASTContext.h"
#include "ast/ASTKindProperty.h"
#include "ast/ASTSet.h"
#include "ast/ASTTypeID.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Allocator.h"

namespace ast {

class ASTContextImpl {
public:
  ~ASTContextImpl() {
    for (auto &[ptr, destructor] : destructors)
      destructor(ptr);
  }

  ASTKindProperty *getASTKindProperty(ID id) {
    auto it = propertiesMap.find(id);
    if (it == propertiesMap.end())
      return nullptr;
    return it->second;
  }

  void registerAST(ID id, ASTKindProperty &&property) {
    auto *newProperty = allocator.Allocate<ASTKindProperty>();
    new (newProperty) ASTKindProperty(std::move(property));
    auto [it, unique] = propertiesMap.try_emplace(id, newProperty);
    (void)unique;
    assert(unique && "ASTKindProperty already registered");
  }

  void *alloc(std::size_t size, std::size_t align, void (*destructor)(void *)) {
    void *ptr = allocator.Allocate(size, align);
    destructors.emplace_back(ptr, destructor);
    return ptr;
  }

  void *getASTSet(ID id, ASTContext *ctx, ASTContext::AllocSetFn fn) {
    auto [it, inserted] = astSetMap.try_emplace(id, nullptr);
    if (!inserted)
      return it->second.get();
    it->second = fn(ctx);
    it->second->RegisterSet();
    return it->second.get();
  }

private:
  llvm::BumpPtrAllocator allocator;
  llvm::DenseMap<ID, ASTKindProperty *> propertiesMap;
  llvm::DenseMap<ID, std::unique_ptr<ASTSet>> astSetMap;

  llvm::SmallVector<std::pair<void *, void (*)(void *)>> destructors;
};

ASTContext::ASTContext() : impl(new ASTContextImpl()) {}
ASTContext::ASTContext(ASTSetRegistry &registry) : impl(new ASTContextImpl()) {}

ASTContext::~ASTContext() { delete impl; }

void ASTContext::RegisterAST(ID id, ASTKindProperty &&property) {
  impl->registerAST(id, std::move(property));
}

void *ASTContext::allocImpl(std::size_t size, std::size_t align,
                            void (*destructor)(void *)) {
  return impl->alloc(size, align, destructor);
}

void *ASTContext::getOrRegisterASTSetImpl(ID id, AllocSetFn fn) {
  return impl->getASTSet(id, this, fn);
}

ASTKindProperty *ASTContext::GetASTKindProperty(ID id) {
  return impl->getASTKindProperty(id);
}

} // namespace ast
