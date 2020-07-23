// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyDisplayViewTransform(py::module & m)
{
    DisplayViewTransformRcPtr DEFAULT = DisplayViewTransform::Create();

    auto cls = py::class_<DisplayViewTransform, 
                          DisplayViewTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "DisplayViewTransform", DS(DisplayViewTransform))
        .def(py::init(&DisplayViewTransform::Create), DS(DisplayViewTransform, Create))
        .def(py::init([](const std::string & src,
                         const std::string & display,
                         const std::string & view,
                         bool looksBypass,
                         bool dataBypass,
                         TransformDirection dir) 
                      {
                          DisplayViewTransformRcPtr p = DisplayViewTransform::Create();
                          if (!src.empty())     { p->setSrc(src.c_str()); }
                          if (!display.empty()) { p->setDisplay(display.c_str()); }
                          if (!view.empty())    { p->setView(view.c_str()); }
                          p->setLooksBypass(looksBypass);
                          p->setDataBypass(dataBypass);
                          p->setDirection(dir);
                          p->validate();
                          return p;
                      }), 
             DS(DisplayViewTransform, DisplayViewTransform),
             "src"_a = DEFAULT->getSrc(),
             "display"_a = DEFAULT->getDisplay(),
             "view"_a = DEFAULT->getView(),
             "looksBypass"_a = DEFAULT->getLooksBypass(),
             "dataBypass"_a = DEFAULT->getDataBypass(),
             "direction"_a = DEFAULT->getDirection())

        .def("getSrc", &DisplayViewTransform::getSrc, DS(DisplayViewTransform, getSrc))
        .def("setSrc", &DisplayViewTransform::setSrc, DS(DisplayViewTransform, setSrc), "src"_a)
        .def("getDisplay", &DisplayViewTransform::getDisplay, DS(DisplayViewTransform, getDisplay))
        .def("setDisplay", &DisplayViewTransform::setDisplay, DS(DisplayViewTransform, setDisplay), "display"_a)
        .def("getView", &DisplayViewTransform::getView, DS(DisplayViewTransform, getView))
        .def("setView", &DisplayViewTransform::setView, DS(DisplayViewTransform, setView), "view"_a)
        .def("getLooksBypass", &DisplayViewTransform::getLooksBypass, DS(DisplayViewTransform, getLooksBypass))
        .def("setLooksBypass", &DisplayViewTransform::setLooksBypass, DS(DisplayViewTransform, setLooksBypass), "looksBypass"_a)
        .def("getDataBypass", &DisplayViewTransform::getDataBypass, DS(DisplayViewTransform, getDataBypass))
        .def("setDataBypass", &DisplayViewTransform::setDataBypass, DS(DisplayViewTransform, setDataBypass), "dataBypass"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
