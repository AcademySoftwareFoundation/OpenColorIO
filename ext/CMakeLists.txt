include(ExternalProject)
include(FindExtPackage)

###############################################################################
### External library install paths ###

set(EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")
set(EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
set(EXT_INCLUDE_DIR "${EXT_DIST_ROOT}/include")
set(EXT_LIBRARY_DIR "${EXT_DIST_ROOT}/lib")
set(EXT_PC_DIR "${EXT_LIBRARY_DIR}/pkgconfig")

if(NOT ENV{PKG_CONFIG_PATH} MATCHES "^${EXT_PC_DIR}:")
	set(ENV{PKG_CONFIG_PATH} "${EXT_PC_DIR}:$ENV{PKG_CONFIG_PATH}")
endif()


###############################################################################
### Common options ###

set(EXT_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(EXT_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(EXT_CMAKE_ARGS 
	${EXT_CMAKE_ARGS} 
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_INSTALL_PREFIX=${EXT_DIST_ROOT}
)
if(CMAKE_TOOLCHAIN_FILE)
	set(EXT_CMAKE_ARGS 
		${EXT_CMAKE_ARGS} 
		-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	)
endif()


###############################################################################
### Expat ###

add_library(expat UNKNOWN IMPORTED GLOBAL)
set(EXPAT_MIN_VERSION 2.2.5)
find_ext_package(EXPAT ${EXPAT_MIN_VERSION})

if(EXPAT_INSTALL_EXT)
    string(STRIP "${EXT_C_FLAGS} ${EXPAT_C_FLAGS}" EXPAT_C_FLAGS)
    string(STRIP "${EXT_CXX_FLAGS} ${EXPAT_CXX_FLAGS}" EXPAT_CXX_FLAGS)

	set(EXPAT_CMAKE_ARGS
		${EXT_CMAKE_ARGS}
        -DBUILD_examples:BOOL=OFF
        -DBUILD_tests:BOOL=OFF
        -DBUILD_shared:BOOL=OFF
        -DCMAKE_C_FLAGS=${EXPAT_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${EXPAT_CXX_FLAGS}
		-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    )

    set(EXPAT_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
    set(EXPAT_LIBRARY 
        "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX}")

    # Hack to let imported target be built from ExternalProject_Add
    file(MAKE_DIRECTORY ${EXPAT_INCLUDE_DIRS})

    message(STATUS "LibExpat ${EXPAT_VERSION} will be installed to: ${EXT_DIST_ROOT}")
    ExternalProject_Add(expat_install
        GIT_REPOSITORY "https://github.com/libexpat/libexpat.git"
        GIT_TAG "R_${EXPAT_VERSION_U}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_ROOT}/libexpat"
        BUILD_BYPRODUCTS ${EXPAT_LIBRARY}
        SOURCE_SUBDIR expat
        CMAKE_ARGS ${EXPAT_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
	)
    add_dependencies(expat expat_install)
endif()

set_property(TARGET expat PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${EXPAT_INCLUDE_DIRS})
set_property(TARGET expat PROPERTY IMPORTED_LOCATION ${EXPAT_LIBRARY})


###############################################################################
### Yaml-cpp ###

add_library(yamlcpp UNKNOWN IMPORTED GLOBAL)
set(YAMLCPP_MIN_VERSION 0.3.0)
find_ext_package(YamlCpp ${YAMLCPP_MIN_VERSION})

if(YAMLCPP_INSTALL_EXT)
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		# C++11 deprecates std::auto_ptr
		set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} -Wno-deprecated-declarations")
	endif()

    if(UNIX)
        set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} -fPIC -fvisibility=hidden")
        if(OCIO_INLINES_HIDDEN)
            set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} -fvisibility-inlines-hidden")
        endif()
    endif()

    string(STRIP "${EXT_CXX_FLAGS} ${YAMLCPP_CXX_FLAGS}" YAMLCPP_CXX_FLAGS)

	set(YAMLCPP_CMAKE_ARGS
		${EXT_CMAKE_ARGS}
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DYAML_CPP_BUILD_TESTS:BOOL=OFF
		-DYAML_CPP_BUILD_TOOLS:BOOL=OFF
		-DYAML_CPP_BUILD_CONTRIB:BOOL=OFF
		-DCMAKE_CXX_FLAGS=${YAMLCPP_CXX_FLAGS}
		-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	)

    if(NOT BUILD_SHARED_LIBS)
		#TODO: Find a way to merge in the static libs when built with internal yamlcpp
		message(WARNING
		    "Building STATIC libOpenColorIO using the in-built Yaml-cpp. "
		    "Yaml-cpp's symbols are NOT included in the output binary!"
        )
	endif()

    set(YAMLCPP_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
    set(YAMLCPP_LIBRARY 
        "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

	# Hack to let imported target be built from ExternalProject_Add
	file(MAKE_DIRECTORY ${YAMLCPP_INCLUDE_DIRS})

	if(YAMLCPP_VERSION_MINOR LESS 6 AND YAMLCPP_VERSION_PATCH LESS 3)
        set(YAMLCPP_GIT_TAG "release-${YAMLCPP_VERSION}")
    else()
        set(YAMLCPP_GIT_TAG "yaml-cpp-${YAMLCPP_VERSION}")
    endif()

    message(STATUS "Yaml-cpp ${YAMLCPP_VERSION} will be installed to: ${EXT_DIST_ROOT}")
	ExternalProject_Add(yamlcpp_install
        GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
        GIT_TAG ${YAMLCPP_GIT_TAG}
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_ROOT}/yaml-cpp"
        BUILD_BYPRODUCTS ${YAMLCPP_LIBRARY}
		CMAKE_ARGS ${YAMLCPP_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
	)
	add_dependencies(yamlcpp yamlcpp_install)
endif()

if(YAMLCPP_VERSION VERSION_LESS "0.5.0")
	set_property(TARGET yamlcpp PROPERTY INTERFACE_COMPILE_DEFINITIONS "OLDYAML")
endif()
set_property(TARGET yamlcpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${YAMLCPP_INCLUDE_DIRS})
set_property(TARGET yamlcpp PROPERTY IMPORTED_LOCATION ${YAMLCPP_LIBRARY})


###############################################################################
### LCMS2 ###

add_library(lcms2 UNKNOWN IMPORTED GLOBAL)
set(LCMS2_MIN_VERSION 2.2)
find_ext_package(LCMS2 ${LCMS2_MIN_VERSION})

if(LCMS2_INSTALL_EXT)
	string(STRIP "${EXT_C_FLAGS} ${LCMS2_C_FLAGS}" LCMS2_C_FLAGS)

	set(LCMS2_CMAKE_ARGS
		${EXT_CMAKE_ARGS}
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DCMAKE_C_FLAGS=${LCMS2_C_FLAGS}
	)

	set(LCMS2_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
	set(LCMS2_LIBRARY 
	    "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")

	# Hack to let imported target be built from ExternalProject_Add
	file(MAKE_DIRECTORY ${LCMS2_INCLUDE_DIRS})

    message(STATUS "LCMS2 ${LCMS2_VERSION} will be installed to: ${EXT_DIST_ROOT}")
	ExternalProject_Add(lcms2_install
        GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
        GIT_TAG "lcms${LCMS2_VERSION}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_ROOT}/Little-CMS"
        BUILD_BYPRODUCTS ${LCMS2_LIBRARY}
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_MODULE_PATH}/BuildLCMS2.cmake" "CMakeLists.txt"
        CMAKE_ARGS ${LCMS2_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
	)
	add_dependencies(lcms2 lcms2_install)
endif()

set_property(TARGET lcms2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LCMS2_INCLUDE_DIRS})
set_property(TARGET lcms2 PROPERTY IMPORTED_LOCATION ${LCMS2_LIBRARY})


###############################################################################
### IlmBase (Half part only) ###

add_library(ilmbase UNKNOWN IMPORTED GLOBAL)
set(ILMBASE_MIN_VERSION 2.3.0)
find_ext_package(IlmBase ${ILMBASE_MIN_VERSION})

if(ILMBASE_INSTALL_EXT)
	string(STRIP "${EXT_CXX_FLAGS} ${ILMBASE_CXX_FLAGS}" ILMBASE_CXX_FLAGS)

	set(ILMBASE_CMAKE_ARGS
		${EXT_CMAKE_ARGS}
		-DOPENEXR_BUILD_OPENEXR:BOOL=OFF
		-DOPENEXR_BUILD_PYTHON_LIBS:BOOL=OFF
		-DOPENEXR_BUILD_SHARED:BOOL=OFF
		-DOPENEXR_BUILD_STATIC:BOOL=ON
		-DOPENEXR_ENABLE_TESTS:BOOL=OFF
		-DCMAKE_CXX_FLAGS=${ILMBASE_CXX_FLAGS}
		-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	)

	set(ILMBASE_INCLUDE_DIRS "${EXT_INCLUDE_DIR}")
	set(ILMBASE_LIBRARY 
	    "${EXT_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}Half-${ILMBASE_VERSION_MAJOR}_${ILMBASE_VERSION_MINOR}_s${CMAKE_STATIC_LIBRARY_SUFFIX}")

	# Hack to let imported target be built from ExternalProject_Add
	file(MAKE_DIRECTORY ${ILMBASE_INCLUDE_DIRS})

    message(STATUS "IlmBase ${ILMBASE_VERSION} will be installed to: ${EXT_DIST_ROOT}")
	ExternalProject_Add(ilmbase_install
        GIT_REPOSITORY "https://github.com/openexr/openexr.git"
        GIT_TAG "v${ILMBASE_VERSION}"
        GIT_SHALLOW TRUE
        PREFIX "${EXT_BUILD_ROOT}/openexr"
        BUILD_BYPRODUCTS ${ILMBASE_LIBRARY}
        CMAKE_ARGS ${ILMBASE_CMAKE_ARGS}
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --target Half_static
        INSTALL_COMMAND ${CMAKE_COMMAND} -P "IlmBase/Half/cmake_install.cmake"
		EXCLUDE_FROM_ALL TRUE
	)
	add_dependencies(ilmbase ilmbase_install)
endif()

set_property(TARGET ilmbase PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIRS})
set_property(TARGET ilmbase PROPERTY IMPORTED_LOCATION ${ILMBASE_LIBRARY})


###############################################################################
### sampleicc ###
add_library(sampleicc INTERFACE IMPORTED GLOBAL)
set_property(TARGET sampleicc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include")
