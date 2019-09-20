// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class Allocation extends LoadLibrary
{
    private final int m_enum;
    protected Allocation(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final Allocation
        ALLOCATION_UNKNOWN = new Allocation(0);
    public static final Allocation
        ALLOCATION_UNIFORM = new Allocation(1);
    public static final Allocation
        ALLOCATION_LG2 = new Allocation(2);
}
