/*
Copyright (c) 2003-2017 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _OPENCOLORIO_PS_DIALOG_H_
#define _OPENCOLORIO_PS_DIALOG_H_

#include <string>

enum DialogResult
{
    RESULT_OK,
    RESULT_CANCEL,
    RESULT_EXPORT
};

enum DialogSource
{
    SOURCE_ENVIRONMENT,
    SOURCE_STANDARD,
    SOURCE_CUSTOM
};

enum DialogAction
{
    ACTION_LUT,
    ACTION_CONVERT,
    ACTION_DISPLAY
};

enum DialogInterp
{
    INTERPO_NEAREST,
    INTERPO_LINEAR,
    INTERPO_TETRAHEDRAL,
    INTERPO_BEST
};

typedef struct DialogParams
{
    DialogSource    source;
    std::string     config; // path when source == SOURCE_CUSTOM
    DialogAction    action;
    bool            invert;
    DialogInterp    interpolation;
    std::string     inputSpace;
    std::string     outputSpace;
    std::string     device;
    std::string     transform;
} DialogParams;


// return true if user hit OK
// if user hit OK, params block will have been modified
//
// send in block of parameters
// plugHndl is bundle identifier string on Mac, hInstance on win
// mwnd is the main window, Windows only (NULL on Mac)

DialogResult OpenColorIO_PS_Dialog(DialogParams &params, const void *plugHndl, const void *mwnd);

void OpenColorIO_PS_About(const void *plugHndl, const void *mwnd);

#endif // _OPENCOLORIO_PS_DIALOG_H_
