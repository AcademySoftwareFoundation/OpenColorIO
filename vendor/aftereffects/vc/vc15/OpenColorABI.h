// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OPENCOLORABI_H
#define INCLUDED_OCIO_OPENCOLORABI_H

// Makefile configuration options
#define OCIO_NAMESPACE OpenColorIO
#define OCIO_VERSION "2.0.0"
#define OCIO_VERSION_NS v200

/* Version as a single 4-byte hex number, e.g. 0x01050200 == 1.5.2
   Use this for numeric comparisons, e.g. #if OCIO_VERSION_HEX >= ... 
   Note: in the case where SOVERSION is overridden at compile-time,
   this will reflect the original API version number.
   */
#define OCIO_VERSION_HEX ((2 << 24) | \
                          (0 << 16) | \
                          (0 <<  8))


// Namespace / version mojo
#define OCIO_NAMESPACE_ENTER namespace OCIO_NAMESPACE { namespace OCIO_VERSION_NS
#define OCIO_NAMESPACE_EXIT using namespace OCIO_VERSION_NS; }
#define OCIO_NAMESPACE_USING using namespace OCIO_NAMESPACE;

// shared_ptr / dynamic_pointer_cast
#include <memory>
#define OCIO_SHARED_PTR std::shared_ptr
#define OCIO_DYNAMIC_POINTER_CAST std::dynamic_pointer_cast

// If supported, define OCIOEXPORT, OCIOHIDDEN
// (used to choose which symbols to export from OpenColorIO)
#if defined __GNUC__
    #if __GNUC__ >= 4
        #define OCIOEXPORT __attribute__ ((visibility("default")))
        #define OCIOHIDDEN __attribute__ ((visibility("hidden")))
    #else
        #define OCIOEXPORT
        #define OCIOHIDDEN
    #endif
#elif defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER)
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
#else // Others platforms not supported atm
    #define OCIOEXPORT
    #define OCIOHIDDEN
#endif

#endif // INCLUDED_OCIO_OPENCOLORABI_H
