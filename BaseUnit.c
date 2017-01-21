#include "SysToolX.h"
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

void BaseOpen(TCHAR *filename, bin_file *bf) {
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
              (bf->head->sign1 == 0x4D454843) && // CHEM
              (bf->head->sign2 == 0x45534142) && // BASE
              (sz == bf->head->size)             // filesize
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

void BaseList(bin_file *bf, BASEPROC *proc, void *parm) {
bin_item *bi;
DWORD i;
BYTE *p;
  // sanity check
  if (bf && bf->data && proc) {
    p = bf->data;
    for (i = 0; i < bf->head->count; i++) {
      bi = (bin_item *) p;
      // call user proc
      if (!proc(bi, parm)) {
        break;
      }
      // next row
      p += sizeof(bi[0]);
      p += bi->sz_spec;
      p += bi->sz_form;
    }
  }
}

/*BOOL WINAPI BaseProc(bin_item *bi, void *parm) {
  return(FALSE);
}*/
