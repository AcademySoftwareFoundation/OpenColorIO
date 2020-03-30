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

    auto cls = py::class_<Baker, BakerRcPtr /* holder */>(m, "Baker")
        .def(py::init(&Baker::Create))
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
             "shaperSize"_a = DEFAULT->getShaperSize())

        .def_static("getFormats", []() { return FormatIterator(nullptr); })

        .def("getConfig", &Baker::getConfig)
        .def("setConfig", &Baker::setConfig, "config"_a)
        .def("getFormat", &Baker::getFormat)
        .def("setFormat", &Baker::setFormat, "formatName"_a)
        .def("getFormatMetadata", 
             (const FormatMetadata & (Baker::*)() const) &Baker::getFormatMetadata, 
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (FormatMetadata & (Baker::*)()) &Baker::getFormatMetadata, 
             py::return_value_policy::reference_internal)
        .def("getInputSpace", &Baker::getInputSpace)
        .def("setInputSpace", &Baker::setInputSpace, "inputSpace"_a)
        .def("getShaperSpace", &Baker::getShaperSpace)
        .def("setShaperSpace", &Baker::setShaperSpace, "shaperSpace"_a)
        .def("getLooks", &Baker::getLooks)
        .def("setLooks", &Baker::setLooks, "looks"_a)
        .def("getTargetSpace", &Baker::getTargetSpace)
        .def("setTargetSpace", &Baker::setTargetSpace, "targetSpace"_a)
        .def("getShaperSize", &Baker::getShaperSize)
        .def("setShaperSize", &Baker::setShaperSize, "shapersize"_a)
        .def("getCubeSize", &Baker::getCubeSize)
        .def("setCubeSize", &Baker::setCubeSize, "cubesize"_a)
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
            });

    py::class_<FormatIterator>(cls, "FormatIterator")
        .def("__len__", [](FormatIterator & it) { return Baker::getNumFormats(); })
        .def("__getitem__", [](FormatIterator & it, int i) 
            { 
                it.checkIndex(i, Baker::getNumFormats());
                return py::make_tuple(Baker::getFormatNameByIndex(i), 
                                      Baker::getFormatExtensionByIndex(i));
            })
        .def("__iter__", [](FormatIterator & it) -> FormatIterator & { return it; })
        .def("__next__", [](FormatIterator & it)
            {
                int i = it.nextIndex(Baker::getNumFormats());
                return py::make_tuple(Baker::getFormatNameByIndex(i), 
                                      Baker::getFormatExtensionByIndex(i));
            });
}

} // namespace OCIO_NAMESPACE
