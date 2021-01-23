// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum MixingColorSpaceManagerIterator
{
    IT_MIXING_SPACE = 0,
    IT_MIXING_ENCODING
};

using MixingSpaceIterator = PyIterator<MixingColorSpaceManagerRcPtr, IT_MIXING_SPACE>;
using MixingEncodingIterator = PyIterator<MixingColorSpaceManagerRcPtr, IT_MIXING_ENCODING>;

} // namespace

void bindPyMixingHelpers(py::module & m)
{
     auto clsMixingSlider = 
          py::class_<MixingSlider>(
              m.attr("MixingSlider"));

     auto clsMixingColorSpaceManager =
          py::class_<MixingColorSpaceManager, MixingColorSpaceManagerRcPtr>(
              m.attr("MixingColorSpaceManager"));

     auto clsMixingSpaceIterator = 
          py::class_<MixingSpaceIterator>(
               clsMixingColorSpaceManager, "MixingSpaceIterator");

     auto clsMixingEncodingIterator = 
          py::class_<MixingEncodingIterator>(
               clsMixingColorSpaceManager, "MixingEncodingIterator");

    clsMixingSlider
        .def("setSliderMinEdge", &MixingSlider::setSliderMinEdge,
             DOC(MixingSlider, setSliderMinEdge))
        .def("getSliderMinEdge", &MixingSlider::getSliderMinEdge,
             DOC(MixingSlider, getSliderMinEdge))
        .def("setSliderMaxEdge", &MixingSlider::setSliderMaxEdge,
             DOC(MixingSlider, setSliderMaxEdge))
        .def("getSliderMaxEdge", &MixingSlider::getSliderMaxEdge,
             DOC(MixingSlider, getSliderMaxEdge))
        .def("sliderToMixing", &MixingSlider::sliderToMixing, "sliderUnits"_a,
             DOC(MixingSlider, sliderToMixing))
        .def("mixingToSlider", &MixingSlider::mixingToSlider, "mixingUnits"_a,
             DOC(MixingSlider, mixingToSlider));

    defRepr(clsMixingSlider);

    clsMixingColorSpaceManager
        .def(py::init([](ConstConfigRcPtr & config)
             {
                 return MixingColorSpaceManager::Create(config);
             }), 
             "config"_a.none(false),
             DOC(MixingColorSpaceManager, Create))
        .def("getMixingSpaces", [](MixingColorSpaceManagerRcPtr & self)
             {
                 return MixingSpaceIterator(self);
             })
        .def("setSelectedMixingSpaceIdx", &MixingColorSpaceManager::setSelectedMixingSpaceIdx,
             DOC(MixingColorSpaceManager, setSelectedMixingSpaceIdx))
        .def("setSelectedMixingSpace", &MixingColorSpaceManager::setSelectedMixingSpace,
             "mixingSpace"_a.none(false),
             DOC(MixingColorSpaceManager, setSelectedMixingSpace))
        .def("getSelectedMixingSpaceIdx", &MixingColorSpaceManager::getSelectedMixingSpaceIdx,
             DOC(MixingColorSpaceManager, getSelectedMixingSpaceIdx))
        .def("isPerceptuallyUniform", &MixingColorSpaceManager::isPerceptuallyUniform,
             DOC(MixingColorSpaceManager, isPerceptuallyUniform))
        .def("getMixingEncodings", [](MixingColorSpaceManagerRcPtr & self)
             {
                 return MixingEncodingIterator(self);
             })
        .def("setSelectedMixingEncodingIdx", &MixingColorSpaceManager::setSelectedMixingEncodingIdx,
             DOC(MixingColorSpaceManager, setSelectedMixingEncodingIdx))
        .def("setSelectedMixingEncoding", &MixingColorSpaceManager::setSelectedMixingEncoding,
             "mixingEncoding"_a.none(false),
             DOC(MixingColorSpaceManager, setSelectedMixingEncoding))
        .def("getSelectedMixingEncodingIdx", &MixingColorSpaceManager::getSelectedMixingEncodingIdx,
             DOC(MixingColorSpaceManager, getSelectedMixingEncodingIdx))
        .def("refresh", &MixingColorSpaceManager::refresh, "config"_a.none(false),
             DOC(MixingColorSpaceManager, refresh))
        .def("getProcessor", &MixingColorSpaceManager::getProcessor,
             "workingSpaceName"_a.none(false),
             "displayName"_a.none(false),
             "viewName"_a.none(false),
             "direction"_a = TRANSFORM_DIR_FORWARD,
             DOC(MixingColorSpaceManager, getProcessor))
        .def("getSlider",
             (MixingSlider & (MixingColorSpaceManager::*)()) &MixingColorSpaceManager::getSlider,
             py::return_value_policy::reference_internal,
             DOC(MixingColorSpaceManager, getSlider))
        .def("getSlider",
             (MixingSlider & (MixingColorSpaceManager::*)(float sliderMixingMinEdge,
                                                          float sliderMixingMaxEdge))
                &MixingColorSpaceManager::getSlider,
             "sliderMixingMinEdge"_a.none(false), "sliderMixingMaxEdge"_a.none(false),
             py::return_value_policy::reference_internal,
             DOC(MixingColorSpaceManager, getSlider));

    defRepr(clsMixingColorSpaceManager);

    clsMixingSpaceIterator
        .def("__len__", [](MixingSpaceIterator & it)
             {
                 return static_cast<int>(it.m_obj->getNumMixingSpaces());
             })
        .def("__getitem__", [](MixingSpaceIterator & it, int i)
             {
                 it.checkIndex(i, static_cast<int>(it.m_obj->getNumMixingSpaces()));
                 return it.m_obj->getMixingSpaceUIName(i);
             })
        .def("__iter__", [](MixingSpaceIterator & it) -> MixingSpaceIterator &
             {
                 return it;
             })
        .def("__next__", [](MixingSpaceIterator & it)
             {
                 int i = it.nextIndex(static_cast<int>(it.m_obj->getNumMixingSpaces()));
                 return it.m_obj->getMixingSpaceUIName(i);
             });

    clsMixingEncodingIterator
        .def("__len__", [](MixingEncodingIterator & it)
             {
                 return static_cast<int>(it.m_obj->getNumMixingEncodings());
             })
        .def("__getitem__", [](MixingEncodingIterator & it, int i)
             {
                 it.checkIndex(i, static_cast<int>(it.m_obj->getNumMixingEncodings()));
                 return it.m_obj->getMixingEncodingName(i);
             })
        .def("__iter__", [](MixingEncodingIterator & it) -> MixingEncodingIterator &
             {
                 return it;
             })
        .def("__next__", [](MixingSpaceIterator & it)
             {
                 int i = it.nextIndex(static_cast<int>(it.m_obj->getNumMixingEncodings()));
                 return it.m_obj->getMixingEncodingName(i);
             });
}

} // namespace OCIO_NAMESPACE
