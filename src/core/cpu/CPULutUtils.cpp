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


#include "CPULutUtils.h"

#include "../opdata/OpDataTools.h"
#include "../opdata/OpDataInvLut1D.h"


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // Allow us to temporarily manipulate the style without cloning the object.
        class LutStyleGuard
        {
        public:
            LutStyleGuard(const OpData::OpDataLut1DRcPtr & lutToGuard)
                :   m_lut(0x0)
                ,   m_wasFast(false)
            { 
                const OpData::InvLut1D * 
                    l = dynamic_cast<const OpData::InvLut1D*>(lutToGuard.get());
                m_lut = const_cast<OpData::InvLut1D*>(l);

                m_wasFast = m_lut->getInvStyle() == OpData::InvLut1D::FAST;

                m_lut->setInvStyle(OpData::InvLut1D::EXACT);
            }

            OpData::InvLut1D * lut() const { return m_lut; }

            ~LutStyleGuard()
            { 
                if (m_wasFast)
                {
                    m_lut->setInvStyle(OpData::InvLut1D::FAST);
                }
            }

        private:
            OpData::InvLut1D * m_lut;
            bool               m_wasFast;

        private:
            LutStyleGuard();
            LutStyleGuard(const LutStyleGuard&);
        };
    }

    namespace InvLutUtil
    {
        // The domain to use for the FastLut is a challenging problem since we don't
        // know the input and output color space of the LUT.  In particular, we don't
        // know if a half or normal domain would be better.  For now, we use a
        // heuristic which is based on the original input bit-depth of the inverse LUT
        // (the output bit-depth of the forward LUT).  (We preserve the original depth
        // as a member since typically by the time this routine is called, the depth
        // has been reset to 32f.)  However, there are situations where the origDepth
        // is not reliable (e.g. a user creates a transform in Custom mode and exports it).
        // Ultimately, the goal is to replace this with an automated algorithm that
        // computes the best domain based on analysis of the curvature of the LUT.

        OpData::OpDataLut1DRcPtr makeFastLut1D(
            const OpData::OpDataLut1DRcPtr & lut, bool forGPU)
        {
            if(lut->getOpType() != OpData::OpData::InvLut1DType)
            {
                throw Exception("Cannot apply InvLut1DOp op, "
                                "Not an inverse LUT 1D data");
            }

            // The composition needs to use the EXACT renderer.
            // (Also avoids infinite loop.)
            // So temporarily set the style to EXACT.
            LutStyleGuard guard(lut);

            // 
            BitDepth depth(guard.lut()->getOriginalInDepth());

            // For typical LUTs (e.g. gamma tables from ICC monitor profiles)
            // we can use a smaller FastLUT on the GPU.
            // Currently allowing 16f to be subsampled for GPU but using 16i as a way
            // to indicate not to subsample certain LUTs (e.g. float-conversion LUTs).
            if (forGPU && depth != BIT_DEPTH_UINT16)
            {
                // GPU will always interpolate rather than look-up.
                // Use a smaller table for better efficiency.
                // TODO: Investigate performance/quality trade-off.
                depth = BIT_DEPTH_UINT12;
            }

            // But if the LUT has values outside [0,1] (e.g. some OCIO cases),
            // use a half-domain fastLUT.
            if (guard.lut()->hasExtendedDomain())
            {
                depth = BIT_DEPTH_F16;
            }

            // Make a domain for the composed Lut1D.
            OpData::OpDataLut1DRcPtr newDomain(OpData::Lut1D::makeLookupDomain(depth));

            // Regardless of what depth is used to build the domain, set the in & out to the
            // actual depth so that scaling is done correctly.
            newDomain->setInputBitDepth(lut->getInputBitDepth());
            newDomain->setOutputBitDepth(lut->getInputBitDepth());

            // Compose the Lut1D newDomain with our invLut1D (using EXACT style).
            OpData::OpDataLut1DRcPtr newLut( 
                OpData::Compose(newDomain, lut, OpData::COMPOSE_RESAMPLE_NO));

            newLut->setHueAdjust(lut->getHueAdjust());

            return newLut;
        }
    }

    namespace
    {
        // Allow us to temporarily manipulate the style without cloning the object.
        class Lut3DStyleGuard
        {
        public:
            Lut3DStyleGuard(const OpData::OpDataInvLut3DRcPtr & lut)
                :   m_lut(lut)
                ,   m_wasFast(lut->getInvStyle() == OpData::InvLut3D::FAST)
            { 
                m_lut->setInvStyle(OpData::InvLut3D::EXACT);
            }

            ~Lut3DStyleGuard()
            { 
                if (m_wasFast)
                {
                    m_lut->setInvStyle(OpData::InvLut3D::FAST);
                }
            }

        private:
            OpData::OpDataInvLut3DRcPtr m_lut;
            bool                        m_wasFast;
        };
    }

    namespace InvLutUtil
    {
        OpData::OpDataLut3DRcPtr makeFastLut3D(const OpData::OpDataInvLut3DRcPtr & lut)
        {
            // The composition needs to use the EXACT renderer.
            // (Also avoids infinite loop.)
            // So temporarily set the style to EXACT.
            Lut3DStyleGuard guard(lut);

            // Make a domain for the composed Lut3D.
            // TODO: Using a large number like 64 here is better for accuracy, 
            // but it causes a delay of several seconds when creating the renderer. 
            const unsigned GridSize = 48u;
            OpData::OpDataLut3DRcPtr newDomain(new OpData::Lut3D(GridSize));

            // Regardless of what depth is used to build the domain, set the in & out to the
            // actual depth so that scaling is done correctly.
            newDomain->setInputBitDepth( lut->getInputBitDepth() );
            newDomain->setOutputBitDepth( lut->getInputBitDepth() );

            // Compose the Lut3D newDomain with our invLut3D (using EXACT style).
            OpData::OpDataLut3DRcPtr newLut(OpData::Compose(newDomain, lut));

            // The EXACT inversion style computes an inverse to the tetrahedral
            // style of forward evalutation.
            // TODO: Although this seems like the "correct" thing to do, it does
            // not seem to help accuracy (and is slower).  To investigate ...
            //newLut->setInterpolation(SYNCOLOR::LUT3D_ALGO_TETRAHEDRAL);

            return newLut;
        }
    }

}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"
#include "CPUGamutMapUtils.h"

OCIO_NAMESPACE_USING


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#include "../FileFormatCTF.h"


OIIO_ADD_TEST(InvLutUtil, InvLut1D_LutSize_ExtendedDomain)
{
    const std::string filename("lut1d_inv_gpu_err.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(filename));
    OIIO_CHECK_EQUAL(transform->getOps().size(), 1);
    OIIO_CHECK_EQUAL(transform->getOps().get(0)->getOpType(), 
                     OCIO::OpData::OpData::InvLut1DType);

    // Ordinarily the entries would be determined by the inDepth.
    // This is just to make sure the 32f depth of the above is not what gets the half domain.
    transform->getOps().get(0)->setInputBitDepth(OCIO::BIT_DEPTH_UINT10);

    OpData::OpDataLut1DRcPtr 
        lut(dynamic_cast<OCIO::OpData::InvLut1D *>(transform->getOps().remove(0)));

    OCIO::OpData::OpDataLut1DRcPtr newLut;
    OIIO_CHECK_NO_THROW(newLut = OCIO::InvLutUtil::makeFastLut1D(lut, true));

    // This LUT has values outside [0,1], so the fastLut needs to have a half domain
    // even on GPU.
    OIIO_CHECK_EQUAL(newLut->getArray().getLength(), 65536u);
    OIIO_CHECK_ASSERT(newLut->isInputHalfDomain());
}

OIIO_ADD_TEST(InvLutUtil, InvLut1D_LutSize_GPUOpt)
{
    const std::string filename("lut1d_inv.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(filename));
    OIIO_CHECK_EQUAL(transform->getOps().size(), 2);
    OIIO_CHECK_EQUAL(transform->getOps().get(1)->getOpType(), 
                     OCIO::OpData::OpData::InvLut1DType);

    OpData::OpDataLut1DRcPtr 
        lut(dynamic_cast<OCIO::OpData::InvLut1D *>(transform->getOps().remove(1)));

    OCIO::OpData::OpDataLut1DRcPtr newLut;
    OIIO_CHECK_NO_THROW(newLut = OCIO::InvLutUtil::makeFastLut1D(lut, true));

    // This LUT has all values in [0,1], so the fastLut should be compact for more efficient
    // evaluation and less texture usage on GPU.
    OIIO_CHECK_EQUAL(newLut->getArray().getLength(), 4096u);
    OIIO_CHECK_ASSERT(!newLut->isInputHalfDomain());
}

OIIO_ADD_TEST(InvLutUtil, InvLut1D_LutSize_HalfDomain)
{
    const std::string filename("lut1d_halfdom.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(filename));
    OIIO_CHECK_EQUAL(transform->getOps().size(), 1);
    OIIO_CHECK_EQUAL(transform->getOps().get(0)->getOpType(), 
                     OCIO::OpData::OpData::Lut1DType);

    OpData::OpDataLut1DRcPtr 
        fwdLut(dynamic_cast<OCIO::OpData::Lut1D *>(transform->getOps().remove(0)));

    OCIO::OpData::OpDataLut1DRcPtr invLut(new OCIO::OpData::InvLut1D( *fwdLut ));

    OCIO::OpData::OpDataLut1DRcPtr newLut;
    OIIO_CHECK_NO_THROW(newLut = OCIO::InvLutUtil::makeFastLut1D(invLut, true));

    // Source LUT has an extended domain, so fastLut should have a half domain.
    OIIO_CHECK_EQUAL(newLut->getArray().getLength(), 65536u);
    OIIO_CHECK_ASSERT(newLut->isInputHalfDomain());
}

OIIO_ADD_TEST(InvLutUtil, InvLut3D_LutSize)
{
    const std::string filename("lut3d_example.clf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(filename));
    OIIO_CHECK_EQUAL(transform->getOps().size(), 1);
    OIIO_CHECK_EQUAL(transform->getOps().get(0)->getOpType(),
        OCIO::OpData::OpData::Lut3DType);

    OpData::OpDataLut3DRcPtr
        fwdLut(dynamic_cast<OCIO::OpData::Lut3D *>(transform->getOps().remove(0)));

    OCIO::OpData::OpDataInvLut3DRcPtr invLut(new OCIO::OpData::InvLut3D(*fwdLut));

    OCIO::OpData::OpDataLut3DRcPtr newLut;
    OIIO_CHECK_NO_THROW(newLut = OCIO::InvLutUtil::makeFastLut3D(invLut));

    OIIO_CHECK_EQUAL(newLut->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(newLut->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(newLut->getArray().getLength(), 48u);
}


OIIO_ADD_TEST(GamutMapUtil, order3_test)
{
    const float posinf = std::numeric_limits<float>::infinity();
    const float qnan = std::numeric_limits<float>::quiet_NaN();

    // { A, NaN, B } with A > B test (used to be a crash)
    {
        const float RGB[] = { 65504.f, -qnan, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }
    // Triple NaN test
    {
        const float RGB[] = { qnan, qnan, -qnan };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }
    // -Inf test
    {
        const float RGB[] = { 65504.f, -posinf, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 0);
        OIIO_CHECK_EQUAL(mid, 2);
        OIIO_CHECK_EQUAL(min, 1);
    }
    // Inf test
    {
        const float RGB[] = { 0.f, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 2);
    }
    // Double Inf test
    {
        const float RGB[] = { posinf, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 2);
    }

    // Equal values.
    {
        const float RGB[] = { 0.f, 0.f, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        // In this case we only really care that they are distinct and in [0,2]
        // so this test could be changed (it is ok, but overly restrictive).
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }

    // Now test the six typical possibilities.
    {
        const float RGB[] = { 3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 0);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { -3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 2);
        OIIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -0.3f, 2.f, -1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { 3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 0);
        OIIO_CHECK_EQUAL(mid, 2);
        OIIO_CHECK_EQUAL(min, 1);
    }
    {
        const float RGB[] = { 3.f, -2.f, 10.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 1);
    }

}

#endif
