set(LLVM_LINK_COMPONENTS
  Support
  TableGen
)

add_tablegen(ast-tblgen AST 
  EXPORT AST
  ast-tblgen.cpp
  CXXType.cpp
  CXXComponent.cpp
  TableGenEmitter.cpp
  TableGenModel.cpp
)
