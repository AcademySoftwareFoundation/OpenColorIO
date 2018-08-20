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

#include "FileTransform.h"
#include "pystring/pystring.h"
#include "Lut1DOp.h"
#include "MatrixOps.h"
#include "ExponentOps.h"
#include <sstream>

#include "iccProfileReader.h"

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

OCIO_NAMESPACE_ENTER
{

    class LocalCachedFile : public CachedFile
    {
    public:
        LocalCachedFile() {};
        ~LocalCachedFile() {};

        // Matrix part
        float mMatrix44[16];

        // Gamma
        float mGammaRGB[4];

        // LUT 1d
        Lut1DRcPtr lut;
    };

    typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;

    class LocalFileFormat : public FileFormat
    {
    public:
        ~LocalFileFormat() {};

        virtual void GetFormatInfo(FormatInfoVec & formatInfoVec) const;

        virtual CachedFileRcPtr Read(
            std::istream & istream,
            const std::string & fileName) const;

        virtual void BuildFileOps(OpRcPtrVec & ops,
            const Config& config,
            const ConstContextRcPtr & context,
            CachedFileRcPtr untypedCachedFile,
            const FileTransform& fileTransform,
            TransformDirection dir) const;

        virtual bool IsBinary() const
        {
            return true;
        }

    private:
        static void ThrowErrorMessage(const std::string & error,
            const std::string & fileName);
    };

    void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
    {
        FormatInfo info;
        info.name = "International Color Consortium profile";
        info.extension = "icc";
        info.capabilities = FORMAT_CAPABILITY_READ;
        formatInfoVec.push_back(info);

        // .icm is also fine
        info.name = "Image Color Matching profile";
        info.extension = "icm";
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

    // Try and load the format
    // Raise an exception if it can't be loaded.
    CachedFileRcPtr LocalFileFormat::Read(
        std::istream & istream,
        const std::string & fileName) const
    {
        SampleICC::IccContent icc;
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

        // TODO: Capture device name and creation date metadata
        // in order to help users select the correct profile.

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
        for (i = 0; i<count; i++) {
            if (!SampleICC::Read32(istream, &icc.mTags[i].mTagInfo.sig, 1)
                || !SampleICC::Read32(istream, &icc.mTags[i].mTagInfo.offset, 1)
                || !SampleICC::Read32(istream, &icc.mTags[i].mTagInfo.size, 1))
            {
                ThrowErrorMessage(
                    "Error loading tag offset table from header.",
                    fileName);
            }
        }

        // Validate
        std::string error;
        if (!icc.Validate(error))
        {
            ThrowErrorMessage(error, fileName);
        }

        LocalCachedFileRcPtr cachedFile
            = LocalCachedFileRcPtr(new LocalCachedFile());

        // Matrix part of the Matrix/TRC Model
        {
            const SampleICC::IccTagXYZ * red =
                dynamic_cast<SampleICC::IccTagXYZ*>(
                    icc.LoadTag(istream, icSigRedColorantTag));
            const SampleICC::IccTagXYZ * green =
                dynamic_cast<SampleICC::IccTagXYZ*>(
                    icc.LoadTag(istream, icSigGreenColorantTag));
            const SampleICC::IccTagXYZ * blue =
                dynamic_cast<SampleICC::IccTagXYZ*>(
                    icc.LoadTag(istream, icSigBlueColorantTag));

            if (!red || !green || !blue)
            {
                ThrowErrorMessage("Illegal matrix tag in ICC profile.",
                    fileName);
            }

            cachedFile->mMatrix44[0] = (float)(*red).GetXYZ().X / 65536.0f;
            cachedFile->mMatrix44[1] = (float)(*green).GetXYZ().X / 65536.0f;
            cachedFile->mMatrix44[2] = (float)(*blue).GetXYZ().X / 65536.0f;
            cachedFile->mMatrix44[3] = 0.0f;

            cachedFile->mMatrix44[4] = (float)(*red).GetXYZ().Y / 65536.0f;
            cachedFile->mMatrix44[5] = (float)(*green).GetXYZ().Y / 65536.0f;
            cachedFile->mMatrix44[6] = (float)(*blue).GetXYZ().Y / 65536.0f;
            cachedFile->mMatrix44[7] = 0.0f;

            cachedFile->mMatrix44[8] = (float)(*red).GetXYZ().Z / 65536.0f;
            cachedFile->mMatrix44[9] = (float)(*green).GetXYZ().Z / 65536.0f;
            cachedFile->mMatrix44[10] = (float)(*blue).GetXYZ().Z / 65536.0f;
            cachedFile->mMatrix44[11] = 0.0f;

            cachedFile->mMatrix44[12] = 0.0f;
            cachedFile->mMatrix44[13] = 0.0f;
            cachedFile->mMatrix44[14] = 0.0f;
            cachedFile->mMatrix44[15] = 1.0f;
        }

        // Extract the "B" Curve part of the Matrix/TRC Model
        const SampleICC::IccTag * redTRC =
            icc.LoadTag(istream, icSigRedTRCTag);
        const SampleICC::IccTag * greenTRC =
            icc.LoadTag(istream, icSigGreenTRCTag);
        const SampleICC::IccTag * blueTRC =
            icc.LoadTag(istream, icSigBlueTRCTag);
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

            const SampleICC::IccTagParametricCurve * red =
                dynamic_cast<const SampleICC::IccTagParametricCurve*>(redTRC);
            const SampleICC::IccTagParametricCurve * green =
                dynamic_cast<const SampleICC::IccTagParametricCurve*>(greenTRC);
            const SampleICC::IccTagParametricCurve * blue =
                dynamic_cast<const SampleICC::IccTagParametricCurve*>(blueTRC);

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
            const SampleICC::IccTagCurve * red =
                dynamic_cast<const SampleICC::IccTagCurve*>(redTRC);
            const SampleICC::IccTagCurve * green =
                dynamic_cast<const SampleICC::IccTagCurve*>(greenTRC);
            const SampleICC::IccTagCurve * blue =
                dynamic_cast<const SampleICC::IccTagCurve*>(blueTRC);

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
                cachedFile->lut = Lut1D::Create();
                cachedFile->lut->luts[0] = red->GetCurve();
                cachedFile->lut->luts[1] = green->GetCurve();
                cachedFile->lut->luts[2] = blue->GetCurve();
            }
        }

        return cachedFile;
    }

    void
    LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
        const Config& /*config*/,
        const ConstContextRcPtr & /*context*/,
        CachedFileRcPtr untypedCachedFile,
        const FileTransform& fileTransform,
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

        TransformDirection newDir = CombineTransformDirections(dir,
            fileTransform.getDirection());
        if (newDir == TRANSFORM_DIR_UNKNOWN)
        {
            std::ostringstream os;
            os << "Cannot build file format transform,";
            os << " unspecified transform direction.";
            throw Exception(os.str().c_str());
        }

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
        static const float D50_to_D65_m44[] = {
            0.955509474537f,  -0.023074829492f, 0.063312392987f, 0.0f,
            -0.028327238868f,  1.00994465504f,  0.021055592145f, 0.0f,
            0.012329273379f,  -0.020536209966f, 1.33072998567f,  0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        // The matrix/TRC transform in the ICC profile converts
        // display device code values to the CIE XYZ based version 
        // of the ICC profile connection space (PCS).
        // So we will adopt this convention as the "forward" direction.
        if (newDir == TRANSFORM_DIR_FORWARD)
        {
            if (cachedFile->lut.get())
            {
                CreateLut1DOp(ops, cachedFile->lut,
                    INTERP_LINEAR, TRANSFORM_DIR_FORWARD);
            }
            else
            {
                CreateExponentOp(ops, cachedFile->mGammaRGB,
                    TRANSFORM_DIR_FORWARD);
            }

            CreateMatrixOp(ops, cachedFile->mMatrix44, TRANSFORM_DIR_FORWARD);

            CreateMatrixOp(ops, D50_to_D65_m44, TRANSFORM_DIR_FORWARD);
        }
        else if (newDir == TRANSFORM_DIR_INVERSE)
        {
            CreateMatrixOp(ops, D50_to_D65_m44, TRANSFORM_DIR_INVERSE);

            // The ICC profile tags form a matrix that converts RGB to CIE XYZ.
            // Invert since we are building a PCS -> device transform.
            CreateMatrixOp(ops, cachedFile->mMatrix44, TRANSFORM_DIR_INVERSE);

            // The LUT / gamma stored in the ICC profile works in
            // the gamma->linear direction.
            if (cachedFile->lut.get())
            {
                CreateLut1DOp(ops, cachedFile->lut,
                    INTERP_LINEAR, TRANSFORM_DIR_INVERSE);
            }
            else
            {
                CreateExponentOp(ops, cachedFile->mGammaRGB,
                    TRANSFORM_DIR_INVERSE);
            }
        }

    }

    FileFormat * CreateFileFormatICC()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "OpBuilders.h"
#include <fstream>

OIIO_ADD_TEST(FileFormatICC, Types)
{
    // Test types used
    OIIO_CHECK_EQUAL(1, sizeof(icUInt8Number));
    OIIO_CHECK_EQUAL(2, sizeof(icUInt16Number));
    OIIO_CHECK_EQUAL(4, sizeof(icUInt32Number));

    OIIO_CHECK_EQUAL(4, sizeof(icInt32Number));

    OIIO_CHECK_EQUAL(4, sizeof(icS15Fixed16Number));
}

OCIO::LocalCachedFileRcPtr LoadICCFile(const std::string & filePath)
{
    // Open the filePath
    std::ifstream filestream;
    filestream.open(filePath.c_str(), std::ios_base::binary);

    std::string root, extension, name;
    OCIO::pystring::os::path::splitext(root, extension, filePath);

    name = OCIO::pystring::os::path::basename(root);

    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(filestream, name);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

void BuildOps(const std::string & filePath,
    OCIO::OpRcPtrVec & fileOps,
    OCIO::TransformDirection dir)
{
    // Create a FileTransform
    OCIO::FileTransformRcPtr pFileTransform
        = OCIO::FileTransform::Create();
    // A transform file does not define any interpolation (contrary to config
    // file), this is to avoid exception when creating the operation.
    pFileTransform->setInterpolation(OCIO::INTERP_LINEAR);
    pFileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    // Create empty Config to use
    OCIO::ConfigRcPtr pConfig = OCIO::Config::Create();

    OCIO::ContextRcPtr pContext = OCIO::Context::Create();

    OCIO::BuildFileOps(fileOps, *(pConfig.get()), pContext,
        *(pFileTransform.get()), dir);
}

#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

OIIO_ADD_TEST(FileFormatICC, TestFile)
{
    OCIO::LocalCachedFileRcPtr iccFile;
    {
        // This example uses a profile with a 1024-entry LUT for the TRC.
        const std::string iccFileName(ocioTestFilesDir +
            std::string("/sRGB_Color_Space_Profile.icm"));
        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(BuildOps(iccFileName, ops,
            OCIO::TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, false));
        OIIO_CHECK_EQUAL(4, ops.size());
        OIIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
        OIIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
        OIIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[2]->getInfo());
        OIIO_CHECK_EQUAL("<InvLut1DOp>", ops[3]->getInfo());

        std::vector<float> v0(4, 0.0f);
        v0[0] = 1.0f;
        std::vector<float> v1(4, 0.0f);
        v1[1] = 1.0f;
        std::vector<float> v2(4, 0.0f);
        v2[2] = 1.0f;
        std::vector<float> v3(4, 0.0f);
        v3[3] = 1.0f;

        std::vector<float> tmp = v0;
        ops[1]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(1.04788959f, tmp[0]);
        OIIO_CHECK_EQUAL(0.0295844227f, tmp[1]);
        OIIO_CHECK_EQUAL(-0.00925218873f, tmp[2]);
        OIIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v1;
        ops[1]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(0.0229206420f, tmp[0]);
        OIIO_CHECK_EQUAL(0.990481913f, tmp[1]);
        OIIO_CHECK_EQUAL(0.0150730424f, tmp[2]);
        OIIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v2;
        ops[1]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(-0.0502183102f, tmp[0]);
        OIIO_CHECK_EQUAL(-0.0170795303f, tmp[1]);
        OIIO_CHECK_EQUAL(0.751668990f, tmp[2]);
        OIIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v3;
        ops[1]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(0.0f, tmp[0]);
        OIIO_CHECK_EQUAL(0.0f, tmp[1]);
        OIIO_CHECK_EQUAL(0.0f, tmp[2]);
        OIIO_CHECK_EQUAL(1.0f, tmp[3]);

        tmp = v0;
        ops[2]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(3.13411215332385f, tmp[0]);
        OIIO_CHECK_EQUAL(-0.978787296139183f, tmp[1]);
        OIIO_CHECK_EQUAL(0.0719830443856949f, tmp[2]);
        OIIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v1;
        ops[2]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(-1.61739245955187f, tmp[0]);
        OIIO_CHECK_EQUAL(1.91627958642662f, tmp[1]);
        OIIO_CHECK_EQUAL(-0.228985850247545f, tmp[2]);
        OIIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v2;
        ops[2]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(-0.49063340456472f, tmp[0]);
        OIIO_CHECK_EQUAL(0.033454714231382f, tmp[1]);
        OIIO_CHECK_EQUAL(1.4053851315845f, tmp[2]);
        OIIO_CHECK_EQUAL(0.0f, tmp[3]);

        tmp = v3;
        ops[2]->apply(&(tmp[0]), 1);
        OIIO_CHECK_EQUAL(0.0f, tmp[0]);
        OIIO_CHECK_EQUAL(0.0f, tmp[1]);
        OIIO_CHECK_EQUAL(0.0f, tmp[2]);
        OIIO_CHECK_EQUAL(1.0f, tmp[3]);

        // Knowing the LUT has 1024 elements
        // and is inverted, verify values for a given index
        // are converted to index * step
        const float error = 1e-5f;
        
        // LUT value at index 200 before inversion
        tmp[0] = 0.0317235067f;
        tmp[1] = 0.0317235067f;
        tmp[2] = 0.0317235067f;
        ops[3]->apply(&(tmp[0]), 1);
        OIIO_CHECK_CLOSE(200.0f / 1023.0f, tmp[0], error);
        OIIO_CHECK_CLOSE(200.0f / 1023.0f, tmp[1], error);
        OIIO_CHECK_CLOSE(200.0f / 1023.0f, tmp[2], error);


        // Get cached file to access LUT size
        OIIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OIIO_CHECK_ASSERT((bool)iccFile);
        OIIO_CHECK_ASSERT((bool)(iccFile->lut));

        OIIO_CHECK_EQUAL(0.0f, iccFile->lut->from_min[0]);
        OIIO_CHECK_EQUAL(1.0f, iccFile->lut->from_max[0]);

        OIIO_CHECK_EQUAL(1024, iccFile->lut->luts[0].size());
        OIIO_CHECK_EQUAL(1024, iccFile->lut->luts[1].size());
        OIIO_CHECK_EQUAL(1024, iccFile->lut->luts[2].size());

        OIIO_CHECK_EQUAL(0.0317235067f, iccFile->lut->luts[0][200]);
        OIIO_CHECK_EQUAL(0.0317235067f, iccFile->lut->luts[1][200]);
        OIIO_CHECK_EQUAL(0.0317235067f, iccFile->lut->luts[2][200]);
    }

    {
        // This test uses a profile where the TRC is a 1-entry curve,
        // to be interpreted as a gamma value.
        const std::string iccFileName(ocioTestFilesDir +
            std::string("/AdobeRGB1998.icc"));
        OIIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OIIO_CHECK_ASSERT((bool)iccFile);
        OIIO_CHECK_ASSERT(!(bool)(iccFile->lut));

        OIIO_CHECK_EQUAL(0.609741211f, iccFile->mMatrix44[0]);
        OIIO_CHECK_EQUAL(0.205276489f, iccFile->mMatrix44[1]);
        OIIO_CHECK_EQUAL(0.149185181f, iccFile->mMatrix44[2]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[3]);

        OIIO_CHECK_EQUAL(0.311111450f, iccFile->mMatrix44[4]);
        OIIO_CHECK_EQUAL(0.625671387f, iccFile->mMatrix44[5]);
        OIIO_CHECK_EQUAL(0.0632171631f, iccFile->mMatrix44[6]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[7]);
        
        OIIO_CHECK_EQUAL(0.0194702148f, iccFile->mMatrix44[8]);
        OIIO_CHECK_EQUAL(0.0608673096f, iccFile->mMatrix44[9]);
        OIIO_CHECK_EQUAL(0.744567871f, iccFile->mMatrix44[10]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[11]);

        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[12]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[13]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[14]);
        OIIO_CHECK_EQUAL(1.0f, iccFile->mMatrix44[15]);

        OIIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[0]);
        OIIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[1]);
        OIIO_CHECK_EQUAL(2.19921875f, iccFile->mGammaRGB[2]);
        OIIO_CHECK_EQUAL(1.0f, iccFile->mGammaRGB[3]);
    }

    {
        // This test uses a profile where the TRC is 
        // a parametric curve of type 0 (a single gamma value).
        const std::string iccFileName(ocioTestFilesDir +
            std::string("/LM-1760W.icc"));
        OIIO_CHECK_NO_THROW(iccFile = LoadICCFile(iccFileName));

        OIIO_CHECK_ASSERT((bool)iccFile);
        OIIO_CHECK_ASSERT(!(bool)(iccFile->lut));

        OIIO_CHECK_EQUAL(0.504470825f, iccFile->mMatrix44[0]);
        OIIO_CHECK_EQUAL(0.328125000f, iccFile->mMatrix44[1]);
        OIIO_CHECK_EQUAL(0.131607056f, iccFile->mMatrix44[2]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[3]);

        OIIO_CHECK_EQUAL(0.264923096f, iccFile->mMatrix44[4]);
        OIIO_CHECK_EQUAL(0.682678223f, iccFile->mMatrix44[5]);
        OIIO_CHECK_EQUAL(0.0523834229f, iccFile->mMatrix44[6]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[7]);

        OIIO_CHECK_EQUAL(0.0144805908f, iccFile->mMatrix44[8]);
        OIIO_CHECK_EQUAL(0.0871734619f, iccFile->mMatrix44[9]);
        OIIO_CHECK_EQUAL(0.723556519f, iccFile->mMatrix44[10]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[11]);

        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[12]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[13]);
        OIIO_CHECK_EQUAL(0.0f, iccFile->mMatrix44[14]);
        OIIO_CHECK_EQUAL(1.0f, iccFile->mMatrix44[15]);

        OIIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[0]);
        OIIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[1]);
        OIIO_CHECK_EQUAL(2.17384338f, iccFile->mGammaRGB[2]);
        OIIO_CHECK_EQUAL(1.0f, iccFile->mGammaRGB[3]);
    }
}

OIIO_ADD_TEST(FileFormatICC, TestApply)
{
    {
        const std::string iccFileName(ocioTestFilesDir +
            std::string("/sRGB_Color_Space_Profile.icm"));
        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(BuildOps(iccFileName, ops,
            OCIO::TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, true));

        // apply ops
        float srcImage[] = {
            -0.1f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.9f, 1.0f };

        const float dstImage[] = {
            0.013221f, 0.005287f, 0.069636f, 0.0f,
            0.188847f, 0.204323f, 0.330955f, 0.5f,
            0.722887f, 0.882591f, 1.078655f, 1.0f };
        const float error = 1e-5f;

        OCIO::OpRcPtrVec::size_type numOps = ops.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            ops[i]->apply(srcImage, 3);
        }

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
        }

        // inverse
        OCIO::OpRcPtrVec opsInv;
        OIIO_CHECK_NO_THROW(BuildOps(iccFileName, opsInv,
            OCIO::TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(opsInv, true));

        numOps = opsInv.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            opsInv[i]->apply(srcImage, 3);
        }

        const float bckImage[] = {
            // neg values are clamped by the LUT and won't round-trip
            0.0f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            // >1 values are clamped by the LUT and won't round-trip
            0.7f, 1.0f, 1.0f, 1.0f };

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(srcImage[i], bckImage[i], error);
        }
    }

    {
        const std::string iccFileName(ocioTestFilesDir +
            std::string("/LM-1760W.icc"));
        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(BuildOps(iccFileName, ops,
            OCIO::TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, true));

        // apply ops
        float srcImage[] = {
            -0.1f, 0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.9f, 1.0f };

        const float dstImage[] = {
            0.012437f, 0.004702f, 0.070333f, 0.0f,
            0.188392f, 0.206965f, 0.343595f, 0.5f,
            1.210458f, 1.058761f, 4.003706f, 1.0f };
        const float error = 1e-5f;
        const float error2 = 1e-4f;

        OCIO::OpRcPtrVec::size_type numOps = ops.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            ops[i]->apply(srcImage, 3);
        }

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(srcImage[i], dstImage[i], error);
        }

        // inverse
        OCIO::OpRcPtrVec opsInv;
        OIIO_CHECK_NO_THROW(BuildOps(iccFileName, opsInv,
            OCIO::TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(opsInv, true));

        numOps = opsInv.size();
        for (OCIO::OpRcPtrVec::size_type i = 0; i < numOps; ++i)
        {
            opsInv[i]->apply(srcImage, 3);
        }

        const float bckImage[] = {
            // neg values are clamped by the LUT and won't round-trip
            0.0f, 0.000078f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f, 0.5f,
            0.7f, 1.0f, 1.9f, 1.0f };

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(srcImage[i], bckImage[i], error2);
        }
    }

}

OIIO_ADD_TEST(FileFormatICC, Endian)
{
    unsigned char test[8];
    test[0] = 0x11;
    test[1] = 0x22;
    test[2] = 0x33;
    test[3] = 0x44;
    test[4] = 0x55;
    test[5] = 0x66;
    test[6] = 0x77;
    test[7] = 0x88;

    SampleICC::Swap32Array((void*)test, 2);

    OIIO_CHECK_EQUAL(test[0], 0x44);
    OIIO_CHECK_EQUAL(test[1], 0x33);
    OIIO_CHECK_EQUAL(test[2], 0x22);
    OIIO_CHECK_EQUAL(test[3], 0x11);

    OIIO_CHECK_EQUAL(test[4], 0x88);
    OIIO_CHECK_EQUAL(test[5], 0x77);
    OIIO_CHECK_EQUAL(test[6], 0x66);
    OIIO_CHECK_EQUAL(test[7], 0x55);

    SampleICC::Swap16Array((void*)test, 4);

    OIIO_CHECK_EQUAL(test[0], 0x33);
    OIIO_CHECK_EQUAL(test[1], 0x44);

    OIIO_CHECK_EQUAL(test[2], 0x11);
    OIIO_CHECK_EQUAL(test[3], 0x22);

    OIIO_CHECK_EQUAL(test[4], 0x77);
    OIIO_CHECK_EQUAL(test[5], 0x88);

    OIIO_CHECK_EQUAL(test[6], 0x55);
    OIIO_CHECK_EQUAL(test[7], 0x66);

    SampleICC::Swap64Array((void*)test, 1);

    OIIO_CHECK_EQUAL(test[7], 0x33);
    OIIO_CHECK_EQUAL(test[6], 0x44);
    OIIO_CHECK_EQUAL(test[5], 0x11);
    OIIO_CHECK_EQUAL(test[4], 0x22);
    OIIO_CHECK_EQUAL(test[3], 0x77);
    OIIO_CHECK_EQUAL(test[2], 0x88);
    OIIO_CHECK_EQUAL(test[1], 0x55);
    OIIO_CHECK_EQUAL(test[0], 0x66);

}

#endif // OCIO_UNIT_TEST
