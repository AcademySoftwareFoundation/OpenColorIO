
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//

#ifndef _OPENCOLORIC_AE_DIALOG_H_
#define _OPENCOLORIC_AE_DIALOG_H_

#include <string>
#include <map>
#include <vector>

typedef std::map<std::string, std::string> ExtensionMap; // map[ ext ] = format

bool OpenFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd);

bool SaveFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd);

bool GetMonitorProfile(char *path, int buf_len, const void *hwnd);


typedef std::vector<std::string> MenuVec;

int PopUpMenu(const MenuVec &menu_items, int selected_index, const void *hwnd);


void ErrorMessage(const char *message);

#ifdef __APPLE__
void SetMickeyCursor();
#endif


#endif // _OPENCOLORIC_AE_DIALOG_H_