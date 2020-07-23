# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Automatic generation of docstrings for the Python bindings from the C++ headers.

# Compute compilation flags for 'docstrings' target, which extracts docstrings from the C++ header files
if (NOT WIN32)
  if(NOT ${CMAKE_CXX_FLAGS} STREQUAL "")
    string(REPLACE " " ";" MKDOC_CXX_FLAGS_LIST ${CMAKE_CXX_FLAGS})
  endif()
  get_property(MKDOC_INCLUDE_DIRECTORIES DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
  get_property(MKDOC_COMPILE_DEFINITIONS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INTERFACE_COMPILE_DEFINITIONS)

#   message (FATAL_ERROR "${INCLUDE_DIRECTORIES}")


  foreach (value ${MKDOC_INCLUDE_DIRECTORIES})
    list(APPEND MKDOC_CXX_FLAGS_LIST -I${value})
  endforeach()

  foreach (value ${MKDOC_COMPILE_DEFINITIONS})
    list(APPEND MKDOC_CXX_FLAGS_LIST -D${value})
  endforeach()

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=include
                    ERROR_QUIET OUTPUT_VARIABLE CLANG_RESOURCE_DIR)
    get_filename_component(CLANG_RESOURCE_DIR ${CLANG_RESOURCE_DIR} DIRECTORY)
    if (CLANG_RESOURCE_DIR)
      list(APPEND MKDOC_CXX_FLAGS_LIST "-resource-dir=${CLANG_RESOURCE_DIR}")
    endif()
  endif()

  list(REMOVE_ITEM MKDOC_CXX_FLAGS_LIST "-fno-fat-lto-objects")
  list(REMOVE_ITEM MKDOC_CXX_FLAGS_LIST "-flto")

  add_custom_target(docstrings USES_TERMINAL COMMAND
    python3 ${PROJECT_SOURCE_DIR}/share/python/mkdoc.py
    -I${PROJECT_BINARY_DIR}/include/OpenColorIO/OpenColorIO
    -Wno-pragma-once-outside-header
    -ferror-limit=100000
    ${PROJECT_SOURCE_DIR}/include/OpenColorIO/OpenColorIO.h
    ${PROJECT_SOURCE_DIR}/include/OpenColorIO/OpenColorTransforms.h
    ${PROJECT_SOURCE_DIR}/include/OpenColorIO/OpenColorTypes.h
    > ${PROJECT_SOURCE_DIR}/src/bindings/python/PyDocStr.h)
endif()