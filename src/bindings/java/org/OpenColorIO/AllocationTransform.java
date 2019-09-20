// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class AllocationTransform extends Transform
{
    public AllocationTransform() { super(); }
    protected AllocationTransform(long impl) { super(impl); }
    public native AllocationTransform Create();
    public native Allocation getAllocation();
    public native void setAllocation(Allocation allocation);
    public native int getNumVars();
    public native void getVars(float[] vars);
    public native void setVars(int numvars, float[] vars);
}
