// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;
import java.nio.FloatBuffer;

public class PlanarImageDesc extends ImageDesc
{
    public PlanarImageDesc(FloatBuffer rData, FloatBuffer gData, FloatBuffer bData,
                           FloatBuffer aData, long width, long height)
    {
        super();
        create(rData, gData, bData, aData, width, height);
    }
    public PlanarImageDesc(FloatBuffer rData, FloatBuffer gData, FloatBuffer bData,
                           FloatBuffer aData, long width, long height,
                           long yStrideBytes)
    {
        super();
        create(rData, gData, bData, aData, width, height, yStrideBytes);
    }
    protected PlanarImageDesc(long impl) { super(impl); }
    public native void dispose();
    protected void finalize() { dispose(); }
    protected native void create(FloatBuffer rData, FloatBuffer gData, FloatBuffer bData,
                                 FloatBuffer aData, long width, long height);
    protected native void create(FloatBuffer rData, FloatBuffer gData, FloatBuffer bData,
                                 FloatBuffer aData, long width, long height,
                                 long yStrideBytes);
    public native FloatBuffer getRData();
    public native FloatBuffer getGData();
    public native FloatBuffer getBData();
    public native FloatBuffer getAData();
    public native long getWidth();
    public native long getHeight();
    public native long getYStrideBytes();
};
