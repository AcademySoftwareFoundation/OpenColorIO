..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Context
*******

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class Context**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ContextRcPtr createEditableCopy() const

      .. cpp:function:: const char *getCacheID() const

      .. cpp:function:: void setSearchPath(const char *path)

      .. cpp:function:: const char *getSearchPath() const

      .. cpp:function:: int getNumSearchPaths() const

      .. cpp:function:: const char *getSearchPath(int index) const

      .. cpp:function:: void clearSearchPaths()

      .. cpp:function:: void addSearchPath(const char *path)

      .. cpp:function:: void setWorkingDir(const char *dirname)

      .. cpp:function:: const char *getWorkingDir() const

      .. cpp:function:: void setStringVar(const char *name, const char *value)

      .. cpp:function:: const char *getStringVar(const char *name) const

      .. cpp:function:: int getNumStringVars() const

      .. cpp:function:: const char *getStringVarNameByIndex(int index) const

      .. cpp:function:: void clearStringVars()

      .. cpp:function:: void setEnvironmentMode(EnvironmentMode mode)

      .. cpp:function:: EnvironmentMode getEnvironmentMode() const

      .. cpp:function:: void loadEnvironment()

         Seed all string vars with the current environment.

      .. cpp:function:: const char *resolveStringVar(const char *val) const

         Do a string lookup. Do a file lookup.

         Evaluate the specified variable (as needed). Will not throw
         exceptions.

      .. cpp:function:: const char *resolveFileLocation(const char *filename) const

         Do a file lookup. Do a file lookup.

         Evaluate all variables (as needed). Also, walk the full search
         path until the file is found. If the filename cannot be found,
         an exception will be thrown.

      .. cpp:function:: ~Context()

      -[ Public Static Functions ]-

      .. cpp:function:: ContextRcPtr Create()


   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.Context

      .. py:class:: SearchPathIterator

      .. py:class:: StringVarIterator

      .. py:class:: StringVarNameIterator

      .. py:method:: addSearchPath(self: PyOpenColorIO.Context, path: str) -> None

      .. py:method:: clearSearchPaths(self: PyOpenColorIO.Context) -> None

      .. py:method:: clearStringVars(self: PyOpenColorIO.Context) -> None

      .. py:method:: getCacheID(self: PyOpenColorIO.Context) -> str

      .. py:method:: getEnvironmentMode(self: PyOpenColorIO.Context) -> PyOpenColorIO.EnvironmentMode

      .. py:method:: getSearchPath(self: PyOpenColorIO.Context) -> str

      .. py:method:: getSearchPaths(self: PyOpenColorIO.Context) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Context>, 2>

      .. py:method:: getStringVars(self: PyOpenColorIO.Context) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Context>, 1>

      .. py:method:: getWorkingDir(self: PyOpenColorIO.Context) -> str

      .. py:method:: loadEnvironment(self: PyOpenColorIO.Context) -> None

      .. py:method:: resolveFileLocation(self: PyOpenColorIO.Context, fileName: str) -> str

      .. py:method:: resolveStringVar(self: PyOpenColorIO.Context, value: str) -> str

      .. py:method:: setEnvironmentMode(self: PyOpenColorIO.Context, mode: PyOpenColorIO.EnvironmentMode) -> None

      .. py:method:: setSearchPath(self: PyOpenColorIO.Context, path: str) -> None

      .. py:method:: setWorkingDir(self: PyOpenColorIO.Context, dirName: str) -> None
