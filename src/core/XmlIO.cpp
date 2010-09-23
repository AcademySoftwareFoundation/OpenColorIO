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

#include "MathUtils.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"
#include "tinyxml/tinyxml.h"
#include "XmlIO.h"

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
            static ConstFileTransformRcPtr filetransform_ = FileTransform::Create();
            return filetransform_;
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
        // MatrixTransform
        
        MatrixTransformRcPtr CreateMatrixTransform(const TiXmlElement * element)
        {
            if(!element)
                throw Exception("CreateMatrixTransform received null XmlElement.");
            
            if(std::string(element->Value()) != "matrix")
            {
                std::ostringstream os;
                os << "HandleElement passed incorrect element type '";
                os << element->Value() << "'. ";
                os << "Expected 'matrix'.";
                throw Exception(os.str().c_str());
            }
            
            MatrixTransformRcPtr t = MatrixTransform::Create();
            float matrix[16];
            float offset[4];
            t->getValue(matrix, offset);
            
            const TiXmlAttribute* pAttrib = element->FirstAttribute();
            
            
            while(pAttrib)
            {
                std::string attrName = pystring::lower(pAttrib->Name());
                
                if(pystring::startswith(attrName, "m_"))
                {
                    std::string strval = pystring::slice(attrName,
                        (int)std::string("m_").size());
                    int mindex = 0;
                    if(!StringToInt(&mindex, strval.c_str()) || mindex > 15 || mindex < 0)
                    {
                        std::ostringstream os;
                        os << "Matrix has invalid index specified, '";
                        os << attrName << "'. ";
                        os << "Expected m_{0-15}.";
                        throw Exception(os.str().c_str());
                    }
                    
                    float value = 0.0;
                    if(!StringToFloat(&value, pAttrib->Value()))
                    {
                        std::ostringstream os;
                        os << "Matrix has invalid value specified, '";
                        os << pAttrib->Value() << "'. ";
                        os << "Expected float.";
                        throw Exception(os.str().c_str());
                    }
                    
                    matrix[mindex] = value;
                }
                
                if(pystring::startswith(attrName, "b_"))
                {
                    std::string strval = pystring::slice(attrName,
                        (int)std::string("b_").size());
                    int bindex = 0;
                    if(!StringToInt(&bindex, strval.c_str()) || bindex > 3 || bindex < 0)
                    {
                        std::ostringstream os;
                        os << "Matrix offset has invalid index specified, '";
                        os << attrName << "'. ";
                        os << "Expected b_{0-3}.";
                        throw Exception(os.str().c_str());
                    }
                    
                    float value = 0.0;
                    if(!StringToFloat(&value, pAttrib->Value()))
                    {
                        std::ostringstream os;
                        os << "Matrix has invalid value specified, '";
                        os << pAttrib->Value() << "'. ";
                        os << "Expected float.";
                        throw Exception(os.str().c_str());
                    }
                    
                    offset[bindex] = value;
                }
                
                pAttrib = pAttrib->Next();
            }
            
            t->setValue(matrix, offset);
            
            return t;
        }
        
        TiXmlElement * GetElement(const ConstMatrixTransformRcPtr & t)
        {
            TiXmlElement * element = new TiXmlElement( "matrix" );
            
            float matrix[16];
            float offset[4];
            t->getValue(matrix, offset);
            
            // Get the defaults
            MatrixTransformRcPtr dMtx = MatrixTransform::Create();
            float dmatrix[16];
            float doffset[4];
            dMtx->getValue(dmatrix, doffset);
            
            const float abserror = 1e-9f;
            
            for(unsigned int i=0; i<16; ++i)
            {
                if(equalWithAbsError(matrix[i], dmatrix[i], abserror))
                    continue;
                
                std::ostringstream attrname;
                attrname << "m_" << i;
                element->SetDoubleAttribute(attrname.str(), matrix[i]);
            }
            
            for(unsigned int i=0; i<4; ++i)
            {
                if(equalWithAbsError(offset[i], doffset[i], abserror))
                    continue;
                
                std::ostringstream attrname;
                attrname << "b_" << i;
                element->SetDoubleAttribute(attrname.str(), offset[i]);
            }
            
            return element;
        }
        
        
        
        
        
        ///////////////////////////////////////////////////////////////////////
        //
        // ColorSpaceTransform
        
        ColorSpaceTransformRcPtr CreateColorSpaceTransform(const TiXmlElement * element)
        {
            if(!element)
                throw Exception("ColorSpaceTransform received null XmlElement.");
            
            if(std::string(element->Value()) != "colorspacetransform")
            {
                std::ostringstream os;
                os << "HandleElement passed incorrect element type '";
                os << element->Value() << "'. ";
                os << "Expected 'colorspacetransform'.";
                throw Exception(os.str().c_str());
            }
            
            ColorSpaceTransformRcPtr t = ColorSpaceTransform::Create();
            
            const char * src = element->Attribute("src");
            const char * dst = element->Attribute("dst");
            
            if(!src || !dst)
            {
                std::ostringstream os;
                os << "ColorSpaceTransform must specify both src and dst ";
                os << "ColorSpaces.";
                throw Exception(os.str().c_str());
            }
            
            t->setSrc(src);
            t->setDst(dst);
            
            const char * direction = element->Attribute("direction");
            if(direction)
            {
                t->setDirection( TransformDirectionFromString(direction) );
            }
            
            return t;
        }
        
        
        
        TiXmlElement * GetElement(const ConstColorSpaceTransformRcPtr & t)
        {
            TiXmlElement * element = new TiXmlElement( "colorspacetransform" );
            
            element->SetAttribute("src", t->getSrc());
            element->SetAttribute("dst", t->getDst());
            
            if(t->getDirection() != TRANSFORM_DIR_FORWARD)
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
                else if(elementtype == "colorspacetransform")
                {
                    t->push_back( CreateColorSpaceTransform(pElem) );
                }
                else if(elementtype == "matrix")
                {
                    t->push_back( CreateMatrixTransform(pElem) );
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
                else if(ConstColorSpaceTransformRcPtr colorSpaceTransform = \
                    DynamicPtrCast<const ColorSpaceTransform>(child))
                {
                    TiXmlElement * childElement = GetElement(colorSpaceTransform);
                    element->LinkEndChild( childElement );
                }
                else if(ConstMatrixTransformRcPtr matrixTransform = \
                    DynamicPtrCast<const MatrixTransform>(child))
                {
                    TiXmlElement * childElement = GetElement(matrixTransform);
                    element->LinkEndChild( childElement );
                }
                else
                {
                    throw Exception("Cannot serialize Transform type to XML");
                }
            }
            
            return element;
        }
    }
    
    
    
    
    ///////////////////////////////////////////////////////////////////////
    //
    // ColorSpace
    
    TiXmlElement * GetColorSpaceElement(const ConstColorSpaceRcPtr & cs)
    {
        TiXmlElement * element = new TiXmlElement( "colorspace" );
        element->SetAttribute("name", cs->getName());
        element->SetAttribute("family", cs->getFamily());
        element->SetAttribute("bitdepth", BitDepthToString(cs->getBitDepth()));
        element->SetAttribute("isdata", BoolToString(cs->isData()));
        element->SetAttribute("gpuallocation", GpuAllocationToString(cs->getGpuAllocation()));
        element->SetDoubleAttribute("gpumin", cs->getGpuMin());
        element->SetDoubleAttribute("gpumax", cs->getGpuMax());
        
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

    ColorSpaceRcPtr CreateColorSpaceFromElement(const TiXmlElement * element)
    {
        if(std::string(element->Value()) != "colorspace")
            throw Exception("HandleElement GroupTransform passed incorrect element type.");
        
        ColorSpaceRcPtr cs = ColorSpace::Create();
        
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
                else if(attrName == "gpuallocation") cs->setGpuAllocation( GpuAllocationFromString(pAttrib->Value()) );
                else if(attrName == "gpumin" && pAttrib->QueryDoubleValue(&dval) == TIXML_SUCCESS ) cs->setGpuMin( (float)dval );
                else if(attrName == "gpumax" && pAttrib->QueryDoubleValue(&dval) == TIXML_SUCCESS ) cs->setGpuMax( (float)dval );
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
    
}
OCIO_NAMESPACE_EXIT
