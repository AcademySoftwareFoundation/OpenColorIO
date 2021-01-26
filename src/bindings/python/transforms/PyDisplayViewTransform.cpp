// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyDisplayViewTransform(py::module & m)
{
    DisplayViewTransformRcPtr DEFAULT = DisplayViewTransform::Create();

    auto clsDisplayViewTransform = 
        py::class_<DisplayViewTransform, DisplayViewTransformRcPtr, Transform>(
            m.attr("DisplayViewTransform"))

        .def(py::init(&DisplayViewTransform::Create), 
             DOC(DisplayViewTransform, Create))
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
             "src"_a = DEFAULT->getSrc(),
             "display"_a = DEFAULT->getDisplay(),
             "view"_a = DEFAULT->getView(),
             "looksBypass"_a = DEFAULT->getLooksBypass(),
             "dataBypass"_a = DEFAULT->getDataBypass(),
             "direction"_a = DEFAULT->getDirection(), 
             DOC(DisplayViewTransform, Create))

        .def("getSrc", &DisplayViewTransform::getSrc, 
             DOC(DisplayViewTransform, getSrc))
        .def("setSrc", &DisplayViewTransform::setSrc, "src"_a, 
             DOC(DisplayViewTransform, setSrc))
        .def("getDisplay", &DisplayViewTransform::getDisplay, 
             DOC(DisplayViewTransform, getDisplay))
        .def("setDisplay", &DisplayViewTransform::setDisplay, "display"_a, 
             DOC(DisplayViewTransform, setDisplay))
        .def("getView", &DisplayViewTransform::getView, 
             DOC(DisplayViewTransform, getView))
        .def("setView", &DisplayViewTransform::setView, "view"_a, 
             DOC(DisplayViewTransform, setView))
        .def("getLooksBypass", &DisplayViewTransform::getLooksBypass, 
             DOC(DisplayViewTransform, getLooksBypass))
        .def("setLooksBypass", &DisplayViewTransform::setLooksBypass, "looksBypass"_a, 
             DOC(DisplayViewTransform, setLooksBypass))
        .def("getDataBypass", &DisplayViewTransform::getDataBypass, 
             DOC(DisplayViewTransform, getDataBypass))
        .def("setDataBypass", &DisplayViewTransform::setDataBypass, "dataBypass"_a, 
             DOC(DisplayViewTransform, setDataBypass));

    defRepr(clsDisplayViewTransform);
}

} // namespace OCIO_NAMESPACE
