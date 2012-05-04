/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
