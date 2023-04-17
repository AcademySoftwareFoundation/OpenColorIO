#==========
#
# Copyright (c) 2010, Dan Bethell.
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
#   Nuke_FOUND    
#   Nuke_INCLUDE_DIR
#   Nuke_COMPILE_FLAGS
#   Nuke_LINK_FLAGS
#   Nuke_LIBRARIES
#   Nuke_LIBRARY_DIR
#
# Usage: 
#   FIND_PACKAGE( Nuke )
#   FIND_PACKAGE( Nuke REQUIRED )
#
# Note:
# You can tell the module where Nuke is installed by setting
# the NUKE_INSTALL_PATH (or setting the NDK_PATH environment
# variable before calling FIND_PACKAGE.
# 
# E.g. 
#   SET( NUKE_INSTALL_PATH "/usr/local/Nuke5.2v3" )
#   FIND_PACKAGE( Nuke REQUIRED )
#
#==========

# our includes
FIND_PATH( Nuke_INCLUDE_DIR DDImage/Op.h
  $ENV{NDK_PATH}/include
  ${NUKE_INSTALL_PATH}/include
  )

# our library
FIND_LIBRARY( Nuke_LIBRARIES DDImage
  $ENV{NDK_PATH}
  ${NUKE_INSTALL_PATH}
  )

SET( Nuke_COMPILE_FLAGS "" )
SET( Nuke_LINK_FLAGS "" )
IF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    SET( Nuke_COMPILE_FLAGS "-arch i386" )
    SET( Nuke_LINK_FLAGS "-arch i386" )
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )

# our library path
GET_FILENAME_COMPONENT( Nuke_LIBRARY_DIR ${Nuke_LIBRARIES} PATH )

# did we find everything?
INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Nuke" DEFAULT_MSG
  Nuke_INCLUDE_DIR
  Nuke_LIBRARIES
  Nuke_LIBRARY_DIR
  )

SET( Nuke_API_VERSION "" )
IF( NUKE_FOUND )
  TRY_RUN(NUKE_VERSION_EXITCODE NUKE_VERSION_COMPILED
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/share/cmake/modules/src/TestForDDImageVersion.cxx
    COMPILE_DEFINITIONS
      "-I${Nuke_INCLUDE_DIR}"
    RUN_OUTPUT_VARIABLE
      Nuke_API_VERSION
    )
  message(STATUS "Nuke_API_VERSION: --${Nuke_API_VERSION}--")
ENDIF()
