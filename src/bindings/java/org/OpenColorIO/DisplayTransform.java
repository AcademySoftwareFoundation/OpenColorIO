// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class DisplayTransform extends Transform
{
    public DisplayTransform() { super(); }
    protected DisplayTransform(long impl) { super(impl); }
    public native DisplayTransform Create();
    public native void setInputColorSpaceName(String name);
    public native String getInputColorSpaceName();
    public native void setLinearCC(Transform cc);
    public native Transform getLinearCC();
    public native void setColorTimingCC(Transform cc);
    public native Transform getColorTimingCC();
    public native void setChannelView(Transform transform);
    public native Transform getChannelView();
    public native void setDisplay(String display);
    public native String getDisplay();
    public native void setView(String view);
    public native String getView();
    public native void setDisplayCC(Transform cc);
    public native Transform getDisplayCC();
    public native void setLooksOverride(String looks);
    public native String getLooksOverride();
    public native void setLooksOverrideEnabled(boolean enabled);
    public native boolean getLooksOverrideEnabled();
}
