#ifndef AST_TABLEGEN_CONTEXT_H
#define AST_TABLEGEN_CONTEXT_H

#include "llvm/Support/Allocator.h"

namespace ast::tblgen {

class TableGenContext {
public:
  TableGenContext() = default;
  ~TableGenContext() {
    for (auto &[ptr, destructor] : destructors)
      destructor(ptr);
  }

  template <typename T, typename... Args> T *Alloc(Args &&...args) {
    auto *ptr = allocator.Allocate<T>();
    new (ptr) T(std::forward<Args>(args)...);
    destructors.emplace_back(
        ptr, +[](void *ptr) { static_cast<T *>(ptr)->~T(); });
    return static_cast<T *>(ptr);
  }

private:
  llvm::BumpPtrAllocator allocator;
  llvm::SmallVector<std::pair<void *, void (*)(void *)>> destructors;
};

} // namespace ast::tblgen

#endif // AST_TABLEGEN_CONTEXT_H
