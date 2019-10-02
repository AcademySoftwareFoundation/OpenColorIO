// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class CDLTransform extends Transform
{
    public CDLTransform() { super(); }
    protected CDLTransform(long impl) { super(impl); }
    public native CDLTransform Create();
    public native CDLTransform CreateFromFile(String src, String cccid);
    public native boolean equals(CDLTransform obj);
    public native String getXML();
    public native void setXML(String xml);
    public native void setSlope(float[] rgb);
    public native void getSlope(float[] rgb);
    public native void setOffset(float[] rgb);
    public native void getOffset(float[] rgb);
    public native void setPower(float[] rgb);
    public native void getPower(float[] rgb);
    public native void setSOP(float[] vec9);
    public native void getSOP(float[] vec9);
    public native void setSat(float sat);
    public native float getSat();
    public native void getSatLumaCoefs(float[] rgb);
    public native void setID(String id);
    public native String getID();
    public native void setDescription(String desc);
    public native String getDescription();
}
