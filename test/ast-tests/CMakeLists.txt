add_executable(ASTTests ASTTests.cpp TestAST.cpp)

target_link_libraries(ASTTests PRIVATE AST doctest)

target_include_directories(ASTTests PRIVATE ${CMAKE_SOURCE_DIR}/third-party/doctest)
