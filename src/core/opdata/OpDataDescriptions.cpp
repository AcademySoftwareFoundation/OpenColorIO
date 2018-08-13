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

#include "OpDataDescriptions.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace OpData
{

    Descriptions::Descriptions()
    {
    }

    Descriptions::~Descriptions()
    {
    }


} // exit OpData namespace

}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OIIO_ADD_TEST(OpDataDescriptions, TestAccessors)
{
    OCIO::OpData::Descriptions desc;

    OIIO_CHECK_ASSERT(desc.getList().size() == 0);

    desc += "Description 1";
    OIIO_CHECK_ASSERT(desc.getList().size() == 1);
    OIIO_CHECK_ASSERT(desc.getList()[0] == "Description 1");

    desc += "Description 2";
    OIIO_CHECK_ASSERT(desc.getList().size() == 2);
    OIIO_CHECK_ASSERT(desc.getList()[0] == "Description 1");
    OIIO_CHECK_ASSERT(desc.getList()[1] == "Description 2");

    OCIO::OpData::Descriptions desc2;
    desc2 += "Description 3";

    desc2 += desc;
    OIIO_CHECK_ASSERT(desc2.getList().size() == 3);
    OIIO_CHECK_ASSERT(desc2.getList()[0] == "Description 3");
    OIIO_CHECK_ASSERT(desc2.getList()[1] == "Description 1");
    OIIO_CHECK_ASSERT(desc2.getList()[2] == "Description 2");

    OIIO_CHECK_ASSERT(!(desc2 == desc));

}

OIIO_ADD_TEST(OpDataDescriptions, TestEqual)
{
    OCIO::OpData::Descriptions desc;
    desc += "Description 1";
    desc += "Description 2";

    OCIO::OpData::Descriptions desc2;
    desc2 += "Description 1";
    desc2 += "Description 2";


    OIIO_CHECK_ASSERT(desc2 == desc);
}

#endif
