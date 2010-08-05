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

#include "CDLTransform.h"

#include "MatrixOps.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    CDLTransformRcPtr CDLTransform::Create()
    {
        return CDLTransformRcPtr(new CDLTransform(), &deleter);
    }
    
    void CDLTransform::deleter(CDLTransform* t)
    {
        delete t;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    CDLTransform::CDLTransform()
        : m_impl(new CDLTransform::Impl)
    {
    }
    
    TransformRcPtr CDLTransform::createEditableCopy() const
    {
        CDLTransformRcPtr transform = CDLTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    CDLTransform::~CDLTransform()
    {
    }
    
    CDLTransform& CDLTransform::operator= (const CDLTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection CDLTransform::getDirection() const
    {
        return m_impl->getDirection();
    }
    
    void CDLTransform::setDirection(TransformDirection dir)
    {
        m_impl->setDirection(dir);
    }
    
    const char * CDLTransform::getXML() const
    {
        return m_impl->getXML();
    }
    
    void CDLTransform::setXML(const char * xml)
    {
        m_impl->setXML(xml);
    }

    void CDLTransform::sanityCheck() const
    {
        m_impl->sanityCheck();
    }

    void CDLTransform::setSlope(const float * rgb)
    {
        m_impl->setSlope(rgb);
    }
    
    void CDLTransform::getSlope(float * rgb) const
    {
        m_impl->getSlope(rgb);
    }

    void CDLTransform::setOffset(const float * rgb)
    {
        m_impl->setOffset(rgb);
    }
    
    void CDLTransform::getOffset(float * rgb) const
    {
        m_impl->getOffset(rgb);
    }

    void CDLTransform::setPower(const float * rgb)
    {
        m_impl->setPower(rgb);
    }
    
    void CDLTransform::getPower(float * rgb) const
    {
        m_impl->getPower(rgb);
    }

    void CDLTransform::setSOP(const float * vec9)
    {
        m_impl->setSOP(vec9);
    }
    
    void CDLTransform::getSOP(float * vec9) const
    {
        m_impl->getSOP(vec9);
    }

    void CDLTransform::setSat(float sat)
    {
        m_impl->setSat(sat);
    }
    
    float CDLTransform::getSat() const
    {
        return m_impl->getSat();
    }

    void CDLTransform::getSatLumaCoefs(float * rgb) const
    {
        m_impl->getSatLumaCoefs(rgb);
    }

    void CDLTransform::setID(const char * id)
    {
        m_impl->setID(id);
    }
    
    const char * CDLTransform::getID() const
    {
        return m_impl->getID();
    }

    void CDLTransform::setDescription(const char * desc)
    {
        m_impl->setDescription(desc);
    }
    
    const char * CDLTransform::getDescription() const
    {
        return m_impl->getDescription();
    }
    
    std::ostream& operator<< (std::ostream& os, const CDLTransform& t)
    {
        os << "<CDLTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        
        // TODO: CDL Ostream
        /*
        os << "interpolation=" << InterpolationToString(t.getInterpolation()) << ", ";
        os << "src='" << t.getSrc() << "'";
        */
        os << ">\n";
        return os;
    }
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    namespace
    {
        const char * DEFAULT_CDL_XML = 
            "<ColorCorrection id=''>"
            " <SOPNode>"
            "  <Description/> "
            "  <Slope>1 1 1</Slope> "
            "  <Offset>0 0 0</Offset> "
            "  <Power>1 1 1</Power> "
            " </SOPNode> "
            " <SatNode>"
            "  <Saturation> 1 </Saturation> "
            " </SatNode> "
            " </ColorCorrection>";
    }
    
    
    
    CDLTransform::Impl::Impl() :
        m_direction(TRANSFORM_DIR_FORWARD),
        m_doc(0x0)
    {
        setXML(DEFAULT_CDL_XML);
    }
    
    CDLTransform::Impl::~Impl()
    {
        delete m_doc;
        m_doc = 0x0;
    }
    
    CDLTransform::Impl& CDLTransform::Impl::operator= (const Impl & rhs)
    {
        m_direction = rhs.m_direction;
        m_xml = "";
        
        delete m_doc;
        m_doc = 0x0;
        
        if(rhs.m_doc) m_doc = new TiXmlDocument(*rhs.m_doc);
        
        return *this;
    }
    
    TransformDirection CDLTransform::Impl::getDirection() const
    {
        return m_direction;
    }
    
    void CDLTransform::Impl::setDirection(TransformDirection dir)
    {
        m_direction = dir;
    }
    
    const char * CDLTransform::Impl::getXML() const
    {
        if(m_xml.empty() && m_doc)
        {
            TiXmlPrinter printer;
            //printer.SetIndent("    ");
            printer.SetStreamPrinting();
            m_doc->Accept( &printer );
            m_xml = printer.Str();
        }
        
        return m_xml.c_str();
    }
    
    void CDLTransform::Impl::setXML(const char * xml)
    {
        m_xml = "";
        delete m_doc;
        m_doc = 0x0;
        
        m_doc = new TiXmlDocument();
        m_doc->Parse(xml);
        
        const TiXmlElement* rootElement = m_doc->RootElement();
        if(!rootElement)
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            throw Exception(os.str().c_str());
        }
        if(std::string(rootElement->Value()) != "ColorCorrection")
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Root element is type '" << rootElement->Value() << "', ";
            os << "ColorCorrection expected.";
            throw Exception(os.str().c_str());
        }
    }
    
    void CDLTransform::Impl::sanityCheck() const
    {
        getSlope(0);
        getOffset(0);
        getPower(0);
        getSat();
        getID();
        getDescription();
    }

    void CDLTransform::Impl::setSOPVec(const float * rgb, const char * name)
    {
        m_xml = "";
        
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection.SOPNode.");
        errorName += name;
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).FirstChild( "SOPNode" ).FirstChild(name).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        std::string s = FloatVecToString(rgb, 3);
        SetText(element, s.c_str());
    }
    
    void CDLTransform::Impl::getSOPVec(float * rgb, const char * name) const
    {
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection.SOPNode.");
        errorName += name;
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).FirstChild( "SOPNode" ).FirstChild(name).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        std::vector<std::string> lineParts;
        pystring::split(pystring::strip(element->GetText()), lineParts);
        std::vector<float> floatArray;
        
        if((lineParts.size() != 3) || (!StringVecToFloatVec(floatArray, lineParts)))
        {
            std::ostringstream os;
            os << "Invalid CDL element. " << errorName << " is not convertible to 3 floats.";
            throw Exception(os.str().c_str());
        }
        
        if(rgb) memcpy(rgb, &floatArray[0], 3*sizeof(float));
    }
    
        
    void CDLTransform::Impl::setSlope(const float * rgb)
    {
        setSOPVec(rgb, "Slope");
    }
    
    void CDLTransform::Impl::getSlope(float * rgb) const
    {
        getSOPVec(rgb, "Slope");
    }
    
    void CDLTransform::Impl::setOffset(const float * rgb)
    {
        setSOPVec(rgb, "Offset");
    }
    
    void CDLTransform::Impl::getOffset(float * rgb) const
    {
        getSOPVec(rgb, "Offset");
    }
    
    void CDLTransform::Impl::setPower(const float * rgb)
    {
        setSOPVec(rgb, "Power");
    }
    
    void CDLTransform::Impl::getPower(float * rgb) const
    {
        getSOPVec(rgb, "Power");
    }
    
    void CDLTransform::Impl::setSOP(const float * vec9)
    {
        setSlope(&vec9[0]);
        setOffset(&vec9[3]);
        setPower(&vec9[6]);
    }
    
    void CDLTransform::Impl::getSOP(float * vec9) const
    {
        getSlope(&vec9[0]);
        getOffset(&vec9[3]);
        getPower(&vec9[6]);
    }
    
    void CDLTransform::Impl::setSat(float sat)
    {
        m_xml = "";
        
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection.SatNode.Saturation");
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).FirstChild( "SatNode" ).FirstChild( "Saturation" ).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        std::string s = FloatToString(sat);
        SetText(element, s.c_str());
    
    }
    
    float CDLTransform::Impl::getSat() const
    {
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection.SatNode.Saturation");
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).FirstChild( "SatNode" ).FirstChild( "Saturation" ).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        float sat = 0.0;
        if(!StringToFloat(&sat, element->GetText()))
        {
            std::ostringstream os;
            os << "Invalid CDL element. " << errorName << " not convertible to float.";
            throw Exception(os.str().c_str());
        }
        
        return sat;
    }

    void CDLTransform::Impl::getSatLumaCoefs(float * rgb) const
    {
        if(!rgb) return;
        rgb[0] = 0.2126f;
        rgb[1] = 0.7152f;
        rgb[2] = 0.0722f;
    }
    

    void CDLTransform::Impl::setID(const char * id)
    {
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection");
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        element->SetAttribute("id", id);
    }
    
    const char * CDLTransform::Impl::getID() const
    {
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection");
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        const char * id = element->Attribute("id");
        if(id) return id;
        return "";
    }

    void CDLTransform::Impl::setDescription(const char * desc)
    {
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection.SOPNode.Description");
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).FirstChild( "SOPNode" ).FirstChild( "Description" ).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        SetText(element, desc);
    }
    
    const char * CDLTransform::Impl::getDescription() const
    {
        TiXmlHandle docHandle( m_doc );
        std::string errorName("ColorCorrection.SOPNode.Description");
        
        TiXmlElement* element = docHandle.FirstChild( "ColorCorrection" ).FirstChild( "SOPNode" ).FirstChild( "Description" ).ToElement();
        if(!element)
        {
            std::ostringstream os;
            os << "Invalid CDL element. Could not find " << errorName;
            throw Exception(os.str().c_str());
        }
        
        const char * text = element->GetText();
        if(text) return text;
        return "";
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildCDLOps(OpRcPtrVec * opVec,
                     const Config & /*config*/,
                     const CDLTransform & cdlTransform,
                     TransformDirection dir)
    {
        if(!opVec) return;
        
        float scale4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getSlope(scale4);
        
        float offset4[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        cdlTransform.getOffset(offset4);
        
        float lumaCoef3[] = { 1.0f, 1.0f, 1.0f };
        cdlTransform.getSatLumaCoefs(lumaCoef3);
        
        float sat = cdlTransform.getSat();
        
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  cdlTransform.getDirection());
        
        // TODO: Confirm ASC Sat math is correct.
        // TODO: ASC Power + Clamp
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            // 1) Scale + Offset
            CreateScaleOffsetOp(opVec, scale4, offset4, TRANSFORM_DIR_FORWARD);
            
            // 2) Power + Clamp
            
            // 3) Saturation + Clamp
            CreateSaturationOp(opVec, sat, lumaCoef3, TRANSFORM_DIR_FORWARD);
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            // 3) Saturation + Clamp
            CreateSaturationOp(opVec, sat, lumaCoef3, TRANSFORM_DIR_INVERSE);
            
            // 2) Power + Clamp
            
            // 1) Scale + Offset
            CreateScaleOffsetOp(opVec, scale4, offset4, TRANSFORM_DIR_INVERSE);
        }
    }
}
OCIO_NAMESPACE_EXIT
