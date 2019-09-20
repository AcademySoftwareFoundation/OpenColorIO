// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class LookTransform extends Transform
{
    public LookTransform() { super(); }
    protected LookTransform(long impl) { super(impl); }
    public native LookTransform Create();
    public native String getSrc();
    public native void setSrc(String src);
    public native String getDst();
    public native void setDst(String dst);
    public native void setLooks(String looks);
    public native String getLooks();
}
