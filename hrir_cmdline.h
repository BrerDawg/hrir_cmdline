/*
Copyright (C) 2024 BrerDawg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


//hrir_cmdline.h

//v1.01


#ifndef hrir_cmdline_h
#define hrir_cmdline_h

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <stdlib.h>
#include <math.h>




//linux code
#ifndef compile_for_windows

//#include <X11/Intrinsic.h>
//#include <X11/StringDefs.h>
//#include <X11/Shell.h>
//#include <X11/Xaw/Form.h>
//#include <X11/Xaw/Command.h>

#define _FILE_OFFSET_BITS 64			//large file handling
//#define _LARGE_FILES
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>		//MakeIniPathFilename(..) needs this
#endif


//windows code
#ifdef compile_for_windows
#include <windows.h>
#include <process.h>
#include <winnls.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <conio.h>

#define WC_ERR_INVALID_CHARS 0x0080		//missing from gcc's winnls.h
#endif


#include "globals.h"
#include "GCProfile.h"
#include "audio_formats.h"
#include "filter_code.h"
#include "gc_rtaudio.h"
#include "rt_code.h"
#include <termios.h>													//for 'kbhit()'
#include <unistd.h>														//for 'kbhit()'
#include <fcntl.h>														//for 'kbhit()'
//---
#ifdef use_mysofa
//	#include "mysofa.h"													//not used
#endif
//---

#include <../src/SOFA.h>
#include <../src/SOFAString.h>

#define cn_layout_max 8

#define cnsAppName "gc_hrtf"



#endif


