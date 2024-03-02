# A Makefile for both Linux and Windows, 30-dec-2015


#define all executables here
app_name= hrir_cmdline


all: ${app_name}

#define compiler options	
CC=g++

ifneq ($(OS),Windows_NT)			#linux?
#	CFLAGS=-g -O0 -fpermissive -fno-inline -Dbuild_date="\"`date +%Y\ %b\ %d`\"" #-Dbuild_date="\"2016 Mar 23\""
#	LIBS=-L/usr/X11/lib -L/usr/local/lib -lfltk_images /usr/local/lib/libfltk.a -lfltk_png -lfltk_z -ljpeg -lrt -lm -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lpthread -ldl -lX11 -lfftw3 #-ljack
	INCLUDE= -I/usr/local/include -I/home/gc/API_Cpp-master/libsofa/dependencies/include/ -I/home/gc/API_Cpp-master/libsofa/src/
	CFLAGS=-g -O3  -Wfatal-errors -Wfatal-errors -fpermissive -fno-inline -Dbuild_date="\"`date +%Y-%b-%d`\"" #-Dbuild_date="\"2016-Mar-23\""			#64 bit
	LIBS= -L/home/gc/API_Cpp-master/libsofa/dependencies/lib/linux/ -L/home/gc/API_Cpp-master/libsofa/build/ -lrt -lasound `pkg-config --libs rtaudio` -lsofa -lnetcdf_c++4 -lnetcdf -lhdf5_hl -lhdf5 -lz -lcurl -ldl -lm     				#64 bit
else								#windows?
#	CFLAGS=-g -O0 -fno-inline -DWIN32 -mms-bitfields -Dcompile_for_windows -Dbuild_date="\"`date +%Y\ %b\ %d`\""
#	LIBS= -L/usr/local/lib -static -lm -lasound `pkg-config --libs rtaudio`
#	INCLUDE= -I/usr/local/include
endif


#define object files for each executable, see dependancy list at bottom
obj1= hrir_cmdline.o GCProfile.o audio_formats.o filter_code.o gc_rtaudio.o rt_code.o     #gc_srateconv.o
#obj2= remez.o
#obj3= remez1.o remez1.tab.o


#linker definition
hrir_cmdline: $(obj1)
	$(CC) $(CFLAGS) -o $@ $(obj1) $(LIBS)


#linker definition
#remez: $(obj2)
#	$(CC) $(CFLAGS) -o $@ $(obj2) $(LIBS)


#linker definition
#remez1: $(obj3)
#	$(CC) $(CFLAGS) -o $@ $(obj3) $(LIBS)


#compile definition for all cpp files to be complied into .o files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.cxx
	$(CC) $(CFLAGS) $(INCLUDE) -c $<


#dependancy list per each .o file
hrir_cmdline.o: hrir_cmdline.h globals.h GCProfile.h audio_formats.h filter_code.h gc_rtaudio.h rt_code.h mysofa.h             #gc_srateconv.h
GCProfile.o: GCProfile.h
audio_formats.o: audio_formats.h GCProfile.h
#gc_srateconv.o: gc_srateconv.h GCProfile.h
filter_code.o: filter_code.h
#mgraph.o:  mgraph.h GCProfile.h

gc_rtaudio.o: gc_rtaudio.h
rt_code.o: rt_code.h globals.h




.PHONY : clean
clean : 
		-rm hrir_cmdline $(obj1)
#		-rm remez.exe $(obj2)


