
Globals
*******

Methods
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: void OpenColorIO::ClearAllCaches ()

         OpenColorIO, during normal usage, tends to cache certain information (such as the contents of LUTs on disk, intermediate results, etc.). Calling this function will flush all such information. Under normal usage, this is not necessary, but it can be helpful in particular instances, such as designing OCIO profiles, and wanting to re-read luts without restarting. 

      .. cpp:function:: const char * OpenColorIO::GetVersion ()

         Get the version number for the library, as a dot-delimited string (e.g., “1.0.0”). 

         This is also available at compile time as OCIO_VERSION. 

      .. cpp:function:: int OpenColorIO::GetVersionHex ()

         Get the version number for the library, as a single 4-byte hex number (e.g., 0x01050200 for “1.5.2”), to be used for numeric comparisons. 

         This is also at compile time as OCIO_VERSION_HEX. 

      .. cpp:function:: LoggingLevel OpenColorIO::GetLoggingLevel ()

         Get the global logging level. 

         You can override this at runtime using the OCIO_LOGGING_LEVEL environment variable. The client application that sets this should use SetLoggingLevel, and not the environment variable. The default value is INFO. 

      .. cpp:function:: void OpenColorIO::SetLoggingLevel (LoggingLevel level)

         Set the global logging level. 

      .. cpp:function:: void OpenColorIO::SetLoggingFunction (LoggingFunction logFunction)

         Set the logging function to use; otherwise, use the default (i.e. std::cerr). 

         **Note**
            The logging mechanism is thread-safe. 

      .. cpp:function:: void OpenColorIO::ResetToDefaultLoggingFunction ()

      .. cpp:function:: void OpenColorIO::LogMessage (LoggingLevel level, const char *message)

         Log a message using the library logging function. 

      .. cpp:function:: const char * OpenColorIO::GetEnvVariable (const char *name)

         Call modifies the string obtained from a previous call as the method always uses the same memory buffer. 

         **Warning**
            This method is not thread safe. 

      .. cpp:function:: void OpenColorIO::SetEnvVariable (const char *name, const char *value)

         **Warning**
            This method is not thread safe. 

      **Exceptions**

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Exception” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::ExceptionMissingFile” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml


**Exceptions**
