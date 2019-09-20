// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
