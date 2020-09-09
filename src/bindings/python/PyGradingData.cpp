// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{
enum GradingDataIterator
{
    IT_CONTROL_POINT = 0,
};

using GradingControlPointIterator = PyIterator<GradingBSplineCurveRcPtr, IT_CONTROL_POINT>;

void CopyGradingBSpline(GradingBSplineCurveRcPtr to, const ConstGradingBSplineCurveRcPtr from)
{
    const size_t numPt = from->getNumControlPoints();
    to->setNumControlPoints(numPt);
    for (size_t pt = 0; pt < numPt; ++pt)
    {
        to->getControlPoint(pt) = from->getControlPoint(pt);
    }
}
}

void bindPyGradingData(py::module & m)
{
    py::class_<GradingRGBM>(m, "GradingRGBM")
        .def(py::init<>())
        .def(py::init<double, double, double, double>())
        .def_readwrite("red", &GradingRGBM::m_red)
        .def_readwrite("green", &GradingRGBM::m_green)
        .def_readwrite("blue", &GradingRGBM::m_blue)
        .def_readwrite("master", &GradingRGBM::m_master)
        .def("__repr__",
            [](const GradingRGBM & a)
        {
            std::ostringstream oss;
            oss << a;
            return oss.str();
        });

    py::class_<GradingPrimary>(m, "GradingPrimary")
        .def(py::init<GradingStyle>())
        .def("validate", &GradingPrimary::validate)
        .def_readwrite("brightness", &GradingPrimary::m_brightness)
        .def_readwrite("contrast", &GradingPrimary::m_contrast)
        .def_readwrite("gamma", &GradingPrimary::m_gamma)
        .def_readwrite("offset", &GradingPrimary::m_offset)
        .def_readwrite("exposure", &GradingPrimary::m_exposure)
        .def_readwrite("lift", &GradingPrimary::m_lift)
        .def_readwrite("gain", &GradingPrimary::m_gain)
        .def_readwrite("pivot", &GradingPrimary::m_pivot)
        .def_readwrite("saturation", &GradingPrimary::m_saturation)
        .def_readwrite("clampWhite", &GradingPrimary::m_clampWhite)
        .def_readwrite("clampBlack", &GradingPrimary::m_clampBlack)
        .def_readwrite("pivotWhite", &GradingPrimary::m_pivotWhite)
        .def_readwrite("pivotBlack", &GradingPrimary::m_pivotBlack)
        .def_property_readonly_static("NoClampBlack", [](py::object /* self */)
        {
            return GradingPrimary::NoClampBlack();
        })
        .def_property_readonly_static("NoClampWhite", [](py::object /* self */)
        {
            return GradingPrimary::NoClampWhite();
        })
        .def("__repr__",
            [](const GradingPrimary & a)
        {
            std::ostringstream oss;
            oss << a;
            return oss.str();
        });

    py::class_<GradingRGBMSW>(m, "GradingRGBMSW")
        .def(py::init<>())
        .def(py::init<double, double, double, double, double, double>())
        .def(py::init<double, double>())
        .def_readwrite("red", &GradingRGBMSW::m_red)
        .def_readwrite("green", &GradingRGBMSW::m_green)
        .def_readwrite("blue", &GradingRGBMSW::m_blue)
        .def_readwrite("master", &GradingRGBMSW::m_master)
        .def_readwrite("start", &GradingRGBMSW::m_start)
        .def_readwrite("width", &GradingRGBMSW::m_width)
        .def("__repr__",
                [](const GradingRGBMSW & a)
        {
            std::ostringstream oss;
            oss << a;
            return oss.str();
        });

    py::class_<GradingTone>(m, "GradingTone")
        .def(py::init<GradingStyle>())
        .def("validate", &GradingTone::validate)
        .def_readwrite("blacks", &GradingTone::m_blacks)
        .def_readwrite("whites", &GradingTone::m_whites)
        .def_readwrite("shadows", &GradingTone::m_shadows)
        .def_readwrite("highlights", &GradingTone::m_highlights)
        .def_readwrite("midtones", &GradingTone::m_midtones)
        .def_readwrite("scontrast", &GradingTone::m_scontrast)
        .def("__repr__",
            [](const GradingTone& a)
        {
            std::ostringstream oss;
            oss << a;
            return oss.str();
        });

    auto clsGCP = py::class_<GradingControlPoint>(m, "GradingControlPoint")
        .def(py::init<>())
        .def(py::init<float, float>())
        .def_readwrite("x", &GradingControlPoint::m_x)
        .def_readwrite("y", &GradingControlPoint::m_y)
        .def("__repr__", [](const GradingControlPoint & cp)
        {
            std::ostringstream oss;
            oss << cp;
            return oss.str();
        });

    auto clsGBSC = py::class_<GradingBSplineCurve, GradingBSplineCurveRcPtr /*holder*/>(m, "GradingBSplineCurve")
        .def(py::init([](size_t size)
            {
                GradingBSplineCurveRcPtr c = GradingBSplineCurve::Create(size);
                return c;
            }))
        .def(py::init([](const std::vector<float> & values)
            {
                const auto size = values.size();
                if (size < 4)
                {
                    throw Exception("GradingBSpline needs at least 4 values.");
                }
                if (size % 2 != 0)
                {
                    throw Exception("GradingBSpline needs an even number of values.");
                }
                auto numCtrlPts = size / 2;

                GradingBSplineCurveRcPtr c = GradingBSplineCurve::Create(numCtrlPts);
                for (size_t p = 0; p < numCtrlPts; ++p)
                {
                    c->getControlPoint(p).m_x = values[2 * p];
                    c->getControlPoint(p).m_y = values[2 * p + 1];
                }
                return c;
            }))
        .def("validate", &GradingBSplineCurve::validate)
        .def("setNumControlPoints", &GradingBSplineCurve::setNumControlPoints, "size"_a)
        .def("getControlPoints", [](GradingBSplineCurveRcPtr & self)
            {
                return GradingControlPointIterator(self);
            })
        .def("__repr__", [](const GradingBSplineCurveRcPtr & bs)
            {
                std::ostringstream oss;
                oss << *bs;
                return oss.str();
            });

    py::class_<GradingControlPointIterator>(clsGBSC, "GradingControlPointIterator")
        .def("__len__", [](GradingControlPointIterator & it)
        {
            return it.m_obj->getNumControlPoints();
        })
        .def("__getitem__", [](GradingControlPointIterator & it, int i)
        {
            return it.m_obj->getControlPoint(i);
        })
        .def("__setitem__", [](GradingControlPointIterator & it, int i, const GradingControlPoint & cpt)
        {
            it.m_obj->getControlPoint(i) = cpt;
        })
        .def("__iter__", [](GradingControlPointIterator & it) -> GradingControlPointIterator &
        {
            return it;
        })
        .def("__next__", [](GradingControlPointIterator & it)
        {
            int numPt = static_cast<int>(it.m_obj->getNumControlPoints());
            size_t i = static_cast<size_t>(it.nextIndex(numPt));
            return it.m_obj->getControlPoint(i);
        });


   GradingRGBCurveRcPtr DEFAULT_RGBCURVE = GradingRGBCurve::Create(GRADING_LOG);
   auto cls = py::class_<GradingRGBCurve, GradingRGBCurveRcPtr /*holder*/>(m, "GradingRGBCurve")
        .def(py::init([](GradingStyle style)
            {
                GradingRGBCurveRcPtr rgbcurve = GradingRGBCurve::Create(style);
                return rgbcurve;
            }))
        .def(py::init([](const GradingBSplineCurveRcPtr & red,
                         const GradingBSplineCurveRcPtr & green,
                         const GradingBSplineCurveRcPtr & blue, 
                         const GradingBSplineCurveRcPtr & master)
            {
                GradingRGBCurveRcPtr rgbcurve = GradingRGBCurve::Create(red, green, blue, master);
                return rgbcurve;
            }),
            "red"_a = DEFAULT_RGBCURVE->getCurve(RGB_RED),
            "green"_a = DEFAULT_RGBCURVE->getCurve(RGB_GREEN),
            "blue"_a = DEFAULT_RGBCURVE->getCurve(RGB_BLUE),
            "master"_a = DEFAULT_RGBCURVE->getCurve(RGB_MASTER))
       .def_property("red", [](const GradingRGBCurveRcPtr & rgbcurve)
            {
                return rgbcurve->getCurve(RGB_RED);
            },
            [](const GradingRGBCurveRcPtr & rgbCurve, const GradingBSplineCurveRcPtr & red)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_RED), red);
            })
        .def_property("green", [](const GradingRGBCurveRcPtr & rgbcurve)
            {
                return rgbcurve->getCurve(RGB_GREEN);
            },
                    [](const GradingRGBCurveRcPtr & rgbCurve, const GradingBSplineCurveRcPtr & green)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_GREEN), green);
            })
        .def_property("blue", [](const GradingRGBCurveRcPtr & rgbcurve)
            {
                return rgbcurve->getCurve(RGB_BLUE);
            },
                    [](const GradingRGBCurveRcPtr & rgbCurve, const GradingBSplineCurveRcPtr & blue)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_BLUE), blue);
            })
       .def_property("master", [](const GradingRGBCurveRcPtr & rgbcurve)
            {
                return rgbcurve->getCurve(RGB_MASTER);
            },
            [](const GradingRGBCurveRcPtr & rgbCurve, const GradingBSplineCurveRcPtr & master)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_MASTER), master);
            })
        .def("__repr__", [](const GradingRGBCurveRcPtr & rgbcurve)
            {
                std::ostringstream oss;
                oss << *rgbcurve;
                return oss.str();
            });

}

} // namespace OCIO_NAMESPACE
