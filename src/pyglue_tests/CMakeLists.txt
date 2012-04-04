# setuptools (duplicated from docs/CMakeLists.txt)
set(SETUPTOOLS_VERSION 0.6c11)
ExternalProject_Add(setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/setuptools-${SETUPTOOLS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} python setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} python setup.py install --prefix=${EXTDIST_ROOT}
)

# nosetests for test running
set(NOSE_VERSION 1.1.2)
ExternalProject_Add(nose
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/nose-${NOSE_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} python setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} python setup.py install --prefix=${EXTDIST_ROOT} --install-scripts=${EXTDIST_ROOT}/bin
)

# Create shell script to run tests
message(STATUS "Create ocio_pyglue_tests.sh from ocio_pyglue_tests.sh.in")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_pyglue_tests.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/ocio_pyglue_tests.sh @ONLY)

# Register with ctest (for "make test" or "ctest" commands)
add_test(NAME ocio_pyglue_tests
    COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_pyglue_tests.sh
    )
