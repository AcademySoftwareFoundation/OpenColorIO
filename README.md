<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

OpenColorIO
===========

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![CI Status](https://github.com/AcademySoftwareFoundation/OpenColorIO/workflows/CI/badge.svg)](https://github.com/AcademySoftwareFoundation/OpenColorIO/actions?query=workflow%3ACI)
[![GPU CI Status](https://github.com/AcademySoftwareFoundation/OpenColorIO/workflows/GPU/badge.svg)](https://github.com/AcademySoftwareFoundation/OpenColorIO/actions?query=workflow%3AGPU)
[![Analysis Status](https://github.com/AcademySoftwareFoundation/OpenColorIO/workflows/Analysis/badge.svg)](https://github.com/AcademySoftwareFoundation/OpenColorIO/actions?query=workflow%3AAnalysis)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=AcademySoftwareFoundation_OpenColorIO&metric=alert_status)](https://sonarcloud.io/dashboard?id=AcademySoftwareFoundation_OpenColorIO)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2612/badge)](https://bestpractices.coreinfrastructure.org/projects/2612)

Introduction
------------

[linear]: https://origin-flash.sonypictures.com/ist/imageworks/cloudy1.jpg
[log]: https://origin-flash.sonypictures.com/ist/imageworks/cloudy3.jpg
[vd]: https://origin-flash.sonypictures.com/ist/imageworks/cloudy2.jpg

![lnh][linear] ![lm10][log] ![vd8][vd]

OpenColorIO (OCIO) is a complete color management solution geared towards
motion picture production with an emphasis on visual effects and computer
animation. OCIO provides a straightforward and consistent user experience
across all supporting applications while allowing for sophisticated back-end
configuration options suitable for high-end production usage. OCIO is
compatible with the Academy Color Encoding Specification (ACES) and is
LUT-format agnostic, supporting many popular formats.

OpenColorIO is released as version 1.0 and has been in development since 2003.
OCIO represents the culmination of years of production experience earned on
such films as SpiderMan 2 (2004), Surf's Up (2007), Cloudy with a Chance of
Meatballs (2009), Alice in Wonderland (2010), and many more. OpenColorIO is
natively supported in commercial applications like Katana, Mari, Nuke, Maya,
Houdini, Silhouette FX, and
[others](https://opencolorio.org/CompatibleSoftware.html).

OpenColorIO is free and open source software ([LICENSE](LICENSE)), and
one of several projects actvively sponsored by the ASWF
([Academy Software Foundation](https://www.aswf.io/)).

OpenColorIO Project Mission
---------------------------

The OpenColorIO project is committed to providing an industry standard solution 
for highly precise, performant, and consistent color management across digital 
content creation applications and pipelines.

OpenColorIO aims to:

* be stable, secure, and thouroughly tested on Linux, macOS, and Windows
* be performant on modern CPUs and GPUs
* be simple, scalable, and well documented
* be compatible with critical color and imaging standards
* provide lossless color processing wherever possible
* maintain config backwards compatability across major versions
* have every new feature carefully reviewed by leaders from the motion picture, 
  VFX, animation, and video game industries
* have a healthy and active community
* receive wide industry adoption

OpenColorIO Project Governance
------------------------------

OpenColorIO is governed by the Academy Software Foundation (ASWF). See 
[GOVERNANCE.md](GOVERNANCE.md) for detailed infomation about how the project 
operates.

Web Resources
-------------

* Website: <http://opencolorio.org>
* Mailing lists:
  * Developer: <ocio-dev@lists.aswf.io>
  * User: <ocio-user@lists.aswf.io>
* Slack workspace: <https://opencolorio.slack.com>
  * New users can join via <http://slack.opencolorio.org>

Reference Configs
-----------------

Reference OCIO configuration files and associated LUTs can be found at the
Imageworks [OpenColorIO-Configs](https://github.com/imageworks/OpenColorIO-Configs)
repository.

The following reference implementations are provided:

* SPI: Sony Pictures Imageworks
  * spi-anim
  * spi-vfx
* ACES: Academy Color Encoding System
  * aces_1.0.3
  * aces_1.0.2
  * aces_1.0.1
  * aces_0.7.1
  * aces_0.1.1
* Other
  * nuke-default

Acknowledgements
----------------

OpenColorIO represents the generous contributions of many organizations and
individuals. The "Contributors to the OpenColorIO Project" copyright statement
used throughout the project reflects that the OCIO source is a collaborative
effort, often with multiple copyright holders within a single file. Copyright
for specific portions of code can be traced to an originating contributor using
git commit history.

OpenColorIO was originally developed and made open source by
[Sony Pictures Imageworks](http://opensource.imageworks.com). The core design,
and the majority of OCIO 1.0 code was authored by Imageworks, who continue to
support and contribute to OCIO 2.0 development.

The design and development of OpenColorIO 2.0 is being led by Autodesk.
Autodesk submitted a proposal to revitalize the project in 2017, and have
authored the majority of OCIO 2.0 code in the years since.

Significant contributions have also been made by Industrial Light & Magic,
DNEG, and many individuals. See
[Contributors](https://github.com/AcademySoftwareFoundation/OpenColorIO/graphs/contributors)
for a complete list.

See [THIRD-PARTY.md](THIRD-PARTY.md) for license information
about portions of OpenColorIO that have been imported from other projects.

---
Images from "Cloudy With A Chance of Meatballs" Copyright 2011 Sony Pictures Inc.
All Rights Reserved.
