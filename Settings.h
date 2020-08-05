#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <windows.h>

void LoadSettings(int *cfg, int *def, DWORD len, TCHAR *str);
void SaveSettings(int *cfg, int *def, DWORD len, TCHAR *str);

#endif
