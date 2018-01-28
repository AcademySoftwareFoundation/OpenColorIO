
###############################################################################
### JNI JUNIT TESTS ###

file(GLOB _TESTS_FILES "org/OpenColorIO/*.java")
set(_JCLASS_PATH "${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}/..:${CMAKE_SOURCE_DIR}/ext/junit-4.9b4.jar")
set(_JLIB_PATH "${CMAKE_CURRENT_BINARY_DIR}/..")

add_custom_target(JNITests
                  COMMAND javac -cp ${_JCLASS_PATH}
                    -d ${CMAKE_CURRENT_BINARY_DIR} ${_TESTS_FILES}
                  COMMAND java -cp ${_JCLASS_PATH}
                    -Djava.library.path=${_JLIB_PATH} junit.textui.TestRunner
                    OpenColorIOTestSuite
                  DEPENDS OpenColorIO-JNI
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Setting Up and Running JNI OCIO JUNIT tests")

add_test(jniglue "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target JNITests)
