find_package(Java REQUIRED)

set(SOURCES
	org/OpenColorIO/BakerTest.java
	org/OpenColorIO/ColorSpaceTest.java
	org/OpenColorIO/ConfigTest.java
	org/OpenColorIO/ContextTest.java
	org/OpenColorIO/GlobalsTest.java
	org/OpenColorIO/GpuShaderDescTest.java
	org/OpenColorIO/LookTest.java
	org/OpenColorIO/OpenColorIOTestSuite.java
	org/OpenColorIO/PackedImageDescTest.java
	org/OpenColorIO/PlanarImageDescTest.java
	org/OpenColorIO/TransformsTest.java
)
set(_JCLASS_PATH "${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}/../../src/bindings/java:${CMAKE_SOURCE_DIR}/ext/junit-4.9b4.jar")
set(_JLIB_PATH "${CMAKE_CURRENT_BINARY_DIR}/../../src/bindings/java")

set(JAVA_TEST_OUTPUT "bla")

add_custom_command(OUTPUT ${JAVA_TEST_OUTPUT}
	COMMAND ${Java_JAVAC_EXECUTABLE} -cp ${_JCLASS_PATH} -d ${CMAKE_CURRENT_BINARY_DIR} ${SOURCES}
	DEPENDS ${SOURCES} OpenColorIO_JNI
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Compiling .java files"
)

add_test(test_java ${Java_JAVA_EXECUTABLE} -cp ${_JCLASS_PATH} -Djava.library.path=${_JLIB_PATH} junit.textui.TestRunner OpenColorIOTestSuite)
