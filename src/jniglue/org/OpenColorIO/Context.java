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

public class Context extends LoadLibrary
{
    public Context() { super(); }
    protected Context(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native Context Create();
    public native Context createEditableCopy();
    public native String getCacheID();
    public native void setSearchPath(String path);
    public native String getSearchPath();
    public native void setWorkingDir(String dirname);
    public native String getWorkingDir();
    public native void setStringVar(String name, String value);
    public native String getStringVar(String name);
    public native int getNumStringVars();
    public native String getStringVarNameByIndex(int index);
    public native void clearStringVars();
    public native void setEnvironmentMode(EnvironmentMode mode);
    public native EnvironmentMode getEnvironmentMode();
    public native void loadEnvironment();
    public native String resolveStringVar(String val);
    public native String resolveFileLocation(String filename) throws ExceptionMissingFile;
}
