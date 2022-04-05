// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum BakerIterator
{
    IT_FORMAT = 0
};

using FormatIterator = PyIterator<BakerRcPtr, IT_FORMAT>;

} // namespace

void bindPyBaker(py::module & m)
{
    BakerRcPtr DEFAULT = Baker::Create();

    auto clsBaker = 
        py::class_<Baker, BakerRcPtr>(
             m.attr("Baker"));

    auto clsFormatIterator = 
        py::class_<FormatIterator>(
            clsBaker, "FormatIterator",
            R"doc(
Iterator on LUT baker Formats.

Each item is a tuple containing format name and format extension.
)doc");

    clsBaker
        .def(py::init(&Baker::Create), 
             DOC(Baker, Create))
        .def(py::init([](const ConfigRcPtr & config,
                         const std::string & format,
                         const std::string & inputSpace,
                         const std::string & targetSpace,
                         const std::string & looks,
                         int cubeSize,
                         const std::string & shaperSpace,
                         int shaperSize) 
            {
                BakerRcPtr p = Baker::Create();
                p->setConfig(config);
                p->setFormat(format.c_str());
                p->setInputSpace(inputSpace.c_str());
                p->setTargetSpace(targetSpace.c_str());
                p->setCubeSize(cubeSize);
                p->setShaperSize(shaperSize);
                if (!looks.empty())       { p->setLooks(looks.c_str()); }
                if (!shaperSpace.empty()) { p->setShaperSpace(shaperSpace.c_str()); }
                return p;
            }), 
             "config"_a,
             "format"_a,
             "inputSpace"_a,
             "targetSpace"_a,
             "looks"_a = DEFAULT->getLooks(),
             "cubeSize"_a = DEFAULT->getCubeSize(),
             "shaperSpace"_a = DEFAULT->getShaperSpace(),
             "shaperSize"_a = DEFAULT->getShaperSize(),
             DOC(Baker, Create))

        .def("__deepcopy__", [](const ConstBakerRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def_static("getFormats", []()
            { 
                return FormatIterator(nullptr); 
            })

        .def("getConfig", &Baker::getConfig, 
             DOC(Baker, getConfig))
        .def("setConfig", &Baker::setConfig, "config"_a, 
             DOC(Baker, setConfig))
        .def("getFormat", &Baker::getFormat, 
             DOC(Baker, getFormat))
        .def("setFormat", &Baker::setFormat, "formatName"_a, 
             DOC(Baker, setFormat))
        .def("getFormatMetadata", 
             (FormatMetadata & (Baker::*)()) &Baker::getFormatMetadata, 
             py::return_value_policy::reference_internal,
             DOC(Baker, getFormatMetadata))
        .def("getInputSpace", &Baker::getInputSpace, 
             DOC(Baker, getInputSpace))
        .def("setInputSpace", &Baker::setInputSpace, "inputSpace"_a, 
             DOC(Baker, setInputSpace))
        .def("getShaperSpace", &Baker::getShaperSpace, 
             DOC(Baker, getShaperSpace))
        .def("setShaperSpace", &Baker::setShaperSpace, "shaperSpace"_a, 
             DOC(Baker, setShaperSpace))
        .def("getLooks", &Baker::getLooks, 
             DOC(Baker, getLooks))
        .def("setLooks", &Baker::setLooks, "looks"_a, 
             DOC(Baker, setLooks))
        .def("getDisplay", &Baker::getDisplay, 
             DOC(Baker, getDisplay))
        .def("getView", &Baker::getView, 
             DOC(Baker, getView))
        .def("setDisplayView", &Baker::setDisplayView, "display"_a, "view"_a, 
             DOC(Baker, setDisplayView))
        .def("getTargetSpace", &Baker::getTargetSpace, 
             DOC(Baker, getTargetSpace))
        .def("setTargetSpace", &Baker::setTargetSpace, "targetSpace"_a, 
             DOC(Baker, setTargetSpace))
        .def("getShaperSize", &Baker::getShaperSize, 
             DOC(Baker, getShaperSize))
        .def("setShaperSize", &Baker::setShaperSize, "shaperSize"_a, 
             DOC(Baker, setShaperSize))
        .def("getCubeSize", &Baker::getCubeSize, 
             DOC(Baker, getCubeSize))
        .def("setCubeSize", &Baker::setCubeSize, "cubeSize"_a, 
             DOC(Baker, setCubeSize))
        .def("bake", [](BakerRcPtr & self, const std::string & fileName) 
            {
                std::ofstream f(fileName.c_str());
                self->bake(f);
                f.close();
            }, 
             "fileName"_a)
        .def("bake", [](BakerRcPtr & self) 
            {
                std::ostringstream os;
                self->bake(os);
                return os.str();
            },
            DOC(Baker, bake));

    clsFormatIterator
        .def("__len__", [](FormatIterator & /* it */) { return Baker::getNumFormats(); })
        .def("__getitem__", [](FormatIterator & it, int i) 
            { 
                it.checkIndex(i, Baker::getNumFormats());
                return py::make_tuple(Baker::getFormatNameByIndex(i), 
                                      Baker::getFormatExtensionByIndex(i));
            })
        .def("__iter__", [](FormatIterator & it) -> FormatIterator & 
            { 
                return it; 
            })
        .def("__next__", [](FormatIterator & it)
            {
                int i = it.nextIndex(Baker::getNumFormats());
                return py::make_tuple(Baker::getFormatNameByIndex(i), 
                                      Baker::getFormatExtensionByIndex(i));
            });
}

} // namespace OCIO_NAMESPACE
