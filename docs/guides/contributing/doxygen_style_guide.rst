..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _doxygen-style-guide:

OpenColorIO Doxygen Style Guide
===============================

This guide introduces a consistent style for documenting OCIO public C++ headers
with Doxygen. New code should be documented with the following conventions.

Source Code Documentation Syntax
********************************

Doxygen documentation needs only to be applied to the public interface for which
we want to generate OCIO API documentation files. Implementation code that does
not participate in this should still be enhanced by source code comments as
appropriate, but these comments are not required to follow the style outlined
here.

Doxygen Tags
++++++++++++

The following tags are generally supported:

* ``brief``: Describes function or class succinctly, ideally in one sentence.
* ``param``: Describes function parameters.
* ``tparam``: Describes a template parameter.
* ``return``: Describes return values.
* ``ref``: A reference to another method/class.
* ``note``: Describes a note you want a user to be attentive of.
* ``warning``: Describes a warning to bring a users attention to.
* ``code{<extension>}``: The start of a code block. The extension denotes the language e.g. (".py", ".cpp")
* ``endcode``: The end of a code block.

Additional Doxygen tags may be supported, but will need to be tested by building
the Sphinx docs. Not all tags need to be used in every docstring.

Block Comments
++++++++++++++

Where possible please try to split the tag and names from the descriptive text.

.. code-block:: cpp

   /**
    * \brief A brief description.
    *
    * Detailed description. More detail.
    * \see Some reference
    *
    * \param <name>
    *      Parameter description.
    * \tparam <name>
    *      Template parameter description.
    * \return
    *      Return value description.
    */

**Example**

.. code-block:: cpp

    /**
     * \brief Returns a compressed version of a string.
     *
     * Compresses an input string using the foobar algorithm.
     *
     * \param
     *      Uncompressed The input string.
     * \return
     *      A compressed version of the input string.
     */
    std::string compress(const std::string& uncompressed);

Single Line Comments
++++++++++++++++++++

Single line comments should be equivalent in scope to the `\brief` in the
block comments above. It should be reserved for functions that don't have
parameters, or members that don't require much description.

.. code-block:: cpp

    /// A brief description.

**Example**

.. code-block:: cpp

    /// A default constructor
    MyClass();

In-Line Comments
++++++++++++++++

In-line comments serve the same purpose as the single line comments, but allows
for greater structural flexibility.

.. code-block:: cpp

    ///< A brief description.

**Example**

.. code-block:: cpp

    enum EnumType
    {
      int EVal1,     ///< enum description 1.
      int EVal2      ///< enum description 2.
    };

Documentation Language
**********************

Remember that the docstrings serve both the C++ and Python documentation. Try to
use generic language where possible, without referring to C++ or Python
constructs exclusively. If you do need to describe additional details for a
specific implementation, please refer to which language you are speaking about.
