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



//rt_code.cpp
//v1.01		26-feb-2024			//



#include "rt_code.h"



#define twopi (2.0f*(float)M_PI)



mystr tim1;

extern bool brecord;
extern int record_cnt;
extern audio_formats af0;

extern rtaud rta;
extern st_rtaud_arg_tag st_rta_arg;

extern float srate;
extern int framecnt;
extern int audio_source;
extern audio_formats af0;
extern int fir_state00;
extern int fir_state10;
//extern filter_code::st_fir fir00, fir01;
extern filter_code::st_fir fir10, fir11;
extern st_3D_src_tag st3d[cn_3d_sources_max];

extern float aud_gain_master;
extern float aud_gain_hrft;													
extern bool invert_ch1;
extern int audcnt;
extern int cur_aud;

int proc_cnt = 0;
float freq0 =  framecnt * 1.0;

float osc_freq0 = 200.0f;
float osc_freq1 = 1000.0f;
float theta0 = 0;
float theta1 = 0;




//generates a rnd num between -1.0 -> 1.0
double rnd()
{
double drnd =  (double)( RAND_MAX / 2 - rand() ) / (double)( RAND_MAX / 2 );

return drnd;
}


float rndf()
{
float ff =  (float)( RAND_MAX / 2 - rand() ) / (float)( RAND_MAX / 2 );

return ff;
}



float srconv_bf0[65536];
float srconv_head_bf[2048];


int rt_cnt = 0;
//-------------------- realtime audio proc -----------------------------
int cb_audio_proc_rtaudio( void *bf_out, void *bf_in, int frames, double streamTime, RtAudioStreamStatus status, void *arg_in )
{
bool vb = 0;

double dt = tim1.time_passed( tim1.ns_tim_start );
tim1.time_start( tim1.ns_tim_start );

if( !(proc_cnt % 40) )
	{
	if(vb) printf( "cb_audio_proc_rtaudio() - dt: %f, frames: %d\n", dt, frames );
	}
proc_cnt++;


float *rtabf0 = (float*)bf_out;



int aud_src = audio_source;												//only process one audio source in this 'for' loop for this call, avoids any mid loop change from user's main thread


//---

	for( int j = 0; j < audcnt; j++ )
		{
		if( st3d[j].fir_state00 == 1 )									//perform fir state transitioning
			{
			st3d[j].fir_state00 = 2;	
			}

		if( st3d[j].fir_state00 == 3 )
			{
			st3d[j].fir_state00 = 0;	
			}

		if( st3d[j].fir_state10 == 11 )
			{
			st3d[j].fir_state10 = 12;	
			}

		if( st3d[j].fir_state10 == 13 )
			{
			st3d[j].fir_state10 = 10;	
			}
		}
//---



int j = 0;
for ( int i = 0; i < frames; i++ )										//2 chans per loop
	{

	if( aud_src == 0 )													//whitenoise
		{
		float famp = 0.01f;
		rtabf0[j] = rndf()*famp*aud_gain_master;						//ch0	
		rtabf0[j+1] = rndf()*famp*aud_gain_master;						//ch1
		goto done_aud;
		}





	if( aud_src == 3 )													//tones
		{
		float famp = 0.01f;

		float theta0_inc = osc_freq0 * (twopi / srate);
		float theta1_inc = osc_freq1 * (twopi / srate);

		rtabf0[j] = famp * aud_gain_master * sinf( theta0 );
		rtabf0[j+1] = famp * aud_gain_master * sinf( theta1 );

		theta0 += theta0_inc;
		if( theta0 >= twopi ) theta0 -= twopi;

		theta1 += theta1_inc;
		if( theta1 >= twopi ) theta1 -= twopi;
		goto done_aud;
		}




	if( aud_src == 1 )													//unfiltered audio
		{
		float famp = 0.1f;

		int ptr = st3d[cur_aud].aud_ptr;
		
		if( ptr >= st3d[cur_aud].af0.sizech0 ) st3d[cur_aud].aud_ptr = 0;
		
		float fmono = famp * st3d[cur_aud].af0.pch0[ptr];
		fmono += famp * st3d[cur_aud].af0.pch1[ptr];

		fmono /= 2.0f;


		rtabf0[j] = fmono*aud_gain_master;	
		rtabf0[j+1] = fmono*aud_gain_master;
		goto done_aud;
		}





//---------
	if( aud_src == 2 )													//filtered audio
		{
		float fsum0 = 0;
		float fsum1 = 0;
		for( int k = 0; k < audcnt; k++ )
			{
			int ptr = st3d[k].aud_ptr;
			
			if( ptr >= st3d[k].af0.sizech0 ) 
				{
				ptr = 0;
				st3d[k].aud_ptr = 0;
				}

			rtabf0[j] = fsum0;	
			rtabf0[j+1] = fsum1;

	//---

			if( st3d[k].fir_state00 == 2 )
				{
				float fmono = st3d[k].af0.pch0[ptr];
				fmono += st3d[k].af0.pch1[ptr];
			
				fmono = (fmono * aud_gain_master * aud_gain_hrft * st3d[k].gain) / 2.0f;
				if( !st3d[k].audible ) fmono = 0;

				filter_code::fir_in( st3d[k].fir00, fmono );
				filter_code::fir_in( st3d[k].fir01, fmono );


				float f0 = filter_code::fir_out( st3d[k].fir00 );
				float f1 = filter_code::fir_out( st3d[k].fir01 );

				if( st3d[k].invert_ch1 ) f1 = -f1;

				fsum0 += f0;
				fsum1 += f1;

				rtabf0[j] = fsum0;	
				rtabf0[j+1] = fsum1;
				}
			
	//---



	//---
			
			if( st3d[k].fir_state10 == 12 )
				{
				float fmono = st3d[k].af0.pch0[ptr];
				fmono += st3d[k].af0.pch1[ptr];
			
				fmono = (fmono * aud_gain_master * aud_gain_hrft * st3d[k].gain) / 2.0f;

				if( !st3d[k].audible ) fmono = 0;

				filter_code::fir_in( st3d[k].fir10, fmono );
				filter_code::fir_in( st3d[k].fir11, fmono );

				float f0 = filter_code::fir_out( st3d[k].fir10 );
				float f1 = filter_code::fir_out( st3d[k].fir11 );

				if( st3d[k].invert_ch1 ) f1 = -f1;

				fsum0 += f0;
				fsum1 += f1;
				
				rtabf0[j] = fsum0;	
				rtabf0[j+1] = fsum1;
				}
			}
//---

		if( brecord )
			{
			af0.push_ch0( fsum0 );
			af0.push_ch1( fsum1 );
			}

		goto done_aud;
		}
//---------
		

done_aud:
	j+=2;
	for( int j = 0; j < audcnt; j++ )
		{
		st3d[j].aud_ptr++;
		}
	}

if( brecord )
	{
	if( !( rt_cnt%50 ) )
		{
		if( brecord )
			{
			printf( "record time: %f\n", record_cnt * (1.0f/srate) * frames  );
			}
		}
	record_cnt++;
	}

rt_cnt++;

return 0;
}
