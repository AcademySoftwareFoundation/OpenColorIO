// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;
import java.nio.FloatBuffer;

public class Processor extends LoadLibrary
{
    public Processor() { super(); }
    protected Processor(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native Processor Create();
    public native boolean isNoOp();
    public native boolean hasChannelCrosstalk();
    public native void apply(ImageDesc img);
    public native void applyRGB(float[] pixel);
    public native void applyRGBA(float[] pixel);
    public native String getCpuCacheID();
    public native String getGpuShaderText(GpuShaderDesc shaderDesc);
    public native String getGpuShaderTextCacheID(GpuShaderDesc shaderDesc);
    public native void getGpuLut3D(FloatBuffer lut3d, GpuShaderDesc shaderDesc);
    public native String getGpuLut3DCacheID(GpuShaderDesc shaderDesc);
};

