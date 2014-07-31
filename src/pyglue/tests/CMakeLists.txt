
###############################################################################
### PYTHON UNIT TESTS ###

add_custom_target(PYTests
                  COMMAND ${PYTHON} OpenColorIOTestSuite.py ${CMAKE_BINARY_DIR}
                  DEPENDS PyOpenColorIO
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  COMMENT "Setting Up and Running PYTHON OCIO UNIT tests")

add_test(pyglue "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target PYTests)