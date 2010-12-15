PROJECT = ocioconvert
PROJECT_TYPE = bin
OPT_LEVEL = 3
OUT = out

CXXFILES = $(shell find $(SRCDIR)/src -name \*.cpp)

SPCOMP2_DEPEND = OpenImageIO?v=29

CFLAGS += -Wall -DOPENIMAGEIO_NAMESPACE=SPI_OpenImageIO_v29

IPATH += -I/net/soft_scratch/users/color/ocio/0.7.2/spinux1/include/
LPATH += -L/net/soft_scratch/users/color/ocio/0.7.2/spinux1/lib/
LDFLAGS += -lOpenColorIO -ldl

MAKEFILE = Makefile
MAKEFILE_SOURCE = /usr/local/spi/lib/make
include ${MAKEFILE_SOURCE}/Makefile.unique
