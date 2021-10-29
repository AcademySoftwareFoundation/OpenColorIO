// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLut3DTransform(py::module & m)
{
    Lut3DTransformRcPtr DEFAULT = Lut3DTransform::Create();

    auto clsLut3DTransform = 
        py::class_<Lut3DTransform, Lut3DTransformRcPtr, Transform>(
            m.attr("Lut3DTransform"))

        .def(py::init([]() 
            { 
                return Lut3DTransform::Create(); 
            }),
             DOC(Lut3DTransform, Create))
        .def(py::init([](unsigned long gridSize) 
            { 
                return Lut3DTransform::Create(gridSize); 
            }),
             "gridSize"_a,
             DOC(Lut3DTransform, Create, 2))
        .def(py::init([](unsigned long gridSize,
                         BitDepth fileOutputBitDepth,
                         Interpolation interpolation,
                         TransformDirection dir)
            {
                Lut3DTransformRcPtr p = Lut3DTransform::Create(gridSize);
                p->setFileOutputBitDepth(fileOutputBitDepth);
                p->setInterpolation(interpolation);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "gridSize"_a = DEFAULT->getGridSize(),
             "fileOutputBitDepth"_a = DEFAULT->getFileOutputBitDepth(),
             "interpolation"_a = DEFAULT->getInterpolation(),
             "direction"_a = DEFAULT->getDirection(),
             DOC(Lut3DTransform, Create, 2))

        .def("getFileOutputBitDepth", &Lut3DTransform::getFileOutputBitDepth,
             DOC(Lut3DTransform, getFileOutputBitDepth))
        .def("setFileOutputBitDepth", &Lut3DTransform::setFileOutputBitDepth, "bitDepth"_a,
             DOC(Lut3DTransform, setFileOutputBitDepth))
        .def("getFormatMetadata", 
             (FormatMetadata & (Lut3DTransform::*)()) &Lut3DTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(Lut3DTransform, getFormatMetadata))
        .def("equals", &Lut3DTransform::equals, "other"_a,
             DOC(Lut3DTransform, equals))
        .def("getGridSize", &Lut3DTransform::getGridSize,
             DOC(Lut3DTransform, getGridSize))
        .def("setGridSize", &Lut3DTransform::setGridSize, "gridSize"_a,
             DOC(Lut3DTransform, setGridSize))
        .def("getValue", [](Lut3DTransformRcPtr & self, 
                            unsigned long indexR, 
                            unsigned long indexG, 
                            unsigned long indexB) 
            {
                float r, g, b;
                self->getValue(indexR, indexG, indexB, r, g, b);
                return py::make_tuple(r, g, b);
            }, 
            "indexR"_a, "indexG"_a, "indexB"_a,
             DOC(Lut3DTransform, getValue))
        .def("setValue", &Lut3DTransform::setValue, 
             "indexR"_a, "indexG"_a, "indexB"_a, "r"_a, "g"_a, "b"_a,
             DOC(Lut3DTransform, setValue))
        .def("setData", [](Lut3DTransformRcPtr & self, py::buffer & data) 
            {
                py::buffer_info info = data.request();
                checkBufferType(info, py::dtype("float32"));
                
                unsigned long gs = getBufferLut3DGridSize(info);

                py::gil_scoped_release gil;

                self->setGridSize(gs);

                float * values = static_cast<float *>(info.ptr);

                for (unsigned long indexR = 0; indexR < gs; indexR++)
                {
                    for (unsigned long indexG = 0; indexG < gs; indexG++)
                    {
                        for (unsigned long indexB = 0; indexB < gs; indexB++)
                        {
                            const unsigned long i = 3 * ((indexR*gs + indexG)*gs + indexB);
                            self->setValue(indexR, indexG, indexB, 
                                            values[i], 
                                            values[i+1], 
                                            values[i+2]);
                        }
                    }
                }
            },
             "data"_a)
        .def("getData", [](Lut3DTransformRcPtr & self) 
            {
                py::gil_scoped_release release;

                unsigned long gs = self->getGridSize();
                std::vector<float> values;
                values.reserve(static_cast<size_t>(gs*gs*gs * 3));

                for (unsigned long indexR = 0; indexR < gs; indexR++)
                {
                    for (unsigned long indexG = 0; indexG < gs; indexG++)
                    {
                        for (unsigned long indexB = 0; indexB < gs; indexB++)
                        {
                            float r, g, b;
                            self->getValue(indexR, indexG, indexB, r, g, b);
                            values.push_back(r);
                            values.push_back(g);
                            values.push_back(b);
                        }
                    }
                }

                py::gil_scoped_acquire acquire;

                return py::array(py::dtype("float32"), 
                                 { static_cast<py::ssize_t>(values.size()) },
                                 { sizeof(float) },
                                 values.data());
            })
        .def("getInterpolation", &Lut3DTransform::getInterpolation,
             DOC(Lut3DTransform, getInterpolation))
        .def("setInterpolation", &Lut3DTransform::setInterpolation, "interpolation"_a,
             DOC(Lut3DTransform, setInterpolation));

    defRepr(clsLut3DTransform);
}

} // namespace OCIO_NAMESPACE
