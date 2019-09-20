// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class BitDepth extends LoadLibrary
{
    private final int m_enum;
    protected BitDepth(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final BitDepth
        BIT_DEPTH_UNKNOWN = new BitDepth(0);
    public static final BitDepth
        BIT_DEPTH_UINT8 = new BitDepth(1);
    public static final BitDepth
        BIT_DEPTH_UINT10 = new BitDepth(2);
    public static final BitDepth
        BIT_DEPTH_UINT12 = new BitDepth(3);
    public static final BitDepth
        BIT_DEPTH_UINT14 = new BitDepth(4);
    public static final BitDepth
        BIT_DEPTH_UINT16 = new BitDepth(5);
    public static final BitDepth
        BIT_DEPTH_UINT32 = new BitDepth(6);
    public static final BitDepth
        BIT_DEPTH_F16 = new BitDepth(7);
    public static final BitDepth
        BIT_DEPTH_F32 = new BitDepth(8);
}
