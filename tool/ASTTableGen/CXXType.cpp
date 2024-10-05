#include "CXXType.h"

namespace ast::tblgen::cxx {

std::string RawType::toString() const { return str; }

std::string ConstType::toString() const { return "const " + type->toString(); }

std::string PointerType::toString() const { return pointee->toString() + " *"; }

std::string ReferenceType::toString() const {
  return referencedType->toString() + " &";
}

const Type *createConstReferenceType(TableGenContext *context,
                                     const Type *type) {
  return ConstType::create(context, ReferenceType::create(context, type));
}

} // namespace ast::tblgen::cxx
