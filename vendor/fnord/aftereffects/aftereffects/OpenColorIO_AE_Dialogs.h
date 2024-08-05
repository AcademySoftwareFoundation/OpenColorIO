// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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