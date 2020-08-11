Documentation guidelines
========================

OpenColorIO is documentated using reStructuredText, processed by
`Sphinx <http://sphinx-doc.org/>`__.

The documentation primarily lives in the ``docs/`` folder, within the
main OpenColorIO repoistory.

The rST source for the C++ API documentation is extracted from
comments in the public header files in ``export/``

The Python API documentation is extracted from dummy .py files within
the ``src/pyglue/DocStrings/`` folder


Building the docs
*****************

Just like a :ref:`regular build from source <building-from-source>`,
but specify the ``-D OCIO_BUILD_DOCS=yes`` argument to CMake.

Then run the ``make doc`` target. The default HTML output will be
created in ``build_dir/docs/build-html/``

Note that CMake must be run before each invokation of ``make`` to copy
the edited rST files.

Initial run:

    $ mkdir build && cd build

Then after each change you wish to preview:

    $ cmake -D OCIO_BUILD_DOCS=yes .. && make doc


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
  `=====` underline), and an approriate label for referencing from
  other pages. For example, a new file
  ``docs/userguide/baking_luts.rst`` might start like this::

      .. _userguide-bakingluts:

      Baking LUT's
      ============

      In order to bake a LUT, ...

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
