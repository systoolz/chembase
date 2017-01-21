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

static TCHAR dlgText[] = TEXT("0.00\0Element\0Spectral Line\0Formula\0Energy (eV)\0Details");
static TCHAR BaseFile[] = TEXT("ChemBase.bin");
static TCHAR fmtStr[] = TEXT("%lu.%02lu");
// anchors for controls
static WORD dlgAnchors[] = {
  MAKEWORD(IDC_LIST, WACS_LEFT  | WACS_RIGHT | WACS_TOP | WACS_BOTTOM),
  MAKEWORD(IDC_INFO, WACS_LEFT  | WACS_RIGHT | WACS_BOTTOM),
  MAKEWORD(IDC_COPY, WACS_RIGHT | WACS_BOTTOM),
  MAKEWORD(IDC_SITE, WACS_RIGHT | WACS_BOTTOM),
  MAKEWORD(IDC_XCHG, WACS_RIGHT | WACS_TOP),
  MAKEWORD(IDC_SHOW, WACS_RIGHT | WACS_TOP),
  MAKEWORD(IDC_LINE, WACS_RIGHT | WACS_TOP   | WACS_BOTTOM),
  MAKEWORD(IDC_SPEC, WACS_RIGHT | WACS_TOP),
  0
};
static BYTE dlgCols[] = {50, 80, 199, 70, 60, 0};

#define FIND_COMBOBOX 0x10000000
#define FIND_CHECKBOX 0x20000000

static bin_file bf;

#pragma pack(push, 1)
typedef struct {
  HWND wnd[2];
  DWORD data[3];
} usr_parm;
#pragma pack(pop)

// BYTE (ANSI) to TCHAR
TCHAR *ANSI2Unicode(TCHAR *s, BYTE *p, DWORD sz) {
  while (sz--) {
    *s = *p;
    p++;
    s++;
  }
  *s = 0;
  s++;
  return(s);
}

void RowStrFmt(bin_item *bi, TCHAR *s) {
  // sanity check
  if (bi && s) {
    // element
    s = ANSI2Unicode(s, (BYTE *)&bi->element, HIBYTE(bi->element) ? 2 : 1);
    // spectral line
    s = ANSI2Unicode(s, (BYTE *)&bi[1], bi->sz_spec - 1);
    // formula
    s = ANSI2Unicode(s, ((BYTE *)(&bi[1])) + bi->sz_spec, bi->sz_form - 1);
    // energy
    wsprintf(s, fmtStr, bi->energy / 100, bi->energy % 100);
    s += lstrlen(s) + 1;
    // details prefix "R-"
    s[0] = LOBYTE(bi->detail >> 24);
    s[1] = (LOBYTE(s[0]) >> 7) * TEXT('-');
    s[0] &= 0x7F;
    s[2] = 0;
    s += lstrlen(s);
    wsprintf(s, TEXT("%lu"), bi->detail & 0xFFFFFF);
    s += lstrlen(s) + 1;
    // two sequental nulls - terminator
    *s = 0;
  }
}

int AddItemToListView(HWND wnd, TCHAR *s, LPARAM lparm) {
LV_ITEM li;
  li.iItem = -1;
  // sanity check
  if (wnd && s) {
    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_TEXT | LVIF_PARAM;
    li.pszText = s;
    li.lParam = lparm;
    li.iItem = ListView_GetItemCount(wnd);
    ListView_InsertItem(wnd, &li);
    li.mask ^= LVIF_PARAM;
    // subitems
    for (li.pszText += (lstrlen(li.pszText) + 1); li.pszText[0]; li.pszText += (lstrlen(li.pszText) + 1)) {
      li.iSubItem++;
      ListView_SetItem(wnd, &li);
    }
  }
  return(li.iItem);
}

BOOL WINAPI FindProc(bin_item *bi, void *parm) {
TCHAR s[1024];
LV_FINDINFO lf;
usr_parm *up;
  // user params
  up = (usr_parm *) parm;
  switch (LOBYTE(up->data[2] >> 28) & 0x0F) {
    case 0: // main search reasult
      // check element
      lf.flags = LOWORD(up->data[2]);
      // some element selected
      if (lf.flags == bi->element) {
        // spectral line
        ANSI2Unicode(s, (BYTE *)&bi[1], bi->sz_spec);
        // test on intersection with the selected lines
        ZeroMemory(&lf, sizeof(lf));
        lf.flags = LVFI_STRING;
        lf.psz = s;
        lf.flags = ListView_FindItem(up->wnd[1], -1, &lf);
        // found and checked
        lf.flags = ((lf.flags != (DWORD) -1) && (ListView_GetCheckState(up->wnd[1], lf.flags))) ? 0 : 1;
      }
      // pass filter and energy in the range
      if ((!lf.flags) && (bi->energy >= up->data[0]) && (bi->energy <= up->data[1])) {
        RowStrFmt(bi, s);
        AddItemToListView(up->wnd[0], s, (LPARAM) bi);
      }
      break;
    case 1: // FIND_COMBOBOX
      // element name
      ANSI2Unicode(s, (BYTE *)&bi->element, 2);
      // not in combobox already
      if (SendMessage(up->wnd[0], CB_FINDSTRINGEXACT, -1, (LPARAM) s) == CB_ERR) {
        SendMessage(up->wnd[0], CB_ADDSTRING, 0, (LPARAM) s);
      }
      break;
    case 2: // FIND_CHECKBOX
      if (bi->element == up->data[0]) {
        // spectral line
        ANSI2Unicode(s, (BYTE *)&bi[1], bi->sz_spec);
        // init structure
        ZeroMemory(&lf, sizeof(lf));
        lf.flags = LVFI_STRING;
        lf.psz = s;
        // not in listbox already
        if (ListView_FindItem(up->wnd[0], -1, &lf) == -1) {
          // add and checked by default
          ListView_SetCheckState(up->wnd[0], AddItemToListView(up->wnd[0], s, 0), TRUE);
        }
      }
      break;
  }
  return(TRUE);
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
  // convert to digit
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
DWORD i;
  // flag: init
  if (flag & CID_INIT) {
    // prevent update
    SendMessage(wnd, WM_SETREDRAW, FALSE, 0);
    if (flag & CID_CBOX) {
      // combobox
      SendMessage(wnd, CB_RESETCONTENT, 0, 0);
      // first item must be empty
      flag = 0;
      SendMessage(wnd, CB_ADDSTRING, 0, (LPARAM) &flag);
    } else {
      // listview
      // PATCH: Windows 98 can add 27k+ items just fine (around 10 seconds)
      // but deleting with ListView_DeleteAllItems() can take up to whole 10 minutes!
      // items MUST be deleted from last to first (around 15 seconds) and not vice versa
      // or it takes the same 10 minutes (the Microsoft way)
      // this bug was fixed at least in Windows XP and newer
      for (i = ListView_GetItemCount(wnd); i; i--) {
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

void EnergyFind(HWND wnd, bin_file *bfx, DWORD vitem, DWORD vdiff) {
usr_parm up;
TCHAR s[3];
  // vmin / vmax
  up.data[0] = vitem - min(vitem, vdiff);
  up.data[1] = vitem + vdiff;
  // additional filter - element
  GetDlgItemText(wnd, IDC_ELEM, s, 3);
  up.data[2] = s[0] ? MAKEWORD(s[0], s[1]) : 0;
  // update window handle
  up.wnd[0] = GetDlgItem(wnd, IDC_LIST);
  up.wnd[1] = GetDlgItem(wnd, IDC_LINE);
  // init
  CtrlInitDone(up.wnd[0], CID_LIST | CID_INIT);
  // find items
  BaseList(bfx, FindProc, &up);
  // done
  CtrlInitDone(up.wnd[0], CID_LIST | CID_DONE);
}

void AddElements(HWND wnd, bin_file *bfx) {
usr_parm up;
  up.wnd[0] = GetDlgItem(wnd, IDC_ELEM);
  // combobox flag
  up.data[2] = FIND_COMBOBOX;
  // init
  CtrlInitDone(up.wnd[0], CID_CBOX | CID_INIT);
  // find items
  BaseList(bfx, FindProc, &up);
  // done
  CtrlInitDone(up.wnd[0], CID_CBOX | CID_DONE);
  // fix combobox height
  AdjustComboBoxHeight(up.wnd[0], 8);
}

void AddSpectralLines(HWND wnd, bin_file *bfx) {
usr_parm up;
TCHAR s[3];
  up.wnd[0] = GetDlgItem(wnd, IDC_LINE);
  // checkbox flag
  up.data[2] = FIND_CHECKBOX;
  // init
  CtrlInitDone(up.wnd[0], CID_LIST | CID_INIT);
  // got something to find?
  GetDlgItemText(wnd, IDC_ELEM, s, 3);
  if (s[0]) {
    up.data[0] = MAKEWORD(s[0], s[1]);
    // find items
    BaseList(bfx, FindProc, &up);
  }
  // at least one item
  up.data[0] = ListView_GetItemCount(up.wnd[0]) ? TRUE : FALSE;
  DialogEnableWindow(wnd, IDC_XCHG, up.data[0]);
  DialogEnableWindow(wnd, IDC_SHOW, up.data[0]);
  // done
  CtrlInitDone(up.wnd[0], CID_LIST | CID_DONE);
}

void ShowInfo(HWND wnd) {
TCHAR s[1024], z[1024], *fmt, *x, *p[5];
LV_ITEM li;
DWORD i;
HWND wh;
  ZeroMemory(&li, sizeof(li));
  wh = GetDlgItem(wnd, IDC_LIST);
  // find selected item
  li.iItem = ListView_GetNextItem(wh, -1, LVNI_SELECTED);
  if (li.iItem != -1) {
    // get selected item
    li.mask = LVIF_PARAM;
    ListView_GetItem(wh, &li);
    RowStrFmt((bin_item *) li.lParam, s);
    fmt = LangLoadString(IDS_INFO);
    if (fmt) {
      x = s;
      for (i = 0; i < 5; i++) {
        p[i] = x;
        x += *x ? (lstrlen(x) + 1) : 0;
      }
      wsprintf(z, fmt, p[0], p[1], p[2], p[3], p[4]);
      lstrcpy(s, z);
      FreeMem(fmt);
    } else {
      s[0] = 0;
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

int CALLBACK ItemSortFunc(LPARAM lparm1, LPARAM lparm2, LPARAM lparmsort) {
bin_item *bi1, *bi2;
int result;
  bi1 = (bin_item *) lparm1;
  bi2 = (bin_item *) lparm2;
  switch (LOBYTE(lparmsort)) {
    case 0: // element
      result = LOBYTE(bi1->element);
      result -= LOBYTE(bi2->element);
      if (!result) {
        result = HIBYTE(bi1->element);
        result -= HIBYTE(bi2->element);
      }
      break;
    case 1: // spectral line
      result = lstrcmpA((CCHAR *)&bi1[1], (CCHAR *)&bi2[1]);
      break;
    case 2: // formula
      result = lstrcmpA(((CCHAR *)&bi1[1]) + bi1->sz_spec, ((CCHAR*)&bi2[1]) + bi2->sz_spec);
      break;
    case 4: // details
      result = bi1->detail & 0x7FFFFFFF;
      result -= bi2->detail & 0x7FFFFFFF;
      break;
    case 3: // energy
    default: // default
      result = bi1->energy;
      result -= bi2->energy;
      break;
  }
  // ascending or descending sort order
  result *= HIBYTE(lparmsort) ? -1 : 1;
  return(result);
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
      // check database after icons
      // or window icon will be empty in taskbar
      BaseOpen(BaseFile, &bf);
      // failed?
      if (!bf.data) {
        // report error
        MsgBox(wnd, MAKEINTRESOURCE(IDS_ERR_BASE), MB_ICONERROR | MB_OK);
        // close window
        PostMessage(wnd, WM_COMMAND, MAKELONG(IDCANCEL, BN_CLICKED), 0);
        break;
      }
      // copyrights, etc.
      s = LangLoadString(IDS_HELP);
      if (s) {
        SetDlgItemText(wnd, IDC_INFO, s);
        FreeMem(s);
      }
      // limit text length
      SendDlgItemMessage(wnd, IDC_ITEM, EM_LIMITTEXT, 10, 0);
      SendDlgItemMessage(wnd, IDC_DIFF, EM_LIMITTEXT, 10, 0);
      // default texts
      s = dlgText;
      SetDlgItemText(wnd, IDC_ITEM, s);
      SetDlgItemText(wnd, IDC_DIFF, s);
      // add columns
      ZeroMemory(&lc, sizeof(lc));
      lc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
      lc.fmt = LVCFMT_LEFT;
      wh = GetDlgItem(wnd, IDC_LIST);
      for (i = 0; dlgCols[i]; i++) {
        s += lstrlen(s) + 1;
        lc.cx = dlgCols[i];
        lc.pszText = s;
        ListView_InsertColumn(wh, i, &lc);
      }
      // fix list report styles
      SendMessage(wh, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT/* | LVS_EX_GRIDLINES*/);
      // default sort order by energy ascending
      SetWindowLong(wh, GWL_USERDATA, MAKEWORD(3, 0));
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
      // checkboxes
      SendMessage(wh, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
      // fill in combobox
      AddElements(wnd, &bf);
      // anchors for resize
      for (i = 0; dlgAnchors[i]; i++) {
        WndAnchorsFlag(GetDlgItem(wnd, LOBYTE(dlgAnchors[i])), HIBYTE(dlgAnchors[i]));
      }
      SetWindowLong(wnd, GWL_USERDATA, (LONG) WndAnchorsInit(wnd));
      // must be true because there are modified controls and styles
      result = TRUE;
      break;
    case WM_COMMAND:
      if (HIWORD(wparm) == BN_CLICKED) {
        switch (LOWORD(wparm)) {
          // exit from program
          case IDCANCEL:
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
            // sort by last sort order
            wh = GetDlgItem(wnd, IDC_LIST);
            ListView_SortItems(wh, ItemSortFunc, GetWindowLong(wh, GWL_USERDATA));
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
            i = SendDlgItemMessage(wnd, IDC_INFO, EM_GETLINE, 4, (LPARAM) z);
            z[i] = 0;
            // skip to URL
            for (s = z; *s && (*s != TEXT(':')); s++);
            if (*s) {
              for (s++; *s == TEXT(' '); s++);
            }
            // open url
            CoInitialize(NULL);
            ShellExecute(wnd, NULL, s, NULL, NULL, SW_SHOWNORMAL);
            CoUninitialize();
            break;
        }
      }
      // update spectral lines in listview by combobox selected item
      if (wparm == MAKELONG(IDC_ELEM, CBN_SELCHANGE)) {
        AddSpectralLines(wnd, &bf);
      }
      break;
    case WM_NOTIFY:
      // update element info
      if ((wparm == IDC_LIST) && (((NMHDR *) lparm)->code == LVN_ITEMCHANGED)) {
        ShowInfo(wnd);
      }
      // sort by clicked column header
      if ((wparm == IDC_LIST) && (((NMHDR *) lparm)->code == LVN_COLUMNCLICK)) {
        nmlv = (NM_LISTVIEW *) lparm;
        if (nmlv->iItem == -1) {
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
          // save sort order
          ListView_SortItems(nmlv->hdr.hwndFrom, ItemSortFunc, i);
          // sort list
          SetWindowLong(nmlv->hdr.hwndFrom, GWL_USERDATA, i);
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
