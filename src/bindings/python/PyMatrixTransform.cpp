// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyMatrixTransform(py::module & m)
{
    MatrixTransformRcPtr DEFAULT = MatrixTransform::Create();

    std::array<double, 16> DEFAULT_M44;
    DEFAULT->getMatrix(DEFAULT_M44.data());

    std::array<double, 4> DEFAULT_OFFSET4;
    DEFAULT->getOffset(DEFAULT_OFFSET4.data());

    py::class_<MatrixTransform, 
               MatrixTransformRcPtr /* holder */, 
               Transform /* base */>(m, "MatrixTransform")
        .def(py::init(&MatrixTransform::Create))
        .def(py::init([](const std::array<double, 16> & m44,
                         const std::array<double, 4> & offset4, 
                         TransformDirection dir) 
            {
                MatrixTransformRcPtr p = MatrixTransform::Create();
                p->setMatrix(m44.data());
                p->setOffset(offset4.data());
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "m44"_a = DEFAULT_M44,
             "offset4"_a = DEFAULT_OFFSET4,
             "dir"_a = DEFAULT->getDirection())

        // TODO: Update static convenience functions to construct a MatrixTransform in C++
        .def_static("Fit", [](const std::array<double, 4> & oldmin4,
                              const std::array<double, 4> & oldmax4,
                              const std::array<double, 4> & newmin4,
                              const std::array<double, 4> & newmax4)
            {
                double m44[16];
                double offset4[4];
                MatrixTransform::Fit(m44, offset4,
                                     oldmin4.data(), oldmax4.data(),
                                     newmin4.data(), newmax4.data());
                MatrixTransformRcPtr p = MatrixTransform::Create();
                p->setMatrix(m44);
                p->setOffset(offset4);
                p->validate();
                return p;
            },
             "oldmin4"_a = std::array<double, 4>{ 0.0, 0.0, 0.0, 0.0 }, 
             "oldmax4"_a = std::array<double, 4>{ 1.0, 1.0, 1.0, 1.0 },
             "newmin4"_a = std::array<double, 4>{ 0.0, 0.0, 0.0, 0.0 }, 
             "newmax4"_a = std::array<double, 4>{ 1.0, 1.0, 1.0, 1.0 })
        .def_static("Identity", []()
            {
                double m44[16];
                double offset4[4];
                MatrixTransform::Identity(m44, offset4);
                MatrixTransformRcPtr p = MatrixTransform::Create();
                p->setMatrix(m44);
                p->setOffset(offset4);
                p->validate();
                return p;
            })
        .def_static("Sat", [](double sat, const std::array<double, 3> & lumaCoef3)
            {
                double m44[16];
                double offset4[4];
                MatrixTransform::Sat(m44, offset4, sat, lumaCoef3.data());
                MatrixTransformRcPtr p = MatrixTransform::Create();
                p->setMatrix(m44);
                p->setOffset(offset4);
                p->validate();
                return p;
            },
             "sat"_a, "lumaCoef3"_a)
        .def_static("Scale", [](const std::array<double, 4> & scale4)
            {
                double m44[16];
                double offset4[4];
                MatrixTransform::Scale(m44, offset4, scale4.data());
                MatrixTransformRcPtr p = MatrixTransform::Create();
                p->setMatrix(m44);
                p->setOffset(offset4);
                p->validate();
                return p;
            },
             "scale4"_a)
        .def_static("View", [](std::array<int, 4> & channelHot4,
                               const std::array<double, 3> & lumaCoef3)
            {
                double m44[16];
                double offset4[4];
                MatrixTransform::View(m44, offset4, channelHot4.data(), lumaCoef3.data());
                MatrixTransformRcPtr p = MatrixTransform::Create();
                p->setMatrix(m44);
                p->setOffset(offset4);
                p->validate();
                return p;
            },
             "channelHot4"_a, "scale4"_a)

        .def("getFormatMetadata", 
             (FormatMetadata & (MatrixTransform::*)()) &MatrixTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (MatrixTransform::*)() const) 
             &MatrixTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &MatrixTransform::equals, "other"_a)
        .def("getMatrix", [](MatrixTransformRcPtr self)
            {
                std::array<double, 16> m44;
                self->getMatrix(m44.data());
                return m44;
            })
        .def("setMatrix", [](MatrixTransformRcPtr self, const std::array<double, 16> & m44)
            { 
                self->setMatrix(m44.data());
            }, 
             "m44"_a)
        .def("getOffset", [](MatrixTransformRcPtr self)
            {
                std::array<double, 4> offset4;
                self->getOffset(offset4.data());
                return offset4;
            })
        .def("setOffset", [](MatrixTransformRcPtr self, const std::array<double, 4> & offset4)
            { 
                self->setOffset(offset4.data());
            }, 
             "offset4"_a)
        .def("getFileInputBitDepth", &MatrixTransform::getFileInputBitDepth)
        .def("setFileInputBitDepth", &MatrixTransform::setFileInputBitDepth, "bitDepth"_a)
        .def("getFileOutputBitDepth", &MatrixTransform::getFileOutputBitDepth)
        .def("setFileOutputBitDepth", &MatrixTransform::setFileOutputBitDepth, "bitDepth"_a);
}

} // namespace OCIO_NAMESPACE
