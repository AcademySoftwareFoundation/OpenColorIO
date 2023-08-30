// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <vector>

#include "PyDynamicProperty.h"
#include "PyOpenColorIO.h"
#include "PyUtils.h"
#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyCPUProcessor(py::module & m)
{
    auto clsCPUProcessor = 
        py::class_<CPUProcessor, CPUProcessorRcPtr>(
            m.attr("CPUProcessor"))

        .def("isNoOp", &CPUProcessor::isNoOp, 
             DOC(CPUProcessor, isNoOp))
        .def("isIdentity", &CPUProcessor::isIdentity, 
             DOC(CPUProcessor, isIdentity))
        .def("hasChannelCrosstalk", &CPUProcessor::hasChannelCrosstalk, 
             DOC(CPUProcessor, hasChannelCrosstalk))
        .def("getCacheID", &CPUProcessor::getCacheID, 
             DOC(CPUProcessor, getCacheID))
        .def("getInputBitDepth", &CPUProcessor::getInputBitDepth, 
             DOC(CPUProcessor, getInputBitDepth))
        .def("getOutputBitDepth", &CPUProcessor::getOutputBitDepth, 
             DOC(CPUProcessor, getOutputBitDepth))
        .def("getDynamicProperty", [](CPUProcessorRcPtr & self, DynamicPropertyType type) 
            {
                return PyDynamicProperty(self->getDynamicProperty(type));
            }, 
            "type"_a, 
             DOC(CPUProcessor, getDynamicProperty))
        .def("hasDynamicProperty",
             (bool (CPUProcessor::*)(DynamicPropertyType) const noexcept)
             &CPUProcessor::hasDynamicProperty,
             "type"_a,
             DOC(CPUProcessor, hasDynamicProperty))
        .def("isDynamic", &CPUProcessor::isDynamic,
             DOC(CPUProcessor, isDynamic))

        .def("apply", [](CPUProcessorRcPtr & self, PyImageDesc & imgDesc) 
            {
                self->apply((*imgDesc.m_img));
            },
             "imgDesc"_a,
             py::call_guard<py::gil_scoped_release>(), 
             R"doc(
Apply to an image with any kind of channel ordering while respecting 
the input and output bit-depths. Image values are modified in place.

.. note::
    The GIL is released during processing, freeing up Python to execute 
    other threads concurrently.

.. note::
    For large images, ``applyRGB`` or ``applyRGBA`` are preferred for 
    processing a NumPy array. The Python ``ImageDesc`` implementation 
    requires copying all values (once) in order to own the underlying 
    pointer. The dedicated packed ``apply*`` methods utilize 
    ``ImageDesc`` on the C++ side so avoid the copy.

)doc")
        .def("apply", [](CPUProcessorRcPtr & self, 
                         PyImageDesc & srcImgDesc, 
                         PyImageDesc & dstImgDesc)
            {
                self->apply((*srcImgDesc.m_img), (*dstImgDesc.m_img));
            },
             "srcImgDesc"_a, "dstImgDesc"_a,
             py::call_guard<py::gil_scoped_release>(),
             R"doc(
Apply to an image with any kind of channel ordering while respecting 
the input and output bit-depths. Modified srcImgDesc image values are
written to the dstImgDesc image, leaving srcImgDesc unchanged.

.. note::
    The GIL is released during processing, freeing up Python to execute 
    other threads concurrently.

.. note::
    For large images, ``applyRGB`` or ``applyRGBA`` are preferred for 
    processing a NumPy array. The Python ``ImageDesc`` implementation 
    requires copying all values (once) in order to own the underlying 
    pointer. The dedicated packed ``apply*`` methods utilize 
    ``ImageDesc`` on the C++ side so avoid the copy.

)doc")
        .def("applyRGB", [](CPUProcessorRcPtr & self, py::buffer & data) 
            {
                py::buffer_info info = data.request();
                checkBufferDivisible(info, 3);

                // Interpret as single row of RGB pixels
                BitDepth bitDepth = getBufferBitDepth(info);

                py::gil_scoped_release release;

                long numChannels = 3;
                long width = (long)info.size / numChannels;
                long height = 1;
                ptrdiff_t chanStrideBytes = (ptrdiff_t)info.itemsize;
                ptrdiff_t xStrideBytes = chanStrideBytes * numChannels;
                ptrdiff_t yStrideBytes = xStrideBytes * width;

                PackedImageDesc img(info.ptr, 
                                    width, height, 
                                    numChannels, 
                                    bitDepth, 
                                    chanStrideBytes, 
                                    xStrideBytes, 
                                    yStrideBytes);
                self->apply(img);
            },
             "data"_a, 
             R"doc(
Apply to a packed RGB array adhering to the Python buffer protocol. 
This will typically be a NumPy array. Input and output bit-depths are
respected but must match. Any array size or shape is supported as long
as the flattened array size is divisible by 3. Array values are 
modified in place.

.. note::
    This differs from the C++ implementation which only applies to a 
    single pixel. This method uses a ``PackedImageDesc`` under the 
    hood to apply to an entire image at once. The GIL is released 
    during processing, freeing up Python to execute other threads 
    concurrently.

)doc")
        .def("applyRGB", [](CPUProcessorRcPtr & self, std::vector<float> & data) 
            {
                checkVectorDivisible(data, 3);

                long numChannels = 3;
                long width = (long)data.size() / numChannels;
                long height = 1;

                PackedImageDesc img(&data[0], width, height, numChannels);
                self->apply(img);

                return data;
            },
             "data"_a,
             py::call_guard<py::gil_scoped_release>(), 
             R"doc(
Apply to a packed RGB list of float values. Any size is supported as 
long as the list length is divisible by 3. A new list with processed
float values is returned, leaving the input list unchanged.

.. note::
    This differs from the C++ implementation which only applies to a 
    single pixel. This method uses a ``PackedImageDesc`` under the 
    hood to apply to an entire image at once. The GIL is released 
    during processing, freeing up Python to execute other threads 
    concurrently.

.. note::
    For large images, a NumPy array should be preferred over a list.
    List values are copied on input and output, where an array is 
    modified in place.

)doc")
        .def("applyRGBA", [](CPUProcessorRcPtr & self, py::buffer & data) 
            {
                py::buffer_info info = data.request();
                checkBufferDivisible(info, 4);

                // Interpret as single row of RGBA pixels
                BitDepth bitDepth = getBufferBitDepth(info);

                py::gil_scoped_release release;

                long numChannels = 4;
                long width = (long)info.size / numChannels;
                long height = 1;
                ptrdiff_t chanStrideBytes = (ptrdiff_t)info.itemsize;
                ptrdiff_t xStrideBytes = chanStrideBytes * numChannels;
                ptrdiff_t yStrideBytes = xStrideBytes * width;

                PackedImageDesc img(info.ptr, 
                                    width, height, 
                                    numChannels, 
                                    bitDepth, 
                                    chanStrideBytes, 
                                    xStrideBytes, 
                                    yStrideBytes);
                self->apply(img);
            },
             "data"_a, 
             R"doc(
Apply to a packed RGBA array adhering to the Python buffer protocol. 
This will typically be a NumPy array. Input and output bit-depths are
respected but must match. Any array size or shape is supported as long
as the flattened array size is divisible by 4. Array values are 
modified in place.

.. note::
    This differs from the C++ implementation which only applies to a 
    single pixel. This method uses a ``PackedImageDesc`` under the 
    hood to apply to an entire image at once. The GIL is released 
    during processing, freeing up Python to execute other threads 
    concurrently.

)doc")
        .def("applyRGBA", [](CPUProcessorRcPtr & self, std::vector<float> & data) 
            {
                checkVectorDivisible(data, 4);

                long numChannels = 4;
                long width = (long)data.size() / numChannels;
                long height = 1;

                PackedImageDesc img(&data[0], width, height, numChannels);
                self->apply(img);

                return data;
            },
             "data"_a,
             py::call_guard<py::gil_scoped_release>(), 
             R"doc(
Apply to a packed RGBA list of float values. Any size is supported as 
long as the list length is divisible by 4. A new list with processed
float values is returned, leaving the input list unchanged.

.. note::
    This differs from the C++ implementation which only applies to a 
    single pixel. This method uses a ``PackedImageDesc`` under the 
    hood to apply to an entire image at once. The GIL is released 
    during processing, freeing up Python to execute other threads 
    concurrently.

.. note::
    For large images, a NumPy array should be preferred over a list.
    List values are copied on input and output, where an array is 
    modified in place.

)doc");
}

} // namespace OCIO_NAMESPACE
