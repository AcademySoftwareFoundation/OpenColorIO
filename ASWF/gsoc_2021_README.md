<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

# OpenColorIO Google Summer of Code 2021

OpenColorIO (OCIO) is a complete color management solution geared towards
motion picture production with an emphasis on visual effects and computer 
animation. OCIO provides a straightforward and consistent user experience
across all supporting applications while allowing for sophisticated back-end
configuration options suitable for high-end production usage. OCIO is compatible
with the Academy Color Encoding Specification (ACES) and is LUT-format agnostic,
supporting many popular formats.

OpenColorIO version 1.0 was released in 2011, having been in development since
2003. OCIO v1 represented the culmination of years of production experience
earned on such films as SpiderMan 2 (2004), Surf’s Up (2007), Cloudy with a
Chance of Meatballs (2009), Alice in Wonderland (2010), and many more.  
OpenColorIO v2 has been in development since 2017 and was feature complete as of
SIGGRAPH 2020. After a stabilization and bug-fixing period, an official 2.0.0 
release was made in January 2021! 

While OCIO is a color management library (written in C++ with a Python API), 
its only knowledge of color science comes from its execution of the transforms 
defined in the OCIO configuration file, written in YAML. These transforms are 
either defined by the end user in a custom OCIO config or inherited from the 
publicly available configs.  By specifying your desired config file in the local 
environment all OCIO compatible applications and software libraries will be able 
to see your defined color transform “universe”. You can then direct the 
transformation of image data from one defined OCIO Color Space to another, in 
addition to the other transforms documented elsewhere - the options are endless!

## About the Academy Software Foundation

The Academy Software Foundation (ASWF) is collaboration between the Linux
Foundation and the Academy of Motion Picture Arts and Sciences, serving as
an umbrella organization that is the home to several key open source
software projects used by the film industry (primarily visual effects and
animation production). The projects we manage are some of the most
high-profile and widely used open source projects originating and used in
film production studios. It's a great opportunity for students to get
involved with projects that will be used professionally on films!

The mission of the ASWF is to increase the quality and quantity of
contributions to the content creation industry's open source software base;
to provide a neutral forum to coordinate cross-project efforts; to provide a
common build and test infrastructure; and to provide individuals and
organizations a clear path to participation in advancing our open source
ecosystem.

In 2020, the ASWF projects OpenEXR, OpenTimelineIO, and OpenCue mentored 
GSoC students. This year, OpenColorIO is the ASWF project that looks forward 
to welcoming and mentoring GSoC students.

## Application Instructions for Students

Please review the CONTRIBUTING guidelines thoroughly before applying. OCIO 
maintains a contributor license agreement (CLA) which must be signed to 
accept contributions.

**Successfully submitting a Pull Request to the OCIO project is a strict**
**requirement prior to submitting an application.**
This is not negotiable. We will not even read applications from students who 
have not submitted an acceptable PR to the project. It can be a truly minimal 
change -- for example, fixing a typo in an error message. You will find issues 
tagged "good first issue" on our GitHub repo, which are ideal candidates for 
your small trial PR to qualify for application.

## Application template

Please use this application template. Feel free to expand or tell us any other
relevant details, but this is a good place to start. The applications will be 
judged by representatives of the OCIO technical steering comittee.

* Student name:
* School / year / area of study:
* Relevant courses, materials studied, jobs, programming projects, skills, other 
open source projects you've contributed to. Go ahead and tell us anything about 
your background that you think we will find interesting for our projects.
* Project proposal - please explain in detail and give specifics on acceptance 
criteria -- how we can measure your success, how we can test your code. Provide 
a weekly timeline for expected progress.
* Please tell us which test PR you had merged into the OCIO repo:

## Project information and suggested ideas list

Home page:  https://opencolorio.org/ <br>
GitHub:     https://github.com/AcademySoftwareFoundation/OpenColorIO <br>
Skills/knowledge relevant to the project: C++11, CMake, color science

Mentor suggestions: Patrick Hodoul, Doug Walker, Michael Dolan

## Project Ideas:

**Add OCIO support to FFmpeg or another FOSS project**
An important effort for OCIO is expanding the libraries and applications where
OCIO is implemented. Users have specifically called out FFmpeg as a library and
command-line tool where they would like to see OCIO support, but any other FOSS 
project leveraged by digital artists or production pipelines could be considered. 
This could also include upgrading an existing implementation to support the OCIO 
v2 API and leverage new functionality.
Special skills: Familiarity with prospective host library or tool, and knowledge 
of working with image buffers and authoring plugins.

**OpenFX Plugin Development**
OCIO currently has a number of plug-ins in the GitHub repo for proprietary 
application interfaces. (And in several cases, these have become out of sync 
with the actual plug-ins shipped by vendors.) It would be great to have OpenFX 
plug-ins in the repo. These would have the benefit of bringing OCIO support to 
a wider range of applications. Furthermore, the existing plug-ins are all based 
on OCIO v1, the new OpenFX plug-ins would be the perfect place to showcase the 
new OCIO v2 features.
Special skills: Access to a host application that supports OpenFX plug-ins and 
ability to build the basic demo plug-ins in the OpenFX SDK.

**Add support for more ICC profile structures**
OCIO currently supports basic ICC monitor profiles.  These are the type that 
are most often set as monitor profiles on Mac and Windows.  But it would be nice 
for OCIO to be able to read and convert some of the more advanced ICC profile 
structures such as the A2B* and B2A* tags.  There are some monitor profiles that
use these tags to implement gamut mapping. OCIO already has operators to support
the most common usage of these tags, so the project is simply about importing a
profile into existing OCIO operators. It would not be a goal of this project to 
import structures that don’t already have an OCIO equivalent, such as 
4-dimensional tables.
