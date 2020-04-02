// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyPackedImageDesc(py::module & m)
{
    py::class_<PyPackedImageDesc, PyImageDesc /* base */>(m, "PackedImageDesc")
        .def(py::init([](py::buffer & data, long width, long height, long numChannels) 
            { 
                PyPackedImageDesc * p = new PyPackedImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = data;
                py::gil_scoped_acquire acquire;

                py::buffer_info info = p->m_data[0].request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferSize(info, width*height*numChannels);
                
                p->m_img = std::make_shared<PackedImageDesc>(info.ptr, width, height, numChannels);

                return p;
            }),
             "data"_a, "width"_a, "height"_a, "numChannels"_a)
        .def(py::init([](py::buffer & data,
                         long width, long height,
                         long numChannels,
                         BitDepth bitDepth,
                         ptrdiff_t chanStrideBytes,
                         ptrdiff_t xStrideBytes,
                         ptrdiff_t yStrideBytes) 
            { 
                PyPackedImageDesc * p = new PyPackedImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = data;
                py::gil_scoped_acquire acquire;

                py::buffer_info info = p->m_data[0].request();
                checkBufferType(info, bitDepth);
                checkBufferSize(info, width*height*numChannels);

                p->m_img = std::make_shared<PackedImageDesc>(info.ptr, 
                                                             width, height, 
                                                             numChannels, 
                                                             bitDepth, 
                                                             chanStrideBytes, 
                                                             xStrideBytes, 
                                                             yStrideBytes);
                return p;
            }),
             "data"_a, "width"_a, "height"_a, "numChannels"_a, "bitDepth"_a, "chanStrideBytes"_a, 
             "xStrideBytes"_a, "yStrideBytes"_a)
        .def(py::init([](py::buffer & data, 
                         long width, long height, 
                         ChannelOrdering chanOrder) 
            { 
                PyPackedImageDesc * p = new PyPackedImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = data;
                py::gil_scoped_acquire acquire;

                py::buffer_info info = p->m_data[0].request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferSize(info, width*height*chanOrderToNumChannels(chanOrder));

                p->m_img = std::make_shared<PackedImageDesc>(info.ptr, width, height, chanOrder);

                return p;
            }),
             "data"_a, "width"_a, "height"_a, "chanOrder"_a)
        .def(py::init([](py::buffer & data,
                         long width, long height,
                         ChannelOrdering chanOrder,
                         BitDepth bitDepth,
                         ptrdiff_t chanStrideBytes,
                         ptrdiff_t xStrideBytes,
                         ptrdiff_t yStrideBytes) 
            { 
                PyPackedImageDesc * p = new PyPackedImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = data;
                py::gil_scoped_acquire acquire;
                
                py::buffer_info info = p->m_data[0].request();
                checkBufferType(info, bitDepth);
                checkBufferSize(info, width*height*chanOrderToNumChannels(chanOrder));

                p->m_img = std::make_shared<PackedImageDesc>(info.ptr, 
                                                             width, height, 
                                                             chanOrder, 
                                                             bitDepth, 
                                                             chanStrideBytes, 
                                                             xStrideBytes, 
                                                             yStrideBytes);
                return p;
            }),
             "data"_a, "width"_a, "height"_a, "chanOrder"_a, "bitDepth"_a, "chanStrideBytes"_a, 
             "xStrideBytes"_a, "yStrideBytes"_a)
        
        .def("getData", [](const PyPackedImageDesc & self) 
            {
                PackedImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() * p->getNumChannels() },
                                 { p->getChanStrideBytes() },
                                 p->getData());
            })
        .def("getChannelOrder", [](const PyPackedImageDesc & self) 
            {
                return self.getImg()->getChannelOrder();
            })
        .def("getNumChannels", [](const PyPackedImageDesc & self) 
            {
                return self.getImg()->getNumChannels();
            })
        .def("getChanStrideBytes", [](const PyPackedImageDesc & self) 
            {
                return self.getImg()->getChanStrideBytes();
            });
}

} // namespace OCIO_NAMESPACE
