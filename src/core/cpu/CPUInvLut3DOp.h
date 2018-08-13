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


#ifndef INCLUDED_OCIO_CPU_INVLUT3DOP
#define INCLUDED_OCIO_CPU_INVLUT3DOP


#include <OpenColorIO/OpenColorIO.h>

#include "../opdata/OpDataInvLut3D.h"

#include "CPUOp.h"


OCIO_NAMESPACE_ENTER
{
    class InvLut3DRenderer : public CPUOp
    {

        // A level of the RangeTree
        struct treeLevel
        {
            unsigned              elems;          // number of elements on this level
            unsigned              chans;          // in/out channels of the LUT
            std::vector<float>    minVals;        // min LUT value for the sub-tree
            std::vector<float>    maxVals;        // max LUT value for the sub-tree
            std::vector<unsigned> child0offsets;  // offsets to the first children
            std::vector<unsigned> numChildren;    // number of children in the subtree

            treeLevel()
            {
                elems = 0u;  chans = 0u;
            }
        };
        typedef std::vector<treeLevel> TreeLevels;

        // Structure that identifies the base grid for a position in the LUT
        struct baseInd
        {
            unsigned inds[3];  // indices into the LUT
            unsigned hash;     // hash for this location

            bool operator<(const baseInd& val) const { return hash < val.hash; }

            baseInd()
            {
                inds[0] = 0u;  inds[1] = 0u;  inds[2] = 0u;  hash = 0u;
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
            // Constructor
            RangeTree();

            // Populate the tree using the LUT values
            // - gridVector Pointer to the vectorized 3d-LUT values
            // - gridSize The dimension of each side of the 3d-LUT
            void initialize(float *gridVector, unsigned gridSize);

            // Destructor
            virtual ~RangeTree();

            // Populate the tree using the LUT values
            inline unsigned getChans() const { return m_chans; }

            // Get the length of each side of the LUT captured by the tree
            inline const unsigned* getGridSize() const { return m_gsz; }

            // Get the depth (number of levels) in the tree
            inline unsigned getDepth() const { return m_depth; }

            // Get the tree levels data structure
            inline const TreeLevels& getLevels() const { return m_levels; }

            // Get the offsets to the base of the vectors
            inline const BaseIndsVec& getBaseInds() const { return m_baseInds; }

            // Debugging method to print tree properties
            // void print() const;

        private:
            // Populate the tree using the LUT values
            void initInds();

            // Populate the tree using the LUT values
            void initRanges(float *grvec);

            // Populate the tree using the LUT values
            void indsToHash(const unsigned i);

            // Populate the tree using the LUT values
            void updateChildren(const std::vector<unsigned> &hashes, const unsigned level);

            // Populate the tree using the LUT values
            void updateRanges(const unsigned level);

            unsigned              m_chans;        // in/out channels of the LUT
            unsigned              m_gsz[4];       // grid size of the LUT
            unsigned              m_depth;        // depth of the tree
            TreeLevels            m_levels;       // tree level structure
            BaseIndsVec           m_baseInds;     // indices for LUT base grid points
            std::vector<unsigned> m_levelScales;  // scaling of the tree levels
        };

    public:


        static CPUOpRcPtr GetRenderer(const OpData::OpDataInvLut3DRcPtr & lut);

        InvLut3DRenderer(const OpData::OpDataInvLut3DRcPtr & lut);
        virtual ~InvLut3DRenderer();

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

        void resetData();

        virtual void updateData(const OpData::OpDataInvLut3DRcPtr & lut);

        // Extrapolate the 3d-LUT to handle values outside the LUT gamut
        void extrapolate3DArray(const OpData::OpDataInvLut3DRcPtr & lut);

    protected:
        float     m_scale;         // output scaling for the r, g and b components
        unsigned  m_dim;           // grid size of the extrapolated 3d-LUT
        float     m_alphaScaling;  // bit-depth scale factor for alpha channel
        float     m_inMax;         // value to clamp inputs to apply method
        RangeTree m_tree;          // object to allow fast range queries of the LUT
        float*    m_grvec;         // extrapolated 3d-LUT values

    private:
        InvLut3DRenderer();
        InvLut3DRenderer(const InvLut3DRenderer&);
        InvLut3DRenderer& operator=(const InvLut3DRenderer&);
    };

}
OCIO_NAMESPACE_EXIT


#endif