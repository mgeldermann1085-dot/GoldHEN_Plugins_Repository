TARGET  = pes_thigh
ODIR    = ../$(TARGET)/build
SDIR    = ../$(TARGET)/source
IDIR    = ../$(TARGET)/include
OUTDIR  = ../../bin/plugins
ELFDIR  = $(OUTDIR)/elf_final
PRXDIR  = $(OUTDIR)/prx_final/$(TARGET)

CC      = clang
LD      = ld.lld
CSELF   = $(OO_PS4_TOOLCHAIN)/bin/linux/create-fself

CFLAGS  = -D__FINAL__=1 --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables \
          -c -D__FINAL__=1 -D__USE_PRINTF__ -fcolor-diagnostics -Wall \
          -isysroot $(OO_PS4_TOOLCHAIN) \
          -isystem $(OO_PS4_TOOLCHAIN)/include \
          -I$(GOLDHEN_SDK)/include \
          -I$(IDIR) \
          -I../../common

LDFLAGS = -m elf_x86_64 -pie \
          --script $(OO_PS4_TOOLCHAIN)/link.x \
          -e _init --eh-frame-hdr \
          -L$(OO_PS4_TOOLCHAIN)/lib \
          -L$(GOLDHEN_SDK) \
          -lSceLibcInternal -lGoldHEN_Hook -lkernel -lSceSysmodule

SRCS    = $(wildcard $(SDIR)/*.c)
OBJS    = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(PRXDIR)/$(TARGET).prx

$(ODIR):
	mkdir -p $(ODIR)

$(ELFDIR):
	mkdir -p $(ELFDIR)

$(PRXDIR):
	mkdir -p $(PRXDIR)

$(ODIR)/%.o: $(SDIR)/%.c | $(ODIR)
	$(CC) $(CFLAGS) -o $@ $<

$(ELFDIR)/$(TARGET).elf: $(OBJS) | $(ELFDIR)
	$(LD) $(GOLDHEN_SDK)/build/crtprx.o $(OBJS) -o $@ $(LDFLAGS)

$(PRXDIR)/$(TARGET).prx: $(ELFDIR)/$(TARGET).elf | $(PRXDIR)
	$(CSELF) -in=$< -out=$(ELFDIR)/$(TARGET).oelf --lib=$@ --paid 0x3800000000000011

clean:
	rm -rf $(ODIR) $(ELFDIR)/$(TARGET).elf $(ELFDIR)/$(TARGET).oelf $(PRXDIR)
