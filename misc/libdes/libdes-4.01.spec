Summary: DES Library.
Name: libdes
Version: 4.01
Release: 3
ExclusiveArch: i386
Copyright: BSD style
Group: Development/Libraries
Source: libdes-4.01.tar.gz
Patch0: libdes-4.01-shared.patch
Buildroot: /var/tmp/libdes-%{PACKAGE_VERSION}-root

%description
Libdes is the DES library from Eric A. Young. It contains all DES modes
someone can think of and is very efficient implemented.

This package does not use the assembler routines.

%prep
%setup -c -n libdes-%{PACKAGE_VERSION}
cd $RPM_BUILD_DIR
%patch0

%build
make -f Makefile clean
make -f Makefile depend
make -f Makefile x86-elf
export LD_LIBRARY_PATH=.
./destest

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
install -s -m 755 des $RPM_BUILD_ROOT/usr/bin/des
mkdir -p $RPM_BUILD_ROOT/usr/include
install -m 644 des.h $RPM_BUILD_ROOT/usr/include/des.h
mkdir -p $RPM_BUILD_ROOT/usr/lib
install -m 644 libdes.so $RPM_BUILD_ROOT/usr/lib/libdes.so.4.1
mkdir -p $RPM_BUILD_ROOT/usr/man/man1
install -m 644 des.man $RPM_BUILD_ROOT/usr/man/man1/des.1
mkdir -p $RPM_BUILD_ROOT/usr/man/man3
install -m 644 des_crypt.man $RPM_BUILD_ROOT/usr/man/man3/des_crypt.3
( cd $RPM_BUILD_ROOT/usr/lib; ln -s libdes.so.4.1 libdes.so.4 )
( cd $RPM_BUILD_ROOT/usr/lib; ln -s libdes.so.4 libdes.so )

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYRIGHT FILES INSTALL KERBEROS MODES.DES README VERSION des.doc
/usr/lib/libdes.so
/usr/lib/libdes.so.4
/usr/lib/libdes.so.4.1
/usr/bin/des
/usr/man/man3/des_crypt.3
/usr/man/man1/des.1
/usr/include/des.h

%changelog
* Tue Mar 7 2000 Matthias Bruestle <m@mbsks.franken.de>
- Patch for PIC.
 
* Tue Dec 7 1999 Matthias Bruestle <m@mbsks.franken.de>
- Second RPM ever.

