#include "CXXComponent.h"
#include <variant>

namespace ast::tblgen::cxx {

void bodyCodePrint(ComponentPrinter &printer, const BodyCode &body) {
  printer.OS() << '{';
  if (body.size() < 2) {
    if (!body.empty())
      printer.OS() << ' ' << body[0] << ' ';
  } else {
    {
      ComponentPrinter::IndentAddScope indent(printer, 2);
      for (const auto &stmt : body)
        printer.Line() << stmt;
    }
    printer.Line();
  }
  printer.OS() << '}';
}

void Class::print(ComponentPrinter &printer) const {
  printer.OS() << (isClass() ? "class " : "struct ") << getName();
  if (getImplementation()) {
    const auto &impl = *getImplementation();
    printer.OS() << " : ";
    for (const auto &[inheritAccess, inheritClass] : impl.Bases) {
      if (inheritAccess)
        printer.OS() << "public ";
      printer.OS() << inheritClass->toString();
    }

    printer.OS() << " {";

    for (const auto &[access, members] : impl.Blocks) {
      switch (access) {
      case AccessModifier::Public:
        printer.Line() << "public:";
        break;
      case AccessModifier::Protected:
        printer.Line() << "protected:";
        break;
      case AccessModifier::Private:
        printer.Line() << "private:";
        break;
      }
      ComponentPrinter::IndentAddScope indent(printer, 2);
      for (const auto &member : members)
        std::visit([&printer](const auto &member) { member->print(printer); },
                   member);
    }

    (impl.Blocks.empty() ? printer.Line() : printer.OS()) << '}';
  }
  printer.OS() << ';';
}

void Class::Method::print(ComponentPrinter &printer) const {
  auto printFunction = [&]() {
    printer.OS() << getReturnType()->toString() << ' ' << getName() << '(';
    for (auto I = getParams().begin(), E = getParams().end(); I != E;) {
      if (I != getParams().begin())
        printer.OS() << ", ";
      const auto &[paramName, paramType] = *I;
      printer.OS() << paramType->toString() << ' ' << paramName;
    }
    printer.OS() << ')';
  };

  std::visit(
      [&]<typename T>(const T &attr) {
        if constexpr (std::is_same_v<T, InstanceAttribute>) {
          printFunction();
          if (attr.IsConst)
            printer.OS() << " const";
          bodyCodePrint(printer, attr.Body);
        } else if constexpr (std::is_same_v<T, VirtualAttribute>) {
          printer.OS() << "virtual ";
          printFunction();
          if (const auto &bodyCode =
                  std::get<std::optional<BodyCode>>(attr.BodyOrPure)) {
            if (bodyCode)
              bodyCodePrint(printer, *bodyCode);
            else
              printer.OS() << ';';
          } else
            printer.OS() << " = 0;";
        } else if constexpr (std::is_same_v<T, OverrideAttribute>) {
          printFunction();
          printer.OS() << " override";
          if (attr.Body)
            bodyCodePrint(printer, *attr.Body);
          else
            printer.OS() << ';';
        } else if constexpr (std::is_same_v<T, StaticAttribute>) {
          printer.OS() << "static ";
          printFunction();
          if (attr.Body)
            bodyCodePrint(printer, *attr.Body);
          else
            printer.OS() << ';';
        } else /* monostate -> Declaration */ {
          printFunction();
          printer.OS() << ';';
        }
      },
      getAttribute());
}

void Class::Constructor::print(ComponentPrinter &printer) const {
  printer.OS() << getName() << '(';
  for (auto I = getParams().begin(), E = getParams().end(); I != E;) {
    if (I != getParams().begin())
      printer.OS() << ", ";
    const auto &[paramName, paramType] = *I;
    printer.OS() << paramType->toString() << ' ' << paramName;
  }
  printer.OS() << ')';
  if (getImplement()) {
    printer.OS() << " : ";
    const auto &[initializers, body] = *getImplement();
    if (!initializers.empty()) {
      for (auto I = initializers.begin(), E = initializers.end(); I != E;) {
        if (I != initializers.begin())
          printer.OS() << ", ";
        const auto &[fieldName, initExpr] = *I;
        printer.OS() << fieldName << '(' << initExpr << ')';
      }
    }
    bodyCodePrint(printer, body);
  } else
    printer.OS() << ';';
}

void Class::Destructor::print(ComponentPrinter &printer) const {
  printer.OS() << "~" << getName() << "()";
  if (getBody()) {
    bodyCodePrint(printer, *getBody());
  } else
    printer.OS() << ';';
}

void Class::Field::print(ComponentPrinter &printer) const {
  if (isStatic())
    printer.OS() << "static ";
  const auto &[name, type] = getDecl();
  printer.OS() << type->toString() << ' ' << name << ';';
}

void Class::RawCode::print(ComponentPrinter &printer) const {
  printer.OS() << getCode();
}

void Class::Friend::print(ComponentPrinter &printer) const {
  printer.OS() << "friend " << (isClass() ? "class " : "struct ")
               << getFriendType() << ';';
}

void VarDecl::print(ComponentPrinter &printer) const {
  switch (getAccess()) {
  case Access::Extern:
    printer.OS() << "extern ";
    break;
  case Access::Static:
    printer.OS() << "static ";
    break;
  default:
    break;
  }
  const auto &[name, type] = getDecl();
  printer.OS() << type->toString() << ' ' << name << ';';
}

void Function::print(ComponentPrinter &printer) const {
  if (getAttribute()) {
    printer.OS() << "[[" << getAttribute() << "]]";
  }
  switch (getAccess()) {
  case Access::Extern:
    printer.OS() << "extern ";
    break;
  case Access::Static:
    printer.OS() << "static ";
    break;
  case Access::Inline:
    printer.OS() << "inline ";
    break;
  default:
    break;
  }
  printer.OS() << getReturnType()->toString() << ' ' << getName() << '(';
  for (auto I = getParams().begin(), E = getParams().end(); I != E;) {
    if (I != getParams().begin())
      printer.OS() << ", ";
    const auto &[paramName, paramType] = *I;
    printer.OS() << paramType->toString() << ' ' << paramName;
  }
  printer.OS() << ')';
  if (getBody()) {
    bodyCodePrint(printer, *getBody());
  } else
    printer.OS() << ';';
}

void ClassConstructor::print(ComponentPrinter &printer) const {
  for (const auto &access : getNamespaces())
    printer.OS() << access << "::";
  printer.OS() << getName() << "::" << getName() << '(';
  for (auto I = getParams().begin(), E = getParams().end(); I != E;) {
    if (I != getParams().begin())
      printer.OS() << ", ";
    const auto &[paramName, paramType] = *I;
    printer.OS() << paramType->toString() << ' ' << paramName;
  }
  printer.OS() << ')';
  if (!getBody().Initializers.empty()) {
    printer.OS() << " : ";
    for (auto I = getBody().Initializers.begin(),
              E = getBody().Initializers.end();
         I != E;) {
      if (I != getBody().Initializers.begin())
        printer.OS() << ", ";
      const auto &[fieldName, initExpr] = *I;
      printer.OS() << fieldName << '(' << initExpr << ')';
    }
  }
  bodyCodePrint(printer, getBody().Body);
}

void ClassDestructor::print(ComponentPrinter &printer) const {
  for (const auto &access : getNamespaces())
    printer.OS() << access << "::";
  printer.OS() << getName() << "::~" << getName() << "()";
  bodyCodePrint(printer, getBody());
}

void Using::print(ComponentPrinter &printer) const {
  printer.OS() << "using ";
  if (const auto *alias = getAlias())
    printer.OS() << alias->AliasName << " = " << alias->Type->toString();
  else {
    auto *member = getMember();
    for (auto ns : member->Namespaces)
      printer.OS() << ns << "::";
    printer.OS() << member->Name;
  }
  printer.OS() << ';';
}

} // namespace ast::tblgen::cxx
