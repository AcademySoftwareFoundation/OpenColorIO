// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>
#include <fstream>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "iccProfileReader.h"
#include "ops/gamma/GammaOp.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/matrix/MatrixOp.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"


/*
Support for ICC profiles.
ICC color management is the de facto standard in areas such as printing
and OS-level color management.
ICC profiles are a widely used method of storing color information for
computer displays and that is the main purpose of this format reader.
The "matrix/TRC" model for a monitor is parsed and converted into
an OCIO compatible form.
Other types of ICC profiles are not currently supported in this reader.
*/

namespace OCIO_NAMESPACE
{

class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile() = default;
    ~LocalCachedFile() = default;

    // The profile description.
    std::string mProfileDescription;

    // Matrix part
    double mMatrix44[16]{ 0.0 };

    // Gamma
    float mGammaRGB[4]{ 1.0f };

    // 1D LUT
    Lut1DOpDataRcPtr lut;
};

typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;

class LocalFileFormat : public FileFormat
{
public:
    LocalFileFormat() = default;
    ~LocalFileFormat() = default;

    void getFormatInfo(FormatInfoVec & formatInfoVec) const override;

    // Only reads the information data of the file.
    static LocalCachedFileRcPtr ReadInfo(std::istream & istream, 
                                         const std::string & fileName,
                                         SampleICC::IccContent & icc);

    CachedFileRcPtr read(std::istream & istream,
                         const std::string & fileName,
                         Interpolation interp) const override;

    void buildFileOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        CachedFileRcPtr untypedCachedFile,
                        const FileTransform & fileTransform,
                        TransformDirection dir) const override;

    bool isBinary() const override
    {
        return true;
    }

private:
    static void ThrowErrorMessage(const std::string & error, const std::string & fileName);
};

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "International Color Consortium profile";
    info.extension = "icc";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);

    // .icm and .pf file extensions are also fine.

    info.name = "Image Color Matching profile";
    info.extension = "icm";
    formatInfoVec.push_back(info);

    info.name = "ICC profile";
    info.extension = "pf";
    formatInfoVec.push_back(info);
}

void LocalFileFormat::ThrowErrorMessage(const std::string & error,
                                        const std::string & fileName)
{
    std::ostringstream os;
    os << "Error parsing .icc file (";
    os << fileName;
    os << ").  ";
    os << error;

    throw Exception(os.str().c_str());
}

LocalCachedFileRcPtr LocalFileFormat::ReadInfo(std::istream & istream, 
                                               const std::string & fileName,
                                               SampleICC::IccContent & icc)
{
    istream.seekg(0);
    if (!istream.good()
        || !SampleICC::Read32(istream, &icc.mHeader.size, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.cmmId, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.version, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.deviceClass, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.colorSpace, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.pcs, 1)
        || !SampleICC::Read16(istream, &icc.mHeader.date.year, 1)
        || !SampleICC::Read16(istream, &icc.mHeader.date.month, 1)
        || !SampleICC::Read16(istream, &icc.mHeader.date.day, 1)
        || !SampleICC::Read16(istream, &icc.mHeader.date.hours, 1)
        || !SampleICC::Read16(istream, &icc.mHeader.date.minutes, 1)
        || !SampleICC::Read16(istream, &icc.mHeader.date.seconds, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.magic, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.platform, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.flags, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.manufacturer, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.model, 1)
        || !SampleICC::Read64(istream, &icc.mHeader.attributes, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.renderingIntent, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.illuminant.X, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.illuminant.Y, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.illuminant.Z, 1)
        || !SampleICC::Read32(istream, &icc.mHeader.creator, 1)
        || (SampleICC::Read8(istream, 
            &icc.mHeader.profileID,
            sizeof(icc.mHeader.profileID))
                != sizeof(icc.mHeader.profileID))
        || (SampleICC::Read8(istream,
            &icc.mHeader.reserved[0],
            sizeof(icc.mHeader.reserved))
                != sizeof(icc.mHeader.reserved))
        )
    {
        ThrowErrorMessage("Error loading header.", fileName);
    }

    if (icc.mHeader.magic != icMagicNumber)
    {
        ThrowErrorMessage("Wrong magic number.", fileName);
    }

    icUInt32Number count, i;

    if (!SampleICC::Read32(istream, &count, 1))
    {
        ThrowErrorMessage("Error loading number of tags.", fileName);
    }

    icc.mTags.resize(count);

    // Read Tag offset table. 
    for (i = 0; i<count; i++)
    {
        if (!SampleICC::Read32(istream, &icc.mTags[i].mTagInfo.sig, 1)
            || !SampleICC::Read32(istream, &icc.mTags[i].mTagInfo.offset, 1)
            || !SampleICC::Read32(istream, &icc.mTags[i].mTagInfo.size, 1))
        {
            ThrowErrorMessage("Error loading tag offset table from header.", fileName);
        }
    }

    // Validate
    std::string error;
    if (!icc.Validate(error))
    {
        ThrowErrorMessage(error, fileName);
    }

    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    // Get the profile description.
    {
        // First try the Apple private 'dscm' tag, which tends to have more accurate descriptions in
        // Apple profiles. Fall back to the standard 'desc' tag if 'dscm' is not present.

        SampleICC::IccTypeReader * reader = icc.LoadTag(istream, icSigProfileDescriptionMLTag);
        if (!reader)
        {
            reader = icc.LoadTag(istream, icSigProfileDescriptionTag);
        }

        if (!reader)
        {
            // The tags are missing.
            cachedFile->mProfileDescription = "";
            return cachedFile;
        }

        const SampleICC::IccTextDescriptionTypeReader * desc =
            dynamic_cast<const SampleICC::IccTextDescriptionTypeReader *>(reader);
        if (desc)
        {
            cachedFile->mProfileDescription = desc->GetText();
        }
        else
        {
            // The profile description implementation is a list of localized unicode strings. But
            // the OCIO implementation only returns the english string.
            const SampleICC::IccMultiLocalizedUnicodeTypeReader * desc = 
                dynamic_cast<const SampleICC::IccMultiLocalizedUnicodeTypeReader *>(reader);
            if (desc)
            {
                cachedFile->mProfileDescription = desc->GetText();
            }
            else
            {
                ThrowErrorMessage("The 'desc' (or 'dcsm') reader is missing.", fileName);
            }
        }
    }

    return cachedFile;
}

// Try and load the format
// Raise an exception if it can't be loaded.
CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation /*interp*/) const
{
    SampleICC::IccContent icc;
    LocalCachedFileRcPtr cachedFile = ReadInfo(istream, fileName, icc);

    // Matrix part of the Matrix/TRC Model
    {
        const SampleICC::IccXYZArrayTypeReader * red =
            dynamic_cast<SampleICC::IccXYZArrayTypeReader*>(
                icc.LoadTag(istream, icSigRedColorantTag));
        const SampleICC::IccXYZArrayTypeReader * green =
            dynamic_cast<SampleICC::IccXYZArrayTypeReader*>(
                icc.LoadTag(istream, icSigGreenColorantTag));
        const SampleICC::IccXYZArrayTypeReader * blue =
            dynamic_cast<SampleICC::IccXYZArrayTypeReader*>(
                icc.LoadTag(istream, icSigBlueColorantTag));

        if (!red || !green || !blue)
        {
            ThrowErrorMessage("Illegal matrix tag in ICC profile.",
                fileName);
        }

        cachedFile->mMatrix44[0] =  (double)(*red).GetXYZ().X / 65536.0;
        cachedFile->mMatrix44[1] =  (double)(*green).GetXYZ().X / 65536.0;
        cachedFile->mMatrix44[2] =  (double)(*blue).GetXYZ().X / 65536.0;
        cachedFile->mMatrix44[3] =  0.0;

        cachedFile->mMatrix44[4] =  (double)(*red).GetXYZ().Y / 65536.0;
        cachedFile->mMatrix44[5] =  (double)(*green).GetXYZ().Y / 65536.0;
        cachedFile->mMatrix44[6] =  (double)(*blue).GetXYZ().Y / 65536.0;
        cachedFile->mMatrix44[7] =  0.0;

        cachedFile->mMatrix44[8] =  (double)(*red).GetXYZ().Z / 65536.0;
        cachedFile->mMatrix44[9] =  (double)(*green).GetXYZ().Z / 65536.0;
        cachedFile->mMatrix44[10] = (double)(*blue).GetXYZ().Z / 65536.0;
        cachedFile->mMatrix44[11] = 0.0;

        cachedFile->mMatrix44[12] = 0.0;
        cachedFile->mMatrix44[13] = 0.0;
        cachedFile->mMatrix44[14] = 0.0;
        cachedFile->mMatrix44[15] = 1.0;
    }

    // Extract the "B" Curve part of the Matrix/TRC Model
    const SampleICC::IccTypeReader * redTRC   = icc.LoadTag(istream, icSigRedTRCTag);
    const SampleICC::IccTypeReader * greenTRC = icc.LoadTag(istream, icSigGreenTRCTag);
    const SampleICC::IccTypeReader * blueTRC  = icc.LoadTag(istream, icSigBlueTRCTag);
    if (!redTRC || !greenTRC || !blueTRC)
    {
        ThrowErrorMessage("Illegal curve tag in ICC profile.", fileName);
    }

    static const std::string strSameType(
        "All curves in the ICC profile must be of the same type.");
    if (redTRC->IsParametricCurve())
    {
        if (!greenTRC->IsParametricCurve() || !blueTRC->IsParametricCurve())
        {
            ThrowErrorMessage(strSameType, fileName);
        }

        const SampleICC::IccParametricCurveTypeReader * red =
            dynamic_cast<const SampleICC::IccParametricCurveTypeReader*>(redTRC);
        const SampleICC::IccParametricCurveTypeReader * green =
            dynamic_cast<const SampleICC::IccParametricCurveTypeReader*>(greenTRC);
        const SampleICC::IccParametricCurveTypeReader * blue =
            dynamic_cast<const SampleICC::IccParametricCurveTypeReader*>(blueTRC);

        if (!red || !green || !blue)
        {
            ThrowErrorMessage(strSameType, fileName);
        }

        if (red->GetNumParam() != 1
            || green->GetNumParam() != 1
            || blue->GetNumParam() != 1)
        {
            ThrowErrorMessage(
                "Expecting 1 param in parametric curve tag of ICC profile.",
                fileName);
        }

        cachedFile->mGammaRGB[0] = SampleICC::icFtoD(red->GetParam()[0]);
        cachedFile->mGammaRGB[1] = SampleICC::icFtoD(green->GetParam()[0]);
        cachedFile->mGammaRGB[2] = SampleICC::icFtoD(blue->GetParam()[0]);
        cachedFile->mGammaRGB[3] = 1.0f;
    }
    else
    {
        if (greenTRC->IsParametricCurve() || blueTRC->IsParametricCurve())
        {
            ThrowErrorMessage(strSameType, fileName);
        }
        const SampleICC::IccCurveTypeReader * red =
            dynamic_cast<const SampleICC::IccCurveTypeReader*>(redTRC);
        const SampleICC::IccCurveTypeReader * green =
            dynamic_cast<const SampleICC::IccCurveTypeReader*>(greenTRC);
        const SampleICC::IccCurveTypeReader * blue =
            dynamic_cast<const SampleICC::IccCurveTypeReader*>(blueTRC);

        if (!red || !green || !blue)
        {
            ThrowErrorMessage(strSameType, fileName);
        }

        const size_t curveSize = red->GetCurve().size();
        if (green->GetCurve().size() != curveSize
            || blue->GetCurve().size() != curveSize)
        {
            ThrowErrorMessage(
                "All curves in the ICC profile must be of the same length.",
                fileName);
        }

        if (0 == curveSize)
        {
            ThrowErrorMessage("Curves with no values in ICC profile.",
                fileName);
        }
        else if (1 == curveSize)
        {
            // The curve value shall be interpreted as a gamma value.
            //
            // In this case, the 16-bit curve value is to be interpreted as
            // an unsigned fixed-point 8.8 number.
            // (But we want to multiply by 65535 to undo the normalization
            // applied by SampleICC)
            cachedFile->mGammaRGB[0] =
                red->GetCurve()[0] * 65535.0f / 256.0f;
            cachedFile->mGammaRGB[1] =
                green->GetCurve()[0] * 65535.0f / 256.0f;
            cachedFile->mGammaRGB[2] =
                blue->GetCurve()[0] * 65535.0f / 256.0f;
            cachedFile->mGammaRGB[3] = 1.0f;

        }
        else
        {
            // The LUT stored in the profile takes gamma-corrected values
            // and linearizes them.
            // The entries are encoded as 16-bit ints that may be
            // normalized by 65535 to interpret them as [0,1].
            // The LUT will be inverted to convert output-linear values
            // into values that may be sent to the display.
            const auto lutLength = static_cast<unsigned long>(curveSize);
            cachedFile->lut = std::make_shared<Lut1DOpData>(lutLength);

            const auto & rc = red->GetCurve();
            const auto & gc = green->GetCurve();
            const auto & bc = blue->GetCurve();

            auto & lutData = cachedFile->lut->getArray();

            for (unsigned long i = 0; i < lutLength; ++i)
            {
                lutData[i * 3 + 0] = rc[i];
                lutData[i * 3 + 1] = gc[i];
                lutData[i * 3 + 2] = bc[i];
            }

            // Set the file bit-depth based on what is in the ICC profile
            // (even though SampleICC has normalized the values).
            cachedFile->lut->setFileOutputBitDepth(BIT_DEPTH_UINT16);
        }
    }

    return cachedFile;
}

void
LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                const Config & /*config*/,
                                const ConstContextRcPtr & /*context*/,
                                CachedFileRcPtr untypedCachedFile,
                                const FileTransform & fileTransform,
                                TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile =
        DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if (!cachedFile)
    {
        std::ostringstream os;
        os << "Cannot build Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    // The matrix in the ICC profile converts monitor RGB to the CIE XYZ
    // based version of the ICC profile connection space (PCS).
    // Because the PCS white point is D50, the ICC profile builder must
    // adapt the native device matrix to D50.
    // The ICC spec recommends a von Kries style chromatic adaptation
    // using the "Bradford" matrix.
    // However for the purposes of OCIO, it is much more convenient
    // for the profile to be balanced to D65 since that is the native
    // white point that most displays will be balanced to.
    // The matrix below is the Bradford matrix to convert
    // a D50 XYZ to a D65 XYZ.
    // In most cases, combining this with the matrix in the ICC profile
    // recovers what would be the actual matrix for a D65 native monitor.
    static constexpr double D50_to_D65_m44[] = {
            0.955509474537, -0.023074829492, 0.063312392987, 0.0,
           -0.028327238868,  1.00994465504,  0.021055592145, 0.0,
            0.012329273379, -0.020536209966, 1.33072998567,  0.0,
            0.0,             0.0,            0.0,            1.0
    };

    const auto fileInterp = fileTransform.getInterpolation();

    Lut1DOpDataRcPtr lut;

    if (cachedFile->lut)
    {
        bool fileInterpUsed = false;
        lut = HandleLUT1D(cachedFile->lut, fileInterp, fileInterpUsed);

        if (!fileInterpUsed)
        {
            LogWarningInterpolationNotUsed(fileInterp, fileTransform);
        }
    }

    // The matrix/TRC transform in the ICC profile converts display device code values to the
    // CIE XYZ based version of the ICC profile connection space (PCS).
    // However, in OCIO the most common use of an ICC monitor profile is as a display color space,
    // and in that usage it is more natural for the XYZ to display code value transform to be called
    // the forward direction.

    switch (newDir)
    {
    case TRANSFORM_DIR_INVERSE:
    {
        // Monitor code value to CIE XYZ.
        if (lut)
        {
            CreateLut1DOp(ops, lut, TRANSFORM_DIR_FORWARD);
        }
        else
        {
            const GammaOpData::Params redParams   = { cachedFile->mGammaRGB[0] };
            const GammaOpData::Params greenParams = { cachedFile->mGammaRGB[1] };
            const GammaOpData::Params blueParams  = { cachedFile->mGammaRGB[2] };
            const GammaOpData::Params alphaParams = { cachedFile->mGammaRGB[3] };
            auto gamma = std::make_shared<GammaOpData>(GammaOpData::BASIC_FWD, 
                                                       redParams,
                                                       greenParams,
                                                       blueParams,
                                                       alphaParams);
            CreateGammaOp(ops, gamma, TRANSFORM_DIR_FORWARD);
        }

        CreateMatrixOp(ops, cachedFile->mMatrix44, TRANSFORM_DIR_FORWARD);

        CreateMatrixOp(ops, D50_to_D65_m44, TRANSFORM_DIR_FORWARD);
        break;
    }
    case TRANSFORM_DIR_FORWARD:
    {
        // CIE XYZ to monitor code value.
        
        CreateMatrixOp(ops, D50_to_D65_m44, TRANSFORM_DIR_INVERSE);

        // The ICC profile tags form a matrix that converts RGB to CIE XYZ.
        // Invert since we are building a PCS -> device transform.
        CreateMatrixOp(ops, cachedFile->mMatrix44, TRANSFORM_DIR_INVERSE);

        // The LUT / gamma stored in the ICC profile works in
        // the gamma->linear direction.
        if (lut)
        {
            CreateLut1DOp(ops, lut, TRANSFORM_DIR_INVERSE);
        }
        else
        {
            const GammaOpData::Params redParams   = { cachedFile->mGammaRGB[0] };
            const GammaOpData::Params greenParams = { cachedFile->mGammaRGB[1] };
            const GammaOpData::Params blueParams  = { cachedFile->mGammaRGB[2] };
            const GammaOpData::Params alphaParams = { cachedFile->mGammaRGB[3] };
            auto gamma = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                       redParams,
                                                       greenParams,
                                                       blueParams,
                                                       alphaParams);

            CreateGammaOp(ops, gamma, TRANSFORM_DIR_FORWARD);
        }
        break;
    }
    }
}

FileFormat * CreateFileFormatICC()
{
    return new LocalFileFormat();
}

std::string GetProfileDescriptionFromICCProfile(const char * ICCProfileFilepath)
{
    std::ifstream filestream(ICCProfileFilepath, std::ios_base::binary);
    if (!filestream.good())
    {
        std::ostringstream os;
        os << "The specified file '";
        os << ICCProfileFilepath << "' could not be opened. ";
        os << "Please confirm the file exists with appropriate read permissions.";
        throw Exception(os.str().c_str());
    }

    SampleICC::IccContent icc;
    LocalCachedFileRcPtr file = LocalFileFormat::ReadInfo(filestream, ICCProfileFilepath, icc);

    std::string desc = file->mProfileDescription;
    if (desc.empty())
    {
        // Fallback to the filename if the description is missing or empty.
        std::string path, filename;
        pystring::os::path::split(path, filename, ICCProfileFilepath);
        desc = filename;
    }

    return desc;
}

} // namespace OCIO_NAMESPACE

