// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class GpuLanguage extends LoadLibrary
{
    private final int m_enum;
    protected GpuLanguage(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final GpuLanguage
      GPU_LANGUAGE_UNKNOWN = new GpuLanguage(0);
    public static final GpuLanguage
      GPU_LANGUAGE_CG = new GpuLanguage(1);
    public static final GpuLanguage
      GPU_LANGUAGE_GLSL_1_0 = new GpuLanguage(2);
    public static final GpuLanguage
      GPU_LANGUAGE_GLSL_1_3 = new GpuLanguage(3);
}
