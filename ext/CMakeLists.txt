include(ExternalProject)

set(LIB_RT_SUFFIX "")
if (MSVC)
	set(LIB_RT_SUFFIX "md")
	if(NOT ${MSVC_SHARED_RT}) # User wants to have static runtime libraries (/MT, /ML)
		if(${MSVC_STHREADED_RT}) # User wants to have old single-threaded static runtime libraries
			set(LIB_RT_SUFFIX "ml")
		else()
			set(LIB_RT_SUFFIX "mt")
		endif()
	endif()
endif()

######################################################################
# Patch
add_custom_target(patch)
if(WIN32)
	add_executable(patch_exec patch.cpp)
	set(PATCH_COMMAND $<TARGET_FILE:patch_exec>)
	add_dependencies(patch patch_exec)
else()
	set(PATCH_COMMAND "patch")
endif()


######################################################################
# TinyXML
add_library(tinyxml INTERFACE IMPORTED GLOBAL)
if(USE_EXTERNAL_TINYXML)
	find_package(TinyXML 2.6.1 REQUIRED)
else()
	set(TINYXML_INSTALL_DIR "${CMAKE_BINARY_DIR}/ext/dist")
	set(TINYXML_CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DCMAKE_INSTALL_PREFIX=${TINYXML_INSTALL_DIR} 
		-DOCIO_INLINES_HIDDEN:BOOL=${OCIO_INLINES_HIDDEN}
	)
	if(CMAKE_TOOLCHAIN_FILE)
		set(TINYXML_CMAKE_ARGS 
			${TINYXML_CMAKE_ARGS}
			-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
		)
	endif()

	if(NOT BUILD_SHARED_LIBS)
		#TODO: Find a way to merge in the static libs when built with internal yamlcpp
		message(WARNING " Building STATIC libOpenColorIO using the in-built TinyXML. TinyXML's symbols are NOT included in the output binary!")
	endif()

	ExternalProject_Add(TinyXML
		URL "${CMAKE_SOURCE_DIR}/ext/tinyxml_2_6_1.tar.gz"
		PATCH_COMMAND ${PATCH_COMMAND} -f -p 1 < "${CMAKE_SOURCE_DIR}/ext/tinyxml_2_6_1.patch"
		BINARY_DIR "${CMAKE_BINARY_DIR}/ext/build/tinyxml"
		INSTALL_DIR "${TINYXML_INSTALL_DIR}"
		CMAKE_ARGS ${TINYXML_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
	)
	set(TINYXML_INCLUDE_DIRS "${TINYXML_INSTALL_DIR}/include")
	set(TINYXML_LIBRARIES "${TINYXML_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml${CMAKE_STATIC_LIBRARY_SUFFIX}")
	file(MAKE_DIRECTORY ${TINYXML_INCLUDE_DIRS}) # Hack to let imported target be built from ExternalProject_Add
	add_dependencies(TinyXML patch)
	add_dependencies(tinyxml TinyXML)
endif()

set_property(TARGET tinyxml PROPERTY INTERFACE_LINK_LIBRARIES ${TINYXML_LIBRARIES})
set_property(TARGET tinyxml PROPERTY INTERFACE_COMPILE_DEFINITIONS "TIXML_USE_STL")
set_property(TARGET tinyxml PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${TINYXML_INCLUDE_DIRS})


######################################################################
# YamlCpp

add_library(yamlcpp INTERFACE IMPORTED GLOBAL)
if(USE_EXTERNAL_YAML)
	find_package(YamlCpp 0.3.0 REQUIRED)
else()
	set(YAMLCPP_VERSION "0.3.0")
	set(YAML_CPP_INSTALL_DIR "${CMAKE_BINARY_DIR}/ext/dist")
	set(YAML_CPP_CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DCMAKE_INSTALL_PREFIX=${YAML_CPP_INSTALL_DIR}
		-DYAML_CPP_BUILD_TOOLS:BOOL=FALSE
		-DOCIO_INLINES_HIDDEN:BOOL=${OCIO_INLINES_HIDDEN}
	)
	if(CMAKE_TOOLCHAIN_FILE)
		set(YAML_CPP_CMAKE_ARGS
			${YAML_CPP_CMAKE_ARGS}
			-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
		)
	endif()

	if(NOT BUILD_SHARED_LIBS)
		#TODO: Find a way to merge in the static libs when built with internal yamlcpp
		message(WARNING " Building STATIC libOpenColorIO using the in-built Yaml-cpp. Yaml-cpp's symbols are NOT included in the output binary!")
	endif()
	ExternalProject_Add(Yaml-cpp
		URL "${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAMLCPP_VERSION}.tar.gz"
		PATCH_COMMAND ${PATCH_COMMAND} -f -p 1 < "${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAMLCPP_VERSION}.patch"
		BINARY_DIR "${CMAKE_BINARY_DIR}/ext/build/yaml-cpp"
		INSTALL_DIR "${YAML_CPP_INSTALL_DIR}"
		CMAKE_ARGS ${YAML_CPP_CMAKE_ARGS}
		EXCLUDE_FROM_ALL TRUE
	)
	set(YAMLCPP_INCLUDE_DIRS "${YAML_CPP_INSTALL_DIR}/include")
	set(YAMLCPP_LIBNAME "libyaml-cpp${LIB_RT_SUFFIX}")
	set(YAMLCPP_LIBRARY "${YAML_CPP_INSTALL_DIR}/lib/${YAMLCPP_LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
	file(MAKE_DIRECTORY ${YAMLCPP_INCLUDE_DIRS}) # Hack to let imported target be built from ExternalProject_Add
	add_dependencies(Yaml-cpp patch)
	add_dependencies(yamlcpp Yaml-cpp)
endif()

if(YAMLCPP_VERSION VERSION_LESS "0.5.0")
	set_property(TARGET yamlcpp PROPERTY INTERFACE_COMPILE_DEFINITIONS "OLDYAML")
endif()
set_property(TARGET yamlcpp PROPERTY INTERFACE_LINK_LIBRARIES ${YAMLCPP_LIBRARY})
set_property(TARGET yamlcpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${YAMLCPP_INCLUDE_DIRS})

######################################################################
# LCMS2

add_library(lcms2 INTERFACE IMPORTED GLOBAL)
if(USE_EXTERNAL_LCMS)
	find_package(LCMS2 2.1 REQUIRED)
else()
	set(LCMS2_INSTALL_DIR "${CMAKE_BINARY_DIR}/ext/dist")
	set(LCMS2_CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DCMAKE_INSTALL_PREFIX=${LCMS2_INSTALL_DIR}
	)
	ExternalProject_Add(LCMS2
		URL "${CMAKE_SOURCE_DIR}/ext/lcms2-2.1.tar.gz"
		PATCH_COMMAND ${PATCH_COMMAND} -f -p 1 < "${CMAKE_SOURCE_DIR}/ext/lcms2-2.1.patch"
		BINARY_DIR "${CMAKE_BINARY_DIR}/ext/build/lcms2"
		INSTALL_DIR "${LCMS2_INSTALL_DIR}"
		CMAKE_ARGS ${LCMS2_CMAKE_ARGS}
	)
	set(LCMS2_INCLUDE_DIRS "${LCMS2_INSTALL_DIR}/include")
	set(LCMS2_LIBRARY ${LCMS2_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX})
	file(MAKE_DIRECTORY ${LCMS2_INCLUDE_DIRS}) # Hack to let imported target be built from ExternalProject_Add
	add_dependencies(lcms2 LCMS2)
endif()
set_property(TARGET lcms2 PROPERTY INTERFACE_LINK_LIBRARIES ${LCMS2_LIBRARY})
set_property(TARGET lcms2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LCMS2_INCLUDE_DIRS})


######################################################################
# sampleicc
add_library(sampleicc INTERFACE IMPORTED GLOBAL)
set_property(TARGET sampleicc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include")


######################################################################
# oiio unit tests
add_library(oiio_unittest INTERFACE IMPORTED GLOBAL)
set_property(TARGET oiio_unittest PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/oiio/src/include")
