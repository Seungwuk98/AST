#ifndef AST_DATA_HANDLER_H
#define AST_DATA_HANDLER_H

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ast {
class AST;
}

namespace ast::detail {
template <typename T, typename Enable = void> struct ASTDataHandler {
  /// static bool isEqual(const T &lhs, const T &rhs);
  /// static void walk(const T &data, std::function<void(AST)>);
};

template <> struct ASTDataHandler<std::string> {
  static bool isEqual(const std::string &lhs, const std::string &rhs) {
    return lhs == rhs;
  }
  static void walk(const std::string &data,
                   const std::function<void(AST)> &fn) {}
};

template <typename T>
struct ASTDataHandler<T, std::enable_if_t<std::disjunction_v<
                             std::is_integral<T>, std::is_floating_point<T>>>> {
  static bool isEqual(T lhs, T rhs) { return lhs == rhs; }
  static void walk(T data, std::function<void(AST)> fn) {}
};

template <typename... Ts> struct ASTDataHandler<std::tuple<Ts...>> {
  using Tuple = std::tuple<Ts...>;

  template <std::size_t... I>
  static bool isEqual(const Tuple &lhs, const Tuple &rhs,
                      std::index_sequence<I...>) {
    return (
        ASTDataHandler<std::remove_cvref_t<std::tuple_element_t<I, Tuple>>>::
            isEqual(std::get<I>(lhs), std::get<I>(rhs)) &&
        ...);
  }

  static bool isEqual(const Tuple &lhs, const Tuple &rhs) {
    return isEqual(lhs, rhs, std::make_index_sequence<sizeof...(Ts)>{});
  }

  static void walk(const Tuple &data, const std::function<void(AST)> &fn) {
    std::apply(
        [&]<typename... Args>(Args &&...args) {
          (ASTDataHandler<std::remove_cvref_t<Args>>::walk(args, fn), ...);
        },
        data);
  }
};

template <typename F, typename S> struct ASTDataHandler<std::pair<F, S>> {
  using Pair = std::pair<F, S>;

  static bool isEqual(const Pair &lhs, const Pair &rhs) {
    return ASTDataHandler<F>::isEqual(lhs.first, rhs.first) &&
           ASTDataHandler<S>::isEqual(lhs.second, rhs.second);
  }

  static void walk(const Pair &data, const std::function<void(AST)> &fn) {
    ASTDataHandler<F>::walk(data.first, fn);
    ASTDataHandler<S>::walk(data.second, fn);
  }
};

template <typename T> struct ASTDataHandler<std::optional<T>> {
  using Optional = std::optional<T>;

  static bool isEqual(const Optional &lhs, const Optional &rhs) {
    if (!lhs && !rhs)
      return true;
    if (lhs && rhs)
      return ASTDataHandler<std::remove_cvref_t<T>>::isEqual(*lhs, *rhs);
    return false;
  };

  static void walk(const Optional &data, const std::function<void(AST)> &fn) {
    if (data)
      ASTDataHandler<std::remove_cvref_t<T>>::walk(*data, fn);
  }
};

template <typename T>
bool vectorIsEqualImpl(llvm::ArrayRef<T> lhs, llvm::ArrayRef<T> rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (const auto &[l, r] : llvm::zip(lhs, rhs)) {
    if (!ASTDataHandler<std::remove_cvref_t<T>>::isEqual(l, r))
      return false;
  }
  return true;
}

template <typename T>
void vectorWalkImpl(llvm::ArrayRef<T> data,
                    const std::function<void(AST)> &fn) {
  for (const auto &elem : data)
    ASTDataHandler<std::remove_cvref_t<T>>::walk(elem, fn);
}

template <typename T> struct ASTDataHandler<std::vector<T>> {
  using Vector = std::vector<T>;

  static bool isEqual(const Vector &lhs, const Vector &rhs) {
    return vectorIsEqualImpl<T>(lhs, rhs);
  }

  static void walk(const Vector &data, const std::function<void(AST)> &fn) {
    vectorWalkImpl<T>(data, fn);
  }
};

template <typename T> struct ASTDataHandler<llvm::SmallVector<T>> {
  using Vector = llvm::SmallVector<T>;

  static bool isEqual(const Vector &lhs, const Vector &rhs) {
    return vectorIsEqualImpl<T>(lhs, rhs);
  }

  static void walk(const Vector &data, const std::function<void(AST)> &fn) {
    vectorWalkImpl<T>(data, fn);
  }
};

template <typename T>
struct ASTDataHandler<T, std::enable_if_t<std::is_base_of_v<AST, T>>> {
  static bool isEqual(const T lhs, const T rhs) { return lhs.isEqual(rhs); }
  static void walk(T data, const std::function<void(AST)> &fn) { fn(data); }
};

} // namespace ast::detail

#endif // AST_DATA_HANDLER_H
