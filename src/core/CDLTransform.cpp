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

#include "ExponentOps.h"
#include "MatrixOps.h"
#include "MathUtils.h"
#include "OpBuilders.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include "tinyxml/tinyxml.h"

OCIO_NAMESPACE_ENTER
{
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
        
        // http://ticpp.googlecode.com/svn/docs/ticpp_8h-source.html#l01670
        
        void SetText( TiXmlElement* element, const char * value)
        {
            if ( element->NoChildren() )
            {
                element->LinkEndChild( new TiXmlText( value ) );
            }
            else
            {
                if ( 0 == element->GetText() )
                {
                    element->InsertBeforeChild( element->FirstChild(), TiXmlText( value ) );
                }
                else
                {
                    // There already is text, so change it
                    element->FirstChild()->SetValue( value );
                }
            }
        }
        
    }
    
    CDLTransformRcPtr CDLTransform::Create()
    {
        return CDLTransformRcPtr(new CDLTransform(), &deleter);
    }
    
    void CDLTransform::deleter(CDLTransform* t)
    {
        delete t;
    }
    
    class CDLTransform::Impl
    {
    public:
        TransformDirection dir_;
        TiXmlDocument * doc_;
        mutable std::string xml_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            doc_(0x0)
        {
            setXML(DEFAULT_CDL_XML);
        }
        
        ~Impl()
        {
            delete doc_;
            doc_ = 0x0;
        }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            xml_ = "";
            
            delete doc_;
            doc_ = 0x0;
            
            if(rhs.doc_) doc_ = new TiXmlDocument(*rhs.doc_);
            return *this;
        }
        
        const char * getXML() const
        {
            if(xml_.empty() && doc_)
            {
                TiXmlPrinter printer;
                //printer.SetIndent("    ");
                printer.SetStreamPrinting();
                doc_->Accept( &printer );
                xml_ = printer.Str();
            }
            
            return xml_.c_str();
        }
        
        void setXML(const char * xml)
        {
            xml_ = "";
            delete doc_;
            doc_ = 0x0;
            
            doc_ = new TiXmlDocument();
            doc_->Parse(xml);
            
            const TiXmlElement* rootElement = doc_->RootElement();
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
        
        void setSOPVec(const float * rgb, const char * name)
        {
            xml_ = "";
            
            TiXmlHandle docHandle( doc_ );
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
        
        void getSOPVec(float * rgb, const char * name) const
        {
            TiXmlHandle docHandle( doc_ );
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
    };
    
    
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
        delete m_impl;
        m_impl = NULL;
    }
    
    CDLTransform& CDLTransform::operator= (const CDLTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection CDLTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void CDLTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    const char * CDLTransform::getXML() const
    {
        return getImpl()->getXML();
    }
    
    void CDLTransform::setXML(const char * xml)
    {
        getImpl()->setXML(xml);
        
        // These will throw an exception if the xml is in any way invalid.
        getSlope(0);
        getOffset(0);
        getPower(0);
        getSat();
        getID();
        getDescription();
    }
    
    // We use this approach, rather than comparing XML to get around the
    // case where setXML with extra data was provided.
    
    bool CDLTransform::equals(const ConstCDLTransformRcPtr & other) const
    {
        if(!other) return false;
        
        if(getImpl()->dir_ != other->getImpl()->dir_) return false;
        
        float sop1[9];
        getSOP(sop1);
        
        float sop2[9];
        other->getSOP(sop2);
        
        const float abserror = 1e-9f;
        
        for(int i=0; i<9; ++i)
        {
            if(!equalWithAbsError(sop1[i], sop2[i], abserror))
            {
                return false;
            }
        }
        
        float sat1 = getSat();
        float sat2 = other->getSat();
        if(!equalWithAbsError(sat1, sat2, abserror))
        {
            return false;
        }
        
        std::string desc1 = getDescription();
        std::string desc2 = other->getDescription();
        if(desc1 != desc2) return false;
        
        std::string id1 = getID();
        std::string id2 = other->getID();
        if(id1 != id2) return false;
        
        return true;
    }
    
    void CDLTransform::setSlope(const float * rgb)
    {
        getImpl()->setSOPVec(rgb, "Slope");
    }
    
    void CDLTransform::getSlope(float * rgb) const
    {
        getImpl()->getSOPVec(rgb, "Slope");
    }

    void CDLTransform::setOffset(const float * rgb)
    {
        getImpl()->setSOPVec(rgb, "Offset");
    }
    
    void CDLTransform::getOffset(float * rgb) const
    {
        getImpl()->getSOPVec(rgb, "Offset");
    }

    void CDLTransform::setPower(const float * rgb)
    {
        getImpl()->setSOPVec(rgb, "Power");
    }
    
    void CDLTransform::getPower(float * rgb) const
    {
        getImpl()->getSOPVec(rgb, "Power");
    }

    void CDLTransform::setSOP(const float * vec9)
    {
        setSlope(&vec9[0]);
        setOffset(&vec9[3]);
        setPower(&vec9[6]);
    }
    
    void CDLTransform::getSOP(float * vec9) const
    {
        getSlope(&vec9[0]);
        getOffset(&vec9[3]);
        getPower(&vec9[6]);
    }

    void CDLTransform::setSat(float sat)
    {
        getImpl()->xml_ = "";
        
        TiXmlHandle docHandle( getImpl()->doc_ );
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
    
    float CDLTransform::getSat() const
    {
        TiXmlHandle docHandle( getImpl()->doc_ );
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

    void CDLTransform::getSatLumaCoefs(float * rgb) const
    {
        if(!rgb) return;
        rgb[0] = 0.2126f;
        rgb[1] = 0.7152f;
        rgb[2] = 0.0722f;
    }

    void CDLTransform::setID(const char * id)
    {
        getImpl()->xml_ = "";
        
        TiXmlHandle docHandle( getImpl()->doc_ );
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
    
    const char * CDLTransform::getID() const
    {
        TiXmlHandle docHandle( getImpl()->doc_ );
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

    void CDLTransform::setDescription(const char * desc)
    {
        getImpl()->xml_ = "";
        
        TiXmlHandle docHandle( getImpl()->doc_ );
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
    
    const char * CDLTransform::getDescription() const
    {
        TiXmlHandle docHandle( getImpl()->doc_ );
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
    
    void BuildCDLOps(OpRcPtrVec & ops,
                     const Config & /*config*/,
                     const CDLTransform & cdlTransform,
                     TransformDirection dir)
    {
        float scale4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getSlope(scale4);
        
        float offset4[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        cdlTransform.getOffset(offset4);
        
        float power4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getPower(power4);
        
        float lumaCoef3[] = { 1.0f, 1.0f, 1.0f };
        cdlTransform.getSatLumaCoefs(lumaCoef3);
        
        float sat = cdlTransform.getSat();
        
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  cdlTransform.getDirection());
        
        // TODO: Confirm ASC Sat math is correct.
        // TODO: Handle Clamping conditions more explicitly
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);
            
            // 2) Power + Clamp
            CreateExponentOp(ops, power4, TRANSFORM_DIR_FORWARD);
            
            // 3) Saturation + Clamp
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD);
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            // 3) Saturation + Clamp
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE);
            
            // 2) Power + Clamp
            CreateExponentOp(ops, power4, TRANSFORM_DIR_INVERSE);
            
            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_INVERSE);
        }
    }
}
OCIO_NAMESPACE_EXIT
