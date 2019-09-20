// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#ifndef INCLUDED_OCIO_YAML_H
#define INCLUDED_OCIO_YAML_H

OCIO_NAMESPACE_ENTER
{
    
    class OCIOYaml
    {
    public:
        void open(std::istream& istream, ConfigRcPtr& c, const char* filename = NULL) const;
        void write(std::ostream& ostream, const Config* c) const;
    };
    
}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_YAML_H
