EE_BIN = assteroids.elf
EE_CBIN = assteroids_comp.elf
EE_OBJS = resources.o maths.o system.o render.o game.o main.o
EE_LIBS = -ldebug -lpad -lgs -ldraw -lgraph -ldma -lpacket -lvux -lmath3d

EE_OPTFLAGS += -std=c99 -Ofast -mload-store-pairs -Wno-missing-braces -Wno-unused-function -Wno-switch -Wno-unused-variable -Wno-unused-label -Wno-multichar

all: $(EE_BIN) strip $(EE_CBIN)

strip: $(EE_BIN)
ifeq ($(STRIP),1)
	$(EE_STRIP) -s $(EE_BIN)
endif

$(EE_CBIN): $(EE_BIN)
	ps2-packer $(EE_BIN) $(EE_CBIN)

clean:
	rm -f $(EE_BIN) $(EE_CBIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN) -D

emu: $(EE_BIN)
	cmd.exe /C "emu.cmd $(EE_BIN)"

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
