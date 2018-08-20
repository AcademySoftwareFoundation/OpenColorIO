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


#ifndef INCLUDED_OCIO_OPDATAVEC_H
#define INCLUDED_OCIO_OPDATAVEC_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpData.h"

#include <vector>


OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
    
// The class encapsulates an ordered list of Op
class OpDataVec
{
public:
    OpDataVec();
    OpDataVec(const OpDataVec & l);
    ~OpDataVec();

    // Assign a OpDataVec to the list
    // - l the list to copy
    OpDataVec & operator=(const OpDataVec & l);

    // Append a OpDataVec to the list and sync bit-depths
    // (note that the ops are cloned as they are appended).
    // - l the list to append     
    OpDataVec & operator+=(const OpDataVec & l);

    // Append an op (without cloning) to the list and sync bit-depths
    // - op the op to append
    void append(OpData * op);

    // Prepend an op (without cloning) to the list and sync bit-depths
    // - op the op to append
    void prepend(OpData * op);

    // Insert an Op at idx (without cloning) and shift following ops right.
    // Bit-depths are synced at the interface but the input/output depths
    // of the list may change.  (Insertion at start of empty list is ok.)
    // - op the pointer to the replacement op
    // - idx the index where op will go
    void insert(OpData * op, const unsigned idx);

    // Insert an Op at idx (without cloning) and free what was at idx.
    // Bit-depths are synced at the interface but the input/output depths
    // of the list may change.
    // - op the pointer to the replacement op
    // - idx the index where op will go
    //
    void replace(OpData * op, const unsigned idx);

    // Delete and free the Op at idx.
    // Bit-depths are synced at the interface but the input/output depths
    // of the list may change.
    // - idx the index of the op to erase
    void erase(const unsigned idx);

    // Remove without freeing the Op at idx.
    // Bit-depths are synced at the interface but the input/output depths
    // of the list may change.
    // - idx the index of the op to remove
    // Return the removed op
    OpData * remove(const unsigned idx);

    // Validate the state of the instance
    void validate() const;

    // Test if the list is empty
    // Return true upon success
    bool empty() const;

    // Get the size of the list
    // Return the actual number of element
    unsigned size() const;

    // Get the idx'th op from the list
    // - idx is the index of the requested OpData
    // Return const Pointer to the Op in the oplist at idx'th index from the list.
    const OpData * get(unsigned idx) const;
    const OpData * operator[](unsigned idx) const { return get(idx); }

    // Get the idx'th op from the list
    // - idx is the index of the requested OpData
    // Return Pointer to the Op in the oplist at idx'th index from the list.
    OpData * get(unsigned idx);
    OpData * operator[](unsigned idx) { return get(idx); }

    // Check if an OpList is equal to this instance.
    // - op is the other OpdataVec to compare with.
    // Return true if they are equal, false otherwise
    bool operator==(const OpDataVec & op) const;

    // Obtain the inverted OpList for this instance.
    void inverse(OpDataVec & ops) const;

    // Obtain a clone of the list of ops
    void clone(OpDataVec & ops, OpData::CloneType type) const;

protected:
    // Free the content list
    void freeContent();

private:
    // Type definition for a list of process nodes
    typedef std::vector<OpData*> List;

    // List containing all Ops
    List m_content;
};
}
}
OCIO_NAMESPACE_EXIT


#endif
