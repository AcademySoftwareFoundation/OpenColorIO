// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class Look extends LoadLibrary
{
    public Look() { super(); }
    protected Look(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native Look Create();
    public native String getName();
    public native void setName(String name);
    public native String getProcessSpace();
    public native void setProcessSpace(String processSpace);
    public native String getDescription();
    public native void setDescription(String description);
    public native Transform getTransform();
    public native void setTransform(Transform transform);
    public native Transform getInverseTransform();
    public native void setInverseTransform(Transform transform);
};

