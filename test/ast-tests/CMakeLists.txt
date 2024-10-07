add_executable(ASTTests ASTTests.cpp TestAST.cpp TestAST2.cpp)

target_link_libraries(ASTTests PRIVATE AST doctest)

target_include_directories(ASTTests PRIVATE 
  ${CMAKE_SOURCE_DIR}/third-party/doctest
  ${CMAKE_CURRENT_BINARY_DIR}
)


set(LLVM_TARGET_DEFINITIONS TestAST2.td)
ast_tablegen(TestAST2.hpp.inc --ast-decl-gen)
ast_tablegen(TestAST2.cpp.inc --ast-def-gen)
add_public_tablegen_target(TestAST2Gen)

add_dependencies(ASTTests TestAST2Gen)
