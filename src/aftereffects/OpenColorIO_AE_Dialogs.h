

#ifndef _OPENCOLORIC_AE_DIALOG_H_
#define _OPENCOLORIC_AE_DIALOG_H_

#include <string>
#include <map>
#include <vector>

typedef std::map<std::string, std::string> ExtensionMap; // map[ ext ] = format

bool OpenFile(char *path, int buf_len, ExtensionMap &extensions, void *hwnd);


typedef std::vector<std::string> MenuVec;

int PopUpMenu(MenuVec &menu_items, int selected);


#ifdef __APPLE__
void SetMickeyCursor();
#endif


#endif // _OPENCOLORIC_AE_DIALOG_H_