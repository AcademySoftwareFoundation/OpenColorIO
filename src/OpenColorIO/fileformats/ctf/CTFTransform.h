// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FILEFORMATS_CTF_CTFTRANSFORM_H
#define INCLUDED_OCIO_FILEFORMATS_CTF_CTFTRANSFORM_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FormatMetadata.h"
#include "fileformats/xmlutils/XMLWriterUtils.h"
#include "Op.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

class CTFVersion
{
public:
    // Will throw if versionString is not formatted like a version.
    static void ReadVersion(const std::string & versionString,
                            CTFVersion & versionOut);

    CTFVersion()
        : m_major(0)
        , m_minor(0)
        , m_revision(0)
    {
    }
    CTFVersion(unsigned int major, unsigned int minor, unsigned int revision)
        : m_major(major)
        , m_minor(minor)
        , m_revision(revision)
    {
    }
    CTFVersion(unsigned int major, unsigned int minor)
        : m_major(major)
        , m_minor(minor)
        , m_revision(0)
    {
    }

    CTFVersion(const CTFVersion & otherVersion)
        : m_major(otherVersion.m_major)
        , m_minor(otherVersion.m_minor)
        , m_revision(otherVersion.m_revision)
    {
    }

    CTFVersion & operator=(const CTFVersion & rhs);

    bool operator<(const CTFVersion & rhs) const;
    bool operator>(const CTFVersion & rhs) const;
    bool operator<=(const CTFVersion & rhs) const;
    bool operator>=(const CTFVersion & rhs) const;
    bool operator==(const CTFVersion & rhs) const;

    ~CTFVersion() {}

    friend std::ostream & operator<< (std::ostream & stream,
                                      const CTFVersion & rhs)
    {
        stream << rhs.m_major;
        if (rhs.m_minor != 0 || rhs.m_revision != 0)
        {
            stream << "." << rhs.m_minor;
            if (rhs.m_revision != 0)
            {
                stream << "." << rhs.m_revision;
            }
        }
        return stream;
    }

private:
    unsigned int m_major;
    unsigned int m_minor;
    unsigned int m_revision;
};

//
// Process List Versions.
//

// Version 1.2 2012 initial Autodesk version.
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_2 = CTFVersion(1, 2);

// Version 1.3 2012-12 revised matrix.
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_3 = CTFVersion(1, 3);

// Version 1.4 2013-07 adds ACES v0.2.
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_4 = CTFVersion(1, 4);

// Version 1.5 2014-01 adds ACES v0.7.
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_5 = CTFVersion(1, 5);

// Version 1.6 2014-05 adds invLut3D.
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_6 = CTFVersion(1, 6);

// Version 1.7 2015-01 adds 'invert' flag to referenceOp and to the transform,
// adds 1.0 styles to ACES op, adds CLF v2 support (IndexMap, alt. Range, CDL styles).
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_7 = CTFVersion(1, 7);

// Version 1.8 2017-10 adds Function op as a valid element in CTF files.
static const CTFVersion CTF_PROCESS_LIST_VERSION_1_8 = CTFVersion(1, 8);

// TODO: Version 2.0 (TBD) sync with OCIO and incorporate CLF v3 changes.
static const CTFVersion CTF_PROCESS_LIST_VERSION_2_0 = CTFVersion(2, 0);

// Add new version before this line
// and do not forget to update the following line.
static const CTFVersion CTF_PROCESS_LIST_VERSION = CTF_PROCESS_LIST_VERSION_2_0;


// Version 1.0 initial Autodesk version for InfoElt.
#define CTF_INFO_ELEMENT_VERSION_1_0 1.0f

// Version 2.0 Flame 2017 Ext1 InfoElt.
#define CTF_INFO_ELEMENT_VERSION_2_0 2.0f

// .. Add new version for InfoElt before this line
//    and do not forget to update the following line.

#define CTF_INFO_ELEMENT_VERSION CTF_INFO_ELEMENT_VERSION_2_0


class CTFReaderTransform
{
public:
    CTFReaderTransform();
    CTFReaderTransform(const OpRcPtrVec & ops, const FormatMetadataImpl & metadata);

    ~CTFReaderTransform()
    {
    }

    const std::string & getID() const
    {
        return m_id;
    }
    void setID(const char * id)
    {
        m_id = id;
    }
    const std::string & getName() const
    {
        return m_name;
    }
    void setName(const char * name)
    {
        m_name = name;
    }
    const std::string & getInverseOfId() const
    {
        return m_inverseOfId;
    }
    void setInverseOfId(const char * id)
    {
        m_inverseOfId = id;
    }
    FormatMetadataImpl & getInfoMetadata()
    {
        return m_infoMetadata;
    }
    const FormatMetadataImpl & getInfoMetadata() const
    {
        return m_infoMetadata;
    }
    const ConstOpDataVec & getOps() const
    {
        return m_ops;
    }
    ConstOpDataVec & getOps()
    {
        return m_ops;
    }
    const StringUtils::StringVec & getDescriptions() const
    {
        return m_descriptions;
    }
    StringUtils::StringVec & getDescriptions()
    {
        return m_descriptions;
    }

    const std::string & getInputDescriptor() const
    {
        return m_inDescriptor;
    }

    void setInputDescriptor(const std::string & in)
    {
        m_inDescriptor = in;
    }

    const std::string & getOutputDescriptor() const
    {
        return m_outDescriptor;
    }

    void setOutputDescriptor(const std::string & out)
    {
        m_outDescriptor = out;
    }

    void setCTFVersion(const CTFVersion & ver);
    void setCLFVersion(const CTFVersion & ver);

    const CTFVersion & getCTFVersion() const;
    const CTFVersion & getCLFVersion() const;
    bool isCLF() const;

    void fromMetadata(const FormatMetadataImpl & metadata);
    void toMetadata(FormatMetadataImpl & metadata) const;

    // Helper methods to keep the output bit-depth of the Op currently parsed.
    void setPreviousOutBitDepth(BitDepth out)
    {
        m_prevOutBD = out;
    }
    BitDepth getPreviousOutBitDepth() const
    {
        return m_prevOutBD;
    }

private:
    std::string m_id;
    std::string m_name;
    std::string m_inverseOfId;
    std::string m_inDescriptor;
    std::string m_outDescriptor;
    FormatMetadataImpl m_infoMetadata;

    ConstOpDataVec m_ops;
    StringUtils::StringVec m_descriptions;

    // CTF version used even for CLF files.
    // CLF versions <= 2.0 are interpreted as CTF version 1.7.
    CTFVersion m_version;

    // Original CLF version (for reference).
    CTFVersion m_versionCLF;

    // Track bit-depth of ops when loading a CTFTransform.
    BitDepth m_prevOutBD = BIT_DEPTH_UNKNOWN;
};

typedef OCIO_SHARED_PTR<CTFReaderTransform> CTFReaderTransformPtr;
typedef OCIO_SHARED_PTR<const CTFReaderTransform> ConstCTFReaderTransformPtr;

class TransformWriter : public XmlElementWriter
{
public:
    TransformWriter() = delete;
    TransformWriter(const TransformWriter &) = delete;
    TransformWriter& operator=(const TransformWriter &) = delete;

    TransformWriter(XmlFormatter & formatter,
                    ConstCTFReaderTransformPtr transform,
                    bool isCLF);

    virtual ~TransformWriter();

    void write() const override;

private:
    void writeProcessListMetadata(const FormatMetadataImpl & m) const;
    void writeOpMetadata(const FormatMetadataImpl & m) const;
    void writeOps(const CTFVersion & version) const;

private:
    ConstCTFReaderTransformPtr m_transform;
    bool                       m_isCLF;
};


// Helper function to extract the values of FormatMetadata elements with a
// given name. Used to get Description values.
void GetElementsValues(const FormatMetadataImpl::Elements & elements,
                       const std::string & name, 
                       StringUtils::StringVec & values);

} // namespace OCIO_NAMESPACE

#endif
