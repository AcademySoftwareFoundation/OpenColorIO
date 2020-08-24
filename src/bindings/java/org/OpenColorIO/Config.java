// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    public native void validate();
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
