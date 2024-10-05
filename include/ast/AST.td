#ifndef AST_TD
#define AST_TD

class ASTSet {
  string setName;
  string namespace;
}

class AST {
  string namespace;
  string parent = "::ast::AST";

  code extraClassDeclaration;
  code extraClassDefinition;

  dag treeMember;
  dag tag;
}

class DataFormat {
  string paramType;
  string viewType;
}

def String : DataFormat {
  let paramType = "::std::string";
  let viewType = "::llvm::StringRef";
}

class Vector<DataFormat element> : DataFormat {
  let paramType = "::std::vector";
  let viewType = "::llvm::ArrayRef";
  DataFormat elementType = element;
}

class SmallVector<DataFormat element> : DataFormat {
  let paramType = "::llvm::SmallVector";
  let viewType = "::llvm::ArrayRef";
  DataFormat elementType = element;
}

class Optional<DataFormat element> : DataFormat {
  let paramType = "::llvm::Optional";
  DataFormat elementType = element;
}

class Tuple<list<DataFormat> element> : DataFormat {
  let paramType = "::std::tuple";
  list<DataFormat> elementTypes = element;
}

class Pair<DataFormat first, DataFormat second> : DataFormat {
  let paramType = "::std::pair";
  DataFormat firstType = first;
  DataFormat secondType = second;
}

class Variant<list<DataFormat> element> : DataFormat {
  let paramType = "::std::variant";
  list<DataFormat> elementTypes = element;
}

class
    UserDefineType<string paramType_, string viewType_ = paramType_, bit cr = 0>
    : DataFormat {
  let paramType = paramType_;
  let viewType = viewType_;
  bit viewConstRef = cr;
}

def ins;

def SizeT : UserDefineType<"size_t">;
def Bool : UserDefineType<"bool">;
def I8 : UserDefineType<"::std::int8_t">;
def I16 : UserDefineType<"::std::int16_t">;
def I32 : UserDefineType<"::std::int32_t">;
def I64 : UserDefineType<"::std::int64_t">;
def U8 : UserDefineType<"::std::uint8_t">;
def U16 : UserDefineType<"::std::uint16_t">;
def U32 : UserDefineType<"::std::uint32_t">;
def U64 : UserDefineType<"::std::uint64_t">;
def Char : UserDefineType<"char">;
def Short : UserDefineType<"short">;
def Int : UserDefineType<"int">;
def Long : UserDefineType<"long">;
def LongLong : UserDefineType<"long long">;
def UChar : UserDefineType<"unsigned char">;
def UShort : UserDefineType<"unsigned short">;
def UInt : UserDefineType<"unsigned int">;
def ULong : UserDefineType<"unsigned long">;
def ULongLong : UserDefineType<"unsigned long long">;

def Float : UserDefineType<"float">;
def Double : UserDefineType<"double">;
def LongDouble : UserDefineType<"long double">;

#endif // AST_TD
