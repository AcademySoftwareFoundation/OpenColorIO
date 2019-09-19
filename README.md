<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

OpenColorIO
===========

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Build Status](https://dev.azure.com/imageworks/OpenColorIO/_apis/build/status/imageworks.OpenColorIO?branchName=master)](https://dev.azure.com/imageworks/OpenColorIO/_build/latest?definitionId=1&branchName=master)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=imageworks_OpenColorIO&metric=alert_status)](https://sonarcloud.io/dashboard?id=imageworks_OpenColorIO)
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

OpenColorIO is free and open source software ([LICENSE.md](LICENSE.md)), and
one of several projects actvively sponsored by the ASWF
([Academy Software Foundation](https://www.aswf.io/)).

Web Resources
-------------

* Web page: <http://opencolorio.org>
* Mail lists:
  * Developer: <ocio-dev@lists.aswf.io>
  * User: <ocio-user@lists.aswf.io>
* Slack channel: <https://opencolorio.slack.com>
  * Please request access on the ocio-dev list with the email address you
    would like to be invited on.

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
[Contributors](https://github.com/imageworks/OpenColorIO/graphs/contributors)
for a complete list.

See [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md) for license information
about portions of OpenColorIO that have been imported from other projects.

---
Images from "Cloudy With A Chance of Meatballs" Copyright 2011 Sony Pictures Inc.
All Rights Reserved.
