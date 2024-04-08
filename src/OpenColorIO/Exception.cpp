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

ExceptionAddColorspace::ExceptionAddColorspace(const char * msg, AddColorspaceError errorCode)
    :   Exception(msg), m_errorCode(errorCode)
{
}

ExceptionAddColorspace::ExceptionAddColorspace(const ExceptionAddColorspace & e)
    :   Exception(e)
{
}

AddColorspaceError ExceptionAddColorspace::getErrorCode() const noexcept
{
    return m_errorCode;
}

ExceptionAddColorspace::~ExceptionAddColorspace()
{
}

ExceptionAddNamedTransform::ExceptionAddNamedTransform(const char * msg, AddNamedTransformError errorCode)
    :   Exception(msg), m_errorCode(errorCode)
{
}

ExceptionAddNamedTransform::ExceptionAddNamedTransform(const ExceptionAddNamedTransform & e)
    :   Exception(e)
{
}

AddNamedTransformError ExceptionAddNamedTransform::getErrorCode() const noexcept
{
    return m_errorCode;
}

ExceptionAddNamedTransform::~ExceptionAddNamedTransform()
{
}

} // namespace OCIO_NAMESPACE

