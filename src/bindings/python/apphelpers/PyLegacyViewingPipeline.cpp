// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyLegacyViewingPipeline(py::module & m)
{
    auto clsLegacyViewingPipeline =
        py::class_<LegacyViewingPipeline, LegacyViewingPipelineRcPtr>(
            m.attr("LegacyViewingPipeline"))

        .def(py::init(&LegacyViewingPipeline::Create),
             DOC(LegacyViewingPipeline, Create))
        .def("getDisplayViewTransform", &LegacyViewingPipeline::getDisplayViewTransform,
             DOC(LegacyViewingPipeline, getDisplayViewTransform))
        .def("setDisplayViewTransform", &LegacyViewingPipeline::setDisplayViewTransform,
             DOC(LegacyViewingPipeline, setDisplayViewTransform))
        .def("getLinearCC", &LegacyViewingPipeline::getLinearCC,
             DOC(LegacyViewingPipeline, getLinearCC))
        .def("setLinearCC", &LegacyViewingPipeline::setLinearCC,
             DOC(LegacyViewingPipeline, setLinearCC))
        .def("getColorTimingCC", &LegacyViewingPipeline::getColorTimingCC,
             DOC(LegacyViewingPipeline, getColorTimingCC))
        .def("setColorTimingCC", &LegacyViewingPipeline::setColorTimingCC,
             DOC(LegacyViewingPipeline, setColorTimingCC))
        .def("getChannelView", &LegacyViewingPipeline::getChannelView,
             DOC(LegacyViewingPipeline, getChannelView))
        .def("setChannelView", &LegacyViewingPipeline::setChannelView,
             DOC(LegacyViewingPipeline, setChannelView))
        .def("getDisplayCC", &LegacyViewingPipeline::getDisplayCC,
             DOC(LegacyViewingPipeline, getDisplayCC))
        .def("setDisplayCC", &LegacyViewingPipeline::setDisplayCC,
             DOC(LegacyViewingPipeline, setDisplayCC))
        .def("setLooksOverrideEnabled", &LegacyViewingPipeline::setLooksOverrideEnabled,
             DOC(LegacyViewingPipeline, setLooksOverrideEnabled))
        .def("getLooksOverrideEnabled", &LegacyViewingPipeline::getLooksOverrideEnabled,
             DOC(LegacyViewingPipeline, getLooksOverrideEnabled))
        .def("setLooksOverride", &LegacyViewingPipeline::setLooksOverride, "looks"_a.none(false),
             DOC(LegacyViewingPipeline, setLooksOverride))
        .def("getLooksOverride", &LegacyViewingPipeline::getLooksOverride,
             DOC(LegacyViewingPipeline, getLooksOverride))
        .def("getProcessor", [](LegacyViewingPipelineRcPtr & self,
                                const ConstConfigRcPtr & config,
                                const ConstContextRcPtr & context)
            {
                ConstContextRcPtr usedContext = context ? context : config->getCurrentContext();
                return self->getProcessor(config, usedContext);
            },
             "config"_a.none(false),
             "context"_a = ConstContextRcPtr(),
             DOC(LegacyViewingPipeline, getProcessor));

    defRepr(clsLegacyViewingPipeline);
}

} // namespace OCIO_NAMESPACE
