/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef INCLUDED_PYOCIO_PYTRANSFORM_H
#define INCLUDED_PYOCIO_PYTRANSFORM_H

#include <PyOpenColourIO/PyOpenColourIO.h>

OCIO_NAMESPACE_ENTER
{
    // TODO: Maybe put this in a pyinternal namespace?
    
    typedef struct {
        PyObject_HEAD
        ConstTransformRcPtr * constcppobj;
        TransformRcPtr * cppobj;
        bool isconst;
    } PyOCIO_Transform;
    
    extern PyTypeObject PyOCIO_TransformType;
    bool AddTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_AllocationTransformType;
    bool AddAllocationTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_CDLTransformType;
    bool AddCDLTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_ColourSpaceTransformType;
    bool AddColourSpaceTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_DisplayTransformType;
    bool AddDisplayTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_ExponentTransformType;
    bool AddExponentTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_FileTransformType;
    bool AddFileTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_GroupTransformType;
    bool AddGroupTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_LogTransformType;
    bool AddLogTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_LookTransformType;
    bool AddLookTransformObjectToModule( PyObject* m );
    
    extern PyTypeObject PyOCIO_MatrixTransformType;
    bool AddMatrixTransformObjectToModule( PyObject* m );
}
OCIO_NAMESPACE_EXIT

#endif
