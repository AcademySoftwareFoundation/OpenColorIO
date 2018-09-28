find_package(PythonInterp 2.7 REQUIRED)

add_test(test_python ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/OpenColorIOTestSuite.py ${CMAKE_BINARY_DIR} ${CMAKE_BUILD_TYPE})

