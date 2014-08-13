## automake.am
##
## Automake.am for the WavLib Packages
##
## by Fred Gleason <fredg@salemradiolabs.com>
##
## Use automake to process this into a Makefile.in

SUBDIRS = doc

AM_CFLAGS = -Wall -DPREFIX=\"$(prefix)\" -DQTDIR=\"$(QTDIR)\"


bin_PROGRAMS = agen ameter aproc infowav nmix recwav trim wavgen mptrim

dist_agen_SOURCES = agen.c wavlib.c wavlib.h
agen_LDADD = -lm

dist_ameter_SOURCES = ameter.c wavlib.c wavlib.h
ameter_LDADD = -lm

dist_aproc_SOURCES = aproc.c wavlib.c wavlib.h
aproc_LDADD = -lm

dist_infowav_SOURCES = infowav.c wavlib.c wavlib.h
infowav_LDADD = -lm

dist_nmix_SOURCES = nmix.c wavlib.c wavlib.h
nmix_LDADD = -lm

dist_recwav_SOURCES = recwav.c wavlib.c wavlib.h
recwav_LDADD = -lm

dist_trim_SOURCES = trim.c wavlib.c wavlib.h
trim_LDADD = -lm

dist_wavgen_SOURCES = wavgen.c wavlib.c wavlib.h
wavgen_LDADD = -lm

dist_mptrim_SOURCES = mptrim.c wavlib.c wavlib.h
mptrim_LDADD = -lm

man_MANS = agen.1 ameter.1 aproc.1 infowav.1 mptrim.1 nmix.1 recwav.1\
           trim.1 wavgen.1

EXTRA_DIST = History wavlib.spec.in\
             agen.1 ameter.1 aproc.1 infowav.1 mptrim.1 nmix.1 recwav.1\
             trim.1 wavgen.1 autogen.sh get_target.sh get_distro.sh

rpm:	dist
	cp wavlib-$(VERSION).tar.gz $(RPM_ROOT)/SOURCES/
	$(RPMBUILD) -ba --target $(ARCH)-$(VENDOR)-linux wavlib.spec
	mv $(RPM_ROOT)/RPMS/$(ARCH)/wavlib-$(VERSION)-@RPM_RELEASE@.$(ARCH).rpm $(top_srcdir)/
	rm $(RPM_ROOT)/SOURCES/wavlib-$(VERSION).tar.gz
	rm -rf $(RPM_ROOT)/BUILD/wavlib-$(VERSION)


CLEANFILES = *~ moc_* *.qm *.rpm
DISTCLEANFILES = wavlib.spec
MAINTAINERCLEANFILES = *~ Makefile.in configure aclocal.m4 *.tar.gz\
                       depcomp install-sh missing
