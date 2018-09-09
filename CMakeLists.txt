project(OpenColorIO)
set(OCIO_VERSION_MAJOR 1)
set(OCIO_VERSION_MINOR 1)
set(OCIO_VERSION_PATCH 0)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake)
if(NOT DEFINED CMAKE_FIRST_RUN)
    SET(CMAKE_FIRST_RUN 1 CACHE INTERNAL "")
endif()

###############################################################################
### GLOBAL ###

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
option(OCIO_WARNING_AS_ERROR "Set build error level for CI testing" OFF)
option(OCIO_BUILD_SHARED "Set to OFF to disable building the shared core library" ON)
option(OCIO_BUILD_STATIC "Set to OFF to disable building the static core library" ON)
option(OCIO_BUILD_TRUELIGHT "Set to OFF to disable truelight" ON)
option(OCIO_BUILD_APPS "Set to OFF to disable command-line apps" ON)
option(OCIO_BUILD_NUKE "Set to OFF to disable building nuke plugins" ON)
option(OCIO_BUILD_DOCS "Specify whether to build documentation" UNIX)
option(OCIO_BUILD_TESTS "Specify whether to build unittests" ON)
option(OCIO_BUILD_GPU_TESTS "Specify whether to build unittests" ON)
option(OCIO_BUILD_PYGLUE "Specify whether to build python bindings" ON)
option(OCIO_BUILD_JNIGLUE "Specify whether to build java bindings" OFF)
option(OCIO_STATIC_JNIGLUE "Specify whether to statically link ocio to the java bindings" ON)

option(OCIO_USE_SSE "Specify whether to enable SSE CPU performance optimizations" ON)
option(OCIO_INLINES_HIDDEN "Specify whether to build with -fvisibility-inlines-hidden" UNIX)

# Use boost's shared_ptr by default on Windows (as <VS2010 doesn't have it),
# Use std::tr1::shared_ptr by default on other platforms
option(OCIO_USE_BOOST_PTR "Set to ON to enable boost shared_ptr (necessary when tr1 is not available)" WIN32)

option(OCIO_PYGLUE_LINK "If ON, link the Python module to the core shared library" OFF)
option(OCIO_PYGLUE_RESPECT_ABI "If ON, the Python module install path includes Python UCS version" OFF)
option(OCIO_PYGLUE_SONAME "If ON, soname/soversion will be set for Python module library" OFF)
option(OCIO_PYGLUE_LIB_PREFIX "If ON, prefix the Python module with 'lib'" OFF)

if(UNIX AND NOT APPLE)
    option(USE_EXTERNAL_YAML "Use system installed yaml-cpp library." OFF)
    option(USE_EXTERNAL_TINYXML "Use system installed tinyxml library." OFF)
    option(USE_EXTERNAL_LCMS "Use system install lcms2 library." OFF)
endif()

# This does not include the SOVERSION override, on purpose, so that the
# OCIO_VERSION value will be an accurate reflection of the underlying library version.
set(OCIO_VERSION "${OCIO_VERSION_MAJOR}.${OCIO_VERSION_MINOR}.${OCIO_VERSION_PATCH}")

if(NOT SOVERSION)
    set(SOVERSION ${OCIO_VERSION_MAJOR} CACHE STRING "Set the SO version in the SO name of the output library")
endif()

###############################################################################
if(POLICY CMP0054)
    ## Only interpret if() arguments as variables or keywords when unquoted
    cmake_policy(SET CMP0054 NEW)
endif()

include(ParseArguments)
include(OCIOMacros)
include(ExternalProject)
include(GNUInstallDirs)

enable_language(CXX)

ENABLE_TESTING()

if(APPLE OR IPHONE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING
            "Setting OSX Architectures, options are: ppc;i386;ppc64;x86_64"
            FORCE)
    endif()
    if(CMAKE_FIRST_RUN)
        message(STATUS "Setting OSX Architectures to: ${CMAKE_OSX_ARCHITECTURES}")
    endif()
endif()

# Set the default built type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
    message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()

# Set the default namespace
if(NOT OCIO_NAMESPACE)
  set(OCIO_NAMESPACE OpenColorIO CACHE STRING
      "Specify the master OCIO C++ namespace: Options include OpenColorIO OpenColorIO_<YOURFACILITY> etc."
      FORCE)
endif(NOT OCIO_NAMESPACE)
messageonce("Setting Namespace to: ${OCIO_NAMESPACE}")

# If CMAKE_INSTALL_EXEC_PREFIX is not specified, install binaries
# directly into the regular install prefix
if(NOT CMAKE_INSTALL_EXEC_PREFIX)
    messageonce("Exec prefix not specified, defaulting to ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_INSTALL_EXEC_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

messageonce("Use Boost Ptr: ${OCIO_USE_BOOST_PTR}")
if(OCIO_USE_BOOST_PTR)
    set(Boost_ADDITIONAL_VERSIONS "1.49" "1.45" "1.44" "1.43" "1.43.0" "1.42"
                                  "1.42.0" "1.41" "1.41.0" "1.40"
                                  "1.40.0" "1.39" "1.39.0" "1.38"
                                  "1.38.0" "1.37" "1.37.0" "1.34.1"
                                  "1_34_1")
    set(Boost_USE_MULTITHREADED ON)
    find_package(Boost 1.34)
    if(NOT Boost_FOUND)
        message(FATAL_ERROR "OCIO_USE_BOOST_PTR is specified, but a boost installation could not be found.")
    else()
        set(OCIO_USE_BOOST_PTR 1)
    endif()

    set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
    # set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
else()
    set(OCIO_USE_BOOST_PTR 0)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(OCIO_INLINES_HIDDEN)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                        OUTPUT_VARIABLE GCC_VERSION)
        if (GCC_VERSION VERSION_LESS 4.2)
            message(STATUS "WARNING: GCC Version < 4.2 - disabling hidden inlines")
            set(OCIO_INLINES_HIDDEN OFF)
        endif()
    endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    # Enable a bunch of compiler warnings...
    # http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Wconversion -Wcast-qual -Wformat=2")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif(CMAKE_COMPILER_IS_GNUCXX)

###############################################################################
### Python ###

OCIOFindPython()

# Find Python, used for (possibly) building pyglue, and now to
# construct the external project path
set(EXTDIST_ROOT ${CMAKE_BINARY_DIR}/ext/dist)
set(EXTDIST_BINPATH ${EXTDIST_ROOT}/bin)
if(PYTHON_OK)
    set(EXTDIST_PYTHONPATH ${EXTDIST_ROOT}/${PYTHON_VARIANT_PATH})
    if(UNIX)
        set(PYTHONPATH ${EXTDIST_PYTHONPATH}:$ENV{PYTHONPATH})
    else()
        # The line below should work but ';' is always replaced by a space
        #   which breaks the build. Anyway in the worst scenario, no expected
        #   modules are present in the local python so install them.
        #set(PYTHONPATH ${EXTDIST_PYTHONPATH}\\;$ENV{PYTHONPATH})
        set(PYTHONPATH ${EXTDIST_PYTHONPATH})
    endif()
endif()

messageonce("Setting EXTDIST_BINPATH: ${EXTDIST_BINPATH}")
messageonce("Setting EXTDIST_PYTHONPATH: ${EXTDIST_PYTHONPATH}")

###############################################################################
### tinyxml ###
##TODO: yaml and tinyxml : when there are not USE_EXTERNAL_TINYXML, use the same cmake schema/instructions => maybe refactorize in a cmake functions/macros ?

if(USE_EXTERNAL_TINYXML)
    set(TINYXML_VERSION_MIN "2.6.1")
    find_package(TinyXML)
    if(TINYXML_FOUND)
        if(TINYXML_VERSION VERSION_EQUAL ${TINYXML_VERSION_MIN} OR
           TINYXML_VERSION VERSION_GREATER ${TINYXML_VERSION_MIN})
            message(STATUS "External TinyXML will be used.")
        else()
            message(FATAL_ERROR "ERROR: ${TINYXML_VERSION} found, but ${TINYXML_VERSION_MIN} or newer is required.")
        endif()
    else(TINYXML_FOUND)
        message(STATUS "TinyXML was not found. Perhaps you forgot to install the development package?")
    endif(TINYXML_FOUND)

else(USE_EXTERNAL_TINYXML)
    set(TINYXML_VERSION 2_6_1)
    set(TINYXML_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/ext/dist -DOCIO_INLINES_HIDDEN:BOOL=${OCIO_INLINES_HIDDEN})
    if(CMAKE_TOOLCHAIN_FILE)
        set(TINYXML_CMAKE_ARGS ${TINYXML_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()
    set(TINYXML_CMAKE_ARGS ${TINYXML_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
    set(TINYXML_ZIPFILE     "${CMAKE_SOURCE_DIR}/ext/tinyxml_${TINYXML_VERSION}.tar.gz")
    set(TINYXML_PATCHFILE   "${CMAKE_SOURCE_DIR}/ext/tinyxml_${TINYXML_VERSION}.patch")
    set(TINYXML_SOURCE_DIR  "${CMAKE_BINARY_DIR}/ext")
    ## Create our TINYXML_LIB target
    if(CMAKE_VERSION VERSION_GREATER "2.8.7")
        option(TINYXML_OBJECT_LIB_EMBEDDED ${OCIO_BUILD_STATIC} "directly embedded tinyxml 3rdParty object lib into our resulting static lib, no link needed anymore")
    else()
        set(TINYXML_OBJECT_LIB_EMBEDDED OFF CACHE BOOL "directly embedded tinyxml 3rdParty object lib into our resulting static lib, no link needed anymore" FORCE)
        message("Force disable TINYXML_OBJECT_LIB_EMBEDDED feature due to CMake Version less than 2.8.8")
    endif()
    mark_as_advanced(TINYXML_OBJECT_LIB_EMBEDDED) ## two way to build with tinyxml (objects embedded or static transitive link)
    if(TINYXML_OBJECT_LIB_EMBEDDED)
        set(tinyxml_sources ${TINYXML_SOURCE_DIR}/tinyxml/tinystr.cpp       ${TINYXML_SOURCE_DIR}/tinyxml/tinyxml.cpp
                            ${TINYXML_SOURCE_DIR}/tinyxml/tinyxmlerror.cpp  ${TINYXML_SOURCE_DIR}/tinyxml/tinyxmlparser.cpp )
        set(tinyxml_headers ${TINYXML_SOURCE_DIR}/tinyxml/tinystr.h         ${TINYXML_SOURCE_DIR}/tinyxml/tinyxml.h )
        add_custom_command(                                 ## will be done at build time
            OUTPUT ${tinyxml_sources} ${tinyxml_headers}    ## expected output files
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${TINYXML_ZIPFILE}
            DEPENDS ${TINYXML_ZIPFILE}
            WORKING_DIRECTORY ${TINYXML_SOURCE_DIR}
            COMMENT "Unpacking ${TINYXML_ZIPFILE} to ${TINYXML_SOURCE_DIR}"
            VERBATIM
        )
        include_directories(BEFORE ${TINYXML_SOURCE_DIR}/tinyxml) ## needed to build correctly
        add_library(TINYXML_LIB OBJECT ${tinyxml_sources})
        ## embedded tinyxml objects files (no static link needed anymore)
        ## => great news when build staticaly since we do not want another client project have to link also with tinyxml when he want to use this project
        ## => could be problematic if the client project use another version of tinyxml... In this case build tinyxml as shared lib with all projects could be a solution
        ## => TODO: so maybe provide a simple cmake way to build 3rdParty as shared and auto install with this project ?
        set_target_properties(TINYXML_LIB PROPERTIES COMPILE_FLAGS "-DTIXML_USE_STL -fPIC -fvisibility-inlines-hidden -fvisibility=hidden")
        add_definitions(-DTIXML_USE_STL) ## needed to build correctly, and also need to be propagated in child projects (client projects)
        list(APPEND EXTERNAL_OBJECTS $<TARGET_OBJECTS:TINYXML_LIB>)
    else()
        find_package(Git REQUIRED) ## in order to apply patch (for crossplateform compatibility)
        ExternalProject_Add(tinyxml
            URL             ${TINYXML_ZIPFILE}
            SOURCE_DIR      ${TINYXML_SOURCE_DIR}/tinyxml
            PATCH_COMMAND   ${GIT_EXECUTABLE} apply --ignore-whitespace ${TINYXML_PATCHFILE}
            BINARY_DIR      ext/build/tinyxml
            INSTALL_DIR     ext/dist
            CMAKE_ARGS      ${TINYXML_CMAKE_ARGS}
        )
        if(WIN32)
            set(TINYXML_STATIC_LIBRARIES  ${PROJECT_BINARY_DIR}/ext/dist/lib/tinyxml.lib)
        else()
            set(TINYXML_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libtinyxml.a)
        endif()
        add_library(TINYXML_LIB STATIC IMPORTED)
        ## static is the .lib location, shared is the .dll/.so location (see IMPORTED_IMPLIB for the associated .lib archive location on windows)
        set_property(TARGET TINYXML_LIB PROPERTY IMPORTED_LOCATION ${TINYXML_STATIC_LIBRARIES})
        add_dependencies(TINYXML_LIB tinyxml)
        list(APPEND EXTERNAL_LIBRARIES TINYXML_LIB)
    endif()
    set_target_properties(TINYXML_LIB PROPERTIES FOLDER External)
endif(USE_EXTERNAL_TINYXML)

###############################################################################
### YAML ###

if(USE_EXTERNAL_YAML)
    # Set minimum yaml version for non-patched sources.
    set(YAML_VERSION_MIN "0.3.0")
    include(FindPkgConfig)
    pkg_check_modules(PC_YAML_CPP REQUIRED QUIET yaml-cpp)
    find_path(YAML_CPP_INCLUDE_DIR yaml-cpp/yaml.h
        HINTS  ${PC_YAML_CPP_INCLUDEDIR} ${PC_YAML_CPP_INCLUDE_DIRS} )
    find_library(YAML_CPP_LIBRARY LIBRARY_NAMES yaml-cpp libyaml-cpp
        HINTS ${PC_YAML_CPP_LIBRARY_DIRS} )
    set(YAML_CPP_LIBRARIES ${YAML_CPP_LIBRARY})
    set(YAML_CPP_INCLUDE_DIRS ${YAML_CPP_INCLUDE_DIR})
    set(YAML_CPP_VERSION ${PC_YAML_CPP_VERSION})

    if(YAML_CPP_VERSION VERSION_LESS ${YAML_VERSION_MIN})
        message(FATAL_ERROR "ERROR: yaml-cpp ${YAML_VERSION_MIN} or greater is required.")
    endif()

    find_package_handle_standard_args(yaml-cpp
                                      REQUIRED_VARS YAML_CPP_LIBRARIES YAML_CPP_INCLUDE_DIRS )
    set(YAML_CPP_FOUND ${YAML-CPP_FOUND})
    mark_as_advanced(YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARY YAML-CPP_FOUND)

    if(YAML_CPP_FOUND)
        if(YAML_CPP_VERSION VERSION_GREATER "0.5.0")
            # Need to also get the boost headers here, as yaml-cpp 0.5.0+ requires them.
            # Don't bother doing this step if we are already including the boost headers for shared_ptr
            if(NOT OCIO_USE_BOOST_PTR)
                set(Boost_ADDITIONAL_VERSIONS "1.49" "1.45" "1.44" "1.43" "1.43.0" "1.42"
                                              "1.42.0" "1.41" "1.41.0" "1.40"
                                              "1.40.0" "1.39" "1.39.0" "1.38"
                                              "1.38.0" "1.37" "1.37.0" "1.34.1"
                                              "1_34_1")
                set(Boost_USE_MULTITHREADED ON)
                find_package(Boost 1.34)
                if(NOT Boost_FOUND)
                    message(FATAL_ERROR "Error: Detected system yaml-cpp version ${YAML_CPP_VERSION} is greater than 0.5.0, and therefore requires boost, but a boost installation could not be found.")
                endif()

                set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
            endif()
        endif()
        set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${YAML_CPP_INCLUDE_DIRS})
    else(YAML_CPP_FOUND)
        message(FATAL_ERROR "ERROR: System yaml-cpp library was not found. Make sure the library is installed and the pkg-config file exists.")
    endif(YAML_CPP_FOUND)
else(USE_EXTERNAL_YAML) ## provide 2 ways to build this dependency
    set(YAML_CPP_VERSION 0.3.0)
    set(YAML_CPP_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/ext/dist -DYAML_CPP_BUILD_TOOLS:BOOL=FALSE -DOCIO_INLINES_HIDDEN:BOOL=${OCIO_INLINES_HIDDEN})
    if(CMAKE_TOOLCHAIN_FILE)
        set(YAML_CPP_CMAKE_ARGS ${YAML_CPP_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()
    set(YAML_CPP_CMAKE_ARGS ${YAML_CPP_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
    set(YAML_CPP_ZIPFILE        "${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}.tar.gz")
    set(YAML_CPP_SOURCE_DIR     "${CMAKE_BINARY_DIR}/ext")
    set(YAML_CPP_PATCHFILE      "${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}.patch")
    set(YAML_CPP_SRCLISTFILE    "${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}-sourcesList.txt") ## see cmd line for this generated file bellow
    ## Create our YAML_CPP_LIB target
    if(CMAKE_VERSION VERSION_GREATER "2.8.7")
        option(YAML_CPP_OBJECT_LIB_EMBEDDED ${OCIO_BUILD_STATIC} "directly embedded yaml-cpp 3rdParty lib into our resulting static lib, no link needed anymore")
    else()
        set(YAML_CPP_OBJECT_LIB_EMBEDDED OFF CACHE BOOL "directly embedded yaml-cpp 3rdParty object lib into our resulting static lib, no link needed anymore" FORCE)
        message("Force disable YAML_CPP_OBJECT_LIB_EMBEDDED feature due to CMake Version less than 2.8.8")
    endif()
    mark_as_advanced(YAML_CPP_OBJECT_LIB_EMBEDDED)
    if(YAML_CPP_OBJECT_LIB_EMBEDDED)
        ## generated at extraction level dir, with (use git bash or cygwin under windows):
        ## find yaml-cpp -type f \( -name "*.h" -o -name "*.cpp" \) -not \( -path "*/test/*" -o -path "*/util/*" \) > yaml-cpp-sourcesList.txt
        ## this will exclude test and util paths since we want YAML_CPP_BUILD_TOOLS:BOOL=OFF (see externalProject cmake args)
        file(STRINGS ${YAML_CPP_SRCLISTFILE} YAML_CPP_SRCLISTFILE_CONTENT)
        foreach(yamlsrc ${YAML_CPP_SRCLISTFILE_CONTENT})
            list(APPEND yamlcpp_sources ${YAML_CPP_SOURCE_DIR}/${yamlsrc})  ## get absolute filepath
        endforeach()
        include_directories(BEFORE ${YAML_CPP_SOURCE_DIR}/yaml-cpp/include) ## needed to build correctly
        add_custom_command(             ## will be done at build time
            OUTPUT ${yamlcpp_sources}   ## expected output files
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${YAML_CPP_ZIPFILE}
            DEPENDS ${YAML_CPP_ZIPFILE}
            WORKING_DIRECTORY ${YAML_CPP_SOURCE_DIR}
            COMMENT "Unpacking ${YAML_CPP_ZIPFILE} to ${YAML_CPP_SOURCE_DIR}"
            VERBATIM
        )
        add_library(YAML_CPP_LIB OBJECT ${yamlcpp_sources})
        ## embedded yaml-cpp objects files (no static link needed anymore)
        ## => great news when build statically since we do not want another client project have to link also with yaml-cpp when he want to use this project
        ## => could be problematic if the client project use another version of yaml-cpp... In this case build yaml-cpp as shared lib with all projects could be a solution
        ## => TODO: so maybe provide a simple cmake way to build 3rdParty as shared and auto install with this project ?
        list(APPEND EXTERNAL_OBJECTS $<TARGET_OBJECTS:YAML_CPP_LIB>)
    else()
        find_package(Git REQUIRED) ## in order to apply patch (for crossplateform compatibility)
        ExternalProject_Add(YAML_CPP_LOCAL
            URL             ${YAML_CPP_ZIPFILE}
            SOURCE_DIR      ${YAML_CPP_SOURCE_DIR}/yaml-cpp
            PATCH_COMMAND   ${GIT_EXECUTABLE} apply --ignore-whitespace ${YAML_CPP_PATCHFILE}
            BINARY_DIR      ext/build/yaml-cpp
            INSTALL_DIR     ext/dist
            CMAKE_ARGS      ${YAML_CPP_CMAKE_ARGS}
        )
        set(YAML_CPP_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
        set(YAML_CPP_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
        add_library(YAML_CPP_LIB STATIC IMPORTED)
        if(WIN32)
            set(YAML_CPP_STATIC_DEBUG_LIBRARIES     ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cppmdd.lib)
            set(YAML_CPP_STATIC_OPTIMIZED_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cppmd.lib)
            set_property(TARGET YAML_CPP_LIB PROPERTY IMPORTED_LOCATION_DEBUG   ${YAML_CPP_STATIC_DEBUG_LIBRARIES})
            set_property(TARGET YAML_CPP_LIB PROPERTY IMPORTED_LOCATION_RELEASE ${YAML_CPP_STATIC_OPTIMIZED_LIBRARIES})
        else()
            set(YAML_CPP_STATIC_GENERAL_LIBRARIES           ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cpp.a)
            set_property(TARGET YAML_CPP_LIB PROPERTY IMPORTED_LOCATION ${YAML_CPP_STATIC_GENERAL_LIBRARIES})
        endif()
        add_dependencies(YAML_CPP_LIB  YAML_CPP_LOCAL)
        list(APPEND EXTERNAL_LIBRARIES YAML_CPP_LIB)
    endif()
    set_target_properties(YAML_CPP_LIB PROPERTIES FOLDER External)
endif(USE_EXTERNAL_YAML)

if(YAML_CPP_VERSION VERSION_LESS "0.5.0")
    set(YAML_CPP_COMPILE_FLAGS "-DOLDYAML")
endif()

###############################################################################
### Externals ###

set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_BINARY_DIR}/ext/dist/include)

## maybe better to separate too general COMPILE_FLAGS (which are compiler specific) and split with specific COMPILE_DEFINITIONS
## so we can easily export it from cmake for the installation and be used easily in child projects ?

if(UNIX)
    set(GCC_COMPILE_FLAGS "${GCC_COMPILE_FLAGS} -fPIC -fvisibility=hidden")
    if(OCIO_INLINES_HIDDEN)
        set(GCC_COMPILE_FLAGS "${GCC_COMPILE_FLAGS} -fvisibility-inlines-hidden")
    endif()
else()
    set(OCIO_INLINES_HIDDEN OFF)
endif()

set(EXTERNAL_COMPILE_FLAGS "-DTIXML_USE_STL ${YAML_CPP_COMPILE_FLAGS} ${GCC_COMPILE_FLAGS}")

set(EXTERNAL_LINK_FLAGS "")
set(EXTERNAL_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
## EXTERNAL_OBJECTS (used with add_library) or EXTERNAL_LIBRARIES (used with target_link_libraries) are available
## One will be empty (no effect) whereas the other will contain external dep to use (as object to incorporate or static lib to link)
if(EXTERNAL_OBJECTS)
    list(REMOVE_DUPLICATES EXTERNAL_OBJECTS)
elseif(EXTERNAL_LIBRARIES)
    list(REMOVE_DUPLICATES EXTERNAL_LIBRARIES)
endif()

###############################################################################
### Documentation ###

messageonce("Generate Documentation: ${OCIO_BUILD_DOCS}")

if(OCIO_BUILD_DOCS)
    if(PYTHON_OK)
        add_subdirectory(docs)
    else()
        message(STATUS "Building the documentation requires Python, but locating Python failed: ${PYTHON_ERR}")
    endif()
endif()

###############################################################################
### SSE ###

messageonce("SSE Optimizations: ${OCIO_USE_SSE}")

if(OCIO_USE_SSE)
    if(WIN32)
        # SSE instructions are automatically compiled into 64-bit applications so enabling the option is redundant and
        # actually produces an unknown option warning in Visual Studio.
        if(NOT CMAKE_CL_64)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
        endif()
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    endif()
    add_definitions("-DUSE_SSE")
endif()

###############################################################################
### TRUELIGHT ###

if(OCIO_BUILD_TRUELIGHT)
    find_package(Truelight)
    if(TRUELIGHT_FOUND)
        messageonce("Will build the truelight transform support against ${Truelight_LIBRARY_DIR}")
        add_definitions(-DOCIO_TRUELIGHT_SUPPORT)
        set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Truelight_INCLUDE_DIR})
        set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} ${Truelight_COMPILE_FLAGS}")
        set(EXTERNAL_LINK_FLAGS "${EXTERNAL_LINK_FLAGS} ${Truelight_LINK_FLAGS}")
        set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Truelight_LIBRARIES})
    else()
        messageonce("Not building truelight transform support. Add the flag -D TRUELIGHT_INSTALL_PATH=... or set the TRUELIGHT_ROOT environment variable")
    endif()
endif()


###############################################################################
### CORE ###

add_subdirectory(src/core)

if(OCIO_BUILD_GPU_TESTS OR OCIO_BUILD_APPS)
    OCIOFindOpenGL()
endif()


###############################################################################
### CPU TESTS ###

messageonce("Build Unit Tests: ${OCIO_BUILD_TESTS}")
if(OCIO_BUILD_TESTS)
    add_subdirectory(src/core_tests)
else()
    messageonce("Not building unit tests...")
endif()


###############################################################################
### GPU TESTS ###

messageonce("Build GPU Unit Tests: ${OCIO_BUILD_GPU_TESTS}")
if(OCIO_BUILD_GPU_TESTS)
    if(NOT OPENGL_FOUND OR NOT GLUT_FOUND OR NOT GLEW_FOUND OR NOT OCIO_BUILD_SHARED)
        messageonce("Not building gpu unit tests. Requirement(s) found, OpenColorIO:${OCIO_BUILD_SHARED}, OpenGL:${OPENGL_FOUND}, GLUT:${GLUT_FOUND}, GLEW:${GLEW_FOUND}")
        set(OCIO_BUILD_GPU_TESTS OFF)
    else()
        add_subdirectory(src/core_gpu_tests)
    endif()
else()
    messageonce("Not building gpu unit tests.")
endif()


###############################################################################
### OpenGL Engine ###

if(OCIO_BUILD_APPS AND OPENGL_FOUND AND GLEW_FOUND)
    messageonce("Build the OpenGL engine")
    add_subdirectory(src/lang-glsl)
else()
    messageonce("Not building the OpenGL engine. Requirement(s) found, OpenGL:${OPENGL_FOUND}, GLEW:${GLEW_FOUND}")
endif()


###############################################################################
### APPS ###

if(OCIO_BUILD_APPS AND (OCIO_BUILD_STATIC OR OCIO_BUILD_SHARED) )

    # Try to find OpenImageIO (OIIO) and OpenGL stuff
    OCIOFindOpenImageIO()

    if(OIIO_FOUND)
        add_subdirectory(src/apps/ocioconvert)
        add_subdirectory(src/apps/ociolutimage)
    else()
        messageonce("Not building ocioconvert/ociolutimage. Requirement(s) found: OIIO:${OIIO_FOUND}")
    endif()

    # ociodisplay, displays color-transformed images (uses OpenImageIO,
    # and OpenGL)
    if(OPENGL_FOUND AND GLUT_FOUND AND GLEW_FOUND AND OIIO_FOUND)
        add_subdirectory(src/apps/ociodisplay)
    else()
        ## GLEW : http://glew.sourceforge.net/install.html
        ## on windows, if you use STATIC version, do not forget to use : GEW_BUILD=GLEW_STATIC preprocessor definition
        messageonce("Not building ociodisplay. Requirement(s) found, OpenGL:${OPENGL_FOUND}, GLUT:${GLUT_FOUND}, GLEW:${GLEW_FOUND}, OIIO:${OIIO_FOUND}")
    endif()

    # ociocheck: verifies an OCIO config
    add_subdirectory(src/apps/ociocheck)

    # ociobakelut writes out luts
    add_subdirectory(src/apps/ociobakelut)

else()
    message(STATUS "Disable build of apps. See cmake options : OCIO_BUILD_APPS and OCIO_BUILD_SHARED/OCIO_BUILD_STATIC (required)")
endif()

###############################################################################
### NUKE ###

if(OCIO_BUILD_NUKE)
    find_package(Nuke)
    if(NUKE_FOUND)
        messageonce("Will build the Nuke plugins against ${Nuke_LIBRARY_DIR}")
        add_subdirectory(src/nuke)
    else()
        messageonce("Not building Nuke plugins. Add the flag -D NUKE_INSTALL_PATH=... or set the NDK_PATH environment variable")
    endif()
endif()

###############################################################################
### PYGLUE ###

if(OCIO_BUILD_PYGLUE AND NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug")
    if(PYTHON_OK)
        add_subdirectory(src/pyglue)
    else()
        messageonce("Python bindings will not be built: ${PYTHON_ERR}")
    endif()
endif()

###############################################################################
### JNIGLUE ###

if(OCIO_BUILD_JNIGLUE)
    OCIOFindJava()
    if(Java_FOUND)
      add_subdirectory(src/jniglue)
    else()
      messageonce("Java bindings will not be built as we can't find Java")
    endif()
endif()

###############################################################################
### Configure env script ###
configure_file(${CMAKE_SOURCE_DIR}/share/ocio/setup_ocio.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/setup_ocio.sh @ONLY)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/setup_ocio.sh DESTINATION ${CMAKE_INSTALL_DATADIR}/ocio/)

###############################################################################
### CPACK ###

set(CPACK_PACKAGE_VERSION_MAJOR ${SOVERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${OCIO_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OCIO_VERSION_PATCH})
set(CPACK_GENERATOR None)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "ocio.${OCIO_VERSION}")
include(CPack)

###############################################################################
### CTEST ###

if(OCIO_BUILD_TESTS)
    set(TEST_DEPENDS ocio_core_tests)
endif()

if(OCIO_BUILD_GPU_TESTS)
    set(TEST_DEPENDS ${TEST_DEPENDS} ocio_core_gpu_tests)
endif()

add_custom_target(test_verbose
                  COMMAND ctest -VV
                  DEPENDS ${TEST_DEPENDS}
                  COMMENT "Running ctest with verbose output")

# Log CMake first run done
SET(CMAKE_FIRST_RUN 0 CACHE INTERNAL "")



## Export install
## Example of usage by another project :
##  set(OpenColorIO_DIR "" CACHE PATH "the installation dir of OpenColorIO (Where to find OpenColorIOConfig.cmake)")
##  option(OpenColorIO_USE_STATIC OFF "use static target version of OpenColorIO")
##  find_package(OpenColorIO)
##  if(OPENCOLORIO_FOUND)
##      include_directories(${OpenColorIO_INCLUDE_DIR})
##      foreach(OCIO_DEF ${OCIO_COMPILE_DEFINITIONS})
##          add_definitions(-D${OCIO_DEF})
##      endforeach()
##  endif()
## <Then use latter in your project>
## Target_link_library(<myTarget> ${OpenColorIO_LIBRARIES})
get_property(OCIO_DIR_COMPILE_DEFINITIONS DIRECTORY PROPERTY COMPILE_DEFINITIONS)
if(NOT OCIO_DIR_COMPILE_DEFINITIONS)
    set(OCIO_DIR_COMPILE_DEFINITIONS )
endif()
if(TARGET OpenColorIO)
    get_target_property(OCIO_SHARED_COMPILE_DEFINITIONS OpenColorIO COMPILE_DEFINITIONS)
    if(OCIO_SHARED_COMPILE_DEFINITIONS)
        set(OCIO_SHARED_COMPILE_DEFINITIONS ${OCIO_SHARED_COMPILE_DEFINITIONS} ${OCIO_DIR_COMPILE_DEFINITIONS})
        list(REMOVE_DUPLICATES OCIO_SHARED_COMPILE_DEFINITIONS)
    elseif(OCIO_DIR_COMPILE_DEFINITIONS)
        set(OCIO_SHARED_COMPILE_DEFINITIONS ${OCIO_DIR_COMPILE_DEFINITIONS})
    else()
        set(OCIO_SHARED_COMPILE_DEFINITIONS )
    endif()
endif()
if(TARGET OpenColorIO_STATIC)
    get_target_property(OCIO_STATIC_COMPILE_DEFINITIONS OpenColorIO_STATIC COMPILE_DEFINITIONS)
    if(OCIO_STATIC_COMPILE_DEFINITIONS)
        set(OCIO_STATIC_COMPILE_DEFINITIONS ${OCIO_STATIC_COMPILE_DEFINITIONS} ${OCIO_DIR_COMPILE_DEFINITIONS})
        list(REMOVE_DUPLICATES OCIO_STATIC_COMPILE_DEFINITIONS)
    elseif(OCIO_DIR_COMPILE_DEFINITIONS)
        set(OCIO_STATIC_COMPILE_DEFINITIONS ${OCIO_DIR_COMPILE_DEFINITIONS})
    else()
        set(OCIO_STATIC_COMPILE_DEFINITIONS )
    endif()
endif()
install(EXPORT OpenColorIO DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenColorIO)
file(WRITE "${CMAKE_BINARY_DIR}/OpenColorIOConfig.cmake"
    "
    get_filename_component(OpenColorIO_DIR \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)

    ## include
    set(OpenColorIO_INCLUDE_DIR     \"\${OpenColorIO_DIR}/include\")
    set(OpenColorIO_INCLUDE_DIRS    \"\${OpenColorIO_INCLUDE_DIR}\")
    message(STATUS OpenColorIO_INCLUDE_DIRS=\${OpenColorIO_INCLUDE_DIRS})

    ## targets libraries + associated definitions
    if(NOT TARGET OpenColorIO)
        include(\"\${OpenColorIO_DIR}/${CMAKE_INSTALL_LIBDIR}/cmake/OpenColorIO/OpenColorIO.cmake\") ## thanks to imported target
        if(TARGET OpenColorIO AND NOT OpenColorIO_USE_STATIC)
            message(STATUS \"shared target OpenColorIO : see OpenColorIO_LIBRARY\")
            set(OpenColorIO_LIBRARY         OpenColorIO)
            set(OCIO_COMPILE_DEFINITIONS    ${OCIO_SHARED_COMPILE_DEFINITIONS})
        endif()
        if(NOT TARGET OpenColorIO AND TARGET OpenColorIO_STATIC)
            message(STATUS \"static target OpenColorIO_STATIC : see OpenColorIO_LIBRARY\")
            set(OpenColorIO_LIBRARY         OpenColorIO_STATIC)
            set(OCIO_COMPILE_DEFINITIONS    ${OCIO_STATIC_COMPILE_DEFINITIONS})
        elseif(TARGET OpenColorIO_STATIC AND OpenColorIO_USE_STATIC)
            message(STATUS \"static target OpenColorIO_STATIC : see OpenColorIO_LIBRARY\")
            set(OpenColorIO_LIBRARY OpenColorIO_STATIC)
            set(OCIO_COMPILE_DEFINITIONS    ${OCIO_STATIC_COMPILE_DEFINITIONS})
        endif()
    endif()
    set(OpenColorIO_LIBRARIES \${OpenColorIO_LIBRARY})
    message(STATUS OpenColorIO_LIBRARIES=\${OpenColorIO_LIBRARIES})

    ## display available preprocessor definition to use
    if(OCIO_COMPILE_DEFINITIONS)
        message(STATUS \"OCIO_COMPILE_DEFINITIONS=\")
        foreach(OCIO_DEF \${OCIO_COMPILE_DEFINITIONS})
            message(STATUS \"   \${OCIO_DEF}\")
        endforeach()
    endif()

    ## found
    if(OpenColorIO_INCLUDE_DIRS AND OpenColorIO_LIBRARIES)
        set(OpenColorIO_FOUND ON)
        set(OPENCOLORIO_FOUND ON)
    else()
        set(OpenColorIO_FOUND OFF)
        set(OPENCOLORIO_FOUND OFF)
    endif()
    message(STATUS OPENCOLORIO_FOUND=\${OPENCOLORIO_FOUND})
    "
)
install(FILES "${CMAKE_BINARY_DIR}/OpenColorIOConfig.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenColorIO)
