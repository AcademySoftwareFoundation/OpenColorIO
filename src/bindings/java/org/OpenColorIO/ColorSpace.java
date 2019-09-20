// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class ColorSpace extends LoadLibrary
{
    public ColorSpace() { super(); }
    protected ColorSpace(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native ColorSpace Create();
    public native ColorSpace createEditableCopy();
    public native String getName();
    public native void setName(String name);
    public native String getFamily();
    public native void setFamily(String family);
    public native String getEqualityGroup();
    public native void setEqualityGroup(String equalityGroup);
    public native String getDescription();
    public native void setDescription(String description);
    public native BitDepth getBitDepth();
    public native void setBitDepth(BitDepth bitDepth);
    public native boolean isData();
    public native void setIsData(boolean isData);
    public native Allocation getAllocation();
    public native void setAllocation(Allocation allocation);
    public native int getAllocationNumVars();
    public native void getAllocationVars(float[] vars);
    public native void setAllocationVars(int numvars, float[] vars);
    public native Transform getTransform(ColorSpaceDirection dir);
    public native void setTransform(Transform transform, ColorSpaceDirection dir);
}
