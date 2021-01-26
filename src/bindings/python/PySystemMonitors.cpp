// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PySystemMonitors.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace 
{

enum SystemMonitorsIterator
{
    IT_MONITORS = 0
};

using MonitorIterator = PyIterator<PySystemMonitors, IT_MONITORS>;

} // namespace

void bindPySystemMonitors(py::module & m)
{
    auto clsSystemMonitors = 
        py::class_<PySystemMonitors>(
            m.attr("SystemMonitors"));

    auto clsMonitorIterator = 
        py::class_<MonitorIterator>(
            clsSystemMonitors, "MonitorIterator");

    clsSystemMonitors
        .def(py::init<>(),
             DOC(SystemMonitors, SystemMonitors))

        .def("getMonitors", [](PySystemMonitors & self) 
            {
                return MonitorIterator(self);
            });

    clsMonitorIterator
        .def("__len__", [](MonitorIterator & it) { return it.m_obj.getNumMonitors(); })
        .def("__getitem__", [](MonitorIterator & it, int i) 
            { 
                // SystemMonitors provides index check with exception.
                return py::make_tuple(it.m_obj.getMonitorName(i), 
                                      it.m_obj.getProfileFilepath(i));
            })
        .def("__iter__", [](MonitorIterator & it) -> MonitorIterator & { return it; })
        .def("__next__", [](MonitorIterator & it)
            {
                const int i = it.nextIndex((int)it.m_obj.getNumMonitors());
                return py::make_tuple(it.m_obj.getMonitorName(i), 
                                      it.m_obj.getProfileFilepath(i));
            });
}

} // namespace OCIO_NAMESPACE
