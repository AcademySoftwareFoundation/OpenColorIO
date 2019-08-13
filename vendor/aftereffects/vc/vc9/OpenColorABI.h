// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OPENCOLORABI_H
#define INCLUDED_OCIO_OPENCOLORABI_H

// Makefile configuration options
#define OCIO_NAMESPACE OpenColorIO
#define OCIO_USE_BOOST_PTR 1
#define OCIO_VERSION "1.1.0"
#define OCIO_VERSION_NS v1

/* Version as a single 4-byte hex number, e.g. 0x01050200 == 1.5.2
   Use this for numeric comparisons, e.g. #if OCIO_VERSION_HEX >= ... 
   Note: in the case where SOVERSION is overridden at compile-time,
   this will reflect the original API version number.
   */
#define OCIO_VERSION_HEX ((1 << 24) | \
                          (1 << 16) | \
                          (0 <<  8))


// Namespace / version mojo
#define OCIO_NAMESPACE_ENTER namespace OCIO_NAMESPACE { namespace OCIO_VERSION_NS
#define OCIO_NAMESPACE_EXIT using namespace OCIO_VERSION_NS; }
#define OCIO_NAMESPACE_USING using namespace OCIO_NAMESPACE;

// shared_ptr / dynamic_pointer_cast
#if OCIO_USE_BOOST_PTR
#include <boost/shared_ptr.hpp>
#define OCIO_SHARED_PTR boost::shared_ptr
#define OCIO_DYNAMIC_POINTER_CAST boost::dynamic_pointer_cast
#elif __GNUC__ >= 4
#include <tr1/memory>
#define OCIO_SHARED_PTR std::tr1::shared_ptr
#define OCIO_DYNAMIC_POINTER_CAST std::tr1::dynamic_pointer_cast
#else
#error OCIO needs gcc 4 or later to get access to <tr1/memory> (or specify USE_BOOST_PTR instead)
#endif

#ifdef OpenColorIO_SHARED
	// If supported, define OCIOEXPORT, OCIOHIDDEN
	// (used to choose which symbols to export from OpenColorIO)
	#if defined __linux__ || __APPLE__
		#if __GNUC__ >= 4
			#define OCIOEXPORT __attribute__ ((visibility("default")))
			#define OCIOHIDDEN __attribute__ ((visibility("hidden")))
		#else
			#define OCIOEXPORT
			#define OCIOHIDDEN
		#endif
	#elif defined(_WIN32)
		// Windows requires you to export from the main library and then import in any others
		#if defined OpenColorIO_EXPORTS
			#define OCIOEXPORT __declspec(dllexport)
		#else
			#define OCIOEXPORT __declspec(dllimport)
		#endif
		#define OCIOHIDDEN
	#else // Others platforms not supported atm
		#define OCIOEXPORT
		#define OCIOHIDDEN
	#endif
#else
	#define OCIOEXPORT
	#define OCIOHIDDEN
#endif

// Windows defines these troublesome macros that collide with std::limits
#if defined(_WIN32)
#undef min
#undef max
#endif

#endif // INCLUDED_OCIO_OPENCOLORABI_H
