// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

// TODO: The 'reinterpret_cast' is a short-term solution waiting for the new C++
// syntax to pass RGBA values as argument. While waiting for the final solution,
// disable the SonarCloud warnings.
/* @SuppressWarnings('all') */

void bindPyLogCameraTransform(py::module & m)
{
    LogCameraTransformRcPtr DEFAULT = LogCameraTransform::Create();

    std::array<double, 3> DEFAULT_LOG_SIDE_SLOPE;
    DEFAULT->getLogSideSlopeValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LOG_SIDE_SLOPE.data()));

    std::array<double, 3> DEFAULT_LOG_SIDE_OFFSET;
    DEFAULT->getLogSideOffsetValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LOG_SIDE_OFFSET.data()));

    std::array<double, 3> DEFAULT_LIN_SIDE_SLOPE;
    DEFAULT->getLinSideSlopeValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LIN_SIDE_SLOPE.data()));

    std::array<double, 3> DEFAULT_LIN_SIDE_OFFSET;
    DEFAULT->getLinSideOffsetValue(*reinterpret_cast<double(*)[3]>(DEFAULT_LIN_SIDE_OFFSET.data()));

    py::class_<LogCameraTransform, 
               LogCameraTransformRcPtr /* holder */, 
               Transform /* base */>(m, "LogCameraTransform")
        .def(py::init(&LogCameraTransform::Create))
        .def(py::init([](const std::array<double, 3> & linSideBreak,
                         const std::array<double, 3> & logSideSlope,
                         const std::array<double, 3> & logSideOffset,
                         const std::array<double, 3> & linSideSlope,
                         const std::array<double, 3> & linSideOffset,
                         TransformDirection dir) 
            {
                LogCameraTransformRcPtr p = LogCameraTransform::Create();
                p->setLinSideBreakValue(*reinterpret_cast<const double(*)[3]>(linSideBreak.data()));
                p->setLogSideSlopeValue(*reinterpret_cast<const double(*)[3]>(logSideSlope.data()));
                p->setLogSideOffsetValue(*reinterpret_cast<const double(*)[3]>(logSideOffset.data()));
                p->setLinSideSlopeValue(*reinterpret_cast<const double(*)[3]>(linSideSlope.data()));
                p->setLinSideOffsetValue(*reinterpret_cast<const double(*)[3]>(linSideOffset.data()));
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "linSideBreak"_a,
             "logSideSlope"_a = DEFAULT_LOG_SIDE_SLOPE,
             "logSideOffset"_a = DEFAULT_LOG_SIDE_OFFSET,
             "linSideSlope"_a = DEFAULT_LIN_SIDE_SLOPE,
             "linSideOffset"_a = DEFAULT_LIN_SIDE_OFFSET,
             "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (LogCameraTransform::*)()) &LogCameraTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (LogCameraTransform::*)() const) 
             &LogCameraTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &LogCameraTransform::equals, "other"_a)
        .def("getBase", &LogCameraTransform::getBase)
        .def("setBase", &LogCameraTransform::setBase, "base"_a)
        .def("getLogSideSlopeValue", [](LogCameraTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLogSideSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLogSideSlopeValue", [](LogCameraTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLogSideSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLogSideOffsetValue", [](LogCameraTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLogSideOffsetValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLogSideOffsetValue", [](LogCameraTransformRcPtr self, 
                                         const std::array<double, 3> & values)
            { 
                self->setLogSideOffsetValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLinSideSlopeValue", [](LogCameraTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLinSideSlopeValue", [](LogCameraTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLinSideSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLinSideOffsetValue", [](LogCameraTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideOffsetValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLinSideOffsetValue", [](LogCameraTransformRcPtr self, 
                                         const std::array<double, 3> & values)
            { 
                self->setLinSideOffsetValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLinSideBreakValue", [](LogCameraTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinSideBreakValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLinSideBreakValue", [](LogCameraTransformRcPtr self, 
                                        const std::array<double, 3> & values)
            { 
                self->setLinSideBreakValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("getLinearSlopeValue", [](LogCameraTransformRcPtr self)
            {
                std::array<double, 3> values;
                self->getLinearSlopeValue(*reinterpret_cast<double(*)[3]>(values.data()));
                return values;
            })
        .def("setLinearSlopeValue", [](LogCameraTransformRcPtr self, 
                                       const std::array<double, 3> & values)
            { 
                self->setLinearSlopeValue(*reinterpret_cast<const double(*)[3]>(values.data()));
            }, 
             "values"_a)
        .def("unsetLinearSlopeValue", &LogCameraTransform::unsetLinearSlopeValue);
}

} // namespace OCIO_NAMESPACE
