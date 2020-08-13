..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _aswf-docker:

ASWF Docker
===========

OpenColorIO's CI infrastructure utilizes ASWF-managed docker containers for 
Linux builds. Each image implements a specific `VFX Reference Platform 
<https://vfxplatform.com/>`__ calendar year and all dependencies for a specific 
ASWF project.

For example, the `aswf/ci-ocio:2020 
<https://hub.docker.com/layers/aswf/ci-ocio/2020/images/sha256-1ab7e788cec1524394af561d38e0bfd23e8c50de9c6a83990ab6a2326b510b37?context=explore>`__ 
container provides a build environment with all upstream OpenColorIO 
dependencies and adheres to VFX Reference Platform CY2020. VFX Reference 
Platform calendar years starting with 2018 up to the current draft year 
specification are supported.

These Docker images are available in the `aswf DockerHub repository 
<https://hub.docker.com/u/aswf>`__ for public use. See the table in the 
`aswf-docker source GitHub repository 
<https://github.com/AcademySoftwareFoundation/aswf-docker>`__ for a summary of
all available images.
