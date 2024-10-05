#ifndef AST_TABLEGEN_CXXTYPE_HPP
#define AST_TABLEGEN_CXXTYPE_HPP

#include "TableGenContext.h"
#include "llvm/ADT/StringRef.h"
#include <string>

namespace ast::tblgen::cxx {

class Type {
public:
  enum class Kind {
    RawType,
    ConstType,
    Pointer,
    Reference,
  };

  Kind getKind() const { return kind; }
  virtual std::string toString() const = 0;

protected:
  Type(Kind kind) : kind(kind) {}

private:
  Kind kind;
};

class RawType : public Type {
public:
  llvm::StringRef getStr() const { return str; }
  const std::optional<llvm::SmallVector<const Type *>> &getTypeArgs() const {
    return typeArgs;
  }

  static RawType *
  create(TableGenContext *context, llvm::StringRef str,
         const std::optional<llvm::SmallVector<const Type *>> &templateArgs) {
    return context->Alloc<RawType>(str, templateArgs);
  }

  static bool classof(const Type *type) {
    return type->getKind() == Kind::RawType;
  }

  std::string toString() const override;

private:
  friend class ::ast::tblgen::TableGenContext;
  RawType(llvm::StringRef str,
          const std::optional<llvm::SmallVector<const Type *>> &typeArgs)
      : Type(Kind::RawType), str(str), typeArgs(typeArgs) {}

  std::string str;
  std::optional<llvm::SmallVector<const Type *>> typeArgs;
};

class ConstType : public Type {
public:
  const Type *getType() const { return type; }

  static ConstType *create(TableGenContext *context, const Type *type) {
    return context->Alloc<ConstType>(type);
  }

  static bool classof(const Type *type) {
    return type->getKind() == Kind::ConstType;
  }

  std::string toString() const override;

private:
  friend class ::ast::tblgen::TableGenContext;
  ConstType(const Type *type) : Type(Kind::ConstType), type(type) {}
  const Type *type;
};

class PointerType : public Type {
public:
  const Type *getPointee() const { return pointee; }

  static PointerType *create(TableGenContext *context, const Type *pointee) {
    return context->Alloc<PointerType>(pointee);
  }

  static bool classof(const Type *type) {
    return type->getKind() == Kind::Pointer;
  }

  std::string toString() const override;

private:
  friend class ::ast::tblgen::TableGenContext;
  PointerType(const Type *pointee) : Type(Kind::Pointer), pointee(pointee) {}
  const Type *pointee;
};

class ReferenceType : public Type {
public:
  const Type *getReferencedType() const { return referencedType; }

  static ReferenceType *create(TableGenContext *context,
                               const Type *referencedType) {
    return context->Alloc<ReferenceType>(referencedType);
  }

  static bool classof(const Type *type) {
    return type->getKind() == Kind::Reference;
  }

  std::string toString() const override;

private:
  friend class ::ast::tblgen::TableGenContext;
  ReferenceType(const Type *referencedType)
      : Type(Kind::Reference), referencedType(referencedType) {}

  const Type *referencedType;
};

const Type *createConstReferenceType(TableGenContext *context,
                                     const Type *type);

} // namespace ast::tblgen::cxx

#endif // AST_TABLEGEN_CXXTYPE_HPP
