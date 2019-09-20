// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

package org.OpenColorIO;
import org.OpenColorIO.*;

public class LoggingLevel extends LoadLibrary
{
    private final int m_enum;
    protected LoggingLevel(int type) { super(); m_enum = type; }
    public native String toString();
    public native boolean equals(Object obj);
    public static final LoggingLevel
        LOGGING_LEVEL_NONE = new LoggingLevel(0);
    public static final LoggingLevel
        LOGGING_LEVEL_WARNING = new LoggingLevel(1);
    public static final LoggingLevel
        LOGGING_LEVEL_INFO = new LoggingLevel(2);
    public static final LoggingLevel
        LOGGING_LEVEL_DEBUG = new LoggingLevel(3);
    public static final LoggingLevel
        LOGGING_LEVEL_UNKNOWN = new LoggingLevel(255);
}
