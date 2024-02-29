# hrir_cmdline
3D audio test app, uses a HRIR sofa file to produce realtime audio allowing you to position audio sources.



under development.........


Feb 2024 v1.01



### hrir_cmdline
Linux command line utility that uses Head Related Impulse Response (HRIR) sofa files, allows testing of 3D binaural audio using headphones.

>`(This code can drive audio levels to their fullest, so set a safe maximum with your system's volume control before use)`

### Further infomation
https://en.wikipedia.org/wiki/Head-related_transfer_function
https://sofacoustics.org/data/examples/
https://github.com/sofacoustics/API_Cpp



### Theory and Purpose
SOFA files contain data for audio processing, some contain HRIR impulse responses which can be used to generate 3D audio. The impulse responses are used to filter user audio in the time domain. They  can be made in a number of ways, one way is with a sharp sound pulse emitter (i.e. impulse) from a device/speaker. The pulsed sound is position somewhere near a dummy head with ears (~1.5M). Each ear has an in-ear pickup microphone which record the impulse. The impulses ends up being gain and phase modulated by the shape of the ear and its pinna and is therefore dependent on where the sound was generated in 3D space relative to the head. The pair of impulse responses capture just one position in 3D space. The sound source is rotated to different angle/positions to build up good coverage. To reduce data, stepped/quantized angles positions (such as Azimuth and Elevation) are only gathered in the sofa file. Therefore a pair of impulses is associated with one 3D position (set of angles, they could also be defined as an x,y,z vector). These impulse responses can later be used to convolve (fir filter) any mono audio source. The result is that the audio source under goes gain and phase changes which can mimic what your ears hear. Having access to all these impulse responses allows for movement of your audio source in 3D space. 

Everyone's ears are slightly different, so impulse responses work best when tailored to the individual. Using someone else's will cause cruder 3D location perception and is therefore a compromise.

This demo allows you to hear the 3D effect, it's only for headphones, in-ear variety work well. You need to make sure you have no other audio processing turned on elsewhere which could 
imbalance gain or phase on the headphones.



### Prep
You need a suitable HRIR sofa file (https://sofacoustics.org/data/examples/).
Some do not work as 'libsofa' (which this demo uses) can't read them and will cause an 'assert' within the library itself, could be versioning or malformed file issue. 

But, file 'SimpleFreeFieldHRIR_1.0.sofa' works well, its samplerate is 48KHz.
It covers 360 degrees of azimuth (horiz rotation around your ear line), 0 degs the sound source forward of you, +90 degs is left, and 275 degs is right.
it coveress +80 to -30 degrees of elevation (plus is upward, minus is down).

You also need a 16 bit PCM .wav or .au audio file that has the same samplerate as the sofa file. E.g. water pouring into a cup or footsteps on gravel are good choices as they have high frequency content, the file will loop play, best to make sure its quite long, or it will become annoying. The file can be stereo, it will be mono'ed on the fly as its being fed into the fir filters.


The app does not record into a file, it just renders to system audio.

The libsofa build has a useful test app that tries to dump information about a sofa file at: /home/you/API_Cpp-master/libsofa/build/sofainfo, it will also 'assert' on some files.



### Code and Shortcomings
Where pieces of code or ideas are originated from other people, the comments in source code will give links to who or where it came from.

Some of the code is re-purposed from other projects I've tinkered with over the years, so excuse the antiquated c style, evolving conventions and inefficiencies.

The audio files are loaded into memory and have a size limit, they need to be under 10 mins in length at 48KHz.

The switching between different angles is not seemless for audio due to simplicity of the code.

The filter switching between 'on-air' to 'prepared for on-air' is also simply coded and likely not thread safe. The filters are prepared in the main thread, 
then flagged so the realtime audio proc thread can switch over to them.

SIMD is not coded, so filtering is inefficiently implemented.




### Dependency

Requires RTAudio library which allows use with pulseaudio or jack audio.

Requires 'libsofa' and all its dependencies, get it from github and follow its build instructions, fortunately it also provides the all the libraries it depends on, in ../API_Cpp-master/libsofa/dependencies/lib/linux/

If you don't want to install libsofa into your system folders, you can edit this app's Makefile and point paths to where you built libsofa, NOTE: the ordering of library filenames does matter. Path examples:

INCLUDE= -I/home/you/API_Cpp-master/libsofa/src/     etc etc
LIBS= -L/home/you/API_Cpp-master/libsofa/dependencies/lib/linux/   etc etc

FIX LIBRARY libsofa path ????????????????/


### Build
The code was developed with gcc on Ubuntu 20.04 64 bit.

Build this app by typing 'make', no installation is required, run app from where you built it.


### Usage
>`(This code can drive audio levels to their fullest, so set a safe maximum with your system's volume control before use)`

Run app with './hrir_cmdline -h' to get help and usage examples.

You can use multiple audio file that have the same samplerate as the sofa file. If you run with '-v' verbose on, it will show you what the sofa file samplerate is.

Also shown is all the impulse entries like below, pos[0] is azimuth(degrees), then elevation(degrees), then radius in meters:
<span style="color:blue">some *blue* text</span>
```lib_sofa_get_file_details() - Measure idx [000] -   pos[0]: 0.0000 pos[1]: -30.0000   pos[2]: 1.2000``` 


Press '1' to hear original unfiltered audio and adjust master gain levels with '[' and ']' keys.
Press '2' to hear 3D audio through HRIR impulse responses.

You can adjust the currently selected audio file's replay gain with ',' and '.' keys

use 'a s' to adj azimuth, horizotal plane around your earline.
Use 'w z'  to adj elevation upward and downward.

You are able Solo and Mute.

There is an auto rotate feature which moves azimuth or elevation automatically, between available angles held in sofa file.

You are able to invert ch1 (right chan) to see if you have a 180 deg phase error on your set up.

There is a test tone and white noise options.

Flip between ????????

???????????????
You can save your current layout of sound positions and gain by using 'L'  (file: zzlayout_new.txt)
Load a layout with 'l' (file: zzlayout.txt)
???????????????

##### Download audio file 'zz_audio.au' to hear a recording of what this demo produces in headphones (Note: the app does not record audio files).



```
```






