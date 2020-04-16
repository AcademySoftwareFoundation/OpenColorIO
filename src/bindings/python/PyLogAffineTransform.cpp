// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLogAffineTransform(py::module & m)
{
    LogAffineTransformRcPtr DEFAULT = LogAffineTransform::Create();

    std::array<double, 3> DEFAULT_LOG_SIDE_SLOPE;
    DEFAULT->getLogSideSlopeValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LOG_SIDE_SLOPE.data()));

    std::array<double, 3> DEFAULT_LOG_SIDE_OFFSET;
    DEFAULT->getLogSideOffsetValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LOG_SIDE_OFFSET.data()));

    std::array<double, 3> DEFAULT_LIN_SIDE_SLOPE;
    DEFAULT->getLinSideSlopeValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LIN_SIDE_SLOPE.data()));

    std::array<double, 3> DEFAULT_LIN_SIDE_OFFSET;
    DEFAULT->getLinSideOffsetValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LIN_SIDE_OFFSET.data()));

    py::class_<LogAffineTransform, 
               LogAffineTransformRcPtr /* holder */, 
               Transform /* base */>(m, "LogAffineTransform")
        .def(py::init(&LogAffineTransform::Create))
        .def(py::init([](const std::array<double, 3> & logSideSlope,
                         const std::array<double, 3> & logSideOffset,
                         const std::array<double, 3> & linSideSlope,
                         const std::array<double, 3> & linSideOffset,
                         TransformDirection dir) 
            {
                LogAffineTransformRcPtr p = LogAffineTransform::Create();
                p->setLogSideSlopeValue(*reinterpret_cast<const double(*)[3]>(logSideSlope.data()));
                p->setLogSideOffsetValue(*reinterpret_cast<const double(*)[3]>(logSideOffset.data()));
                p->setLinSideSlopeValue(*reinterpret_cast<const double(*)[3]>(linSideSlope.data()));
                p->setLinSideOffsetValue(*reinterpret_cast<const double(*)[3]>(linSideOffset.data()));
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "logSideSlope"_a = DEFAULT_LOG_SIDE_SLOPE,
             "logSideOffset"_a = DEFAULT_LOG_SIDE_OFFSET,
             "linSideSlope"_a = DEFAULT_LIN_SIDE_SLOPE,
             "linSideOffset"_a = DEFAULT_LIN_SIDE_OFFSET,
             "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (LogAffineTransform::*)()) &LogAffineTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (LogAffineTransform::*)() const) 
             &LogAffineTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &LogAffineTransform::equals, "other"_a)
        .def("getBase", &LogAffineTransform::getBase)
        .def("setBase", &LogAffineTransform::setBase, "base"_a)
        .def("getLogSideSlopeValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLogSideSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLogSideSlopeValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLogSideSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLogSideOffsetValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLogSideOffsetValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLogSideOffsetValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLogSideOffsetValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLinSideSlopeValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLinSideSlopeValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLinSideSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLinSideOffsetValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideOffsetValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLinSideOffsetValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLinSideOffsetValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a);
}

} // namespace OCIO_NAMESPACE
