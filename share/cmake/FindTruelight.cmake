#==========
#
# Copyright (c) 2011, Malcolm Humphreys.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the name of Dan Bethell nor the names of any
#       other contributors to this software may be used to endorse or
#       promote products derived from this software without specific prior
#       written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========
#
# Variables defined by this module:
#   Truelight_FOUND    
#   Truelight_INCLUDE_DIR
#   Truelight_COMPILE_FLAGS
#   Truelight_LINK_FLAGS
#   Truelight_LIBRARIES
#   Truelight_LIBRARY_DIR
#
# Usage: 
#   FIND_PACKAGE( Truelight )
#   FIND_PACKAGE( Truelight REQUIRED )
#
# Note:
# You can tell the module where Truelight is installed by setting
# the TRUELIGHT_INSTALL_PATH (or setting the TRUELIGHT_ROOT environment
# variable before calling FIND_PACKAGE.
# 
# E.g. 
#   SET( TRUELIGHT_INSTALL_PATH "/usr/fl/truelight" )
#   FIND_PACKAGE( Truelight REQUIRED )
#
#==========

# our includes
FIND_PATH( Truelight_INCLUDE_DIR truelight.h
  $ENV{TRUELIGHT_ROOT}/include
  ${TRUELIGHT_INSTALL_PATH}/include
  )

# our library
FIND_LIBRARY( Truelight_LIBRARIES libtruelight.a
  $ENV{TRUELIGHT_ROOT}/lib
  ${TRUELIGHT_INSTALL_PATH}/lib
  )

SET( Truelight_COMPILE_FLAGS "" )
SET( Truelight_LINK_FLAGS "" )
IF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    SET( Truelight_COMPILE_FLAGS "" )
    SET( Truelight_LINK_FLAGS "-framework CoreServices -framework IOKit" )
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )

# our library path
GET_FILENAME_COMPONENT( Truelight_LIBRARY_DIR ${Truelight_LIBRARIES} PATH )

# did we find everything?
INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Truelight" DEFAULT_MSG
  Truelight_INCLUDE_DIR
  Truelight_LIBRARIES
  Truelight_LIBRARY_DIR
  )
