include(ExternalProject)

# Slightly hacky method to force unittest data extraction to be a target
set(UNITTEST_EXTRACT_DIR "${CMAKE_BINARY_DIR}/testdata/dist")
ExternalProject_Add(UNITTEST_DATA
	URL "${CMAKE_CURRENT_SOURCE_DIR}/unittestsfiles.tar.gz"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/testdata/build"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITTEST_EXTRACT_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/testdata/build ${UNITTEST_EXTRACT_DIR}
	EXCLUDE_FROM_ALL TRUE
)
# Need to add this superfluous file command to allow imported target to depend on ExternalProject
file(MAKE_DIRECTORY ${UNITTEST_EXTRACT_DIR}) 
add_library(unittest_data INTERFACE IMPORTED GLOBAL)
set_property(TARGET unittest_data PROPERTY INTERFACE_COMPILE_DEFINITIONS "OCIO_UNIT_TEST_FILES_DIR=${UNITTEST_EXTRACT_DIR}")
add_dependencies(unittest_data UNITTEST_DATA)
