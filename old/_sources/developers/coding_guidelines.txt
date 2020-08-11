Coding guidelines
=================

There are only two important rules:

1. When making changes, conform to the style and conventions of the surrounding code.

2. Strive for clarity, even if that means occasionally breaking the guidelines. Use your head and ask for advice if your common sense seems to disagree with the conventions.

File Conventions
****************

C++ implementation should be named ``*.cpp``. Headers should be named ``*.h``.
All source files should begin with the copyright and license, which can be cut and pasted from other source files). For NEW source files, please do change the copyright year to the present. However DO NOT edit existing files just to update the copyright year, it just creates pointless deltas and offers no increased protection

Line Length
***********

Each line of text in your code should be at most 80 characters long.

Generally the only exceptions are for comments with example commands or URLs - to make cut and paste easier.

The other exception is for those rare cases where letting a line be longer (and wrapping on an 80-character window) is actually a better and clearer alternative than trying to split it into two lines. Sometimes this happens, but it's rare.

DO NOT alter somebody else's code to re-wrap lines (or change whitespace) just because you found something that violates the rules. Let the group/author/leader know, and resist the temptation to change it yourself.

Formatting
**********

* Indent 4 spaces at a time, and use actual spaces, not tabs.  This is particularly critical on python code.  The only exception currently allowed in within Makefiles, where tab characters are sometimes required.

* Opening brace on the line following the condition or loop.

* The contents of namespaces are not indented.

* Function names should be on the same line as their return values.

* Function calls should NOT have a space between the function name and the opening parenthesis. A single space should be added after each required comma.

Here is a short code fragement that shows these concepts in action:

.. code-block:: c++

    namespace
    {


    int MungeMyNumbers(int a, int b)
    {
        int x = a + b;
    
        if (a == 0 || b==0)
        {
            x += 1;
            x += 2;
        }
        else
        {
            for (int i=0; i<16; ++i)
            {
                x += a * i;
            }
        }
    
        return x;
    }


    } // namespace

Misc. Rules
***********

* Avoid macros when possible.

* Anonymous namespaces are preferred when sensible.

* Variable names should be camelCase, as well as longAndExplicit.

* Avoid long function implementations. Break up work into small, manageable chunks.

* Use "TODO:" comments for code that is temporary, a short-term solution, or good-enough but not perfect. This is vastly preferred to leaving subtle corner cases undocumented.

* Always initialize variables on construction.

* Do not leave dead code paths around. (This is what revision history is for)

* Includes should always be ordered as follows: C library, C++ library, other libraries' .h, OCIO public headers, OCIO private headers. Includes within a category should be alphabetized.

* The C++ "using" directive is not allowed.

* Static / global variables should be avoided at all costs.

* Use const whenever it makes sense to do so.

* The use of Boost is not allowed, except for unit_test_framework and shared_ptr.

* Default function arguments are not allowed.

Bottom Line
***********

When in doubt, look elsewhere in the code base for examples of similar structures and try to format your code in the same manner.


Portions of this document have been blatantly lifted from `OpenImageIO
<http://openimageio.org/wiki/index.php?title=Coding_Style_Guide>`__,
and `Google
<http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml>`__.
