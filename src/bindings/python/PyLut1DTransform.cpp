// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLut1DTransform(py::module & m)
{
    Lut1DTransformRcPtr DEFAULT = Lut1DTransform::Create();

    py::class_<Lut1DTransform, 
               Lut1DTransformRcPtr /* holder */, 
               Transform /* base */>(m, "Lut1DTransform")
        .def(py::init([]() { return Lut1DTransform::Create(); }))
        .def(py::init([](unsigned long length, bool isHalfDomain) 
            { 
                return Lut1DTransform::Create(length, isHalfDomain); 
            }),
             "length"_a, "isHalfDomain"_a)
        .def(py::init([](unsigned long length, 
                         bool isHalfDomain,
                         bool isRawHalfs,
                         BitDepth fileOutputBitDepth,
                         Lut1DHueAdjust hueAdjust,
                         Interpolation interpolation,
                         TransformDirection dir)
            {
                Lut1DTransformRcPtr p = Lut1DTransform::Create(length, isHalfDomain);
                p->setOutputRawHalfs(isRawHalfs);
                p->setFileOutputBitDepth(fileOutputBitDepth);
                p->setHueAdjust(hueAdjust);
                p->setInterpolation(interpolation);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "length"_a = DEFAULT->getLength(),
             "isHalfDomain"_a = DEFAULT->getInputHalfDomain(),
             "isRawHalfs"_a = DEFAULT->getOutputRawHalfs(),
             "fileOutputBitDepth"_a = DEFAULT->getFileOutputBitDepth(),
             "hueAdjust"_a = DEFAULT->getHueAdjust(),
             "interpolation"_a = DEFAULT->getInterpolation(),
             "dir"_a = DEFAULT->getDirection())

        .def("getFileOutputBitDepth", &Lut1DTransform::getFileOutputBitDepth)
        .def("setFileOutputBitDepth", &Lut1DTransform::setFileOutputBitDepth, "bitDepth"_a)
        .def("getFormatMetadata", 
             (FormatMetadata & (Lut1DTransform::*)()) &Lut1DTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (Lut1DTransform::*)() const) 
             &Lut1DTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &Lut1DTransform::equals, "other"_a)
        .def("getLength", &Lut1DTransform::getLength)
        .def("setLength", &Lut1DTransform::setLength, "length"_a)
        .def("getValue", [](Lut1DTransformRcPtr & self, unsigned long index) 
            {
                float r, g, b;
                self->getValue(index, r, g, b);
                return py::make_tuple(r, g, b);
            }, 
            "index"_a)
        .def("setValue", &Lut1DTransform::setValue, "index"_a, "r"_a, "g"_a, "b"_a)
        .def("setData", [](Lut1DTransformRcPtr & self, py::buffer & data) 
            {
                py::buffer_info info = data.request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferDivisible(info, 3);

                py::gil_scoped_release release;

                unsigned long size = (info.size >= 0 ? static_cast<unsigned long>(info.size) : 0);
                float * values = static_cast<float *>(info.ptr);

                self->setLength(size / 3);

                for (unsigned long i = 0; i < size; i += 3)
                {
                    self->setValue(i / 3, values[i], values[i+1], values[i+2]);
                }
            },
             "data"_a)
        .def("getData", [](Lut1DTransformRcPtr & self) 
            {
                py::gil_scoped_release gil;

                std::vector<float> values;
                values.reserve(static_cast<size_t>(self->getLength() * 3));

                for (unsigned long i = 0; i < self->getLength(); i++)
                {
                    float r, g, b;
                    self->getValue(i, r, g, b);
                    values.push_back(r);
                    values.push_back(g);
                    values.push_back(b);
                }
                
                py::gil_scoped_acquire acquire;

                return py::array(py::dtype("float32"), 
                                 { static_cast<ssize_t>(values.size()) },
                                 { sizeof(float) },
                                 values.data());
            })
        .def("getInputHalfDomain", &Lut1DTransform::getInputHalfDomain)
        .def("setInputHalfDomain", &Lut1DTransform::setInputHalfDomain, "isHalfDomain"_a)
        .def("getOutputRawHalfs", &Lut1DTransform::getOutputRawHalfs)
        .def("setOutputRawHalfs", &Lut1DTransform::setOutputRawHalfs, "isRawHalfs"_a)
        .def("getHueAdjust", &Lut1DTransform::getHueAdjust)
        .def("setHueAdjust", &Lut1DTransform::setHueAdjust, "algo"_a)
        .def("getInterpolation", &Lut1DTransform::getInterpolation)
        .def("setInterpolation", &Lut1DTransform::setInterpolation, "algo"_a);
}

} // namespace OCIO_NAMESPACE
