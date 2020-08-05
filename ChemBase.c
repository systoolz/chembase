#include <windows.h>

// new messages
#ifndef _WIN32_IE
#define _WIN32_IE 0x0300
#endif

#include <commctrl.h>

// old GCC headers
#if (_WIN32_IE >= 0x0300)

#ifndef ListView_SetCheckState
#define ListView_SetCheckState(hwndLV, i, fCheck) \
  ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#endif

#ifndef ListView_GetCheckState
#define ListView_GetCheckState(hwndLV, i) \
  ((((UINT)(SNDMSG((hwndLV), LVM_GETITEMSTATE, (WPARAM)(i), LVIS_STATEIMAGEMASK))) >> 12) -1)
#endif

#endif

// CoInitialize() / CoUninitialize()
#include <shlwapi.h>
// ShellExecute()
#include <shellapi.h>

#include "resource/ChemBase.h"
#include "SysToolX.h"
#include "BaseUnit.h"
#include "WAnchors.h"
// v1.06
#include "Settings.h"

PCSD TCHAR dlgText[] = TEXT("Element\0Spectral Line\0Formula\0Energy (eV)\0Details");
PCSD TCHAR BaseFile[] = TEXT("ChemBase.bin");
PCSD TCHAR fmtStr[] = TEXT("%u.%02u");
PCSD TCHAR fmtLst[] = TEXT("%hs|%hs|%hs|%u.%02u|%hs%u|");
PCSD TCHAR fmtA2U[] = TEXT("%hs");
// v1.06
#define MAX_CFG 14
#define CFG_X 0
#define CFG_Y 1
#define CFG_W 2
#define CFG_H 3
#define CFG_N 4
#define CFG_D 5
#define CFG_E 6
#define CFG_L 7
#define CFG_S 8
#define CFG_C 9
PCSD TCHAR iniStr[] = TEXT(".\\ChemBase.ini\0ChemBase");
// default settings values
PCSD int defcfg[MAX_CFG][2] = {
  // window x and y position
  {TEXT('x'),   CW_USEDEFAULT},
  {TEXT('y'),   CW_USEDEFAULT},
  // window width and height
  {TEXT('w'),   0},
  {TEXT('h'),   0},
  // energy and dispersion
  {TEXT('n'),   0},
  {TEXT('d'),   0},
  // selected element id
  {TEXT('e'),  -1},
  // spectral lines bit flag mask
  {TEXT('l'),  -1},
  // column sort order (negative for descending)
  {TEXT('s'),   4},
  // columns sizes
  {TEXT('1'),  50},
  {TEXT('2'),  80},
  {TEXT('3'), 199},
  {TEXT('4'),  70},
  {TEXT('5'),  60}
};
// v1.06 database tables
#define BASE_ALLDATA 0
#define BASE_ELEMENT 1
#define BASE_SPCLINE 2
#define BASE_FORMULA 3
// v1.06 listview columns
#define COLS_ELEMENT 0
#define COLS_SPCLINE 1
#define COLS_FORMULA 2
#define COLS_ENERGYL 3
#define COLS_DETAILS 4
#define COLS_INTOTAL 5

// anchors for controls
PCSD WORD dlgAnchors[] = {
  MAKEWORD(IDC_LIST, WACS_LEFT  | WACS_RIGHT | WACS_TOP | WACS_BOTTOM),
  MAKEWORD(IDC_INFO, WACS_LEFT  | WACS_RIGHT | WACS_BOTTOM),
  MAKEWORD(IDC_COPY, WACS_RIGHT | WACS_BOTTOM),
  MAKEWORD(IDC_SITE, WACS_RIGHT | WACS_BOTTOM),
  MAKEWORD(IDC_XCHG, WACS_RIGHT | WACS_TOP),
  MAKEWORD(IDC_SHOW, WACS_RIGHT | WACS_TOP),
  MAKEWORD(IDC_LINE, WACS_RIGHT | WACS_TOP   | WACS_BOTTOM),
  MAKEWORD(IDC_SPEC, WACS_RIGHT | WACS_TOP)
};

static bin_file bf;

void ANSI2Unicode(TCHAR *su, CCHAR *sa) {
  wsprintf(su, fmtA2U, sa);
}

int GetElementId(HWND wnd) {
int i;
  i = -1;
  if (wnd) {
    i = SendMessage(wnd, CB_GETCURSEL, 0, 0);
    if (i >= 0) {
      i = SendMessage(wnd, CB_GETITEMDATA, i, 0);
    }
  }
  return(i);
}

void AddElementSD(HWND wnd, TCHAR *str, int dat) {
int i;
  if (wnd && str) {
    i = SendMessage(wnd, CB_ADDSTRING, 0, (LPARAM) str);
    if (i >= 0) {
      SendMessage(wnd, CB_SETITEMDATA, i, (LPARAM) dat);
    }
  }
}

void BuildListRow(TCHAR *s, bin_file *bfx, bin_item *bi) {
CCHAR d[3];
BYTE b;
  // sanity check
  if (s && bfx && bi) {
    b = bi->detail >> 24;
    d[0] = b & 0x7F;
    d[1] = '-' * (b >> 7);
    d[2] = 0;
    wsprintf(s, fmtLst,
      BaseItem(bfx, BASE_ELEMENT, bi->element),
      BaseItem(bfx, BASE_SPCLINE, bi->spcline),
      BaseItem(bfx, BASE_FORMULA, bi->formula),
      bi->energy / 100, bi->energy % 100,
      d, bi->detail & 0x00FFFFFF
    );
    while (*s) {
      if (*s == TEXT('|')) { *s = 0; }
      s++;
    }
  }
}

int AddListViewItem(HWND wnd, TCHAR *s, LPARAM lparm) {
LV_ITEM li;
  li.iItem = -1;
  // sanity check
  if (wnd && s) {
    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_TEXT | LVIF_PARAM;
    li.pszText = s;
    li.lParam = lparm;
    li.iItem = ListView_GetItemCount(wnd);
    li.iItem = ListView_InsertItem(wnd, &li);
    if (li.iItem != -1) {
      li.mask ^= LVIF_PARAM;
      // subitems
      for (li.pszText += (lstrlen(li.pszText) + 1); li.pszText[0]; li.pszText += (lstrlen(li.pszText) + 1)) {
        li.iSubItem++;
        ListView_SetItem(wnd, &li);
      }
    }
  }
  return(li.iItem);
}

DWORD GetAndFixEditFloat(HWND wnd) {
DWORD result, i, j, d;
TCHAR s[16];
  GetWindowText(wnd, s, 11);
  j = 0;
  d = 0;
  for (i = 0; s[i]; i++) {
    // only digits and dot
    if (((s[i] >= TEXT('0')) && (s[i] <= TEXT('9'))) || (s[i] == TEXT('.') && (!d))) {
      // only one dot allowed
      if (s[i] == TEXT('.')) {
        d = j + 1;
      } else {
        // dot ignored
        s[j] = s[i];
        j++;
      }
    }
  }
  // dot
  d = d ? (d - 1) : j;
  // only 2 digits after dot
  for (i = 0; i < 2; i++) {
    if (d >= j) {
      s[d] = TEXT('0');
      j++;
    }
    d++;
  }
  s[d] = 0;
  // convert to number
  result = 0;
  for (i = 0; s[i]; i++) {
    result *= 10;
    result += (s[i] - TEXT('0'));
  }
  // fix edit text
  wsprintf(s, fmtStr, result / 100, result % 100);
  SetWindowText(wnd, s);
  return(result);
}

#define CID_DONE 0
#define CID_INIT 2
#define CID_LIST 0
#define CID_CBOX 1
void CtrlInitDone(HWND wnd, DWORD flag) {
int i;
  // flag: init
  if (flag & CID_INIT) {
    // prevent update
    SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
    if (flag & CID_CBOX) {
      // combobox
      SendMessage(wnd, CB_RESETCONTENT, 0, 0);
    } else {
      // v1.06 - remove focus to speedup a bit deletion process
      i = ListView_GetNextItem(wnd, -1, LVNI_FOCUSED);
      if (i >= 0) {
        ListView_SetItemState(wnd, i, 0, LVIS_FOCUSED | LVIS_SELECTED);
      }
      // listview
      // PATCH: Windows 98 can add 27k+ items just fine (around 10 seconds)
      // but deleting with ListView_DeleteAllItems() can take up to whole 10 minutes!
      // items MUST be deleted from last to first (around 15 seconds) and not vice versa
      // or it takes the same 10 minutes (the Microsoft way)
      // this bug was fixed at least in Windows XP and newer
      for (i = ListView_GetItemCount(wnd); i > 0; i--) {
        ListView_DeleteItem(wnd, i - 1);
      }
      // just in case
      ListView_DeleteAllItems(wnd);
    }
  // flag: done
  } else {
    // restore update
    SendMessage(wnd, WM_SETREDRAW, TRUE, 0);
    UpdateWindow(wnd);
  }
}

// v1.06 reworked
void AddElements(HWND wnd, bin_file *bfx) {
TCHAR buf[1025];
CCHAR *data;
WORD *offs;
DWORD i;
  wnd = GetDlgItem(wnd, IDC_ELEM);
  // init
  CtrlInitDone(wnd, CID_CBOX | CID_INIT);
  // first item must be empty
  *buf = 0;
  AddElementSD(wnd, buf, -1);
  offs = (WORD *) BaseData(bfx, BASE_ELEMENT);
  if (offs) {
    data = (CCHAR *) offs;
    for (i = 0; i < *offs; i++) {
      ANSI2Unicode(buf, &data[offs[i] * 2]);
      AddElementSD(wnd, buf, i);
    }
  }
  // done
  CtrlInitDone(wnd, CID_CBOX | CID_DONE);
  // fix combobox height
  AdjustComboBoxHeight(wnd, 8);
  // v1.06 select first item - removes need for double down cursor selection
  SendMessage(wnd, CB_SETCURSEL, 0, 0);
}

// v1.06 reworked
void AddSpectralLines(HWND wnd, bin_file *bfx) {
TCHAR buf[1025];
LV_FINDINFO lf;
bin_item *bi;
LV_ITEM li;
WORD *offs;
CCHAR *s;
HWND wh;
DWORD i;
int j;
  // got something to find?
  j = GetElementId(GetDlgItem(wnd, IDC_ELEM));
  wh = wnd;
  wnd = GetDlgItem(wnd, IDC_LINE);
  // init
  CtrlInitDone(wnd, CID_LIST | CID_INIT);
  // something selected
  if (j >= 0) {
    offs = (WORD *) BaseData(bfx, BASE_ALLDATA);
    i = *offs;
    bi = (bin_item *) &offs[1];
    offs = (WORD *) BaseData(bfx, BASE_SPCLINE);
    s = (CCHAR *) offs;
    ZeroMemory(&lf, sizeof(lf));
    lf.flags = LVFI_PARAM;
    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_PARAM | LVIF_TEXT;
    li.pszText = buf;
    while (i--) {
      if (bi->element == j) {
        // not in listbox already
        lf.lParam = bi->spcline;
        if (ListView_FindItem(wnd, -1, &lf) == -1) {
          // add and set check mark by default
          ANSI2Unicode(buf, &s[offs[bi->spcline] * 2]);
          li.lParam = bi->spcline;
          ListView_SetCheckState(wnd, ListView_InsertItem(wnd, &li), TRUE);
        }
      }
      // next row
      bi++;
    }
  }
  // at least one item
  i = (ListView_GetItemCount(wnd) > 0) ? TRUE : FALSE;
  DialogEnableWindow(wh, IDC_XCHG, i);
  DialogEnableWindow(wh, IDC_SHOW, i);
  // done
  CtrlInitDone(wnd, CID_LIST | CID_DONE);
  // v1.06 - select first item if any
  if (i) {
    ListView_SetItemState(wnd, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
  }
}

// v1.06 reworked
int CALLBACK ItemSortFunc(LPARAM lparm1, LPARAM lparm2, LPARAM lparmsort) {
bin_item *bi1, *bi2;
int result;
  bi1 = (bin_item *) lparm1;
  bi2 = (bin_item *) lparm2;
  // v1.06 note: all items id in database sorted in ascending order now
  switch (LOBYTE(lparmsort)) {
    case COLS_ELEMENT:
      result = bi1->element - bi2->element;
      break;
    case COLS_SPCLINE:
      result = bi1->spcline - bi2->spcline;
      break;
    case COLS_FORMULA:
      result = bi1->formula - bi2->formula;
      break;
    case COLS_DETAILS:
      result = (bi1->detail & 0x7FFFFFFF) - (bi2->detail & 0x7FFFFFFF);
      break;
    case COLS_ENERGYL:
    default:
      result = bi1->energy - bi2->energy;
      break;
  }
  // v1.06 equal values - sort by records order
  // this will prevent situation when switching
  // between ascending and descending on the same column
  // shifts selected item from first on ascending to
  // second to last on descending and vice versa
  // this may happen if few first elements have equal
  // values in the sorting column
  if (!result) {
    result = ((LPARAM) bi1) - ((LPARAM) bi2);
  }
  // ascending or descending sort order
  result *= 1 - (HIBYTE(lparmsort) * 2);
  return(result);
}

void EnergyFind(HWND wnd, bin_file *bfx, DWORD vitem, DWORD vdiff) {
TCHAR s[1025];
LV_FINDINFO lf;
bin_item *bi;
WORD *offs;
HWND wh;
DWORD i;
int j;
  // energy vmin / vmax (vitem / vdiff)
  i = (vitem > vdiff) ? (vitem - vdiff) : 0;
  vdiff += vitem;
  vitem = i;
  // additional filter by selected element
  j = GetElementId(GetDlgItem(wnd, IDC_ELEM));
  // window handles
  wh = GetDlgItem(wnd, IDC_LINE);
  wnd = GetDlgItem(wnd, IDC_LIST);
  // init
  CtrlInitDone(wnd, CID_LIST | CID_INIT);
  offs = (WORD *) BaseData(bfx, BASE_ALLDATA);
  i = *offs;
  bi = (bin_item *) &offs[1];
  ZeroMemory(&lf, sizeof(lf));
  lf.flags = LVFI_PARAM;
  while (i--) {
    // energy
    if ((bi->energy >= vitem) && (bi->energy <= vdiff)) {
      lf.lParam = bi->spcline;
      // element any or matched with selected spectral lines
      if ((j == -1) || (
        (j == bi->element) && (ListView_GetCheckState(wh, ListView_FindItem(wh, -1, &lf)))
      )) {
        BuildListRow(s, bfx, bi);
        AddListViewItem(wnd, s, (LPARAM) bi);
      }
    }
    // next row
    bi++;
  }
  // sort by last sort order
  ListView_SortItems(wnd, ItemSortFunc, GetWindowLong(wnd, GWL_USERDATA));
  // done
  CtrlInitDone(wnd, CID_LIST | CID_DONE);
  // v1.06 - select first item if any
  if (ListView_GetItemCount(wnd) > 0) {
    ListView_SetItemState(wnd, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
  }
}

void ShowInfo(HWND wnd, bin_file *bfx) {
TCHAR s[1025], z[1025], *fmt, *x, *p[COLS_INTOTAL];
LV_ITEM li;
DWORD i;
HWND wh;
  wh = GetDlgItem(wnd, IDC_LIST);
  ZeroMemory(&li, sizeof(li));
  // find selected item
  li.iItem = ListView_GetNextItem(wh, -1, LVNI_SELECTED);
  if (li.iItem != -1) {
    // get selected item
    li.mask = LVIF_PARAM;
    ListView_GetItem(wh, &li);
    BuildListRow(s, bfx, (bin_item *) li.lParam);
    fmt = LangLoadString(IDS_INFO);
    if (fmt) {
      x = s;
      for (i = 0; i < COLS_INTOTAL; i++) {
        p[i] = x;
        x += *x ? (lstrlen(x) + 1) : 0;
      }
      wsprintf(z, fmt, p[COLS_ELEMENT], p[COLS_SPCLINE], p[COLS_FORMULA], p[COLS_ENERGYL], p[COLS_DETAILS]);
      lstrcpy(s, z);
      FreeMem(fmt);
    } else {
      *s = 0;
    }
    // set text
    SetDlgItemText(wnd, IDC_INFO, s);
    // enable buttons
    DialogEnableWindow(wnd, IDC_COPY, TRUE);
    DialogEnableWindow(wnd, IDC_SITE, TRUE);
  }
}

void InvertLines(HWND wnd) {
int i;
  for (i = 0; i < ListView_GetItemCount(wnd); i++) {
    ListView_SetCheckState(wnd, i, ListView_GetCheckState(wnd, i) ? 0 : 1);
  }
}

// v1.06
void SetDlgValue(HWND wnd, int dlg, int value) {
TCHAR buf[1025];
  value = (value < 0) ? (-value) : value;
  wsprintf(buf, fmtStr, value / 100, value % 100);
  SetDlgItemText(wnd, dlg, buf);
}

// v1.06
void GetWndDefRect(HWND wnd, RECT *rc) {
  if (wnd && rc) {
    if (IsIconic(wnd) || IsZoomed(wnd)) {
      SendMessage(wnd, WM_SYSCOMMAND, (WPARAM) SC_RESTORE, 0);
    }
    GetWindowRect(wnd, rc);
  }
}

// v1.06
void EnsureWndOnScreen(RECT *rc) {
RECT wr;
  if (rc) {
    ZeroMemory(&wr, sizeof(wr));
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &wr, 0)) {
      if ((rc->left + 32) >= wr.right) {
        rc->left = wr.right - rc->right;
      }
      if (rc->left < wr.left) {
        rc->left = wr.left;
      }
      if ((rc->top + 32) >= wr.bottom) {
        rc->top = wr.bottom - rc->bottom;
      }
      if (rc->top < wr.top) {
        rc->top = wr.top;
      }
    }
  }
}

// v1.06
void InitSettings(HWND wnd) {
int cfg[MAX_CFG], l;
RECT rc;
HWND wh;
  ZeroMemory(cfg, sizeof(cfg));
  LoadSettings(cfg, (int *) defcfg, MAX_CFG, (TCHAR *) iniStr);
  // window
  GetWndDefRect(wnd, &rc);
  if ((cfg[CFG_X] != CW_USEDEFAULT) && (cfg[CFG_Y] != CW_USEDEFAULT)) {
    rc.left = cfg[CFG_X];
    rc.top = cfg[CFG_Y];
  }
  if ((cfg[CFG_W] != 0) && (cfg[CFG_H] != 0)) {
    rc.right = cfg[CFG_W];
    rc.bottom = cfg[CFG_H];
  } else {
    rc.right -= rc.left;
    rc.bottom -= rc.top;
  }
  EnsureWndOnScreen(&rc);
  MoveWindow(wnd, rc.left, rc.top, rc.right, rc.bottom, TRUE);
  SetDlgValue(wnd, IDC_ITEM, cfg[CFG_N]);
  SetDlgValue(wnd, IDC_DIFF, cfg[CFG_D]);
  // find and select element
  wh = GetDlgItem(wnd, IDC_ELEM);
  for (l = SendMessage(wh, CB_GETCOUNT, 0, 0); l > 0; l--) {
    if (SendMessage(wh, CB_GETITEMDATA, l - 1, 0) == cfg[CFG_E]) {
      // select element
      SendMessage(wh, CB_SETCURSEL, l - 1, 0);
      // update lines list
      SendMessage(wnd, WM_COMMAND, MAKELONG(IDC_ELEM, CBN_SELCHANGE), (LPARAM) wh);
      break;
    }
  }
  // check spectral lines
  wh = GetDlgItem(wnd, IDC_LINE);
  for (l = ListView_GetItemCount(wh); l > 0; l--) {
    ListView_SetCheckState(wh, l - 1, (cfg[CFG_L] & 1) ? TRUE : FALSE);
    cfg[CFG_L] >>= 1;
  }
  // sort order
  if (cfg[CFG_S] < 0) {
    // descending
    cfg[CFG_S] = -cfg[CFG_S];
    l = 1;
  } else {
    // ascending
    l = 0;
  }
  wh = GetDlgItem(wnd, IDC_LIST);
  if ((cfg[CFG_S] > 0) && (cfg[CFG_S] <= COLS_INTOTAL)) {
    cfg[CFG_S]--;
    SetWindowLong(wh, GWL_USERDATA, MAKEWORD(LOBYTE(cfg[CFG_S]), l));
  }
  // column sizes
  for (l = 0; l < COLS_INTOTAL; l++) {
    ListView_SetColumnWidth(wh, l, (cfg[CFG_C + l] < 5) ? defcfg[CFG_C + l][1] : cfg[CFG_C + l]);
  }
}

// v1.06
void KeepSettings(HWND wnd) {
int cfg[MAX_CFG];
RECT rc;
HWND wh;
int l;
  ZeroMemory(cfg, sizeof(cfg));
  // window
  GetWndDefRect(wnd, &rc);
  cfg[CFG_X] = rc.left;
  cfg[CFG_Y] = rc.top;
  cfg[CFG_W] = rc.right - rc.left;
  cfg[CFG_H] = rc.bottom - rc.top;
  cfg[CFG_N] = GetAndFixEditFloat(GetDlgItem(wnd, IDC_ITEM));
  cfg[CFG_D] = GetAndFixEditFloat(GetDlgItem(wnd, IDC_DIFF));
  cfg[CFG_E] = GetElementId(GetDlgItem(wnd, IDC_ELEM));
  wh = GetDlgItem(wnd, IDC_LINE);
  for (l = 0; l < ListView_GetItemCount(wh); l++) {
    cfg[CFG_L] <<= 1;
    cfg[CFG_L] |= ListView_GetCheckState(wh, l) ? 1 : 0;
  }
  wh = GetDlgItem(wnd, IDC_LIST);
  l = GetWindowLong(wh, GWL_USERDATA);
  if (HIBYTE(l)) {
    // descending
    l = LOBYTE(l) + 1;
    l = -l;
  } else {
    // ascending
    l = LOBYTE(l) + 1;
  }
  cfg[CFG_S] = l;
  // columns
  for (l = 0; l < 5; l++) {
    cfg[CFG_C + l] = ListView_GetColumnWidth(wh, l);
  }
  SaveSettings(cfg, (int *) defcfg, MAX_CFG, (TCHAR *) iniStr);
}

BOOL CALLBACK DlgPrc(HWND wnd, UINT umsg, WPARAM wparm, LPARAM lparm) {
TCHAR *s, z[100];
NM_LISTVIEW *nmlv;
LV_COLUMN lc;
BOOL result;
RECT rc;
DWORD i;
HWND wh;
  result = FALSE;
  switch (umsg) {
    case WM_INITDIALOG:
      // add icons
      lparm = (LPARAM) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
      SendMessage(wnd, WM_SETICON, ICON_BIG  , lparm);
      SendMessage(wnd, WM_SETICON, ICON_SMALL, lparm);
      // copyrights, etc.
      s = LangLoadString(IDS_HELP);
      if (s) {
        SetDlgItemText(wnd, IDC_INFO, s);
        FreeMem(s);
      }
      // limit text length
      SendDlgItemMessage(wnd, IDC_ITEM, EM_LIMITTEXT, 10, 0);
      SendDlgItemMessage(wnd, IDC_DIFF, EM_LIMITTEXT, 10, 0);
      // add columns
      ZeroMemory(&lc, sizeof(lc));
      lc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
      lc.fmt = LVCFMT_LEFT;
      lc.pszText = (TCHAR *) dlgText;
      wh = GetDlgItem(wnd, IDC_LIST);
      for (i = 0; i < COLS_INTOTAL; i++) {
        lc.cx = defcfg[CFG_C + i][1];
        ListView_InsertColumn(wh, i, &lc);
        lc.pszText += lstrlen(lc.pszText) + 1;
      }
      // fix list report styles
      SendMessage(wh, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
      // default sort order by energy ascending
      SetWindowLong(wh, GWL_USERDATA, MAKEWORD(COLS_ENERGYL, 0));
      // fix spectral line listbox
      wh = GetDlgItem(wnd, IDC_LINE);
      // to vertical, not horizontal scrolls - must be at least one column added
      ZeroMemory(&lc, sizeof(lc));
      lc.mask = LVCF_FMT | LVCF_WIDTH;
      lc.fmt = LVCFMT_LEFT;
      // max available width without scrollbar
      GetClientRect(wh, &rc);
      lc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
      ListView_InsertColumn(wh, 0, &lc);
      // add checkboxes
      SendMessage(wh, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
      // v1.06 - moved initialization here to prevent KeepSettings() from saving uninitialized values
      BaseLoad((TCHAR *) BaseFile, &bf);
      // failed?
      if (!bf.data) {
        // report error
        MsgBox(wnd, MAKEINTRESOURCE(IDS_ERR_BASE), MB_ICONERROR | MB_OK);
        // close window
        PostMessage(wnd, WM_COMMAND, MAKELONG(IDCANCEL, BN_CLICKED), 0);
        break;
      }
      // fill in combobox
      AddElements(wnd, &bf);
      // anchors for resize
      for (i = 0; i < LIST_LEN(dlgAnchors); i++) {
        WndAnchorsFlag(GetDlgItem(wnd, LOBYTE(dlgAnchors[i])), HIBYTE(dlgAnchors[i]));
      }
      SetWindowLong(wnd, GWL_USERDATA, (LONG) WndAnchorsInit(wnd));
      // v1.06 load settings
      InitSettings(wnd);
      // must be true because there are modified controls and styles
      result = TRUE;
      break;
    case WM_COMMAND:
      if (HIWORD(wparm) == BN_CLICKED) {
        switch (LOWORD(wparm)) {
          // exit from program
          case IDCANCEL:
            // v1.06 save settings
            KeepSettings(wnd);
            // cleanup
            WndAnchorsFree((wacsitem *) GetWindowLong(wnd, GWL_USERDATA));
            SetWindowLong(wnd, GWL_USERDATA, 0);
            // close database
            BaseFree(&bf);
            // PATCH: Windows 98 bug - don't allow to remove items by internal code
            CtrlInitDone(GetDlgItem(wnd, IDC_LIST), CID_LIST | CID_INIT);
            // end dialog
            EndDialog(wnd, 0);
            break;
          case IDC_FIND:
            // 0.00 - 4000.00
            EnergyFind(wnd, &bf,
              GetAndFixEditFloat(GetDlgItem(wnd, IDC_ITEM)),
              GetAndFixEditFloat(GetDlgItem(wnd, IDC_DIFF))
            );
            break;
          case IDC_XCHG:
            InvertLines(GetDlgItem(wnd, IDC_LINE));
            break;
          case IDC_SHOW:
            EnergyFind(wnd, &bf,
              // anything
              0,
              (DWORD) -1
            );
            break;
          case IDC_COPY:
            // select all
            SendDlgItemMessage(wnd, IDC_INFO, EM_SETSEL, 0, -1);
            // copy to clipboard
            SendDlgItemMessage(wnd, IDC_INFO, WM_COPY, 0, 0);
            break;
          case IDC_SITE:
            // get last line
            *((WORD *)z) = 99;
            i = SendDlgItemMessage(wnd, IDC_INFO, EM_GETLINE,
              // v1.06 fix for long formula word wrapping lines
              SendDlgItemMessage(wnd, IDC_INFO, EM_GETLINECOUNT, 0, 0) - 1,
              (LPARAM) z
            );
            z[i] = 0;
            // skip to URL
            for (s = z; *s && (*s != TEXT(':')); s++);
            for (s++; *s == TEXT(' '); s++);
            if (*s) {
              // open url
              CoInitialize(NULL);
              ShellExecute(wnd, NULL, s, NULL, NULL, SW_SHOWNORMAL);
              CoUninitialize();
            }
            break;
        }
      }
      // update spectral lines in listview by combobox selected item
      if (wparm == MAKELONG(IDC_ELEM, CBN_SELCHANGE)) {
        AddSpectralLines(wnd, &bf);
      }
      break;
    case WM_NOTIFY:
      // only for listview
      if (wparm == IDC_LIST) {
        nmlv = (NM_LISTVIEW *) lparm;
        // update element info
        if ((nmlv->hdr.code == NM_CLICK) || (nmlv->hdr.code == LVN_ITEMCHANGED)) {
          ShowInfo(wnd, &bf);
        }
        // sort by clicked column header
        if ((nmlv->hdr.code == LVN_COLUMNCLICK) && (nmlv->iItem == -1)) {
          // load sort order
          i = GetWindowLong(nmlv->hdr.hwndFrom, GWL_USERDATA);
          // ascending or descending if this column already sorted
          if (LOBYTE(i) == nmlv->iSubItem) {
            i = HIBYTE(i) ^ 1;
          } else {
            i = 0;
          }
          // make sort order
          i = MAKEWORD(nmlv->iSubItem, i);
          // sort list
          ListView_SortItems(nmlv->hdr.hwndFrom, ItemSortFunc, i);
          // save sort order
          SetWindowLong(nmlv->hdr.hwndFrom, GWL_USERDATA, i);
          // v1.06 scroll to selected item
          ListView_EnsureVisible(
            nmlv->hdr.hwndFrom,
            ListView_GetNextItem(nmlv->hdr.hwndFrom, -1, LVNI_FOCUSED),
            FALSE
          );
        }
      }
      break;
    case WM_SIZE:
    case WM_SIZING:
      WndAnchorsSize((wacsitem *) GetWindowLong(wnd, GWL_USERDATA), (umsg == WM_SIZE) ? NULL : ((RECT *) lparm));
      result = TRUE;
      break;
  }
  return(result);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  // DialogBox*() won't work with manifest.xml in resources if this never called
  // or comctl32.dll not linked to the executable file
  // or not called DLLinit through LoadLibrary(TEXT("comctl32.dll"))
  InitCommonControls();

  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CHEMBASE), 0, &DlgPrc, 0);

  ExitProcess(0);
  return(0);
}
