// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class FileTransform extends Transform
{
    public FileTransform() { super(); }
    protected FileTransform(long impl) { super(impl); }
    public native FileTransform Create();
    public native String getSrc();
    public native void setSrc(String src);
    public native String getCCCId();
    public native void setCCCId(String id);
    public native Interpolation getInterpolation();
    public native void setInterpolation(Interpolation interp);
    public native int getNumFormats();
    public native String getFormatNameByIndex(int index);
    public native String getFormatExtensionByIndex(int index);
}
