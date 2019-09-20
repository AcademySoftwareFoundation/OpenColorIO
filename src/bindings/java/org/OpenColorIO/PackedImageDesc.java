// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;
import java.nio.FloatBuffer;

public class PackedImageDesc extends ImageDesc
{
    public PackedImageDesc(FloatBuffer data, long width, long height, long numChannels)
    {
        super();
        create(data, width, height, numChannels);
    }
    public PackedImageDesc(FloatBuffer data, long width, long height, long numChannels,
                           long chanStrideBytes, long xStrideBytes, long yStrideBytes)
    {
        super();
        create(data, width, height, numChannels, chanStrideBytes, xStrideBytes, yStrideBytes);
    }
    protected PackedImageDesc(long impl) { super(impl); }
    protected native void create(FloatBuffer data, long width, long height, long numChannels);
    protected native void create(FloatBuffer data, long width, long height, long numChannels,
                                 long chanStrideBytes, long xStrideBytes, long yStrideBytes);
    public native void dispose();
    protected void finalize() { dispose(); }
    public native FloatBuffer getData();
    public native long getWidth();
    public native long getHeight();
    public native long getNumChannels();
    public native long getChanStrideBytes();
    public native long getXStrideBytes();
    public native long getYStrideBytes();
};
