#include "BaseUnit.h"

void BaseFree(bin_file *bf) {
  if (bf) {
    if (bf->head) { UnmapViewOfFile(bf->head); }
    if (bf->fm) { CloseHandle(bf->fm); }
    if (bf->fl != INVALID_HANDLE_VALUE) { CloseHandle(bf->fl); }
    ZeroMemory(bf, sizeof(bf[0]));
    bf->fl = INVALID_HANDLE_VALUE;
  }
}

void BaseLoad(TCHAR *filename, bin_file *bf) {
DWORD sz;
  if (filename && bf) {
    ZeroMemory(bf, sizeof(bf[0]));
    bf->fl = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (bf->fl != INVALID_HANDLE_VALUE) {
      sz = GetFileSize(bf->fl, NULL);
      if (sz >= sizeof(bf->head[0])) {
        bf->fm = CreateFileMapping(bf->fl, NULL, PAGE_READONLY, 0, 0, NULL);
        if (bf->fm) {
          bf->head = (bin_head *) MapViewOfFile(bf->fm, FILE_MAP_READ, 0, 0, 0);
          if (bf->head) {
            if (
              (bf->head->sign1 == 0x6D656843) && // Chem
              (bf->head->sign2 == 0x65736142) && // Base
              (sz == bf->head->fsize)            // filesize
            ) {
              bf->data = (BYTE *) bf->head;
              bf->data += sizeof(bf->head[0]);
            }
          }
        }
      }
      // any error - cleanup
      if (!bf->data) {
        BaseFree(bf);
      }
    }
  }
}

void *BaseData(bin_file *bf, DWORD ntab) {
void *result;
DWORD *offs;
  result = NULL;
  // sanity check
  if (bf && bf->data && (ntab < bf->head->count)) {
    offs = (DWORD *) bf->data;
    // table in range
    if (ntab < (*offs / 4)) {
      result = &bf->data[offs[ntab]];
    }
  }
  return(result);
}

void *BaseItem(bin_file *bf, DWORD ntab, WORD nidx) {
WORD *w;
BYTE *p;
  p = (BYTE *) BaseData(bf, ntab);
  if (p) {
    // item in range
    w = (WORD *) p;
    if (nidx < *w) {
      p += ntab ? (w[nidx] * 2) : (2 + (nidx * sizeof(bin_item)));
    } else {
      p = NULL;
    }
  }
  return(p);
}
