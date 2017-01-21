#ifndef __BASEUNIT_H
#define __BASEUNIT_H

#include <windows.h>

#pragma pack(push, 1)
typedef struct {
  DWORD sign1;
  DWORD sign2;
  DWORD count;
  DWORD  size;
} bin_head;

typedef struct {
  DWORD energy;
  DWORD detail;
  WORD element;
  BYTE sz_spec;
  BYTE sz_form;
//  CCHAR line[sz_spec];
//  CCHAR form[sz_form];
} bin_item;

typedef struct {
  bin_head *head;
  BYTE *data;
  HANDLE fm;
  HANDLE fl;
} bin_file;
#pragma pack(pop)

typedef BOOL (WINAPI BASEPROC)(bin_item *bi, void *parm);

void BaseFree(bin_file *bf);
void BaseOpen(TCHAR *filename, bin_file *bf);
void BaseList(bin_file *bf, BASEPROC *proc, void *parm);

#endif
