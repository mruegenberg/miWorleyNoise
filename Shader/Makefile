CC = gcc
CPP = g++

#########################
# User Settings:

# Uncomment your Maya version here
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

# End of user settings.
#########################

INC = -I$(MENTALRAY_DEV_DIR)
LIB = 
LIB_STATIC = 
CFLAGS = -c -O3 -fPIC -DBIT64 -dynamic -fno-common -std=c99
LIBTOOL = libtool

OBJS = texture_worleynoise.o texture_worleynoise3d.o common.o
SRCS = texture_worleynoise.c texture_worleynoise3d.c common.c
LIBFILE = worleynoise.dylib
MIFILES = worleynoise*.mi
ICONS_SMALL = render_texture_worleynoise*.png
ICONS_LARGE = texture_worleynoise*.png

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
	cp $(MIFILES) $(MENTALRAY_DIR)/shaders/include
	cp $(ICONS_SMALL) $(MENTALRAY_DIR)/icons
	cp $(ICONS_LARGE) $(MENTALRAY_DIR)/icons

uninstall :	
	rm $(MENTALRAY_DIR)/shaders/$(LIBFILE)
	rm $(MENTALRAY_DIR)/shaders/include/$(MIFILES)
	rm $(MENTALRAY_DIR)/icons/$(ICONS_SMALL)
	rm $(MENTALRAY_DIR)/icons/$(ICONS_LARGE)
