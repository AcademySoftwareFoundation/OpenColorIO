// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class GpuShaderDesc extends LoadLibrary
{
    public GpuShaderDesc() { super(); create(); }
    protected GpuShaderDesc(long impl) { super(impl); }
    private native void create();
    public native void dispose();
    protected void finalize() { dispose(); }
    public native void setLanguage(GpuLanguage lang);
    public native GpuLanguage getLanguage();
    public native void setFunctionName(String name);
    public native String getFunctionName();
    public native void setLut3DEdgeLen(int len);
    public native int getLut3DEdgeLen();
    public native String getCacheID();
};
