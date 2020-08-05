#include "SysToolX.h"

void FreeMem(void *block) {
  if (block) {
    LocalFree(block);
  }
}

void *GetMem(DWORD dwSize) {
  return((void *) LocalAlloc(LPTR, dwSize));
}

TCHAR *LangLoadString(UINT sid) {
TCHAR *res;
WORD *p;
HRSRC hr;
int i;
  res = NULL;
  hr = FindResource(NULL, MAKEINTRESOURCE(sid / 16 + 1), RT_STRING);
  p = hr ? (WORD *) LockResource(LoadResource(NULL, hr)) : NULL;
  if (p) {
    for (i = 0; i < (sid & 15); i++) {
      p += 1 + *p;
    }
    res = STR_ALLOC(*p);
    if (res) {
      LoadString(NULL, sid, res, *p + 1);
    }
  }
  return(res);
}

TCHAR *GetWndText(HWND wnd) {
TCHAR *result;
int sz;
  sz = GetWindowTextLength(wnd);
  result = STR_ALLOC(sz);
  if (result) {
    GetWindowText(wnd, result, sz + 1);
    result[sz] = 0;
  }
  return(result);
}

int MsgBox(HWND wnd, TCHAR *lpText, UINT uType) {
int result;
TCHAR *s, *t;
  s = NULL;
  if (!HIWORD(lpText)) {
    s = LangLoadString(LOWORD(lpText));
  }
  t = GetWndText(wnd);
  result = MessageBox(wnd, s ? s : lpText, t, uType);
  if (t) { FreeMem(t); }
  if (s) { FreeMem(s); }
  return(result);
}

// http://blogs.msdn.com/b/oldnewthing/archive/2004/08/04/208005.aspx
void DialogEnableWindow(HWND hdlg, int idControl, BOOL bEnable) {
HWND hctl;
  hctl = GetDlgItem(hdlg, idControl);
  if ((bEnable == FALSE) && (hctl == GetFocus())) {
    SendMessage(hdlg, WM_NEXTDLGCTL, 0, FALSE);
  }
  EnableWindow(hctl, bEnable);
}

// PATCH: Windows 98 combobox fix (common controls version < 6)
// http://blogs.msdn.com/b/oldnewthing/archive/2006/03/10/548537.aspx
// ! http://vbnet.mvps.org/index.html?code/comboapi/comboheight.htm
void AdjustComboBoxHeight(HWND hWndCmbBox, DWORD MaxVisItems) {
RECT rc;
  GetWindowRect(hWndCmbBox, &rc);
  rc.right -= rc.left;
  ScreenToClient(GetParent(hWndCmbBox), (POINT *) &rc);
  rc.bottom = (MaxVisItems + 2) * SendMessage(hWndCmbBox, CB_GETITEMHEIGHT, 0, 0);
  MoveWindow(hWndCmbBox, rc.left, rc.top, rc.right, rc.bottom, TRUE);
  // PATCH: enable integral height, ComboBox must be created with CBS_NOINTEGRALHEIGHT
  SetWindowLong(hWndCmbBox, GWL_STYLE, (GetWindowLong(hWndCmbBox, GWL_STYLE) | CBS_NOINTEGRALHEIGHT) ^ CBS_NOINTEGRALHEIGHT);
}
