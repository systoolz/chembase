#ifndef __BASEUNIT_H
#define __BASEUNIT_H

#include <windows.h>

#pragma pack(push, 1)
typedef struct {
  DWORD sign1;
  DWORD sign2;
  DWORD fsize;
  DWORD count;
} bin_head;

typedef struct {
  BYTE element;
  BYTE spcline;
  WORD formula;
  DWORD energy;
  DWORD detail;
} bin_item;

typedef struct {
  bin_head *head;
  BYTE *data;
  HANDLE fm;
  HANDLE fl;
} bin_file;
#pragma pack(pop)

void BaseFree(bin_file *bf);
void BaseLoad(TCHAR *filename, bin_file *bf);
void *BaseData(bin_file *bf, DWORD ntab);
void *BaseItem(bin_file *bf, DWORD ntab, WORD nidx);

#endif
