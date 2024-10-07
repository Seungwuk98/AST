cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(AST)

find_package(LLVM REQUIRED 17 CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(AddLLVM)
include(TableGen)
include(ASTTableGen)

llvm_map_components_to_libnames(llvm_libs support core irreader)

add_subdirectory(third-party)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(tool/ASTTableGen)

set(AST_TABLEGEN_EXE "${AST_TABLEGEN_EXE}" CACHE INTERNAL "")
set(AST_TABLEGEN_TARGET "${AST_TABLEGEN_TARGET}" CACHE INTERNAL "")

add_subdirectory(test)

