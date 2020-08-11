
Context
*******

.. class:: Context


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ContextRcPtr OpenColorIO::Context::Create()

      .. cpp:function:: ContextRcPtr OpenColorIO::Context::createEditableCopy() const

Python

.. py:method:: Context.__init__(*args, **kwargs)

   Overloaded function.

   1. __init__(self: PyOpenColorIO.Context) -> None

   2. __init__(self: PyOpenColorIO.Context, workingDir: str = ‘’, searchPaths: List[str] = [], stringVars: Dict[str, str] = {}, environmentMode: PyOpenColorIO.EnvironmentMode = EnvironmentMode.ENV_ENVIRONMENT_LOAD_PREDEFINED) -> None


Methods
=======

.. tabs::

   .. group-tab:: C++

      **Cache**

      .. cpp:function:: const char *OpenColorIO::Context::getCacheID() const

      **Search Paths**

      .. cpp:function:: void OpenColorIO::Context::setSearchPath(const char *path)

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Context::getSearchPath” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - const char *getSearchPath() const
            - const char *getSearchPath(int index) const

      .. cpp:function:: int OpenColorIO::Context::getNumSearchPaths() const

      .. cpp:function:: void OpenColorIO::Context::clearSearchPaths()

      .. cpp:function:: void OpenColorIO::Context::addSearchPath(const char *path)

      **Working Directory**

      .. cpp:function:: void OpenColorIO::Context::setWorkingDir(const char *dirname)

      .. cpp:function:: const char *OpenColorIO::Context::getWorkingDir() const

      **String Vars**

      .. cpp:function:: void OpenColorIO::Context::setStringVar(const char *name, const char *value)

      .. cpp:function:: const char *OpenColorIO::Context::getStringVar(const char *name) const

      .. cpp:function:: int OpenColorIO::Context::getNumStringVars() const

      .. cpp:function:: const char *OpenColorIO::Context::getStringVarNameByIndex(int index) const

      .. cpp:function:: void OpenColorIO::Context::clearStringVars()

      .. cpp:function:: const char *OpenColorIO::Context::resolveStringVar(const char *val) const

         Do a string lookup. Do a file lookup.

         Evaluate the specified variable (as needed). Will not throw exceptions. 

      .. cpp:function:: const char *OpenColorIO::Context::resolveFileLocation(const char *filename) const

         Do a file lookup. Do a file lookup.

         Evaluate all variables (as needed). Also, walk the full search path until the file is found. If the filename cannot be found, an exception will be thrown. 

      **Environment Mode**

      .. cpp:function:: void OpenColorIO::Context::setEnvironmentMode(EnvironmentMode mode)

      .. cpp:function:: EnvironmentMode OpenColorIO::Context::getEnvironmentMode() const

      .. cpp:function:: void OpenColorIO::Context::loadEnvironment()

         Seed all string vars with the current environment. 


   .. group-tab:: Python


      **Cache**

      .. py:method:: Context.getCacheID(self: PyOpenColorIO.Context) -> str

      **Search Paths**

      .. py:method:: Context.getSearchPath(self: PyOpenColorIO.Context) -> str

      .. py:method:: Context.setSearchPath(self: PyOpenColorIO.Context, path: str) -> None

      .. py:method:: Context.getSearchPaths(self: PyOpenColorIO.Context) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Context>, 2>

      .. py:method:: Context.clearSearchPaths(self: PyOpenColorIO.Context) -> None

      .. py:method:: Context.addSearchPath(self: PyOpenColorIO.Context, path: str) -> None

      **Working Directory**

      .. py:method:: Context.getWorkingDir(self: PyOpenColorIO.Context) -> str

      .. py:method:: Context.setWorkingDir(self: PyOpenColorIO.Context, dirName: str) -> None

      **String Vars**

      .. py:method:: Context.getStringVars(self: PyOpenColorIO.Context) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Context>, 1>

      .. py:method:: Context.clearStringVars(self: PyOpenColorIO.Context) -> None

      .. py:method:: Context.resolveStringVar(self: PyOpenColorIO.Context, value: str) -> str

      .. py:method:: Context.resolveFileLocation(self: PyOpenColorIO.Context, fileName: str) -> str

      **Environment Mode**

      .. py:method:: Context.getEnvironmentMode(self: PyOpenColorIO.Context) -> PyOpenColorIO.EnvironmentMode

      .. py:method:: Context.setEnvironmentMode(self: PyOpenColorIO.Context, mode: PyOpenColorIO.EnvironmentMode) -> None

      .. py:method:: Context.loadEnvironment(self: PyOpenColorIO.Context) -> None
