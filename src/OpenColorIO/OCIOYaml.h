// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#ifndef INCLUDED_OCIO_YAML_H
#define INCLUDED_OCIO_YAML_H

namespace OCIO_NAMESPACE
{

namespace OCIOYaml
{

void Read(std::istream & istream, ConfigRcPtr & c, const char * filename);
void Write(std::ostream & ostream, const Config & c);

} // namespace OCIOYaml

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_YAML_H
