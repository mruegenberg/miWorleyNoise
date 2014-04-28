CC = gcc
CPP = g++

#MAYA_DIR = /Applications/Autodesk/maya2011
#MAYA_DIR = /Applications/Autodesk/maya2012
#MAYA_DIR = /Applications/Autodesk/maya2013
MAYA_DIR = /Applications/Autodesk/maya2015
# Windows standard MAYA_DIR:
# C:\Program Files\Autodesk\Maya2013

# Maya's mental ray directory: (appended to MAYA_DIR)
MENTALRAY_DIR = $(MAYA_DIR)/../mentalrayForMaya2015

# The mentalray dev dir. Appended to MENTALRAY_DIR
MENTALRAY_DEV_DIR = $(MENTALRAY_DIR)/devkit/include

INC = -I$(MENTALRAY_DEV_DIR)
LIB = 
LIB_STATIC = 
CFLAGS = -c -O3 -fPIC -DBIT64 -dynamic -fno-common -std=c99
LIBTOOL = libtool

OBJS = texture_worleynoise.o common.o
SRCS = texture_worleynoise.c common.c
LIBFILE = worleynoise.dylib
MIFILE = worleynoise.mi
ICON_SMALL = render_texture_worleynoise.png
ICON_LARGE = texture_worleynoise.png

all: dylib 

$(OBJS): 
	$(CC) $(CFLAGS) $(INC) $(LIB) $(SRCS) $(LIB_STATIC)

dylib : $(OBJS) 
	$(LIBTOOL) -flat_namespace -undefined suppress -dynamic -o $(LIBFILE)  $(OBJS)

clean: 
	rm -f $(OBJS) 
	rm -f $(LIBFILE)

install:	
	cp $(LIBFILE) $(MENTALRAY_DIR)/shaders
	cp $(MIFILE) $(MENTALRAY_DIR)/shaders/include
	cp $(ICON_SMALL) $(MENTALRAY_DIR)/icons
	cp $(ICON_LARGE) $(MENTALRAY_DIR)/icons

uninstall :	
	rm $(MENTALRAY_DIR)/shaders/$(LIBFILE)
	rm $(MENTALRAY_DIR)/shaders/include/$(MIFILE)
	rm $(MENTALRAY_DIR)/icons/$(ICON_SMALL)
	rm $(MENTALRAY_DIR)/icons/$(ICON_LARGE)
