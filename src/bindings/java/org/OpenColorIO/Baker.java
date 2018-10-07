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

public class Baker extends LoadLibrary
{
    public Baker() { super(); }
    protected Baker(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native Baker Create();
    public native Baker createEditableCopy();
    public native void setConfig(Config config);
    public native Config getConfig();
    public native void setFormat(String formatName);
    public native String getFormat();
    public native void setType(String type);
    public native String getType();
    public native void setMetadata(String metadata);
    public native String getMetadata();
    public native void setInputSpace(String inputSpace);
    public native String getInputSpace();
    public native void setShaperSpace(String shaperSpace);
    public native String getShaperSpace();
    public native void setLooks(String looks);
    public native String getLooks();
    public native void setTargetSpace(String targetSpace);
    public native String getTargetSpace();
    public native void setShaperSize(int shapersize);
    public native int getShaperSize();
    public native void setCubeSize(int cubesize);
    public native int getCubeSize();
    public native String bake();
    public native int getNumFormats();
    public native String getFormatNameByIndex(int index);
    public native String getFormatExtensionByIndex(int index);
}
