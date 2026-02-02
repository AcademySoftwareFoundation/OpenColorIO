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

} // namespace

void bindPyGradingData(py::module & m)
{
    GradingRGBCurveRcPtr DEFAULT_RGB_CURVE = GradingRGBCurve::Create(GRADING_LOG);
    GradingControlPoint DEFAULT_CONTROL_POINT = GradingControlPoint();

    auto clsGradingRGBM = 
        py::class_<GradingRGBM>(
            m.attr("GradingRGBM"));

    auto clsGradingPrimary = 
        py::class_<GradingPrimary>(
            m.attr("GradingPrimary"));

    auto clsGradingRGBMSW = 
        py::class_<GradingRGBMSW>(
            m.attr("GradingRGBMSW"));

    auto clsGradingTone = 
        py::class_<GradingTone>(
            m.attr("GradingTone"));

    auto clsGradingControlPoint = 
        py::class_<GradingControlPoint>(
            m.attr("GradingControlPoint"));

    auto clsGradingBSplineCurve = 
        py::class_<GradingBSplineCurve, GradingBSplineCurveRcPtr /*holder*/>(
            m.attr("GradingBSplineCurve"));

    auto clsGradingControlPointIterator = 
        py::class_<GradingControlPointIterator>(
            clsGradingBSplineCurve, "GradingControlPointIterator");

    auto clsGradingRGBCurve = 
        py::class_<GradingRGBCurve, GradingRGBCurveRcPtr /*holder*/>(
            m.attr("GradingRGBCurve"));

    auto clsGradingHueCurve = 
        py::class_<GradingHueCurve, GradingHueCurveRcPtr /*holder*/>(
            m.attr("GradingHueCurve"));

    clsGradingRGBM
        .def(py::init<>(), 
             DOC(GradingRGBM, GradingRGBM))
        .def(py::init<double, double, double, double>(), 
             "red"_a, "green"_a, "blue"_a, "master"_a,
             DOC(GradingRGBM, GradingRGBM, 2))

        .def("__eq__", [](const GradingRGBM &self, const GradingRGBM &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingRGBM &self, const GradingRGBM &other)
            {
                return self != other;
            }, py::is_operator())

        .def_readwrite("red", &GradingRGBM::m_red, 
                       DOC(GradingRGBM, m_red))
        .def_readwrite("green", &GradingRGBM::m_green, 
                       DOC(GradingRGBM, m_green))
        .def_readwrite("blue", &GradingRGBM::m_blue, 
                       DOC(GradingRGBM, m_blue))
        .def_readwrite("master", &GradingRGBM::m_master, 
                       DOC(GradingRGBM, m_master));

    defRepr(clsGradingRGBM);

    clsGradingPrimary
        .def(py::init<GradingStyle>(), 
             DOC(GradingPrimary, GradingPrimary))

        .def("__eq__", [](const GradingPrimary &self, const GradingPrimary &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingPrimary &self, const GradingPrimary &other)
            {
                return self != other;
            }, py::is_operator())

        .def("validate", &GradingPrimary::validate, 
             DOC(GradingPrimary, validate))

        .def_readwrite("brightness", &GradingPrimary::m_brightness, 
                       DOC(GradingPrimary, m_brightness))
        .def_readwrite("contrast", &GradingPrimary::m_contrast, 
                       DOC(GradingPrimary, m_contrast))
        .def_readwrite("gamma", &GradingPrimary::m_gamma, 
                       DOC(GradingPrimary, m_gamma))
        .def_readwrite("offset", &GradingPrimary::m_offset, 
                       DOC(GradingPrimary, m_offset))
        .def_readwrite("exposure", &GradingPrimary::m_exposure, 
                       DOC(GradingPrimary, m_exposure))
        .def_readwrite("lift", &GradingPrimary::m_lift, 
                       DOC(GradingPrimary, m_lift))
        .def_readwrite("gain", &GradingPrimary::m_gain, 
                       DOC(GradingPrimary, m_gain))
        .def_readwrite("pivot", &GradingPrimary::m_pivot, 
                       DOC(GradingPrimary, m_pivot))
        .def_readwrite("saturation", &GradingPrimary::m_saturation, 
                       DOC(GradingPrimary, m_saturation))
        .def_readwrite("clampWhite", &GradingPrimary::m_clampWhite, 
                       DOC(GradingPrimary, m_clampWhite))
        .def_readwrite("clampBlack", &GradingPrimary::m_clampBlack, 
                       DOC(GradingPrimary, m_clampBlack))
        .def_readwrite("pivotWhite", &GradingPrimary::m_pivotWhite, 
                       DOC(GradingPrimary, m_pivotWhite))
        .def_readwrite("pivotBlack", &GradingPrimary::m_pivotBlack, 
                       DOC(GradingPrimary, m_pivotBlack))

        .def_property_readonly_static("NoClampBlack", [](py::object /* self */)
            {
                return GradingPrimary::NoClampBlack();
            }, 
                                      DOC(GradingPrimary, m_brightness))
        .def_property_readonly_static("NoClampWhite", [](py::object /* self */)
            {
                return GradingPrimary::NoClampWhite();
            }, 
                                      DOC(GradingPrimary, m_brightness));

    defRepr(clsGradingPrimary);

    clsGradingRGBMSW
        .def(py::init<>(), 
             DOC(GradingRGBMSW, GradingRGBMSW))
        .def(py::init<double, double, double, double, double, double>(), 
             "red"_a, "green"_a, "blue"_a, "master"_a, "start"_a, "width"_a,
             DOC(GradingRGBMSW, GradingRGBMSW, 2))
        .def(py::init<double, double>(), 
             "start"_a, "width"_a,
             DOC(GradingRGBMSW, GradingRGBMSW, 3))

        .def("__eq__", [](const GradingRGBMSW &self, const GradingRGBMSW &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingRGBMSW &self, const GradingRGBMSW &other)
            {
                return self != other;
            }, py::is_operator())

        .def_readwrite("red", &GradingRGBMSW::m_red, 
                       DOC(GradingRGBMSW, m_red))
        .def_readwrite("green", &GradingRGBMSW::m_green, 
                       DOC(GradingRGBMSW, m_green))
        .def_readwrite("blue", &GradingRGBMSW::m_blue, 
                       DOC(GradingRGBMSW, m_blue))
        .def_readwrite("master", &GradingRGBMSW::m_master, 
                       DOC(GradingRGBMSW, m_master))
        .def_readwrite("start", &GradingRGBMSW::m_start, 
                       DOC(GradingRGBMSW, m_start))
        .def_readwrite("width", &GradingRGBMSW::m_width, 
                       DOC(GradingRGBMSW, m_width));

    defRepr(clsGradingRGBMSW);

    clsGradingTone
        .def(py::init<GradingStyle>(), 
             DOC(GradingTone, GradingTone))

        .def("validate", &GradingTone::validate, 
             DOC(GradingTone, validate))

        .def("__eq__", [](const GradingTone &self, const GradingTone &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingTone &self, const GradingTone &other)
            {
                return self != other;
            }, py::is_operator())

        .def_readwrite("blacks", &GradingTone::m_blacks, 
                       DOC(GradingTone, m_blacks))
        .def_readwrite("whites", &GradingTone::m_whites, 
                       DOC(GradingTone, m_whites))
        .def_readwrite("shadows", &GradingTone::m_shadows, 
                       DOC(GradingTone, m_shadows))
        .def_readwrite("highlights", &GradingTone::m_highlights, 
                       DOC(GradingTone, m_highlights))
        .def_readwrite("midtones", &GradingTone::m_midtones, 
                       DOC(GradingTone, m_midtones))
        .def_readwrite("scontrast", &GradingTone::m_scontrast, 
                       DOC(GradingTone, m_scontrast));

    defRepr(clsGradingTone);

    clsGradingControlPoint
        .def(py::init<>(), 
             DOC(GradingControlPoint, GradingControlPoint))
        .def(py::init<float, float>(), 
             "x"_a = DEFAULT_CONTROL_POINT.m_x, 
             "y"_a = DEFAULT_CONTROL_POINT.m_y,
             DOC(GradingControlPoint, GradingControlPoint, 2))

        .def("__eq__", [](const GradingControlPoint &self, const GradingControlPoint &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingControlPoint &self, const GradingControlPoint &other)
            {
                return self != other;
            }, py::is_operator())

        .def_readwrite("x", &GradingControlPoint::m_x, 
                       DOC(GradingControlPoint, m_x))
        .def_readwrite("y", &GradingControlPoint::m_y, 
                       DOC(GradingControlPoint, m_y));

    defRepr(clsGradingControlPoint);

    clsGradingBSplineCurve
        .def(py::init([](size_t size)
            {
                return GradingBSplineCurve::Create(size);
            }),
             "size"_a,
             DOC(GradingBSplineCurve, Create))
        .def(py::init([](size_t size,
                         HueCurveType curveType)
            {
                return GradingBSplineCurve::Create(size, curveType);
            }),
             "size"_a,
             "huecurvetype"_a,
             DOC(GradingBSplineCurve, Create))
        .def(py::init([](size_t size,
                         BSplineType splineType)
            {
                return GradingBSplineCurve::Create(size, splineType);
            }),
             "size"_a,
             "splinetype"_a,
             DOC(GradingBSplineCurve, Create))
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
            }),
             DOC(GradingBSplineCurve, Create, 2))
        .def(py::init([](const std::vector<float> & values,
                         HueCurveType curveType)
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

                GradingBSplineCurveRcPtr c = GradingBSplineCurve::Create(numCtrlPts, curveType);
                for (size_t p = 0; p < numCtrlPts; ++p)
                {
                    c->getControlPoint(p).m_x = values[2 * p];
                    c->getControlPoint(p).m_y = values[2 * p + 1];
                }
                return c;
            }),
             DOC(GradingBSplineCurve, Create, 2))

        .def("__eq__", [](const GradingBSplineCurve &self, const GradingBSplineCurve &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingBSplineCurve &self, const GradingBSplineCurve &other)
            {
                return self != other;
            }, py::is_operator())

        .def("validate", &GradingBSplineCurve::validate, 
             DOC(GradingBSplineCurve, validate))
        .def("setNumControlPoints", &GradingBSplineCurve::setNumControlPoints, "size"_a, 
             DOC(GradingBSplineCurve, setNumControlPoints))
        .def("getControlPoints", [](GradingBSplineCurveRcPtr & self)
            {
                return GradingControlPointIterator(self);
            })
        .def("getSplineType", &GradingBSplineCurve::getSplineType, 
             DOC(GradingBSplineCurve, getSplineType))
        .def("setSplineType", &GradingBSplineCurve::setSplineType, "splinetype"_a,
             DOC(GradingBSplineCurve, setSplineType))
        .def("slopesAreDefault", &GradingBSplineCurve::slopesAreDefault, 
             DOC(GradingBSplineCurve, slopesAreDefault))
        .def("getSlopes", [](GradingBSplineCurveRcPtr self)
            {
                std::vector<float> slopes;
                const size_t numPts = self->getNumControlPoints();
                slopes.resize(numPts);
                for (size_t pt = 0; pt < numPts; ++pt)
                {
                    slopes[pt] = self->getSlope(pt);
                }
                return slopes;
            })
        .def("setSlopes", [](GradingBSplineCurveRcPtr self, const std::vector<float> & slopes)
            {
                const size_t numPts = self->getNumControlPoints();
                const auto size = slopes.size();
                if (size != numPts)
                {
                    throw Exception("Length of slopes vector must match number of control points.");
                }
                for (size_t pt = 0; pt < numPts; ++pt)
                {
                    self->setSlope(pt, slopes[pt]);
                }
            });

    defRepr(clsGradingBSplineCurve);

    clsGradingControlPointIterator
        .def("__len__", [](GradingControlPointIterator & it)
            {
                return it.m_obj->getNumControlPoints();
            })
        .def("__getitem__", [](GradingControlPointIterator & it, int i)
            {
                return it.m_obj->getControlPoint(i);
            })
        .def("__setitem__", [](GradingControlPointIterator & it, 
                               int i, 
                               const GradingControlPoint & cpt)
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

    clsGradingRGBCurve
        .def(py::init([](GradingStyle style)
            {
                return GradingRGBCurve::Create(style);
            }), 
             "style"_a,
             DOC(GradingRGBCurve, GradingRGBCurve))
        .def(py::init([](const GradingBSplineCurveRcPtr & red,
                         const GradingBSplineCurveRcPtr & green,
                         const GradingBSplineCurveRcPtr & blue, 
                         const GradingBSplineCurveRcPtr & master)
            {
                return GradingRGBCurve::Create(red, green, blue, master);
            }),
                         "red"_a = DEFAULT_RGB_CURVE->getCurve(RGB_RED),
                         "green"_a = DEFAULT_RGB_CURVE->getCurve(RGB_GREEN),
                         "blue"_a = DEFAULT_RGB_CURVE->getCurve(RGB_BLUE),
                         "master"_a = DEFAULT_RGB_CURVE->getCurve(RGB_MASTER),
                         DOC(GradingRGBCurve, GradingRGBCurve, 2))

        .def("__eq__", [](const GradingRGBCurve &self, const GradingRGBCurve &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingRGBCurve &self, const GradingRGBCurve &other)
            {
                return self != other;
            }, py::is_operator())

        .def("validate", &GradingRGBCurve::validate, 
             DOC(GradingRGBCurve, validate))
        .def("isIdentity", &GradingRGBCurve::isIdentity, 
             DOC(GradingRGBCurve, isIdentity))

       .def_property("red", 
                     [](const GradingRGBCurveRcPtr & rgbCurve)
            {
                return rgbCurve->getCurve(RGB_RED);
            },
                     [](const GradingRGBCurveRcPtr & rgbCurve, 
                        const GradingBSplineCurveRcPtr & red)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_RED), red);
            })
        .def_property("green", 
                      [](const GradingRGBCurveRcPtr & rgbCurve)
            {
                return rgbCurve->getCurve(RGB_GREEN);
            },
                      [](const GradingRGBCurveRcPtr & rgbCurve, 
                         const GradingBSplineCurveRcPtr & green)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_GREEN), green);
            })
        .def_property("blue", 
                      [](const GradingRGBCurveRcPtr & rgbCurve)
            {
                return rgbCurve->getCurve(RGB_BLUE);
            },
                      [](const GradingRGBCurveRcPtr & rgbCurve, 
                         const GradingBSplineCurveRcPtr & blue)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_BLUE), blue);
            })
       .def_property("master", 
                     [](const GradingRGBCurveRcPtr & rgbCurve)
            {
                return rgbCurve->getCurve(RGB_MASTER);
            },
                     [](const GradingRGBCurveRcPtr & rgbCurve, 
                        const GradingBSplineCurveRcPtr & master)
            {
                CopyGradingBSpline(rgbCurve->getCurve(RGB_MASTER), master);
            });

    defRepr(clsGradingHueCurve);

    clsGradingHueCurve
        .def(py::init([](GradingStyle style)
            {
                return GradingHueCurve::Create(style);
            }), 
             "style"_a,
             DOC(GradingHueCurve, GradingHueCurve))
        .def(py::init([](const GradingBSplineCurveRcPtr & hue_hue,
                         const GradingBSplineCurveRcPtr & hue_sat,
                         const GradingBSplineCurveRcPtr & hue_lum, 
                         const GradingBSplineCurveRcPtr & lum_sat, 
                         const GradingBSplineCurveRcPtr & sat_sat, 
                         const GradingBSplineCurveRcPtr & lum_lum, 
                         const GradingBSplineCurveRcPtr & sat_lum, 
                         const GradingBSplineCurveRcPtr & hue_fx)
            {
                return GradingHueCurve::Create(hue_hue, hue_sat, hue_lum, lum_sat,
                                               sat_sat, lum_lum, sat_lum, hue_fx);
            }),
                DOC(GradingHueCurve, GradingHueCurve, 2))

        .def("__eq__", [](const GradingHueCurve &self, const GradingHueCurve &other)
            {
                return self == other;
            }, py::is_operator())
        .def("__ne__", [](const GradingHueCurve &self, const GradingHueCurve &other)
            {
                return self != other;
            }, py::is_operator())

        .def("validate", &GradingHueCurve::validate, 
             DOC(GradingHueCurve, validate))
        .def("isIdentity", &GradingHueCurve::isIdentity, 
             DOC(GradingHueCurve, isIdentity))
        .def("getDrawCurveOnly", &GradingHueCurve::getDrawCurveOnly, 
             DOC(GradingHueCurve, getDrawCurveOnly))
        .def("setDrawCurveOnly", &GradingHueCurve::setDrawCurveOnly, "drawcurveonly"_a, 
             DOC(GradingHueCurve, setDrawCurveOnly))

       .def_property("hue_hue", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(HUE_HUE);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & hue_hue)
            {
                CopyGradingBSpline(hueCurve->getCurve(HUE_HUE), hue_hue);
            })
       .def_property("hue_sat", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(HUE_SAT);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & hue_sat)
            {
                CopyGradingBSpline(hueCurve->getCurve(HUE_SAT), hue_sat);
            })
       .def_property("hue_lum", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(HUE_LUM);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & hue_lum)
            {
                CopyGradingBSpline(hueCurve->getCurve(HUE_LUM), hue_lum);
            })
       .def_property("lum_sat", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(LUM_SAT);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & lum_sat)
            {
                CopyGradingBSpline(hueCurve->getCurve(LUM_SAT), lum_sat);
            })
       .def_property("sat_sat", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(SAT_SAT);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & sat_sat)
            {
                CopyGradingBSpline(hueCurve->getCurve(SAT_SAT), sat_sat);
            })
       .def_property("lum_lum", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(LUM_LUM);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & lum_lum)
            {
                CopyGradingBSpline(hueCurve->getCurve(LUM_LUM), lum_lum);
            })
       .def_property("sat_lum", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(SAT_LUM);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & sat_lum)
            {
                CopyGradingBSpline(hueCurve->getCurve(SAT_LUM), sat_lum);
            })
       .def_property("hue_fx", 
                     [](const GradingHueCurveRcPtr & hueCurve)
            {
                return hueCurve->getCurve(HUE_FX);
            },
                     [](const GradingHueCurveRcPtr & hueCurve, 
                        const GradingBSplineCurveRcPtr & hue_fx)
            {
                CopyGradingBSpline(hueCurve->getCurve(HUE_FX), hue_fx);
            });

    defRepr(clsGradingHueCurve);
}

} // namespace OCIO_NAMESPACE
