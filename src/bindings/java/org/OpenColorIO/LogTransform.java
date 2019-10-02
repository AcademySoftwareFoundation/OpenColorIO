// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class LogTransform extends Transform
{
    public LogTransform() { super(); }
    protected LogTransform(long impl) { super(impl); }
    public native LogTransform Create();
    public native void setBase(float val);
    public native float getBase();
}
