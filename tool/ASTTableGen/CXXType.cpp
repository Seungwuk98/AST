#include "CXXType.h"
#include "llvm/Support/raw_ostream.h"

namespace ast::tblgen::cxx {

std::string RawType::toString() const {
  std::string result = str;
  llvm::raw_string_ostream ss(result);
  if (typeArgs) {
    ss << '<';
    for (auto I = typeArgs->begin(), E = typeArgs->end(); I != E; ++I) {
      if (I != typeArgs->begin())
        ss << ", ";
      ss << (*I)->toString();
    }
    ss << '>';
  }
  return result;
}

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
