cmake_minimum_required(VERSION 3.11)

project(OpenColorIO_Ext)

include(ExternalProject)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cmake")

###############################################################################
### options ###

set(OCIO_INSTALL_EXT "NONE" CACHE STRING "Install external dependencies at build time")
set_property(CACHE OCIO_INSTALL_EXT PROPERTY STRINGS NONE MISSING ALL)
set(OCIO_INSTALL_EXT_OPTIONS MISSING ALL)

option(OCIO_EXT_INLINES_HIDDEN "Specify whether to build with -fvisibility-inlines-hidden" ${UNIX})

###############################################################################
### ext paths ###

set(EXT_BUILD_DIR "${CMAKE_BINARY_DIR}/ext/build")
set(EXT_DIST_DIR "${CMAKE_BINARY_DIR}/ext/dist")
set(EXT_INCLUDE_DIR "${EXT_DIST_DIR}/include")
set(EXT_LIBRARY_DIR "${EXT_DIST_DIR}/lib")
set(EXT_PC_DIR "${EXT_LIBRARY_DIR}/pkgconfig")
set(EXT_SITE_PACKAGES_DIR 
    "${EXT_LIBRARY_DIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages")

if(NOT ENV{PKG_CONFIG_PATH} MATCHES "^${EXT_PC_DIR}:")
	set(ENV{PKG_CONFIG_PATH} "${EXT_PC_DIR}:$ENV{PKG_CONFIG_PATH}")
endif()

if(NOT ENV{PYTHONPATH} MATCHES "^${EXT_SITE_PACKAGES_DIR}:")
    set(ENV{PYTHONPATH} "${EXT_SITE_PACKAGES_DIR}:$ENV{PYTHONPATH}")
endif()

###############################################################################
### patch ###

add_custom_target(patch)
if(WIN32)
	add_executable(patch_exec patch.cpp)
	set(PATCH_COMMAND $<TARGET_FILE:patch_exec>)
	add_dependencies(patch patch_exec)
else()
	set(PATCH_COMMAND "patch")
endif()

###############################################################################
### python packages ###

if(OCIO_BUILD_DOCS AND OCIO_INSTALL_EXT IN_LIST OCIO_INSTALL_EXT_ON_OPTIONS)
    include(FindPythonInterp)

    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR 
            "No Python interpreter was found! "
            "Use -DOCIO_INSTALL_EXT_PYTHON_ALL=OFF to skip Python dependencies.")
    endif()

    function(pip_install PKG VER)
        message(STATUS "${PKG} ${VER} will be installed to: ${EXT_SITE_PACKAGES_DIR}")
        add_custom_target(${PKG} ALL)
        add_custom_command(TARGET ${PKG} PRE_BUILD
            COMMAND pip install --quiet 
                                --disable-pip-version-check
                                --install-option="--prefix=${EXT_DIST_DIR}" 
                                -I ${PKG}==${VER}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endfunction()

    if(PYTHON_VERSION_MAJOR EQUAL 2)
        pip_install(setuptools 1.1.6)
        pip_install(docutils 0.14)
        pip_install(MarkupSafe 1.1.1)
        pip_install(Jinja2 2.10.1)
        pip_install(Pygments 2.4.2)
        pip_install(Sphinx 1.8.5)
    elseif(PYTHON_VERSION_MAJOR EQUAL 3)
        pip_install(setuptools 1.1.6)
        pip_install(docutils-python3 0.13)
        pip_install(MarkupSafe 1.1.1)
        pip_install(Jinja2 2.10.1)
        pip_install(Pygments 2.4.2)
        pip_install(Sphinx 2.1.2)
    endif()
endif()

###############################################################################
### c/c++ common options ###

if(UNIX)
    set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -fPIC -fvisibility=hidden")
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -fPIC -fvisibility=hidden")
    if(OCIO_EXT_INLINES_HIDDEN)
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -fvisibility-inlines-hidden")
    endif()
endif()

###############################################################################
### libexpat ###

add_library(libexpat UNKNOWN IMPORTED GLOBAL)

set(LIBEXPAT_MIN_VERSION 2.2.5)

if(NOT OCIO_INSTALL_EXT STREQUAL "ALL")
    set(LIBEXPAT_FIND_ARGS Expat ${LIBEXPAT_MIN_VERSION})
    if(OCIO_INSTALL_EXT STREQUAL "NONE")
        list(APPEND LIBEXPAT_FIND_ARGS REQUIRED)
    endif()
    find_package(${LIBEXPAT_FIND_ARGS})
endif()

if((NOT LIBEXPAT_FOUND AND OCIO_INSTALL_EXT STREQUAL "MISSING") 
        OR (OCIO_INSTALL_EXT STREQUAL "ALL"))

    set(LIBEXPAT_VERSION $<IF:GET_LIBEXPAT_VERSION,GET_LIBEXPAT_VERSION,LIBEXPAT_MIN_VERSION>)
    string(REPLACE "." "_" LIBEXPAT_VERSION_U ${LIBEXPAT_VERSION})

    message(STATUS "LIBEXPAT ${LIBEXPAT_VERSION} will be installed to: ${EXT_DIST_DIR}")

    string(STRIP "${CMAKE_C_FLAGS} ${EXTRA_C_FLAGS} ${LIBEXPAT_C_FLAGS}" LIBEXPAT_C_FLAGS)
    string(STRIP "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS} ${LIBEXPAT_CXX_FLAGS}" LIBEXPAT_CXX_FLAGS)

    set(LIBEXPAT_CMAKE_ARGS
	    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	    -DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
        -DBUILD_examples:BOOL=OFF
        -DBUILD_tests:BOOL=OFF
        -DBUILD_shared:BOOL=OFF
        -DCMAKE_C_FLAGS=${LIBEXPAT_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${LIBEXPAT_CXX_FLAGS}
    )

    if(CMAKE_TOOLCHAIN_FILE)
        set(LIBEXPAT_CMAKE_ARGS 
            "${LIBEXPAT_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    endif()

    set(LIBEXPAT_LIBRARY 
        "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(LIBEXPAT_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")

    ExternalProject_Add(ext_libexpat
        GIT_REPOSITORY "https://github.com/libexpat/libexpat.git"
        GIT_TAG "R_${LIBEXPAT_VERSION_U}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/libexpat"
        BUILD_BYPRODUCTS "${LIBEXPAT_LIBRARY}"
        SOURCE_SUBDIR expat
        CMAKE_ARGS ${LIBEXPAT_CMAKE_ARGS}
    )

    add_dependencies(libexpat ext_libexpat)
endif()

set_property(TARGET libexpat PROPERTY IMPORTED_LOCATION ${LIBEXPAT_LIBRARY})
set_property(TARGET libexpat PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LIBEXPAT_INCLUDE_DIRS})

###############################################################################
### yaml-cpp ###

set(YAMLCPP_MIN_VERSION 0.3.0)
find_package(YamlCpp ${YAMLCPP_MIN_VERSION})

if(NOT YAMLCPP_FOUND OR OCIO_INSTALL_EXT_LIBS_ALWAYS)
    if(USE_YAMLCPP_VERSION)
        set(YAMLCPP_VERSION ${USE_YAMLCPP_VERSION})
    else()
        set(YAMLCPP_VERSION ${YAMLCPP_MIN_VERSION})
    endif()
    string(REPLACE "." ";" YAMLCPP_VERSION_LIST ${YAMLCPP_VERSION})
    list(GET YAMLCPP_VERSION_LIST 0 YAMLCPP_VERSION_MAJOR)
    list(GET YAMLCPP_VERSION_LIST 1 YAMLCPP_VERSION_MINOR)
    list(GET YAMLCPP_VERSION_LIST 2 YAMLCPP_VERSION_PATCH)

    message(STATUS "YAMLCPP ${YAMLCPP_VERSION} will be installed to: ${EXT_DIST_DIR}")

    if(YAMLCPP_VERSION_MINOR LESS 6 AND YAMLCPP_VERSION_PATCH LESS 3)
        set(YAMLCPP_GIT_TAG "release-${YAMLCPP_VERSION}")
    else()
        set(YAMLCPP_GIT_TAG "yaml-cpp-${YAMLCPP_VERSION}")
    endif()

    string(STRIP "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS} ${YAMLCPP_CXX_FLAGS}" YAMLCPP_CXX_FLAGS)

    ExternalProject_Add(yaml-cpp
        GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
        GIT_TAG ${YAMLCPP_GIT_TAG}
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/yaml-cpp"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
                   -DBUILD_SHARED_LIBS:BOOL=OFF
                   -DYAML_CPP_BUILD_TESTS:BOOL=OFF
                   -DYAML_CPP_BUILD_TOOLS:BOOL=OFF
                   -DYAML_CPP_BUILD_CONTRIB:BOOL=OFF
                   -DCMAKE_CXX_FLAGS=${YAMLCPP_COMPILE_FLAGS}
    )
endif()

###############################################################################
### ilmbase (half only) ###

set(ILMBASE_MIN_VERSION 2.3.0)
find_package(IlmBase ${ILMBASE_MIN_VERSION})

if(NOT ILMBASE_FOUND OR OCIO_INSTALL_EXT_LIBS_ALWAYS)
    if(USE_ILMBASE_VERSION)
        set(ILMBASE_VERSION ${USE_ILMBASE_VERSION})
    else()
        set(ILMBASE_VERSION ${ILMBASE_MIN_VERSION})
    endif()

    message(STATUS "ILMBASE ${ILMBASE_VERSION} will be installed to: ${EXT_DIST_DIR}")
    
    string(STRIP "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS} ${ILMBASE_CXX_FLAGS}" ILMBASE_CXX_FLAGS)

    ExternalProject_Add(ilmbase
        GIT_REPOSITORY "https://github.com/openexr/openexr.git"
        GIT_TAG "v${ILMBASE_VERSION}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/openexr"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
                   -DOPENEXR_BUILD_OPENEXR:BOOL=OFF
                   -DOPENEXR_BUILD_PYTHON_LIBS:BOOL=OFF
                   -DOPENEXR_BUILD_SHARED:BOOL=OFF
                   -DOPENEXR_BUILD_STATIC:BOOL=ON
                   -DOPENEXR_ENABLE_TESTS:BOOL=OFF
                   -DCMAKE_CXX_FLAGS=${ILMBASE_CXX_FLAGS}
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --target Half_static
        INSTALL_COMMAND ${CMAKE_COMMAND} -P "IlmBase/Half/cmake_install.cmake"
    )
endif()

###############################################################################
### lcms2 ###

set(LCMS2_MIN_VERSION 2.2)
find_package(LCMS2 ${LCMS2_MIN_VERSION})

if(NOT LCMS2_FOUND OR OCIO_INSTALL_EXT_LIBS_ALWAYS)
    if(USE_LCMS2_VERSION)
        set(LCMS2_VERSION ${USE_LCMS2_VERSION})
    else()
        set(LCMS2_VERSION ${LCMS2_MIN_VERSION})
    endif()

    message(STATUS "LCMS2 ${LCMS2_VERSION} will be installed to: ${EXT_DIST_DIR}")

    string(STRIP "${CMAKE_C_FLAGS} ${EXTRA_C_FLAGS} ${LCMS2_C_FLAGS}" LCMS2_C_FLAGS)

    ExternalProject_Add(lcms2
        GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
        GIT_TAG "lcms${LCMS2_VERSION}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/Little-CMS"
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_MODULE_PATH}/BuildLCMS2.cmake" "CMakeLists.txt"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
                   -DBUILD_SHARED_LIBS:BOOL=OFF
                   -DCMAKE_C_FLAGS=${LCMS2_C_FLAGS}
    )
endif()
