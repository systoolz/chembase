#ifndef __WANCHORS_H
#define __WANCHORS_H

#include <windows.h>

#define WACS_LEFT   1
#define WACS_RIGHT  2
#define WACS_TOP    4
#define WACS_BOTTOM 8

#pragma pack(push, 1)
typedef struct {
  DWORD flag;
  HWND wh;
  RECT rc;
} wacsitem;
#pragma pack(pop)

void WndAnchorsFlag(HWND wnd, DWORD flag);
wacsitem *WndAnchorsInit(HWND wnd);
void WndAnchorsSize(wacsitem *wi, RECT *mr);
void WndAnchorsFree(wacsitem *wi);

#endif
