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

    auto clsLogAffineTransform = 
        py::class_<LogAffineTransform, LogAffineTransformRcPtr, Transform>(
            m.attr("LogAffineTransform"))

        .def(py::init(&LogAffineTransform::Create),
             DOC(LogAffineTransform, Create))
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
             "direction"_a = DEFAULT->getDirection(),
             DOC(LogAffineTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (LogAffineTransform::*)()) &LogAffineTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(LogAffineTransform, getFormatMetadata))
        .def("equals", &LogAffineTransform::equals, "other"_a,
             DOC(LogAffineTransform, equals))
        .def("getBase", &LogAffineTransform::getBase,
             DOC(LogAffineTransform, getBase))
        .def("setBase", &LogAffineTransform::setBase, "base"_a,
             DOC(LogAffineTransform, setBase))
        .def("getLogSideSlopeValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLogSideSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            },
             DOC(LogAffineTransform, getLogSideSlopeValue))
        .def("setLogSideSlopeValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLogSideSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a,
             DOC(LogAffineTransform, setLogSideSlopeValue))
        .def("getLogSideOffsetValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLogSideOffsetValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            },
             DOC(LogAffineTransform, getLogSideOffsetValue))
        .def("setLogSideOffsetValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLogSideOffsetValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a,
             DOC(LogAffineTransform, setLogSideOffsetValue))
        .def("getLinSideSlopeValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            },
             DOC(LogAffineTransform, getLinSideSlopeValue))
        .def("setLinSideSlopeValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLinSideSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a,
             DOC(LogAffineTransform, setLinSideSlopeValue))
        .def("getLinSideOffsetValue", [](LogAffineTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideOffsetValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            },
             DOC(LogAffineTransform, getLinSideOffsetValue))
        .def("setLinSideOffsetValue", [](LogAffineTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLinSideOffsetValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a,
             DOC(LogAffineTransform, setLinSideOffsetValue));

    defRepr(clsLogAffineTransform);
}

} // namespace OCIO_NAMESPACE
