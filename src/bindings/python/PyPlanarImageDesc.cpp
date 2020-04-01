// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyPlanarImageDesc(py::module & m)
{
    py::class_<PyPlanarImageDesc, PyImageDesc /* base */>(m, "PlanarImageDesc")
        .def(py::init([](py::buffer & rData, 
                         py::buffer & gData, 
                         py::buffer & bData, 
                         py::buffer & aData, 
                         long width, long height) 
            { 
                PyPlanarImageDesc d;

                py::gil_scoped_release release;
                d.m_data[0] = rData;
                d.m_data[1] = gData;
                d.m_data[2] = bData;
                d.m_data[3] = aData;
                py::gil_scoped_acquire acquire;

                py::buffer_info rInfo = d.m_data[0].request();
                py::buffer_info gInfo = d.m_data[1].request();
                py::buffer_info bInfo = d.m_data[2].request();
                py::buffer_info aInfo = d.m_data[3].request();

                checkBufferType(rInfo, py::dtype("float32"));
                checkBufferSize(rInfo, width*height);
                checkBufferType(gInfo, py::dtype("float32"));
                checkBufferSize(gInfo, width*height);
                checkBufferType(bInfo, py::dtype("float32"));
                checkBufferSize(bInfo, width*height);
                checkBufferType(aInfo, py::dtype("float32"));
                checkBufferSize(aInfo, width*height);

                d.m_img = std::make_shared<PlanarImageDesc>(rInfo.ptr, 
                                                            gInfo.ptr, 
                                                            bInfo.ptr, 
                                                            aInfo.ptr, 
                                                            width, height);
                return d;
            }), 
             "rData"_a, "gData"_a, "bData"_a, "aData"_a, "width"_a, "height"_a)
        .def(py::init([](py::buffer & rData, 
                         py::buffer & gData, 
                         py::buffer & bData, 
                         py::buffer & aData, 
                         long width, long height,
                         BitDepth bitDepth,
                         ptrdiff_t xStrideBytes,
                         ptrdiff_t yStrideBytes) 
            { 
                PyPlanarImageDesc d;

                py::gil_scoped_release release;
                d.m_data[0] = rData;
                d.m_data[1] = gData;
                d.m_data[2] = bData;
                d.m_data[3] = aData;
                py::gil_scoped_acquire acquire;

                py::buffer_info rInfo = d.m_data[0].request();
                py::buffer_info gInfo = d.m_data[1].request();
                py::buffer_info bInfo = d.m_data[2].request();
                py::buffer_info aInfo = d.m_data[3].request();

                checkBufferType(rInfo, bitDepth);
                checkBufferSize(rInfo, width*height);
                checkBufferType(gInfo, bitDepth);
                checkBufferSize(gInfo, width*height);
                checkBufferType(bInfo, bitDepth);
                checkBufferSize(bInfo, width*height);
                checkBufferType(aInfo, bitDepth);
                checkBufferSize(aInfo, width*height);

                d.m_img = std::make_shared<PlanarImageDesc>(rInfo.ptr, 
                                                            gInfo.ptr, 
                                                            bInfo.ptr, 
                                                            aInfo.ptr, 
                                                            width, height,
                                                            bitDepth,
                                                            xStrideBytes,
                                                            yStrideBytes);
                return d;
            }),
             "rData"_a, "gData"_a, "bData"_a, "aData"_a, "width"_a, "height"_a, "bitDepth"_a,
             "xStrideBytes"_a, "yStrideBytes"_a)
        
        .def("getRData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getRData());
            })
        .def("getGData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getGData());
            })
        .def("getBData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getBData());
            })
        .def("getAData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getAData());
            });
}

} // namespace OCIO_NAMESPACE
