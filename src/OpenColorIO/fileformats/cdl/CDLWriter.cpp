// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "fileformats/cdl/CDLWriter.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "fileformats/xmlutils/XMLWriterUtils.h"
#include "ParseUtils.h"
#include "transforms/CDLTransform.h"
#include "transforms/FileTransform.h"

namespace OCIO_NAMESPACE
{

void WriteStrings(XmlFormatter & fmt, const char * tag, const StringUtils::StringVec & strings)
{
    for (const auto & it : strings)
    {
        fmt.writeContentTag(tag, it);
    }
}

void ExtractCDLMetadata(const FormatMetadata & metadata,
                        StringUtils::StringVec & mainDesc,
                        StringUtils::StringVec & inputDesc,
                        StringUtils::StringVec & viewingDesc,
                        StringUtils::StringVec & sopDesc,
                        StringUtils::StringVec & satDesc)
{
    const int nbElt = metadata.getNumChildrenElements();
    for (int i = 0; i < nbElt; ++i)
    {
        const auto & elt = metadata.getChildElement(i);
        if (0 == Platform::Strcasecmp(elt.getElementName(), METADATA_DESCRIPTION))
        {
            mainDesc.push_back(ConvertSpecialCharToXmlToken(elt.getElementValue()));
        }
        else if (0 == Platform::Strcasecmp(elt.getElementName(), METADATA_INPUT_DESCRIPTION))
        {
            inputDesc.push_back(ConvertSpecialCharToXmlToken(elt.getElementValue()));
        }
        else if (0 == Platform::Strcasecmp(elt.getElementName(), METADATA_VIEWING_DESCRIPTION))
        {
            viewingDesc.push_back(ConvertSpecialCharToXmlToken(elt.getElementValue()));
        }
        else if (0 == Platform::Strcasecmp(elt.getElementName(), METADATA_SOP_DESCRIPTION))
        {
            sopDesc.push_back(ConvertSpecialCharToXmlToken(elt.getElementValue()));
        }
        else if (0 == Platform::Strcasecmp(elt.getElementName(), METADATA_SAT_DESCRIPTION))
        {
            satDesc.push_back(ConvertSpecialCharToXmlToken(elt.getElementValue()));
        }
    }
}

void Write(XmlFormatter & fmt, const ConstCDLTransformRcPtr & cdl)
{
    const auto & metadata = cdl->getFormatMetadata();

    XmlFormatter::Attributes attributes;
    const char * id = metadata.getAttributeValue(METADATA_ID);

    if (id && *id)
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_ID, std::string(id)));
    }

    const char * name = metadata.getName();
    if (name && *name)
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_NAME, std::string(name)));
    }

    fmt.writeStartTag(CDL_TAG_COLOR_CORRECTION, attributes);
    {
        XmlScopeIndent scopeIndent(fmt);

        StringUtils::StringVec mainDesc;
        StringUtils::StringVec inputDesc;
        StringUtils::StringVec viewingDesc;
        StringUtils::StringVec sopDesc;
        StringUtils::StringVec satDesc;
        ExtractCDLMetadata(metadata, mainDesc, inputDesc, viewingDesc, sopDesc, satDesc);
        WriteStrings(fmt, TAG_DESCRIPTION, mainDesc);
        WriteStrings(fmt, METADATA_INPUT_DESCRIPTION, inputDesc);
        WriteStrings(fmt, METADATA_VIEWING_DESCRIPTION, viewingDesc);

        fmt.writeStartTag(TAG_SOPNODE);
        {
            XmlScopeIndent scopeIndent(fmt);
            WriteStrings(fmt, TAG_DESCRIPTION, sopDesc);
            double rgb[3]{ 0. };
            cdl->getSlope(rgb);
            fmt.writeContentTag(TAG_SLOPE, DoubleVecToString(rgb, 3));
            cdl->getOffset(rgb);
            fmt.writeContentTag(TAG_OFFSET, DoubleVecToString(rgb, 3));
            cdl->getPower(rgb);
            fmt.writeContentTag(TAG_POWER, DoubleVecToString(rgb, 3));
        }
        fmt.writeEndTag(TAG_SOPNODE);
        fmt.writeStartTag(TAG_SATNODE);
        {
            XmlScopeIndent scopeIndent(fmt);
            WriteStrings(fmt, TAG_DESCRIPTION, satDesc);
            fmt.writeContentTag(TAG_SATURATION, DoubleToString(cdl->getSat()));
        }
        fmt.writeEndTag(TAG_SATNODE);
    }
    fmt.writeEndTag(CDL_TAG_COLOR_CORRECTION);
}

} // namespace OCIO_NAMESPACE
