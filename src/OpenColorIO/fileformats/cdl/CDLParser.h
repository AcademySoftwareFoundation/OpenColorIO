// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_CDL_CDLPARSER_H
#define INCLUDED_OCIO_FILEFORMATS_CDL_CDLPARSER_H

#include <string>
#include <istream>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/CDLTransform.h"

namespace OCIO_NAMESPACE
{

class FormatMetadataImpl;

class CDLParser
{
public:
    explicit CDLParser(const std::string& xmlFile);
    virtual ~CDLParser();

    void parse(std::istream & istream) const;

    // Can be called after parse
    void getCDLTransforms(CDLTransformMap & transformMap,
                          CDLTransformVec & transformVec,
                          FormatMetadataImpl & metadata) const;

    // Can be called after parse
    void getCDLTransform(CDLTransformRcPtr & transform) const;

    bool isCC() const;
    bool isCCC() const;

private:
    // The hidden implementation's declaration
    class Impl;
    Impl * m_impl; 

    CDLParser() = delete;
    CDLParser(const CDLParser&) = delete;
    CDLParser& operator=(const CDLParser&) = delete;
};

} // namespace OCIO_NAMESPACE

#endif
