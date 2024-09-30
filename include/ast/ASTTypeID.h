#ifndef AST_TYPE_ID_H
#define AST_TYPE_ID_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include <cstdint>

namespace ast {

class ID {
public:
  ID() : value(0) {}
  ID(intptr_t value) : value(value) {}

  static ID getFromVoidPointer(const void *ptr) {
    return ID(reinterpret_cast<intptr_t>(ptr));
  }

  operator bool() const { return value != 0; }
  bool operator==(ID other) const { return value == other.value; }
  bool operator!=(ID other) const { return value != other.value; }

  intptr_t getValue() const { return value; }

  template <typename Class> static ID get();

  friend ::llvm::hash_code hash_value(ID id) {
    return ::llvm::hash_value(id.value);
  }

private:
  intptr_t value;
};

class alignas(8) SelfID {
public:
  SelfID() = default;
  SelfID(const SelfID &) = delete;
  SelfID &operator=(const SelfID &) = delete;
  SelfID(SelfID &&) = delete;
  SelfID &operator=(SelfID &&) = delete;

  ID getID() { return ID::getFromVoidPointer(this); }
};

namespace detail {
template <typename T> class GetIDHelper {};
} // namespace detail

template <typename Class> ID ID::get() {
  return detail::GetIDHelper<Class>::get();
}
} // namespace ast

#define DECLARE_TYPE_ID(Class)                                                 \
  namespace ast::detail {                                                      \
  template <> class GetIDHelper<Class> {                                       \
  public:                                                                      \
    static ast::ID get() { return id.getID(); }                                \
                                                                               \
  private:                                                                     \
    static SelfID id;                                                          \
  };                                                                           \
  }

#define DEFINE_TYPE_ID(Class)                                                  \
  namespace ast::detail {                                                      \
  SelfID GetIDHelper<Class>::id{};                                             \
  }

namespace llvm {
template <> struct DenseMapInfo<ast::ID> {
  static ast::ID getEmptyKey() { return DenseMapInfo<intptr_t>::getEmptyKey(); }
  static ast::ID getTombstoneKey() {
    return DenseMapInfo<intptr_t>::getTombstoneKey();
  }
  static unsigned getHashValue(ast::ID id) {
    return DenseMapInfo<intptr_t>::getHashValue(id.getValue());
  }
  static bool isEqual(ast::ID lhs, ast::ID rhs) { return lhs == rhs; }
};
} // namespace llvm
#endif // AST_TYPE_ID_H
