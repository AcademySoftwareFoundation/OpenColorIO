// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class MatrixTransform extends Transform
{
    public MatrixTransform() { super(); }
    protected MatrixTransform(long impl) { super(impl); }
    public native MatrixTransform Create();
    public native boolean equals(MatrixTransform obj);
    public native void setValue(float[] m44, float[] offset4);
    public native void getValue(float[] m44, float[] offset4);
    public native void setMatrix(float[] m44);
    public native void getMatrix(float[] m44);
    public native void setOffset(float[] offset4);
    public native void getOffset(float[] offset4);
    public native void Fit(float[] m44, float[] offset4,
                           float[] oldmin4, float[] oldmax4,
                           float[] newmin4, float[] newmax4);
    public native void Identity(float[] m44, float[] offset4);
    public native void Sat(float[] m44, float[] offset4,
                           float sat, float[] lumaCoef3);
    public native void Scale(float[] m44, float[] offset4,
                             float[] scale4);
    public native void View(float[] m44, float[] offset4,
                            int[] channelHot4, float[] lumaCoef3);
}
