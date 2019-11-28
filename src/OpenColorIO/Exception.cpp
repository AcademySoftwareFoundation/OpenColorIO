// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

Exception::Exception(const char * msg)
    :   std::runtime_error(msg)
{
}

Exception::Exception(const Exception & e)
    :   std::runtime_error(e)
{
}

Exception::~Exception()
{
}


ExceptionMissingFile::ExceptionMissingFile(const char * msg)
    :   Exception(msg)
{
}

ExceptionMissingFile::ExceptionMissingFile(const ExceptionMissingFile & e)
    :   Exception(e)
{
}

ExceptionMissingFile::~ExceptionMissingFile()
{
}

} // namespace OCIO_NAMESPACE

