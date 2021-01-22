// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

void bindPyDisplayViewHelpers(py::module & m)
{
    auto mDisplayViewHelpers = m.def_submodule("DisplayViewHelpers")
        .def("GetProcessor", [](const ConstConfigRcPtr & config,
                                const ConstContextRcPtr & context,
                                const char * workingName,
                                const char * displayName,
                                const char * viewName,
                                const ConstMatrixTransformRcPtr & channelView,
                                TransformDirection direction)
            {
                ConstContextRcPtr usedContext = context ? context : config->getCurrentContext();
                return DisplayViewHelpers::GetProcessor(config, 
                                                        usedContext, 
                                                        workingName, 
                                                        displayName, 
                                                        viewName, 
                                                        channelView, 
                                                        direction);
            },
             "config"_a.none(false),
             "context"_a = ConstContextRcPtr(),
             "workingSpaceName"_a.none(false),
             "displayName"_a.none(false),
             "viewName"_a.none(false),
             "channelView"_a = ConstMatrixTransformRcPtr(),
             "direction"_a = TRANSFORM_DIR_FORWARD,
             DOC(DisplayViewHelpers, GetProcessor))
        .def("GetIdentityProcessor", &DisplayViewHelpers::GetIdentityProcessor,
             "config"_a.none(false), 
             DOC(DisplayViewHelpers, GetIdentityProcessor))
        .def("AddDisplayView", [](ConfigRcPtr & config,
                                const std::string & displayName,
                                const std::string & viewName,
                                const std::string & lookName,              // Could be empty
                                const std::string & colorSpaceName,        // Could be empty
                                const std::string & colorSpaceFamily,      // Could be empty
                                const std::string & colorSpaceDescription, // Could be empty
                                const std::string & categories,            // Could be empty
                                const std::string & transformFilePath,
                                const std::string & connectionColorSpaceName)
            {
                DisplayViewHelpers::AddDisplayView(config, 
                                                   displayName.c_str(), 
                                                   viewName.c_str(),
                                                   lookName.c_str(), 
                                                   colorSpaceName.c_str(),
                                                   colorSpaceFamily.c_str(),
                                                   colorSpaceDescription.c_str(), 
                                                   categories.c_str(),
                                                   transformFilePath.c_str(),
                                                   connectionColorSpaceName.c_str());
            },
             "config"_a.none(false),
             "displayName"_a,
             "viewName"_a,
             "lookName"_a = std::string(""),
             "colorSpaceName"_a = std::string(""),
             "colorSpaceFamily"_a = std::string(""),
             "colorSpaceDescription"_a = std::string(""),
             "colorSpaceCategories"_a = std::string(""),
             "transformFilePath"_a,
             "connectionColorSpaceName"_a,
             DOC(DisplayViewHelpers, AddDisplayView))
        .def("RemoveDisplayView", &DisplayViewHelpers::RemoveDisplayView,
             "config"_a.none(false), "displayName"_a.none(false), "viewName"_a.none(false),
             DOC(DisplayViewHelpers, RemoveDisplayView));
}

} // namespace OCIO_NAMESPACE
