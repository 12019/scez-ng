CRYPTLIBPATH=/usr/local/lib
OPENSSLPATH=/usr/local/ssl/lib
PERLINCLUDE=-I/usr/lib/perl5/5.00503/i386-linux/CORE/

HDRS = \
	scgeneral.h \
	screader.h \
	scsmartcard.h \
	scbasiccard.h \
	sccryptoflex.h \
	sccyberflex.h \
	scgeldkarte.h \
	scgpk4000.h \
	scgsmsim.h \
	scmfc.h \
	scmultiflex.h \
	scopenplatform.h \
	scproton.h \
	scquick.h \
	scsmartcafe.h \
	sctcos.h

#	scgeneral.h \

systems:
	@echo "Parameter:"
	@echo ""
	@echo "   <system>"
	@echo "   <system>-libdes"
	@echo "   <system>-cryptlib"
	@echo "   <system>-openssl"
	@echo ""
	@echo "With system:"
	@echo ""
	@echo "   Linux, FreeBSD, HP-UX, IRIX, OSF1"

Linux: $(HDRS)
	swig -perl5 -dascii -I. -DSWIG -D__linux__ scez.i 
	gcc -O3 -Wall -fpic -Dbool=char -DSWIG_C -I. \
		$(PERLINCLUDE) -c scez_wrap.c
	gcc -O3 -Wall -shared -o scez.so scez_wrap.o -L.. -lscez -lsio -ldl \
		-lkeeper

Linux-libdes: $(HDRS)
	swig -perl5 -dascii -I. -DSWIG -DWITH_DES -D__linux__ scez.i 
	gcc -O3 -Wall -fpic -Dbool=char -DSWIG_C -DWITH_DES -I. \
		$(PERLINCLUDE) -c scez_wrap.c
	gcc -O3 -Wall -shared -o scez.so scez_wrap.o -L.. -lscez -lsio -ldl \
		-ldes -lkeeper

Linux-cryptlib: $(HDRS)
	swig -perl5 -dascii -I. -DSWIG -DWITH_DES -D__linux__ scez.i 
	gcc -O3 -Wall -fpic -Dbool=char -DSWIG_C -DWITH_DES -I. \
		$(PERLINCLUDE) -c scez_wrap.c
	gcc -O3 -Wall -shared -o scez.so scez_wrap.o -L.. -lscez -lsio -ldl \
		$(CRYPTLIBPATH) -lcrypt -lkeeper

Linux-openssl: $(HDRS)
	swig -perl5 -dascii -I. -DSWIG -DWITH_DES -D__linux__ scez.i 
	gcc -O3 -Wall -fpic -Dbool=char -DSWIG_C -DWITH_DES -I. \
		$(PERLINCLUDE) -c scez_wrap.c
	gcc -O3 -Wall -shared -o scez.so scez_wrap.o -L.. -lscez -lsio -ldl \
		$(OPENSSLPATH) -lcrypto -lkeeper

clean:
	rm -f core *~ .~* *_wrap* *.so *.pm $(HDRS)

nice:
	rm -f core *~ .~* $(HDRS)

scgeneral.h :
	sed -e 's/WORD/unsigned short/g' <../scgeneral.h >scgeneral.h

screader.h :
	sed -e 's/WORD/unsigned short/g' <../screader.h >screader.h

scsmartcard.h :
	sed -e 's/WORD/unsigned short/g' <../scsmartcard.h >scsmartcard.h

scbasiccard.h :
	sed -e 's/WORD/unsigned short/g' <../scbasiccard.h >scbasiccard.h

sccryptoflex.h :
	sed -e 's/WORD/unsigned short/g' <../sccryptoflex.h >sccryptoflex.h

sccyberflex.h :
	sed -e 's/WORD/unsigned short/g' <../sccyberflex.h >sccyberflex.h

scgeldkarte.h :
	sed -e 's/WORD/unsigned short/g' <../scgeldkarte.h >scgeldkarte.h

scgpk4000.h :
	sed -e 's/WORD/unsigned short/g' <../scgpk4000.h >scgpk4000.h

scgsmsim.h :
	sed -e 's/WORD/unsigned short/g' <../scgsmsim.h >scgsmsim.h

scmfc.h :
	sed -e 's/WORD/unsigned short/g' <../scmfc.h >scmfc.h

scmultiflex.h :
	sed -e 's/WORD/unsigned short/g' <../scmultiflex.h >scmultiflex.h

scopenplatform.h :
	sed -e 's/WORD/unsigned short/g' <../scopenplatform.h >scopenplatform.h

scproton.h :
	sed -e 's/WORD/unsigned short/g' <../scproton.h >scproton.h

scquick.h :
	sed -e 's/WORD/unsigned short/g' <../scquick.h >scquick.h

scsmartcafe.h :
	sed -e 's/WORD/unsigned short/g' <../scsmartcafe.h >scsmartcafe.h

sctcos.h :
	sed -e 's/WORD/unsigned short/g' <../sctcos.h >sctcos.h


