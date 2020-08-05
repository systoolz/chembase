# GCC makefile
CC=gcc
LD=ld
EXEC_FILE=ChemBase.exe
LIB_PATH?=.
CFLAGS= -Os -s -Wall -pedantic -mwindows -I.
LDFLAGS= --strip-all --subsystem windows -L $(LIB_PATH) -l kernel32 -l user32 -l shell32 -l comctl32 -l ole32 -nostdlib --exclude-libs msvcrt.a -e_WinMain@16

OBJ_EXT=.o
OBJS=ChemBase${OBJ_EXT} BaseUnit${OBJ_EXT} SysToolX${OBJ_EXT} WAnchors${OBJ_EXT} Settings${OBJ_EXT}

all: $(EXEC_FILE)

$(EXEC_FILE): $(OBJS)
	${LD} ${OBJS} resource/ChemBase.res -o $@ ${LDFLAGS}


o=${OBJ_EXT}

wipe:
	@rm -rf *${OBJ_EXT}

clean:	wipe all
