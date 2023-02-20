# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = assteroids.elf
EE_OBJS = resources.o maths.o system.o render.o game.o main.o
EE_LIBS = -ldebug -lpad -lgs -ldraw -lgraph -ldma -lpacket -lvux -lmath3d

ifeq ($(RELEASE),1)
	EE_OPTFLAGS = -Wl,--gc-sections -flto -fdata-sections -ffunction-sections
endif

EE_OPTFLAGS += -std=c99 -Ofast -mload-store-pairs -Wno-missing-braces -Wno-unused-function -Wno-switch -Wno-unused-variable -Wno-unused-label -Wno-multichar

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN) -D

emu: $(EE_BIN)
	cmd.exe /C "emu.cmd $(EE_BIN)"

strip: $(EE_BIN)
	$(EE_STRIP) -s $(EE_BIN)

compact: $(EE_BIN)
	mv $(EE_BIN) $(EE_BIN:.elf=.elf.org)
	ps2-packer $(EE_BIN:.elf=.elf.org) $(EE_BIN)

reset:
	ps2client reset

release: strip compact

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
