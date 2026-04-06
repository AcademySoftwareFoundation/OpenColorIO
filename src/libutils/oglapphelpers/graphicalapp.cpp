// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "graphicalapp.h"

#ifdef OCIO_GL_ENABLED
#include "oglapp.h"
#endif

#ifdef OCIO_DIRECTX_ENABLED
#include "dxapp.h"
#endif

namespace OCIO_NAMESPACE
{

GraphicalAppRcPtr GraphicalApp::CreateApp(const char * winTitle, int winWidth, int winHeight)
{
#ifdef OCIO_GL_ENABLED
    return OglApp::CreateApp(winTitle, winWidth, winHeight);
#elif defined(OCIO_DIRECTX_ENABLED)
    return std::make_shared<DxApp>(winTitle, winWidth, winHeight);
#else
    (void)winTitle; (void)winWidth; (void)winHeight;
    throw Exception("No suitable GPU backend available for GraphicalApp::CreateApp");
#endif
}

} // namespace OCIO_NAMESPACE
