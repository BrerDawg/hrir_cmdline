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

//hrir_cmdline.cpp

//v1.01		02-mar-2024			//


//refer: https://github.com/sofacoustics/API_Cpp
//refer: https://github.com/sofacoustics/API_Cpp/blob/master/libsofa/src/sofainfo.cpp		//from https://github.com/sofacoustics/API_Cpp: useful code which builds a 'sofainfo' app that dumps sofa file details, the code shows the use of libsofa functions,  will 'assert' on some sofa files though, could just be versioning issues

//refer: https://github.com/hoene/libmysofa								//not used for this prj
//refer: https://www.sofaconventions.org/mediawiki/index.php/Files

#include "hrir_cmdline.h"

vector<double> vcoeff0, vcoeff1;





bool gverb = 0;
int exit_code;

float srate_wav = 48000;
float srate = srate_wav;

#ifdef use_mysofa														//no used
//struct MYSOFA_EASY *hrtf = NULL;
#endif

bool brecord_allow = 1;													//for user record
bool brecord = 0;
audio_formats af0;
st_audio_formats_tag saf0;
string sdemo_record = "zzrecord.wav";
int record_cnt = 0;

rtaud rta;																//rtaudio
st_rtaud_arg_tag st_rta_arg;											//this is used in audio proc callback to work out chan in/out counts
int framecnt;
bool audio_started = 0;
float time_per_sample;
int audio_source = 0;

float aud_gain_master = 0.5f;
float aud_gain_hrft = 1.0f;													
bool invert_ch1 = 0;

st_3D_src_tag st3d[cn_3d_sources_max];
int audcnt = 0;															//number of audio files specified on cmdline
int cur_aud = 0;														//current audio src which user can manipulate 

//filter_code::st_fir fir00, fir01;										//these allow filter switch over in audio proc, when a new hrir impulse resp. is avail
//filter_code::st_fir fir10, fir11;

//int fir_state00 = 0;													//0: fir00/fir01 is inactive and can be modified by 'main()' thread
																		//1: fir00/fir01 is loaded and waiting to be flagged as active by audio proc thread
																		//2: fir00/fir01 is active in audio proc thread
																		//3: fir00/fir01 is flagged to be deactivated by audio proc thread


//int fir_state10 = 0;													//10: fir10/fir10 is inactive and can be modified by 'main()' thread
																		//11: fir10/fir10 is loaded and waiting to be flagged as active by audio proc thread
																		//12: fir10/fir10 is active in audio proc thread
																		//13: fir10/fir10 is flagged to be deactivated by audio proc thread




string swav_out_fname = "zzzout.wav";

string swav_in_fname = "./crinkle_plastic.wav";

string ssofa_fname = "SimpleFreeFieldHRIR_1.0.sofa";
string slayout_fname = "zzlayout";
string slayout_new_fname = "zzlayout_new.txt";
int layout_idx = 0;


bool bpause_on_err = 1;


vector<st_imp_resp_tag> vsrc_pos;										//holds the azim, elev and radius values for each entry found in .sofa file






//function prototypes
int find_imp_resp_idx_from_spherical( sofa::File *theFile, float azim, float elev );
bool get_imp_resp_samples_at_idx( sofa::File *theFile, sofa::SimpleFreeFieldHRIR *hrir, unsigned int idx, vector<double> &v0, vector<double> &v1  );









void do_error_exit( int exit_cde )
{
if( bpause_on_err )
	{
	printf( "press a key to exit.\n"  );
	getchar();
	}

exit( exit_cde );
}









bool start_audio()
{
printf("start_audio()\n" );


rta.verbose = 1;

//rtaudio_probe();									//dump a list of devices to console


RtAudio::StreamOptions options;

options.streamName = cnsAppName;
options.numberOfBuffers = 2;						//for jack (at least) this is hard set by jack's settings and can't be changed via this parameter

options.flags = 0; 	//0 means interleaved, use oring options, refer 'RtAudioStreamFlags': RTAUDIO_NONINTERLEAVED, RTAUDIO_MINIMIZE_LATENCY, RTAUDIO_HOG_DEVICE, RTAUDIO_SCHEDULE_REALTIME, RTAUDIO_ALSA_USE_DEFAULT, RTAUDIO_JACK_DONT_CONNECT
					// !!! when using RTAUDIO_JACK_DONT_CONNECT, you can create any number of channels, you can't do this if auto connecting as 'openStream()' will fail if there is not enough channel mating ports  

options.priority = 0;								//only used with flag 'RTAUDIO_SCHEDULE_REALTIME'



uint16_t device_num_out = 0;						//use 0 for default device to be used
int channels_out = 2;								//if auto connecting (not RTAUDIO_JACK_DONT_CONNECT), there must be enough mathing ports or 'openStream()' will fail
int first_chan_out = 0;

uint16_t frames = 2048;
unsigned int audio_format = RTAUDIO_FLOAT32;		//see rtaudio docs 'RtAudioFormat' for supported format types, adj audio proc code to suit

//st_osc_params.freq0 = 200;							//set up some audio proc callback user params
//st_osc_params.gain0 = 0.1;

//st_osc_params.freq1 = 600;
//st_osc_params.gain1 = 0.1;
//st_rta_arg.usr_ptr = (void*)&st_osc_params;



if( !rta.start_stream_out( device_num_out, channels_out, first_chan_out, srate, frames, audio_format, &options, &st_rta_arg, (void*)cb_audio_proc_rtaudio ) )		//output only
//if( !rta.start_stream_out( device_num_out, channels_out, first_chan_out, srate, frames, audio_format, &options, &st_rta_arg, (void*)cb_audio_proc_rtaudio_migrate ) )		//output only
	{
	printf("start_audio() - failed to open audio device!!!!\n" );
	return 0;	
	}
else{
	printf("start_audio() - audio out device %d opened, options.flags 0x%02x, srate is: %d, framecnt: %d\n", device_num_out, options.flags, rta.get_srate(), rta.get_framecnt()  );							 //output only	
	}

srate = rta.get_srate();
framecnt = rta.get_framecnt();

printf("start_audio() - srate %f, framecnt: %d\n", srate, framecnt );


time_per_sample = 1.0/srate;

audio_started = 1;
return 1;
}












void stop_audio()
{
printf( "stop_audio()\n" );
audio_started = 0;

mystr m1;
m1.delay_ms( (float)framecnt / srate * 1e3 );												//wait one audio proc period


rta.stop_stream();

}













//refer: https://cboard.cprogramming.com/c-programming/177987-there-inkey-c.html
int kbhit(void)
{
struct termios oldt, newt;
int ch;
int oldf;

tcgetattr(STDIN_FILENO, &oldt);
newt = oldt;
newt.c_lflag &= ~(ICANON | ECHO);
tcsetattr(STDIN_FILENO, TCSANOW, &newt);
oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

ch = getchar();

tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
fcntl(STDIN_FILENO, F_SETFL, oldf);
  
if(ch != EOF)
  {
  ungetc(ch, stdin);
  return 1;
  }
  
return 0;
}













void show_usage()
{
printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
printf("Head Related Impulse Response v1.01 (HRIR Binaural 3D for Headphones)\n");
printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

printf("Need to specify 2 params minimum, eg:\n");
printf("./hrir_cmdline sofa=SimpleFreeFieldHRIR_1.0.sofa aud=src_fname                 //'sofa_fname' is file that has impulse responses\n" );
printf("./hrir_cmdline sofa=SimpleFreeFieldHRIR_1.0.sofa aud=water.wav aud=crinkle.wav //multiple audio files will work, srate must match sofa srate ('.wav' '.au' only) \n" ); 
printf("./hrir_cmdline sofa=SimpleFreeFieldHRIR_1.0.sofa aud=audio.au -v               //verbose on, shows srate of sofa file\n" ); 
printf("\n");
printf("Note: Set system volume to SAFE level, then adj master gain, finally adj hrir gain for each audio file\n\n");
printf("Note: use '5' or '6' to select an audio file when you have specified multiple\n");

printf("use '0' to  sel white noise\n" );
printf("use '1' to  sel play original audio file in a loop (without filtering)\n" );
printf("use '2' to  sel play audio file(s) in a loop (with hrir filtering)\n" );
printf("use '5' '6' to  change which audio file is cur. sel\n" );
printf("use '9' to  sel play 2 tones (lower freq is on left channel)\n" );
printf("use 'a' 's' to  change azimuth\n" );
printf("use 'w' 'z' to  change elevation\n" );
//printf("use 'r' 't' to  change radius(does nothing)\n" );
printf("use 'c' 'C' to  zero to front(center, zero azim/elev)\n" );
printf("use 'f' 'F' to  flip between front(center) and cur azim/elev\n" );
printf("use 'l' 'L' to  load a layout with gain/azim/elev/radius etc from file 'zzlayoutxx.txt'\n" );
printf("use ':'     to  save a layout with gain/azim/elev/radius etc to file 'zzlayout_new.txt'\n" );
printf("use '[' ']' to  alter master gain (use { } also )\n" );
printf("use ',' '.' to  alter hrir filter gain (use < > also )\n" );
printf("use 'm'     to  solo the audio file\n" );
printf("use 'M'     to  mute audio file\n" );
printf("use 'r'     to  auto rotate azimuth\n" );
printf("use 'R'     to  auto cycle between all avail elevations\n" );
printf("use 'i' 'I' to  invert phase of ch1 (right chan)\n" );
printf("use '|'     to  toggle record, recording saved on exit, as 'zzrecord.wav'\n" );
printf("use 'v' 'V' to  toggle verbose\n" );
printf("use 'h' 'H' to  show this\n" );
printf("use 'b' 'B' to  bye/exit\n" );
printf("==================================================================\n");
}









#ifdef use_mysof

/*
bool load_hrir_fir( int which, int filter_length, float azim, float elev, float radius )
{
if( which < 0 ) 
	{
	printf("load_hrir_fir() - which was < 0 \n");
	return 0;
	}

if( which > 1 ) 
	{
	printf("load_hrir_fir() - which was > 1 \n");
	return 0;
	}

if( filter_length <= 0 ) 
	{
	printf("load_hrir_fir() - filter_length was <= 0\n");
	return 0;
	}

float spherical[3];



//refer: https://github.com/hoene/libmysofa
//with phi (azimuth in degree, measured counterclockwise from the X axis), theta (elevation in degree, measured up from the X-Y plane), and r (distance between listener and source) as parameters in the float array and x,y,z as response in the same array.


spherical[0] = azim;					//azimuth   (here I assume y axis goes up/down, so rotation about it should be in rotation in horizontal plane )
spherical[1] = elev;					//elevation
spherical[2] = radius;					//distance of source


mysofa_s2c(spherical);

float xx = spherical[0];
float yy = spherical[1];
float zz = spherical[2];


printf( "load_hrir_fir() - azimuth %f  elev %f  radius %f\n", azim, elev, radius );
printf( "load_hrir_fir() - xx %f  yy %f  zz  %f\n", xx, yy, zz );



float *leftIR = new float[filter_length]; 								// [-1. till 1]
float *rightIR = new float[filter_length];
float leftDelay;          												// unit is sec.
float rightDelay;	

mysofa_getfilter_float( hrtf, xx, yy, zz, leftIR, rightIR, &leftDelay, &rightDelay);
//mysofa_getfilter_float_nointerp( hrtf, xx, yy, zz, leftIR, rightIR, &leftDelay, &rightDelay);
printf( "load_hrir_fir() - leftDelay %f  rightDelay %f\n", leftDelay, rightDelay );





vcoeff0.clear();
vcoeff1.clear();

for( int i = 0; i < filter_length; i++ )								//get impulse resp.
	{
	vcoeff0.push_back( leftIR[i] );
	vcoeff1.push_back( rightIR[i] );
	}

delete[] leftIR;
delete[] rightIR;



if( 0 )
	{
	filter_code::filter_fir_windowed( filter_code::fwt_hann, filter_code::fpt_lowpass, 500, 2400, 2400, srate, vcoeff0 );

	vcoeff1 = vcoeff0;
	}




if( which == 0 )
	{
	
	filter_code::delete_filter( fir00 );
	filter_code::delete_filter( fir01 );
	
	if( filter_code::create_filter_from_coeffs( fir00, vcoeff0 ) == 0 )
		{
		printf( "failed in call: 'create_filter_from_coeffs()' see 'fir00'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}

	if( filter_code::create_filter_from_coeffs( fir01, vcoeff1 ) == 0 )
		{
		printf( "failed in call: 'create_filter_from_coeffs()' see 'fir01'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}
	}


if( which == 1 )
	{
	filter_code::delete_filter( fir10 );
	filter_code::delete_filter( fir11 );

	if( filter_code::create_filter_from_coeffs( fir10, vcoeff0 ) == 0 )
		{
		printf( "failed in call: 'create_filter_from_coeffs()' see 'fir10'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}

	if( filter_code::create_filter_from_coeffs( fir11, vcoeff1 ) == 0 )
		{
		printf( "failed in call: 'create_filter_from_coeffs()' see 'fir11'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}
	}



return 1;
}
*/
#endif









//create fir using impulse response at spec idx
bool lib_sofa_load_hrir_fir( sofa::File *theFile, sofa::SimpleFreeFieldHRIR *hrir, int struct_idx, int which, float azim, float elev, float radius, bool force_internal_filter_ir_test, int &idx_used )
{
bool vb = gverb;

if( which < 0 ) 
	{
	printf("lib_sofa_load_hrir_fir() - which was < 0 \n");
	return 0;
	}

if( which > 1 ) 
	{
	printf("lib_sofa_load_hrir_fir() - which was > 1 \n");
	return 0;
	}


if(vb)printf("lib_sofa_load_hrir_fir() - struct_idx %d  which %d  azim %f   elev %f\n", struct_idx, which, azim, elev );

//vector<double> v0, v1;

idx_used = find_imp_resp_idx_from_spherical( theFile, azim, elev );

if( idx_used < 0 )
	{
	printf( "did not find a close match to azim %.4f  elev %.4f\n", azim, elev );	
	}
else{
	if(vb)printf( "found a close match to azim %.4f   elev %.4f   at index  %d\n", azim, elev, idx_used );	

	if( get_imp_resp_samples_at_idx( theFile, hrir, idx_used, vcoeff0, vcoeff1 ) == 0 )
		{
		printf( "for index %d, failed to find a data for position azim %.4f  elev %.4f\n", idx_used, azim, elev );
		}
	else{
		if(vb)printf( "for index %d, at azim %.4f  elev %.4f, got imp resp. sizes %d %d\n", idx_used, azim, elev, (int)vcoeff0.size(), (int)vcoeff1.size() );
		}
	}



if( force_internal_filter_ir_test )
	{
	filter_code::filter_fir_windowed( filter_code::fwt_hann, filter_code::fpt_lowpass, 512, 18000, 18000, srate, vcoeff0 );

	vcoeff1 = vcoeff0;
	}


if( which == 0 )
	{
	filter_code::delete_filter( st3d[struct_idx].fir00 );
	filter_code::delete_filter( st3d[struct_idx].fir01 );
	
//	filter_code::delete_filter( fir00 );
//	filter_code::delete_filter( fir01 );
	
	if( filter_code::create_filter_from_coeffs( st3d[struct_idx].fir00, vcoeff0 ) == 0 )
		{
		printf( "lib_sofa_load_hrir_fir() - failed in call: 'create_filter_from_coeffs()' see 'fir00'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}

	if( filter_code::create_filter_from_coeffs( st3d[struct_idx].fir01, vcoeff1 ) == 0 )
		{
		printf( "lib_sofa_load_hrir_fir() - failed in call: 'create_filter_from_coeffs()' see 'fir01'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}
	}



if( which == 1 )
	{
	filter_code::delete_filter( st3d[struct_idx].fir10 );
	filter_code::delete_filter( st3d[struct_idx].fir11 );

	if( filter_code::create_filter_from_coeffs( st3d[struct_idx].fir10, vcoeff0 ) == 0 )
		{
		printf( "lib_sofa_load_hrir_fir() - failed in call: 'create_filter_from_coeffs()' see 'fir10'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}

	if( filter_code::create_filter_from_coeffs( st3d[struct_idx].fir11, vcoeff1 ) == 0 )
		{
		printf( "lib_sofa_load_hrir_fir() - failed in call: 'create_filter_from_coeffs()' see 'fir11'\n"  );
		exit_code = 1;
		do_error_exit( exit_code );
		}
	}


return 1;

}














static inline const std::size_t array3DIndex(const unsigned long i,
                                             const unsigned long j,
                                             const unsigned long k,
                                             const unsigned long dim1,
                                             const unsigned long dim2,
                                             const unsigned long dim3)
{
    return dim2 * dim3 * i + dim3 * j + k;
}







static inline const std::size_t array2DIndex(const unsigned long i,
                                             const unsigned long j,
                                             const unsigned long dim1,
                                             const unsigned long dim2)
{
    return dim2 * i + j;
}






/*

bool get_data_2_receivers( unsigned int idx, const sofa::SimpleFreeFieldHRIR *hrir, vector<float> &vf0, vector<float> &vf1 )
{
bool vb = 1;

if(vb)printf( "get_data_2_receivers()\n" );

vf0.clear();
vf1.clear();

const unsigned int M = (unsigned int) hrir->GetNumMeasurements();
const unsigned int R = (unsigned int) hrir->GetNumReceivers();
const unsigned int N = (unsigned int) hrir->GetNumDataSamples();

if( R < 2 )
	{
	printf( "get_data_2_receivers() - num of receivers is insufficient, only have %d\n", R );
	return 0;
	}


if( idx >= M )
	{
	printf( "get_data_2_receivers() - idx is out of range, limit is %d\n", M );
	return 0;
	}
	
if(vb)printf( "get_data_2_receivers() - M(num measurements): %d  R(num receivers): %d  N(num data smpls): %d\n", M, R, N );




return 1;
}
*/






//returns index to a 'vsrc_pos' entry, that best matches 'azim' abd ''
//returns -1 if none found

int find_imp_resp_idx_from_spherical( sofa::File *theFile, float azim, float elev )
{
bool vb = gverb;
bool vb1 = 0;

if(vb)printf( "find_imp_resp_idx_from_spherical() - need azim %f  elev %f\n", azim, elev );

if( vsrc_pos.size() == 0 ) return -1;

if( vsrc_pos.size() == 1 ) return 0;									//only one entry ?


float delta;
float delta_min = 1e6;													//some big num
int delta_min_idx = 0;

float delta_azim_min = 1e6;												//some big num
int delta_azim_idx = 0;

float delta_elev_min = 1e6;												//some big num
int delta_elev_idx = 0;

//find closest azim idx

//vector<int>vidx0, vidx1;

for( int i = 0; i < vsrc_pos.size(); i++ )
	{
	st_imp_resp_tag oir0 = vsrc_pos[i];
//	st_imp_resp_tag oir1 = vsrc_pos[i+1];

	float delta0 = fabs( azim - oir0.azim );

	if( delta0 < delta_azim_min )										//find all entries are a minimum away from azim
		{
//		delta_azim_min = delta0;
//		delta_azim_idx = i;
//		vidx0.push_back( i );
//if(vb)printf( "get_imp_resp_idx_from_spherical() min delta azim %f\n", delta0 );

if(vb1)printf( "vsrc_pos  azim %f elev %f\n", oir0.azim, oir0.elev );
		}


	float delta1 = fabsf( oir0.elev - elev );							//find all entries are a minimum away from elev
	if( delta1 < delta_elev_min )
		{
//		delta_elev_min = delta1;
//		delta_elev_idx = i;
//		vidx1.push_back( i );
//if(vb)printf( "get_imp_resp_idx_from_spherical() min delta elev %f\n", delta1 );
		}
	
	delta = delta0 + delta1;
	
	if( delta < delta_min )
		{
		delta_min = delta;
		delta_min_idx = i;
		}
	




/*
for( int i = 0; i < (vsrc_pos.size() - 1); i++ )
	{
	st_imp_resp_tag oir0 = vsrc_pos[i];
	st_imp_resp_tag oir1 = vsrc_pos[i+1];
	
	if( ( elev >= oir0.elev ) && ( elev <= oir1.elev ) )				//assume sorted by elev
		{
		if( ( azim >= oir0.azim ) && ( azim <= oir1.azim ) )
			{
			float delta0 = fabsf( oir0.elev - elev );
			float delta1 = fabsf( oir1.elev - elev );
			
			if( delta0 < delta1 )										//work out which entry is closest 
				{
				return i;
				}
			else{
				if( (i+1) >= vsrc_pos.size() ) return i;
				else return i+1;
				}
			}
		}
*/

/*
float delta_azim_min = 1e6;												//some big num
float delta_elev_min = 1e6;												//some big num
int delta_azim_idx;

	if( ( azim >= oir0.azim ) && ( azim <= oir1.azim ) )
		{
		if( ( elev >= oir0.elev ) && ( elev <= oir1.elev ) )			//assume sorted by azim
			{
			float delta0 = fabsf( oir0.azim - azim );
			float delta1 = fabsf( oir1.azim - azim );
			
			if( delta0 < delta1 )										//work out which entry is closest 
				{
				return i;
				}
			else{
				if( (i+1) >= vsrc_pos.size() ) return i;
				else return i+1;
				}
			}
		
		}

*/
	}
/*
for( int i = 0; i < vidx0.size(); i++ )
	{
	for( int j = 0; j < vidx1.size(); j++ )
		{
		if( vidx0[i] == vidx1[j] )										//both azim and elev were at minimum from this entry?
			{
			return vidx0[i];
			}
		}
	}
*/

if(vb)printf( "find_imp_resp_idx_from_spherical() - nearest match found at idx %d, azim %f  elev %f\n", delta_min_idx, vsrc_pos[delta_min_idx].azim,  vsrc_pos[delta_min_idx].elev );

return delta_min_idx;
}













bool get_imp_resp_samples_at_idx( sofa::File *theFile, sofa::SimpleFreeFieldHRIR *hrir, unsigned int idx, vector<double> &v0, vector<double> &v1  )
{
bool vb = gverb;


if(vb)printf( "get_imp_resp_samples_at_idx() - idx %d\n", idx );

v0.clear();
v1.clear();

//if(vb)printf( "get_imp_resp_samples_at_idx() - here 1\n" );


if( idx >= vsrc_pos.size() )
	{
	printf( "get_imp_resp_samples_at_idx() - index %d is out of range, limit is %d\n", idx, (int)vsrc_pos.size() - 1 );
	return 0;
	}

if( hrir == 0 ) 
	{
	printf( "get_imp_resp_samples_at_idx() - hrir pointer is zero\n"  );
	return 0;
	}
 
const unsigned int M = (unsigned int) hrir->GetNumMeasurements();
const unsigned int R = (unsigned int) hrir->GetNumReceivers();
const unsigned int N = (unsigned int) hrir->GetNumDataSamples();

//if(vb)printf( "get_imp_resp_samples_at_idx() - M %d R %d N %d\n", M, R, N );

//if(vb)printf( "get_imp_resp_samples_at_idx() - here 2\n" );

std::vector< double > data;
 
//v0.push_back( 0 );
//v1.push_back( 0 );

//return 1;
hrir->GetDataIR( data );


//if(vb)printf( "get_imp_resp_samples_at_idx() - here 3\n" );

int i = idx;

for( std::size_t j = 0; j < R; j++ )
	{
	for( std::size_t k = 0; k < N; k++ )
		{
		const std::size_t index = array3DIndex( i, j, k, M, R, N );
		
		if( j == 0 )
			{
			v0.push_back( data[ index ] );
			}
			
		if( j == 1 )
			{
			v1.push_back( data[ index ] );
			}
			
//		output << data[ index ] << std::endl;
		}
	}


//if(vb)printf( "get_imp_resp_samples_at_idx() - here 4\n" );

return 1;
}









//refer https://github.com/sofacoustics/API_Cpp   sofainfo.cpp
bool lib_sofa_open( string fname, sofa::File **theFile_in, sofa::SimpleFreeFieldHRIR **hrir_in )
{
bool vb = gverb;


sofa::File *theFile = new sofa::File( fname.c_str() );

*theFile_in = theFile;

const bool isSOFA = theFile->IsValid();

if( isSOFA == 0 )
	{
	*theFile_in = 0;
	*hrir_in = 0;
	printf( "lib_sofa_open() - sofa file not valid, file '%s'\n", ssofa_fname.c_str() );
	return 0;
	}


if(vb)printf( "lib_sofa_open() - sofa reported as valid, file '%s'\n", ssofa_fname.c_str() );
 



sofa::SimpleFreeFieldHRIR *hrir = new sofa::SimpleFreeFieldHRIR( ssofa_fname.c_str() );
*hrir_in = hrir;


const bool isHRIR = hrir->IsValid();

if( isHRIR == 0 )
	{
	*hrir_in = 0;
	printf( "lib_sofa_open() - hrir.IsValid() failed, file '%s'\n", ssofa_fname.c_str() );
	return 0;
	}

if(vb)printf( "lib_sofa_open() - hrir reported as valid, file '%s'\n", ssofa_fname.c_str() );


double sr = 0.0;
bool ok = hrir->GetSamplingRate( sr );

if( ok == 0 )
	{
	printf( "lib_sofa_open() - hrir.GetSamplingRate() failed, file '%s'\n", ssofa_fname.c_str() );
	return 0;
	}

sofa::Units::Type units;
hrir->GetSamplingRateUnits( units );

if(vb)printf( "lib_sofa_open() - hrir.GetSamplingRateUnits():  %.2f %s\n", sr, sofa::Units::GetName( units ).c_str()  );

//int idx = 0;
//vector<float> vf0, vf1;
//get_data_2_receivers( idx, hrir, vf0, vf1 );


const unsigned int M = (unsigned int) hrir->GetNumMeasurements();
const unsigned int R = (unsigned int) hrir->GetNumReceivers();
const unsigned int N = (unsigned int) hrir->GetNumDataSamples();

//if(vb)printf( "hrir->GetSamplingRateUnits():  M(num measurements): %d  R(num receivers): %d  N(num data smpls): %d\n", M, R, N );

std::vector< double > data;
hrir->GetDataIR( data );

if(vb)printf( "lib_sofa_open() - hrir.GetDataIR()  data.size() %d\n", (int)data.size() );


sofa::Coordinates::Type coordinates;

ok = theFile->GetListenerUp( coordinates, units );
if( ok == 0 )
	{
	if(vb)printf( "lib_sofa_open() - call to GetListenerUp() failed, '%s'\n", ssofa_fname.c_str() );
	}

//        SOFA_ASSERT( ok == true );


return isSOFA;

}



//, float &azim_min , float &azim_max, float &elev_min, float &elev_max


//get each impulse response's details
//refer https://github.com/sofacoustics/API_Cpp   sofainfo.cpp PrintSource(..)
bool lib_sofa_get_file_details( sofa::File *theFile, vector<st_imp_resp_tag> &vir, st_3D_src_tag &o3d )
{
bool vb = gverb;
bool vb1 = gverb;

if(vb)printf("lib_sofa_get_file_details()\n" );

vir.clear();

    sofa::Coordinates::Type coordinates;
    sofa::Units::Type units;
    const bool ok = theFile->GetSourcePosition( coordinates, units );
     
    SOFA_ASSERT( ok == true );

if(vb)printf("lib_sofa_get_file_details() - coordinates type is %d, refer to enum: sofa::Coordinates::Type (e.g: kCartesian, kSpherical)\n", (int)coordinates );					//gc

if( coordinates == sofa::Coordinates::kSpherical );
	{
		
	}
   
 //   SOFA_ASSERT( ok == true );
    
	if(vb) printf( "lib_sofa_get_file_details() - SourcePosition:Type %s\n", sofa::Coordinates::GetName( coordinates ).c_str() ); 
	if(vb) printf( "lib_sofa_get_file_details() - SourcePosition:Type %s\n", sofa::Units::GetName( units ).c_str() ); 
//    output << sofa::String::PadWith( "SourcePosition:Type" ) << " = " << sofa::Coordinates::GetName( coordinates ) << std::endl;
//    output << sofa::String::PadWith( "SourcePosition:Units" ) << " = " << sofa::Units::GetName( units ) << std::endl;
    
    std::vector< std::size_t > dims;
    theFile->GetVariableDimensions( dims, "SourcePosition" );
    
if(vb)printf("lib_sofa_get_file_details() - 'SourcePosition' dims.size() %d\n", (int)dims.size() );
    SOFA_ASSERT( dims.size() == 2 );
    
    std::vector< double > pos;
    pos.resize( dims[0] * dims[1] );
    
    theFile->GetSourcePosition( &pos[0], dims[0], dims[1] );
 
if(vb)printf("lib_sofa_get_file_details() - dims[0] size %d  dims[1] size %d\n", dims[0], dims[1] );
if(vb)printf("lib_sofa_get_file_details() - GetSourcePosition() returned pos.size() %d\n", (int)pos.size() );

if(vb)printf("lib_sofa_get_file_details() - SourcePosition=\n" );
   
//    output << sofa::String::PadWith( "SourcePosition" ) << " = " ;

st_imp_resp_tag oir;


o3d.azim_max = -1e6;								//some large neg num
o3d.azim_min = 1e6;									//some large pos num

o3d.elev_max = -1e6;								//some large neg num
o3d.elev_min = 1e6;									//some large pos num

if(vb)printf( "\n" );		//gc
for( std::size_t i = 0; i < dims[0]; i++ )
    {
	if(vb1)printf( "lib_sofa_get_file_details() - Measure idx [%03d] - ", i );		//gc
        for( std::size_t j = 0; j < dims[1]; j++ )
        {
            const std::size_t index = array2DIndex( i, j, dims[0], dims[1] );
            
            if( j == 0 ) oir.azim = pos[ index ];
            if( j == 1 ) oir.elev = pos[ index ];
            if( j == 2 )
				{
				oir.radius = pos[ index ];
				
				if( oir.azim < o3d.azim_min ) o3d.azim_min = oir.azim ;
				if( oir.azim > o3d.azim_max ) o3d.azim_max = oir.azim ;
				
				if( oir.elev < o3d.elev_min ) o3d.elev_min = oir.elev ;
				if( oir.elev > o3d.elev_max ) o3d.elev_max = oir.elev ;

				vir.push_back( oir );
				}
            
//            output << pos[ index ] << " ";		//gc

		string s1;
		mystr m1;

		strpf( s1, "pos[%d]: %.4f ", j, pos[ index ] );
		m1 = s1;
		
		m1.prepadstr( s1, 17, 17 );

		if(vb1)printf( "%s", s1.c_str() );
        }


if(vb1)printf( "\n" );
    }
    
if(vb)printf( "lib_sofa_get_file_details() - azim_min %f   azim_max %f\n", o3d.azim_min, o3d.azim_max );
if(vb)printf( "lib_sofa_get_file_details() - elev_min %f   elev_max %f\n", o3d.elev_min, o3d.elev_max );

//    output << std::endl;
    
return 1;
}








bool save_layout( string fname, int cnt_max )
{
bool vb = gverb;

string s1, st;
mystr m1;

for( int j = 0; j < cnt_max; j++ )
	{
	strpf( s1, "%f %f %f %f %s\n", st3d[j].gain, st3d[j].azim, st3d[j].elev, st3d[j].radius, st3d[j].aud_fname.c_str() );
	st += s1;
	}

m1 = st;
if( !m1.writefile( fname ) ) return 0;

return 1;
}








bool load_layout( string fname, int cnt_max )
{
bool vb = gverb;

string s1;
mystr m1;
vector<string>vstr;


if( !m1.readfile( fname, 100000 ) )
	{
	printf("load_layout() - failed to read file: '%s'\n", fname.c_str() );
	return 0;
	}

m1.LoadVectorStrings( vstr, '\n' );
int cnt = vstr.size();


for( int i = 0; i < cnt; i++ )            //load control
	{
	if( i >= audcnt ) break;
	
	float f0 = 0;
	float f1 = 0;
	float f2 = 0;
	float f3 = 0;
	char sz0[255];
	
	sscanf( vstr[i].c_str(), "%f %f %f %f %s", &f0, &f1, &f2, &f3, sz0 );
	
	if(vb)printf("load_layout() - %d: gain %f  azim %f  elev %f  radius %f\n", i, f0, f1, f2, f3 );
	st3d[i].gain = f0;
	st3d[i].azim = f1;
	st3d[i].elev = f2;
	st3d[i].radius = f3;
//	st3d[i].aud_fname = sz0;
	}


return 1;	
}



void audible_flag_update()
{

bool b_have_solo = 0;
for( int j = 0; j < audcnt; j++ )										//have a solo ?
	{
	if( st3d[j].solo ) 
		{
		b_have_solo = 1;
		break;
		}
	}


for( int j = 0; j < audcnt; j++ )
	{
	if( st3d[j].solo ) 
		{
		st3d[j].audible = 1;
		}
	else{
		if( b_have_solo )
			{
			st3d[j].audible = 0;
			}
		else{
			if( st3d[j].mute ) st3d[j].audible = 0;
			else st3d[j].audible = 1;
			}
		}
	}
}




void solo_set( int idx, bool solo_state )
{


for( int j = 0; j < audcnt; j++ )
	{
	if( j == idx ) st3d[j].solo = solo_state;
	else st3d[j].solo = 0;
	}

audible_flag_update();

}








int main(int argc, char **argv)
{
bool vb = gverb;

string s1, s2, st;
mystr m1;

float azim = 0;
float elev = 0;
float radius = 5;
int letter = 0;

bool auto_rot_azim = 0;
bool auto_rot_elev = 0;

exit_code = 0;

if( argc < 3 )
	{
	show_usage();
	exit_code = 1;
	do_error_exit( exit_code );
	}

int sofa_cnt_in = 0;													//counts of how many specified on command line
int aud_cnt_in = 0;

for( int i = 1; i < argc; i++ )
	{
	if(vb)printf("[%d] '%s'\n", i, argv[i] );
	s1 = argv[i];
	if( s1.find( "sofa=" ) !=string::npos )
		{
		strpf( s2, "%d", sofa_cnt_in );									//insert a num to "sofa=", like this "sofa0="
		s1.insert( 4, s2 );	
		sofa_cnt_in++;
		}

	if( s1.find( "aud=" ) !=string::npos )
		{
		strpf( s2, "%d", aud_cnt_in );										//insert a num to "aud=", like this "aud3="
		s1.insert( 3, s2 );	
		aud_cnt_in++;
		}

	st += s1;
	st += ",";
	printf( "s1 = '%s'\n", s1.c_str() );
	}

char sz0[255];
char sz1[255];



if( ( st.find( "-h" ) != string::npos ) || ( st.find( "--h" ) != string::npos ) )
	{
	show_usage();
	exit_code = 0;	
	exit( exit_code );
	}

if( ( st.find( "-v" ) != string::npos ) || ( st.find( "--v" ) != string::npos ) )
	{
	gverb = 1;
	vb = gverb;
	printf("verbose is on\n" );
	}



//---- gather params off cmd line ----
m1 = st;
string sequ;
float fv;
int iv;
unsigned int ui;

bool param_sofa = 0;
bool param_aud0 = 0;

int sofacnt = 0;															//counts of how many specified on command line

if( m1.ExtractParamVal_with_delimit( "sofa0=", ",", sequ ) )
	{
	param_sofa = 1;
	
	ssofa_fname = sequ;
	if(vb)printf( "sofa0='%s'\n", sequ.c_str() );
	sofacnt++;
	}



//load audio files
for( int i = 0; i < cn_3d_sources_max; i++ )
	{
	strpf( s1, "aud%d=", i );
	if( m1.ExtractParamVal_with_delimit( s1, ",", sequ ) )
		{
		param_aud0 = 1;
		
		st3d[i].aud_fname = sequ;
		if(vb)printf( "aud%d='%s'\n", i, sequ.c_str() );


		st3d[i].af0.verb = 0;                                							//store progress in: string 'log'
		st3d[i].af0.log_to_printf = 0;                       							//show to console as well


		en_audio_formats fmt;
		if( st3d[i].af0.find_file_format( "", st3d[i].aud_fname, fmt ) == 0 )
			{
			printf( "failed to find file format: '%s'\n", st3d[i].aud_fname.c_str() );
			exit_code = 1;
			do_error_exit( exit_code );
			}


		saf0.format = fmt;

		if( st3d[i].af0.load_malloc( "", st3d[i].aud_fname, 0, saf0 ) == 0 )
			{
			printf( "failed to load file: '%s'\n", st3d[i].aud_fname.c_str() );
			exit_code = 1;
			do_error_exit( exit_code );
			}
		else{
			printf( "loaded aud file ok: '%s'\n", st3d[i].aud_fname.c_str() );
			}

		audcnt++;
		}
	else{
		break;
		}
	}

//----

if( sofacnt == 0 )
	{
	printf( "No sofa file was specified\n" );
	exit_code = 1;
	do_error_exit( exit_code );
	}

if( audcnt == 0 )
	{
	printf( "No audio file were specified\n" );
	exit_code = 1;
	do_error_exit( exit_code );
	}

//getchar();

//sscanf( st.c_str(), "%s %s", sz0, sz1 );

//ssofa_fname = sz0;
//swav_in_fname = sz1;

int filter_length;
int err;


//ssofa_fname = "SimpleFreeFieldHRIR_1.0.sofa";
//ssofa_fname = "SimpleFreeFieldHRTF_1.0.sofa";
//ssofa_fname = "MIT_KEMAR_normal_pinna.sofa";							//sounds soft
//ssofa_fname = "MIT_KEMAR_large_pinna.sofa";							//sounds soft

sofa::File *theFile = 0;
sofa::SimpleFreeFieldHRIR *hrir = 0;

if( lib_sofa_open( ssofa_fname, &theFile, &hrir ) != 1 )
	{
	printf( " failed in call sofa_open(), file '%s'\n", ssofa_fname.c_str() );
		
	}

printf( "opened a valid 'SimpleFreeFieldHRIR, file '%s'\n", ssofa_fname.c_str() );

//std::ostream & output = std::cout;
st_3D_src_tag o3d;

if( lib_sofa_get_file_details( theFile, vsrc_pos, o3d ) == 0 )
	{
	}

printf( "vir.size() %d\n", (int)vsrc_pos.size() );



//vector<double> v0, v1;

/*
int idx = get_imp_resp_idx_from_spherical( theFile, azim, elev );

if( idx < 0 )
	{
	printf( "did not find a close match to azim %.4f  elev %.4f\n", azim, elev );	
	}
else{
	printf( "found a close match to azim %.4f   elev %.4f   at index  %d\n", azim, elev, idx );	

	if( get_imp_resp_samples_at_idx( theFile, hrir, idx, vcoeff0, vcoeff1 ) == 0 )
		{
		printf( "for index %d, failed to find a data for position azim %.4f  elev %.4f\n", idx, azim, elev );
		}
	else{
		printf( "for index %d, at azim %.4f  elev %.4f, got imp resp. sizes %d %d\n", idx, azim, elev, (int)vcoeff0.size(), (int)vcoeff1.size() );
		}
	}
*/


//if( theFile ) delete theFile;


//getchar();
//return 0;

//----
#ifdef use_mysofa
/*
hrtf = mysofa_open( ssofa_fname.c_str(), 48000, &filter_length, &err);

if( hrtf == NULL )
	{
	printf( "failed to open file, err %d: '%s'\n", err, ssofa_fname.c_str() );
	exit_code = 1;
	do_error_exit( exit_code );
	}
else{
	printf( "loaded sofa file ok, filter_length %d: '%s'\n", filter_length, ssofa_fname.c_str() );
	}



if( load_hrtf_fir( 0, filter_length, azim, elev, radius ) == 0 )		//load fir00 fir01
	{
	exit_code = 1;
	do_error_exit( exit_code );
	}

if( load_hrtf_fir( 1, filter_length, azim, elev, radius ) == 0 )		//load fir10 fir11
	{
	exit_code = 1;
	do_error_exit( exit_code );
	}
*/
#endif
//----


/*
float *leftIR = new float[filter_length]; 								// [-1. till 1]
float *rightIR = new float[filter_length];
float leftDelay;          												// unit is sec.
float rightDelay;	



//refer: https://github.com/hoene/libmysofa
//with phi (azimuth in degree, measured counterclockwise from the X axis), theta (elevation in degree, measured up from the X-Y plane), and r (distance between listener and source) as parameters in the float array and x,y,z as response in the same array.


float xx = 0.0;
float yy = 0.0;
float zz = 0.0;


float spherical[3];

spherical[0] = azim;					//azimuth   (here I assume y axis goes up/down, so rotation about it should be in rotation in horizontal plane )
spherical[1] = elev;					//elevation
spherical[2] = radius;					//distance of source


mysofa_s2c(spherical);

xx = spherical[0];
yy = spherical[1];
zz = spherical[2];

printf( "------------\n" );
printf( "srcfile: '%s'\n", swav_in_fname.c_str() );

printf( "azimuth %f  elev %f  radius %f\n", azim, elev, radius );
printf( "xx %f  yy %f  zz  %f\n", xx, yy, zz );


fir00.created = 0;
fir01.created = 0;
fir00.bypass = 0;
fir01.bypass = 0;

fir10.created = 0;
fir11.created = 0;
fir10.bypass = 0;
fir11.bypass = 0;


mysofa_getfilter_float( hrtf, xx, yy, zz, leftIR, rightIR, &leftDelay, &rightDelay);

printf( "leftDelay %f  rightDelay %f\n", leftDelay, rightDelay );

printf( "------------\n" );
*/


/*

for( int i = 0; i < filter_length; i++ )								//get impulse resp.
	{
	vcoeff0.push_back( leftIR[i] );
	vcoeff1.push_back( rightIR[i] );
	}


if( 0 )
	{
	filter_code::filter_fir_windowed( filter_code::fwt_hann, filter_code::fpt_lowpass, 500, 2400, 2400, srate, vcoeff0 );

	vcoeff1 = vcoeff0;
	}




if( filter_code::create_filter_from_coeffs( fir00, vcoeff0 ) == 0 )
	{
	printf( "failed in call: 'create_filter_from_coeffs()' see 'fir00'\n"  );
	exit_code = 1;
	return exit_code; 
	}

if( filter_code::create_filter_from_coeffs( fir01, vcoeff1 ) == 0 )
	{
	printf( "failed in call: 'create_filter_from_coeffs()' see 'fir01'\n"  );
	exit_code = 1;
	return exit_code; 
	}


if( filter_code::create_filter_from_coeffs( fir10, vcoeff0 ) == 0 )
	{
	printf( "failed in call: 'create_filter_from_coeffs()' see 'fir10'\n"  );
	exit_code = 1;
	return exit_code; 
	}

if( filter_code::create_filter_from_coeffs( fir11, vcoeff1 ) == 0 )
	{
	printf( "failed in call: 'create_filter_from_coeffs()' see 'fir11'\n"  );
	exit_code = 1;
	return exit_code; 
	}


delete[] leftIR;
delete[] rightIR;
*/



/*

st3d[0].fir_state00 = 0;
st3d[0].fir_state10 = 0;


st3d[0].fir00.verb = gverb;
st3d[0].fir00.suser0 = "fir00";
st3d[0].fir00.suser1 = "in function xxxx";
st3d[0].fir00.user_id0 = 0;
st3d[0].fir00.user_id1 = 0;
st3d[0].fir00.bypass = 0;
st3d[0].fir00.created = 0;


st3d[0].fir01.verb = gverb;
st3d[0].fir01.suser0 = "fir01";
st3d[0].fir01.suser1 = "in function xxxx";
st3d[0].fir01.user_id0 = 0;
st3d[0].fir01.user_id1 = 0;
st3d[0].fir01.bypass = 0;
st3d[0].fir01.created = 0;

st3d[0].fir10.verb = gverb;
st3d[0].fir10.suser0 = "fir10";
st3d[0].fir10.suser1 = "in function xxxx";
st3d[0].fir10.user_id0 = 0;
st3d[0].fir10.user_id1 = 0;
st3d[0].fir10.bypass = 0;
st3d[0].fir10.created = 0;

st3d[0].fir11.verb = gverb;
st3d[0].fir11.suser1 = "in function xxxx";
st3d[0].fir11.suser0 = "fir11";
st3d[0].fir11.user_id0 = 0;
st3d[0].fir11.user_id1 = 0;
st3d[0].fir11.bypass = 0;
st3d[0].fir11.created = 0;
*/


//---
if( 0 )
	{
	filter_code::filter_fir_windowed( filter_code::fwt_hann, filter_code::fpt_lowpass, 512, 2000, 2000, srate, vcoeff0 );

	vcoeff1 = vcoeff0;
	}
//---



int idx_used;
int idx_used_last = -1;
bool b_index_not_changed = 0;


//load stuctures, make filters
for( int i = 0; i < audcnt; i++ )
	{
	if(vb)printf("-----\n");
	//prepare filters with data
	
	st3d[i].mute = 0;
	st3d[i].solo = 0;
	st3d[i].audible = 1;
	st3d[i].gain = 1.0;
	st3d[i].invert_ch1 = 0;
	st3d[i].azim = 0;
	st3d[i].elev = 0;
	st3d[i].radius = 0;
		
	strpf( s1, "aud%d_", i );

	st3d[i].fir00.verb = gverb;
	st3d[i].fir00.suser0 = s1;
	st3d[i].fir00.suser0 += "fir00";
	st3d[i].fir00.suser1 = "in function xxxx";
	st3d[i].fir00.user_id0 = 0;
	st3d[i].fir00.user_id1 = 0;
	st3d[i].fir00.bypass = 0;
	st3d[i].fir00.created = 0;

	st3d[i].fir01.verb = gverb;
	st3d[i].fir01.suser0 = s1;
	st3d[i].fir01.suser0 += "fir01";
	st3d[i].fir01.suser1 = "in function xxxx";
	st3d[i].fir01.user_id0 = 1;
	st3d[i].fir01.user_id1 = 1;
	st3d[i].fir01.bypass = 0;
	st3d[i].fir01.created = 0;

	st3d[i].fir10.verb = gverb;
	st3d[i].fir10.suser0 = s1;
	st3d[i].fir10.suser0 += "fir10";
	st3d[i].fir10.suser1 = "in function xxxx";
	st3d[i].fir10.user_id0 = 10;
	st3d[i].fir10.user_id1 = 10;
	st3d[i].fir10.bypass = 0;
	st3d[i].fir10.created = 0;


	st3d[i].fir11.verb = gverb;
	st3d[i].fir11.suser0 = s1;
	st3d[i].fir11.suser0 += "fir11";
	st3d[i].fir11.suser1 = "in function xxxx";
	st3d[i].fir11.user_id0 = 11;
	st3d[i].fir11.user_id1 = 11;
	st3d[i].fir11.bypass = 0;
	st3d[i].fir11.created = 0;


	if( lib_sofa_load_hrir_fir( theFile, hrir, i, 0, azim, elev, radius, 0, idx_used ) == 0 )		//load fir00 fir01
		{
		exit_code = 1;
		do_error_exit( exit_code );
		}

	if( lib_sofa_load_hrir_fir( theFile, hrir, i, 1, azim, elev, radius, 0, idx_used ) == 0 )		//load fir10 fir11
		{
		exit_code = 1;
		do_error_exit( exit_code );
		}

	if(vb)printf("-----\n");
	st3d[i].aud_ptr = 0;

	st3d[i].fir00_updated = 1;
	st3d[i].fir10_updated = 0;

	st3d[i].fir_state00 = 2;											//put the prepared fir on-air
	st3d[i].fir_state10 = 10;
	}






/*
for( int i = 0; i < af0.sizech0; i++ )									//convolve audio samples and fir impulse resp.
	{
	float fmono = af0.pch0[i];
	fmono += af0.pch1[i];
	
	fmono /= 2.0f;

	filter_code::fir_in( fir0, fmono );
	filter_code::fir_in( fir1, fmono );

	float f0 = 0;
	float f1 = 0;
	if( i >= vcoeff0.size() )
		{
		f0 = filter_code::fir_out( fir0 );
		f1 = filter_code::fir_out( fir1 );
		}
	
	af1.push_ch0( f0*0.3f );
	af1.push_ch1( f1*0.3f );
	}






if( af1.save_malloc( "", swav_out_fname, 32767, saf0 ) == 0 )
	{
	printf( "failed to save file: '%s'\n", swav_out_fname.c_str() );
	exit_code = 1;
	goto do_exit; 
	}
*/


//st3d[0].fir_state00 = 2;												//put prepared fir on-air
//st3d[0].fir_state10 = 10;
audio_source = 2;


start_audio();


int main_cnt0 = 0;

int fir_update = 0;
//st3d[0].fir00_updated = 1;
//st3d[0].fir10_updated = 0;

bool toggle_front = 0;
bool cmd_valid =1;
bool bsolo = 0;
float elev_dir = 1;
while(1)
	{
	cmd_valid = 0;
	if( main_cnt0 == 0 ) cmd_valid = 1;									//set this, so on first run the status of settings is shown

	bool bchanged = 0; 
	if( kbhit() )
		{
		letter = getchar();
		if( ( letter == 'b' ) || ( letter == 'B' ) ) break;


		if( ( letter == '0' ) )
			{
			audio_source = 0;
			cmd_valid = 1;
			}

		if( ( letter == '1' ) )
			{
			audio_source = 1;
			cmd_valid = 1;
			}

		if( ( letter == '2' ) )
			{
			audio_source = 2;
			cmd_valid = 1;
			}

		if( ( letter == '9' ) )
			{
			audio_source = 3;
			cmd_valid = 1;
			}


		if( ( letter == '5' ) )
			{
			cur_aud--;
			if( cur_aud < 0 ) cur_aud = audcnt - 1;
			solo_set( cur_aud, bsolo );
			printf( "\ncurrent src sel is: '%s'\n", st3d[cur_aud].aud_fname.c_str() );
			bchanged = 1;
			cmd_valid = 1;
			}

		if( ( letter == '6' ) )
			{
			cur_aud++;
			if( cur_aud >= audcnt ) cur_aud = 0;
			solo_set( cur_aud, bsolo );
			printf( "\ncurrent src sel is: '%s'\n", st3d[cur_aud].aud_fname.c_str() );
			bchanged = 1;
			cmd_valid = 1;
			}

		if( ( letter == 'l' ) || ( letter == 'L' ) )
			{
			if( letter == 'l' ) layout_idx++;
			if( letter == 'L' ) layout_idx--;

			if( layout_idx > cn_layout_max ) layout_idx = 0;
			if( layout_idx < 0 ) layout_idx = cn_layout_max;

			if( layout_idx == 0 ) strpf( s1, "%s", slayout_new_fname.c_str(), layout_idx );		//idx 0 is the recordable layout
			else strpf( s1, "%s%d.txt", slayout_fname.c_str(), layout_idx - 1 );				//idx 1 = layout0.txt   2 = layout1.txt   etc up to 8 = layout7.txt


			if( load_layout( s1, audcnt ) == 0 )
				{
				printf( "\nfailed to load layout '%s'\n", s1.c_str() );
				}
			else{
				printf( "\nloaded layout '%s'\n", s1.c_str() );
				}
			
			for( int j = 0; j < audcnt; j++ )
				{
				st3d[j].fir00_updated = 0;
				st3d[j].fir10_updated = 0;
				}


			bchanged = 1;
			cmd_valid = 1;
			}



		if( letter == ':' )
			{
			if( save_layout( slayout_new_fname, audcnt ) == 0 )
				{
				printf( "\nfailed to save layout to '%s'\n", slayout_new_fname.c_str() );
				}
			else{
				printf( "\nsaving layout to '%s'\n", slayout_new_fname.c_str() );
				layout_idx = 0;
				}

			cmd_valid = 1;
			}


		if( ( letter == 's' ) || ( letter == 'S' ) )
			{
			toggle_front = 0;
			azim -= 5;
			st3d[cur_aud].azim -= 5;
			bchanged = 1;
			cmd_valid = 1;
			}

		if( ( letter == 'a' ) || ( letter == 'A' ) )
			{
			toggle_front = 0;
			azim += 5;
			st3d[cur_aud].azim += 5;
			bchanged = 1;
			cmd_valid = 1;
			}


		if( ( letter == 'w' ) || ( letter == 'W' ) )
			{
			toggle_front = 0;
			elev += 5;
			st3d[cur_aud].elev += 5;
			bchanged = 1;
			cmd_valid = 1;
			}

		if( ( letter == 'z' ) || ( letter == 'Z' ) )
			{
			toggle_front = 0;
			elev -= 5;
			st3d[cur_aud].elev -= 5;
			bchanged = 1;
			cmd_valid = 1;
			}

/*
		if( ( letter == 'r' ) || ( letter == 'R' ) )
			{
			toggle_front = 0;
			radius -= 5;
			bchanged = 1;
			cmd_valid = 1;
			}

		if( ( letter == 't' ) || ( letter == 'T' ) )
			{
			toggle_front = 0;
			radius += 1;
			bchanged = 1;
			cmd_valid = 1;
			}
*/
		if( ( letter == 'f' ) || ( letter == 'F' ) )
			{
			toggle_front = !toggle_front;
			printf( "\nflip to center is %d (1 means azim and elev flipped to zero)\n", toggle_front );
			bchanged = 1;
			cmd_valid = 1;
			}

		if( ( letter == 'c' ) || ( letter == 'C' ) )
			{
			st3d[cur_aud].azim = 0;
			st3d[cur_aud].elev = 0;
			st3d[cur_aud].radius = 1.4f;												//this is not used coded yet, depends hrir file contents having different radii avail
			toggle_front = 0;
			bchanged = 1;
			cmd_valid = 1;
			}


		if( ( letter == 'r' ) || ( letter == 'R' ) )
			{
			if( letter == 'r' )
				{
				auto_rot_azim = !auto_rot_azim;
				}

			if( letter == 'R' )
				{
				auto_rot_elev = !auto_rot_elev;
				if( auto_rot_elev ) elev_dir = 1;
				}
			cmd_valid = 1;
			}


		if( ( letter == 'i' ) || ( letter == 'I' ) )
			{
			st3d[cur_aud].invert_ch1 = !st3d[cur_aud].invert_ch1;
			printf( "\ninvert_ch1 %d (1 means ch1 (right ch) is phase inverted, relative to ch0)\n", st3d[cur_aud].invert_ch1 );
			cmd_valid = 1;
			}

		if( ( letter == 'm' ) || ( letter == 'M' ) )
			{
			if( letter == 'm' )
				{
				bsolo = !bsolo;
				
				solo_set( cur_aud, bsolo );
/*				
				for( int j = 0; j < audcnt; j++ )						//so solo
					{
					if( bsolo )
						{
						if( j == cur_aud ) st3d[j].mute = 0;
						else st3d[j].mute = 1;
						}
					else{
						st3d[j].mute = 0;								//un-mute all
						}
					printf( "\nsolo %d (1 means an audio src has solo)\n", bsolo );
					}			
*/
				}

			if( letter == 'M' )
				{
				
				st3d[cur_aud].mute = !st3d[cur_aud].mute;
				audible_flag_update();

				printf( "\nmute %d (1 means an audio src is muted)\n", st3d[cur_aud].mute );
				}
			cmd_valid = 1;
			}

		if( ( letter == ',' ) )
			{
//			aud_gain_hrft -= 1.0f;
			st3d[cur_aud].gain -= 0.5f;
			printf( "\n[%d].gain %.2f\n", cur_aud, st3d[cur_aud].gain );
			cmd_valid = 1;
			}

		if( ( letter == '.' ) )
			{
//			aud_gain_hrft += 1.0f;
			st3d[cur_aud].gain += 0.5f;
			printf( "\n[%d].gain %.2f\n", cur_aud, st3d[cur_aud].gain );
			cmd_valid = 1;
			}

		if( ( letter == '<' ) )
			{
//			aud_gain_hrft -= 0.25f;
			st3d[cur_aud].gain -= 0.25f;
			printf( "\n[%d].gain %.2f\n", cur_aud, st3d[cur_aud].gain );
			cmd_valid = 1;
			}

		if( ( letter == '>' ) )
			{
//			aud_gain_hrft += 0.25f;
			st3d[cur_aud].gain += 0.25f;
			printf( "\n[%d].gain %.2f\n", cur_aud, st3d[cur_aud].gain );
			cmd_valid = 1;
			}

		if( ( letter == '[' ) )
			{
			aud_gain_master -= 0.25f;
			printf( "\nmaster gain %.2f\n", aud_gain_master );
			cmd_valid = 1;
			}

		if( ( letter == ']' ) )
			{
			aud_gain_master += 0.25f;
			printf( "\nmaster gain %.2f\n", aud_gain_master );
			cmd_valid = 1;
			}

		if( ( letter == '{' ) )
			{
			aud_gain_master -= 0.1f;
			printf( "\nmaster gain %.2f\n", aud_gain_master );
			cmd_valid = 1;
			}

		if( ( letter == '}' ) )
			{
			aud_gain_master += 0.1f;
			printf( "\nmaster gain %.2f\n", aud_gain_master );
			cmd_valid = 1;
			}

		if( ( letter == 'h' ) || ( letter == 'H' ) || ( letter == '?' ) )
			{
			printf( "\n" );
			show_usage();
			cmd_valid = 1;
			}

		if( ( letter == 'v' ) || ( letter == 'V' ) )
			{
			gverb = !gverb;
			vb = gverb;
			for( int j = 0; j < audcnt; j++ )
				{
				st3d[j].fir00.verb = gverb;
				st3d[j].fir01.verb = gverb;
				st3d[j].fir10.verb = gverb;
				st3d[j].fir11.verb = gverb;
				}
			
			printf( "\nverbose %d\n", gverb );
			cmd_valid = 1;
			}


		if( letter == '|' )
			{
			if( brecord_allow )
				{
				if( brecord == 0 )
					{
					af0.clear_ch0();
					af0.clear_ch1();
					record_cnt = 0;
					}
				brecord = !brecord;
				if( brecord ) printf( "\nRECORDING\n" );
				}
			cmd_valid = 1;
			}

//		if( azim >=360.0f ) azim -= 360.0f;
//		if( azim < 0.0f ) azim += 360.0f;

//		if( elev > 360.0f ) elev = 360.0f;
//		if( elev < -360.0f ) elev = -360.0;





		if( radius <= 0.0f ) radius = 1;
		}
	else{
		//auto rotation
		if( ( auto_rot_azim ) || ( auto_rot_elev ) )
			{
			if( ( st3d[cur_aud].fir00_updated ) || ( st3d[cur_aud].fir10_updated ) )
				{
				if( auto_rot_azim ) st3d[cur_aud].azim += 5;
				if( auto_rot_elev ) 
					{
					st3d[cur_aud].elev += 5 * elev_dir;
					if( st3d[cur_aud].elev >= o3d.elev_max ) elev_dir= -elev_dir;
					if( st3d[cur_aud].elev <= o3d.elev_min ) elev_dir= -elev_dir;
					}
				
				
				st3d[cur_aud].fir00_updated = 0;
				st3d[cur_aud].fir10_updated = 0;
				fir_update = 1;													//trig fir impulse resp. update sequence
				cmd_valid = 1;
				bchanged = 1;
				}
			else{
				fir_update = 1;											//if here neither fir was ready, force an update
				bchanged = 1;
				}
			}
		m1.delay_us( 200000 );
		}

	for( int j = 0; j < audcnt; j++ )
		{
		if( st3d[j].azim >=360.0f ) st3d[j].azim -= 360.0f;
		if( st3d[j].azim < 0.0f ) st3d[j].azim += 360.0f;

		if( st3d[j].elev > 360.0f ) st3d[j].elev = 360.0f;
		if( st3d[j].elev < -360.0f ) st3d[j].elev = -360.0;
		}


	if( audio_source == 0 ) s1 = "White Noise";
	if( audio_source == 1 ) s1 = "Original";
	if( audio_source == 2 ) s1 = "Hrir Filtered";
	if( audio_source == 3 ) s1 = "Tones";

	s2 = "";
	if( st3d[cur_aud].mute ) s2 = "MUTE";
	if( bsolo ) s2 = "SOLO";

	if( st3d[cur_aud].invert_ch1 ) s2 += "INVERTch1";

	float listen_azim = st3d[cur_aud].azim;
	float listen_elev = st3d[cur_aud].elev;


	if( b_index_not_changed )
		{
		if(vb)printf("\nimpulse response index did not change, azim or elev may not be have an entry in sofa file\n" );
		b_index_not_changed = 0;
		}
	idx_used_last = idx_used;
	
	
//	if( cmd_valid )
//		{
//		if(!gverb)
//			{
//			if( idx_used ) 
//				{
//				printf("\r-> aud[%d] azim %.2f elev %.2f radi %.2f impRspIdx %04d %s  %s   cur: '%s'    \n", cur_aud, listen_azim, listen_elev, radius, idx_used, s1.c_str(), s2.c_str(), st3d[cur_aud].aud_fname.c_str() );
//				}
//			}
//		}



	if( bchanged )
		{
		st3d[cur_aud].fir00_updated = 0;
		st3d[cur_aud].fir10_updated = 0;
		fir_update = 1;													//trig fir impulse resp. update sequence
		}


	main_cnt0++;
	if( !( main_cnt0 % 1 ) )
		{
		
		if( fir_update != 0 )
			{
//			printf("here0 fir_update %d  fir_state %d\n", fir_update, fir_state );



			for( int j = 0; j < audcnt; j++ )
				{
				listen_azim = st3d[j].azim;
				listen_elev = st3d[j].elev;
				
				if( toggle_front )
					{
					listen_azim = 0;
					listen_elev = 0;
					}

				if( !st3d[j].fir10_updated )
					{
	//				printf("here01\n" );
	//				if( ( fir_state == 2 ) || (  fir_state == 10  ) )		//fir00 in use, or fir10 is deactivated ?




					if( ( st3d[j].fir_state00 == 2 ) || (  st3d[j].fir_state10 == 10  ) )	//fir00 in use, or fir10 is deactivated ?
						{
						if(vb)printf("\n" );
	//----
	#ifdef use_mysofa
//						if( load_hrtf_fir( 1, filter_length, listen_azim, listen_elev, radius ) == 0 )	//update alternate fir, load fir10 fir11
//							{
//							exit_code = 1;
//							do_error_exit( exit_code );
//							}
	#endif
	//----

						if( lib_sofa_load_hrir_fir( theFile, hrir, j, 1, listen_azim, listen_elev, radius, 0, idx_used ) == 0 )		//load fir10 fir11
							{
							exit_code = 1;
							do_error_exit( exit_code );
							}

						if( idx_used == idx_used_last ) b_index_not_changed = 1;
						

						st3d[j].fir10_updated = 1;
						st3d[j].fir_state00 = 3;									//flag to deactivate fir00
						}
					}

				if( !st3d[j].fir00_updated )
					{
	//				printf("here05\n" );
					if( ( st3d[j].fir_state10 == 12 ) || (  st3d[j].fir_state00 == 0  ) )		//fir10 in use, or fir00 is deactivated ?
						{
						if(vb)printf("\n" );

	//----
	#ifdef use_mysofa
//						if( load_hrtf_fir( 0, filter_length, listen_azim, listen_elev, radius ) == 0 )	//update alternate fir, load fir00 fir01
//							{
//							exit_code = 1;
//							do_error_exit( exit_code );
//							}
	#endif
	//----


	//	printf("\r -> azim %.2f  elev %.2f  radius %.2f  gain %.2f  fir_state %d  fir00_updated %d  fir10_updated %d   %s:     %c       ", azim, elev, radius, aud_gain, fir_state, fir00_updated, fir10_updated, s1.c_str(), letter );
						if( lib_sofa_load_hrir_fir( theFile, hrir, j, 0, listen_azim, listen_elev, radius, 0, idx_used ) == 0 )		//load fir00 fir01
							{
							exit_code = 1;
							do_error_exit( exit_code );
							}

						if( idx_used == idx_used_last ) b_index_not_changed = 1;

						st3d[j].fir00_updated = 1;
						st3d[j].fir_state10 = 13;										//flag to deactivate f10
	//					printf("here06\n" );
						}
					}


				fir_update++;
				if( fir_update >= 2 ) fir_update = 0;
			
				}
			}
		}



	for( int j = 0; j < audcnt; j++ )
		{
		if( ( st3d[j].fir_state00 == 0 ) || ( st3d[j].fir_state10 == 10 ) )				//fir00 or f10 deactivated ?									
			{
			if( st3d[j].fir00_updated )											//fir00 updated ?
				{
				if( st3d[j].fir_state00 == 0 )
					{
					st3d[j].fir_state00 = 1;					//flag to activate fir00
					if(gverb)printf("\nactivating %s  (fir10 is %d)\n", st3d[j].fir00.suser0.c_str(), st3d[j].fir_state10 );
					}
				}
			else{
				if( st3d[j].fir10_updated )										//fir10 updated ?
					{
					if( st3d[j].fir_state10 == 10 ) 
						{
						st3d[j].fir_state10 = 11;			//flag to activate fir10
						if(gverb)printf("\nactivating %s (fir00 is %d)\n", st3d[j].fir10.suser0.c_str(), st3d[j].fir_state00 );
						}
					}
				else{													//if here none are updated
					}
				
				}
			}
		}
	
	if( cmd_valid )
		{
		float listen_azim = st3d[cur_aud].azim;
		float listen_elev = st3d[cur_aud].elev;
		if(gverb)
			{
			printf("-> azim %.2f elev %.2f impRspIdx %04d fir_stat00 %d fir_stat00 %d  fir00_updat %d  fir10_updat %d   %s  %s aud[%d]: %s:      \n", listen_azim, listen_elev, idx_used, st3d[cur_aud].fir_state00, st3d[cur_aud].fir_state10, st3d[cur_aud].fir00_updated, st3d[cur_aud].fir10_updated, s1.c_str(), s2.c_str(), cur_aud, st3d[cur_aud].aud_fname.c_str() );
			}
		else{
//			if( idx_used ) 
				{
				printf("-> azim %.2f elev %.2f impRspIdx %04d %s  %s   aud[%d]: '%s'    \n",listen_azim, listen_elev, idx_used, s1.c_str(), s2.c_str(), cur_aud, st3d[cur_aud].aud_fname.c_str() );
				}
			}
		}

	m1.delay_us( 1 );
	}


stop_audio();

m1.delay_ms( framecnt*time_per_sample );




//----
#ifdef use_mysofa
//mysofa_close( hrtf );
#endif
//----

for( int j = 0; j < audcnt; j++ )
	{
	filter_code::delete_filter( st3d[j].fir00 );
	filter_code::delete_filter( st3d[j].fir01 );
	filter_code::delete_filter( st3d[j].fir10 );
	filter_code::delete_filter( st3d[j].fir11 );
	}


if( brecord_allow )
	{
	if( af0.sizech0 == 0 )
		{
		printf( "nothing to record in: '%s'\n", s1.c_str() );
		}
	else{
		saf0.srate = srate;
		saf0.format = en_af_wav_pcm;
		saf0.encoding = 0;
		saf0.channels = 2;
		saf0.offset = 0;
		saf0.is_big_endian = 0;
		saf0.bits_per_sample = 16;
		
		string sdemo_record = "zzrecord.wav";
		
		af0.normalise( 1.0, saf0 );
		
		if( af0.save_malloc( "", sdemo_record, 32767, saf0 ) == 0 )
			{
			printf( "failed to save record file: '%s'\n", s1.c_str() );
			}
		else{
			printf( "record saved in file: '%s'\n", s1.c_str() );
			}
		}
	}


return exit_code;
}
