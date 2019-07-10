include(ExternalProject)

###############################################################################
# External dependency paths

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
# Python packages

if(OCIO_BUILD_DOCS AND OCIO_INSTALL_EXT IN_LIST OCIO_INSTALL_EXT_OPTIONS)
	# OCIO_INSTALL_EXT option MISSING is treated as ALL for Python packages
    include(FindPythonInterp)

    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR 
            "No Python interpreter was found! "
            "Use -DOCIO_BUILD_DOCS=OFF to skip Python dependencies.")
    endif()

    function(pip_install pkg ver)
        message(STATUS "${pkg} ${ver} will be installed to: ${EXT_SITE_PACKAGES_DIR}")
        add_custom_target(${pkg} ALL)
        add_custom_command(TARGET ${pkg} PRE_BUILD
            COMMAND pip install --quiet 
                                --disable-pip-version-check
                                --install-option="--prefix=${EXT_DIST_DIR}" 
                                -I ${pkg}==${ver}
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
# Patch

add_custom_target(patch)
if(WIN32)
	add_executable(patch_exec patch.cpp)
	set(PATCH_COMMAND $<TARGET_FILE:patch_exec>)
	add_dependencies(patch patch_exec)
else()
	set(PATCH_COMMAND "patch")
endif()

###############################################################################
# C/C++ common options

macro(find_lib_package lib ver)
	if(NOT OCIO_INSTALL_EXT STREQUAL "ALL")
		set(_LIB_FIND_ARGS ${lib} ${ver})
		if(OCIO_INSTALL_EXT STREQUAL "NONE")
			list(APPEND _LIB_FIND_ARGS REQUIRED)
		endif()
		find_package(${_LIB_FIND_ARGS})
	endif()
endmacro()

function(print_lib_info lib ver)
	message(STATUS "${lib} ${ver} will be installed to: ${EXT_DIST_DIR}")
	if(NOT BUILD_SHARED_LIBS)
		#TODO: Find a way to merge in the static libs when built with ext installed libs
		message(WARNING "Building STATIC libOpenColorIO. "
						"${lib}'s symbols are NOT included in the output binary!")
	endif()
endfunction()

if(UNIX)
    set(EXT_C_FLAGS "${EXT_C_FLAGS} -fPIC -fvisibility=hidden")
    set(EXT_CXX_FLAGS "${EXT_CXX_FLAGS} -fPIC -fvisibility=hidden")
    if(OCIO_INLINES_HIDDEN)
        set(EXT_CXX_FLAGS "${EXT_CXX_FLAGS} -fvisibility-inlines-hidden")
    endif()
endif()

###############################################################################
# Expat

add_library(libexpat UNKNOWN IMPORTED GLOBAL)

set(LIBEXPAT_MIN_VERSION 2.2.5)

find_lib_package(Expat ${LIBEXPAT_MIN_VERSION})

if((NOT LIBEXPAT_FOUND AND OCIO_INSTALL_EXT STREQUAL "MISSING") 
        OR (OCIO_INSTALL_EXT STREQUAL "ALL"))

	if(GET_LIBEXPAT_VERSION AND GET_LIBEXPAT_VERSION VERSION_GREATER_EQUAL LIBEXPAT_MIN_VERSION)
		set(LIBEXPAT_VERSION ${GET_LIBEXPAT_VERSION})
	else()
		set(LIBEXPAT_VERSION ${LIBEXPAT_MIN_VERSION})
	endif()
    string(REPLACE "." "_" LIBEXPAT_VERSION_U ${LIBEXPAT_VERSION})
	print_lib_info(Expat ${LIBEXPAT_VERSION})

    string(STRIP "${CMAKE_C_FLAGS} ${EXT_C_FLAGS} ${LIBEXPAT_C_FLAGS}" LIBEXPAT_C_FLAGS)
    string(STRIP "${CMAKE_CXX_FLAGS} ${EXT_CXX_FLAGS} ${LIBEXPAT_CXX_FLAGS}" LIBEXPAT_CXX_FLAGS)

    set(LIBEXPAT_CMAKE_ARGS
	    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	    -DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
        -DBUILD_examples:BOOL=OFF
        -DBUILD_tests:BOOL=OFF
        -DBUILD_shared:BOOL=OFF
        -DCMAKE_C_FLAGS=${LIBEXPAT_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${LIBEXPAT_CXX_FLAGS}
		-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    )
    if(CMAKE_TOOLCHAIN_FILE)
        set(LIBEXPAT_CMAKE_ARGS 
            ${LIBEXPAT_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()

    set(LIBEXPAT_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
    set(LIBEXPAT_LIBRARY 
        "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX}")

    ExternalProject_Add(ext_libexpat
        GIT_REPOSITORY "https://github.com/libexpat/libexpat.git"
        GIT_TAG "R_${LIBEXPAT_VERSION_U}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/libexpat"
        BUILD_BYPRODUCTS "${LIBEXPAT_LIBRARY}"
        SOURCE_SUBDIR expat
        CMAKE_ARGS ${LIBEXPAT_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
    )
    add_dependencies(libexpat ext_libexpat)
endif()

set_property(TARGET libexpat PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LIBEXPAT_INCLUDE_DIRS})
set_property(TARGET libexpat PROPERTY IMPORTED_LOCATION ${LIBEXPAT_LIBRARY})

###############################################################################
# Yaml-cpp

add_library(yamlcpp UNKNOWN IMPORTED GLOBAL)

set(YAMLCPP_MIN_VERSION 0.3.0)

find_lib_package(YamlCpp ${YAMLCPP_MIN_VERSION})

if((NOT YAMLCPP_FOUND AND OCIO_INSTALL_EXT STREQUAL "MISSING") 
        OR (OCIO_INSTALL_EXT STREQUAL "ALL"))

	if(GET_YAMLCPP_VERSION AND GET_YAMLCPP_VERSION VERSION_GREATER_EQUAL YAMLCPP_MIN_VERSION)
		set(YAMLCPP_VERSION ${GET_YAMLCPP_VERSION})
	else()
		set(YAMLCPP_VERSION ${YAMLCPP_MIN_VERSION})
	endif()
	string(REPLACE "." ";" YAMLCPP_VERSION_LIST ${YAMLCPP_VERSION})
    list(GET YAMLCPP_VERSION_LIST 0 YAMLCPP_VERSION_MAJOR)
    list(GET YAMLCPP_VERSION_LIST 1 YAMLCPP_VERSION_MINOR)
    list(GET YAMLCPP_VERSION_LIST 2 YAMLCPP_VERSION_PATCH)
	print_lib_info(Yaml-cpp ${YAMLCPP_VERSION})

	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		# C++11 deprecates std::auto_ptr
		set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} -Wno-deprecated-declarations")
	endif()

    string(STRIP "${CMAKE_CXX_FLAGS} ${EXT_CXX_FLAGS} ${YAMLCPP_CXX_FLAGS}" YAMLCPP_CXX_FLAGS)

	set(YAMLCPP_CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DYAML_CPP_BUILD_TESTS:BOOL=OFF
		-DYAML_CPP_BUILD_TOOLS:BOOL=OFF
		-DYAML_CPP_BUILD_CONTRIB:BOOL=OFF
		-DCMAKE_CXX_FLAGS=${YAMLCPP_CXX_FLAGS}
		-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	)
	if(CMAKE_TOOLCHAIN_FILE)
		set(YAMLCPP_CMAKE_ARGS 
		    ${YAMLCPP_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
	endif()

    set(YAMLCPP_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
    set(YAMLCPP_LIBRARY 
        "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

	if(YAMLCPP_VERSION_MINOR LESS 6 AND YAMLCPP_VERSION_PATCH LESS 3)
        set(YAMLCPP_GIT_TAG "release-${YAMLCPP_VERSION}")
    else()
        set(YAMLCPP_GIT_TAG "yaml-cpp-${YAMLCPP_VERSION}")
    endif()

	ExternalProject_Add(ext_yamlcpp
        GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
        GIT_TAG ${YAMLCPP_GIT_TAG}
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/yaml-cpp"
        BUILD_BYPRODUCTS "${YAMLCPP_LIBRARY}"
		CMAKE_ARGS ${YAMLCPP_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
    )
	add_dependencies(yamlcpp ext_yamlcpp)
endif()

if(YAMLCPP_VERSION VERSION_LESS "0.5.0")
	set_property(TARGET yamlcpp PROPERTY INTERFACE_COMPILE_DEFINITIONS "OLDYAML")
endif()
set_property(TARGET yamlcpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${YAMLCPP_INCLUDE_DIRS})
set_property(TARGET yamlcpp PROPERTY IMPORTED_LOCATION ${YAMLCPP_LIBRARY})

###############################################################################
# LCMS2

add_library(lcms2 UNKNOWN IMPORTED GLOBAL)

set(LCMS2_MIN_VERSION 2.2)

find_lib_package(LCMS2 ${LCMS2_MIN_VERSION})

if((NOT LCMS2_FOUND AND OCIO_INSTALL_EXT STREQUAL "MISSING") 
        OR (OCIO_INSTALL_EXT STREQUAL "ALL"))

	if(GET_LCMS2_VERSION AND GET_LCMS2_VERSION VERSION_GREATER_EQUAL LCMS2_MIN_VERSION)
		set(LCMS2_VERSION ${GET_LCMS2_VERSION})
	else()
		set(LCMS2_VERSION ${LCMS2_MIN_VERSION})
	endif()
	print_lib_info(LCMS2 ${LCMS2_VERSION})

	string(STRIP "${CMAKE_C_FLAGS} ${EXT_C_FLAGS} ${LCMS2_C_FLAGS}" LCMS2_C_FLAGS)

	set(LCMS2_CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DCMAKE_C_FLAGS=${LCMS2_C_FLAGS}
	)
	if(CMAKE_TOOLCHAIN_FILE)
		set(LCMS2_CMAKE_ARGS 
		    ${LCMS2_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
	endif()

	set(LCMS2_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
	set(LCMS2_LIBRARY 
	    "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")

	ExternalProject_Add(ext_lcms2
        GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
        GIT_TAG "lcms${LCMS2_VERSION}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/Little-CMS"
        BUILD_BYPRODUCTS "${LCMS2_LIBRARY}"
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_MODULE_PATH}/BuildLCMS2.cmake" "CMakeLists.txt"
        CMAKE_ARGS ${LCMS2_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
    )
	add_dependencies(lcms2 ext_lcms2)
endif()

set_property(TARGET lcms2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LCMS2_INCLUDE_DIRS})
set_property(TARGET lcms2 PROPERTY IMPORTED_LOCATION ${LCMS2_LIBRARY})

###############################################################################
# IlmBase (Half part only)

add_library(ilmbase UNKNOWN IMPORTED GLOBAL)

set(ILMBASE_MIN_VERSION 2.3.0)

find_lib_package(IlmBase ${ILMBASE_MIN_VERSION})

if((NOT ILMBASE_FOUND AND OCIO_INSTALL_EXT STREQUAL "MISSING") 
        OR (OCIO_INSTALL_EXT STREQUAL "ALL"))

	if(GET_ILMBASE_VERSION AND GET_ILMBASE_VERSION VERSION_GREATER_EQUAL ILMBASE_MIN_VERSION)
		set(ILMBASE_VERSION ${GET_ILMBASE_VERSION})
	else()
		set(ILMBASE_VERSION ${ILMBASE_MIN_VERSION})
	endif()
	print_lib_info(IlmBase ${ILMBASE_VERSION})

	string(STRIP "${CMAKE_CXX_FLAGS} ${EXT_CXX_FLAGS} ${ILMBASE_CXX_FLAGS}" ILMBASE_CXX_FLAGS)

	set(ILMBASE_CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${EXT_DIST_DIR}
		-DOPENEXR_BUILD_OPENEXR:BOOL=OFF
		-DOPENEXR_BUILD_PYTHON_LIBS:BOOL=OFF
		-DOPENEXR_BUILD_SHARED:BOOL=OFF
		-DOPENEXR_BUILD_STATIC:BOOL=ON
		-DOPENEXR_ENABLE_TESTS:BOOL=OFF
		-DCMAKE_CXX_FLAGS=${ILMBASE_CXX_FLAGS}
		-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	)
	if(CMAKE_TOOLCHAIN_FILE)
		set(ILMBASE_CMAKE_ARGS 
		    ${ILMBASE_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
	endif()

	set(ILMBASE_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
	set(ILMBASE_LIBRARY 
	    "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}ilmbase${CMAKE_STATIC_LIBRARY_SUFFIX}")

	ExternalProject_Add(ext_ilmbase
        GIT_REPOSITORY "https://github.com/openexr/openexr.git"
        GIT_TAG "v${ILMBASE_VERSION}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_DIR}/openexr"
        BUILD_BYPRODUCTS "${ILMBASE_LIBRARY}"
        CMAKE_ARGS ${ILMBASE_CMAKE_ARGS}
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --target Half_static
        INSTALL_COMMAND ${CMAKE_COMMAND} -P "IlmBase/Half/cmake_install.cmake"
		EXCLUDE_FROM_ALL TRUE
    )
	add_dependencies(ilmbase ext_ilmbase)
endif()

set_property(TARGET ilmbase PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIRS})
set_property(TARGET ilmbase PROPERTY IMPORTED_LOCATION ${ILMBASE_LIBRARY})

###############################################################################
# sampleicc
add_library(sampleicc INTERFACE IMPORTED GLOBAL)
set_property(TARGET sampleicc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include")
