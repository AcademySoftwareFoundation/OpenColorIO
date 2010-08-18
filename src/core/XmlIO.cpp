/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>

#include "Config.h"
#include "FileTransform.h"
#include "GroupTransform.h"
#include "PathUtils.h"
#include "pystring/pystring.h"
#include "tinyxml/tinyxml.h"

#include <cstring>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        ///////////////////////////////////////////////////////////////////////
        //
        // FileTransform
        
        FileTransformRcPtr CreateFileTransform(const TiXmlElement * element)
        {
            if(!element)
                throw Exception("CreateFileTransform received null XmlElement.");
            
            if(std::string(element->Value()) != "file")
            {
                std::ostringstream os;
                os << "HandleElement passed incorrect element type '";
                os << element->Value() << "'. ";
                os << "Expected 'file'.";
                throw Exception(os.str().c_str());
            }
            
            FileTransformRcPtr t = FileTransform::Create();
            
            const TiXmlAttribute* pAttrib = element->FirstAttribute();
            
            while(pAttrib)
            {
                std::string attrName = pystring::lower(pAttrib->Name());
                
                if(attrName == "src")
                {
                    t->setSrc( pAttrib->Value() );
                }
                else if(attrName == "interpolation")
                {
                    t->setInterpolation( InterpolationFromString(pAttrib->Value()) );
                }
                else if(attrName == "direction")
                {
                    t->setDirection( TransformDirectionFromString(pAttrib->Value()) );
                }
                else
                {
                    // TODO: unknown attr
                }
                pAttrib = pAttrib->Next();
            }
            
            return t;
        }
        
        ConstFileTransformRcPtr GetDefaultFileTransform()
        {
            static ConstFileTransformRcPtr ft = FileTransform::Create();
            return ft;
        }
        
        TiXmlElement * GetElement(const ConstFileTransformRcPtr & t)
        {
            TiXmlElement * element = new TiXmlElement( "file" );
            element->SetAttribute("src", t->getSrc());
            
            ConstFileTransformRcPtr def = GetDefaultFileTransform();
            
            if(t->getInterpolation() != def->getInterpolation())
            {
                const char * interp = InterpolationToString(t->getInterpolation());
                element->SetAttribute("interpolation", interp);
            }
            
            if(t->getDirection() != def->getDirection())
            {
                const char * dir = TransformDirectionToString(t->getDirection());
                element->SetAttribute("direction", dir);
            }
            
            return element;
        }
        
        
        
        ///////////////////////////////////////////////////////////////////////
        //
        // GroupTransform
        
        GroupTransformRcPtr CreateGroupTransform(const TiXmlElement * element)
        {
            if(!element)
                throw Exception("CreateGroupTransform received null XmlElement.");
            
            if(std::string(element->Value()) != "group")
            {
                std::ostringstream os;
                os << "HandleElement passed incorrect element type '";
                os << element->Value() << "'. ";
                os << "Expected 'group'.";
                throw Exception(os.str().c_str());
            }
            
            GroupTransformRcPtr t = GroupTransform::Create();
            
            // TODO: better error handling; Require Attrs Exist
            
            // Read attributes
            {
                const TiXmlAttribute* pAttrib = element->FirstAttribute();
                while(pAttrib)
                {
                    std::string attrName = pystring::lower(pAttrib->Name());
                    if(attrName == "direction") t->setDirection( TransformDirectionFromString(pAttrib->Value()) );
                    else
                    {
                        // TODO: unknown attr
                    }
                    pAttrib = pAttrib->Next();
                }
            }
            
            // Traverse Children
            const TiXmlElement* pElem = element->FirstChildElement();
            while(pElem)
            {
                std::string elementtype = pElem->Value();
                if(elementtype == "group")
                {
                    t->push_back( CreateGroupTransform(pElem) );
                }
                else if(elementtype == "file")
                {
                    t->push_back( CreateFileTransform(pElem) );
                }
                else
                {
                    std::ostringstream os;
                    os << "CreateGroupTransform passed unknown element type '";
                    os << elementtype << "'.";
                    throw Exception(os.str().c_str());
                }
                
                pElem = pElem->NextSiblingElement();
            }
            
            return t;
        }
        
        const ConstGroupTransformRcPtr & GetDefaultGroupTransform()
        {
            static ConstGroupTransformRcPtr gt = GroupTransform::Create();
            return gt;
        }
        
        TiXmlElement * GetElement(const ConstGroupTransformRcPtr& t)
        {
            TiXmlElement * element = new TiXmlElement( "group" );
            
            ConstGroupTransformRcPtr def = GetDefaultGroupTransform();
            
            if(t->getDirection() != def->getDirection())
            {
                const char * dir = TransformDirectionToString(t->getDirection());
                element->SetAttribute("direction", dir);
            }
            
            for(int i=0; i<t->size(); ++i)
            {
                ConstTransformRcPtr child = t->getTransform(i);
                
                if(ConstGroupTransformRcPtr groupTransform = \
                    DynamicPtrCast<const GroupTransform>(child))
                {
                    TiXmlElement * childElement = GetElement(groupTransform);
                    element->LinkEndChild( childElement );
                }
                else if(ConstFileTransformRcPtr fileTransform = \
                    DynamicPtrCast<const FileTransform>(child))
                {
                    TiXmlElement * childElement = GetElement(fileTransform);
                    element->LinkEndChild( childElement );
                }
                else
                {
                    throw Exception("Cannot serialize Transform type to XML");
                }
            }
            
            return element;
        }
        
        
        
        
        ///////////////////////////////////////////////////////////////////////
        //
        // ColorSpace
        
        ColorSpaceRcPtr CreateColorSpaceFromElement(const TiXmlElement * element)
        {
            if(std::string(element->Value()) != "colorspace")
                throw Exception("HandleElement GroupTransform passed incorrect element type.");
            
            ColorSpaceRcPtr cs = ColorSpace::Create();
            
            // TODO: clear colorspace
            // TODO: better error handling; Require Attrs Exist
            
            // Read attributes
            {
                const TiXmlAttribute* pAttrib = element->FirstAttribute();
                double dval = 0.0;
                while(pAttrib)
                {
                    std::string attrName = pystring::lower(pAttrib->Name());
                    if(attrName == "name") cs->setName( pAttrib->Value() );
                    else if(attrName == "family") cs->setFamily( pAttrib->Value() );
                    else if(attrName == "bitdepth") cs->setBitDepth( BitDepthFromString(pAttrib->Value()) );
                    else if(attrName == "isdata") cs->setIsData( BoolFromString(pAttrib->Value()) );
                    else if(attrName == "gpuallocation") cs->setGPUAllocation( GpuAllocationFromString(pAttrib->Value()) );
                    else if(attrName == "gpumin" && pAttrib->QueryDoubleValue(&dval) == TIXML_SUCCESS ) cs->setGPUMin( (float)dval );
                    else if(attrName == "gpumax" && pAttrib->QueryDoubleValue(&dval) == TIXML_SUCCESS ) cs->setGPUMax( (float)dval );
                    else
                    {
                        // TODO: unknown attr
                    }
                    pAttrib = pAttrib->Next();
                }
            }
            
            // Traverse Children
            const TiXmlElement* pElem = element->FirstChildElement();
            while(pElem)
            {
                std::string elementtype = pElem->Value();
                if(elementtype == "description")
                {
                    const char * text = pElem->GetText();
                    if(text) cs->setDescription(text);
                }
                else
                {
                    ColorSpaceDirection dir = ColorSpaceDirectionFromString(elementtype.c_str());
                    
                    if(dir != COLORSPACE_DIR_UNKNOWN)
                    {
                        const TiXmlElement* gchildElem = pElem->FirstChildElement();
                        while(gchildElem)
                        {
                            std::string childelementtype = gchildElem->Value();
                            if(childelementtype == "group")
                            {
                                cs->setTransform(CreateGroupTransform(gchildElem), dir);
                                break;
                            }
                            else
                            {
                                std::ostringstream os;
                                os << "CreateColorSpaceFromElement passed incorrect element type '";
                                os << childelementtype << "'. 'group' expected.";
                                throw Exception(os.str().c_str());
                            }
                            
                            gchildElem=gchildElem->NextSiblingElement();
                        }
                    }
                }
                
                pElem=pElem->NextSiblingElement();
            }
            
            return cs;
        }
        
        TiXmlElement * GetElement(const ConstColorSpaceRcPtr & cs)
        {
            TiXmlElement * element = new TiXmlElement( "colorspace" );
            element->SetAttribute("name", cs->getName());
            element->SetAttribute("family", cs->getFamily());
            element->SetAttribute("bitdepth", BitDepthToString(cs->getBitDepth()));
            element->SetAttribute("isdata", BoolToString(cs->isData()));
            element->SetAttribute("gpuallocation", GpuAllocationToString(cs->getGPUAllocation()));
            element->SetDoubleAttribute("gpumin", cs->getGPUMin());
            element->SetDoubleAttribute("gpumax", cs->getGPUMax());
            
            const char * description = cs->getDescription();
            if(strlen(description) > 0)
            {
                TiXmlElement * descElement = new TiXmlElement( "description" );
                element->LinkEndChild( descElement );
                
                TiXmlText * textElement = new TiXmlText( description );
                descElement->LinkEndChild( textElement );
            }
            
            ColorSpaceDirection dirs[2] = { COLORSPACE_DIR_TO_REFERENCE,
                                            COLORSPACE_DIR_FROM_REFERENCE };
            for(int i=0; i<2; ++i)
            {
                ConstGroupTransformRcPtr group = cs->getTransform(dirs[i]);
                
                if(cs->isTransformSpecified(dirs[i]) && !group->empty())
                {
                    std::string childType = ColorSpaceDirectionToString(dirs[i]);
                    
                    TiXmlElement * childElement = new TiXmlElement( childType );
                    
                    // In the words of Jack Handey...
                    // "I believe in making the world safe for our children, but
                    // not our children's children, because I don't think
                    // children should be having sex."
                    
                    TiXmlElement * childsChild = GetElement( group );
                    childElement->LinkEndChild( childsChild );
                
                    element->LinkEndChild( childElement );
                }
            }
            
            return element;
        }
    
    
    
    }
    
    ///////////////////////////////////////////////////////////////////////
    //
    // Config
    
    void Config::Impl::createFromFile(const char * filename)
    {
        // Assuming the config is already empty...
        
        TiXmlDocument doc(filename);
        
        bool loadOkay = doc.LoadFile();
        if(!loadOkay)
        {
            std::ostringstream os;
            os << "Error parsing ocio configuration file, '" << filename;
            os << "'. " << doc.ErrorDesc();
            if(doc.ErrorRow())
            {
                os << " (line " << doc.ErrorRow();
                os << ", col " << doc.ErrorCol() << ")";
            }
            throw Exception(os.str().c_str());
        }
        
        const TiXmlElement* rootElement = doc.RootElement();
        if(!rootElement || std::string(rootElement->Value()) != "ocioconfig")
        {
            std::ostringstream os;
            os << "Error loading '" << filename;
            os << "'. Please confirm file is 'ocioconfig' format.";
            throw Exception(os.str().c_str());
        }
        
        int version = -1;
        
        int lumaSet = 0;
        float lumacoef[3] = { 0.0f, 0.0f, 0.0f };
        
        // Read attributes
        {
            const TiXmlAttribute* pAttrib=rootElement->FirstAttribute();
            while(pAttrib)
            {
                std::string attrName = pystring::lower(pAttrib->Name());
                if(attrName == "version")
                {
                    if (pAttrib->QueryIntValue(&version)!=TIXML_SUCCESS)
                    {
                        std::ostringstream os;
                        os << "Error parsing ocio configuration file, '" << filename;
                        os << "'. Could not parse integer 'version' tag.";
                        throw Exception(os.str().c_str());
                    }
                }
                else if(attrName == "resourcepath")
                {
                    setResourcePath(pAttrib->Value());
                }
                else if(pystring::startswith(attrName, "role_"))
                {
                    std::string role = pystring::slice(attrName, (int)std::string("role_").size());
                    setColorSpaceForRole(role.c_str(), pAttrib->Value());
                }
                else if(pystring::startswith(attrName, "luma_"))
                {
                    std::string channel = pystring::slice(attrName, (int)std::string("luma_").size());
                    
                    int channelindex = -1;
                    if(channel == "r") channelindex = 0;
                    else if(channel == "g") channelindex = 1;
                    else if(channel == "b") channelindex = 2;
                    
                    if(channelindex<0)
                    {
                        std::ostringstream os;
                        os << "Error parsing ocio configuration file, '" << filename;
                        os << "'. Unknown luma channel '" << channel << "'.";
                        throw Exception(os.str().c_str());
                    }
                    
                    double dval;
                    if(pAttrib->QueryDoubleValue(&dval) != TIXML_SUCCESS )
                    {
                        std::ostringstream os;
                        os << "Error parsing ocio configuration file, '" << filename;
                        os << "'. Bad luma value in channel '" << channelindex << "'.";
                        throw Exception(os.str().c_str());
                    }
                    
                    lumacoef[channelindex] = static_cast<float>(dval);
                    ++lumaSet;
                }
                else
                {
                    // TODO: unknown root attr.
                }
                //if (pAttrib->QueryDoubleValue(&dval)==TIXML_SUCCESS) printf( " d=%1.1f", dval);
                
                pAttrib=pAttrib->Next();
            }
        }
        
        if(version == -1)
        {
            std::ostringstream os;
            os << "Config does not specify a version tag. ";
            os << "Please confirm ocio file is of the expect format.";
            throw Exception(os.str().c_str());
        }
        if(version != 1)
        {
            std::ostringstream os;
            os << "Config is format version '" << version << "',";
            os << " but this library only supports version 1.";
            throw Exception(os.str().c_str());
        }
        
        // Traverse children
        const TiXmlElement* pElem = rootElement->FirstChildElement();
        while(pElem)
        {
            std::string elementtype = pElem->Value();
            if(elementtype == "colorspace")
            {
                ColorSpaceRcPtr cs = CreateColorSpaceFromElement( pElem );
                addColorSpace( cs );
            }
            else if(elementtype == "description")
            {
                setDescription(pElem->GetText());
            }
            else if(elementtype == "display")
            {
                const char * device = pElem->Attribute("device");
                const char * name = pElem->Attribute("name");
                const char * colorspace = pElem->Attribute("colorspace");
                if(!device || !name || !colorspace)
                {
                    std::ostringstream os;
                    os << "Error parsing ocio configuration file, '" << filename;
                    os << "'. Invalid <display> specification.";
                    throw Exception(os.str().c_str());
                }
                
                addDisplayDevice(device, name, colorspace);
            }
            else
            {
                std::cerr << "[OCIO WARNING]: Parse error, ";
                std::cerr << "unknown element type '" << elementtype << "'." << std::endl;
            }
            
            pElem=pElem->NextSiblingElement();
        }
        
        m_originalFileDir = path::dirname(filename);
        m_resolvedResourcePath = path::join(m_originalFileDir, m_resourcePath);
        
        if(lumaSet != 3)
        {
            std::ostringstream os;
            os << "Error parsing ocio configuration file, '" << filename;
            os << "'. Could not find required ocioconfig luma_{r,g,b} xml attributes.";
            throw Exception(os.str().c_str());
        }
        
        setDefaultLumaCoefs(lumacoef);
    }
    
    void Config::Impl::writeXML(std::ostream& os) const
    {
        TiXmlDocument doc;
        
        TiXmlElement * element = new TiXmlElement( "ocioconfig" );
        doc.LinkEndChild( element );
        
        try
        {
            element->SetAttribute("version", "1");
            element->SetAttribute("resourcepath", getResourcePath());
            
            // Luma coefficients
            {
                float coef[3] = { 0.0f, 0.0f, 0.0f };
                getDefaultLumaCoefs(coef);
                
                element->SetDoubleAttribute("luma_r", coef[0]);
                element->SetDoubleAttribute("luma_g", coef[1]);
                element->SetDoubleAttribute("luma_b", coef[2]);
            }
            
            const char * description = getDescription();
            if(strlen(description) > 0)
            {
                TiXmlElement * descElement = new TiXmlElement( "description" );
                element->LinkEndChild( descElement );
                
                TiXmlText * textElement = new TiXmlText( description );
                descElement->LinkEndChild( textElement );
            }
            
            for(int i = 0; i < getNumRoles(); ++i)
            {
                std::string role = getRole(i);
                std::string rolekey = std::string("role_") + role;
                std::string roleValue = getColorSpaceForRole(role.c_str())->getName();
                element->SetAttribute(rolekey, roleValue);
            }
            
            for(int i=0; i<getNumDisplayDeviceNames(); ++i)
            {
                const char * device = getDisplayDeviceName(i);
                
                int numTransforms = getNumDisplayTransformNames(device);
                for(int j=0; j<numTransforms; ++j)
                {
                    const char * displayTransformName = getDisplayTransformName(device, j);
                    const char * colorSpace = getDisplayColorSpaceName(device, displayTransformName);
                    
                    if(device && displayTransformName && colorSpace)
                    {
                        TiXmlElement * childElement = new TiXmlElement( "display" );
                        childElement->SetAttribute("device", device);
                        childElement->SetAttribute("name", displayTransformName);
                        childElement->SetAttribute("colorspace", colorSpace);
                        element->LinkEndChild( childElement );
                    }
                }
            }
            
            for(int i = 0; i < getNumColorSpaces(); ++i)
            {
                TiXmlElement * childElement = GetElement(getColorSpaceByIndex(i));
                element->LinkEndChild( childElement );
            }
            
            TiXmlPrinter printer;
            printer.SetIndent("    ");
            doc.Accept( &printer );
            os << printer.Str();
        }
        catch( const std::exception & e)
        {
            std::ostringstream error;
            error << "Error writing xml. ";
            error << e.what();
            throw Exception(error.str().c_str());
        }
    }
}
OCIO_NAMESPACE_EXIT
