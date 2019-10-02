// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class Transform extends LoadLibrary
{
    public Transform() { super(); }
    protected Transform(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    public native Transform createEditableCopy();
    public native TransformDirection getDirection();
    public native void setDirection(TransformDirection dir);
}
