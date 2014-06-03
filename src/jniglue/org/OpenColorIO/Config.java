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

public class Config extends LoadLibrary
{
    public Config() { super(); }
    protected Config(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native Config Create();
    public native Config CreateFromEnv();
    public native Config CreateFromFile(String filename);
    public native Config CreateFromStream(String istream);
    public native Config createEditableCopy();
    public native void sanityCheck();
    public native String getDescription();
    public native void setDescription(String description);
    public native String serialize();
    public native String getCacheID();
    public native String getCacheID(Context context);
    public native Context getCurrentContext();
    public native void addEnvironmentVar(String name, String defaultValue);
    public native int getNumEnvironmentVars();
    public native String getEnvironmentVarNameByIndex(int index);
    public native String getEnvironmentVarDefault(String name);
    public native void clearEnvironmentVars();
    public native String getSearchPath();
    public native void setSearchPath(String path);
    public native String getWorkingDir();
    public native void setWorkingDir(String dirname);
    public native int getNumColorSpaces();
    public native String getColorSpaceNameByIndex(int index);
    public native ColorSpace getColorSpace(String name);
    public native int getIndexForColorSpace(String name);
    public native void addColorSpace(ColorSpace cs);
    public native void clearColorSpaces();
    public native String parseColorSpaceFromString(String str);
    public native boolean isStrictParsingEnabled();
    public native void setStrictParsingEnabled(boolean enabled);
    public native void setRole(String role, String colorSpaceName);
    public native int getNumRoles();
    public native boolean hasRole(String role);
    public native String getRoleName(int index);
    public native String getDefaultDisplay();
    public native int getNumDisplays();
    public native String getDisplay(int index);
    public native int getNumDisplaysActive();
    public native String getDisplayActive(int index);
    public native int getNumDisplaysAll();
    public native String getDisplayAll(int index);
    public native String getDefaultView(String display);
    public native int getNumViews(String display);
    public native String getView(String display, int index);
    public native String getDisplayColorSpaceName(String display, String view);
    public native String getDisplayLooks(String display, String view);
    // TODO: seems that 4 string params causes a memory error in the JNI layer?
    // public native void addDisplay(String display, String view, String colorSpaceName, int looks);
    public native void clearDisplays();
    public native void setActiveDisplays(String displays);
    public native String getActiveDisplays();
    public native void setActiveViews(String views);
    public native String getActiveViews();
    public native void getDefaultLumaCoefs(float[] rgb);
    public native void setDefaultLumaCoefs(float[] rgb);
    public native Look getLook(String name);
    public native int getNumLooks();
    public native String getLookNameByIndex(int index);
    public native void addLook(Look look);
    public native void clearLooks();
    public native Processor getProcessor(Context context, ColorSpace srcColorSpace, ColorSpace dstColorSpace);
    public native Processor getProcessor(ColorSpace srcColorSpace, ColorSpace dstColorSpace);
    public native Processor getProcessor(String srcName, String dstName);
    public native Processor getProcessor(Context context, String srcName, String dstName);
    public native Processor getProcessor(Transform transform);
    public native Processor getProcessor(Transform transform, TransformDirection direction);
    public native Processor getProcessor(Context context, Transform transform, TransformDirection direction);
}
