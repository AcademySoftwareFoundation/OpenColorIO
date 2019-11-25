// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#ifndef INCLUDED_OCIO_YAML_H
#define INCLUDED_OCIO_YAML_H

OCIO_NAMESPACE_ENTER
{
    
    namespace OCIOYaml
    {
        void read(std::istream & istream, ConfigRcPtr & c, const char * filename);
        void write(std::ostream & ostream, const Config * c);
    };
    
}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_YAML_H
