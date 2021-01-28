// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

namespace 
{

void * getBufferData(py::buffer & data, py::dtype dtype, long size)
{
    py::buffer_info info = data.request();
    checkBufferType(info, dtype);
    checkBufferSize(info, size);
    return info.ptr;
}

} // namespace

void bindPyPlanarImageDesc(py::module & m)
{
    auto clsPlanarImageDesc = 
        py::class_<PyPlanarImageDesc, PyImageDesc /* base */>(
            m.attr("PlanarImageDesc"))

        .def(py::init([](py::buffer & rData, 
                         py::buffer & gData, 
                         py::buffer & bData,
                         long width, long height) 
            { 
                PyPlanarImageDesc * p = new PyPlanarImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = rData;
                p->m_data[1] = gData;
                p->m_data[2] = bData;
                long size = width * height;
                py::gil_scoped_acquire acquire;

                py::dtype type("float32");

                p->m_img = std::make_shared<PlanarImageDesc>(
                    getBufferData(p->m_data[0], type, size), 
                    getBufferData(p->m_data[1], type, size), 
                    getBufferData(p->m_data[2], type, size), 
                    nullptr, 
                    width, height);

                return p;
            }), 
             "rData"_a, "gData"_a, "bData"_a, "width"_a, "height"_a,
             DOC(PlanarImageDesc, PlanarImageDesc))
        .def(py::init([](py::buffer & rData, 
                         py::buffer & gData, 
                         py::buffer & bData, 
                         py::buffer & aData, 
                         long width, long height) 
            { 
                PyPlanarImageDesc * p = new PyPlanarImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = rData;
                p->m_data[1] = gData;
                p->m_data[2] = bData;
                p->m_data[3] = aData;
                long size = width * height;
                py::gil_scoped_acquire acquire;

                py::dtype type("float32");

                p->m_img = std::make_shared<PlanarImageDesc>(
                    getBufferData(p->m_data[0], type, size), 
                    getBufferData(p->m_data[1], type, size), 
                    getBufferData(p->m_data[2], type, size), 
                    getBufferData(p->m_data[3], type, size), 
                    width, height);
                return p;
            }), 
             "rData"_a, "gData"_a, "bData"_a, "aData"_a, "width"_a, "height"_a,
             DOC(PlanarImageDesc, PlanarImageDesc, 2))
        .def(py::init([](py::buffer & rData, 
                         py::buffer & gData, 
                         py::buffer & bData,
                         long width, long height,
                         BitDepth bitDepth,
                         ptrdiff_t xStrideBytes,
                         ptrdiff_t yStrideBytes) 
            { 
                PyPlanarImageDesc * p = new PyPlanarImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = rData;
                p->m_data[1] = gData;
                p->m_data[2] = bData;
                long size = width * height;
                py::gil_scoped_acquire acquire;

                py::dtype type = bitDepthToDtype(bitDepth);

                p->m_img = std::make_shared<PlanarImageDesc>(
                    getBufferData(p->m_data[0], type, size), 
                    getBufferData(p->m_data[1], type, size), 
                    getBufferData(p->m_data[2], type, size), 
                    nullptr, 
                    width, height,
                    bitDepth,
                    xStrideBytes,
                    yStrideBytes);

                return p;
            }),
             "rData"_a, "gData"_a, "bData"_a, "width"_a, "height"_a, "bitDepth"_a,
             "xStrideBytes"_a, "yStrideBytes"_a,
             DOC(PlanarImageDesc, PlanarImageDesc, 3))
        .def(py::init([](py::buffer & rData, 
                         py::buffer & gData, 
                         py::buffer & bData, 
                         py::buffer & aData, 
                         long width, long height,
                         BitDepth bitDepth,
                         ptrdiff_t xStrideBytes,
                         ptrdiff_t yStrideBytes) 
            { 
                PyPlanarImageDesc * p = new PyPlanarImageDesc();

                py::gil_scoped_release release;
                p->m_data[0] = rData;
                p->m_data[1] = gData;
                p->m_data[2] = bData;
                p->m_data[3] = aData;
                long size = width * height;
                py::gil_scoped_acquire acquire;

                py::dtype type = bitDepthToDtype(bitDepth);

                p->m_img = std::make_shared<PlanarImageDesc>(
                    getBufferData(p->m_data[0], type, size), 
                    getBufferData(p->m_data[1], type, size), 
                    getBufferData(p->m_data[2], type, size), 
                    getBufferData(p->m_data[3], type, size), 
                    width, height,
                    bitDepth,
                    xStrideBytes,
                    yStrideBytes);
                    
                return p;
            }),
             "rData"_a, "gData"_a, "bData"_a, "aData"_a, "width"_a, "height"_a, "bitDepth"_a,
             "xStrideBytes"_a, "yStrideBytes"_a,
             DOC(PlanarImageDesc, PlanarImageDesc, 4))
        
        .def("getRData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getRData());
            },
             DOC(PlanarImageDesc, getRData))
        .def("getGData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getGData());
            },
             DOC(PlanarImageDesc, getGData))
        .def("getBData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getBData());
            },
             DOC(PlanarImageDesc, getBData))
        .def("getAData", [](const PyPlanarImageDesc & self) 
            {
                PlanarImageDescRcPtr p = self.getImg();
                return py::array(bitDepthToDtype(p->getBitDepth()), 
                                 { p->getHeight() * p->getWidth() },
                                 { bitDepthToBytes(p->getBitDepth()) },
                                 p->getAData());
            },
             DOC(PlanarImageDesc, getAData));
}

} // namespace OCIO_NAMESPACE
