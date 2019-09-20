// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class ColorSpaceTransform extends Transform
{
    public ColorSpaceTransform() { super(); }
    protected ColorSpaceTransform(long impl) { super(impl); }
    public native ColorSpaceTransform Create();
    public native String getSrc();
    public native void setSrc(String src);
    public native String getDst();
    public native void setDst(String dst);
}
