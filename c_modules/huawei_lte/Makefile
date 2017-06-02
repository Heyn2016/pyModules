############################################################################
# 		Python modules
#
# Python:   3.5.2
# Platform: ARMv7 (i.mx6)
# Author:   Heyn (heyunhuan@gmail.com)
# History:  2017/02/09
# GCC:		gcc version 5.2.1 20151005 (Linaro GCC 5.2-2015.11-2)
############################################################################


TARGET  := me909s.cpython-35m-arm-linux-gnueabihf.so


CC      := arm-linux-gnueabihf-gcc
LIBS    := 
LDFLAGS :=
DEFINES :=
INCLUDE := -I ./ -I /home/he/share/Python-3.5.2/Include -I /home/he/share/Python-3.5.2
CFLAGS  := -fPIC -Wall -O3 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H
SHARE   := -shared -o

  
#source file
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
  
.PHONY : everything objs clean veryclean rebuild
  
everything : $(TARGET)
  
all : $(TARGET)
  
objs : $(OBJS)
  
rebuild: veryclean everything
                
clean :
	rm -fr *.o *.so
    
veryclean : clean
	rm -fr $(TARGET)
  
$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) $(SHARE) $@ $(OBJS) $(LDFLAGS) $(LIBS)
