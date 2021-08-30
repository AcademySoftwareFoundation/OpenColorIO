<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

This file documents releases up to 1.1.1.  For a description of version 2.0 and later
please refer to the Releases page on GitHub:
https://github.com/AcademySoftwareFoundation/OpenColorIO/releases

**Version 1.1.1 (April 2 2019):**
    * Added optional compatibility for building apps with OpenImageIO 1.9+
    * Added USE_SSE checks to fix Linux build failure
    * getDisplays() result ordering now matches the active_displays config
      definition or OCIO_ACTIVE_DISPLAYS env var override.
    * Fixed incorrect getDefaultDisplay()/getDefaultView() result when
      OCIO_ACTIVE_DISPLAYS or OCIO_ACTIVE_VIEWS env vars are unset.
    * Fixed Windows-specific GetEnv() bug
    * Fixed Windows and MacOS CI failure cases

**Version 1.1.0 (Jan 5 2018):**
    * libc++ build fixes
    * Added support for YAML > 5.0.1
    * YAML and TinyXML patch fixes
    * Clang visibility fix
    * Added write support for Truelight LUTs
    * Improved OCIOYaml
    * Python string corruption fix
    * Added support for CDL
    * Updated documentation
    * Added args/kwargs support to Python MatrixTransform
    * Added description field to Look objects
    * Improved Python 3 compatibility
    * CSP file read fix
    * Added Linux, MacOS, and Windows continuos integration
    * Improved 1D LUT extrapolation
    * Improved 1D LUT negative handling
    * Improved Windows build system
    * Improved cross-platform build system
    * Undefined role crash fix
    * After Effects plugin updated
    * Added reference Photoshop plugin
    * Added reference Docker image

**Version 1.0.9 (Sep 2 2013):**
    * CDL cccid supports both named id and index lookups
    * ociobakelut / ocioconvert updates
    * FreeBSD compile dixes
    * FileTransform disk cache allows concurrent disk lookups
    * CSP windows fix
    * Python 3 support
    * Fix envvar abs/relative path testing
    * Can explicitly declare config envvars
    * gcc44 compile warning fixes

**Version 1.0.8 (Dec 11 2012):**
    * After Effects plugin
    * Core increased precision for matrix inversion
    * Core md5 symbols no longer leaked
    * CMake compatibility with OIIO 1.0 namespacing
    * Cmake option to control python soname
    * Nuke register_viewers defaults OCIODisplay to "all"
    * Nuke ColorLookup <-> spi1d lut examples
    * Windows uses boost shared_ptr by default
    * Windows fixed csp writing
    * Windows build fixes
    * ociobakelut supports looks

**Version 1.0.7 (April 17 2012):**
    * IRIDAS .look support
    * ociolutimage utility added (handles image <-> 3dlut)
    * CMake build allows optional reliance on system libraries
    * CMake layout changes for python and nuke installs
    * Bumped internals to yaml 0.3.0, pystring 1.1.2
    * Optimized internal handling of Matrix / Exponent Ops
    * Added INTERP_BEST interpolation option
    * Python config.clearLooks() added
    * Python docs revamp
    * Nuke config-dependent knob values now baked into .nk scripts
    * Nuke OCIOLookTransform gets reload button
    * Nuke nodes get updated help text

**Version 1.0.6 (March 12 2012):**
    * JNI (Java) updates
    * ocioconvert arbitrary attr support

**Version 1.0.5 (Feb 22 2012):**
    * Internal optimization framework added
    * SetLoggingLevel(..) bugfix
    * Python API Documentation / website updates
    * Clang compilation fix

**Version 1.0.4 (Jan 25 2012):**
    * ocio2icc deprecated (functionality merged into ociobakelut)
    * rv integration (beta)
    * nuke: updated channel handling
    * Documentation / website updates

**Version 1.0.3 (Dec 21 2011):**
    * Tetrahedral 3dlut interpolation (CPU only)
    * ociocheck and Config.sanityCheck() have improved validation
    * Mari: updated examples
    * Makefile: misc updates to match distro library conventions

**Version 1.0.2 (Nov 30 2011):**
    * 3D lut processing (cpu) is resiliant to nans
    * Nuke OCIOFileTransform gains Reload buttons
    * Installation on multi-lib *nix systems improved
    * Installation handling of soversion for C++/python improved
    * ociobakelut improvements
    * Initial version of Java bindings (alpha)

**Version 1.0.1 (Oct 31 2011):**
    * Luts with incorrect extension are properly loaded
    * ocio2icc / ociobakelut get --lut option (no longer requires ocio config)
    * DisplayTransform looks do not apply to 'data' passes.

**Version 1.0.0 (Oct 3 2011):**
    * ABI Lockoff for 1.0 branch
    * General API Cleanup (removed deprecated / unnecessary functions)
    * New features can be added, but the ABI will only be extended in a binary compatible manner.
      Profiles written from 1.0 will always be readable in future versions.
    * Fixed Truelight Reading Bug
    * ocio2icc no longer requires ocio config (can provide raw lut(s)


-------------------------------------------------------------------------------


**Version 0.8.7 (Oct 3 2011):**
    * Fixed Truelight Reading Bug

**Version 0.8.6 (Sept 7 2011):**
    * Updated .ocio config reading / writing to be forwards compatibile with 1.0
      (Profiles written in 0.8.6+ will be 1.0 compatible. Compatibility from prior versions is likely, though not guaranteed.)
    * Better logging
    * Added ColorSpace.equalitygroup (makes ColorSpace equality explicit)
    * Substantial Nuke node updates
    * Added support for Iridas .itx read/write
    * Windows Build Support

**Version 0.8.5 (Aug 2 2011):**
    * Nuke OCIODisplay fixed (bug from 0.8.4)
    * Updated Houdini HDL Reader / Writer

**Version 0.8.4 (July 25 2011):**
    * Native Look Support
    * Core / Nuke OCIODisplay supports alpha viewing
    * Added Houdini (.lut) writing
    * Added Pandora (.mga,.m3d) reading
    * Additional internal bug fixes

**Version 0.8.3 (June 27 2011):**
    * OCIO::Config symlink resolution fixed (bugfix)
    * Nuke OCIODisplay knobs use string storage (bugfix)
    * Makefile cleanup
    * Documentation cleanup

**Version 0.8.2 (June 7 2011):**
    * Numerous Windows compatibility fixes
    * Python binding improvements
    * OCIO::Baker / ociobakelut improvements
    * Lut1D/3D do not crash on nans (bugfix)
    * Nuke UI doesnt crash in known corner case (bugfix)

**Version 0.8.1 (May 9 2011):**
    * New roles: TEXTURE_PAINT + MATTE_PAINT
    * Mari API Example (src/mari)
    * FileFormat registry updated to allow Windows + Debug support
    * boost_ptr build compatibility option

**Version 0.8.0 (Apr 19 2011):**
    * ABI Lockoff for stable 0.8 branch
      New features can be added, but the ABI will only be extended in a binary compatible manner
    * Otherwise identical to 0.7.9

-------------------------------------------------------------------------------

**Version 0.7.9 (Apr 18 2011):**
    * Added support for .vf luts
    * Misc. Nuke enhancements
    * Adds optional boost ptr support (backwards compatibility)
    * Makefile enhancements (Nuke / CMAKE_INSTALL_EXEC_PREFIX)
    * cdlTransform.setXML crash fix

**Version 0.7.8 (March 31 2011):**
    * Iridas lut (.cube) bugfix, DOMAIN_MIN / DOMAIN_MAX now obeyed
    * Exposed GPU functions in python (needed for Mari)
    * Nuke OCIODisplay cleanup: Fixed knob names and added envvar support
    * ociobaker cleanup
    * tinyxml ABI visibility cleaned up
    * Removed Boost dependency, tr1::shared_ptr now used instead

**Version 0.7.7 (March 1 2011):**
    * Lut baking API + standalone app
    * Truelight runtime support (optional)
    * Cinespace (3d) lut writing
    * CSP prelut support
    * Boost argparse dependency removed
    * SanityCheck is more exhaustive
    * FileTransform supports relative path dirs
    * Python enhancements (transform kwarg support)
    * Makefile enhancements (OIIO Path)
    * Processor API update (code compatible, binary incompatible)

**Version 0.7.6 (Feb 1 2011):**
    * Updated Config Display API (.ocio config format updated)
    * Added ocio2icc app (ICC Profile Generation)
    * Revamp of ASC CDL, added .cc and .ccc support
    * Documentation Improvements
    * Makefile enhancements (Boost_INCLUDE_DIR, etc)

**Version 0.7.5 (Jan 13 2011):**
    * ociodisplay enhancements
    * gpu display bugfix (glsl profile 1.0 only)
    * Makefile enhancements
    * Nuke installation cleanup
    * Doc generation using sphinx (html + pdf)

**Version 0.7.4 (Jan 4 2011):**
    * Added 'Context', allowing for 'per-shot' Transforms
    * Misc API Cleanup: removed old functions + fixed const-ness
    * Added config.sanityCheck() for validation
    * Additional Makefile configuration options, SONAME, etc.

**Version 0.7.3 (Dec 16 2010):**
    * Added example applications: ociodisplay, ocioconvert
    * Makefile: Add boost header dependency
    * Makefile: Nuke plugins are now version specific
    * Fixed bug in GLSL MatrixOp

**Version 0.7.2 (Dec 9 2010):**
    * GPUAllocation refactor (API tweak)
    * Added AllocationTransform
    * Added LogTransform
    * Removed CineonLogToLinTransform
    * A few bug fixes

**Version 0.7.1 (Nov 15 2010):**
    * Additional 3d lut formats: Truelight .cub + Iridas .cube
    * FileTransform supports envvars and search paths
    * Added Nuke plugins: LogConvert + FileTransform
    * Improved OCIO profile formatting
    * GCC visibility used (when available) to hide private symbols
    * A few bug fixes

**Version 0.7.0 (Oct 21 2010):**
    * Switched file format from XML to Yaml

-------------------------------------------------------------------------------

**Version 0.6.1 (Oct 5 2010):**
    * Exposed ExponentTransform
    * Added CineonLogToLinTransform - a simple 'straight-line' negative
      linearization. Not strictly needed (could be done previously
      with LUTs) but often convenient to have.
    * Added DisplayTransform.displayCC for post display lut CC.
    * Many python improvements
    * A few bug fixes
    * A few Makefile enhancements

**Version 0.6.0 (Sept 21 2010):**
    * Start of 0.6, "stable" branch
      
      All 0.6.x builds will be ABI compatible (forward only).
      New features (even experimental ones) will be added to the 0.6 branch,
      as long as binary and source compatibility is maintained.
      Once this no longer is possible, a 0.7 "dev" branch will be forked.
    
    * Split public header into 3 parts for improved readability
      (you still only import <OpenColorIO/OpenColorIO.h> though)
    * Added MatrixTransform
    * Refactored internal unit testing
    * Fixed many compile warnings

-------------------------------------------------------------------------------

**Version 0.5.16 (Sept 16 2010):**
    * PyTransforms now use native python class inheritance
    * OCIO namespace can now be configured at build time (for distribution in commercial apps)
    * Updated make install behavior
    * DisplayTransform accepts generic cc operators (instead of CDL only)
    * A few bug fixes / compile warning fixes

**Version 0.5.15 (Sept 8 2010):**
    * OCIO is well behaved when $OCIO is unset, allowing un-color-managed use.
    * Color Transforms can be applied in python config->getProcessor
    * Simplification of API (getColorSpace allows cs name, role names, and cs objects)
    * Makefile enhancements (courtesy Malcolm Humphreys)
    * A bunch of bug fixes

**Version 0.5.14 (Sept 1 2010):**
    * Python binding enhancements
    * Simplified class implmentations (reduced internal header count)

**Version 0.5.13 (Aug 24 2010):**
    * GPU Processing now supports High Dynamic Range color spaces
    * Added log processing operator
    * Numerous bug fixes
    * Numerous enhancements to python glue
    * Exposed PyOpenColorIO header, for use in apps that require custom python glue
    * Matrix op is optimized for diagonal-only subcases
    * Numerous changes to Nuke Plugin (now with an addition node, OCIODisplay)

**Version 0.5.12 (Aug 18 2010):**
    * Additional DisplayTransform improvements
    * Additional GPU Improvements
    * Added op hashing (processor->getGPULut3DCacheID)

**Version 0.5.11 (Aug 11 2010):**
    * Initial DisplayTransform implementation
    * ASC CDL Support
    * Config Luma coefficients

**Version 0.5.10 (July 22 2010):**
    * Updated Nuke Plugin, now works in OSX
    * Fixed misc. build warnings.
    * Continued GPU progress (still under development)

**Version 0.5.9 (June 28 2010):**
    * Renamed project, classes, namespaces to OpenColorIO (OCIO)
    * Added single-pixel processor path
    * Improved python path makefile detection
    * Continued GPU progress (still under development)

**Version 0.5.8 (June 22 2010):**
    * Support for .3dl
    * Support for matrix ops
    * Code refactor (added Processors) to support gpu/cpu model
    * Much better error checking
    * Compilation support for python 2.5
    * Compilation support for OSX

**Version 0.5.7 (June 14 2010):**
    * Python API is much more fleshed out
    * Improved public C++ header

**Version 0.5.6 (June 8 2010):**
    * PyConfig stub implementation
    * Dropped ImageDesc.init; added PlanarImageDesc / PackedImageDesc
    * Dropped tr1::shared_ptr; added boost::shared_ptr

**Version 0.5.5 (June 4 2010):**
    * .ocio supports path references
    * Switch config envvar to $OCIO
    * Added 3D lut processing ops

**Version 0.5.4 (June 1 2010):**
    * Initial Release
    * CMake linux support 
    * XML OCIO format parsing / saving 
    * Example colorspace configuration with a few 'trivial' colorspaces 
    * Mutable colorspace configuration API 
    * Support for 1D lut processing 
    * Support for SPI 1D fileformats. 
    * Nuke plugin 
