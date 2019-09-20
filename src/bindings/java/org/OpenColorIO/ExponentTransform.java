// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class ExponentTransform extends Transform
{
    public ExponentTransform() { super(); }
    protected ExponentTransform(long impl) { super(impl); }
    public native ExponentTransform Create();
    public native void setValue(float[] vec4);
    public native void getValue(float[] vec4);
}
