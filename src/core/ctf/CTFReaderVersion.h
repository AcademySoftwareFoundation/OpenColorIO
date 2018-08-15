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


#ifndef INCLUDED_OCIO_CTFREADERVERSION_H
#define INCLUDED_OCIO_CTFREADERVERSION_H

// TODO: rename the file to remove Reader

#include <OpenColorIO/OpenColorIO.h>
#include <ostream>

OCIO_NAMESPACE_ENTER
{
// Private namespace to the CTF sub-directory
namespace CTF
{
class Version
{
public:
    // Will throw if versionString is not formatted like a version
    static void ReadVersion(const std::string & versionString, Version & versionOut);

    Version()
        : m_major(0)
        , m_minor(0)
        , m_revision(0)
    {
    }
    Version(int major, int minor, int revision)
        : m_major(major)
        , m_minor(minor)
        , m_revision(revision)
    {
    }
    Version(int major, int minor)
        : m_major(major)
        , m_minor(minor)
        , m_revision(0)
    {
    }

    Version(const Version &otherVersion)
        : m_major(otherVersion.m_major)
        , m_minor(otherVersion.m_minor)
        , m_revision(otherVersion.m_revision)
    {
    }

    Version & operator=(const Version &rhs);

    bool operator<(const Version &rhs) const;
    bool operator<=(const Version &rhs) const;
    bool operator==(const Version &rhs) const;

    ~Version() {}

    friend std::ostream& operator<< (std::ostream& stream, const Version& rhs)
    {
        stream << rhs.m_major;
        if (rhs.m_minor != 0 || rhs.m_revision != 0)
        {
            stream << "." << rhs.m_minor;
            if (rhs.m_revision != 0)
            {
                stream << "." << rhs.m_revision;
            }
        }
        return stream;
    }

private:
    int m_major;
    int m_minor;
    int m_revision;
};

//
// Process List Version
//

// Version 1.2 2012 initial Autodesk version
static const Version CTF_PROCESS_LIST_VERSION_1_2 = Version(1, 2);

// Version 1.3 2012-12 revised matrix
static const Version CTF_PROCESS_LIST_VERSION_1_3 = Version(1, 3);

// Version 1.4 2013-07 adds ACES v0.2
static const Version CTF_PROCESS_LIST_VERSION_1_4 = Version(1, 4);

// Version 1.5 2014-01 adds ACES v0.7
static const Version CTF_PROCESS_LIST_VERSION_1_5 = Version(1, 5);

// Version 1.6 2014-05 adds functionOp, invLut3D
static const Version CTF_PROCESS_LIST_VERSION_1_6 = Version(1, 6);

// Version 1.7 2015-01 adds 'invert' flag to referenceOp and to the transform,
// adds 1.0 styles to ACES op, adds CLF support (IndexMap, alt. Range, CDL styles)
static const Version CTF_PROCESS_LIST_VERSION_1_7 = Version(1, 7);

// Version 1.8 2017-10 adds FunctionOp as a valid element in CTF files,
// adds grading ops and new dynamic parameter framework
static const Version CTF_PROCESS_LIST_VERSION_1_8 = Version(1, 8);

// TODO: follow CTF format changes
// static const Version CTF_PROCESS_LIST_VERSION_1_9 = Version(1, 9);

// .. Add new version before this line
//     and do not forget to update the following line
static const Version CTF_PROCESS_LIST_VERSION = CTF_PROCESS_LIST_VERSION_1_8;

}


//
// Info Element Version
//

// Version 1.0 initial Autodesk version
#define CTF_INFO_ELEMENT_VERSION_1_0 1.0f

// Version 2.0 2017 Ext1
#define CTF_INFO_ELEMENT_VERSION_2_0 2.0f

// .. Add new version before this line
//     and do not forget to update the following line

#define CTF_INFO_ELEMENT_VERSION CTF_INFO_ELEMENT_VERSION_2_0

}
OCIO_NAMESPACE_EXIT

#endif
