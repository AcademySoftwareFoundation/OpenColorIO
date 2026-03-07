// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "graphicalapp.h"
#include "oglapp.h"

namespace OCIO_NAMESPACE
{

GraphicalAppRcPtr GraphicalApp::CreateApp(const char * winTitle, int winWidth, int winHeight)
{
    return OglApp::CreateApp(winTitle, winWidth, winHeight);
}

} // namespace OCIO_NAMESPACE
