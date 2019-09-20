// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class TransformDirection extends LoadLibrary
{
    private final int m_enum;
    protected TransformDirection(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final TransformDirection
        TRANSFORM_DIR_UNKNOWN = new TransformDirection(0);
    public static final TransformDirection
        TRANSFORM_DIR_FORWARD = new TransformDirection(1);
    public static final TransformDirection
        TRANSFORM_DIR_INVERSE = new TransformDirection(2);
}
