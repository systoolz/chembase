#include "Settings.h"

void LoadSettings(int *cfg, int *def, DWORD len, TCHAR *str) {
TCHAR *s;
DWORD i;
  if (cfg && def && str && len) {
    s = str;
    s += lstrlen(s) + 1;
    for (i = 0; i < len; i++) {
      cfg[i] = GetPrivateProfileInt(s, (TCHAR *) &def[i * 2], def[(i * 2) + 1], str);
    }
  }
}

void SaveSettings(int *cfg, int *def, DWORD len, TCHAR *str) {
TCHAR buf[1025], fmt[3], *s;
DWORD i;
  if (cfg && def && str && len) {
    fmt[0] = TEXT('%');
    fmt[1] = TEXT('d');
    fmt[2] = 0;
    s = str;
    s += lstrlen(s) + 1;
    for (i = 0; i < len; i++) {
      wsprintf(buf, fmt, cfg[i]);
      WritePrivateProfileString(s, (TCHAR *) &def[i * 2], buf, str);
    }
  }
}
