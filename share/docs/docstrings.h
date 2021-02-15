// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

/*
  This file's format is adaped from pybind11_mkdoc by Wenzel Jakob, released 
  under The MIT License. 

  See: 
  https://github.com/pybind/pybind11_mkdoc/blob/master/pybind11_mkdoc/mkdoc_lib.py

  This header is used as a stand-in for a generated docstrings.h when 
  'OCIO_BUILD_PYTHON=ON' and 'OCIO_BUILD_DOCS=OFF'. All 'DOC()' calls in the 
  Python bindings will return an empty string.

  -----------------------------------------------------------------------------

  The MIT License (MIT)

  Copyright (c) 2020 Wenzel Jakob

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  -----------------------------------------------------------------------------
 */

#define DOC(...) __doc_none

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static const char *__doc_none = R"doc()doc";

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
