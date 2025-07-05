..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _documentation-guidelines:

Documentation guidelines
========================

OpenColorIO is documented using reStructuredText, processed by
`Sphinx <http://sphinx-doc.org/>`__.

The documentation primarily lives in the ``docs/`` folder, within the
main OpenColorIO repository.

The rST source for the C++ API documentation is extracted from
comments in the public header files in ``include/``

Installation of requirements
****************************

Scripts are available, for each platform, to install the documentation 
requirements.

The ``install_docs_env.sh`` script in the ``share/ci/scripts/<Platform>`` directory
will install the Python-related requirements for building the documentation
(Sphinx, six, testresources, recommonmark, sphinx-press-theme, sphinx-tabs,
and breathe) and Doxygen.  

**Note:** If you are on Linux using yum and don't already have Doxygen installed, you will have to
uncomment the relevant line in ``share/ci/scripts/linux/dnf/install_docs_env.sh``

Use GitBash (`provided with Git for Windows <https://gitforwindows.org/>`_) to 
execute the script on Windows.

Python 3 is required to build the documentation. If you have multiple Python
installs you'll need to make sure pip and CMake find the correct version. You 
can manually inform CMake of which to use by adding this option to the below 
`cmake` command, which configures the documentation build::

    -DPython_ROOT=<Path to Python 3 root directory>

You need to make sure these Python package locations are in your ``PYTHONPATH``
environment variable prior to configuring the build. A good way to accomplish this is
with a virtual environment, which would look like::

    $ python3 -m venv venv
    $ source venv/bin/activate
    $ share/ci/scripts/<platform>/install_docs_env.sh  # macos or windows
    or
    $ share/ci/scripts/linux/<tool>install_docs_env.sh  # for linux, tool is apt or yum
    $ export PYTHONPATH=</path/to>/venv/lib/python<version>/site-packages
    
Obviously, adjust for specific paths and python versions. Also, the above assumes a ``bash``
environment - the commands may be slightly different for other shells.

Building the docs
*****************

The build is just like a :ref:`regular build from source <building-from-source>`,
but specify the ``-D OCIO_BUILD_DOCS=ON`` argument to CMake.

Then run the ``make docs`` target. The default HTML output will be
created in ``build_dir/docs/build-html/``

Note that CMake must be run before each invocation of ``make`` to copy
the edited rST files.

Initial run::

    $ mkdir build && cd build

Then after each change you wish to preview::

    $ cmake -D OCIO_BUILD_DOCS=ON ../ && make docs

Tip: 
    The ``-j`` option to ``make`` is your friend, eg, ``make -j 8`` will make things go much faster.
    The exact number will depend on the resources of your particular machine. The ``nproc`` command (linux) will help you decide.

Updating the Python docs
************************

The conf.py has a switch that detects when it is being run on RTD, and in that case 
will itself run Doxygen to generate the XML needed by breathe prior to building the docs, 
and will also facilitate a CMake configure_file-like process (via Python) to handle 
substitutions in headers and docs source files that CMake would usually handle, but can't 
in this case. One potential plus to all of this is that if someone wants to just build 
OCIO docs, they can technically do so by running sphinx-build in the docs directory, and 
nothing more. Right now that only works when the READTHEDOCS env var == True, but it could 
be easily exposed another way if needed.

These features required several custom Sphinx extensions tuned for our project which are
located under ``share/docs``.

Building the docs -- Excluding the API docs
*******************************************

If you don't need to build the API documentation, there is a quick and dirty way to 
do a docs build.  This approach does not need to compile the C++ code but is not ideal
since it modifies files in the source directory rather than the build directory::

    export READTHEDOCS=True
    cd docs  (in the source directory)
    mkdir _build
    sphinx-build -b html . _build
    <your web browser name> _build/index.html

Basics
******

* Try to keep the writing style consistent with surrounding docs.

* Fix all warnings output by the Sphinx build process. An example of
  such an warning is::

    checking consistency... [...]/build/docs/userguide/writing_configs.rst:: WARNING: document isn't included in any toctree

* Use the following hierarchy of header decorations::

      Level 1 heading
      ===============
  
      Level 2 heading
      ***************
  
      Level 3 heading
      +++++++++++++++
  
      Level 4 heading
      ---------------

* To add a new page, create a new ``.rst`` file in the appropriate
  location. In that directory's ``index.rst``, add the new file to
  the ``toctree`` directive.

  The new file should contain a top-level heading (decorated with
  `=====` underline), and an appropriate label for referencing from
  other pages. For example, a new file
  ``docs/userguide/baking_luts.rst`` might start like this::

      .. _userguide-bakingluts:

      Baking LUT's
      ============

      In order to bake a LUT, ...

Quirks
******

The vuepress theme that we've migrated to has some quirks to its design. For
example, it only allows two nested table of contents (TOC). So things have to be
organized in a slightly different way than other sphinx projects.

The root-level `toc_redirect.rst` points to where to find the different section
TOCs. The name and contents of each sections TOC is defined in that
sub-directory's `_index.rst` file.

In this TOC the `:caption:` directive determines what the name of the section
will be in the sidebar, and in the header of the website. The *H1* header
determines the name of the page in the right/left arrows navigation bar. In a
lot of cases this ends up doubling up the name on the page, but this seems
unavoidable at the present time. If additional explanatory text is put in the
`_index.rst` files then it shouldn't be as problematic.

The site will show all *H1* headers in the side panel by default, these then
expand when selected to show all *H2* headers.

Due to the limited TOC and sidebar depth, we shouldn't be afraid of looong
pages with many *H2* headings to break down the page into logical quadrants.

Emacs rST mode
**************

Emacs' includes a mode for editing rST files. It is documented on `the
docutils site
<http://docutils.sourceforge.net/docs/user/emacs.html>`__

One of the features it includes is readjusting the hierarchy of
heading decorations (the underlines for different heading levels). To
configure this to use OCIO's convention, put the following in your ``.emacs.d/init.el``:

.. code-block:: common-lisp

    (setq rst-preferred-decorations
          '((?= simple 0)
            (?* simple 0)
            (?+ simple 0)
            (?- simple 0)))
