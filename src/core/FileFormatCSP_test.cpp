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

//#include <string>
//#include <iostream>
//#include <ostream>
//#include <fstream>
//#include <sstream>
//#include <iterator>

#define BOOST_TEST_MODULE lutreader_csp
#include <boost/test/unit_test.hpp>

#include <OpenColorIO/OpenColorIO.h>

#include "FileFormatCSP.h"
namespace OCIO = OCIO_NAMESPACE;

BOOST_AUTO_TEST_CASE ( test_simple1D )
{
    
    //
    std::ostringstream strebuf;
    strebuf << "CSPLUTV100"              << "\n";
    strebuf << "1D"                      << "\n";
    strebuf << ""                        << "\n";
    strebuf << "BEGIN METADATA"          << "\n";
    strebuf << "foobar"                  << "\n";
    strebuf << "END METADATA"            << "\n";
    strebuf << ""                        << "\n";
    strebuf << "2"                       << "\n";
    strebuf << "0.0 1.0"                 << "\n";
    strebuf << "0.0 1.0"                 << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
    strebuf << "3"                       << "\n";
    strebuf << "0.0 0.25 1.0"            << "\n";
    strebuf << "0.0 0.25 1.0"            << "\n";
    strebuf << ""                        << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.0 0.0"             << "\n";
    strebuf << "0.2 0.3 0.1"             << "\n";
    strebuf << "0.4 0.5 0.2"             << "\n";
    strebuf << "0.5 0.6 0.3"             << "\n";
    strebuf << "0.6 0.8 0.4"             << "\n";
    strebuf << "1.0 0.9 0.5"             << "\n";
    
    // TODO: add the prelut data here
    float red[6]   = { 0.0, 0.2, 0.4, 0.5, 0.6, 1.0 };
    float green[6] = { 0.0, 0.3, 0.5, 0.6, 0.8, 0.9 };
    float blue[6]  = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5 };
    
    std::istringstream simple1D;
    simple1D.str(strebuf.str());
    
    // Load file
    OCIO::FileFormatCSP tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple1D);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check prelut data
    // NOT DONE YET
    
    // check 1D data
    // red
    unsigned int i;
    for(i = 0; i < csplut->lut1D->luts[0].size(); ++i)
        BOOST_CHECK_EQUAL (red[i], csplut->lut1D->luts[0][i]);
    // green
    for(i = 0; i < csplut->lut1D->luts[1].size(); ++i)
        BOOST_CHECK_EQUAL (green[i], csplut->lut1D->luts[1][i]);
    // blue
    for(i = 0; i < csplut->lut1D->luts[2].size(); ++i)
        BOOST_CHECK_EQUAL (blue[i], csplut->lut1D->luts[2][i]);
    
}


BOOST_AUTO_TEST_CASE ( test_simple3D )
{
    
    //
    std::ostringstream strebuf;
    strebuf << "CSPLUTV100"                                  << "\n";
    strebuf << "3D"                                          << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "BEGIN METADATA"                              << "\n";
    strebuf << "foobar"                                      << "\n";
    strebuf << "END METADATA"                                << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "11"                                          << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "6"                                           << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0"                     << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0"                     << "\n";
    strebuf << "5"                                           << "\n";
    strebuf << "0.0 0.25 0.5 0.75 1.0"                       << "\n";
    strebuf << "0.0 0.25 0.5 0.75 1.0"                       << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "1 2 3"                                       << "\n";
    strebuf << "0.0 0.0 0.0"                                 << "\n";
    strebuf << "1.0 0.0 0.0"                                 << "\n";
    strebuf << "0.0 0.5 0.0"                                 << "\n";
    strebuf << "1.0 0.5 0.0"                                 << "\n";
    strebuf << "0.0 1.0 0.0"                                 << "\n";
    strebuf << "1.0 1.0 0.0"                                 << "\n";
    
    // TODO: add the prelut data here
    // TODO: check how we read from csp into gl memory layout
    float cube[1 * 2 * 3 * 3] = { 0.0, 0.0, 0.0,
                                  1.0, 0.5, 0.0,
                                  1.0, 0.0, 0.0,
                                  0.0, 1.0, 0.0,
                                  0.0, 0.5, 0.0,
                                  1.0, 1.0, 0.0 };
    
    std::istringstream simple3D;
    simple3D.str(strebuf.str());
    
    // Load file
    OCIO::FileFormatCSP tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple3D);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check prelut data
    // NOT DONE YET
    
    // check cube data
    unsigned int i;
    for(i = 0; i < csplut->lut3D->lut.size(); ++i) {
        BOOST_CHECK_EQUAL (cube[i], csplut->lut3D->lut[i]);
    }
    
}
