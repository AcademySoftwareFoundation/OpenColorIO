// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/lut3d/Lut3DOpCPU.h"
#include "ops/OpTools.h"
#include "Platform.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
{
namespace
{

class BaseLut3DRenderer : public OpCPU
{
public:
    explicit BaseLut3DRenderer(ConstLut3DOpDataRcPtr & lut);
    virtual ~BaseLut3DRenderer();

protected:
    void updateData(ConstLut3DOpDataRcPtr & lut);

    // Creates a LUT aligned to a 16 byte boundary with RGB and 0 for alpha
    // in order to be able to load the LUT using _mm_load_ps.
    float* createOptLut(const Array::Values& lut) const;

protected:
    // Keep all these values because they are invariant during the
    // processing. So to slim the processing code, these variables
    // are computed in the constructor.
    float*        m_optLut;
    unsigned long m_dim;
    float         m_step;

private:
    BaseLut3DRenderer() = delete;
    BaseLut3DRenderer(const BaseLut3DRenderer&) = delete;
    BaseLut3DRenderer& operator=(const BaseLut3DRenderer&) = delete;
};

class Lut3DTetrahedralRenderer : public BaseLut3DRenderer
{
public:
    explicit Lut3DTetrahedralRenderer(ConstLut3DOpDataRcPtr & lut);
    virtual ~Lut3DTetrahedralRenderer();

    void apply(const void * inImg, void * outImg, long numPixels) const;
};

class Lut3DRenderer : public BaseLut3DRenderer
{
public:
    explicit Lut3DRenderer(ConstLut3DOpDataRcPtr & lut);
    virtual ~Lut3DRenderer();

    void apply(const void * inImg, void * outImg, long numPixels) const;

};

class InvLut3DRenderer : public OpCPU
{
    typedef std::vector<unsigned long> ulongVector;
    // A level of the RangeTree.
    struct treeLevel
    {
        unsigned long      elems;         // number of elements on this level
        unsigned long      chans;         // in/out channels of the LUT
        std::vector<float> minVals;       // min LUT value for the sub-tree
        std::vector<float> maxVals;       // max LUT value for the sub-tree
        ulongVector        child0offsets; // offsets to the first children
        ulongVector        numChildren;   // number of children in the subtree

        treeLevel()
        {
            elems = 0;  chans = 0;
        }
    };
    typedef std::vector<treeLevel> TreeLevels;

    // Structure that identifies the base grid for a position in the LUT.
    struct baseInd
    {
        unsigned long inds[3] = { 0, 0, 0 }; // indices into the LUT
        unsigned long hash = 0;              // hash for this location

        bool operator<(const baseInd& val) const { return hash < val.hash; }

        baseInd()
        {
        }
    };
    typedef std::vector<baseInd> BaseIndsVec;

    // A class to allow fast range queries in a LUT.  Since LUT interpolation
    // is a convex operation, the output must be between the min and max
    // value for each channel.  This class is a modified nd-tree which allows
    // fast identification of the cubes of the LUT that could potentially
    // contain the inverse.
    class RangeTree
    {
    public:
        RangeTree();
        RangeTree(const RangeTree &) = delete;

        // Populate the tree using the LUT values.
        // - gridVector Pointer to the vectorized 3d-LUT values.
        // - gridSize The dimension of each side of the 3d-LUT.
        void initialize(float *gridVector, unsigned long gridSize);

        virtual ~RangeTree();

        // Number of input and output color channels.
        inline unsigned long getChans() const { return m_chans; }

        // Get the length of each side of the LUT captured by the tree.
        inline const unsigned long* getGridSize() const { return m_gsz; }

        // Get the depth (number of levels) in the tree.
        inline unsigned long getDepth() const { return m_depth; }

        // Get the tree levels data structure.
        inline const TreeLevels& getLevels() const { return m_levels; }

        // Get the offsets to the base of the vectors.
        inline const BaseIndsVec& getBaseInds() const { return m_baseInds; }

        // Debugging method to print tree properties.
        // void print() const;

    private:
        // Initialize the tree with the base index for each LUT cube.
        void initInds();

        // Initialize the tree with the min and max values for each LUT cube.
        void initRanges(float *grvec);

        void indsToHash(const unsigned long i);

        void updateChildren(const ulongVector &hashes,
                            const unsigned long level);

        void updateRanges(const unsigned long level);

        unsigned long   m_chans = 0;          // in/out channels of the LUT
        unsigned long   m_gsz[4] = {0,0,0,0}; // grid size of the LUT
        unsigned long   m_depth = 0;          // depth of the tree
        TreeLevels      m_levels;             // tree level structure
        BaseIndsVec     m_baseInds;           // indices for LUT base grid points
        ulongVector     m_levelScales;        // scaling of the tree levels
    };

public:

    explicit InvLut3DRenderer(ConstLut3DOpDataRcPtr & lut);
    virtual ~InvLut3DRenderer();

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;

    virtual void updateData(ConstLut3DOpDataRcPtr & lut);

    // Extrapolate the 3d-LUT to handle values outside the LUT gamut
    void extrapolate3DArray(ConstLut3DOpDataRcPtr & lut);

protected:
    float              m_scale;        // output scaling for r, g and b
                                       // components
    long               m_dim;          // grid size of the extrapolated 3d-LUT
    RangeTree          m_tree;         // object to allow fast range queries of
                                       // the LUT
    std::vector<float> m_grvec;        // extrapolated 3d-LUT values

private:
    InvLut3DRenderer() = delete;
    InvLut3DRenderer(const InvLut3DRenderer&) = delete;
    InvLut3DRenderer& operator=(const InvLut3DRenderer&) = delete;
};


int GetLut3DIndexBlueFast(int indexR, int indexG, int indexB, long dim)
{
    return 3 * (indexB + (int)dim * (indexG + (int)dim * indexR));
}

#ifdef USE_SSE
//----------------------------------------------------------------------------
// RGB channel ordering.
// Pixels ordered in such a way that the blue coordinate changes fastest,
// then the green coordinate, and finally, the red coordinate changes slowest
//
inline __m128i GetLut3DIndices(const __m128i &idxR,
                               const __m128i &idxG,
                               const __m128i &idxB,
                               const __m128i /*&sizesR*/,
                               const __m128i &sizesG,
                               const __m128i &sizesB)
{
    // SSE2 doesn't have 4-way multiplication for integer registers, so we need
    // split them into two register and multiply-add them separately, and then
    // combine the results.

    // r02 = { sizesG * idxR0, -, sizesG * idxR2, - }
    // r13 = { sizesG * idxR1, -, sizesG * idxR3, - }
    __m128i r02 = _mm_mul_epu32(sizesG, idxR);
    __m128i r13 = _mm_mul_epu32(sizesG, _mm_srli_si128(idxR,4));

    // r02 = { idxG0 + sizesG * idxR0, -, idxG2 + sizesG * idxR2, - }
    // r13 = { idxG1 + sizesG * idxR1, -, idxG3 + sizesG * idxR3, - }
    r02 = _mm_add_epi32(idxG, r02);
    r13 = _mm_add_epi32(_mm_srli_si128(idxG,4), r13);

    // r02 = { sizesB * (idxG0 + sizesG * idxR0), -, sizesB * (idxG2 + sizesG * idxR2), - }
    // r13 = { sizesB * (idxG1 + sizesG * idxR1), -, sizesB * (idxG3 + sizesG * idxR3), - }
    r02 = _mm_mul_epu32(sizesB, r02);
    r13 = _mm_mul_epu32(sizesB, r13);

    // r02 = { idxB0 + sizesB * (idxG0 + sizesG * idxR0), -, idxB2 + sizesB * (idxG2 + sizesG * idxR2), - }
    // r13 = { idxB1 + sizesB * (idxG1 + sizesG * idxR1), -, idxB3 + sizesB * (idxG3 + sizesG * idxR3), - }
    r02 = _mm_add_epi32(idxB, r02);
    r13 = _mm_add_epi32(_mm_srli_si128(idxB,4), r13);

    // r = { idxB0 + sizesB * (idxG0 + sizesG * idxR0),
    //       idxB1 + sizesB * (idxG1 + sizesG * idxR1),
    //       idxB2 + sizesB * (idxG2 + sizesG * idxR2),
    //       idxB3 + sizesB * (idxG3 + sizesG * idxR3) }
    __m128i r = _mm_unpacklo_epi32(_mm_shuffle_epi32(r02, _MM_SHUFFLE(0,0,2,0)),
                                   _mm_shuffle_epi32(r13, _MM_SHUFFLE(0,0,2,0)));

    // return { 4 * (idxB0 + sizesB * (idxG0 + sizesG * idxR0)),
    //          4 * (idxB1 + sizesB * (idxG1 + sizesG * idxR1)),
    //          4 * (idxB2 + sizesB * (idxG2 + sizesG * idxR2)),
    //          4 * (idxB3 + sizesB * (idxG3 + sizesG * idxR3)) }
    return _mm_slli_epi32(r, 2);
}

inline void LookupNearest4(float* optLut,
                           const __m128i &rIndices,
                           const __m128i &gIndices,
                           const __m128i &bIndices,
                           const __m128i &dim,
                           __m128 res[4])
{
    __m128i offsets = GetLut3DIndices(rIndices, gIndices, bIndices, dim, dim, dim);

    int* offsetInt = (int*)&offsets;

    res[0] = _mm_load_ps(optLut + offsetInt[0]);
    res[1] = _mm_load_ps(optLut + offsetInt[1]);
    res[2] = _mm_load_ps(optLut + offsetInt[2]);
    res[3] = _mm_load_ps(optLut + offsetInt[3]);
}
#else

// Linear
inline void lerp_rgb(float* out, float* a, float* b, float* z)
{
    out[0] = (b[0] - a[0]) * z[0] + a[0];
    out[1] = (b[1] - a[1]) * z[1] + a[1];
    out[2] = (b[2] - a[2]) * z[2] + a[2];
}

// Bilinear
inline void lerp_rgb(float* out, float* a, float* b, float* c,
                     float* d, float* y, float* z)
{
    float v1[3];
    float v2[3];

    lerp_rgb(v1, a, b, z);
    lerp_rgb(v2, c, d, z);
    lerp_rgb(out, v1, v2, y);
}

// Trilinear
inline void lerp_rgb(float* out, float* a, float* b, float* c, float* d,
                     float* e, float* f, float* g, float* h,
                     float* x, float* y, float* z)
{
    float v1[3];
    float v2[3];

    lerp_rgb(v1, a,b,c,d,y,z);
    lerp_rgb(v2, e,f,g,h,y,z);
    lerp_rgb(out, v1, v2, x);
}
#endif

BaseLut3DRenderer::BaseLut3DRenderer(ConstLut3DOpDataRcPtr & lut)
    : OpCPU()
    , m_optLut(0x0)
    , m_dim(0)
    , m_step(0.0f)
{
    updateData(lut);
}

BaseLut3DRenderer::~BaseLut3DRenderer()
{
#ifdef USE_SSE
    Platform::AlignedFree(m_optLut);
#else
    free(m_optLut);
#endif
}

void BaseLut3DRenderer::updateData(ConstLut3DOpDataRcPtr & lut)
{
    m_dim = lut->getArray().getLength();

    m_step = ((float)m_dim - 1.0f);

#ifdef USE_SSE
    Platform::AlignedFree(m_optLut);
#else
    free(m_optLut);
#endif
    m_optLut = createOptLut(lut->getArray().getValues());
}

#ifdef USE_SSE
// Creates a LUT aligned to a 16 byte boundary with RGB and 0 for alpha
// in order to be able to load the LUT using _mm_load_ps.
float* BaseLut3DRenderer::createOptLut(const Array::Values& lut) const
{
    const long maxEntries = m_dim * m_dim * m_dim;

    float *optLut =
        (float*)Platform::AlignedMalloc(maxEntries * 4 * sizeof(float), 16);

    float* currentValue = optLut;
    for (long idx = 0; idx<maxEntries; idx++)
    {
        currentValue[0] = SanitizeFloat(lut[idx * 3]);
        currentValue[1] = SanitizeFloat(lut[idx * 3 + 1]);
        currentValue[2] = SanitizeFloat(lut[idx * 3 + 2]);
        currentValue[3] = 0.0f;
        currentValue += 4;
    }

    return optLut;
}
#else
float* BaseLut3DRenderer::createOptLut(const Array::Values& lut) const
{
    const long maxEntries = m_dim * m_dim * m_dim;

    float *optLut =
        (float*)malloc(maxEntries * 3 * sizeof(float));

    float* currentValue = optLut;
    for (long idx = 0; idx<maxEntries; idx++)
    {
        currentValue[0] = SanitizeFloat(lut[idx * 3]);
        currentValue[1] = SanitizeFloat(lut[idx * 3 + 1]);
        currentValue[2] = SanitizeFloat(lut[idx * 3 + 2]);
        currentValue += 3;
    }

    return optLut;
}
#endif

Lut3DTetrahedralRenderer::Lut3DTetrahedralRenderer(ConstLut3DOpDataRcPtr & lut)
    : BaseLut3DRenderer(lut)
{
}

Lut3DTetrahedralRenderer::~Lut3DTetrahedralRenderer()
{
}

void Lut3DTetrahedralRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE

    __m128 step = _mm_set1_ps(m_step);
    __m128 maxIdx = _mm_set1_ps((float)(m_dim - 1));
    __m128i dim = _mm_set1_epi32(m_dim);

    __m128 v[4];
    OCIO_ALIGN(float cmpDelta[4]);

    for (long i = 0; i < numPixels; ++i)
    {
        float newAlpha = (float)in[3];

        __m128 data = _mm_set_ps(in[3], in[2], in[1], in[0]);

        __m128 idx = _mm_mul_ps(data, step);

        idx = _mm_max_ps(idx, EZERO);  // NaNs become 0
        idx = _mm_min_ps(idx, maxIdx);

        // lowIdxInt32 = floor(idx), with lowIdx in [0, maxIdx]
        __m128i lowIdxInt32 = _mm_cvttps_epi32(idx);
        __m128 lowIdx = _mm_cvtepi32_ps(lowIdxInt32);

        // highIdxInt32 = ceil(idx), with highIdx in [1, maxIdx]
        __m128i highIdxInt32 = _mm_sub_epi32(lowIdxInt32,
            _mm_castps_si128(_mm_cmplt_ps(lowIdx, maxIdx)));

        __m128 delta = _mm_sub_ps(idx, lowIdx);
        __m128 delta0 = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 delta1 = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 delta2 = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(2, 2, 2, 2));

        // lh01 = {L0, H0, L1, H1}
        // lh23 = {L2, H2, L3, H3}, L3 and H3 are not used
        __m128i lh01 = _mm_unpacklo_epi32(lowIdxInt32, highIdxInt32);
        __m128i lh23 = _mm_unpackhi_epi32(lowIdxInt32, highIdxInt32);

        // Since the cube is split along the main diagonal, the lowest corner
        // and highest corner are always used.
        // v[0] = { L0, L1, L2 }
        // v[3] = { H0, H1, H2 }

        __m128i idxR, idxG, idxB;
        // Store vertices transposed on idxR, idxG and idxB:
        // idxR = { v0r, v1r, v2r, v3r }
        // idxG = { v0g, v1g, v2g, v3g }
        // idxB = { v0b, v1b, v2b, v3b }

        // Vertices differences (vi-vj) to be multiplied by the delta factors
        __m128 dv0, dv1, dv2;

        // In tetrahedral interpolation, the cube is divided along the main
        // diagonal into 6 tetrahedra.  We compare the relative fractional 
        // position within the cube (deltaArray) to know which tetrahedron
        // we are in and therefore which four vertices of the cube we need.

        // cmpDelta = { delta[0] >= delta[1], delta[1] >= delta[2], delta[2] >= delta[0], - }
        _mm_store_ps(cmpDelta,
                     _mm_cmpge_ps(delta,
                                  _mm_shuffle_ps(delta,
                                                 delta,
                                                 _MM_SHUFFLE(0, 0, 2, 1))));

        if (cmpDelta[0])  // delta[0] > delta[1]
        {
            if (cmpDelta[1])  // delta[1] > delta[2]
            {
                // R > G > B

                // v[1] = { H0, L1, L2 }
                // v[2] = { H0, H1, L2 }

                // idxR = { L0, H0, H0, H0 }
                // idxG = { L1, L1, H1, H1 }
                // idxB = { L2, L2, L2, H2 }
                idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 1, 0));
                idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 2, 2));
                idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 0, 0, 0));

                LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                // Order: R G B => 0 1 2
                dv0 = _mm_sub_ps(v[1], v[0]);
                dv1 = _mm_sub_ps(v[2], v[1]);
                dv2 = _mm_sub_ps(v[3], v[2]);
            }
            else if (!cmpDelta[2])  // delta[0] > delta[2]
            {
                // R > B > G

                // v[1] = { H0, L1, L2 }
                // v[2] = { H0, L1, H2 }

                // idxR = { L0, H0, H0, H0 }
                // idxG = { L1, L1, L1, H1 }
                // idxB = { L2, L2, H2, H2 }
                idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 1, 0));
                idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 2, 2, 2));
                idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 0, 0));

                LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                // Order: R B G => 0 2 1
                dv0 = _mm_sub_ps(v[1], v[0]);
                dv2 = _mm_sub_ps(v[2], v[1]);
                dv1 = _mm_sub_ps(v[3], v[2]);
            }
            else
            {
                // B > R > G

                // v[1] = { L0, L1, H2 }
                // v[2] = { H0, L1, H2 }

                // idxR = { L0, L0, H0, H0 }
                // idxG = { L1, L1, L1, H1 }
                // idxB = { L2, H2, H2, H2 }
                idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 0, 0));
                idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 2, 2, 2));
                idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 1, 0));

                LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                // Order: B R G => 2 0 1
                dv2 = _mm_sub_ps(v[1], v[0]);
                dv0 = _mm_sub_ps(v[2], v[1]);
                dv1 = _mm_sub_ps(v[3], v[2]);
            }
        }
        else
        {
            if (!cmpDelta[1])  // delta[2] > delta[1]
            {
                // B > G > R

                // v[1] = { L0, L1, H2 }
                // v[2] = { L0, H1, H2 }

                // idxR = { L0, L0, L0, H0 }
                // idxG = { L1, L1, H1, H1 }
                // idxB = { L2, H2, H2, H2 }
                idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 0, 0, 0));
                idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 2, 2));
                idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 1, 0));

                LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                // Order: B G R => 2 1 0
                dv2 = _mm_sub_ps(v[1], v[0]);
                dv1 = _mm_sub_ps(v[2], v[1]);
                dv0 = _mm_sub_ps(v[3], v[2]);
            }
            else if (!cmpDelta[2])  // delta[0] > delta[2]
            {
                // G > R > B

                // v[1] = { L0, H1, L2 }
                // v[2] = { H0, H1, L2 }

                // idxR = { L0, L0, H0, H0 }
                // idxG = { L1, H1, H1, H1 }
                // idxB = { L2, L2, L2, H2 }
                idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 0, 0));
                idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 3, 2));
                idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 0, 0, 0));

                LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                // Order: G R B => 1 0 2
                dv1 = _mm_sub_ps(v[1], v[0]);
                dv0 = _mm_sub_ps(v[2], v[1]);
                dv2 = _mm_sub_ps(v[3], v[2]);
            }
            else
            {
                // G > B > R

                // v[1] = { L0, H1, L2 }
                // v[2] = { L0, H1, H2 }

                // idxR = { L0, L0, L0, H0 }
                // idxG = { L1, H1, H1, H1 }
                // idxB = { L2, L2, H2, H2 }
                idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 0, 0, 0));
                idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 3, 2));
                idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 0, 0));

                LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                // Order: G B R => 1 2 0
                dv1 = _mm_sub_ps(v[1], v[0]);
                dv2 = _mm_sub_ps(v[2], v[1]);
                dv0 = _mm_sub_ps(v[3], v[2]);
            }
        }

        __m128 result = _mm_add_ps(_mm_add_ps(v[0], _mm_mul_ps(delta0, dv0)),
            _mm_add_ps(_mm_mul_ps(delta1, dv1), _mm_mul_ps(delta2, dv2)));

        _mm_storeu_ps(out, result);

        out[3] = newAlpha;

        in  += 4;
        out += 4;
    }
#else
    const float dimMinusOne = float(m_dim) - 1.f;

    for (long i = 0; i < numPixels; ++i)
    {
        float newAlpha = (float)in[3];

        float idx[3];
        idx[0] = in[0] * m_step;
        idx[1] = in[1] * m_step;
        idx[2] = in[2] * m_step;

        // NaNs become 0.
        idx[0] = Clamp(idx[0], 0.f, dimMinusOne);
        idx[1] = Clamp(idx[1], 0.f, dimMinusOne);
        idx[2] = Clamp(idx[2], 0.f, dimMinusOne);

        int indexLow[3];
        indexLow[0] = static_cast<int>(std::floor(idx[0]));
        indexLow[1] = static_cast<int>(std::floor(idx[1]));
        indexLow[2] = static_cast<int>(std::floor(idx[2]));

        int indexHigh[3];
        // When the idx is exactly equal to an index (e.g. 0,1,2...)
        // then the computation of highIdx is wrong. However,
        // the delta is then equal to zero (e.g. idx-lowIdx),
        // so the highIdx has no impact.
        indexHigh[0] = static_cast<int>(std::ceil(idx[0]));
        indexHigh[1] = static_cast<int>(std::ceil(idx[1]));
        indexHigh[2] = static_cast<int>(std::ceil(idx[2]));

        float fx = idx[0] - static_cast<float>(indexLow[0]);
        float fy = idx[1] - static_cast<float>(indexLow[1]);
        float fz = idx[2] - static_cast<float>(indexLow[2]);

        // Compute index into LUT for surrounding corners
        const int n000 =
            GetLut3DIndexBlueFast(indexLow[0], indexLow[1], indexLow[2],
                                  m_dim);
        const int n100 =
            GetLut3DIndexBlueFast(indexHigh[0], indexLow[1], indexLow[2],
                                  m_dim);
        const int n010 =
            GetLut3DIndexBlueFast(indexLow[0], indexHigh[1], indexLow[2],
                                  m_dim);
        const int n001 =
            GetLut3DIndexBlueFast(indexLow[0], indexLow[1], indexHigh[2],
                                  m_dim);
        const int n110 =
            GetLut3DIndexBlueFast(indexHigh[0], indexHigh[1], indexLow[2],
                                  m_dim);
        const int n101 =
            GetLut3DIndexBlueFast(indexHigh[0], indexLow[1], indexHigh[2],
                                  m_dim);
        const int n011 =
            GetLut3DIndexBlueFast(indexLow[0], indexHigh[1], indexHigh[2],
                                  m_dim);
        const int n111 =
            GetLut3DIndexBlueFast(indexHigh[0], indexHigh[1], indexHigh[2],
                                  m_dim);

        if (fx > fy) {
            if (fy > fz) {
                out[0] =
                    (1 - fx)  * m_optLut[n000] +
                    (fx - fy) * m_optLut[n100] +
                    (fy - fz) * m_optLut[n110] +
                    (fz)      * m_optLut[n111];

                out[1] =
                    (1 - fx)  * m_optLut[n000 + 1] +
                    (fx - fy) * m_optLut[n100 + 1] +
                    (fy - fz) * m_optLut[n110 + 1] +
                    (fz)      * m_optLut[n111 + 1];

                out[2] =
                    (1 - fx)  * m_optLut[n000 + 2] +
                    (fx - fy) * m_optLut[n100 + 2] +
                    (fy - fz) * m_optLut[n110 + 2] +
                    (fz)      * m_optLut[n111 + 2];
            }
            else if (fx > fz)
            {
                out[0] =
                    (1 - fx)  * m_optLut[n000] +
                    (fx - fz) * m_optLut[n100] +
                    (fz - fy) * m_optLut[n101] +
                    (fy)      * m_optLut[n111];

                out[1] =
                    (1 - fx)  * m_optLut[n000 + 1] +
                    (fx - fz) * m_optLut[n100 + 1] +
                    (fz - fy) * m_optLut[n101 + 1] +
                    (fy)      * m_optLut[n111 + 1];

                out[2] =
                    (1 - fx)  * m_optLut[n000 + 2] +
                    (fx - fz) * m_optLut[n100 + 2] +
                    (fz - fy) * m_optLut[n101 + 2] +
                    (fy)      * m_optLut[n111 + 2];
            }
            else
            {
                out[0] =
                    (1 - fz)  * m_optLut[n000] +
                    (fz - fx) * m_optLut[n001] +
                    (fx - fy) * m_optLut[n101] +
                    (fy)      * m_optLut[n111];

                out[1] =
                    (1 - fz)  * m_optLut[n000 + 1] +
                    (fz - fx) * m_optLut[n001 + 1] +
                    (fx - fy) * m_optLut[n101 + 1] +
                    (fy)      * m_optLut[n111 + 1];

                out[2] =
                    (1 - fz)  * m_optLut[n000 + 2] +
                    (fz - fx) * m_optLut[n001 + 2] +
                    (fx - fy) * m_optLut[n101 + 2] +
                    (fy)      * m_optLut[n111 + 2];
            }
        }
        else
        {
            if (fz > fy)
            {
                out[0] =
                    (1 - fz)  * m_optLut[n000] +
                    (fz - fy) * m_optLut[n001] +
                    (fy - fx) * m_optLut[n011] +
                    (fx)      * m_optLut[n111];

                out[1] =
                    (1 - fz)  * m_optLut[n000 + 1] +
                    (fz - fy) * m_optLut[n001 + 1] +
                    (fy - fx) * m_optLut[n011 + 1] +
                    (fx)      * m_optLut[n111 + 1];

                out[2] =
                    (1 - fz)  * m_optLut[n000 + 2] +
                    (fz - fy) * m_optLut[n001 + 2] +
                    (fy - fx) * m_optLut[n011 + 2] +
                    (fx)      * m_optLut[n111 + 2];
            }
            else if (fz > fx)
            {
                out[0] =
                    (1 - fy)  * m_optLut[n000] +
                    (fy - fz) * m_optLut[n010] +
                    (fz - fx) * m_optLut[n011] +
                    (fx)      * m_optLut[n111];

                out[1] =
                    (1 - fy)  * m_optLut[n000 + 1] +
                    (fy - fz) * m_optLut[n010 + 1] +
                    (fz - fx) * m_optLut[n011 + 1] +
                    (fx)      * m_optLut[n111 + 1];

                out[2] =
                    (1 - fy)  * m_optLut[n000 + 2] +
                    (fy - fz) * m_optLut[n010 + 2] +
                    (fz - fx) * m_optLut[n011 + 2] +
                    (fx)      * m_optLut[n111 + 2];
            }
            else
            {
                out[0] =
                    (1 - fy)  * m_optLut[n000] +
                    (fy - fx) * m_optLut[n010] +
                    (fx - fz) * m_optLut[n110] +
                    (fz)      * m_optLut[n111];

                out[1] =
                    (1 - fy)  * m_optLut[n000 + 1] +
                    (fy - fx) * m_optLut[n010 + 1] +
                    (fx - fz) * m_optLut[n110 + 1] +
                    (fz)      * m_optLut[n111 + 1];

                out[2] =
                    (1 - fy)  * m_optLut[n000 + 2] +
                    (fy - fx) * m_optLut[n010 + 2] +
                    (fx - fz) * m_optLut[n110 + 2] +
                    (fz)      * m_optLut[n111 + 2];
            }
        }

        out[3] = newAlpha;

        in  += 4;
        out += 4;
    }
#endif
}

Lut3DRenderer::Lut3DRenderer(ConstLut3DOpDataRcPtr & lut)
    : BaseLut3DRenderer(lut)
{
}

Lut3DRenderer::~Lut3DRenderer()
{
}

void Lut3DRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE

    __m128 step = _mm_set1_ps(m_step);
    __m128 maxIdx = _mm_set1_ps((float)(m_dim - 1));
    __m128i dim = _mm_set1_epi32(m_dim);

    __m128 v[8];

    for (long i = 0; i < numPixels; ++i)
    {
        float newAlpha = (float)in[3];

        __m128 data = _mm_set_ps(in[3], in[2], in[1], in[0]);

        __m128 idx = _mm_mul_ps(data, step);

        idx = _mm_max_ps(idx, EZERO);  // NaNs become 0
        idx = _mm_min_ps(idx, maxIdx);

        // lowIdxInt32 = floor(idx), with lowIdx in [0, maxIdx]
        __m128i lowIdxInt32 = _mm_cvttps_epi32(idx);
        __m128 lowIdx = _mm_cvtepi32_ps(lowIdxInt32);

        // highIdxInt32 = ceil(idx), with highIdx in [1, maxIdx]
        __m128i highIdxInt32 = _mm_sub_epi32(lowIdxInt32,
            _mm_castps_si128(_mm_cmplt_ps(lowIdx, maxIdx)));


        __m128 delta = _mm_sub_ps(idx, lowIdx);

        // lh01 = {L0, H0, L1, H1}
        // lh23 = {L2, H2, L3, H3}, L3 and H3 are not used
        __m128i lh01 = _mm_unpacklo_epi32(lowIdxInt32, highIdxInt32);
        __m128i lh23 = _mm_unpackhi_epi32(lowIdxInt32, highIdxInt32);

        // v[0] = { L0, L1, L2 }
        // v[1] = { L0, L1, H2 }
        // v[2] = { L0, H1, L2 }
        // v[3] = { L0, H1, H2 }
        // v[4] = { H0, L1, L2 }
        // v[5] = { H0, L1, H2 }
        // v[6] = { H0, H1, L2 }
        // v[7] = { H0, H1, H2 }

        __m128i idxR_L0, idxR_H0, idxG, idxB;
        // Store vertices transposed on idxR, idxG and idxB:

        // idxR_L0 = { L0, L0, L0, L0 }
        // idxR_H0 = { H0, H0, H0, H0 }
        // idxG = { L1, L1, H1, H1 }
        // idxB = { L2, H2, L2, H2 }
        idxR_L0 = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(0, 0, 0, 0));
        idxR_H0 = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 1, 1));
        idxG = _mm_unpackhi_epi32(lh01, lh01);
        idxB = _mm_unpacklo_epi64(lh23, lh23);

        // Lookup 8 corners of cube
        LookupNearest4(m_optLut, idxR_L0, idxG, idxB, dim, v);
        LookupNearest4(m_optLut, idxR_H0, idxG, idxB, dim, v + 4);

        // Perform the trilinear interpolation
        __m128 wr = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 wg = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 wb = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(2, 2, 2, 2));

        __m128 oneMinusWr = _mm_sub_ps(EONE, wr);
        __m128 oneMinusWg = _mm_sub_ps(EONE, wg);
        __m128 oneMinusWb = _mm_sub_ps(EONE, wb);

        // Compute linear interpolation along the blue axis
        __m128 blue1(_mm_add_ps(_mm_mul_ps(v[0], oneMinusWb),
            _mm_mul_ps(v[1], wb)));

        __m128 blue2(_mm_add_ps(_mm_mul_ps(v[2], oneMinusWb),
            _mm_mul_ps(v[3], wb)));

        __m128 blue3(_mm_add_ps(_mm_mul_ps(v[4], oneMinusWb),
            _mm_mul_ps(v[5], wb)));

        __m128 blue4(_mm_add_ps(_mm_mul_ps(v[6], oneMinusWb),
            _mm_mul_ps(v[7], wb)));

        // Compute linear interpolation along the green axis
        __m128 green1(_mm_add_ps(_mm_mul_ps(blue1, oneMinusWg),
            _mm_mul_ps(blue2, wg)));

        __m128 green2(_mm_add_ps(_mm_mul_ps(blue3, oneMinusWg),
            _mm_mul_ps(blue4, wg)));

        // Compute linear interpolation along the red axis
        __m128 result = _mm_add_ps(_mm_mul_ps(green1, oneMinusWr),
            _mm_mul_ps(green2, wr));

        _mm_storeu_ps(out, result);

        out[3] = newAlpha;

        in  += 4;
        out += 4;
    }
#else
    const float dimMinusOne = float(m_dim) - 1.f;

    for (long i = 0; i < numPixels; ++i)
    {
        float newAlpha = (float)in[3];

        float idx[3];
        idx[0] = in[0] * m_step;
        idx[1] = in[1] * m_step;
        idx[2] = in[2] * m_step;

        // NaNs become 0.
        idx[0] = Clamp(idx[0], 0.f, dimMinusOne);
        idx[1] = Clamp(idx[1], 0.f, dimMinusOne);
        idx[2] = Clamp(idx[2], 0.f, dimMinusOne);

        int indexLow[3];
        indexLow[0] = static_cast<int>(std::floor(idx[0]));
        indexLow[1] = static_cast<int>(std::floor(idx[1]));
        indexLow[2] = static_cast<int>(std::floor(idx[2]));

        int indexHigh[3];
        // When the idx is exactly equal to an index (e.g. 0,1,2...)
        // then the computation of highIdx is wrong. However,
        // the delta is then equal to zero (e.g. idx-lowIdx),
        // so the highIdx has no impact.
        indexHigh[0] = static_cast<int>(std::ceil(idx[0]));
        indexHigh[1] = static_cast<int>(std::ceil(idx[1]));
        indexHigh[2] = static_cast<int>(std::ceil(idx[2]));

        float delta[3];
        delta[0] = idx[0] - static_cast<float>(indexLow[0]);
        delta[1] = idx[1] - static_cast<float>(indexLow[1]);
        delta[2] = idx[2] - static_cast<float>(indexLow[2]);

        // Compute index into LUT for surrounding corners
        const int n000 =
            GetLut3DIndexBlueFast(indexLow[0], indexLow[1], indexLow[2],
                                  m_dim);
        const int n100 =
            GetLut3DIndexBlueFast(indexHigh[0], indexLow[1], indexLow[2],
                                  m_dim);
        const int n010 =
            GetLut3DIndexBlueFast(indexLow[0], indexHigh[1], indexLow[2],
                                  m_dim);
        const int n001 =
            GetLut3DIndexBlueFast(indexLow[0], indexLow[1], indexHigh[2],
                                  m_dim);
        const int n110 =
            GetLut3DIndexBlueFast(indexHigh[0], indexHigh[1], indexLow[2],
                                  m_dim);
        const int n101 =
            GetLut3DIndexBlueFast(indexHigh[0], indexLow[1], indexHigh[2],
                                  m_dim);
        const int n011 =
            GetLut3DIndexBlueFast(indexLow[0], indexHigh[1], indexHigh[2],
                                  m_dim);
        const int n111 =
            GetLut3DIndexBlueFast(indexHigh[0], indexHigh[1], indexHigh[2],
                                  m_dim);

        float x[3], y[3], z[3];
        x[0] = delta[0]; x[1] = delta[0]; x[2] = delta[0];
        y[0] = delta[1]; y[1] = delta[1]; y[2] = delta[1];
        z[0] = delta[2]; z[1] = delta[2]; z[2] = delta[2];

        lerp_rgb(out,
                 &m_optLut[n000], &m_optLut[n001],
                 &m_optLut[n010], &m_optLut[n011],
                 &m_optLut[n100], &m_optLut[n101],
                 &m_optLut[n110], &m_optLut[n111],
                 x, y, z);

        out[3] = newAlpha;

        in  += 4;
        out += 4;
    }
#endif
}

// The inversion code is based on an algorithm in "Numerical Linear Algebra
// and Optimization, vol. 1," by Gill, Murray, and Wright.

// Max input chans
#define MAX_N 4
// Max tree depth
#define DEP 16
// Max length of ops, entering, new_vert, path lists
#define MAX_LIST 30
// Max number of sweeps involved in a factorization program list
#define MAX_SWEEPS 20

// This function tests a given grid of the LUT to see if it contains the inverse.
// A customized matrix factorization updating technique is used to compute this
// as efficiently as possible.
unsigned long invert_hypercube
(
    unsigned long    n,
    float*           x_out,
    const float*     gr,
    unsigned long*   ind2off,
    float*           val,
    unsigned long*   guess,
    unsigned long    list_len,
    long*            ops_list,
    unsigned long*   entering_list,
    unsigned long*   new_vert_list,
    unsigned long*   path_list,
    unsigned long*   path_order
)
{
    // Singularity tolerance
    const double ZERO_TOL = 1.0e-9;
    // Feasibility tolerances
    const double NEGZERO_TOL = -1.0e-9;
    const double ONE_TOL = 1.0 + 1.0e-9;

    unsigned long row_perm[MAX_N], col_perm[MAX_N];
    unsigned long sweep_to[MAX_SWEEPS], sweep_from[MAX_SWEEPS];
    double base_vert[MAX_N], y[MAX_N], U[MAX_N][MAX_N], x[MAX_N], sweep_f[MAX_SWEEPS];
    double b[MAX_N], x2[MAX_N];
    double new_vert[MAX_N];

    long backsub = 0;
    unsigned long infeas = 0;
    const unsigned long nm1 = n - 1;
    const unsigned long nm2 = n - 2;
    unsigned long numsweeps = 0;

    unsigned long base_ind = 0;
    for (unsigned long i = 0; i < n; i++)
    {
        base_ind += guess[i] * ind2off[i];
    }

    for (unsigned long i = 0; i < n; i++)
    {
        row_perm[i] = i;  col_perm[i] = i;
        base_vert[i] = gr[base_ind + i];
        b[i] = val[i] - base_vert[i];
        y[i] = b[i];
        for (unsigned long j = 0; j < n; j++)
        {
            U[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }

    for (unsigned long i = 0; i < list_len; i++)
    {
        backsub = ops_list[i];
        if (backsub < 0)
        {
            numsweeps = 0;  backsub = 0;
            for (unsigned long j = 0; j < n; j++)
            {
                y[j] = b[j];  row_perm[j] = j;  col_perm[j] = j;
                for (unsigned long k = 0; k < n; k++)
                {
                    U[j][k] = (j == k) ? 1.0 : 0.0;
                }
            }
        }

        const unsigned long entering_ind = entering_list[i];
        for (unsigned long j = 0; j < n; j++)
        {
            const unsigned long tmp_ind = base_ind + n * new_vert_list[i];
            new_vert[j] = gr[tmp_ind + j] - base_vert[j];
        }

        for (unsigned long j = 0; j < numsweeps; j++)
        {
            new_vert[sweep_to[j]] -= sweep_f[j] * new_vert[sweep_from[j]];
        }

        unsigned long leaving_nz = 0;

        for (unsigned long j = 0; j < n; j++)
        {
            U[j][entering_ind] = new_vert[j];
            if (col_perm[j] == entering_ind)
            {
                leaving_nz = j + 1;
            }
        }

        if (leaving_nz <= nm2)
        {
            const unsigned long tmp_ind = col_perm[leaving_nz - 1];
            for (unsigned long j = leaving_nz - 1; j < nm2; j++)
            {
                col_perm[j] = col_perm[j + 1];
            }
            col_perm[nm2] = tmp_ind;
        }

        for (unsigned long j = leaving_nz - 1; j < nm1; j++)
        {
            const unsigned long jp1 = j + 1;
            unsigned long piv = j;
            unsigned long col_piv = j;
            double abs_d = fabs(U[row_perm[j]][col_perm[j]]);
            for (unsigned long k = jp1; k < n; k++)
            {
                const double abs_n = fabs(U[row_perm[k]][col_perm[j]]);
                if (abs_n > abs_d)
                {
                    abs_d = abs_n;  piv = k;
                }
            }

            if (abs_d < ZERO_TOL)
            {
                // (decided to always do rank revealing factorization here, slower but more robust)
                for (unsigned long h = jp1; h < n; h++)
                {
                    for (unsigned long k = j; k < n; k++)
                    {
                        const double abs_n = fabs(U[row_perm[k]][col_perm[h]]);
                        if (abs_n > abs_d) {
                            abs_d = abs_n;  piv = k;  col_piv = h;
                        }
                    }
                    if (abs_d > ZERO_TOL)
                    {
                        const unsigned long tmp_ind = col_perm[j];
                        col_perm[j] = col_perm[col_piv];
                        col_perm[col_piv] = tmp_ind;
                    }
                }
            }
            if (piv != j)
            {
                const unsigned long tmp_ind = row_perm[j];
                row_perm[j] = row_perm[piv];
                row_perm[piv] = tmp_ind;
            }

            const double denom = U[row_perm[j]][col_perm[j]];
            for (unsigned long h = jp1; h < n; h++)
            {
                double num = U[row_perm[h]][col_perm[j]];
                if (fabs(num) >= ZERO_TOL)
                {
                    double f = num / denom;
                    U[row_perm[h]][col_perm[j]] = 0.0;
                    for (unsigned long k = jp1; k < n; k++)
                    {
                        U[row_perm[h]][col_perm[k]] -= f * U[row_perm[j]][col_perm[k]];
                    }
                    y[row_perm[h]] -= f * y[row_perm[j]];
                    sweep_to[numsweeps] = row_perm[h];
                    sweep_from[numsweeps] = row_perm[j];
                    sweep_f[numsweeps] = f;
                    numsweeps += 1;
                }
            }
        }

        if (backsub)
        {
            double running_sumx = 0.0;
            for (long js = n - 1; js >= 0; js--)
            {
                const double denom = U[row_perm[js]][col_perm[js]];
                if (fabs(denom) < ZERO_TOL)
                {
                    if (fabs(y[row_perm[js]]) > ZERO_TOL)
                    {
                        infeas = 1;
                        break;
                    }
                    else
                    {
                        x[js] = 0.0;
                        infeas = 0;
                    }
                }
                else
                {
                    double sm = 0.0;
                    for (unsigned long k = js + 1; k < n; k++)
                    {
                        sm += U[row_perm[js]][col_perm[k]] * x[k];
                    }
                    const double x_tmp = (y[row_perm[js]] - sm) / denom;

                    infeas = x_tmp < NEGZERO_TOL;
                    if (infeas)
                    {
                        break;
                    }
                    running_sumx += x_tmp;
                    infeas = running_sumx > ONE_TOL;
                    if (infeas)
                    {
                        break;
                    }

                    x[js] = x_tmp;
                }
            }

            if (!infeas)
            {
                for (unsigned long j = 0; j < n; j++)
                {
                    x2[col_perm[j]] = x[j];
                }

                unsigned long tmp_ind = i * n + n - 1;
                x_out[path_list[tmp_ind]] = float(x2[path_order[0]]);
                tmp_ind--;
                for (unsigned long j = 1; j < n; j++)
                {
                    x_out[path_list[tmp_ind]] = float(x2[path_order[j]] + x_out[path_list[tmp_ind + 1]]);
                    tmp_ind--;
                }

                break;
            }
        }
    }

    if (infeas)
    {
        return 0;
    }
    else
    {
        for (unsigned long j = 0; j < n; j++)
        {
            x_out[j] += (float)guess[j];
        }
        return 1;
    }
}

InvLut3DRenderer::RangeTree::RangeTree()
{
}

InvLut3DRenderer::RangeTree::~RangeTree()
{
}

void InvLut3DRenderer::RangeTree::initRanges(float *grvec)
{
    const unsigned long depthm1 = m_depth - 1;
    const unsigned long N = m_levels[depthm1].elems;
    m_levels[depthm1].minVals.resize(N * m_chans);
    m_levels[depthm1].maxVals.resize(N * m_chans);
    // Our 3d-LUTs are stored with the blue chan varying most rapidly.
    const unsigned long ind0scale = m_gsz[2] * m_gsz[1];
    const unsigned long ind1scale = m_gsz[2];
    unsigned long cornerOffsets[8];
    unsigned long corners = 0;

    if (m_chans == 3)
    {
        corners = 8;
        cornerOffsets[0] = 0;                                    // base
        cornerOffsets[1] = 1;                                    // increment along B
        cornerOffsets[2] = m_gsz[2];                             // increment along G
        cornerOffsets[3] = m_gsz[2] + 1;                         // increment along B + G
        cornerOffsets[4] = m_gsz[2] * m_gsz[1];                  // increment along R
        cornerOffsets[5] = m_gsz[2] * m_gsz[1] + 1;              // increment along B + R
        cornerOffsets[6] = m_gsz[2] * m_gsz[1] + m_gsz[2];       // increment along R + G
        cornerOffsets[7] = m_gsz[2] * m_gsz[1] + m_gsz[2] + 1;   // increment along B + G + R
    }
    else if (m_chans == 2)
    {
        corners = 4;
        cornerOffsets[0] = 0;             // base
        cornerOffsets[1] = 1;             // increment along X
        cornerOffsets[2] = m_gsz[1];      // increment along Y
        cornerOffsets[3] = m_gsz[1] + 1;  // increment along X + Y
    }
    else
    {
        throw Exception("Unsupported channel number.");
    }

    float minVal[MAX_N] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float maxVal[MAX_N] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (unsigned long i = 0; i < N; i++)
    {
        const unsigned long baseOffset = m_baseInds[i].inds[0] * ind0scale +
            m_baseInds[i].inds[1] * ind1scale + m_baseInds[i].inds[2];

        for (unsigned long k = 0; k < m_chans; k++)
        {
            minVal[k] = grvec[baseOffset * m_chans + k];
            maxVal[k] = minVal[k];
        }

        for (unsigned long j = 1; j < corners; j++)
        {
            const unsigned long index = (baseOffset + cornerOffsets[j]) * m_chans;
            for (unsigned long k = 0; k < m_chans; k++)
            {
                minVal[k] = std::min(minVal[k], grvec[index + k]);
                maxVal[k] = std::max(maxVal[k], grvec[index + k]);
            }
        }

        // Expand the ranges slightly to allow for error in forward evaluation.
        const float TOL = 1e-6f;

        for (unsigned long k = 0; k < m_chans; k++)
        {
            m_levels[depthm1].minVals[i * m_chans + k] = minVal[k] - TOL;
            m_levels[depthm1].maxVals[i * m_chans + k] = maxVal[k] + TOL;
        }
    }
}

void InvLut3DRenderer::RangeTree::initInds()
{
    if (m_chans == 3)
    {
        const unsigned long iLim = m_gsz[0] - 1;
        const unsigned long jLim = m_gsz[1] - 1;
        const unsigned long kLim = m_gsz[2] - 1;

        m_baseInds.resize(iLim * jLim * kLim);

        unsigned long cnt = 0;
        for (unsigned long i = 0; i < iLim; i++)
        {
            for (unsigned long j = 0; j < jLim; j++)
            {
                for (unsigned long k = 0; k < kLim; k++)
                {
                    m_baseInds[cnt].inds[0] = i;
                    m_baseInds[cnt].inds[1] = j;
                    m_baseInds[cnt].inds[2] = k;
                    cnt++;
                }
            }
        }
    }
    else if (m_chans == 2)
    {
        const unsigned long iLim = m_gsz[0] - 1;
        const unsigned long jLim = m_gsz[1] - 1;

        m_baseInds.resize(iLim * jLim);

        unsigned long cnt = 0;
        for (unsigned long i = 0; i < iLim; i++)
        {
            for (unsigned long j = 0; j < jLim; j++)
            {
                m_baseInds[cnt].inds[0] = i;
                m_baseInds[cnt].inds[1] = j;
                cnt++;
            }
        }
    }
}

void InvLut3DRenderer::RangeTree::indsToHash(const unsigned long i)
{
    const unsigned long pows2[4] = { 1, 2, 4, 8 };
    unsigned long keyBits[16];
    const unsigned long depthm1 = m_depth - 1;

    for (unsigned long level = 0; level < m_depth; level++)
    {
        keyBits[level] = 0;
        for (unsigned long ch = 0; ch < m_chans; ch++)
        {
            const unsigned long indBit = (m_baseInds[i].inds[ch] >> (depthm1 - level)) & 1;
            keyBits[level] += indBit * pows2[ch];
        }
    }
    unsigned long hash = 0;
    for (unsigned long level = 0; level < m_depth; level++)
    {
        hash += keyBits[level] * m_levelScales[level];
    }
    m_baseInds[i].hash = hash;
}

void InvLut3DRenderer::RangeTree::updateChildren(
    const ulongVector &hashes,
    const unsigned long level
)
{
    const unsigned long levelSize = m_levels[level].elems;
    m_levels[level].child0offsets.resize(levelSize);
    m_levels[level].numChildren.resize(levelSize);

    const unsigned long maxChildren = 1 << m_chans;
    const unsigned long gap = m_levelScales[level + 1] * maxChildren;
    unsigned long cnt = 1;

    m_levels[level].child0offsets[0] = 0;
    const unsigned long prevSize = (const unsigned long)hashes.size();
    for (unsigned long i = 1; i < prevSize; i++)
    {
        if (hashes[i] - hashes[i - 1] > gap)
        {
            m_levels[level].child0offsets[cnt] = i;
            cnt++;
        }
    }

    for (unsigned long i = 0; i < levelSize - 1; i++)
    {
        m_levels[level].numChildren[i] = m_levels[level].child0offsets[i + 1] -
                                         m_levels[level].child0offsets[i];
    }
    const unsigned long tmp = (const unsigned long)(hashes.size() - m_levels[level].child0offsets[levelSize - 1]);
    m_levels[level].numChildren[levelSize - 1] = tmp;
}

void InvLut3DRenderer::RangeTree::updateRanges(const unsigned long level)
{
    const unsigned long maxChildren = 1 << m_chans;
    const unsigned long levelSize = m_levels[level].elems;

    m_levels[level].minVals.resize(levelSize * m_chans);
    m_levels[level].maxVals.resize(levelSize * m_chans);

    for (unsigned long i = 0; i < levelSize; i++)
    {
        const unsigned long index = m_levels[level].child0offsets[i];
        for (unsigned long k = 0; k < m_chans; k++)
        {
            m_levels[level].minVals[i * m_chans + k] =
                m_levels[level + 1].minVals[index * m_chans + k];
            m_levels[level].maxVals[i * m_chans + k] =
                m_levels[level + 1].maxVals[index * m_chans + k];
        }

        // New min/max combine the min/max for all children from next lower level.
        for (unsigned long j = 2; j <= maxChildren; j++)
        {
            if (m_levels[level].numChildren[i] >= j)
            {
                const unsigned long ind = index + j - 1;
                for (unsigned long k = 0; k < m_chans; k++)
                {
                    const float minVal = m_levels[level].minVals[i * m_chans + k];
                    const float childMinVal = m_levels[level + 1].minVals[ind * m_chans + k];
                    if (childMinVal < minVal)
                    {
                        m_levels[level].minVals[i * m_chans + k] = childMinVal;
                    }
                    const float maxVal = m_levels[level].maxVals[i * m_chans + k];
                    const float childMaxVal = m_levels[level + 1].maxVals[ind * m_chans + k];
                    if (childMaxVal > maxVal)
                    {
                        m_levels[level].maxVals[i * m_chans + k] = childMaxVal;
                    }
                }
            }
        }
    }
}

void InvLut3DRenderer::RangeTree::initialize(float *grvec, unsigned long gsz)
{
    m_chans = 3;  // only supporting Lut3D for now
    m_gsz[0] = m_gsz[1] = m_gsz[2] = gsz;
    m_gsz[3] = 0;

    // Determine depth of tree.
    float maxGsz = 0.f;
    for (unsigned long i = 0; i<m_chans; i++)
    {
        maxGsz = std::max(maxGsz, (float)m_gsz[i]);
    }
    int log2base;
    frexp(maxGsz - 2.f, &log2base);
    m_depth = (unsigned long)log2base;

    m_levels.resize(m_depth);

    // Determine size of each level.
    for (unsigned long i = 0; i < m_depth; i++)
    {
        unsigned long levelSize = 1;
        for (unsigned long j = 0; j < m_chans; j++)
        {
            unsigned long g = m_gsz[j] - 2;
            unsigned long m = g >> (((int)m_depth) - 1 - i);
            levelSize *= (m + 1);
        }
        m_levels[i].elems = levelSize;
        m_levels[i].chans = m_chans;
    }

    // Determine scale to use for hash.
    m_levelScales.resize(m_depth);
    for (unsigned long level = 0; level < m_depth; level++)
    {
        const unsigned long depthm1 = m_depth - 1;
        const unsigned long shift = (m_chans + 1) * (depthm1 - level);
        const unsigned long scale = 1 << shift;
        m_levelScales[level] = scale;
    }

    // Initialize indices into 3d-LUT.
    initInds();

    // Calculate hash for indices.

    const unsigned long cnt = (const unsigned long)m_baseInds.size();
    for (unsigned long i = 0; i < cnt; i++)
    {
        indsToHash(i);
    }

    // Sort indices based on hash.
    std::sort(m_baseInds.begin(), m_baseInds.end());

    // Copy sorted hashes into temp vector.
    ulongVector hashes(cnt);
    for (unsigned long i = 0; i < cnt; i++)
    {
        hashes[i] = m_baseInds[i].hash;
    }

    // Initialize min/max ranges from LUT entries.
    initRanges(grvec);

    // Start at bottom of tree and work up, consolidating levels.
    for (int level = m_depth - 2; level >= 0; level--)
    {
        updateChildren(hashes, level);

        const unsigned long levelSize = m_levels[level].elems;

        for (unsigned long i = 0; i < levelSize; i++)
        {
            const unsigned long index = m_levels[level].child0offsets[i];
            hashes[i] = hashes[index];
        }
        hashes.resize(levelSize);

        updateRanges(level);
    }
}

/*    void RangeTree::print() const
{
    for (unsigned long level = 0; level < _depth; level++)
    {
        const unsigned long levelSize = _levels[level].elems;
        std::cout << "level: " << level << "  size: " << levelSize
            << "  scale: " << _levelScales[level] << "\n";

        if (level < _depth - 1)
        {
            std::cout << "offset / num:  ";
            for (unsigned long k = 0; k < 8; k++)
            {
                std::cout << "  " << _levels[level].child0offsets[k] << " / " << _levels[level].numChildren[k];
            }
            std::cout << "\n";
        }
    }

    const unsigned long n = (const unsigned long)_levels[0].child0offsets.size();
    const unsigned long chans = getChans();
    const unsigned long level = 0;
    std::cout << "  min/max at top level\n";
    for (unsigned long i = 0; i < n; i++)
    {
        for (unsigned long k = 0; k < chans; k++)
        {
            std::cout << "  " << _levels[level].minVals[i * chans + k];
            std::cout << " / " << _levels[level].maxVals[i * chans + k] << ",";
        }
        std::cout << "\n";
    }
}*/

float* extrapolate(float RGB[3], float center, float scale)
{
    RGB[0] = (RGB[0] - center) * scale + center;
    RGB[1] = (RGB[1] - center) * scale + center;
    RGB[2] = (RGB[2] - center) * scale + center;
    return RGB;
}

InvLut3DRenderer::InvLut3DRenderer(ConstLut3DOpDataRcPtr & lut)
    : OpCPU()
    , m_scale(0.0f)
    , m_dim(0)
    , m_tree()
{
    updateData(lut);
}

InvLut3DRenderer:: ~InvLut3DRenderer()
{
}

void InvLut3DRenderer::updateData(ConstLut3DOpDataRcPtr & lut)
{
    extrapolate3DArray(lut);

    m_dim = lut->getArray().getLength() + 2;  // extrapolation adds 2

    m_tree.initialize(m_grvec.data(), m_dim);
    //m_tree.print();

    // Converts from index units to inDepth units of the original LUT.
    // (Note that inDepth of the original LUT is outDepth of the inverse LUT.)
    // (Note that the result should be relative to the unextrapolated LUT,
    //  hence the dim - 3.)
    m_scale = 1.0f / (float)(m_dim - 3);
}

void InvLut3DRenderer::extrapolate3DArray(ConstLut3DOpDataRcPtr & lut)
{
    const unsigned long dim = lut->getArray().getLength();
    const unsigned long newDim = dim + 2;

    const Lut3DOpData::Lut3DArray& array =
        static_cast<const Lut3DOpData::Lut3DArray&>(lut->getArray());

    Lut3DOpData::Lut3DArray newArray(newDim);

    // Copy center values.
    for (unsigned long idx = 0; idx<dim; idx++)
    {
        for (unsigned long jdx = 0; jdx<dim; jdx++)
        {
            for (unsigned long kdx = 0; kdx<dim; kdx++)
            {
                float RGB[3];
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(idx + 1, jdx + 1, kdx + 1, RGB);
            }
        }
    }

    const float center = 0.5f;
    const float scale = 4.f;

    // Extrapolate faces.
    for (unsigned long idx = 0; idx<dim; idx++)
    {
        for (unsigned long jdx = 0; jdx<dim; jdx++)
        {
            for (unsigned long kdx = 0; kdx<dim; kdx += (dim - 1))
            {
                float RGB[3];
                const unsigned long index = kdx == 0 ? 0 : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(idx + 1, jdx + 1, index, extrapolate(RGB, center, scale));
            }
        }
    }
    for (unsigned long idx = 0; idx<dim; idx++)
    {
        for (unsigned long jdx = 0; jdx<dim; jdx += (dim - 1))
        {
            for (unsigned long kdx = 0; kdx<dim; kdx++)
            {
                float RGB[3];
                const unsigned long index = jdx == 0 ? 0 : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(idx + 1, index, kdx + 1, extrapolate(RGB, center, scale));
            }
        }
    }
    for (unsigned long idx = 0; idx<dim; idx += (dim - 1))
    {
        for (unsigned long jdx = 0; jdx<dim; jdx++)
        {
            for (unsigned long kdx = 0; kdx<dim; kdx++)
            {
                float RGB[3];
                const unsigned long index = idx == 0 ? 0 : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(index, jdx + 1, kdx + 1, extrapolate(RGB, center, scale));
            }
        }
    }

    // Extrapolate edges.
    for (unsigned long idx = 0; idx<dim; idx += (dim - 1))
    {
        for (unsigned long jdx = 0; jdx<dim; jdx += (dim - 1))
        {
            for (unsigned long kdx = 0; kdx<dim; kdx++)
            {
                float RGB[3];
                const unsigned long indexi = idx == 0u ? 0u : dim + 1;
                const unsigned long indexj = jdx == 0u ? 0u : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(indexi, indexj, kdx + 1, extrapolate(RGB, center, scale));
            }
        }
    }
    for (unsigned long idx = 0; idx<dim; idx++)
    {
        for (unsigned long jdx = 0; jdx<dim; jdx += (dim - 1))
        {
            for (unsigned long kdx = 0; kdx<dim; kdx += (dim - 1))
            {
                float RGB[3];
                const unsigned long indexk = kdx == 0 ? 0 : dim + 1;
                const unsigned long indexj = jdx == 0 ? 0 : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(idx + 1, indexj, indexk, extrapolate(RGB, center, scale));
            }
        }
    }
    for (unsigned long idx = 0; idx<dim; idx += (dim - 1))
    {
        for (unsigned long jdx = 0; jdx<dim; jdx++)
        {
            for (unsigned long kdx = 0; kdx<dim; kdx += (dim - 1))
            {
                float RGB[3];
                const unsigned long indexi = idx == 0 ? 0 : dim + 1;
                const unsigned long indexk = kdx == 0 ? 0 : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(indexi, jdx + 1, indexk, extrapolate(RGB, center, scale));
            }
        }
    }

    // Extrapolate corners.
    for (unsigned long idx = 0; idx<dim; idx += (dim - 1))
    {
        for (unsigned long jdx = 0; jdx<dim; jdx += (dim - 1))
        {
            for (unsigned long kdx = 0; kdx<dim; kdx += (dim - 1))
            {
                float RGB[3];
                const unsigned long indexi = idx == 0 ? 0 : dim + 1;
                const unsigned long indexj = jdx == 0 ? 0 : dim + 1;
                const unsigned long indexk = kdx == 0 ? 0 : dim + 1;
                array.getRGB(idx, jdx, kdx, RGB);
                newArray.setRGB(indexi, indexj, indexk, extrapolate(RGB, center, scale));
            }
        }
    }

    m_grvec = newArray.getValues();
}

// TODO apply() needs further optimization work.

void InvLut3DRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const unsigned long* gsz = m_tree.getGridSize();
    const float maxDim = float(gsz[0] - 3u);  // unextrapolated max
    const unsigned long chans = m_tree.getChans();
    const unsigned long depth = m_tree.getDepth();
    const TreeLevels& levels = m_tree.getLevels();
    const BaseIndsVec& baseInds = m_tree.getBaseInds();

    unsigned long offs[3] = { gsz[2] * gsz[1], gsz[2], 1 };

    unsigned long list_len = 8;
    long ops_list[] =               { 0, 0, 1, 1, 1, 1, 1, 1 };
    unsigned long entering_list[] = { 2, 1, 0, 2, 0, 2, 0, 2 };
    unsigned long new_verts[] = {
        1, 0, 0,
        1, 1, 1,
        1, 1, 0,
        0, 1, 0,
        0, 1, 1,
        0, 0, 1,
        1, 0, 1,
        1, 0, 0 };
    unsigned long path_list[] = {
        0, 0, 0,
        0, 0, 0,
        0, 1, 2,
        1, 0, 2,
        1, 2, 0,
        2, 1, 0,
        2, 0, 1,
        0, 2, 1 };
    unsigned long path_order[] = { 1, 0, 2 };
    unsigned long new_vert_list[8];
    for (int i = 0; i < 8; i++)
    {
        new_vert_list[i] =  // must happen before * chans
            new_verts[i * 3] * offs[0] + new_verts[i * 3 + 1] * offs[1] + new_verts[i * 3 + 2] * offs[2];
    }
    for (unsigned long i = 0; i<chans; i++)
    {
        offs[i] = offs[i] * chans;
    }

    const unsigned long MAX_LEVELS = 16;
    unsigned long currentChild[MAX_LEVELS];
    unsigned long currentNumChildren[MAX_LEVELS];
    unsigned long currentChildInd[MAX_LEVELS];
    for (unsigned long i = 0; i<MAX_LEVELS; i++)
    {
        currentChild[i] = 0;
        currentNumChildren[i] = 1;
        currentChildInd[i] = 0;
    }

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long i = 0; i<numPixels; ++i)
    {
        // Although the inverse LUT has been extrapolated, it may not be enough
        // to cover an HDR float image, so need to clamp.

        // TODO: Should improve this based on actual LUT contents since it
        // is legal for LUT contents to exceed the typical scaling range.
        constexpr float inMax = 1.0f;
        const float R = Clamp(in[0], 0.f, inMax);
        const float G = Clamp(in[1], 0.f, inMax);
        const float B = Clamp(in[2], 0.f, inMax);

        const long depthm1 = depth - 1;
        unsigned long baseIndx[3] = {0, 0, 0};

        currentNumChildren[0] = (unsigned long)levels[0].child0offsets.size();
        currentChild[0] = 0;
        currentChildInd[0] = 0;

        // For now, if no result is found, return 0.
        float result[3] = { 0.f, 0.f, 0.f };

        unsigned long cnt = 0;
        long level = 0;
        while (level >= 0)
        {
            while (currentChild[level] < currentNumChildren[level])
            {
                const unsigned long node = currentChildInd[level];
                const bool inRange =
                    R >= levels[level].minVals[node * chans] &&
                    G >= levels[level].minVals[node * chans + 1] &&
                    B >= levels[level].minVals[node * chans + 2] &&
                    R <= levels[level].maxVals[node * chans] &&
                    G <= levels[level].maxVals[node * chans + 1] &&
                    B <= levels[level].maxVals[node * chans + 2];
                currentChild[level]++;
                currentChildInd[level]++;
                cnt++;

                if (inRange)
                {
                    if (level == depthm1)
                    {
                        for (unsigned long k = 0; k < chans; k++)
                            baseIndx[k] = baseInds[node].inds[k];

                        float fxval[3] = { R, G, B };

                        const bool valid = (invert_hypercube(3, result, m_grvec.data(),
                                                             offs, fxval, baseIndx,
                                                             list_len, ops_list,
                                                             entering_list, new_vert_list,
                                                             path_list, path_order) != 0);

                        if (valid)
                        {
                            level = 0;  // to exit outer loop
                            break;
                        }
                    }
                    else
                    {
                        const int newLevel = level + 1;
                        currentNumChildren[newLevel] = levels[level].numChildren[node];
                        currentChildInd[newLevel] = levels[level].child0offsets[node];
                        level = newLevel;
                        currentChild[level] = 0;
                    }
                }
            }
            level--;

            // Need to subtract 1 since the indices include the extrapolation.
            out[0] = Clamp(result[0] - 1.f, 0.f, maxDim) * m_scale;
            out[1] = Clamp(result[1] - 1.f, 0.f, maxDim) * m_scale;
            out[2] = Clamp(result[2] - 1.f, 0.f, maxDim) * m_scale;
            out[3] = in[3];
        }

        in  += 4;
        out += 4;
    }
}

ConstOpCPURcPtr GetForwardLut3DRenderer(ConstLut3DOpDataRcPtr & lut)
{
    const Interpolation interp = lut->getConcreteInterpolation();
    if (interp == INTERP_TETRAHEDRAL)
    {
        return std::make_shared<Lut3DTetrahedralRenderer>(lut);
    }
    else
    {
        return std::make_shared<Lut3DRenderer>(lut);
    }
}

} // anonymous namspace

ConstOpCPURcPtr GetLut3DRenderer(ConstLut3DOpDataRcPtr & lut)
{
    switch (lut->getDirection())
    {
    case TRANSFORM_DIR_FORWARD:
        return GetForwardLut3DRenderer(lut);
        break;
    case TRANSFORM_DIR_INVERSE:
        return std::make_shared<InvLut3DRenderer>(lut);
        break;
    }
    throw Exception("Illegal LUT3D direction.");
}

} // namespace OCIO_NAMESPACE

