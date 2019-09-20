// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
