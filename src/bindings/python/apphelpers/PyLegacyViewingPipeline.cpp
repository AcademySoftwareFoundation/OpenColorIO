// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
void bindPyLegacyViewingPipeline(py::module & m)
{
    auto cls = py::class_<LegacyViewingPipeline,
                          LegacyViewingPipelineRcPtr /* holder */>(m, "LegacyViewingPipeline")
        .def(py::init(&LegacyViewingPipeline::Create))
        .def("getDisplayViewTransform", &LegacyViewingPipeline::getDisplayViewTransform)
        .def("setDisplayViewTransform", &LegacyViewingPipeline::setDisplayViewTransform)
        .def("getLinearCC", &LegacyViewingPipeline::getLinearCC)
        .def("setLinearCC", &LegacyViewingPipeline::setLinearCC)
        .def("getColorTimingCC", &LegacyViewingPipeline::getColorTimingCC)
        .def("setColorTimingCC", &LegacyViewingPipeline::setColorTimingCC)
        .def("getChannelView", &LegacyViewingPipeline::getChannelView)
        .def("setChannelView", &LegacyViewingPipeline::setChannelView)
        .def("getDisplayCC", &LegacyViewingPipeline::getDisplayCC)
        .def("setDisplayCC", &LegacyViewingPipeline::setDisplayCC)
        .def("setLooksOverrideEnabled", &LegacyViewingPipeline::setLooksOverrideEnabled)
        .def("getLooksOverrideEnabled", &LegacyViewingPipeline::getLooksOverrideEnabled)
        .def("setLooksOverride", &LegacyViewingPipeline::setLooksOverride, "looks"_a.none(false))
        .def("getLooksOverride", &LegacyViewingPipeline::getLooksOverride)
        .def("getProcessor", [](LegacyViewingPipelineRcPtr & self,
                                const ConstConfigRcPtr & config,
                                const ConstContextRcPtr & context)
             {
                 ConstContextRcPtr usedContext = context ? context : config->getCurrentContext();
                 return self->getProcessor(config, usedContext);
             },
             "config"_a.none(false),
             "context"_a = ConstContextRcPtr());

    defStr(cls);
}
}