APP             =scdir
ICONTEXT        ="SCEZ SCDir"
ICON            =scdir.bmp
APPID           =Scl8
RCP             =$(APP).rcp
PRC             =$(APP).prc
SRC             =$(APP).c
GRC             =$(APP).grc
BIN             =$(APP).bin

CC              =m68k-palmos-coff-gcc
PILRC           =pilrc -L ENGLISH
TXT2BITM        =txt2bitm
OBJRES          =m68k-palmos-coff-obj-res
BUILDPRC        =build-prc
BINDIR          =bin

# uncomment this if you want to build a gdb debuggable version
#DEFINES        =-DDEBUG
#INCLUDES =  -I ../PalmOS2 -I../PalmOS2/UI -I../PalmOS2/System -I../PalmOS2/Hardware
#LINKFILES      =-l/usr/local/PalmPilot/m68k-palmos-coff/include/math.h
# LDFLAGS: including -g links gcrt0.o instead of crt0.o
#LDFLAGS         =
# -On: n=Optimization-level (0=none), -g: needed for debugging
#CFLAGS          =-O0 -g $(DEFINES) $(INCLUDES)
CFLAGS          =-O6 -Wall $(DEFINES) $(INCLUDES)
INCLUDES        =-I../..
LDFLAGS         =-L../.. -lSIOLight -lSCEZcard1 -lSCEZmain -lSCEZcard2 -lSCEZreader

all:            $(BINDIR) $(BINDIR)/$(PRC)

$(BINDIR): ; mkdir $(BINDIR)

$(BINDIR)/$(PRC): $(BINDIR)/grc.stamp $(BINDIR)/bin.stamp $(ICON); $(shell cd $(BINDIR); $(BUILDPRC) ../$(PRC) $(ICONTEXT) $(APPID) *.grc *.bin $(LINKFILES))
	ls -l *.prc

$(BINDIR)/grc.stamp:    $(BINDIR)/$(APP) ; $(shell cd $(BINDIR); $(OBJRES) $(APP))
	touch $@

$(BINDIR)/$(APP):       $(BINDIR)/$(SRC:.c=.o) ; $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	cp $@ $(APP)

$(BINDIR)/bin.stamp:    $(RCP) ; $(PILRC) $^ $(BINDIR)
	touch $@

$(BINDIR)/%.o:  %.c ; $(CC) $(CFLAGS) -c $< -o $@
#               touch $<
# enable this line if you want to compile EVERY time.

depend dep: ; $(CC) -M $(SRC) > .dependencies

clean:
	rm -rf *.o $(APP) *.bin *.grc *.prc $(BINDIR)/* *~

