// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class Interpolation extends LoadLibrary
{
    private final int m_enum;
    protected Interpolation(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final Interpolation
        INTERP_UNKNOWN = new Interpolation(0);
    public static final Interpolation
        INTERP_NEAREST = new Interpolation(1);
    public static final Interpolation
        INTERP_LINEAR = new Interpolation(2);
    public static final Interpolation
        INTERP_TETRAHEDRAL = new Interpolation(3);
    public static final Interpolation
        INTERP_BEST = new Interpolation(255);
}
