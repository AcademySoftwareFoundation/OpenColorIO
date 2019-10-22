// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    INTERPO_CUBIC,
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
