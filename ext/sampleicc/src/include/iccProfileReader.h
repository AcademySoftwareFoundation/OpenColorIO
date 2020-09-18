/*
The ICC Software License, Version 0.2


Copyright (c) 2003-2010 The International Color Consortium. All rights
reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

3. In the absence of prior written permission, the names "ICC" and "The
International Color Consortium" must not be used to imply that the
ICC organization endorses or promotes products derived from this
software.


THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE INTERNATIONAL COLOR CONSORTIUM OR
ITS CONTRIBUTING MEMBERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
====================================================================

This software consists of voluntary contributions made by many
individuals on behalf of the The International Color Consortium.


Membership in the ICC is encouraged when this software is used for
commercial purposes.


For more information on The International Color Consortium, please
see <http://www.color.org/>.

*/


/*
This code has been copied and modified from http://sampleicc.sourceforge.net/
This is the minimal amount of code needed to read icc profiles.

Modifications Copyright Contributors to the OpenColorIO Project.
*/


#ifndef INCLUDED_SAMPLEICC_PROFILEREADER_H
#define INCLUDED_SAMPLEICC_PROFILEREADER_H

#include <vector>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>

// Before including http://www.color.org/icProfileHeader.h 
// make sure types will be properly defined.
// There has just been one edit to icProfileHeader.h in order to
// remove a comment within a comment.

#if !defined(_WIN32) // non-PC, perhaps Mac, Linux

#include <stdint.h>

#define ICCUINT32 uint32_t
#define ICCINT32  int32_t
#define ICUINT32TYPE uint32_t
#define ICINT32TYPE  int32_t

#endif

#include "icProfileHeader.h"

namespace SampleICC
{

    void Swap8(icUInt8Number & a, icUInt8Number & b)
    {
        icUInt8Number tmp = a;
        a = b;
        b = tmp;
    }

    void Swap64Array(void *pVoid, icInt32Number num)
    {
        icUInt8Number *ptr = (icUInt8Number*)pVoid;

        while (num > 0)
        {
            Swap8(ptr[0], ptr[7]);
            Swap8(ptr[1], ptr[6]);
            Swap8(ptr[2], ptr[5]);
            Swap8(ptr[3], ptr[4]);
            ptr += 8;
            num--;
        }
    }

    void Swap32Array(void *pVoid, icInt32Number num)
    {
        icUInt8Number *ptr = (icUInt8Number*)pVoid;

        while (num > 0)
        {
            Swap8(ptr[0], ptr[3]);
            Swap8(ptr[1], ptr[2]);
            ptr += 4;
            num--;
        }
    }

    void Swap16Array(void *pVoid, icInt32Number num)
    {
        icUInt8Number *ptr = (icUInt8Number*)pVoid;

        while (num > 0)
        {
            Swap8(ptr[0], ptr[1]);
            ptr += 2;
            num--;
        }
    }

    float icFtoD(icS15Fixed16Number num)
    {
        return (float)((double)num / 65536.0);
    }

    icInt32Number Read8(std::istream & istream, void *pBuf, icInt32Number num)
    {
        if (!istream.good())
            return 0;

        char * pBufChar = (char *)pBuf;
        istream.read(pBufChar, num);

        if (!istream.good())
            return 0;

        return (icInt32Number)num;
    }

    icInt32Number Read64(std::istream & istream, void * pBuf64, icInt32Number num)
    {
        num = Read8(istream, pBuf64, num << 3) >> 3;
        Swap64Array(pBuf64, num);

        return num;
    }

    icInt32Number Read32(std::istream & istream, void * pBuf32, icInt32Number num)
    {
        num = Read8(istream, pBuf32, num << 2) >> 2;
        Swap32Array(pBuf32, num);

        return num;
    }

    icInt32Number Read16(std::istream & istream, void *pBuf16, icInt32Number num)
    {
        num = Read8(istream, pBuf16, num << 1) >> 1;
        Swap16Array(pBuf16, num);

        return num;
    }

    icInt32Number Read16Float(std::istream & istream, void *pBufFloat, icInt32Number num)
    {
        float * ptr = (float*)pBufFloat;
        icUInt16Number tmp;
        icInt32Number i;

        for (i = 0; i < num; ++i)
        {
            if (Read16(istream, &tmp, 1) != 1)
                break;

            *ptr = (float)((float)tmp / 65535.0f);
            ptr++;
        }

        return i;
    }

    class IccTypeReader
    {
    public:
        IccTypeReader() {}
        virtual ~IccTypeReader() {}

        virtual bool Read(std::istream & /* istream */, icUInt32Number /* size */)
        {
            return false;
        }

        virtual bool IsParametricCurve() const { return false; }

        static IccTypeReader * Create(icTagTypeSignature sigType);
    };


    // Note, the textDescriptionType is from the v2 spec (ICC.1:2001-04, pg 60). It is not included
    // in the v4 spec (ICC.1:2010) but it is still found in many v4 profiles.
    class IccTextDescriptionTypeReader : public IccTypeReader
    {
    public:
        IccTextDescriptionTypeReader() = default;
        virtual ~IccTextDescriptionTypeReader() = default;

        virtual bool Read(std::istream & istream, icUInt32Number size)
        {
            mText.clear();

            // Note that tag size include sig that has already been read.

            if (sizeof(icSigProfileDescriptionTag) +
                sizeof(icUInt32Number) + // reserved
                sizeof(icUInt32Number) > size)
            {
                return false;
            }

            if (!istream.good())
            {
                return false;
            }

            icUInt32Number reserved; // reserved
            if (!Read32(istream, &reserved, 1))
            {
                return false;
            }

            // Read the size of the string.

            icUInt32Number textSize = 0;
            if (!Read32(istream, &textSize, 1))
            {
                return false;
            }

            // Read the string itself.            

            if (textSize)
            {
                mText.resize(textSize + 1);

                // Completed with '\0' if the string is smaller than the expected size.
                const icUInt32Number readTextSize = Read8(istream, (void*)mText.c_str(), textSize);
                if (readTextSize != textSize) 
                {
                    mText.clear();
                    return false;
                }
                else
                {
                    // Removes extra '\0' if any.
                    const std::string::size_type pos = mText.find('\0');
                    if (pos != std::string::npos)
                    {
                        mText.resize(pos);
                    }
                }
            }

            return true;
        }

        inline const std::string & GetText() const { return mText; }

    private:
        std::string mText;
    };

    // Custom multi localized unicode reader to only find the one string following
    // the heuristic described below i.e. flavor the USA / English string.
    class IccMultiLocalizedUnicodeTypeReader : public IccTypeReader
    {
    public:
        IccMultiLocalizedUnicodeTypeReader() = default;
        virtual ~IccMultiLocalizedUnicodeTypeReader() = default;

        virtual bool Read(std::istream & istream, icUInt32Number size)
        {
            mText.clear();

            // Note that tag size include sig that has already been read.

            if (sizeof(icTagTypeSignature) + 
                (sizeof(icUInt32Number) * 3) > size)
            {
                return false;
            }

            if (!istream.good())
            {
                return false;
            }

            icUInt32Number reserved, nNumRec, nRecSize;
  
            if (!Read32(istream, &reserved, 1)
                || !Read32(istream, &nNumRec, 1)
                || !Read32(istream, &nRecSize, 1))
            {
                return false;
            }

            // Recognized version name records are 12 bytes each.
            if (nRecSize != 12)
            {
                return false;
            }

            icLanguageCode nLanguageCode;
            icCountryCode nRegionCode;
            icUInt32Number nLength, nOffset;

            // The heuristic for selecting the one string is:
            // 1) US region
            // 2) UK region
            // 3) First EN language
            // 4) First string of any kind
            //
            std::string foundCountryUSA;
            std::string foundContryUK;
            std::string foundLanguageEN;
            std::string foundFirstEntry;

            for (icUInt32Number i = 0; i < nNumRec; ++i)
            {
                if ( (4 * sizeof(icUInt32Number) + (i+1) * 12) > size)
                {
                    return false;
                }

                if (!Read16(istream, &nLanguageCode, 1)
                    || !Read16(istream, &nRegionCode, 1)
                    || !Read32(istream, &nLength, 1)
                    || !Read32(istream, &nOffset, 1))
                {
                    return false;
                }

                if ((nOffset + nLength) > size)
                {
                    return false;
                }

                const icUInt32Number nNumChar = nLength / sizeof(icUInt16Number);
                std::vector<icUInt16Number> unicodeStr(nNumChar, 0);

                // Completed with '\0' if the string is smaller than the expected size.
                const icUInt32Number readTextSize = Read16(istream, (void*)&unicodeStr[0], nNumChar);
                if (readTextSize != nNumChar)
                {
                    return false;
                }
                else
                {
                    std::string str(nNumChar + 1, '\0');
                    for (std::vector<icUInt16Number>::size_type idx = 0; idx < unicodeStr.size(); ++idx)
                    {
                        // As only the English language is supported the Basic Latin character set
                        // is enough for all the English characters.
                        str[idx] = static_cast<icUInt8Number>(unicodeStr[idx]);
                    }

                    // Removes extra '\0' if any.
                    const std::string::size_type pos = str.find('\0');
                    if (pos != std::string::npos)
                    {
                        str.resize(pos);
                    }

                    // As the order of the (country, language) is unknown, read all the strings
                    // before selecting the right one.

                    if (nRegionCode == icCountryCodeUSA)
                    {
                        // As soon as the US is found stop.
                        foundCountryUSA = str;
                        break;
                    }
                    if (nRegionCode == icCountryCodeUnitedKingdom && foundContryUK.empty())
                    {
                        foundContryUK = str;
                    }
                    if (nLanguageCode == icLanguageCodeEnglish && foundLanguageEN.empty())
                    {
                        foundLanguageEN = str;
                    }
                    if (i == 0)
                    {
                        foundFirstEntry = str;
                    }
                }
            }

            if (mText.empty())
            {
                if (!foundCountryUSA.empty())
                {
                    mText = foundCountryUSA;
                }
                else if (!foundContryUK.empty())
                {
                    mText = foundContryUK;
                }
                else if (!foundLanguageEN.empty())
                {
                    mText = foundLanguageEN;
                }
                else
                {
                    mText = foundFirstEntry;
                }
            }

            return true;
        }

        inline const std::string & GetText() const { return mText; }

    private:
        std::string mText;
    };

    class IccXYZArrayTypeReader : public IccTypeReader
    {
    public:
        IccXYZArrayTypeReader() {}

        virtual bool Read(std::istream & istream, icUInt32Number size)
        {
            // Tag size include sig that has already been read.
            if (sizeof(icTagTypeSignature) +
                sizeof(icUInt32Number) +
                sizeof(icXYZNumber) > size
                || !istream.good())
                return false;

            icUInt32Number sizeComp = (icUInt32Number)((size - 2 * sizeof(icUInt32Number)) / sizeof(icXYZNumber));

            // only used to read single XYZ
            if (sizeComp != 1)
                return false;

            icUInt32Number res; // reserved
            if (!Read32(istream, &res, 1))
                return false;

            icUInt32Number num32 = (icUInt32Number)(sizeComp * sizeof(icXYZNumber) / sizeof(icUInt32Number));

            if ((icUInt32Number)Read32(istream, &mXYZ, num32) != num32)
                return false;

            return true;
        }

        const icXYZNumber & GetXYZ() const
        {
            return mXYZ;
        }

    private:
        icXYZNumber mXYZ;

    };

    class IccParametricCurveTypeReader : public IccTypeReader
    {
    public:
        IccParametricCurveTypeReader() : mnNumParam(0), mParam(NULL) {}
        ~IccParametricCurveTypeReader()
        {
            if (mParam)
                delete[] mParam;
        }

        virtual bool IsParametricCurve() const { return true; }

        virtual bool Read(std::istream & istream, icUInt32Number size)
        {
            // Tag size include sig that has already been read.
            icUInt16Number functionType;
            icUInt16Number res16;
            icUInt32Number res32;

            icUInt32Number nHdrSize = (icUInt32Number)(
                sizeof(icTagTypeSignature) +
                sizeof(icUInt32Number) +
                2 * sizeof(icUInt16Number));

            if (nHdrSize > size)
                return false;

            if (nHdrSize + sizeof(icS15Fixed16Number) > size)
                return false;

            if (!istream.good())
                return false;

            if (!Read32(istream, &res32, 1)
                || !Read16(istream, &functionType, 1)
                || !Read16(istream, &res16, 1))
                return false;

            if (0 != functionType) {
                // unsupported function type
                return false;
            }

            if (!mnNumParam) {
                mnNumParam = (icUInt16Number)((size - nHdrSize) / sizeof(icS15Fixed16Number));
                mParam = new icS15Fixed16Number[mnNumParam];
            }

            if (mnNumParam) {
                if (nHdrSize + mnNumParam * sizeof(icS15Fixed16Number) > size)
                    return false;

                if (!Read32(istream, mParam, 1)) {
                    return false;
                }
            }

            return true;
        }

        const icS15Fixed16Number * GetParam() const
        {
            return mParam;
        }

        icUInt16Number  GetNumParam() const
        {
            return mnNumParam;
        }

    private:
        icUInt16Number      mnNumParam;
        icS15Fixed16Number *mParam;

    };

    class IccCurveTypeReader : public IccTypeReader
    {
    public:
        IccCurveTypeReader() {}

        virtual bool Read(std::istream & istream, icUInt32Number size)
        {
            // Tag size include sig that has already been read.
            if (sizeof(icTagTypeSignature) +
                sizeof(icUInt32Number) +
                sizeof(icUInt32Number) > size)
                return false;

            if (!istream.good())
                return false;

            icUInt32Number res32;
            if (!Read32(istream, &res32, 1))
                return false;

            icUInt32Number sizeData;

            if (!Read32(istream, &sizeData, 1))
                return false;

            mCurve.resize(sizeData);

            if (sizeData)
            {
                if (sizeData != (icUInt32Number)Read16Float(istream, &(mCurve[0]), sizeData))
                    return false;
            }

            return true;
        }

        const std::vector<float> & GetCurve() const
        {
            return mCurve;
        }

    private:
        std::vector<float> mCurve;

    };

    IccTypeReader * IccTypeReader::Create(icTagTypeSignature sigType)
    {
        if (icSigXYZArrayType == sigType)
            return new IccXYZArrayTypeReader();
        else if (icSigParametricCurveType == sigType)
            return new IccParametricCurveTypeReader();
        else if (icSigCurveType == sigType)
            return new IccCurveTypeReader();
        else if (icSigTextDescriptionType == sigType)
            return new IccTextDescriptionTypeReader();
        else if (icSigMultiLocalizedUnicodeType == sigType)
            return new IccMultiLocalizedUnicodeTypeReader();

        return nullptr;
    }

    struct IccTagElement
    {
        IccTagElement() = default;

        icTag mTagInfo;
        IccTypeReader * mTagReader = nullptr;
    };
    typedef std::vector<IccTagElement> TagVector;

    struct IccContent
    {
        struct FindSig
        {
            FindSig(const icTagSignature & sig) : mSig(sig) {}
            bool operator()(const IccTagElement & tag) { return tag.mTagInfo.sig == mSig; }
        private:
            FindSig() {}
            icTagSignature mSig;
         };

        bool isMatrixShaper() const
        {
            return HasTag(icSigRedColorantTag)
                && HasTag(icSigGreenColorantTag)
                && HasTag(icSigBlueColorantTag)
                && HasTag(icSigRedTRCTag)
                && HasTag(icSigGreenTRCTag)
                && HasTag(icSigBlueTRCTag);
        }

    public:
        icHeader mHeader;
        TagVector mTags;

        ~IccContent()
        {
            TagVector::iterator it = mTags.begin();
            TagVector::iterator itEnd = mTags.end();
            while (it != itEnd)
            {
                if ((*it).mTagReader)
                {
                    delete (*it).mTagReader;
                    (*it).mTagReader = NULL;
                }
                ++it;
            }
        }

        TagVector::const_iterator FindTag(const icTagSignature & sig) const
        {
            return std::find_if(mTags.begin(), mTags.end(), FindSig(sig));
        }

        TagVector::iterator FindTag(const icTagSignature & sig)
        {
            return std::find_if(mTags.begin(), mTags.end(), FindSig(sig));
        }

        bool HasTag(const icTagSignature & sig) const
        {
            return FindTag(sig) != mTags.end();
        }

        IccTypeReader * LoadTag(std::istream & istream, const icTagSignature & sig)
        {
            TagVector::iterator itTag = FindTag(sig);
            if (itTag == mTags.end())
                return NULL;
            if ((*itTag).mTagReader == NULL)
            {
                istream.seekg((*itTag).mTagInfo.offset);
                if (istream.good())
                {
                    icTagTypeSignature sigType;
                    if (Read32(istream, &sigType, 1))
                    {
                        IccTypeReader * reader = IccTypeReader::Create(sigType);
                        if (reader)
                        {
                            // Read
                            if (reader->Read(istream, (*itTag).mTagInfo.size))
                            {
                                // remember tag if read ok
                                (*itTag).mTagReader = reader;
                            }
                            else
                            {
                                delete reader;
                            }
                        }
                    }
                }

            }
            return (*itTag).mTagReader;
        }

        bool Validate(std::string & error) const
        {
            // Report critical issues.
            std::ostringstream message;

            switch (mHeader.deviceClass) {
            case icSigInputClass:
            case icSigDisplayClass:
            case icSigOutputClass:
            case icSigLinkClass:
            case icSigColorSpaceClass:
            case icSigAbstractClass:
            case icSigNamedColorClass:
                break;

            default:
                message << "Unknown profile class: ";
                message << mHeader.deviceClass;
                message << ". ";
                error = message.str();
                return false;
            }

            switch (mHeader.renderingIntent) {
            case icPerceptual:
            case icRelativeColorimetric:
            case icSaturation:
            case icAbsoluteColorimetric:
                break;

            default:
                message << "Unknown rendering intent: ";
                message << mHeader.renderingIntent;
                message << ". ";
                error = message.str();
                return false;
            }

            if (mTags.empty()) {
                message << "No tags present. ";
                error = message.str();
                return false;
            }
            return true;
        }

        bool ValidateForOCIO(std::string & error) const
        {
            if (!Validate(error)) {
                return false;
            }

            std::ostringstream message;

            // Only the Matrix/TRC Model is supported for now
            if (!isMatrixShaper())
            {
                message << "Only Matrix/TRC Model is supported. ";
                error = message.str();
                return false;
            }

            // Matrix/TRC profiles only use the XYZ PCS
            if (mHeader.pcs != icSigXYZData)
            {
                message << "Unsupported ICC profile connection space. ";
                error = message.str();
                return false;
            }

            if (mHeader.colorSpace != icSigRgbData)
            {
                message << "Unsupported ICC device color space. ";
                error = message.str();
                return false;
            }

            return true;
        }
    };
}

#endif

