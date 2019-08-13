// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class EnvironmentMode extends LoadLibrary
{
    private final int m_enum;
    protected EnvironmentMode(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final EnvironmentMode
        ENV_ENVIRONMENT_UNKNOWN = new EnvironmentMode(0);
    public static final EnvironmentMode
        ENV_ENVIRONMENT_LOAD_PREDEFINED = new EnvironmentMode(1);
    public static final EnvironmentMode
        ENV_ENVIRONMENT_LOAD_ALL = new EnvironmentMode(2);
}
