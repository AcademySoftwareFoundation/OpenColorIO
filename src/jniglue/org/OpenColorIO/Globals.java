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

package org.OpenColorIO;
import org.OpenColorIO.*;

public class Globals extends LoadLibrary
{
    public native void ClearAllCaches();
    public native String GetVersion();
    public native int GetVersionHex();
    public native LoggingLevel GetLoggingLevel();
    public native void SetLoggingLevel(LoggingLevel level);
    public native Config GetCurrentConfig();
    public native void SetCurrentConfig(Config config);
    
    // Types
    public native String BoolToString(boolean val);
    public native boolean BoolFromString(String s);
    public native String LoggingLevelToString(LoggingLevel level);
    public native LoggingLevel LoggingLevelFromString(String s);
    public native String TransformDirectionToString(TransformDirection dir);
    public native TransformDirection TransformDirectionFromString(String s);
    public native TransformDirection GetInverseTransformDirection(TransformDirection dir);
    public native TransformDirection CombineTransformDirections(TransformDirection d1, TransformDirection d2);
    public native String ColorSpaceDirectionToString(ColorSpaceDirection dir);
    public native ColorSpaceDirection ColorSpaceDirectionFromString(String s);
    public native String BitDepthToString(BitDepth bitDepth);
    public native BitDepth BitDepthFromString(String s);
    public native boolean BitDepthIsFloat(BitDepth bitDepth);
    public native int BitDepthToInt(BitDepth bitDepth);
    public native String AllocationToString(Allocation allocation);
    public native Allocation AllocationFromString(String s);
    public native String InterpolationToString(Interpolation interp);
    public native Interpolation InterpolationFromString(String s);
    public native String GpuLanguageToString(GpuLanguage language);
    public native GpuLanguage GpuLanguageFromString(String s);
    public native String EnvironmentModeToString(EnvironmentMode mode);
    public native GpuLanguage EnvironmentModeFromString(String s);
    
    // Roles
    public String ROLE_DEFAULT;
    public String ROLE_REFERENCE;
    public String ROLE_DATA;
    public String ROLE_COLOR_PICKING;
    public String ROLE_SCENE_LINEAR;
    public String ROLE_COMPOSITING_LOG;
    public String ROLE_COLOR_TIMING;
    public String ROLE_TEXTURE_PAINT;
    public String ROLE_MATTE_PAINT;
    
    //
    public Globals() { super(); create(); }
    private native void create();
}
