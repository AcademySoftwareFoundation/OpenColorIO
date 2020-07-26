// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "oglapp.h"

#include "PyOpenColorIO_oglapphelpers.h"

namespace OCIO_NAMESPACE
{
void bindPyOglApp(py::module & m)
{
    auto cls = py::class_<OglApp, OglAppRcPtr /* holder */>(m, "OglApp")
            .def(py::init([](int width, int height)
                {
                    OglAppRcPtr p = std::make_shared<OglApp>(width, height);
                    return p;
                }))
            .def("setYMirror", &OglApp::setYMirror)
            .def("setPrintShader", &OglApp::setPrintShader, "print"_a)
            .def("initImage", &OglApp::initImage,
                 "imageWidth"_a,
                 "imageHeight"_a,
                 "Components"_a,
                 "imageBuffer"_a)
            .def("updateImage", &OglApp::updataImage, "imageBuffer"_a)
            .def("createGLBuffer", &OglApp::createGLBuffer)
            .def("setShader", &OglApp::setShader)
            .def("reshape", &OglApp::reshape,
                 "width"_a,
                 "height"_a)
            .def("updateUniforms", &OglApp::updateUniforms)
            .def("redisplay", &OglApp::redisplay)
            .def("readImage", &OglApp::readImage)
            .def("printGLInfo", &OglApp::printGLInfo)
            .def_static("CreateOglApp", &OglApp::CreateOglApp,
                        "winTitle"_a,
                        "winWidth"_a,
                        "winHeight"_a)
            .def("setUpCommon", &OglApp::setUpCommon);

    py::enum_<OglApp::Components>(cls, "Components")
            .value("Components_RGB", OglApp::Components::COMPONENTS_RGB)
            .value("Components_RGBA", OglApp::Components::COMPONENTS_RGBA)
            .export_values();

}

void bindPyScreenApp(py::module & m)
{
    auto cls = py::class_<ScreenApp,
                          OglAppRcPtr /* holder */,
                          OglApp /* base */>(m, "ScreenApp")
            .def(py::init()
                {
                     OglAppRcPtr p = std::make_shared<OCIO::ScreenApp>(winTitle, winWidth, winHeight);
                     return p;
                 })
            .def("redisplay", &ScreenApp::redisplay)
            .def("printGLInfo", &ScreenApp::printGLInfo);
}

#ifdef OCIO_HEADLESS_ENABLED
void bindPyHeadlessApp(py::module & m)
{
    auto cls = py::class_<HeadlessApp,
                          OglAppRcPtr /* holder */,
                          OglApp /* base */>(m, "HeadlessApp")
            .def(py::init([](const char * winTitle,
                             int winWidth,
                             int winHeight)
                {
                    OglAppRcPtr p = std::make_shared<OCIO::HeadlessApp>(winTitle, winWidth, winHeight);
                    return p;
                }))
            .def("redisplay", &HeadlessApp::redisplay)
            .def("printGLInfo", &HeadlessApp::printGLInfo)
            .def("printEGLInfo", &HeadlessApp::printEGLInfo);
}
#endif

} // namespace OCIO_NAMESPACE
