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

#include <fstream>
#include <tinyxml.h>

#include <OpenColorIO/OpenColorIO.h>

#include "CDLTransform.h"
#include "ExponentOps.h"
#include "MatrixOps.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        /*
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
        
        */
        
        // http://ticpp.googlecode.com/svn/docs/ticpp_8h-source.html#l01670
        
        void SetTiXmlText( TiXmlElement* element, const char * value)
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
        
        std::string BuildXML(const CDLTransform & cdl)
        {
            TiXmlDocument doc;
            
            // TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
            TiXmlElement * root = new TiXmlElement( "ColorCorrection" );
            doc.LinkEndChild( root );
            root->SetAttribute("id", cdl.getID());
            
            TiXmlElement * sop = new TiXmlElement( "SOPNode" );
            root->LinkEndChild( sop );
            
            TiXmlElement * desc = new TiXmlElement( "Description" );
            sop->LinkEndChild( desc );
            SetTiXmlText(desc, cdl.getDescription());
            
            TiXmlElement * slope = new TiXmlElement( "Slope" );
            sop->LinkEndChild( slope );
            float slopeval[3];
            cdl.getSlope(slopeval);
            SetTiXmlText(slope, FloatVecToString(slopeval, 3).c_str());
            
            TiXmlElement * offset = new TiXmlElement( "Offset" );
            sop->LinkEndChild( offset );
            float offsetval[3];
            cdl.getOffset(offsetval);
            SetTiXmlText(offset, FloatVecToString(offsetval, 3).c_str());
            
            TiXmlElement * power = new TiXmlElement( "Power" );
            sop->LinkEndChild( power );
            float powerval[3];
            cdl.getPower(powerval);
            SetTiXmlText(power, FloatVecToString(powerval, 3).c_str());
            
            TiXmlElement * sat = new TiXmlElement( "SatNode" );
            root->LinkEndChild( sat );
            
            TiXmlElement * saturation = new TiXmlElement( "Saturation" );
            sat->LinkEndChild( saturation );
            SetTiXmlText(saturation, FloatToString(cdl.getSat()).c_str());
            
            TiXmlPrinter printer;
            printer.SetStreamPrinting();
            doc.Accept( &printer );
            return printer.Str();
        }
    }
    
    void LoadCDL(CDLTransform * cdl, TiXmlElement * root)
    {
        if(!cdl) return;
        
        if(!root)
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Null root element.";
            throw Exception(os.str().c_str());
        }
        
        if(std::string(root->Value()) != "ColorCorrection")
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Root element is type '" << root->Value() << "', ";
            os << "ColorCorrection expected.";
            throw Exception(os.str().c_str());
        }
        
        TiXmlHandle handle( root );
        
        const char * id = root->Attribute("id");
        if(!id) id = "";
        
        cdl->setID(id);
        
        TiXmlElement* desc = handle.FirstChild( "SOPNode" ).FirstChild("Description").ToElement();
        if(desc)
        {
            const char * text = desc->GetText();
            if(text) cdl->setDescription(text);
        }
        
        std::vector<std::string> lineParts;
        std::vector<float> floatArray;
        
        TiXmlElement* slope = handle.FirstChild( "SOPNode" ).FirstChild("Slope").ToElement();
        if(slope)
        {
            const char * text = slope->GetText();
            if(text)
            {
                pystring::split(pystring::strip(text), lineParts);
                if((lineParts.size() != 3) || (!StringVecToFloatVec(floatArray, lineParts)))
                {
                    std::ostringstream os;
                    os << "Error loading CDL xml. ";
                    os << id << ".SOPNode.Slope text '";
                    os << text << "' is not convertible to 3 floats.";
                    throw Exception(os.str().c_str());
                }
                cdl->setSlope(&floatArray[0]);
            }
        }
        
        TiXmlElement* offset = handle.FirstChild( "SOPNode" ).FirstChild("Offset").ToElement();
        if(offset)
        {
            const char * text = offset->GetText();
            if(text)
            {
                pystring::split(pystring::strip(text), lineParts);
                if((lineParts.size() != 3) || (!StringVecToFloatVec(floatArray, lineParts)))
                {
                    std::ostringstream os;
                    os << "Error loading CDL xml. ";
                    os << id << ".SOPNode.Offset text '";
                    os << text << "' is not convertible to 3 floats.";
                    throw Exception(os.str().c_str());
                }
                cdl->setOffset(&floatArray[0]);
            }
        }
        
        TiXmlElement* power = handle.FirstChild( "SOPNode" ).FirstChild("Power").ToElement();
        if(power)
        {
            const char * text = power->GetText();
            if(text)
            {
                pystring::split(pystring::strip(text), lineParts);
                if((lineParts.size() != 3) || (!StringVecToFloatVec(floatArray, lineParts)))
                {
                    std::ostringstream os;
                    os << "Error loading CDL xml. ";
                    os << id << ".SOPNode.Power text '";
                    os << text << "' is not convertible to 3 floats.";
                    throw Exception(os.str().c_str());
                }
                cdl->setPower(&floatArray[0]);
            }
        }
        
        TiXmlElement* sat = handle.FirstChild( "SatNode" ).FirstChild("Saturation").ToElement();
        if(sat)
        {
            const char * text = sat->GetText();
            if(text)
            {
                float satval = 1.0f;
                if(!StringToFloat(&satval, text))
                {
                    std::ostringstream os;
                    os << "Error loading CDL xml. ";
                    os << id << ".SatNode.Saturation text '";
                    os << text << "' is not convertible to float.";
                    throw Exception(os.str().c_str());
                }
                cdl->setSat(satval);
            }
        }
    }
    
    
    
    void GetCDLTransforms(CDLTransformMap & transforms,
                          TiXmlElement * cccRootElement)
    {
        if(std::string(cccRootElement->Value()) != "ColorCorrectionCollection")
        {
            std::ostringstream os;
            os << "GetCDLTransforms Error. ";
            os << "Root element is type '" << cccRootElement->Value() << "', ";
            os << "ColorCorrectionCollection expected.";
            throw Exception(os.str().c_str());
        }
        
        TiXmlNode * child = cccRootElement->FirstChild("ColorCorrection");
        while(child)
        {
            CDLTransformRcPtr transform = CDLTransform::Create();
            LoadCDL(transform.get(), child->ToElement());
            
            std::string id = transform->getID();
            if(id.empty())
            {
                std::ostringstream os;
                os << "Error loading ccc xml, ";
                os << "All ASC ColorCorrections must specify an 'id' value.";
                throw Exception(os.str().c_str());
            }
            
            CDLTransformMap::iterator iter = transforms.find(id);
            if(iter != transforms.end())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "All ASC ColorCorrections must specify a unique 'id' value. ";
                os << "Duplicate elements with '" << id << "' found.";
                throw Exception(os.str().c_str());
            }
            
            transforms[id] = transform;
            
            child = child->NextSibling("ColorCorrection");
        }
    }
    
    void LoadCDL(CDLTransform * cdl, const char * xml)
    {
        if(!xml || (strlen(xml) == 0))
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Null string provided.";
            throw Exception(os.str().c_str());
        }
        
        TiXmlDocument doc;
        doc.Parse(xml);
        
        if(doc.Error())
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << doc.ErrorDesc() << " (line ";
            os << doc.ErrorRow() << ", character ";
            os << doc.ErrorCol() << ")";
            throw Exception(os.str().c_str());
        }
        
        if(!doc.RootElement())
        {
            std::ostringstream os;
            os << "Error loading CDL xml, ";
            os << "please confirm the xml is valid.";
            throw Exception(os.str().c_str());
        }
        
        LoadCDL(cdl, doc.RootElement()->ToElement());
    }
    
    CDLTransformRcPtr CDLTransform::Create()
    {
        return CDLTransformRcPtr(new CDLTransform(), &deleter);
    }
    
    namespace
    {
        std::string GetCDLLocalCacheKey(const std::string & src,
            const std::string & cccid)
        {
            return src + " : " + cccid;
        }
        
        CDLTransformMap g_cache;
        Mutex g_cacheMutex;
    }
    
    void ClearCDLTransformFileCache()
    {
        AutoMutex lock(g_cacheMutex);
        g_cache.clear();
    }
    
    // TODO: Expose functions for introspecting in ccc file
    // TODO: Share caching with normal cdl pathway
    
    CDLTransformRcPtr CDLTransform::CreateFromFile(const char * src, const char * cccid)
    {
        if(!src || (strlen(src) == 0) || !cccid)
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Source file not specified.";
            throw Exception(os.str().c_str());
        }
        
        // Check cache
        AutoMutex lock(g_cacheMutex);
        {
            CDLTransformMap::iterator iter = 
                g_cache.find(GetCDLLocalCacheKey(src,cccid));
            if(iter != g_cache.end())
            {
                return iter->second;
            }
        }
        
        std::ifstream istream(src);
        if(istream.fail()) {
            std::ostringstream os;
            os << "Error could not read CDL source file '" << src;
            os << "'. Please verify the file exists and appropriate ";
            os << "permissions are set.";
            throw Exception (os.str().c_str());
        }
        
        // Read the file into a string.
        std::ostringstream rawdata;
        rawdata << istream.rdbuf();
        std::string xml = rawdata.str();
        
        if(xml.empty())
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "The specified source file, '";
            os << src << "' appears to be empty.";
            throw Exception(os.str().c_str());
        }
        
        TiXmlDocument doc;
        doc.Parse(xml.c_str());
        
        if(doc.Error())
        {
            std::ostringstream os;
            os << "Error loading CDL xml from file '";
            os << src << "'. ";
            os << doc.ErrorDesc() << " (line ";
            os << doc.ErrorRow() << ", character ";
            os << doc.ErrorCol() << ")";
            throw Exception(os.str().c_str());
        }
        
        if(!doc.RootElement())
        {
            std::ostringstream os;
            os << "Error loading CDL xml from file '";
            os << src << "'. ";
            os << "Please confirm the xml is valid.";
            throw Exception(os.str().c_str());
        }
        
        std::string rootValue = doc.RootElement()->Value();
        if(rootValue == "ColorCorrection")
        {
            // Load a single ColorCorrection into the cache
            CDLTransformRcPtr cdl = CDLTransform::Create();
            LoadCDL(cdl.get(), doc.RootElement()->ToElement());
            g_cache[GetCDLLocalCacheKey(src,cccid)] = cdl;
            return cdl;
        }
        else if(rootValue == "ColorCorrectionCollection")
        {
            // Load all CCs from the ColorCorrectionCollection
            // into the cache
            
            CDLTransformMap transforms;
            GetCDLTransforms(transforms, doc.RootElement());
            
            if(transforms.empty())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "No ColorCorrection elements found in file '";
                os << src << "'.";
                throw Exception(os.str().c_str());
            }
            
            for(CDLTransformMap::iterator iter = transforms.begin();
                iter != transforms.end();
                ++iter)
            {
                g_cache[GetCDLLocalCacheKey(src,iter->first)] = iter->second;
            }
            
            CDLTransformMap::iterator cciter = g_cache.find(GetCDLLocalCacheKey(src,cccid));
            if(cciter == g_cache.end())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "The specified cccid " << cccid << " ";
                os << "could not be found in file '";
                os << src << "'.";
                throw Exception(os.str().c_str());
            }
            
            return cciter->second;
        }
        
        std::ostringstream os;
        os << "Error loading CDL xml from file '";
        os << src << "'. ";
        os << "Root xml element is type '" << rootValue << "', ";
        os << "ColorCorrection or ColorCorrectionCollection expected.";
        throw Exception(os.str().c_str());
    }
    
    void CDLTransform::deleter(CDLTransform* t)
    {
        delete t;
    }
    
    class CDLTransform::Impl
    {
    public:
        TransformDirection dir_;
        
        float sop_[9];
        float sat_;
        std::string id_;
        std::string description_;
        
        mutable std::string xml_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            sat_(1.0f)
        {
            sop_[0] = 1.0f;
            sop_[1] = 1.0f;
            sop_[2] = 1.0f;
            sop_[3] = 0.0f;
            sop_[4] = 0.0f;
            sop_[5] = 0.0f;
            sop_[6] = 1.0f;
            sop_[7] = 1.0f;
            sop_[8] = 1.0f;
        }
        
        ~Impl()
        {
        
        }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            
            memcpy(sop_, rhs.sop_, sizeof(float)*9);
            sat_ = rhs.sat_;
            id_ = rhs.id_;
            description_ = rhs.description_;
            
            return *this;
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
        getImpl()->xml_ = BuildXML(*this);
        return getImpl()->xml_.c_str();
    }
    
    void CDLTransform::setXML(const char * xml)
    {
        LoadCDL(this, xml);
    }
    
    // We use this approach, rather than comparing XML to get around the
    // case where setXML with extra data was provided.
    
    bool CDLTransform::equals(const ConstCDLTransformRcPtr & other) const
    {
        if(!other) return false;
        
        if(getImpl()->dir_ != other->getImpl()->dir_) return false;
        
        const float abserror = 1e-9f;
        
        for(int i=0; i<9; ++i)
        {
            if(!equalWithAbsError(getImpl()->sop_[i], other->getImpl()->sop_[i], abserror))
            {
                return false;
            }
        }
        
        if(!equalWithAbsError(getImpl()->sat_, other->getImpl()->sat_, abserror))
        {
            return false;
        }
        
        if(getImpl()->id_ != other->getImpl()->id_)
        {
            return false;
        }
        
        if(getImpl()->description_ != other->getImpl()->description_)
        {
            return false;
        }
        
        return true;
    }
    
    void CDLTransform::setSlope(const float * rgb)
    {
        memcpy(&getImpl()->sop_[0], rgb, sizeof(float)*3);
    }
    
    void CDLTransform::getSlope(float * rgb) const
    {
        memcpy(rgb, &getImpl()->sop_[0], sizeof(float)*3);
    }

    void CDLTransform::setOffset(const float * rgb)
    {
        memcpy(&getImpl()->sop_[3], rgb, sizeof(float)*3);
    }
    
    void CDLTransform::getOffset(float * rgb) const
    {
        memcpy(rgb, &getImpl()->sop_[3], sizeof(float)*3);
    }

    void CDLTransform::setPower(const float * rgb)
    {
        memcpy(&getImpl()->sop_[6], rgb, sizeof(float)*3);
    }
    
    void CDLTransform::getPower(float * rgb) const
    {
        memcpy(rgb, &getImpl()->sop_[6], sizeof(float)*3);
    }

    void CDLTransform::setSOP(const float * vec9)
    {
        memcpy(&getImpl()->sop_, vec9, sizeof(float)*9);
    }
    
    void CDLTransform::getSOP(float * vec9) const
    {
        memcpy(vec9, &getImpl()->sop_, sizeof(float)*9);
    }

    void CDLTransform::setSat(float sat)
    {
        getImpl()->sat_ = sat;
    }
    
    float CDLTransform::getSat() const
    {
        return getImpl()->sat_;
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
        if(id)
        {
            getImpl()->id_ = id;
        }
        else
        {
            getImpl()->id_ = "";
        }
    }
    
    const char * CDLTransform::getID() const
    {
        return getImpl()->id_.c_str();
    }
    
    void CDLTransform::setDescription(const char * desc)
    {
        if(desc)
        {
            getImpl()->description_ = desc;
        }
        else
        {
            getImpl()->description_ = "";
        }
    }
    
    const char * CDLTransform::getDescription() const
    {
        return getImpl()->description_.c_str();
    }
    
    std::ostream& operator<< (std::ostream& os, const CDLTransform& t)
    {
        float sop[9];
        t.getSOP(sop);
        
        os << "<CDLTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "sop=";
        for (unsigned int i=0; i<9; ++i)
        {
            if(i!=0) os << " ";
            os << sop[i];
        }
        os << ", ";
        os << "sat=" << t.getSat() << ",";
        os << TransformDirectionToString(t.getDirection()) << ", ";
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
