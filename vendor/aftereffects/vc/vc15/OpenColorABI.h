// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OPENCOLORABI_H
#define INCLUDED_OCIO_OPENCOLORABI_H

// Makefile configuration options
#define OCIO_NAMESPACE OpenColorIO

#define OCIO_VERSION_STR        "2.0.1"
#define OCIO_VERSION_STATUS_STR ""
#define OCIO_VERSION_FULL_STR   "2.0.1"

/* Version as a single 4-byte hex number, e.g. 0x01050200 == 1.5.2
   Use this for numeric comparisons, e.g. #if OCIO_VERSION_HEX >= ... 
   Note: in the case where SOVERSION is overridden at compile-time,
   this will reflect the original API version number.
   */
#define OCIO_VERSION_HEX ((2 << 24) | \
                          (0 << 16) | \
                          (1 <<  8))

#define OCIO_VERSION_MAJOR 2
#define OCIO_VERSION_MINOR 0

// shared_ptr / dynamic_pointer_cast
#include <memory>
#define OCIO_SHARED_PTR std::shared_ptr
#define OCIO_DYNAMIC_POINTER_CAST std::dynamic_pointer_cast

// If supported, define OCIOEXPORT, OCIOHIDDEN
// (used to choose which symbols to export from OpenColorIO)
#if defined(_WIN32)
    // Windows requires you to export from the main library and then import in any others
    // only when compiling a dynamic library (i.e. DLL)
    #ifndef OpenColorIO_SKIP_IMPORTS
        #if defined OpenColorIO_EXPORTS
            #define OCIOEXPORT __declspec(dllexport)
        #else
            #define OCIOEXPORT __declspec(dllimport)
        #endif
    #else
        #define OCIOEXPORT
    #endif
    #define OCIOHIDDEN

    #undef min
    #undef max
#elif defined __GNUC__
    #define OCIOEXPORT __attribute__ ((visibility("default")))
    #define OCIOHIDDEN __attribute__ ((visibility("hidden")))
#else // Others platforms not supported atm
    #define OCIOEXPORT
    #define OCIOHIDDEN
#endif

#endif // INCLUDED_OCIO_OPENCOLORABI_H
