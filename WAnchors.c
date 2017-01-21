#include "WAnchors.h"

// it's a fat chance that lower bits already used for something
void WndAnchorsFlag(HWND wnd, DWORD flag) {
  SetWindowLong(wnd, GWL_USERDATA, ((flag & 0x0F) << 28) | GetWindowLong(wnd, GWL_USERDATA));
}

wacsitem *WndAnchorsInit(HWND wnd) {
DWORD i, c, m, f;
wacsitem *wi;
RECT rc;
HWND wh;
  wi = NULL;
  // sanity check
  if (IsWindow(wnd)) {
    m = 0;
    // original window rect
    GetWindowRect(wnd, &rc);
    MapWindowPoints(0, wnd, (POINT *) &rc, 2);
    rc.right -= rc.left;
    rc.bottom -= rc.top;
    // save sizes for all controls
    for (i = 0; i < 2; i++) {
      // affected controls (with flags) counter; +1 for main window
      c = 1;
      for (wh = FindWindowEx(wnd, 0, NULL, NULL); wh; wh = FindWindowEx(wnd, wh, NULL, NULL)) {
        f = GetWindowLong(wh, GWL_USERDATA);
        // control bits set?
        if ((f >> 28) & 0x0F) {
          // second step, memory allocated
          if (i) {
            // new control?!
            if (c >= m) { break; }
            GetWindowRect(wh, &wi[c].rc);
            MapWindowPoints(0, wnd, (POINT *) &wi[c].rc, 2);
            // convert into padding to the edges
            wi[c].rc.right  = rc.right  - wi[c].rc.right;
            wi[c].rc.bottom = rc.bottom - wi[c].rc.bottom;
            wi[c].wh = wh;
            wi[c].flag = (f >> 28) & 0x0F;
            // remove flags
            SetWindowLong(wh, GWL_USERDATA, f & 0x0FFFFFFF);
          }
          c++;
        }
      }
      // allocate buffer
      if (!i) {
        wi = (wacsitem *) LocalAlloc(LPTR, c * sizeof(wi[0]));
        m = c;
      }
      // ups!
      if (!wi) {
        break;
      }
    }
    if (wi) {
      // save main window original size
      GetWindowRect(wnd, &wi[0].rc);
      MapWindowPoints(0, wnd, (POINT *) &wi[0].rc, 2);
      wi[0].rc.right -= wi[0].rc.left;
      wi[0].rc.bottom -= wi[0].rc.top;
      wi[0].rc.left = min(c, m); // items count
      f = GetWindowLong(wnd, GWL_USERDATA);
      wi[0].wh = wnd;
      wi[0].flag = LOBYTE(f >> 28) & 0x0F;
      // remove flags
      SetWindowLong(wnd, GWL_USERDATA, f & 0x0FFFFFFF);
    }
  }
  return(wi);
}

void WndAnchorsSize(wacsitem *wi, RECT *mr) {
int x, y, w, h;
DWORD i;
RECT rc;
  // sanity check
  if (wi) {
    // if WM_SIZING - only recalc
    if (mr) {
      if ((mr->right - mr->left) < wi[0].rc.right) {
        mr->right = mr->left + wi[0].rc.right;
      }
      if ((mr->bottom - mr->top) < wi[0].rc.bottom) {
        mr->bottom = mr->top + wi[0].rc.bottom;
      }
    } else {
      // mr is NULL - WM_SIZED - recalc all
      GetWindowRect(wi[0].wh, &rc);
      MapWindowPoints(0, wi[0].wh, (POINT *) &rc, 2);
      rc.right -= rc.left;
      rc.bottom -= rc.top;
      // first of all check main windows size
      if (rc.right < wi[0].rc.right) {
        rc.right = wi[0].rc.right;
      }
      if (rc.bottom < wi[0].rc.bottom) {
        rc.bottom = wi[0].rc.bottom;
      }
      for (i = 1; i < wi[0].rc.left; i++) {
        // get original sizes
        x = wi[i].rc.left;
        y = wi[i].rc.top;
        w = wi[i].rc.right;
        h = wi[i].rc.bottom;
        w = wi[0].rc.right - x - w;
        h = wi[0].rc.bottom - y - h;
        // x / width
        if ((wi[i].flag & (WACS_LEFT | WACS_RIGHT)) == (WACS_LEFT | WACS_RIGHT)) {
        // both flags set - change width
          w = rc.right - x - wi[i].rc.right;
        } else {
          // right flag set - change x
          if (wi[i].flag & WACS_RIGHT) {
            x = rc.right - w - wi[i].rc.right;
          }
        }
        // y / height
        if ((wi[i].flag & (WACS_TOP | WACS_BOTTOM)) == (WACS_TOP | WACS_BOTTOM)) {
          // both flags set - change height
          h = rc.bottom - y - wi[i].rc.bottom;
        } else {
          // bottom flag set - change y
          if (wi[i].flag & WACS_BOTTOM) {
            y = rc.bottom - h - wi[i].rc.bottom;
          }
        }
        // new size
        MoveWindow(wi[i].wh, x, y, w, h, FALSE);
      }
      // only left and top
      MapWindowPoints(wi[0].wh, 0, (POINT *) &rc, 1);
      // move main window
      MoveWindow(wi[0].wh, rc.left, rc.top, rc.right, rc.bottom, FALSE);
      // fix graphical glitches
      InvalidateRect(wi[0].wh, NULL, TRUE);
    }
  }
}

void WndAnchorsFree(wacsitem *wi) {
  // sanity check
  if (wi) {
    LocalFree(wi);
  }
}
