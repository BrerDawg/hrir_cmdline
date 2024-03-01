Feb 2024 v1.01

# UNDER DEVELOPMENT

### hrir_cmdline
Linux command line utility that uses Head Related Impulse Response (HRIR) sofa files, allows testing of 3D binaural audio using headphones.

:warning:(This code can drive audio levels to their fullest, so set a safe maximum with your system's volume control before use):warning:

### Further infomation
https://en.wikipedia.org/wiki/Head-related_transfer_function</br>
https://sofacoustics.org/data/examples/</br>		
https://github.com/sofacoustics/API_Cpp</br>
</br>
https://github.com/sofacoustics/API_Cpp/blob/master/libsofa/src/sofainfo.cpp</br>
From https://github.com/sofacoustics/API_Cpp: useful code which builds a 'sofainfo' app that dumps sofa file details, the code shows the use of libsofa functions,  will 'assert' on some sofa files though, could just be versioning issues.</br>


### Purpose
This demo allows you to change audio source positioning in 3D and hear the change in realtime. It's only for headphones, in-ear variety work well.

### Theory
SOFA files contain data for audio processing, some contain HRIR impulse responses which can be used to generate 3D audio. The impulse responses are used to filter user audio in the time domain. They  can be made in a number of ways, one way is with a sharp sound pulse emitter (i.e. impulse) from a device/speaker. The pulsed sound is position somewhere near a dummy head with ears (~1.5M). Each ear has an in-ear pickup microphone which record the impulse. The impulses ends up being gain and phase modulated by the shape of the ear and its pinna and is therefore dependent on where the sound was generated in 3D space relative to the head. The pair of impulse responses capture just one position in 3D space. The sound source is rotated to different angle/positions to build up good coverage. To reduce data, stepped/quantized angle positions (defined as Azimuth and Elevation) are only gathered in the sofa file. Therefore a pair of impulses is associated with one 3D position (set of angles, they could also be defined as an x,y,z vector). These impulse responses can later be used to convolve (fir filter) any mono audio source. The result is that the audio source under goes gain and phase changes which can mimic what your ears hear. Having access to all these impulse responses allows for movement of your audio source in 3D space. 

Everyone's ears are slightly different, so impulse responses work best when tailored to the individual. Using someone else's will cause cruder 3D location perception and is therefore a compromise.



### Prep
You need a suitable HRIR sofa file (https://sofacoustics.org/data/examples/).
Some do not work as 'libsofa' (which this demo uses) can't read them and will cause an 'assert' within the library itself, then an exception with core dump, could be versioning or malformed file issue. 

However, file 'SimpleFreeFieldHRIR_1.0.sofa' works well, its samplerate is 48KHz.
It covers 360 degrees of azimuth (horiz rotation around your ear line), at 0 degs the sound source is forward of you, +90 degs is left, and 270 degs is right.
Also, it covers +80 to -30 degrees of elevation (plus is upward, minus is down).

You also need one or more 16 bit PCM .wav or .au audio files that have the same samplerate as the sofa file. E.g. water pouring into a cup or footsteps on gravel are good choices as they have high frequency content, the files will loop play, best to make sure they are quite long, or it will become annoying. The file can be stereo, they will be mono'ed on the fly as they are being fed into the fir filters. Keep each audio file under 100MB per file as this code has storage limits imposed.



### Code and Shortcomings
Where pieces of code or ideas are originated from other people, the comments in source code will give links to who or where it came from.

Some of the code is re-purposed from other projects I've tinkered with over the years, so excuse the antiquated c style, evolving conventions and inefficiencies.

The audio files are loaded into memory and have a size limit, they need to be under 10 mins in length at 48KHz.

The switching between different angles is not seemless for audio due to simplicity of the code.

The filter switching between 'on-air' to 'prepared for on-air' is also simply coded and likely not thread safe. The filters are prepared in the main thread, 
then flagged so the realtime audio proc thread can switch over to them, no interpolation is used to smooth the switch.

SIMD is not coded, so filtering is inefficiently implemented in this demo. Using gcc compiler optimizer at -O3 will allow more audio files to be played in one session without exceeding audio frame times, exceeding frame times will cause audio glitches. If you get glitches reduce the number of audio files.

There is a audio file limit of 8.


### Dependency

Requires RTAudio library which allows use with pulseaudio or jack audio.

Requires 'libsofa' and all its dependencies, get it from github and follow its build instructions, fortunately it also provides the all the libraries it depends on, in ../API_Cpp-master/libsofa/dependencies/lib/linux/

You will need to edit this app's Makefile and point paths to where you built libsofa, NOTE: the ordering of library filenames does matter. Path examples:

INCLUDE= -I/home/you/API_Cpp-master/libsofa/src/     etc etc
LIBS= -L/home/you/API_Cpp-master/libsofa/dependencies/lib/linux/ -L/home/you/API_Cpp-master/libsofa/build/   etc etc


The libsofa build has a useful test app that tries to dump information about a sofa file at: /home/you/API_Cpp-master/libsofa/build/sofainfo, it will also 'assert' and core dump on some files as mentioned above.


### Build
The code was developed with gcc on Ubuntu 20.04 64 bit.

Build this app by typing 'make', no installation is required, run app from where you built it, it will need write permissions to allow the 'layout save' and record features.


### Usage
:warning:(This code can drive audio levels to their fullest, so set a safe maximum with your system's volume control before use):warning:

You need to make sure you have no other audio processing turned on elsewhere which could imbalance gain or phase on the headphones.
 
Run app with './hrir_cmdline -h' to get help and usage examples, or press 'h' once running.

You can specify multiple audio files that have the same samplerate as the sofa file. If you run with '-v' verbose on, it will show you what the sofa file samplerate is. Don't use large audio files as mentioned above. 

There is a limit to how many files can be processed and is dependent on system frame rate and the power of you cpu, exceeding this limit will cause audio glitches.

With verbose enabled on start up using -v, all sofa impulse entries like below are displayed, idx is entry number, pos[0] is azimuth(degrees), then elevation(degrees), then radius(meters):</br>
```lib_sofa_get_file_details() - Measure idx [000] -   pos[0]: 0.0000 pos[1]: -30.0000   pos[2]: 1.2000``` 


Press '1' to hear original unfiltered audio and adjust master gain levels with '[' and ']' keys.</br>
Press '2' to hear 3D audio through HRIR impulse responses.

',' and '.' to adjust the currently selected audio file's replay gain, also '<'  '>'

use 'a s' to adj azimuth around the horizontal plane at your ear line.</br>
Use 'w z'  to adj elevation upward and downward.</br>

'5' or '6' to select a different audio file from those you provided on start up, aud[0] status on cmd line means the 1st audio file is sel,  aud[1] is 2nd, etc.

'm' or 'M' for Solo or Mute of current audio source, respectively.

'f' flip between current angles you have set and zero angles (quick way to temporarily switch to center front).

'r' or 'R' for an auto rotate feature which moves azimuth or elevation automatically, it only moves between the available angles the sofa file has entries for.

'i' inverts ch1 (right chan) to see if you have a 180 deg phase error on your set up, this is only useful in finding wiring errors which are unlikely.

'9' '0' for white noise or test tones.

'|'  toggles record state, records realtime audio which you hear to memory, record is saved as file 'zzrecord.wav' on exit.

'v' to toggle verbose.

'h' '?' for help.

'b' for bye/exit.

You can save your current layout of sound positions and gain by using ':'  (file: zzlayout_new.txt)</br>
Load a layout with 'l' or 'L' keep pressing it till you get to the layout you want, 8 are possible plus the the recordable 'zzlayout_new.txt'.</br> 
The layout file format is: one audio src per line, values are: gain azimuth elevation radius filename.</br>
Radius does nothing in the demo. The audio filenames are for ref only and not used when loading a layout or at start up.</br> 
</br> 

impRspIdx 0050 status on cmd line means the 51th impulse response pair in the sofa file is being used.</br> 


##### Download audio recordings to hear what this demo does:
zzrec0.wav  is azimuth rotation 0->360 degs, (horizontal rotation).</br>
zzrec1.wav  is azimuth rotate to left ear at 90 degs, then elevation upwards to +80 deg, followed by downwards to -30 degs, back to 0 degs elevation, then it returns to front center.</br>
zzrec2.wav  has 3 sounds, the footsteps are azimuth rotated 0->360 deg.</br>






