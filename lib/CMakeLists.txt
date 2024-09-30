add_library(AST STATIC AST.cpp ASTWalker.cpp ASTContext.cpp )

target_link_libraries(AST PRIVATE ${llvm_libs})


