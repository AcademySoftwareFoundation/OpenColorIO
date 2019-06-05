/*
Copyright (c) 2018 Autodesk Inc., et al.
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

#ifndef INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERHELPER_H
#define INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERHELPER_H

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "fileformats/ctf/CTFTransform.h"
#include "ops/OpArray.h"
#include "ops/CDL/CDLOpData.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "ops/FixedFunction/FixedFunctionOpData.h"
#include "ops/Gamma/GammaOpData.h"
#include "ops/IndexMapping.h"
#include "ops/Log/LogOpData.h"
#include "ops/Log/LogUtils.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "ops/Lut3D/Lut3DOpData.h"
#include "ops/Metadata.h"
#include "ops/Range/RangeOpData.h"
#include "ops/reference/ReferenceOpData.h"

OCIO_NAMESPACE_ENTER
{

// Used by CLF/CTF parser.

class CTFReaderTransformElt : public XmlReaderContainerElt
{
public:
    CTFReaderTransformElt(const std::string & name,
                          unsigned int xmlLineNumber,
                          const std::string & xmlFile,
                          bool isCLF);

    ~CTFReaderTransformElt()
    {
    }

    const std::string & getIdentifier() const override;

    void start(const char ** atts) override;

    void end() override;

    void appendDescription(const std::string & desc) override;

    // Get its parent which is a Transform.
    const CTFReaderTransformPtr & getTransform() const;

    const char * getTypeName() const override;

    // Set the current transform CTF version.
    void setVersion(const CTFVersion & ver);

    void setCLFVersion(const CTFVersion & ver);

    // Get current xml transform version
    const CTFVersion & getVersion() const;

private:
    CTFReaderTransformElt() = delete;

    // The associated Transform.
    CTFReaderTransformPtr m_transform;
    // Is it a clf file?
    bool m_isCLF;
};

typedef OCIO_SHARED_PTR<CTFReaderTransformElt> CTFReaderTransformEltRcPtr;

class CTFReaderMetadataElt : public XmlReaderComplexElt
{
public:
    CTFReaderMetadataElt(const std::string & name,
                         ContainerEltRcPtr pParent,
                         unsigned int xmlLineNumber,
                         const std::string & xmlFile);

    virtual ~CTFReaderMetadataElt();

    const std::string& getIdentifier() const override;

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine);

    Metadata & getMetadata() { return m_metadata; }

protected:
    Metadata m_metadata;

private:
    CTFReaderMetadataElt() = delete;
};

class CTFReaderInfoElt : public CTFReaderMetadataElt
{
public:
    CTFReaderInfoElt(const std::string & name,
                     ContainerEltRcPtr pParent,
                     unsigned int xmlLineNumber,
                     const std::string & xmlFile);

    virtual ~CTFReaderInfoElt();

    void start(const char ** atts) override;

    void end() override;

private:
    CTFReaderInfoElt() = delete;
};

class CTFReaderInputDescriptorElt : public XmlReaderPlainElt
{
public:
    CTFReaderInputDescriptorElt(const std::string & name,
                                ContainerEltRcPtr pParent,
                                unsigned int xmlLineNumber,
                                const std::string & xmlFile)
        : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    ~CTFReaderInputDescriptorElt()
    {
    }

    void start(const char ** atts) override
    {
    }

    void end() override
    {
    }

    void setRawData(const char * str, size_t len, unsigned int /*xmlLine*/) override
    {
        CTFReaderTransformElt* pTransform
            = dynamic_cast<CTFReaderTransformElt*>(getParent().get());

        std::string s = pTransform->getTransform()->getInputDescriptor();
        s += std::string(str, len);

        pTransform->getTransform()->setInputDescriptor(s);
    }


private:
    CTFReaderInputDescriptorElt() = delete;
};

class CTFReaderOutputDescriptorElt : public XmlReaderPlainElt
{
public:
    CTFReaderOutputDescriptorElt(const std::string & name,
                                 ContainerEltRcPtr pParent,
                                 unsigned int xmlLineNumber,
                                 const std::string & xmlFile)
        : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }


    ~CTFReaderOutputDescriptorElt()
    {
    }

    void start(const char **atts) override
    {
    }

    void end() override
    {
    }

    void setRawData(const char* str, size_t len, unsigned int xmlLine) override
    {
        CTFReaderTransformElt* pTransform
            = dynamic_cast<CTFReaderTransformElt*>(getParent().get());

        std::string s = pTransform->getTransform()->getOutputDescriptor();
        s += std::string(str, len);

        pTransform->getTransform()->setOutputDescriptor(s);
    }

private:
    CTFReaderOutputDescriptorElt() = delete;
};

class CTFReaderArrayElt : public XmlReaderPlainElt
{
public:
    CTFReaderArrayElt(const std::string & name,
                      ContainerEltRcPtr pParent,
                      unsigned int xmlLineNumber,
                      const std::string & xmlFile);
    ~CTFReaderArrayElt();

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;

    const char * getTypeName() const override;

private:
    CTFReaderArrayElt() = delete;

    // The array to fill (pointer not owned).
    // Array is managed as a member object of an OpData.
    ArrayBase * m_array;

    // The current position to fill.
    unsigned int m_position;
};

class CTFArrayMgt
{
public:
    typedef std::vector<unsigned int> Dimensions;

public:
    CTFArrayMgt();
    virtual ~CTFArrayMgt();

    virtual void setCompleted(bool status)
    {
        m_completed = status;
    }

    bool isCompleted()
    {
        return m_completed;
    }

    virtual ArrayBase * updateDimension(const Dimensions & dims) = 0;

    // Finalize the array data origanization.
    // - position is the position of the last value found.
    virtual void endArray(unsigned int position) = 0;

private:
    bool m_completed;
};

class CTFReaderIndexMapElt : public XmlReaderPlainElt
{
public:
    CTFReaderIndexMapElt(const std::string & name,
                         ContainerEltRcPtr pParent,
                         unsigned xmlLineNumber,
                         const std::string & xmlFile);

    ~CTFReaderIndexMapElt();

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;

    const char * getTypeName() const override;

private:
    CTFReaderIndexMapElt() = delete;

    IndexMapping * m_indexMap;

    // The current position to fill.
    unsigned int m_position;
};

// Class to track creation of IndexMaps.
class CTFIndexMapMgt
{
public:
    // Hold the array dimensions.
    typedef std::vector<unsigned int> DimensionsIM;

public:
    CTFIndexMapMgt()
        : m_completed(false)
    {
    }

    virtual ~CTFIndexMapMgt()
    {
    }

    void setCompletedIM(bool status)
    {
        m_completed = status;
    }

    bool isCompletedIM()
    {
        return m_completed;
    }

    virtual IndexMapping * updateDimensionIM(const DimensionsIM & dims) = 0;

    // Finalize the array data origanization.
    // - position is the position of the last value found.
    virtual void endIndexMap(unsigned int position) = 0;

private:
    bool m_completed;
};

class CTFReaderOpElt;
typedef OCIO_SHARED_PTR<CTFReaderOpElt> CTFReaderOpEltRcPtr;

class CTFReaderOpElt : public XmlReaderContainerElt
{
public:
    // Enumeration of all possible reader types.
    enum Type
    {
        CDLType,
        Lut1DType,
        Lut3DType,
        MatrixType,
        RangeType,
        // CTF types.
        ACESType,
        ExposureContrastType,
        FixedFunctionType,
        GammaType,
        InvLut1DType,
        InvLut3DType,
        LogType,
        ReferenceType,

        // Note: Keep at end of list.
        NoType
    };

    CTFReaderOpElt();

    ~CTFReaderOpElt();

    void setContext(const std::string & name,
                    const CTFReaderTransformPtr & pTransform,
                    unsigned int xmlLineNumber,
                    const std::string & xmlFile);

    const std::string & getIdentifier() const override;

    virtual const OpDataRcPtr getOp() const = 0;

    void appendDescription(const std::string & desc) override;

    void start(const char ** atts) override;

    void end() override;

    const char * getTypeName() const override;

    // Get the right reader using its type and
    // the xml transform version.
    static CTFReaderOpEltRcPtr GetReader(Type type,
                                         const CTFVersion & version);

protected:
    static BitDepth GetBitDepth(const std::string & str);

protected:
    CTFReaderTransformPtr m_transform;  // The parent
};

class CTFReaderACESElt : public CTFReaderOpElt
{
public:
    CTFReaderACESElt();
    ~CTFReaderACESElt();

    void start(const char **atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const FixedFunctionOpDataRcPtr getFixedFunction() const
    {
        return m_fixedFunction;
    }

private:
    FixedFunctionOpDataRcPtr m_fixedFunction;
};

class CTFReaderACESParamsElt : public XmlReaderPlainElt
{
public:
    CTFReaderACESParamsElt(const std::string & name,
                           ContainerEltRcPtr pParent,
                           unsigned int xmlLineNumber,
                           const std::string & xmlFile);

    ~CTFReaderACESParamsElt();

    void start(const char **atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;
};

class CTFReaderCDLElt : public CTFReaderOpElt
{
public:
    CTFReaderCDLElt();

    ~CTFReaderCDLElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const CDLOpDataRcPtr & getCDL() const;

protected:
    CDLOpDataRcPtr m_cdl;
};

class CTFReaderSatNodeElt : public XmlReaderSatNodeBaseElt
{
public:
    CTFReaderSatNodeElt(const std::string & name,
                        ContainerEltRcPtr pParent,
                        unsigned int xmlLineNumber,
                        const std::string & xmlFile);

    CTFReaderSatNodeElt() = delete;

    const CDLOpDataRcPtr & getCDL() const override;

};

class CTFReaderSOPNodeElt : public XmlReaderSOPNodeBaseElt
{
public:
    CTFReaderSOPNodeElt(const std::string & name,
                        ContainerEltRcPtr pParent,
                        unsigned int xmlLocation,
                        const std::string & xmlFile);

    CTFReaderSOPNodeElt() = delete;

    const CDLOpDataRcPtr & getCDL() const override;
};

class CTFReaderFixedFunctionElt : public CTFReaderOpElt
{
public:
    CTFReaderFixedFunctionElt();
    ~CTFReaderFixedFunctionElt();

    void start(const char **atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const FixedFunctionOpDataRcPtr getFixedFunction() const
    {
        return m_fixedFunction;
    }

private:
    FixedFunctionOpDataRcPtr m_fixedFunction;
};

class CTFReaderGammaParamsElt;
typedef OCIO_SHARED_PTR<CTFReaderGammaParamsElt> CTFReaderGammaParamsEltRcPtr;

class CTFReaderGammaElt : public CTFReaderOpElt
{
public:
    CTFReaderGammaElt();
    ~CTFReaderGammaElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const GammaOpDataRcPtr & getGamma() const { return m_gamma; }

    virtual CTFReaderGammaParamsEltRcPtr createGammaParamsElt(
        const std::string & name,
        ContainerEltRcPtr pParent,
        unsigned int xmlLineNumber,
        const std::string & xmlFile) const;

private:
    GammaOpDataRcPtr m_gamma;
};

class CTFReaderGammaElt_1_5 : public CTFReaderGammaElt
{
public:
    CTFReaderGammaElt_1_5() : CTFReaderGammaElt() {}
    ~CTFReaderGammaElt_1_5() {}

    void end() override;

    CTFReaderGammaParamsEltRcPtr createGammaParamsElt(
        const std::string & name,
        ContainerEltRcPtr pParent,
        unsigned int xmlLineNumber,
        const std::string & xmlFile) const override;
};

class CTFReaderGammaParamsElt : public XmlReaderPlainElt
{
public:
    CTFReaderGammaParamsElt(const std::string & name,
                            ContainerEltRcPtr pParent,
                            unsigned int xmlLineNumber,
                            const std::string & xmlFile);

    ~CTFReaderGammaParamsElt();

    void start(const char ** atts);
    void end();

    void setRawData(const char * str, size_t len, unsigned int xmlLine);

protected:
    virtual int getChannelNumber(const char * name) const;
};

class CTFReaderGammaParamsElt_1_5 : public CTFReaderGammaParamsElt
{
public:
    CTFReaderGammaParamsElt_1_5(const std::string & name,
                                ContainerEltRcPtr pParent,
                                unsigned int xmlLineNumber,
                                const std::string & xmlFile);
    ~CTFReaderGammaParamsElt_1_5();

protected:
    int getChannelNumber(const char * name) const override;
};

class CTFReaderInvLut1DElt : public CTFReaderOpElt, public CTFArrayMgt
{
public:
    CTFReaderInvLut1DElt();
    ~CTFReaderInvLut1DElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const Lut1DOpDataRcPtr & getLut() const { return m_invLut; }

    ArrayBase * updateDimension(const Dimensions & dims) override;

    void endArray(unsigned int position) override;

private:
    Lut1DOpDataRcPtr m_invLut;
};

class CTFReaderInvLut3DElt : public CTFReaderOpElt, public CTFArrayMgt
{
public:
    CTFReaderInvLut3DElt();
    ~CTFReaderInvLut3DElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const Lut3DOpDataRcPtr & getLut() const { return m_invLut; }

    ArrayBase * updateDimension(const Dimensions & dims) override;

    void endArray(unsigned int position) override;

private:
    Lut3DOpDataRcPtr m_invLut;
};

class CTFReaderLogElt : public CTFReaderOpElt
{
public:
    CTFReaderLogElt();

    ~CTFReaderLogElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const LogOpDataRcPtr & getLog() const
    {
        return m_log;
    }

    LogUtil::CTFParams & getCTFParams()
    {
        return m_ctfParams;
    }

protected:
    LogUtil::CTFParams m_ctfParams;
    LogOpDataRcPtr m_log;
};

class CTFReaderLogParamsElt : public XmlReaderPlainElt
{
public:
    CTFReaderLogParamsElt(const std::string & name,
                          ContainerEltRcPtr pParent,
                          unsigned int xmlLineNumber,
                          const std::string & xmlFile);

    ~CTFReaderLogParamsElt();

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;
};

class CTFReaderLut1DElt : public CTFReaderOpElt, public CTFArrayMgt, public CTFIndexMapMgt
{
public:
    CTFReaderLut1DElt();
    ~CTFReaderLut1DElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const Lut1DOpDataRcPtr & getLut() const
    {
        return m_lut;
    }

    ArrayBase * updateDimension(const Dimensions & dims) override;

    void endArray(unsigned int position) override;

    IndexMapping * updateDimensionIM(const DimensionsIM & dims) override;

    void endIndexMap(unsigned int position) override;

protected:
    Lut1DOpDataRcPtr m_lut;
    IndexMapping     m_indexMapping;
};

class CTFReaderLut1DElt_1_4 : public CTFReaderLut1DElt
{
public:
    CTFReaderLut1DElt_1_4() : CTFReaderLut1DElt() {}

    ~CTFReaderLut1DElt_1_4() {}

    void start(const char **atts) override;
};

class CTFReaderLut1DElt_1_7 : public CTFReaderLut1DElt_1_4
{
public:
    CTFReaderLut1DElt_1_7() : CTFReaderLut1DElt_1_4() {}

    ~CTFReaderLut1DElt_1_7() {}

    void end() override;
};

class CTFReaderLut3DElt : public CTFReaderOpElt, public CTFArrayMgt, public CTFIndexMapMgt
{
public:
    CTFReaderLut3DElt();

    ~CTFReaderLut3DElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const Lut3DOpDataRcPtr & getLut() const
    {
        return m_lut;
    }

    ArrayBase * updateDimension(const Dimensions & dims) override;

    void endArray(unsigned int position) override;

    IndexMapping * updateDimensionIM(const DimensionsIM & dims) override;

    void endIndexMap(unsigned int position) override;

protected:
    Lut3DOpDataRcPtr m_lut;
    IndexMapping     m_indexMapping;
};

class CTFReaderLut3DElt_1_7 : public CTFReaderLut3DElt
{
public:
    CTFReaderLut3DElt_1_7() : CTFReaderLut3DElt() {}

    ~CTFReaderLut3DElt_1_7() {}

    void end() override;
};

class CTFReaderMatrixElt : public CTFReaderOpElt, public CTFArrayMgt
{
public:
    CTFReaderMatrixElt();

    ~CTFReaderMatrixElt();

    void end() override;

    const OpDataRcPtr getOp() const override;

    // Get the associated Matrix
    const MatrixOpDataRcPtr & getMatrix() const
    {
        return m_matrix;
    }

    ArrayBase * updateDimension(const Dimensions & dims) override;

    void endArray(unsigned int position) override;

protected:
    // Helper method to convert Matrix data from 1.2 to latest.
    void convert_1_2_to_Latest();

    MatrixOpDataRcPtr & getMatrix()
    {
        return m_matrix;
    }

private:
    MatrixOpDataRcPtr m_matrix;
};

class CTFReaderMatrixElt_1_3 : public CTFReaderMatrixElt
{
public:
    CTFReaderMatrixElt_1_3() : CTFReaderMatrixElt() {}

    ~CTFReaderMatrixElt_1_3() {}

    ArrayBase * updateDimension(const Dimensions & dims) override;

    void endArray(unsigned int position) override;
};

class CTFReaderRangeElt : public CTFReaderOpElt
{
public:
    CTFReaderRangeElt();

    ~CTFReaderRangeElt();

    void end() override;

    const OpDataRcPtr getOp() const override;

    const RangeOpDataRcPtr getRange() const
    {
        return m_range;
    }

protected:
    RangeOpDataRcPtr m_range;
};

class CTFReaderRangeElt_1_7 : public CTFReaderRangeElt
{
public:
    CTFReaderRangeElt_1_7();

    ~CTFReaderRangeElt_1_7();

    void start(const char ** atts) override;

    void end() override;

private:
    bool m_isNoClamp;
};

class CTFReaderRangeValueElt : public XmlReaderPlainElt
{
public:
    CTFReaderRangeValueElt(const std::string & name,
                           ContainerEltRcPtr pParent,
                           unsigned int xmlLineNumber,
                           const std::string & xmlFile);

    ~CTFReaderRangeValueElt();

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;
};

class CTFReaderReferenceElt : public CTFReaderOpElt
{
public:
    CTFReaderReferenceElt();

    ~CTFReaderReferenceElt();

    void start(const char ** atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const ReferenceOpDataRcPtr getReference() const
    {
        return m_reference;
    }

private:
    ReferenceOpDataRcPtr m_reference;
};

class CTFReaderExposureContrastElt : public CTFReaderOpElt
{
public:
    CTFReaderExposureContrastElt();
    ~CTFReaderExposureContrastElt();

    void start(const char ** atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const ExposureContrastOpDataRcPtr getExposureContrast() const
    {
        return m_ec;
    }

private:
    ExposureContrastOpDataRcPtr m_ec;
};

class CTFReaderECParamsElt : public XmlReaderPlainElt
{
public:
    CTFReaderECParamsElt(const std::string & name,
                         ContainerEltRcPtr pParent,
                         unsigned int xmlLineNumber,
                         const std::string & xmlFile);

    ~CTFReaderECParamsElt();

    void start(const char ** atts) override;
    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;
};

class CTFReaderDynamicParamElt : public XmlReaderPlainElt
{
public:
    CTFReaderDynamicParamElt(const std::string & name,
                             ContainerEltRcPtr pParent,
                             unsigned int xmlLineNumber,
                             const std::string & xmlFile);
    ~CTFReaderDynamicParamElt();

    void start(const char ** atts) override;
    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;
};

}
OCIO_NAMESPACE_EXIT

#endif