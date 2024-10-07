file(REMOVE ${CMAKE_BINARY_DIR}/tablegen_compile_commands.yml)

function(ast_tablegen ofn) 
  tablegen(AST ${ARGV})
  set(TABLEGEN_OUTPUT ${TABLEGEN_OUTPUT} ${CMAKE_CURRENT_BINARY_DIR}/${ofn} PARENT_SCOPE)

  cmake_parse_arguments(ARG "" "" "DEPENDS;EXTRA_INCLUDES" ${ARGN})
  get_directory_property(tblgen_includes INCLUDE_DIRECTORIES)
  list(APPEND tblgen_includes ${ARG_EXTRA_INCLUDES})
  list(REMOVE_ITEM tblgen_includes "")

  if (IS_ABSOLUTE ${LLVM_TARGET_DEFINITIONS}) 
    set(LLVM_TARGET_DEFINITIONS ${LLVM_TARGET_DEFINITIONS})
  else() 
    set(LLVM_TARGET_DEFINITIONS ${CMAKE_CURRENT_SOURCE_DIR}/${LLVM_TARGET_DEFINITIONS})
  endif()

  file(APPEND ${CMAKE_BINARY_DIR}/tablegen_compile_commands.yml
    "--- !FileInfo:\n"
      "  filepath: \"${LLVM_TARGET_DEFINITIONS_ABSOLUTE}\"\n"
      "  includes: \"${CMAKE_CURRENT_SOURCE_DIR};${tblgen_includes}\"\n"
  )  
endfunction()
