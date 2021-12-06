..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _unit-tests:

Unit tests
==========

All functionality in OpenColorIO must be covered by an automated test. Tests 
should be implemented in a separate but clearly associated source file under 
the tests subdirectory (e.g. tests for ``src/OpenColorIO/Context.cpp`` are 
located in ``tests/cpu/Context_tests.cpp``). This test suite is collectively 
expected to validate the behavior of every part of OCIO:

* Any new functionality should be accompanied by a test that validates its 
  behavior.

* Any change to existing functionality should have tests added if they don't 
  already exist.

The test should should be run, via ``ctest``, before submitting a pull request.
Pull requests will not be merged until tests are present, running and passing 
as part of the OpenColorIO CI system.

For verbose test output (listing each test and result status), run::

  ctest -V

Test framework
==============

C++
***

The OCIO C++ test framework is based on the `OpenImageIO test framework
<https://github.com/OpenImageIO/oiio/blob/master/src/include/OpenImageIO/unittest.h>`__.

The macros defined in `UnitTest.h 
<https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/main/tests/testutils/UnitTest.h>`__
are used to define tests and assert expected behavior:

* ``OCIO_ADD_TEST``: Define a test and add it to the named test ``group``. 
  Added tests will execute whenever ``ctest`` is run. Test groups and names 
  should be clearly named for logical test execution and identification of 
  failures that occur.

* ``OCIO_CHECK_*``: Macros for assertion of expected test results and behavior.
  Checks are provided for result comparison, exception handling, and various 
  degrees of numeric precision validation.

C++ unit test source files are organized into the following subfolders:

* **apphelpers**: Tests for utilities in ``src/libutils/apphelpers``.

* **cpu**: Main OCIO library tests, including functionality which utilizes the 
  CPU renderer.

* **gpu**: GPU-specific tests, which must be run on supported graphics 
  hardware. GPU tests are gated behind a dedicated CMake option 
  ``OCIO_BUILD_GPU_TESTS`` (which is ``ON`` by default).

* **utils**: Tests for utilities in ``src/utils``.

Python
******

The OCIO Python test suite uses the Python-bundled `unittest 
<https://docs.python.org/3.7/library/unittest.html>`__ framework. Each bound 
class has a dedicated ``TestCase`` class with a ``test_*`` method per specific 
test.

Test cases must be imported into `OpenColorIOTestSuite.py 
<https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/main/tests/python/OpenColorIOTestSuite.py>`__
and added to the ``suite`` method to be included in ``ctest`` execution.

Java
****

The OCIO Java test suite uses the `JUnit <https://junit.org/>`__ framework. 
Each bound class extends the ``TestCase`` class with individual assertions begin 
defined in a single ``test_interface`` method.

.. note::
  The OCIO Java bindings have not been actively maintained so the test suite is
  incompatible with the current OpenColorIO API. Contributions to restore 
  support for Java in OCIO are welcome. See `GitHub issue #950 
  <https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/950>`__ for 
  more information.
