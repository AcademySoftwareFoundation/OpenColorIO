/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
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

#ifndef _OPENCOLORIC_AE_DIALOG_H_
#define _OPENCOLORIC_AE_DIALOG_H_

#include <string>
#include <map>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


typedef std::map<std::string, std::string> ExtensionMap; // map[ ext ] = format

bool OpenFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd);

bool SaveFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd);

bool GetMonitorProfile(char *path, int buf_len, const void *hwnd);


typedef std::vector<std::string> ConfigVec;

void GetStdConfigs(ConfigVec &configs);

std::string GetStdConfigPath(const std::string &name);


typedef std::vector<std::string> MenuVec;

int PopUpMenu(const MenuVec &menu_items, int selected_index, const void *hwnd);


bool ColorSpacePopUpMenu(OCIO::ConstConfigRcPtr config, std::string &colorSpace, bool selectRoles, const void *hwnd);


void ErrorMessage(const char *message, const void *hwnd);

#ifdef SUPPLY_HINSTANCE
void SetHInstance(void *hInstance);
#endif

#endif // _OPENCOLORIC_AE_DIALOG_H_