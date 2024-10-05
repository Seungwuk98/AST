#ifndef AST_TABLEGEN_CXXCOMPONENT_HPP
#define AST_TABLEGEN_CXXCOMPONENT_HPP

#include "CXXType.h"
#include "TableGenContext.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include <variant>

namespace ast::tblgen::cxx {

class ComponentPrinter {
public:
  ComponentPrinter(llvm::raw_ostream &os) : os(os) {}

  llvm::raw_ostream &OS() { return os; }
  llvm::raw_ostream &Line() {
    return os << '\n' << std::string(indentLevel, ' ');
  }
  ComponentPrinter &PrintLine() {
    Line();
    return *this;
  }

  struct IndentAddScope {
    IndentAddScope(ComponentPrinter &printer, std::size_t amount)
        : printer(printer), savedIndent(printer.indentLevel) {
      printer.indentLevel += amount;
    }
    ~IndentAddScope() { printer.indentLevel = savedIndent; }

  private:
    ComponentPrinter &printer;
    std::size_t savedIndent;
  };

  struct DefineScope {
    DefineScope(ComponentPrinter &printer, llvm::StringRef symbol)
        : printer(printer), symbol(symbol) {
      assert(printer.indentLevel == 0);
      printer.OS() << "#ifdef " << symbol << '\n';
    }

    ~DefineScope() {
      printer.OS() << "#undef " << symbol << '\n'
                   << "#endif //" << symbol << '\n';
    }

  private:
    ComponentPrinter &printer;
    llvm::StringRef symbol;
  };

  struct NamespaceScope {
    NamespaceScope(ComponentPrinter &printer, llvm::StringRef symbol)
        : printer(printer), symbol(symbol) {
      assert(printer.indentLevel == 0);
      printer.OS() << "namespace " << symbol << " {\n";
    }

    ~NamespaceScope() { printer.OS() << "\n} // " << symbol << '\n'; }

  private:
    ComponentPrinter &printer;
    llvm::StringRef symbol;
  };

private:
  llvm::raw_ostream &os;
  std::size_t indentLevel = 0;
};

using DeclPair = std::pair<std::string, const Type *>;

namespace detail {
template <typename ConcreteType> struct ComponentBase {
  std::string toString() const {
    std::string result;
    llvm::raw_string_ostream ss(result);
    print(ss);
    return ss.str();
  }
  void print(llvm::raw_ostream &OS) const {
    ComponentPrinter printer(OS);
    derived()->print(printer);
  }
  void print(ComponentPrinter &printer) const { derived()->print(printer); }
  void dump() const { print(llvm::errs()); }

private:
  ConcreteType *derived() { return static_cast<ConcreteType *>(this); }
  const ConcreteType *derived() const {
    return static_cast<const ConcreteType *>(this);
  }
};
} // namespace detail

using BodyCode = llvm::SmallVector<std::string>;
class Using;

void bodyCodePrint(ComponentPrinter &printer, const BodyCode &body);

class Class : public detail::ComponentBase<Class> {
public:
  enum class AccessModifier { Public, Protected, Private };
  class Method : public detail::ComponentBase<Method> {
  public:
    struct InstanceAttribute {
      bool IsConst;
      BodyCode Body;
    };

    struct VirtualAttribute {
      // Pure / Impure Body
      std::variant<std::monostate, std::optional<BodyCode>> BodyOrPure;
    };

    struct OverrideAttribute {
      std::optional<BodyCode> Body;
    };

    struct StaticAttribute {
      std::optional<BodyCode> Body;
    };

    using Attribute =
        std::variant<std::monostate, InstanceAttribute, VirtualAttribute,
                     OverrideAttribute, StaticAttribute>;

    const Type *getReturnType() const { return returnType; }
    llvm::StringRef getName() const { return name; }
    llvm::ArrayRef<DeclPair> getParams() const { return params; }
    const auto &getAttribute() const { return attribute; }

    static Method *create(TableGenContext *context, const Type *returnType,
                          llvm::StringRef name, llvm::ArrayRef<DeclPair> params,
                          const Attribute &attribute) {
      return context->Alloc<Method>(returnType, name, params, attribute);
    }

    void print(ComponentPrinter &printer) const;

  private:
    friend class ast::tblgen::TableGenContext;
    Method(const Type *returnType, llvm::StringRef name,
           llvm::ArrayRef<DeclPair> params, const Attribute &attribute)
        : returnType(returnType), name(name.str()), params(params),
          attribute(attribute) {}

    const Type *returnType;
    std::string name;
    llvm::SmallVector<DeclPair> params;
    Attribute attribute;
  };

  class Constructor : public detail::ComponentBase<Constructor> {
  public:
    struct Implement {
      llvm::SmallVector<std::pair<std::string, std::string>> Initializers;
      BodyCode Body;
    };

    llvm::StringRef getName() const { return name; }
    llvm::ArrayRef<DeclPair> getParams() const { return params; }
    const auto &getImplement() const { return implement; }

    static Constructor *create(TableGenContext *context, llvm::StringRef name,
                               llvm::ArrayRef<DeclPair> params,
                               const std::optional<Implement> &implement) {
      return context->Alloc<Constructor>(name, params, implement);
    }

    void print(ComponentPrinter &printer) const;

  private:
    friend class ast::tblgen::TableGenContext;
    Constructor(llvm::StringRef name, llvm::ArrayRef<DeclPair> params,
                const std::optional<Implement> &implement)
        : name(name.str()), params(params), implement(implement) {}

    std::string name;
    llvm::SmallVector<DeclPair> params;
    std::optional<Implement> implement;
  };

  class Destructor : public detail::ComponentBase<Destructor> {
  public:
    llvm::StringRef getName() const { return name; }
    const std::optional<BodyCode> &getBody() const { return body; }

    static Destructor *create(TableGenContext *context, llvm::StringRef name,
                              const std::optional<BodyCode> &body) {
      return context->Alloc<Destructor>(name, body);
    }

    void print(ComponentPrinter &printer) const;

  private:
    friend class ast::tblgen::TableGenContext;
    Destructor(llvm::StringRef name, const std::optional<BodyCode> &body)
        : name(name.str()), body(body) {}
    std::string name;
    std::optional<BodyCode> body;
  };

  class Field : public detail::ComponentBase<Field> {
  public:
    bool isStatic() const { return staticness; }
    const DeclPair &getDecl() const { return decl; }

    static Field *create(TableGenContext *context, bool staticness,
                         const DeclPair &decl) {
      return context->Alloc<Field>(staticness, decl);
    }

    void print(ComponentPrinter &printer) const;

  private:
    friend class ast::tblgen::TableGenContext;
    Field(bool staticness, const DeclPair &decl)
        : staticness(staticness), decl(decl) {}
    bool staticness;
    DeclPair decl;
  };

  class RawCode : public detail::ComponentBase<RawCode> {
  public:
    llvm::StringRef getCode() const { return code; }

    static RawCode *create(TableGenContext *context, llvm::StringRef code) {
      return context->Alloc<RawCode>(code);
    }

    void print(ComponentPrinter &printer) const;

  private:
    friend class ast::tblgen::TableGenContext;
    RawCode(llvm::StringRef code) : code(code.str()) {}
    std::string code;
  };

  class Friend : public detail::ComponentBase<Friend> {
  public:
    enum class ClassOrStruct : bool { Class, Struct };

    bool isClass() const { return classOrStruct == ClassOrStruct::Class; }
    bool isStruct() const { return classOrStruct == ClassOrStruct::Struct; }
    const Type *getFriendType() const { return friendType; }

    static Friend *create(TableGenContext *context, const Type *friendType,
                          ClassOrStruct classOrStruct = ClassOrStruct::Class) {
      return context->Alloc<Friend>(friendType, classOrStruct);
    }

    void print(ComponentPrinter &printer) const;

  private:
    friend class ast::tblgen::TableGenContext;
    Friend(const Type *friendType, ClassOrStruct classOrStruct)
        : friendType(friendType), classOrStruct(classOrStruct) {}
    const Type *friendType;
    ClassOrStruct classOrStruct;
  };

  using ClassMember = std::variant<Method *, Constructor *, Destructor *,
                                   Field *, RawCode *, Friend *, Using *>;

  struct Block {
    AccessModifier Access;
    llvm::SmallVector<ClassMember> Members;
  };

  struct ClassImplementation {
    llvm::SmallVector<Block> Blocks;
    llvm::SmallVector<std::pair<bool, const Type *>> Bases;
  };

  enum class StructOrClass : bool { Struct, Class };

  bool isStruct() const { return structOrClass == StructOrClass::Struct; }
  bool isClass() const { return structOrClass == StructOrClass::Class; }
  llvm::StringRef getName() const { return name; }
  const auto &getImplementation() const { return implementation; }

  static Class *
  create(TableGenContext *context, StructOrClass structOrClass,
         llvm::StringRef name,
         const std::optional<ClassImplementation> &implementation) {
    return context->Alloc<Class>(structOrClass, name, implementation);
  }

  void print(ComponentPrinter &printer) const;

private:
  friend class ast::tblgen::TableGenContext;
  Class(StructOrClass structOrClass, llvm::StringRef name,
        const std::optional<ClassImplementation> &implementation)
      : structOrClass(structOrClass), name(name.str()),
        implementation(implementation) {}
  StructOrClass structOrClass;
  std::string name;
  std::optional<ClassImplementation> implementation;
};

class VarDecl : public detail::ComponentBase<VarDecl> {
public:
  enum class Access { None, Static, Extern };

  Access getAccess() const { return accessness; }
  const DeclPair &getDecl() const { return decl; }

  static VarDecl *create(TableGenContext *context, Access accessness,
                         const DeclPair &decl) {
    return context->Alloc<VarDecl>(accessness, decl);
  }

  void print(ComponentPrinter &printer) const;

private:
  friend class ast::tblgen::TableGenContext;
  VarDecl(Access accessness, const DeclPair &decl)
      : accessness(accessness), decl(decl) {}
  Access accessness;
  DeclPair decl;
};

class Function : public detail::ComponentBase<Function> {
public:
  enum class Access { None, Extern, Static, Inline };

  const std::optional<std::string> &getAttribute() const { return attribute; }
  Access getAccess() const { return accessness; }
  Type *getReturnType() const { return returnType; }
  const auto &getNamespaces() const { return namespaces; }
  llvm::StringRef getName() const { return name; }
  llvm::ArrayRef<DeclPair> getParams() const { return params; }
  const std::optional<BodyCode> &getBody() const { return body; }

  static Function *
  create(TableGenContext *context, const std::optional<std::string> &attributes,
         Access accessness, Type *returnType,
         const std::optional<llvm::SmallVector<std::string>> &namespaces,
         llvm::StringRef name, llvm::ArrayRef<DeclPair> params,
         const std::optional<BodyCode> &body) {
    return context->Alloc<Function>(attributes, accessness, returnType,
                                    namespaces, name, params, body);
  }

  void print(ComponentPrinter &printer) const;

private:
  friend class ast::tblgen::TableGenContext;
  Function(const std::optional<std::string> &attribute, Access accessness,
           Type *returnType,
           const std::optional<llvm::SmallVector<std::string>> &namespaces,
           llvm::StringRef name, llvm::ArrayRef<DeclPair> params,
           const std::optional<BodyCode> &body)
      : attribute(attribute), accessness(accessness), returnType(returnType),
        namespaces(namespaces), name(name.str()), params(params), body(body) {}
  std::optional<std::string> attribute;
  Access accessness;
  Type *returnType;
  std::optional<llvm::SmallVector<std::string>> namespaces;
  std::string name;
  llvm::SmallVector<DeclPair> params;
  std::optional<BodyCode> body;
};

class ClassConstructor : public detail::ComponentBase<ClassConstructor> {
public:
  llvm::ArrayRef<std::string> getNamespaces() const { return namespaces; }
  llvm::StringRef getName() const { return name; }
  llvm::ArrayRef<DeclPair> getParams() const { return params; }
  const auto &getBody() const { return body; }

  static ClassConstructor *create(TableGenContext *context,
                                  llvm::ArrayRef<std::string> namespaces,
                                  llvm::StringRef name,
                                  llvm::ArrayRef<DeclPair> params,
                                  Class::Constructor::Implement body) {
    return context->Alloc<ClassConstructor>(namespaces, name, params, body);
  }

  void print(ComponentPrinter &printer) const;

private:
  friend class ast::tblgen::TableGenContext;
  ClassConstructor(llvm::ArrayRef<std::string> namespaces, llvm::StringRef name,
                   llvm::ArrayRef<DeclPair> params,
                   const Class::Constructor::Implement &body)
      : namespaces(namespaces), name(name.str()), params(params), body(body) {}

  llvm::SmallVector<std::string> namespaces;
  std::string name;
  llvm::SmallVector<DeclPair> params;
  Class::Constructor::Implement body;
};

class ClassDestructor : public detail::ComponentBase<ClassDestructor> {
public:
  llvm::ArrayRef<std::string> getNamespaces() const { return namespaces; }
  llvm::StringRef getName() const { return name; }
  const auto &getBody() const { return body; }

  static ClassDestructor *create(TableGenContext *context,
                                 llvm::ArrayRef<std::string> namespaces,
                                 llvm::StringRef name, const BodyCode &body) {
    return context->Alloc<ClassDestructor>(namespaces, name, body);
  }

  void print(ComponentPrinter &printer) const;

private:
  friend class ast::tblgen::TableGenContext;
  ClassDestructor(llvm::ArrayRef<std::string> namespaces, llvm::StringRef name,
                  const BodyCode &body)
      : namespaces(namespaces), name(name.str()), body(body) {}
  llvm::SmallVector<std::string> namespaces;
  std::string name;
  BodyCode body;
};

class Template : public detail::ComponentBase<Template> {
public:
  struct VariadicParameter {
    std::optional<Type *> ParamType;
    std::string ParamName;
  };

  struct NonVariadicParameter {
    std::optional<Type *> ParamType;
    std::string ParamName;
  };

  using Parameter = std::variant<VariadicParameter, NonVariadicParameter>;

private:
  friend class ast::tblgen::TableGenContext;

  llvm::SmallVector<Parameter> parameters;
};

class Using : public detail::ComponentBase<Using> {
public:
  struct Alias {
    std::string AliasName;
    Type *Type;
  };

  struct Member {
    llvm::SmallVector<std::string> Namespaces;
    std::string Name;
  };

  using Body = std::variant<Alias, Member>;

  const Alias *getAlias() const {
    if (auto *alias = std::get_if<Alias>(&usingBody))
      return alias;
    return nullptr;
  }

  const Member *getMember() const {
    if (auto *member = std::get_if<Member>(&usingBody))
      return member;
    return nullptr;
  }

  static Using *create(TableGenContext *context, const Body &alias) {
    return context->Alloc<Using>(alias);
  }

  void print(ComponentPrinter &printer) const;

private:
  friend class ast::tblgen::TableGenContext;
  Using(const std::variant<Alias, Member> &usingBody) : usingBody(usingBody) {}
  Body usingBody;
};

} // namespace ast::tblgen::cxx

#endif // AST_TABLEGEN_CXXCOMPONENT_HPP
