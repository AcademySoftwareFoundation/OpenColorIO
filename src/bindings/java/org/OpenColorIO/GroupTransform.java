// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class GroupTransform extends Transform
{
    public GroupTransform() { super(); }
    protected GroupTransform(long impl) { super(impl); }
    public native GroupTransform Create();
    public native Transform getTransform(int index);
    public native int size();
    public native void push_back(Transform transform);
    public native void clear();
    public native boolean empty();
}
