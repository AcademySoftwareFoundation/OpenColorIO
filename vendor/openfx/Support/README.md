OFX Support Library
===================

This directory tree contains the source to a C++ library that skins the OFX
C plugin API with a set of classes. It is meant to act as a guide to people 
implementing plugins and hosts using the API and reveal the logical structure 
of the OFX API.

Copyright and License
---------------------

The library is copyright 2004 The Open Effects Association Ltd, and was
written by Bruno Nicoletti (bruno@thefoundry.co.uk).

It was originally released under the GNU Lesser General Public License.

It has subsequently been released under a 'BSD' style license. See the
file 'LICENSE' for details.

Structure
---------

- include: Contains the headers for any client code, including those originally 
           located in 'openfx/Support/Plugins/include'.
- Library: Contains the code that implements the support library.

OpenColorIO Modifications
-------------------------

The library source has been modified by the OpenColorIO project to support 
cross-platform builds with C++ standards 11, 14, and 17 without compiler 
warnings. This is a requirement to pass OCIO CI.

Note: The following warnings must still be disabled when compiling with 
MSVC: 4996, 4101
