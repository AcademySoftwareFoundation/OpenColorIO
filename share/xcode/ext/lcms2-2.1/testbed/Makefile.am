#
# Makefile for building testcms
#

# Don't require all the GNU mandated files
AUTOMAKE_OPTIONS = 1.7 foreign

INCLUDES	=  -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_srcdir)/src

check_PROGRAMS = testcms

CFLAGS = --pedantic -Wall -std=c99 -O2

testcms_LDADD = $(top_builddir)/src/liblcms2.la 
testcms_LDFLAGS = @LDFLAGS@
testcms_SOURCES = testcms2.c

EXTRA_DIST = sRGBSpac.icm bad.icc toosmall.icc UncoatedFOGRA29.icc \
             USWebCoatedSWOP.icc sRGB_v4_ICC_preference.icc \
             sRGB_Color_Space_Profile.icm 

check:
	if [ $(top_srcdir) != $(top_builddir) ]; then \
		cp $(top_srcdir)/testbed/*.ic? $(top_builddir)/testbed; \
	fi
	./testcms
	if [ $(top_srcdir) != $(top_builddir) ]; then \
		rm -f $(top_builddir)/testbed/*.ic?; \
	fi


