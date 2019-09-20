// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
