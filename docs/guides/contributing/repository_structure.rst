..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _repository-structure:

Repository structure
====================

The OpenColorIO repository has a relatively straight-forward structure, and a 
simple branching and merging strategy.

All development work is done directly on the main branch. This represents 
the bleeding-edge of the project and any contributions should be done on top of 
it.

After sufficient work is done on the main branch and OCIO leadership 
determines that a release is due, we will bump the relevant internal versioning 
and tag a commit with the corresponding version number, e.g. v2.0.1. Each Minor 
version also has its own “Release Branch”, e.g. RB-1.1. This marks a branch of 
code dedicated to that Major.Minor version, which allows upstream bug fixes to 
be cherry-picked to a given version while still allowing the main branch to 
continue forward onto higher versions. This basic repository structure keeps 
maintenance low, while remaining simple to understand.

Root
****

The root repository directory contains two groups of files:

1. Configuration files used by Git and other cloud services that interface with 
   the OpenColorIO GitHub account. 

2. Important \*.md documentation on the OpenColorIO project's license, 
   governance, and operational policies.

CMake configuration must be initiated from this directory, which is the .git 
root when cloning the OpenColorIO repository.

.github/workflows
*****************

This directory contains the project GitHub Actions configuration, which defines 
continuous integration (CI) workflows for all supported platforms. Each .yml 
file defines one GHA workflow, which can contain one or more jobs and the 
repository events that trigger the workflow.

ASWF
****

This subdirectory contains important ASWF governance documents like the 
OpenColorIO charter, CLAs and DCO reference.

The ``meetings`` subdirectory contains historical meeting minutes for 
TSC (Technical Steering Committee) and working group meetings.  More 
recent TSC and working group meeting notes are stored on the 
`ASWF Wiki. <https://wiki.aswf.io/display/OCIO/Meetings>`_

docs
****

This directory contains the OpenColorIO documentation source and build 
configuration. 

See :ref:`documentation-guidelines` for information on contributing to OCIO
documentation.

ext
***

Houses modified external dependencies which are permissible to store within 
the OpenColorIO repository. Permissible dependencies have compatible FOSS
licenses which have documented approval by the ASWF governing board.

include
*******

This directory contains the OpenColorIO public C++ headers, which define the 
publicly accessible interface (API) for OCIO. This is the best ground truth 
reference available for developers wanting to explore an OpenColorIO 
implementation in their application.

share
*****

This directory contains supplementary resources for building and using 
OpenColorIO. This includes scripts used by OCIO CI, CMake, and other build 
system and example components. Dockerfiles are provided to assist with 
container-based OpenColorIO builds.

The ``cmake`` subdirectory contains all CMake modules and macros used by the 
OCIO build system, and is the best reference for troubleshooting CMake package
finding issues.

src
***

This directory contains the OpenColorIO source code and is broken into the 
following subdirectories:

- **OpenColorIO**: Core library source, including all code related to 
  transforms, ops, and file format IO.

- **apps**: Source for each OCIO command-line tool or "app" (e.g. 
  ``ociobakelut``). These tools also provide excellent example usage of 
  integrating the OCIO library in a C++ application.

- **apputils**: Supporting utility code shared by the OpenColorIO apps.

- **bindings**: OpenColorIO language bindings for Python and Java (note that 
  the OCIO Java bindings are not actively maintained. Contributions to update
  this work is welcome and appreciated).

- **libutils**: Supplementary and example helper libraries for integrating 
  OpenColorIO into applications. Utilities for using OCIO with OpenGL
  and with OpenImageIO/OpenEXR are included.
  
- **utils**: Common header-only utilities shared by the core OpenColorIO 
  library and the other utility sources.

tests
*****

This directory contains the OpenColorIO test suite, both for C++ and all 
supported bindings. Each core \*.cpp source file has a corresponding 
\*_tests.cpp file with all of its tests. Separate tests suites for CPU and GPU 
functionality are provided since a GPU may not always be available. The 
``tests/data/files`` subdirectory contains a wide array of LUT files and other 
raw data for testing file transforms.

vendor
******

This directory contains source for plugins to integrate OpenColorIO into 
third-party software and DCCs (e.g. Photoshop). Software vendors are encouraged 
to contribute their OCIO integrations back to the project at this location if 
practical (e.g. plugin source).
