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


#include "CPUInvLut3DOp.h"

#include "CPUGamutMapUtils.h"
#include "CPULutUtils.h"
#include "CPULut3DOp.h"
#include "CPULutUtils.h"

#include "../BitDepthUtils.h"
#include "../MathUtils.h"

#include "SSE2.h"
#include "half.h"

#include <algorithm>
#include <math.h>


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // TODO: Use SSE intrinsics to speed this up.

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
        unsigned int invert_hypercube
        (
            unsigned int    n,
            float*          x_out,
            float*          gr,
            unsigned int*   ind2off,
            float*          val,
            unsigned int*   guess,
            unsigned int    list_len,
            int*            ops_list,
            unsigned int*   entering_list,
            unsigned int*   new_vert_list,
            unsigned int*   path_list,
            unsigned int*   path_order
        )
        {
            // Singularity tolerance
            const double ZERO_TOL = 1.0e-9;
            // Feasibility tolerances
            const double NEGZERO_TOL = -1.0e-9;
            const double ONE_TOL = 1.0 + 1.0e-9;

            unsigned int row_perm[MAX_N], col_perm[MAX_N];
            unsigned int sweep_to[MAX_SWEEPS], sweep_from[MAX_SWEEPS];
            double base_vert[MAX_N], y[MAX_N], U[MAX_N][MAX_N], x[MAX_N], sweep_f[MAX_SWEEPS];
            double b[MAX_N], x2[MAX_N];
            double new_vert[MAX_N];

            int backsub = 0;
            unsigned int infeas = 0;
            const unsigned nm1 = n - 1;
            const unsigned nm2 = n - 2;
            unsigned numsweeps = 0;

            unsigned base_ind = 0;
            for (unsigned i = 0; i < n; i++)
            {
                base_ind += guess[i] * ind2off[i];
            }

            for (unsigned i = 0; i < n; i++)
            {
                row_perm[i] = i;  col_perm[i] = i;
                base_vert[i] = gr[base_ind + i];
                b[i] = val[i] - base_vert[i];
                y[i] = b[i];
                for (unsigned j = 0; j < n; j++)
                {
                    U[i][j] = (i == j) ? 1.0 : 0.0;
                }
            }

            for (unsigned i = 0; i < list_len; i++)
            {
                backsub = ops_list[i];
                if (backsub < 0)
                {
                    numsweeps = 0;  backsub = 0;
                    for (unsigned j = 0; j < n; j++)
                    {
                        y[j] = b[j];  row_perm[j] = j;  col_perm[j] = j;
                        for (unsigned k = 0; k < n; k++)
                        {
                            U[j][k] = (j == k) ? 1.0 : 0.0;
                        }
                    }
                }

                const unsigned entering_ind = entering_list[i];
                for (unsigned j = 0; j < n; j++)
                {
                    const unsigned tmp_ind = base_ind + n * new_vert_list[i];
                    new_vert[j] = gr[tmp_ind + j] - base_vert[j];
                }

                for (unsigned j = 0; j < numsweeps; j++)
                {
                    new_vert[sweep_to[j]] -= sweep_f[j] * new_vert[sweep_from[j]];
                }

                unsigned int leaving_nz(0);

                for (unsigned j = 0; j < n; j++)
                {
                    U[j][entering_ind] = new_vert[j];
                    if (col_perm[j] == entering_ind)
                    {
                        leaving_nz = j + 1;
                    }
                }

                if (leaving_nz <= nm2)
                {
                    const unsigned tmp_ind = col_perm[leaving_nz - 1];
                    for (unsigned j = leaving_nz - 1; j < nm2; j++)
                    {
                        col_perm[j] = col_perm[j + 1];
                    }
                    col_perm[nm2] = tmp_ind;
                }

                for (unsigned j = leaving_nz - 1; j < nm1; j++)
                {
                    const unsigned jp1 = j + 1;
                    unsigned piv = j;
                    unsigned col_piv = j;
                    double abs_d = fabs(U[row_perm[j]][col_perm[j]]);
                    for (unsigned k = jp1; k < n; k++)
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
                        for (unsigned h = jp1; h < n; h++)
                        {
                            for (unsigned k = j; k < n; k++)
                            {
                                const double abs_n = fabs(U[row_perm[k]][col_perm[h]]);
                                if (abs_n > abs_d) {
                                    abs_d = abs_n;  piv = k;  col_piv = h;
                                }
                            }
                            if (abs_d > ZERO_TOL)
                            {
                                const unsigned tmp_ind = col_perm[j];
                                col_perm[j] = col_perm[col_piv];
                                col_perm[col_piv] = tmp_ind;
                            }
                        }
                    }
                    if (piv != j)
                    {
                        const unsigned tmp_ind = row_perm[j];
                        row_perm[j] = row_perm[piv];
                        row_perm[piv] = tmp_ind;
                    }

                    const double denom = U[row_perm[j]][col_perm[j]];
                    for (unsigned h = jp1; h < n; h++)
                    {
                        double num = U[row_perm[h]][col_perm[j]];
                        if (fabs(num) >= ZERO_TOL)
                        {
                            double f = num / denom;
                            U[row_perm[h]][col_perm[j]] = 0.0;
                            for (unsigned k = jp1; k < n; k++)
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
                    for (int js = n - 1; js >= 0; js--)
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
                            for (unsigned k = js + 1; k < n; k++)
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
                        for (unsigned j = 0; j < n; j++)
                        {
                            x2[col_perm[j]] = x[j];
                        }

                        unsigned tmp_ind = i * n + n - 1;
                        x_out[path_list[tmp_ind]] = float(x2[path_order[0]]);
                        tmp_ind--;
                        for (unsigned j = 1; j < n; j++)
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
                return (unsigned int)0;
            }
            else
            {
                for (unsigned j = 0; j < n; j++)
                {
                    x_out[j] += (float)guess[j];
                }
                return (unsigned int)1;
            }
        }
    }


    InvLut3DRenderer::RangeTree::RangeTree()
    {
        m_chans = 0u;
        m_gsz[0] = 0u;  m_gsz[1] = 0u;  m_gsz[2] = 0u;  m_gsz[3] = 0u;
        m_depth = 0u;
    }

    InvLut3DRenderer::RangeTree::~RangeTree()
    {
    }

    void InvLut3DRenderer::RangeTree::initRanges(float *grvec)
    {
        const unsigned depthm1 = m_depth - 1;
        const unsigned N = m_levels[depthm1].elems;
        m_levels[depthm1].minVals.resize(N * m_chans);
        m_levels[depthm1].maxVals.resize(N * m_chans);
        // Our 3d-LUTs are stored with the blue chan varying most rapidly.
        const unsigned ind0scale = m_gsz[2] * m_gsz[1];
        const unsigned ind1scale = m_gsz[2];
        unsigned cornerOffsets[8];
        unsigned corners = 0;

        if (m_chans == 3u)
        {
            corners = 8u;
            cornerOffsets[0] = 0;                                   // base
            cornerOffsets[1] = 1;                                   // increment along B
            cornerOffsets[2] = m_gsz[2];                            // increment along G
            cornerOffsets[3] = m_gsz[2] + 1;                        // increment along B + G
            cornerOffsets[4] = m_gsz[2] * m_gsz[1];                 // increment along R
            cornerOffsets[5] = m_gsz[2] * m_gsz[1] + 1;             // increment along B + R
            cornerOffsets[6] = m_gsz[2] * m_gsz[1] + m_gsz[2];      // increment along R + G
            cornerOffsets[7] = m_gsz[2] * m_gsz[1] + m_gsz[2] + 1;  // increment along B + G + R
        }
        else if (m_chans == 2u)
        {
            corners = 4u;
            cornerOffsets[0] = 0;             // base
            cornerOffsets[1] = 1;             // increment along X
            cornerOffsets[2] = m_gsz[1];      // increment along Y
            cornerOffsets[3] = m_gsz[1] + 1;  // increment along X + Y
        }

        float minVal[3] = {0.0f, 0.0f, 0.0f};
        float maxVal[3] = { 0.0f, 0.0f, 0.0f };
        for (unsigned i = 0; i < N; i++)
        {
            const unsigned baseOffset = m_baseInds[i].inds[0] * ind0scale +
                m_baseInds[i].inds[1] * ind1scale + m_baseInds[i].inds[2];

            for (unsigned k = 0; k < m_chans; k++)
            {
                minVal[k] = grvec[baseOffset * m_chans + k];
                maxVal[k] = minVal[k];
            }

            for (unsigned j = 1; j < corners; j++)
            {
                const unsigned index = (baseOffset + cornerOffsets[j]) * m_chans;
                for (unsigned k = 0; k < m_chans; k++)
                {
                    minVal[k] = std::min(minVal[k<3 ? k : 2], grvec[index + k]);
                    maxVal[k] = std::max(maxVal[k<3 ? k : 2], grvec[index + k]);
                }
            }

            // Expand the ranges slightly to allow for error in forward evaluation.
            const float TOL = 1e-6f;

            for (unsigned k = 0; k < m_chans; k++)
            {
                m_levels[depthm1].minVals[i * m_chans + k] = minVal[k] - TOL;
                m_levels[depthm1].maxVals[i * m_chans + k] = maxVal[k] + TOL;
            }
        }
    }

    void InvLut3DRenderer::RangeTree::initInds()
    {
        if (m_chans == 3u)
        {
            const unsigned iLim = m_gsz[0] - 1u;
            const unsigned jLim = m_gsz[1] - 1u;
            const unsigned kLim = m_gsz[2] - 1u;

            m_baseInds.resize(iLim * jLim * kLim);

            unsigned cnt = 0;
            for (unsigned i = 0; i < iLim; i++)
            {
                for (unsigned j = 0; j < jLim; j++)
                {
                    for (unsigned k = 0; k < kLim; k++)
                    {
                        m_baseInds[cnt].inds[0] = i;
                        m_baseInds[cnt].inds[1] = j;
                        m_baseInds[cnt].inds[2] = k;
                        cnt++;
                    }
                }
            }
        }
        else if (m_chans == 2u)
        {
            const unsigned iLim = m_gsz[0] - 1u;
            const unsigned jLim = m_gsz[1] - 1u;

            m_baseInds.resize(iLim * jLim);

            unsigned cnt = 0;
            for (unsigned i = 0; i < iLim; i++)
            {
                for (unsigned j = 0; j < jLim; j++)
                {
                    m_baseInds[cnt].inds[0] = i;
                    m_baseInds[cnt].inds[1] = j;
                    cnt++;
                }
            }
        }
    }

    void InvLut3DRenderer::RangeTree::indsToHash(const unsigned i)
    {
        const unsigned pows2[4] = { 1u, 2u, 4u, 8u };
        unsigned keyBits[16];
        const unsigned depthm1 = m_depth - 1;

        for (unsigned level = 0; level < m_depth; level++)
        {
            keyBits[level] = 0u;
            for (unsigned ch = 0; ch < m_chans; ch++)
            {
                const unsigned indBit = (m_baseInds[i].inds[ch] >> (depthm1 - level)) & 1u;
                keyBits[level] += indBit * pows2[ch];
            }
        }
        unsigned hash = 0u;
        for (unsigned level = 0; level < m_depth; level++)
        {
            hash += keyBits[level] * m_levelScales[level];
        }
        m_baseInds[i].hash = hash;
    }

    void InvLut3DRenderer::RangeTree::updateChildren(
        const std::vector<unsigned> &hashes,
        const unsigned level
    )
    {
        const unsigned levelSize = m_levels[level].elems;
        m_levels[level].child0offsets.resize(levelSize);
        m_levels[level].numChildren.resize(levelSize);

        const unsigned maxChildren = 1u << m_chans;
        const unsigned gap = m_levelScales[level + 1] * maxChildren;
        unsigned cnt = 1;

        m_levels[level].child0offsets[0] = 0;
        const unsigned prevSize = (const unsigned)hashes.size();
        for (unsigned i = 1; i < prevSize; i++)
        {
            if (hashes[i] - hashes[i - 1] > gap)
            {
                m_levels[level].child0offsets[cnt] = i;
                cnt++;
            }
        }

        for (unsigned i = 0; i < levelSize - 1; i++)
        {
            m_levels[level].numChildren[i] = m_levels[level].child0offsets[i + 1] -
                m_levels[level].child0offsets[i];
        }
        const unsigned tmp = (const unsigned)(hashes.size() - m_levels[level].child0offsets[levelSize - 1]);
        m_levels[level].numChildren[levelSize - 1] = tmp;
    }

    void InvLut3DRenderer::RangeTree::updateRanges(const unsigned level)
    {
        const unsigned maxChildren = 1u << m_chans;
        const unsigned levelSize = m_levels[level].elems;

        m_levels[level].minVals.resize(levelSize * m_chans);
        m_levels[level].maxVals.resize(levelSize * m_chans);

        for (unsigned i = 0; i < levelSize; i++)
        {
            const unsigned index = m_levels[level].child0offsets[i];
            for (unsigned k = 0; k < m_chans; k++)
            {
                m_levels[level].minVals[i * m_chans + k] =
                    m_levels[level + 1].minVals[index * m_chans + k];
                m_levels[level].maxVals[i * m_chans + k] =
                    m_levels[level + 1].maxVals[index * m_chans + k];
            }

            // New min/max combine the min/max for all children from next lower level.
            for (unsigned j = 2; j <= maxChildren; j++)
            {
                if (m_levels[level].numChildren[i] >= j)
                {
                    const unsigned ind = index + j - 1u;
                    for (unsigned k = 0; k < m_chans; k++)
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

    void InvLut3DRenderer::RangeTree::initialize(float *grvec, unsigned gsz)
    {
        m_chans = 3u;  // only supporting Lut3D for now
        m_gsz[0] = m_gsz[1] = m_gsz[2] = gsz;
        m_gsz[3] = 0u;

        // Determine depth of tree.
        float maxGsz = 0.f;
        for (unsigned i = 0; i<m_chans; i++)
        {
            maxGsz = std::max(maxGsz, (float)m_gsz[i]);
        }
        int log2base;
        frexp(maxGsz - 2.f, &log2base);
        m_depth = (unsigned)log2base;

        m_levels.resize(m_depth);

        // Determine size of each level.
        for (int i = 0; i<(int)m_depth; i++)
        {
            unsigned levelSize = 1u;
            for (int j = 0; j<(int)m_chans; j++)
            {
                unsigned g = m_gsz[j] - 2;
                unsigned m = g >> (((int)m_depth) - 1 - i);
                levelSize *= (m + 1);
            }
            m_levels[i].elems = levelSize;
            m_levels[i].chans = m_chans;
        }

        // Determine scale to use for hash.
        m_levelScales.resize(m_depth);
        for (int level = 0; level<(int)m_depth; level++)
        {
            const unsigned depthm1 = m_depth - 1;
            const unsigned shift = (m_chans + 1u) * (depthm1 - level);
            const unsigned scale = 1u << shift;
            m_levelScales[level] = scale;
        }

        // Initialize indices into 3d-LUT.
        initInds();

        // Calculate hash for indices.

        const unsigned cnt = (const unsigned)m_baseInds.size();
        for (unsigned i = 0; i < cnt; i++)
        {
            indsToHash(i);
        }

        // Sort indices based on hash.
        std::sort(m_baseInds.begin(), m_baseInds.end());

        // Copy sorted hashes into temp vector.
        std::vector<unsigned> hashes(cnt);
        for (unsigned i = 0; i < cnt; i++)
        {
            hashes[i] = m_baseInds[i].hash;
        }

        // Initialize min/max ranges from LUT entries.
        initRanges(grvec);

        // Start at bottom of tree and work up, consolidating levels.
        for (int level = m_depth - 2; level >= 0; level--)
        {
            updateChildren(hashes, level);

            const unsigned levelSize = m_levels[level].elems;

            for (unsigned i = 0; i < levelSize; i++)
            {
                const unsigned index = m_levels[level].child0offsets[i];
                hashes[i] = hashes[index];
            }
            hashes.resize(levelSize);

            updateRanges(level);
        }
    }

/*    void RangeTree::print() const
    {
        for (unsigned level = 0; level < _depth; level++)
        {
            const unsigned levelSize = _levels[level].elems;
            std::cout << "level: " << level << "  size: " << levelSize
                << "  scale: " << _levelScales[level] << "\n";

            if (level < _depth - 1)
            {
                std::cout << "offset / num:  ";
                for (unsigned k = 0; k < 8; k++)
                {
                    std::cout << "  " << _levels[level].child0offsets[k] << " / " << _levels[level].numChildren[k];
                }
                std::cout << "\n";
            }
        }

        const unsigned n = (const unsigned)_levels[0].child0offsets.size();
        const unsigned chans = getChans();
        const unsigned level = 0;
        std::cout << "  min/max at top level\n";
        for (unsigned i = 0; i < n; i++)
        {
            for (unsigned k = 0; k < chans; k++)
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

    InvLut3DRenderer::InvLut3DRenderer(const OpData::OpDataInvLut3DRcPtr & lut)
        : CPUOp()
        , m_scale(0.0f)
        , m_dim(0)
        , m_alphaScaling(0.0f)
        , m_inMax(0.0f)
        , m_tree()
        , m_grvec(0x0)
    {
        updateData(lut);
    }

    InvLut3DRenderer:: ~InvLut3DRenderer()
    {
        resetData();
    }

    void InvLut3DRenderer::resetData()
    {
        delete[] m_grvec; m_grvec = 0x0;
    }

    void InvLut3DRenderer::updateData(const OpData::OpDataInvLut3DRcPtr & lut)
    {
        resetData();

        extrapolate3DArray(lut);

        m_dim = lut->getArray().getLength() + 2;  // extrapolation adds 2

        m_tree.initialize(m_grvec, m_dim);
        //m_tree.print();

        float outMax = GetBitDepthMaxValue(lut->getOutputBitDepth());

        m_alphaScaling = outMax / GetBitDepthMaxValue(lut->getInputBitDepth());

        // Converts from index units to inDepth units of the original LUT.
        // (Note that inDepth of the original LUT is outDepth of the inverse LUT.)
        // (Note that the result should be relative to the unextrapolated LUT,
        //  hence the dim - 3.)
        m_scale = outMax / (float)(m_dim - 3u);

        // TODO: Should improve this based on actual LUT contents since it
        // is legal for LUT contents to exceed the typical scaling range.
        m_inMax = GetBitDepthMaxValue(lut->getInputBitDepth());
    }

    void InvLut3DRenderer::extrapolate3DArray(const OpData::OpDataInvLut3DRcPtr & lut)
    {
        const unsigned dim = lut->getArray().getLength();
        const unsigned newDim = dim + 2u;

        const OpData::Lut3D::Lut3DArray& array =
            static_cast<const OpData::Lut3D::Lut3DArray&>(lut->getArray());

        // Note: By the time this function is called, the InDepth is the OutDepth
        // of the original LUT.  That is what determines the scaling of the values.
        BitDepth depth = lut->getInputBitDepth();
        OpData::Lut3D::Lut3DArray newArray(newDim, depth);

        // Copy center values.
        for (unsigned idx = 0; idx<dim; idx++)
        {
            for (unsigned jdx = 0; jdx<dim; jdx++)
            {
                for (unsigned kdx = 0; kdx<dim; kdx++)
                {
                    float RGB[3];
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(idx + 1, jdx + 1, kdx + 1, RGB);
                }
            }
        }

        const float center = GetBitDepthMaxValue(depth) * 0.5f;
        const float scale = 4.f;

        // Extrapolate faces.
        for (unsigned idx = 0; idx<dim; idx++)
        {
            for (unsigned jdx = 0; jdx<dim; jdx++)
            {
                for (unsigned kdx = 0; kdx<dim; kdx += (dim - 1u))
                {
                    float RGB[3];
                    const unsigned index = kdx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(idx + 1, jdx + 1, index, extrapolate(RGB, center, scale));
                }
            }
        }
        for (unsigned idx = 0; idx<dim; idx++)
        {
            for (unsigned jdx = 0; jdx<dim; jdx += (dim - 1u))
            {
                for (unsigned kdx = 0; kdx<dim; kdx++)
                {
                    float RGB[3];
                    const unsigned index = jdx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(idx + 1, index, kdx + 1, extrapolate(RGB, center, scale));
                }
            }
        }
        for (unsigned idx = 0; idx<dim; idx += (dim - 1u))
        {
            for (unsigned jdx = 0; jdx<dim; jdx++)
            {
                for (unsigned kdx = 0; kdx<dim; kdx++)
                {
                    float RGB[3];
                    const unsigned index = idx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(index, jdx + 1, kdx + 1, extrapolate(RGB, center, scale));
                }
            }
        }

        // Extrapolate edges.
        for (unsigned idx = 0; idx<dim; idx += (dim - 1u))
        {
            for (unsigned jdx = 0; jdx<dim; jdx += (dim - 1u))
            {
                for (unsigned kdx = 0; kdx<dim; kdx++)
                {
                    float RGB[3];
                    const unsigned indexi = idx == 0u ? 0u : dim + 1u;
                    const unsigned indexj = jdx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(indexi, indexj, kdx + 1, extrapolate(RGB, center, scale));
                }
            }
        }
        for (unsigned idx = 0; idx<dim; idx++)
        {
            for (unsigned jdx = 0; jdx<dim; jdx += (dim - 1u))
            {
                for (unsigned kdx = 0; kdx<dim; kdx += (dim - 1u))
                {
                    float RGB[3];
                    const unsigned indexk = kdx == 0u ? 0u : dim + 1u;
                    const unsigned indexj = jdx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(idx + 1, indexj, indexk, extrapolate(RGB, center, scale));
                }
            }
        }
        for (unsigned idx = 0; idx<dim; idx += (dim - 1u))
        {
            for (unsigned jdx = 0; jdx<dim; jdx++)
            {
                for (unsigned kdx = 0; kdx<dim; kdx += (dim - 1u))
                {
                    float RGB[3];
                    const unsigned indexi = idx == 0u ? 0u : dim + 1u;
                    const unsigned indexk = kdx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(indexi, jdx + 1, indexk, extrapolate(RGB, center, scale));
                }
            }
        }

        // Extrapolate corners.
        for (unsigned idx = 0; idx<dim; idx += (dim - 1u))
        {
            for (unsigned jdx = 0; jdx<dim; jdx += (dim - 1u))
            {
                for (unsigned kdx = 0; kdx<dim; kdx += (dim - 1u))
                {
                    float RGB[3];
                    const unsigned indexi = idx == 0u ? 0u : dim + 1u;
                    const unsigned indexj = jdx == 0u ? 0u : dim + 1u;
                    const unsigned indexk = kdx == 0u ? 0u : dim + 1u;
                    array.getRGB(idx, jdx, kdx, RGB);
                    newArray.setRGB(indexi, indexj, indexk, extrapolate(RGB, center, scale));
                }
            }
        }

        const OpData::Array::Values& values = newArray.getValues();
        const unsigned numValues = newArray.getNumValues();

        float* grvec = new float[numValues];
        m_grvec = grvec;

        for (unsigned i = 0; i < numValues; i++)
        {
            *grvec++ = values[i];
        }

    }

    // TODO Apply() needs further optimization work.

    void InvLut3DRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        const unsigned* gsz = m_tree.getGridSize();
        const float maxDim = float(gsz[0] - 3u);  // unextrapolated max
        const unsigned chans = m_tree.getChans();
        const unsigned depth = m_tree.getDepth();
        const TreeLevels& levels = m_tree.getLevels();
        const BaseIndsVec& baseInds = m_tree.getBaseInds();

        unsigned offs[3] = { gsz[2] * gsz[1], gsz[2], 1 };

        unsigned list_len = 8;
        int ops_list[] = { 0, 0, 1, 1, 1, 1, 1, 1 };
        unsigned entering_list[] = { 2, 1, 0, 2, 0, 2, 0, 2 };
        unsigned new_verts[] = {
            1, 0, 0,
            1, 1, 1,
            1, 1, 0,
            0, 1, 0,
            0, 1, 1,
            0, 0, 1,
            1, 0, 1,
            1, 0, 0 };
        unsigned path_list[] = {
            0, 0, 0,
            0, 0, 0,
            0, 1, 2,
            1, 0, 2,
            1, 2, 0,
            2, 1, 0,
            2, 0, 1,
            0, 2, 1 };
        unsigned path_order[] = { 1, 0, 2 };
        unsigned new_vert_list[8];
        for (unsigned i = 0; i<8; i++)
        {
            new_vert_list[i] =  // must happen before * chans
                new_verts[i * 3] * offs[0] + new_verts[i * 3 + 1] * offs[1] + new_verts[i * 3 + 2] * offs[2];
        }
        for (unsigned i = 0; i<chans; i++)
        {
            offs[i] = offs[i] * chans;
        }

        const unsigned MAX_LEVELS = 16;
        unsigned currentChild[MAX_LEVELS];
        unsigned currentNumChildren[MAX_LEVELS];
        unsigned currentChildInd[MAX_LEVELS];
        for (unsigned i = 0; i<MAX_LEVELS; i++)
        {
            currentChild[i] = 0;
            currentNumChildren[i] = 1;
            currentChildInd[i] = 0;
        }

        for (unsigned i = 0; i<numPixels; ++i)
        {
            // Although the inverse LUT has been extrapolated, it may not be enough
            // to cover an HDR float image, so need to clamp.
            const float R = clamp(rgbaBuffer[0], 0.f, m_inMax);
            const float G = clamp(rgbaBuffer[1], 0.f, m_inMax);
            const float B = clamp(rgbaBuffer[2], 0.f, m_inMax);

            const int depthm1 = depth - 1;
            unsigned baseIndx[3] = {0, 0, 0};

            currentNumChildren[0] = (unsigned)levels[0].child0offsets.size();
            currentChild[0] = 0;
            currentChildInd[0] = 0;

            // For now, if no result is found, return 0.
            float result[3] = { 0.f, 0.f, 0.f };

            unsigned cnt = 0;
            int level = 0;
            while (level >= 0)
            {
                while (currentChild[level] < currentNumChildren[level])
                {
                    const unsigned node = currentChildInd[level];
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
                            for (unsigned k = 0; k < chans; k++)
                                baseIndx[k] = baseInds[node].inds[k];

                            float fxval[3] = { R, G, B };

                            const bool valid = (invert_hypercube(3u, result, m_grvec, offs, fxval, baseIndx,
                                list_len, ops_list, entering_list, new_vert_list, path_list, path_order) != 0);

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
                rgbaBuffer[0] = clamp(result[0] - 1.f, 0.f, maxDim) * m_scale;
                rgbaBuffer[1] = clamp(result[1] - 1.f, 0.f, maxDim) * m_scale;
                rgbaBuffer[2] = clamp(result[2] - 1.f, 0.f, maxDim) * m_scale;
                rgbaBuffer[3] *= m_alphaScaling;

            }
            rgbaBuffer += 4;
        }
    }

    CPUOpRcPtr InvLut3DRenderer::GetRenderer(const OpData::OpDataInvLut3DRcPtr & lut)
    {
        CPUOpRcPtr op(new CPUNoOp);

        if (lut->getInvStyle() == OpData::InvLut3D::FAST)
        {
            //
            // OK to have a scoped ptr here because addAlgorithm will end up copying
            // any data from newLut that is needed for the renderer.  
            //
            // TODO: review this approach.  If the usage of the op pointer in
            //       addAlgorithm is change, this code will be prone to memory
            //       corruptions.
            OpData::OpDataLut3DRcPtr newLut = InvLutUtil::makeFastLut3D(lut);

            // Render with a Lut3D renderer.
            return Lut3DRenderer::GetRenderer(newLut);
        }
        else  // EXACT
        {
            op.reset(new InvLut3DRenderer(lut));
        }
        return op;
    }

}
OCIO_NAMESPACE_EXIT
