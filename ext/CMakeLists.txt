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

###############################################################################
### External C/C++ dependencies ###

# Expat
# https://libexpat.github.io
set(EXPAT_MIN_VERSION 2.2.5)
find_package(Expat ${EXPAT_MIN_VERSION} REQUIRED)

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
set(YAMLCPP_MIN_VERSION 0.3.0)
find_package(YamlCpp ${YAMLCPP_MIN_VERSION} REQUIRED)

# IlmBase (Half part only)
# https://www.openexr.com
set(ILMBASE_MIN_VERSION 2.3.0)
find_package(IlmBase ${ILMBASE_MIN_VERSION} REQUIRED)

# pystring
# https://github.com/imageworks/pystring
set(PYSTRING_MIN_VERSION 1.1.3)
find_package(Pystring ${PYSTRING_MIN_VERSION} REQUIRED)

# LCMS2
# http://www.littlecms.com
set(LCMS2_MIN_VERSION 2.2)
set(LCMS2_FIND_ARGS LCMS2 ${LCMS2_MIN_VERSION})
if(TARGET ociobakelut)
    list(APPEND ${LCMS2_FIND_ARGS} REQUIRED)
endif()
find_package(${LCMS2_FIND_ARGS})

###############################################################################
### sampleicc ###
add_library(sampleicc INTERFACE IMPORTED GLOBAL)
set_property(TARGET sampleicc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include")
