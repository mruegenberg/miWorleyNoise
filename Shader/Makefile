CC = gcc
CPP = g++

#MAYA_DIR = /Applications/Autodesk/maya2011
#MAYA_DIR = /Applications/Autodesk/maya2012
MAYA_DIR = /Applications/Autodesk/maya2013


# Maya 2012's mental ray directory:
#MENTALRAY_DEV_DIR = /devkit/mentalray/include

# Maya 2013's mental ray directory: (appended to MAYA_DIR)
MENTALRAY_DEV_DIR = /mentalray/devkit/include

INC = -I$(MAYA_DIR)$(MENTALRAY_DEV_DIR)
LIB = 
LIB_STATIC = 
CFLAGS = -c -O3 -fPIC -DBIT64 -dynamic -fno-common -std=c99
LIBTOOL = libtool

OBJS = texture_worleynoise.o
SRCS = texture_worleynoise.c
LIBFILE = worleynoise.dylib
MIFILE = worleynoise.mi

all: dylib 

$(OBJS): 
	$(CC) $(CFLAGS) $(INC) $(LIB) $(SRCS) $(LIB_STATIC)

dylib : $(OBJS) 
	$(LIBTOOL) -flat_namespace -undefined suppress -dynamic -o $(LIBFILE)  $(OBJS)

clean: 
	rm -f $(OBJS) 
	rm -f $(LIBFILE)

install:	
	cp $(LIBFILE) $(MAYA_DIR)/mentalray/shaders
	cp $(MIFILE) $(MAYA_DIR)/mentalray/shaders/include