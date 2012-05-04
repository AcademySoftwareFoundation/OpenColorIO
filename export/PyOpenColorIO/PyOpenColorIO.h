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


#ifndef INCLUDED_PYOCIO_PYOCIO_H
#define INCLUDED_PYOCIO_PYOCIO_H

#include <Python.h>

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{
    // ColorSpace
    PyObject * BuildConstPyColorSpace(ConstColorSpaceRcPtr colorSpace);
    PyObject * BuildEditablePyColorSpace(ColorSpaceRcPtr colorSpace);
    bool IsPyColorSpace(PyObject * pyobject);
    bool IsPyColorSpaceEditable(PyObject * pyobject);
    ConstColorSpaceRcPtr GetConstColorSpace(PyObject * pyobject, bool allowCast);
    ColorSpaceRcPtr GetEditableColorSpace(PyObject * pyobject);
    
    // Config
    PyObject * BuildConstPyConfig(ConstConfigRcPtr config);
    PyObject * BuildEditablePyConfig(ConfigRcPtr config);
    bool IsPyConfig(PyObject * config);
    bool IsPyConfigEditable(PyObject * config);
    ConstConfigRcPtr GetConstConfig(PyObject * config, bool allowCast);
    ConfigRcPtr GetEditableConfig(PyObject * config);
    
    // Context
    PyObject * BuildConstPyContext(ConstContextRcPtr context);
    PyObject * BuildEditablePyContext(ContextRcPtr context);
    bool IsPyContext(PyObject * config);
    bool IsPyContextEditable(PyObject * config);
    ConstContextRcPtr GetConstContext(PyObject * context, bool allowCast);
    ContextRcPtr GetEditableContext(PyObject * context);
    
    // Exception
    // Warning: these cannot return valid PyObject pointers before
    // the python module has been initialized. Beware of calling these
    // at static construction time.
    PyObject * GetExceptionPyType();
    PyObject * GetExceptionMissingFilePyType();
    
    // Processor
    PyObject * BuildConstPyProcessor(ConstProcessorRcPtr processor);
    bool IsPyProcessor(PyObject * pyobject);
    ConstProcessorRcPtr GetConstProcessor(PyObject * pyobject);
    
    // ProcessorMetadata
    PyObject * BuildConstPyProcessorMetadata(ConstProcessorMetadataRcPtr metadata);
    bool IsPyProcessorMetadata(PyObject * pyobject);
    ConstProcessorMetadataRcPtr GetConstProcessorMetadata(PyObject * pyobject);
    
    // Transform
    PyObject * BuildConstPyTransform(ConstTransformRcPtr transform);
    PyObject * BuildEditablePyTransform(TransformRcPtr transform);
    bool IsPyTransform(PyObject * pyobject);
    bool IsPyTransformEditable(PyObject * pyobject);
    ConstTransformRcPtr GetConstTransform(PyObject * pyobject, bool allowCast);
    TransformRcPtr GetEditableTransform(PyObject * pyobject);
    
    // Look
    PyObject * BuildConstPyLook(ConstLookRcPtr look);
    PyObject * BuildEditablePyLook(LookRcPtr look);
    bool IsPyLook(PyObject * pyobject);
    bool IsPyLookEditable(PyObject * pyobject);
    ConstLookRcPtr GetConstLook(PyObject * pyobject, bool allowCast);
    LookRcPtr GetEditableLook(PyObject * pyobject);
}
OCIO_NAMESPACE_EXIT

#endif
