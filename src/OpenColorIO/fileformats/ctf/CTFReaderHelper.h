// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERHELPER_H
#define INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERHELPER_H

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "fileformats/ctf/CTFTransform.h"
#include "fileformats/ctf/IndexMapping.h"
#include "fileformats/FormatMetadata.h"
#include "ops/OpArray.h"
#include "ops/cdl/CDLOpData.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/gamma/GammaOpData.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"
#include "ops/gradingtone/GradingToneOpData.h"
#include "ops/log/LogOpData.h"
#include "ops/log/LogUtils.h"
#include "ops/lut1d/Lut1DOpData.h"
#include "ops/lut3d/Lut3DOpData.h"
#include "ops/range/RangeOpData.h"
#include "ops/reference/ReferenceOpData.h"

namespace OCIO_NAMESPACE
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

    void appendMetadata(const std::string & name, const std::string & value) override;

    // Get its parent which is a Transform.
    const CTFReaderTransformPtr & getTransform() const;

    const char * getTypeName() const override;

    // Set the current transform CTF version.
    void setVersion(const CTFVersion & ver);

    void setCLFVersion(const CTFVersion & ver);

    // Get current xml transform version
    const CTFVersion & getVersion() const;
    const CTFVersion & getCLFVersion() const;

    bool isCLF() const;

private:
    CTFReaderTransformElt() = delete;

    // The associated Transform.
    CTFReaderTransformPtr m_transform;
    // Is it a clf file? Or is a clf parser requested.
    bool m_isCLF;
};

typedef OCIO_SHARED_PTR<CTFReaderTransformElt> CTFReaderTransformEltRcPtr;

class CTFReaderMetadataElt : public XmlReaderComplexElt
{
public:
    CTFReaderMetadataElt() = delete;
    CTFReaderMetadataElt(const std::string & name,
                         ContainerEltRcPtr pParent,
                         unsigned int xmlLineNumber,
                         const std::string & xmlFile);

    virtual ~CTFReaderMetadataElt();

    const std::string& getIdentifier() const override;

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine);

    FormatMetadataImpl & getMetadata() { return m_metadata; }

private:
    FormatMetadataImpl m_metadata;
};

class CTFReaderInfoElt : public CTFReaderMetadataElt
{
public:
    CTFReaderInfoElt() = delete;
    CTFReaderInfoElt(const std::string & name,
                     ContainerEltRcPtr pParent,
                     unsigned int xmlLineNumber,
                     const std::string & xmlFile);

    virtual ~CTFReaderInfoElt();

    void start(const char ** atts) override;

    void end() override;
};

class CTFReaderInputDescriptorElt : public XmlReaderPlainElt
{
public:
    CTFReaderInputDescriptorElt() = delete;
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
};

class CTFReaderOutputDescriptorElt : public XmlReaderPlainElt
{
public:
    CTFReaderOutputDescriptorElt() = delete;
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
};

class CTFReaderArrayElt : public XmlReaderPlainElt
{
public:
    CTFReaderArrayElt() = delete;
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
    CTFReaderIndexMapElt() = delete;
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
    CTFIndexMapMgt() = default;
    virtual ~CTFIndexMapMgt() = default;

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
    bool m_completed = false;
};

class CTFReaderOpElt;
typedef OCIO_SHARED_PTR<CTFReaderOpElt> CTFReaderOpEltRcPtr;

class CTFReaderOpElt : public XmlReaderContainerElt
{
public:
    // Enumeration of all possible reader types.
    enum Type
    {
        CDLType = 0,
        Lut1DType,
        Lut3DType,
        MatrixType,
        RangeType,
        // CTF types.
        ACESType,
        ExposureContrastType,
        FixedFunctionType,
        FunctionType,
        GammaType,
        GradingPrimaryType,
        GradingRGBCurveType,
        GradingToneType,
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

    void appendMetadata(const std::string & name, const std::string & value) override;

    void start(const char ** atts) override;

    void end() override;

    const char * getTypeName() const override;

    CTFReaderTransformPtr getTransform()
    {
        return m_transform;
    }

    // Get the right reader using its type and
    // the xml transform version.
    static CTFReaderOpEltRcPtr GetReader(Type type, const CTFVersion & version, bool isCLF);

protected:
    void validateXmlParameters(const char ** atts) const noexcept;

    // Op that implement a start function using the parent class start function
    // need to implement this function.
    virtual bool isOpParameterValid(const char * att) const noexcept;

    static BitDepth GetBitDepth(const std::string & str);

protected:
    CTFReaderTransformPtr m_transform;  // The parent
    BitDepth m_inBitDepth = BIT_DEPTH_UNKNOWN;
    BitDepth m_outBitDepth = BIT_DEPTH_UNKNOWN;
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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

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
    bool isOpParameterValid(const char * att) const noexcept override;

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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    FixedFunctionOpDataRcPtr m_fixedFunction;
};

class CTFReaderFunctionElt : public CTFReaderOpElt
{
public:
    CTFReaderFunctionElt();
    ~CTFReaderFunctionElt();

    void start(const char **atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const FixedFunctionOpDataRcPtr getFixedFunction() const
    {
        return m_fixedFunction;
    }

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

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

protected:
    virtual bool isValid(const GammaOpData::Style style) const noexcept;
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    GammaOpDataRcPtr m_gamma;
};

class CTFReaderGammaElt_1_5 : public CTFReaderGammaElt
{
public:
    CTFReaderGammaElt_1_5() : CTFReaderGammaElt() {}
    ~CTFReaderGammaElt_1_5() {}

    CTFReaderGammaParamsEltRcPtr createGammaParamsElt(
        const std::string & name,
        ContainerEltRcPtr pParent,
        unsigned int xmlLineNumber,
        const std::string & xmlFile) const override;
};

class CTFReaderGammaElt_CTF_2_0 : public CTFReaderGammaElt_1_5
{
public:
    CTFReaderGammaElt_CTF_2_0() : CTFReaderGammaElt_1_5() {}
    ~CTFReaderGammaElt_CTF_2_0() {}

protected:
    bool isValid(const GammaOpData::Style style) const noexcept override;
};

// Same as CTF, except that alpha channel is not allowed
class CTFReaderGammaElt_CLF_3_0 : public CTFReaderGammaElt_CTF_2_0
{
public:
    CTFReaderGammaElt_CLF_3_0() : CTFReaderGammaElt_CTF_2_0() {}
    ~CTFReaderGammaElt_CLF_3_0() {}

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

class CTFReaderGradingPrimaryElt : public CTFReaderOpElt
{
public:
    CTFReaderGradingPrimaryElt();
    ~CTFReaderGradingPrimaryElt() = default;

    void start(const char ** atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const GradingPrimaryOpDataRcPtr & getGradingPrimary() const
    {
        return m_gradingPrimaryOpData;
    }

    GradingPrimary & getValue() { return m_gradingPrimary; }
    const GradingPrimary & getValue() const { return m_gradingPrimary; }

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    GradingPrimary m_gradingPrimary{ GRADING_LOG };
    GradingPrimaryOpDataRcPtr m_gradingPrimaryOpData;
};

class CTFReaderGradingPrimaryParamElt : public XmlReaderPlainElt
{
public:
    CTFReaderGradingPrimaryParamElt(const std::string & name,
                                    ContainerEltRcPtr pParent,
                                    unsigned int xmlLocation,
                                    const std::string & xmlFile);
    ~CTFReaderGradingPrimaryParamElt();

    void start(const char ** atts);
    void end();

    void setRawData(const char* str, size_t len, unsigned int xmlLine);

private:
    void parseRGBMAttrValues(const char ** atts, GradingRGBM & rgbm) const;

    void parsePivotAttrValues(const char ** atts,
                              double & contrast,
                              double & black,
                              double & white) const;

    void parseBWAttrValues(const char ** atts,
                           double & black,
                           double & white) const;

    void parseScalarAttrValue(const char ** atts,
                              const char * tag,
                              double & value) const;
};

class CTFReaderGradingRGBCurveElt : public CTFReaderOpElt
{
public:
    CTFReaderGradingRGBCurveElt();
    ~CTFReaderGradingRGBCurveElt() = default;

    void start(const char ** atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const GradingRGBCurveOpDataRcPtr & getGradingRGBCurve() const
    {
        return m_gradingRGBCurve;
    }

    // For sub-elements.
    GradingRGBCurveRcPtr & getLoadingRGBCurve() { return m_loadingRGBCurve; }

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    // Editable RGBCurve that will be set to the OpData when parsing of the element is done.
    GradingRGBCurveRcPtr m_loadingRGBCurve;
    GradingRGBCurveOpDataRcPtr m_gradingRGBCurve;
};

class CTFReaderGradingCurveElt : public XmlReaderComplexElt
{
public:
    CTFReaderGradingCurveElt(const std::string & name,
                             ContainerEltRcPtr pParent,
                             unsigned int xmlLocation,
                             const std::string & xmlFile);
    ~CTFReaderGradingCurveElt();

    void start(const char ** atts);
    void end();

    GradingBSplineCurveRcPtr getCurve() { return m_curve; }
private:
    GradingBSplineCurveRcPtr m_curve;
};

class CTFReaderGradingCurvePointsElt : public XmlReaderPlainElt
{
public:
    CTFReaderGradingCurvePointsElt(const std::string & name,
                                   ContainerEltRcPtr pParent,
                                   unsigned int xmlLocation,
                                   const std::string & xmlFile);
    ~CTFReaderGradingCurvePointsElt();

    void start(const char ** atts);
    void end();

    void setRawData(const char* str, size_t len, unsigned int xmlLine);

private:
    std::vector<float> m_data;
};

class CTFReaderGradingCurveSlopesElt : public XmlReaderPlainElt
{
public:
    CTFReaderGradingCurveSlopesElt(const std::string & name,
                                   ContainerEltRcPtr pParent,
                                   unsigned int xmlLocation,
                                   const std::string & xmlFile);
    ~CTFReaderGradingCurveSlopesElt();

    void start(const char ** atts);
    void end();

    void setRawData(const char* str, size_t len, unsigned int xmlLine);

private:
    std::vector<float> m_data;
};

class CTFReaderGradingToneElt : public CTFReaderOpElt
{
public:
    CTFReaderGradingToneElt();
    ~CTFReaderGradingToneElt() = default;

    void start(const char ** atts) override;
    void end() override;

    const OpDataRcPtr getOp() const override;

    const GradingToneOpDataRcPtr & getGradingTone() const
    {
        return m_gradingTone;
    }

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    GradingToneOpDataRcPtr m_gradingTone;
};

class CTFReaderGradingToneParamElt : public XmlReaderPlainElt
{
public:
    CTFReaderGradingToneParamElt(const std::string & name,
                                 ContainerEltRcPtr pParent,
                                 unsigned int xmlLocation,
                                 const std::string & xmlFile);
    ~CTFReaderGradingToneParamElt();

    void start(const char ** atts);
    void end();

    void setRawData(const char* str, size_t len, unsigned int xmlLine);

private:
    void parseRGBMSWAttrValues(const char ** atts, GradingRGBMSW & rgbm,
                               bool center, bool pivot) const;

    void parseScalarAttrValue(const char ** atts,
                              const char * tag,
                              double & value) const;
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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    Lut3DOpDataRcPtr m_invLut;
};


class CTFReaderLogParamsElt;
typedef OCIO_SHARED_PTR<CTFReaderLogParamsElt> CTFReaderLogParamsEltRcPtr;

class CTFReaderLogElt : public CTFReaderOpElt
{
public:
    CTFReaderLogElt();

    ~CTFReaderLogElt();

    void start(const char ** atts) override;

    void end() override;

    const OpDataRcPtr getOp() const override;

    const LogOpDataRcPtr & getLog() const noexcept { return m_log; }

    LogUtil::CTFParams & getCTFParams() noexcept { return m_ctfParams; }
    const LogUtil::CTFParams & getCTFParams() const noexcept { return m_ctfParams; }

    void setBase(double base);
    bool isBaseSet() const noexcept { return m_baseSet; }

    virtual CTFReaderLogParamsEltRcPtr createLogParamsElt(const std::string & name,
                                                          ContainerEltRcPtr pParent,
                                                          unsigned int xmlLineNumber,
                                                          const std::string & xmlFile) const;

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

private:
    LogUtil::CTFParams m_ctfParams;
    bool m_baseSet = false;
    LogOpDataRcPtr m_log;
};

class CTFReaderLogElt_2_0 : public CTFReaderLogElt
{
public:
    CTFReaderLogElt_2_0() : CTFReaderLogElt() {}
    ~CTFReaderLogElt_2_0() {}

    void end() override;

    CTFReaderLogParamsEltRcPtr createLogParamsElt(const std::string & name,
                                                  ContainerEltRcPtr pParent,
                                                  unsigned int xmlLineNumber,
                                                  const std::string & xmlFile) const override;
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

    bool parseCineon(const char ** atts, unsigned i, double & gamma, double & refWhite,
                     double & refBlack, double & highlight, double & shadow);

    void setCineon(LogUtil::CTFParams & legacyParams, int chan, double gamma,
                   double refWhite, double refBlack, double highlight, double shadow);
};

class CTFReaderLogParamsElt_2_0 : public CTFReaderLogParamsElt
{
public:
    CTFReaderLogParamsElt_2_0(const std::string & name,
                              ContainerEltRcPtr pParent,
                              unsigned int xmlLineNumber,
                              const std::string & xmlFile)
        : CTFReaderLogParamsElt(name, pParent, xmlLineNumber, xmlFile) {}
    ~CTFReaderLogParamsElt_2_0() {}

    void start(const char ** atts) override;
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
    bool isOpParameterValid(const char * att) const noexcept override;

    Lut1DOpDataRcPtr m_lut;
    IndexMapping     m_indexMapping;
};

class CTFReaderLut1DElt_1_4 : public CTFReaderLut1DElt
{
public:
    CTFReaderLut1DElt_1_4() : CTFReaderLut1DElt() {}

    ~CTFReaderLut1DElt_1_4() {}

    void start(const char **atts) override;

protected:
    bool isOpParameterValid(const char * att) const noexcept override;
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
    bool isOpParameterValid(const char * att) const noexcept override;

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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

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

protected:
    bool isOpParameterValid(const char * att) const noexcept override;

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

} // namespace OCIO_NAMESPACE

#endif
