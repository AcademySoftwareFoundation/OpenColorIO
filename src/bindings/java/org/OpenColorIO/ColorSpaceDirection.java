// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class ColorSpaceDirection extends LoadLibrary
{
    private final int m_enum;
    protected ColorSpaceDirection(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final ColorSpaceDirection
      COLORSPACE_DIR_UNKNOWN = new ColorSpaceDirection(0);
    public static final ColorSpaceDirection
      COLORSPACE_DIR_TO_REFERENCE = new ColorSpaceDirection(1);
    public static final ColorSpaceDirection
      COLORSPACE_DIR_FROM_REFERENCE = new ColorSpaceDirection(2);
}
