// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
