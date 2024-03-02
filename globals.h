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


//globals.h

//v1.01


#ifndef globals_file_h
#define globals_file_h

#include "audio_formats.h"
#include "filter_code.h"



#define cn_3d_sources_max 8


using namespace std;


struct st_imp_resp_tag
{
float azim;								//sperical
float elev;
float radius;

float xx;								//cartesian
float yy;
float zz;
};



struct st_3D_src_tag
{
string hrir_fname;
string aud_fname;

audio_formats af0;

bool solo;
bool mute;
bool audible;															//this is depedent on solo and mute states, it is used by audio proc

float gain;
bool invert_ch1;

float azim;
float elev;
float radius;

float azim_min;
float azim_max;

float elev_min;
float elev_max;

//vector<st_imp_resp_tag> vimp_resp;

int aud_ptr;

bool fir00_updated;														//set to zero to indicate: filter needs to be created and loaded with an impulse respose
bool fir10_updated;

int fir_state00;														//0: fir00/fir01 is inactive and can be modified by 'main()' thread
																		//1: fir00/fir01 is loaded and waiting to be flagged as active by audio proc thread
																		//2: fir00/fir01 is active in audio proc thread
																		//3: fir00/fir01 is flagged to be deactivated by audio proc thread


int fir_state10;														//10: fir10/fir10 is inactive and can be modified by 'main()' thread
																		//11: fir10/fir10 is loaded and waiting to be flagged as active by audio proc thread
																		//12: fir10/fir10 is active in audio proc thread
																		//13: fir10/fir10 is flagged to be deactivated by audio proc thread



filter_code::st_fir fir00, fir01;										//these allow filter switch over in audio proc, when a new hrir impulse resp. is avail
filter_code::st_fir fir10, fir11;

};



#endif
