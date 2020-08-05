#ifndef __SYSTOOLX_H
#define __SYSTOOLX_H

#include <windows.h>

#define PCSD __attribute__ ((aligned(1))) const static
#define LIST_LEN(x) (sizeof(x) / sizeof((x)[0]))
#define STR_ALLOC(x) ((TCHAR *) GetMem((x + 1) * sizeof(TCHAR)))

void FreeMem(void *block);
void *GetMem(DWORD dwSize);

TCHAR *LangLoadString(UINT sid);
TCHAR *GetWndText(HWND wnd);
int MsgBox(HWND wnd, TCHAR *lpText, UINT uType);
void DialogEnableWindow(HWND hdlg, int idControl, BOOL bEnable);
void AdjustComboBoxHeight(HWND hWndCmbBox, DWORD MaxVisItems);

#endif
