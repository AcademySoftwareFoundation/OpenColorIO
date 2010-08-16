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


#ifndef INCLUDED_OCIO_CONFIG_H
#define INCLUDED_OCIO_CONFIG_H

#include <OpenColorIO/OpenColorIO.h>

#include <utility>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    typedef std::vector<ColorSpaceRcPtr> ColorSpacePtrVec;
    typedef std::vector< std::pair<std::string, std::string> > RoleVec;
    
    typedef std::vector<std::string> DisplayKey; // (device, name, colorspace)
    
    class Config::Impl
    {
    public:
        Impl();
        ~Impl();
        
        Impl& operator= (const Impl &);
        
        const char * getResourcePath() const;
        void setResourcePath(const char * path);
        const char * getResolvedResourcePath() const;
        
        const char * getDescription() const;
        void setDescription(const char * description);
        
        void createFromEnv();
        void createFromFile(const char * filename);
        
        void writeXML(std::ostream& os) const;
        
        
        
        // COLORSPACES ////////////////////////////////////////////////////////
        
        int getNumColorSpaces() const;
        
        int getIndexForColorSpace(const char * name) const;
        
        // if another colorspace was already registered with the
        // same name, this will overwrite it.
        // Stores the live reference to this colorspace
        void addColorSpace(ColorSpaceRcPtr cs);
        
        // Stores a copy
        void addColorSpace(const ConstColorSpaceRcPtr & cs);
        
        void clearColorSpaces();
        
        ConstColorSpaceRcPtr getColorSpaceByIndex(int index) const;
        ColorSpaceRcPtr getEditableColorSpaceByIndex(int index);
        
        ConstColorSpaceRcPtr getColorSpaceByName(const char * name) const;
        ColorSpaceRcPtr getEditableColorSpaceByName(const char * name);
        
        const char * parseColorSpaceFromString(const char * str) const;
        
        
        // Roles
        ConstColorSpaceRcPtr getColorSpaceForRole(const char * role) const;
        void setColorSpaceForRole(const char * role, const char * name);
        void unsetRole(const char * role);
        
        int getNumRoles() const;
        const char * getRole(int index) const;
        
        
        // Display Transforms
        int getNumDisplayDevices() const;
        const char * getDisplayDevice(int index) const;
        const char * getDefaultDisplayDevice() const;
        
        int getNumDisplayTransformNames(const char * device) const;
        const char * getDisplayTransformName(const char * device, int index) const;
        const char * getDefaultDisplayTransformName(const char * device) const;
        
        const char * getDisplayColorspace(const char * device, const char * displayTransformName) const;
        
        void addDisplayDevice(const char * device,
                              const char * displayTransformName,
                              const char * csname);
        
        
        // Luma
        void getDefaultLumaCoefs(float * c3) const;
        void setDefaultLumaCoefs(const float * c3);
        
    private:
        Impl(const Impl &);
        
        std::string m_originalFileDir;
        std::string m_resourcePath;
        std::string m_resolvedResourcePath;
        
        std::string m_description;
        
        ColorSpacePtrVec m_colorspaces;
        
        RoleVec m_roleVec;
        
        std::vector<DisplayKey> m_displayDevices;
        
        float m_defaultLumaCoefs[3];
    };
}
OCIO_NAMESPACE_EXIT

#endif
