/* TinySoundFont - v0.9 - SoundFont2 synthesizer - https://github.com/schellingb/TinySoundFont
                                     no warranty implied; use at your own risk
   Do this:
      #define TSF_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #define TSF_IMPLEMENTATION
   #include "tsf.h"

   [OPTIONAL] #define TSF_NO_STDIO to remove stdio dependency
   [OPTIONAL] #define TSF_MALLOC, TSF_REALLOC, and TSF_FREE to avoid stdlib.h
   [OPTIONAL] #define TSF_MEMCPY, TSF_MEMSET, TSF_STRNCMP to avoid string.h
   [OPTIONAL] #define TSF_POW, TSF_POWF, TSF_EXPF, TSF_LOG, TSF_TAN, TSF_LOG10, TSF_SQRTF, TSF_ROUND, TSF_CEIL, TSF_LOG2, TSF_COS, TSF_SIN, TSF_FABS, TSF_FLOOR, TSF_FMOD to avoid math.h

   NOT YET IMPLEMENTED
     - Better low-pass filter without lowering performance too much
     - Support for modulators

   LICENSE (MIT)

   Copyright (C) 2017-2025 Bernhard Schelling
   Based on SFZero, Copyright (C) 2012 Steve Folta (https://github.com/stevefolta/SFZero)

   Permission is hereby granted, free of charge, to any person obtaining a copy of this
   software and associated documentation files (the "Software"), to deal in the Software
   without restriction, including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
   to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef TSF_INCLUDE_TSF_INL
#define TSF_INCLUDE_TSF_INL

#ifdef __cplusplus
extern "C" {
#  define CPP_DEFAULT0 = 0
#else
#  define CPP_DEFAULT0
#endif

//define this if you want the API functions to be static
#ifdef TSF_STATIC
#define TSFDEF static
#else
#define TSFDEF extern
#endif

// The load functions will return a pointer to a struct tsf which all functions
// thereafter take as the first parameter.
// On error the tsf_load* functions will return NULL most likely due to invalid
// data (or if the file did not exist in tsf_load_filename).
typedef struct tsf tsf;
typedef struct tsf_soundbank tsf_soundbank;

#ifndef TSF_NO_STDIO
// Directly load a SoundFont from a .sf2 file path
TSFDEF tsf_soundbank* tsf_soundbank_load_filename(const char* filename);
#endif

// Load a SoundFont from a block of memory
TSFDEF tsf_soundbank* tsf_soundbank_load_memory(const void* buffer, int size);

// Stream structure for the generic loading
struct tsf_stream
{
	// Custom data given to the functions as the first parameter
	void* data;

	// Function pointer will be called to read 'size' bytes into ptr (returns number of read bytes)
	int (*read)(void* data, void* ptr, unsigned int size);

	// Function pointer will be called to skip ahead over 'count' bytes (returns 1 on success, 0 on error)
	int (*skip)(void* data, unsigned int count);
};

// Generic SoundFont loading method using the stream structure above
TSFDEF tsf_soundbank* tsf_soundbank_load(struct tsf_stream* stream);

// Free a font structure
TSFDEF void tsf_soundbank_close(tsf_soundbank* sb);

// Stop all playing notes immediately and reset all channel parameters
TSFDEF void tsf_reset(tsf* f);

// Returns the soundbank and preset index for a preset number, or -1 if none could be found
TSFDEF int tsf_get_presetindex(const tsf* f, const tsf_soundbank** sb, int bank, int preset_number);

// Returns the preset index from a bank and preset number, or -1 if it does not exist in the loaded SoundFont
TSFDEF int tsf_soundbank_get_presetindex(const tsf_soundbank* sb, int bank, int preset_number);

// Returns the number of presets in the loaded SoundFont
TSFDEF int tsf_soundbank_get_presetcount(const tsf_soundbank* sb);

// Returns the name of a preset index >= 0 and < tsf_get_presetcount()
TSFDEF const char* tsf_soundbank_get_presetname(const tsf_soundbank* sb, int preset_index);

// Returns the name of a preset by bank and preset number
TSFDEF const char* tsf_bank_get_presetname(const tsf* f, int bank, int preset_number);

// Supported output modes by the render methods
enum TSFOutputMode
{
	// Two channels with single left/right samples one after another
	TSF_STEREO_INTERLEAVED,
	// Two channels with all samples for the left channel first then right
	TSF_STEREO_UNWEAVED,
	// A single channel (stereo instruments are mixed into center)
	TSF_MONO
};

// Thread safety:
//
// 1. Rendering / voices:
//
// Your audio output which calls the tsf_render* functions will most likely
// run on a different thread than where the playback tsf_note* functions
// are called. In which case some sort of concurrency control like a
// mutex needs to be used so they are not called at the same time.
// Alternatively, you can pre-allocate a maximum number of voices that can
// play simultaneously by calling tsf_set_max_voices after loading.
// That way memory re-allocation will not happen during tsf_note_on and
// TSF should become mostly thread safe.
// There is a theoretical chance that ending notes would negatively influence
// a voice that is rendering at the time but it is hard to say.
// Also be aware, this has not been tested much.
//
// 2. Channels:
//
// Calls to tsf_channel_set_... functions may allocate new channels
// if no channel with that number was previously used. Make sure to
// create all channels at the beginning as required if you call tsf_render*
// from a different thread.

// Setup the parameters for the voice render methods
//   outputmode: if mono or stereo and how stereo channel data is ordered
//   samplerate: the number of samples per second (output frequency)
//   global_gain_db: volume gain in decibels (>0 means higher, <0 means lower)
TSFDEF tsf* tsf_init(enum TSFOutputMode outputmode, int samplerate, float global_gain_db CPP_DEFAULT0);

// Add a sound bank to the list of loaded banks
TSFDEF int tsf_add_soundbank(tsf* f, const tsf_soundbank* sb);

// Set the global gain as a volume factor
//   global_gain: the desired volume where 1.0 is 100%
TSFDEF void tsf_set_volume(tsf* f, float global_gain);

// Set the maximum number of voices to play simultaneously
// Depending on the soundfond, one note can cause many new voices to be started,
// so don't keep this number too low or otherwise sounds may not play.
//   max_voices: maximum number to pre-allocate and set the limit to
//   (tsf_set_max_voices returns 0 if allocation failed, otherwise 1)
TSFDEF int tsf_set_max_voices(tsf* f, int max_voices);

// Start playing a note
//   preset_index: preset index >= 0 and < tsf_get_presetcount()
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   bank: instrument bank number (alternative to preset_index)
//   preset_number: preset number (alternative to preset_index)
//   (tsf_note_on returns 0 if the allocation of a new voice failed, otherwise 1)
//   (tsf_bank_note_on returns 0 if preset does not exist or allocation failed, otherwise 1)
TSFDEF int tsf_note_on(tsf* f, const tsf_soundbank* sb, int preset_index, int key, float vel);
TSFDEF int tsf_bank_note_on(tsf* f, int bank, int preset_number, int key, float vel);

// Stop playing a note
//   (bank_note_off returns 0 if preset does not exist, otherwise 1)
TSFDEF void tsf_note_off(tsf* f, const tsf_soundbank* sb, int preset_index, int key);
TSFDEF int  tsf_bank_note_off(tsf* f, int bank, int preset_number, int key);

// Stop playing all notes (end with sustain and release)
TSFDEF void tsf_note_off_all(tsf* f, int quick);

// Returns the number of active voices
TSFDEF int tsf_active_voice_count(tsf* f);

// Render output samples into a buffer
// You can either render as signed 16-bit values (tsf_render_short) or
// as 32-bit float values (tsf_render_float)
//   buffer: target buffer of size samples * output_channels * sizeof(type)
//   samples: number of samples to render
//   flag_mixing: if 0 clear the buffer first, otherwise mix into existing data
TSFDEF void tsf_render_short(tsf* f, short* buffer, int samples, int flag_mixing CPP_DEFAULT0);
TSFDEF void tsf_render_float(tsf* f, float* buffer, int samples, int flag_mixing CPP_DEFAULT0);
TSFDEF void tsf_render_float_separate(tsf* f, float* bufferL, float* bufferR, int samples, int flag_mixing CPP_DEFAULT0);

// Higher level channel based functions, set up channel parameters
//   channel: channel number
//   preset_index: preset index >= 0 and < tsf_get_presetcount()
//   preset_number: preset number (alternative to preset_index)
//   flag_mididrums: 0 for normal channels, otherwise apply MIDI drum channel rules
//   bank: instrument bank number (alternative to preset_index)
//   pan: stereo panning value from 0.0 (left) to 1.0 (right) (default 0.5 center)
//   volume: linear volume scale factor (default 1.0 full)
//   pitch_wheel: pitch wheel position 0 to 16383 (default 8192 unpitched)
//   pitch_range: range of the pitch wheel in semitones (default 2.0, total +/- 2 semitones)
//   tuning: tuning of all playing voices in semitones (default 0.0, standard (A440) tuning)
//   flag_sustain: 0 to end notes that were held sustained and disable holding sustain otherwise enable it
//   (tsf_set_preset_number and set_bank_preset return 0 if preset does not exist, otherwise 1)
//   (tsf_channel_set_... return 0 if a new channel needed allocation and that failed, otherwise 1)
TSFDEF int tsf_channel_set_presetindex(tsf* f, int channel, const tsf_soundbank* sb, int preset_index);
TSFDEF int tsf_channel_set_presetnumber(tsf* f, int channel, int preset_number, int flag_mididrums CPP_DEFAULT0);
TSFDEF int tsf_channel_set_bank(tsf* f, int channel, int bank);
TSFDEF int tsf_channel_set_bank_preset(tsf* f, int channel, int bank, int preset_number);
TSFDEF int tsf_channel_set_pan(tsf* f, int channel, float pan);
TSFDEF int tsf_channel_set_volume(tsf* f, int channel, float volume);
TSFDEF int tsf_channel_set_pitchwheel(tsf* f, int channel, int pitch_wheel);
TSFDEF int tsf_channel_set_pitchrange(tsf* f, int channel, float pitch_range);
TSFDEF int tsf_channel_set_tuning(tsf* f, int channel, float tuning);
TSFDEF int tsf_channel_set_sustain(tsf* f, int channel, int flag_sustain);
TSFDEF int tsf_channel_set_reverb_send(tsf* f, int channel, int reverb_send);
TSFDEF int tsf_channel_set_chorus_send(tsf* f, int channel, int chorus_send);

// Start or stop playing notes on a channel (needs channel preset to be set)
//   channel: channel number
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   (tsf_channel_note_on returns 0 on allocation failure of new voice, otherwise 1)
TSFDEF int tsf_channel_note_on(tsf* f, int channel, int key, float vel);
TSFDEF void tsf_channel_note_off(tsf* f, int channel, int key);
TSFDEF void tsf_channel_note_off_all(tsf* f, int channel); //end with sustain and release
TSFDEF void tsf_channel_sounds_off_all(tsf* f, int channel); //end immediately

// Apply a MIDI control change to the channel (not all controllers are supported!)
//    (tsf_channel_midi_control returns 0 on allocation failure of new channel, otherwise 1)
TSFDEF int tsf_channel_midi_control(tsf* f, int channel, int controller, int control_value);

// Apply pressure to whole channel
TSFDEF int tsf_channel_set_channel_pressure(tsf* f, int channel, int value);

// Apply pressure to one note on a channel
TSFDEF int tsf_channel_set_key_pressure(tsf* f, int channel, int key, int value);

// Get current values set on the channels
TSFDEF int tsf_channel_get_preset_index(tsf* f, const tsf_soundbank** sb, int channel);
TSFDEF int tsf_channel_get_preset_bank(tsf* f, int channel);
TSFDEF int tsf_channel_get_preset_number(tsf* f, int channel);
TSFDEF float tsf_channel_get_pan(tsf* f, int channel);
TSFDEF float tsf_channel_get_volume(tsf* f, int channel);
TSFDEF int tsf_channel_get_pitchwheel(tsf* f, int channel);
TSFDEF float tsf_channel_get_pitchrange(tsf* f, int channel);
TSFDEF float tsf_channel_get_tuning(tsf* f, int channel);

#ifdef __cplusplus
#  undef CPP_DEFAULT0
}
#endif

// end header
// ---------------------------------------------------------------------------------------------------------
#endif //TSF_INCLUDE_TSF_INL

#ifdef TSF_IMPLEMENTATION
#undef TSF_IMPLEMENTATION

// The lower this block size is the more accurate the effects are.
// Increasing the value significantly lowers the CPU usage of the voice rendering.
// If LFO affects the low-pass filter it can be hearable even as low as 8.
#ifndef TSF_RENDER_EFFECTSAMPLEBLOCK
#define TSF_RENDER_EFFECTSAMPLEBLOCK 64
#endif

// The larger this block size is the less setup work the effects need to do,
// but the larger fixed buffer sizes are needed for each unit of work processed.
#ifndef TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK
#define TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK 128
#endif

// When using tsf_render_short, to do the conversion a buffer of a fixed size is
// allocated on the stack. On low memory platforms this could be made smaller.
// Increasing this above 512 should not have a significant impact on performance.
// The value should be a multiple of TSF_RENDER_EFFECTSAMPLEBLOCK.
#ifndef TSF_RENDER_SHORTBUFFERBLOCK
#define TSF_RENDER_SHORTBUFFERBLOCK 512
#endif

// Grace release time for quick voice off (avoid clicking noise)
#define TSF_FASTRELEASETIME 0.01f

#if !defined(TSF_MALLOC) || !defined(TSF_FREE) || !defined(TSF_REALLOC)
#  include <stdlib.h>
#  define TSF_MALLOC  malloc
#  define TSF_FREE    free
#  define TSF_REALLOC realloc
#endif

#if !defined(TSF_MEMCPY) || !defined(TSF_MEMSET)
#  include <string.h>
#  define TSF_MEMCPY  memcpy
#  define TSF_MEMSET  memset
#  define TSF_STRNCMP strncmp
#endif

#if !defined(TSF_POW) || !defined(TSF_POWF) || !defined(TSF_EXPF) || !defined(TSF_LOG) || !defined(TSF_TAN) || !defined(TSF_LOG10) || !defined(TSF_SQRTF) || !defined(TSF_ROUND) || !defined(TSF_CEIL) || !defined(TSF_LOG2) || !defined(TSF_COS) || !defined(TSF_SIN) || !defined(TSF_FABS)
#  include <math.h>
#  if !defined(__cplusplus) && !defined(NAN) && !defined(powf) && !defined(expf) && !defined(sqrtf)
#    define powf (float)pow // deal with old math.h
#    define expf (float)exp // files that come without
#    define sqrtf (float)sqrt // powf, expf and sqrtf
#  endif
#  define TSF_POW     pow
#  define TSF_POWF    powf
#  define TSF_EXPF    expf
#  define TSF_LOG     log
#  define TSF_TAN     tan
#  define TSF_LOG10   log10
#  define TSF_SQRTF   sqrtf
#  define TSF_ROUND   round
#  define TSF_CEIL    ceil
#  define TSF_LOG2    log2
#  define TSF_COS     cos
#  define TSF_SIN     sin
#  define TSF_FABS    fabs
#  define TSF_FLOOR   floor
#  define TSF_FMOD    fmod
#endif

#ifndef TSF_NO_STDIO
#  include <stdio.h>
#endif

#ifndef TSF_NO_STDDEF
#  include <stddef.h>
#endif

#define TSF_COUNTOF(a) (sizeof(a) / sizeof((a)[0]))

#define TSF_TRUE 1
#define TSF_FALSE 0
#define TSF_BOOL unsigned char
#define TSF_PI 3.14159265358979323846264338327950288
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define TSF_NULL nullptr
#else
#define TSF_NULL 0
#endif

#define TSF_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define TSF_MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

typedef char tsf_fourcc[4];
typedef signed char tsf_s8;
typedef unsigned char tsf_u8;
typedef unsigned short tsf_u16;
typedef signed short tsf_s16;
typedef unsigned int tsf_u32;
typedef unsigned long long tsf_u64;
typedef char tsf_char20[20];
typedef char tsf_char40[40];

#define TSF_FourCCEquals(value1, value2) (value1[0] == value2[0] && value1[1] == value2[1] && value1[2] == value2[2] && value1[3] == value2[3])

struct tsf
{
	const struct tsf_soundbank** soundbanks;
	struct tsf_voice* voices;
	struct tsf_channels* channels;
	struct tsf_modulator* modulators;

	struct tsf_reverb* reverb;
	struct tsf_chorus* chorus;

	float reverbInput[TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK];
	float chorusInput[TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK];

	int bankNum;
	int voiceNum;
	int maxVoiceNum;
	int modNum;
	unsigned int voicePlayIndex;

	enum TSFOutputMode outputmode;
	float outSampleRate;
	float globalGainDB;
	float envelopeSmoothingFactor;
};

#ifndef TSF_NO_STDIO
static int tsf_stream_stdio_read(FILE* f, void* ptr, unsigned int size) { return (int)fread(ptr, 1, size, f); }
static int tsf_stream_stdio_skip(FILE* f, unsigned int count) { return !fseek(f, count, SEEK_CUR); }
TSFDEF tsf_soundbank* tsf_soundbank_load_filename(const char* filename)
{
	tsf_soundbank* res;
	struct tsf_stream stream = { TSF_NULL, (int(*)(void*,void*,unsigned int))&tsf_stream_stdio_read, (int(*)(void*,unsigned int))&tsf_stream_stdio_skip };
	#if __STDC_WANT_SECURE_LIB__
	FILE* f = TSF_NULL; fopen_s(&f, filename, "rb");
	#else
	FILE* f = fopen(filename, "rb");
	#endif
	if (!f)
	{
		//if (e) *e = TSF_FILENOTFOUND;
		return TSF_NULL;
	}
	stream.data = f;
	res = tsf_soundbank_load(&stream);
	fclose(f);
	return res;
}
#endif

struct tsf_stream_memory { const char* buffer; unsigned int total, pos; };
static int tsf_stream_memory_read(struct tsf_stream_memory* m, void* ptr, unsigned int size) { if (size > m->total - m->pos) size = m->total - m->pos; TSF_MEMCPY(ptr, m->buffer+m->pos, size); m->pos += size; return (int)size; }
static int tsf_stream_memory_skip(struct tsf_stream_memory* m, unsigned int count) { if (m->pos + count > m->total) return 0; m->pos += count; return 1; }
TSFDEF tsf_soundbank* tsf_soundbank_load_memory(const void* buffer, int size)
{
	struct tsf_stream stream = { TSF_NULL, (int(*)(void*,void*,unsigned int))&tsf_stream_memory_read, (int(*)(void*,unsigned int))&tsf_stream_memory_skip };
	struct tsf_stream_memory f = { TSF_NULL, 0, 0 };
	f.buffer = (const char*)buffer;
	f.total = (unsigned)size;
	stream.data = &f;
	return tsf_soundbank_load(&stream);
}

static TSF_BOOL _tsf_big_endian = TSF_FALSE;

static void tsf_read_init(void)
{
	union { tsf_u8 bytes[4]; tsf_u32 dword; } u;
	u.dword = 1;
	_tsf_big_endian = u.bytes[3];
}

static int tsf_read_endian(struct tsf_stream* stream, void* buffer, int size)
{
	int res = stream->read(stream->data, buffer, size);
	if (res == size && _tsf_big_endian)
	{
		tsf_u8 *p, *pSrc, *pEnd;
		for (p = (tsf_u8 *)buffer, pEnd = p + size / 2, pSrc = p + size - 1; p != pEnd; p++, pSrc--)
		{
			tsf_u8 tmp = *p; *p = *pSrc; *pSrc = tmp;
		}
	}
	return res;
}

struct tsf_version
{
	tsf_u16 major, minor;
};

struct tsf_sfe_version
{
	tsf_u16 major, minor;
	tsf_char20 specType;
	tsf_u16 draft;
	tsf_char20 verStr;
};

struct tsf_sfe_flags
{
	tsf_u8 branch, leaf;
	tsf_u32 flags;
	const char *featureName;
};

// These are only bit checked at load time to verify support
static struct tsf_sfe_flags tsf_sfe_supported_flags[] = {
	{ 0,  0,        15, "Tuning" },
	{ 0,  1,         3, "Looping" },
	{ 0,  2,         1, "Filter Types" },
	{ 0,  3, 884736096, "Filter Params" },
	{ 0,  4,         7, "Attenuation" },
	{ 0,  5,     69391, "Effects" },
	{ 0,  6,        15, "LFO" },
	{ 0,  7,    524287, "Envelopes" },
	{ 0,  8,    231169, "MIDI CC" },
	{ 0,  9,       127, "Generators" },
	{ 0, 10,       127, "Zones" },
	{ 0, 11,         0, "Reserved" },
	{ 1,  0,     16383, "Modulators" },
	{ 1,  1,        51, "Modulator Controllers" },
	{ 1,  2,    998838, "Modulator Parameters" },
	{ 1,  3, 672137215, "Modulator Parameters" },
	{ 1,  4,         0, "Modulator Parameters" },
	{ 1,  5,         0, "NRPN" },
	{ 1,  6,    263167, "Default Modulators" },
	{ 1,  7,         0, "Reserved" },
	{ 1,  8,         0, "Reserved" },
	{ 2,  0,         1, "24-Bit Samples" },
	{ 2,  1,         0, "8-Bit Samples" },
	{ 2,  2,         0, "32-Bit Samples" },
	{ 2,  3,         0, "64-Bit Samples" },
	{ 3,  0,         1, "SFe Compression" },
	{ 3,  1,         1, "Compression Formats" },
	{ 4,  0,         0, "Metadata" },
	{ 4,  1,         0, "Reserved" },
	{ 4,  2,         0, "Sample ROM" },
	{ 4,  3,         0, "ROM Emulator" },
	{ 4,  4,         0, "Reserved" },
	{ 5,  0,         0, "End of Flags" }
};

static int tsf_sfe_check_flags(const struct tsf_sfe_flags *f)
{
	const struct tsf_sfe_flags *c = tsf_sfe_supported_flags, *cEnd = c + TSF_COUNTOF(tsf_sfe_supported_flags);
	for (; c != cEnd; c++)
	{
		if (c->branch != f->branch || c->leaf != f->leaf) continue;
		if ((f->flags & c->flags) != f->flags) return 0;
		else return 1;
	}
	return 0;
}

struct tsf_soundbank
{
	struct tsf_preset* presets;
	float* bankSamples;
	int presetNum;

	struct tsf_version version, romVersion;
};

enum { TSF_LOOPMODE_NONE, TSF_LOOPMODE_CONTINUOUS, TSF_LOOPMODE_SUSTAIN };

enum { TSF_SEGMENT_NONE, TSF_SEGMENT_DELAY, TSF_SEGMENT_ATTACK, TSF_SEGMENT_HOLD, TSF_SEGMENT_DECAY, TSF_SEGMENT_SUSTAIN, TSF_SEGMENT_RELEASE, TSF_SEGMENT_DONE };

struct tsf_hydra
{
	struct tsf_hydra_phdr *phdrs; struct tsf_hydra_pbag *pbags; struct tsf_hydra_pmod *pmods;
	struct tsf_hydra_pgen *pgens; struct tsf_hydra_inst *insts; struct tsf_hydra_ibag *ibags;
	struct tsf_hydra_imod *imods; struct tsf_hydra_igen *igens; struct tsf_hydra_shdr *shdrs;
	int phdrNum, pbagNum, pmodNum, pgenNum, instNum, ibagNum, imodNum, igenNum, shdrNum;
};

union tsf_hydra_genamount { struct { tsf_u8 lo, hi; } range; tsf_s16 shortAmount; tsf_u16 wordAmount; };
struct tsf_hydra_phdr { tsf_char20 presetName; tsf_u16 preset, bank, presetBagNdx; tsf_u32 library, genre, morphology; };
struct tsf_hydra_pbag { tsf_u16 genNdx, modNdx; };
struct tsf_hydra_pmod { tsf_u16 modSrcOper, modDestOper; tsf_s16 modAmount; tsf_u16 modAmtSrcOper, modTransOper; };
struct tsf_hydra_pgen { tsf_u16 genOper; union tsf_hydra_genamount genAmount; };
struct tsf_hydra_inst { tsf_char20 instName; tsf_u16 instBagNdx; };
struct tsf_hydra_ibag { tsf_u16 instGenNdx, instModNdx; };
struct tsf_hydra_imod { tsf_u16 modSrcOper, modDestOper; tsf_s16 modAmount; tsf_u16 modAmtSrcOper, modTransOper; };
struct tsf_hydra_igen { tsf_u16 genOper; union tsf_hydra_genamount genAmount; };
struct tsf_hydra_shdr { tsf_char20 sampleName; tsf_u32 start, end, startLoop, endLoop, sampleRate; tsf_u8 originalPitch; tsf_s8 pitchCorrection; tsf_u16 sampleLink, sampleType; };

tsf_u32 tsf_hydra_phdr_read_preset(const struct tsf_hydra_phdr *pphdr, const struct tsf_hydra_phdr *pxphdr)
{
	tsf_u32 preset = pphdr->preset;
	//if (pxphdr) preset += pxphdr->preset * 65536; // apparently ignored
	return preset;
}

tsf_u32 tsf_hydra_phdr_read_bank(const struct tsf_hydra_phdr *pphdr, const struct tsf_hydra_phdr *pxphdr)
{
	tsf_u32 bank = pphdr->bank;
	//if (pxphdr) bank += pxphdr->bank * 65536; // apparently ignored
	return bank;
}

tsf_u32 tsf_hydra_phdr_read_presetBagNdx(const struct tsf_hydra_phdr *pphdr, const struct tsf_hydra_phdr *pxphdr)
{
	tsf_u32 presetBagNdx = pphdr->presetBagNdx;
	if (pxphdr) presetBagNdx += pxphdr->presetBagNdx * 65536;
	return presetBagNdx;
}

tsf_u32 tsf_hydra_pbag_read_genNdx(const struct tsf_hydra_pbag *ppbag, const struct tsf_hydra_pbag *pxpbag)
{
	tsf_u32 genNdx = ppbag->genNdx;
	if (pxpbag) genNdx += pxpbag->genNdx * 65536;
	return genNdx;
}

tsf_u32 tsf_hydra_pbag_read_modNdx(const struct tsf_hydra_pbag *ppbag, const struct tsf_hydra_pbag *pxpbag)
{
	tsf_u32 modNdx = ppbag->modNdx;
	if (pxpbag) modNdx += pxpbag->modNdx * 65536;
	return modNdx;
}

tsf_u32 tsf_hydra_ibag_read_instGenNdx(const struct tsf_hydra_ibag *pibag, const struct tsf_hydra_ibag *pxibag)
{
	tsf_u32 instGenNdx = pibag->instGenNdx;
	if (pxibag) instGenNdx += pxibag->instGenNdx * 65536;
	return instGenNdx;
}

tsf_u32 tsf_hydra_ibag_read_instModNdx(const struct tsf_hydra_ibag *pibag, const struct tsf_hydra_ibag *pxibag)
{
	tsf_u32 instModNdx = pibag->instModNdx;
	if (pxibag) instModNdx += pxibag->instModNdx * 65536;
	return instModNdx;
}

tsf_u16 tsf_hydra_inst_read_instBagNdx(const struct tsf_hydra_inst *pinst, const struct tsf_hydra_inst *pxinst)
{
	tsf_u16 instBagNdx = pinst->instBagNdx;
	if (pxinst) instBagNdx |= pxinst->instBagNdx;
	return instBagNdx;
}

#define TSFR(FIELD)  stream->read(stream->data, &i->FIELD, sizeof(i->FIELD));
#define TSFRE(FIELD) tsf_read_endian(stream, &i->FIELD, sizeof(i->FIELD));
static void tsf_hydra_read_phdr(struct tsf_hydra_phdr* i, struct tsf_stream* stream) { TSFR(presetName) TSFRE(preset) TSFRE(bank) TSFRE(presetBagNdx) TSFRE(library) TSFRE(genre) TSFRE(morphology) }
static void tsf_hydra_read_pbag(struct tsf_hydra_pbag* i, struct tsf_stream* stream) { TSFRE(genNdx) TSFRE(modNdx) }
static void tsf_hydra_read_pmod(struct tsf_hydra_pmod* i, struct tsf_stream* stream) { TSFRE(modSrcOper) TSFRE(modDestOper) TSFRE(modAmount) TSFRE(modAmtSrcOper) TSFRE(modTransOper) }
static void tsf_hydra_read_pgen(struct tsf_hydra_pgen* i, struct tsf_stream* stream) { TSFRE(genOper) TSFRE(genAmount) }
static void tsf_hydra_read_inst(struct tsf_hydra_inst* i, struct tsf_stream* stream) { TSFR(instName) TSFRE(instBagNdx) }
static void tsf_hydra_read_ibag(struct tsf_hydra_ibag* i, struct tsf_stream* stream) { TSFRE(instGenNdx) TSFRE(instModNdx) }
static void tsf_hydra_read_imod(struct tsf_hydra_imod* i, struct tsf_stream* stream) { TSFRE(modSrcOper) TSFRE(modDestOper) TSFRE(modAmount) TSFRE(modAmtSrcOper) TSFRE(modTransOper) }
static void tsf_hydra_read_igen(struct tsf_hydra_igen* i, struct tsf_stream* stream) { TSFRE(genOper) TSFRE(genAmount) }
static void tsf_hydra_read_shdr(struct tsf_hydra_shdr* i, struct tsf_stream* stream) { TSFR(sampleName) TSFRE(start) TSFRE(end) TSFRE(startLoop) TSFRE(endLoop) TSFRE(sampleRate) TSFR(originalPitch) TSFR(pitchCorrection) TSFRE(sampleLink) TSFRE(sampleType) }

static void tsf_read_version(struct tsf_version* i, struct tsf_stream* stream) { TSFRE(major) TSFRE(minor) }
static void tsf_read_sfe_version(struct tsf_sfe_version* i, struct tsf_stream* stream) { TSFRE(major) TSFRE(minor) TSFR(specType) TSFRE(draft) TSFR(verStr) }
#undef TSFR
#undef TSFRE

static tsf_u64 tsf_read_riff_size(struct tsf_stream* stream, int riff64)
{
	if (!riff64)
	{
		tsf_u32 size;
		if (!tsf_read_endian(stream, &size, sizeof(size))) return 0;
		return size;
	}
	else
	{
		tsf_u64 size;
		if (!tsf_read_endian(stream, &size, sizeof(size))) return 0;
		return size;
	}
}

struct tsf_riffchunk { tsf_fourcc id; tsf_u64 size; };
struct tsf_envelope { float delay, attack, hold, decay, sustain, release, keynumToHold, keynumToDecay; };
struct tsf_volume_envelope { unsigned char state; TSF_BOOL inRelease; float currentAttenuationDb, attenuation, attenuationTargetGain; int currentSampleTime; float releaseStartDb; int releaseStartTimeSamples; float currentReleaseGain; int attackDuration; int decayDuration; int releaseDuration; float attenuationTarget; float sustainDbRelative; int delayEnd; int attackEnd; int holdEnd; int decayEnd; TSF_BOOL canEndOnSilentSustain; struct tsf_envelope parameters; };
struct tsf_modulation_envelope { unsigned char state; TSF_BOOL inRelease; float attackDuration, decayDuration, holdDuration, releaseDuration, sustainLevel, delayEnd, attackEnd, holdEnd, decayEnd, releaseStartLevel, currentValue, releaseStartTime; struct tsf_envelope parameters; };
struct tsf_voice_lowpass { double QInv, a0, a1, b1, b2, z1, z2; TSF_BOOL active; };
struct tsf_voice_lfo { int samplesUntil; float level, delta; };

struct tsf_reverb_parameters { unsigned char level, preLowpass, character, time, delayFeedback, preDelayTime; };
struct tsf_chorus_parameters { unsigned char level, preLowpass, feedback, delay, rate, depth, sendLevelToReverb, sendLevelToDelay; };
struct tsf_delay_parameters { unsigned char level, preLowpass, timeCenter, timeRatioLeft, timeRatioRight, levelCenter, levelLeft, levelRight, feedback, sendLevelToReverb; };

struct tsf_delay_line { float feedback, gain; float *buffer; unsigned int bufferLength; unsigned int writeIndex; unsigned int time; };

struct tsf_dattorro_delay_line { float *buffer; unsigned int writeIndex, readIndex, writeMask; };

struct tsf_dattorro_reverb { unsigned int preDelay; float preLPF, inputDiffusion[2], decay, decayDiffusion[2], damping, excursionRate, excursionDepth, gain, sampleRate, lp[3], excPhase; unsigned int pDWrite, pDLength; short *taps; float *pDelay; struct tsf_dattorro_delay_line *delays; };

struct tsf_reverb { struct tsf_reverb_parameters parameters; struct tsf_dattorro_reverb dattorro; struct tsf_delay_line delayLeft; struct tsf_delay_line delayRight; unsigned int maxBufferSize; float *delayLeftOutput; float *delayRightOutput; float *delayLeftInput; float *delayPreLPF; float sampleRate; float preLPFfc; float preLPFa; float preLPFz; float characterTimeCoefficient, characterGainCoefficient, characterLPFCoefficient, delayGain, panDelayFeedback, delayFeedback; };

struct tsf_chorus { struct tsf_chorus_parameters parameters; unsigned int maxBufferSize; float preLPFfc; float preLPFa; float preLPFz; float *leftDelayBuffer; float *rightDelayBuffer; float sampleRate; float phase; unsigned int write; float gain; float reverbGain; float delayGain; unsigned int depthSamples; unsigned int delaySamples; float rateInc; float feedbackGain; };

enum tsf_generator_type
{
	INVALID = -1, // Invalid generator
	startAddrsOffset = 0, // Sample control - moves sample start point
	endAddrOffset = 1, // Sample control - moves sample end point
	startloopAddrsOffset = 2, // Loop control - moves loop start point
	endloopAddrsOffset = 3, // Loop control - moves loop end point
	startAddrsCoarseOffset = 4, // Sample control - moves sample start point in 32,768 increments
	modLfoToPitch = 5, // Pitch modulation - modulation lfo pitch modulation in cents
	vibLfoToPitch = 6, // Pitch modulation - vibrato lfo pitch modulation in cents
	modEnvToPitch = 7, // Pitch modulation - modulation envelope pitch modulation in cents
	initialFilterFc = 8, // Filter - lowpass filter cutoff in cents
	initialFilterQ = 9, // Filter - lowpass filter resonance
	modLfoToFilterFc = 10, // Filter modulation - modulation lfo lowpass filter cutoff in cents
	modEnvToFilterFc = 11, // Filter modulation - modulation envelope lowpass filter cutoff in cents
	endAddrsCoarseOffset = 12, // Ample control - move sample end point in 32,768 increments
	modLfoToVolume = 13, // Modulation lfo - volume (tremolo), where 100 = 10dB
	unused1 = 14, // Unused
	chorusEffectsSend = 15, // Effect send - how much is sent to chorus 0 - 1000
	reverbEffectsSend = 16, // Effect send - how much is sent to reverb 0 - 1000
	generatorPan = 17, // Panning - where -500 = left, 0 = center, 500 = right
	unused2 = 18, // Unused
	unused3 = 19, // Unused
	unused4 = 20, // Unused
	delayModLFO = 21, // Mod lfo - delay for mod lfo to start from zero
	freqModLFO = 22, // Mod lfo - frequency of mod lfo, 0 = 8.176 Hz, units: f => 1200log2(f/8.176)
	delayVibLFO = 23, // Vib lfo - delay for vibrato lfo to start from zero
	freqVibLFO = 24, // Vib lfo - frequency of vibrato lfo, 0 = 8.176Hz, unit: f => 1200log2(f/8.176)
	delayModEnv = 25, // Mod env - 0 = 1 s decay till mod env starts
	attackModEnv = 26, // Mod env - attack of mod env
	holdModEnv = 27, // Mod env - hold of mod env
	decayModEnv = 28, // Mod env - decay of mod env
	sustainModEnv = 29, // Mod env - sustain of mod env
	releaseModEnv = 30, // Mod env - release of mod env
	keyNumToModEnvHold = 31, // Mod env - also modulating mod envelope hold with key number
	keyNumToModEnvDecay = 32, // Mod env - also modulating mod envelope decay with key number
	delayVolEnv = 33, // Vol env - delay of envelope from zero (weird scale)
	attackVolEnv = 34, // Vol env - attack of envelope
	holdVolEnv = 35, // Vol env - hold of envelope
	decayVolEnv = 36, // Vol env - decay of envelope
	sustainVolEnv = 37, // Vol env - sustain of envelope
	releaseVolEnv = 38, // Vol env - release of envelope
	keyNumToVolEnvHold = 39, // Vol env - key number to volume envelope hold
	keyNumToVolEnvDecay = 40, // Vol env - key number to volume envelope decay
	instrument = 41, // Zone - instrument index to use for preset zone
	reserved1 = 42, // Reserved
	keyRange = 43, // Zone - key range for which preset / instrument zone is active
	velRange = 44, // Zone - velocity range for which preset / instrument zone is active
	startloopAddrsCoarseOffset = 45, // Sample control - moves sample loop start point in 32,768 increments
	keyNum = 46, // Zone - instrument only: always use this midi number (ignore what's pressed)
	velocity = 47, // Zone - instrument only: always use this velocity (ignore what's pressed)
	initialAttenuation = 48, // Zone - allows turning down the volume, 10 = -1dB
	reserved2 = 49, // Reserved
	endloopAddrsCoarseOffset = 50, // Sample control - moves sample loop end point in 32,768 increments
	coarseTune = 51, // Tune - pitch offset in semitones
	fineTune = 52, // Tune - pitch offset in cents
	sampleID = 53, // Sample - instrument zone only: which sample to use
	sampleModes = 54, // Sample - 0 = no loop, 1 = loop, 2 = start on release, 3 = loop and play till the end in release phase
	reserved3 = 55, // Reserved
	scaleTuning = 56, // Sample - the degree to which MIDI key number influences pitch, 100 = default
	exclusiveClass = 57, // Sample - = cut = choke group
	overridingRootKey = 58, // Sample - can override the sample's original pitch
	unused5 = 59, // Unused
	endOper = 60, // End marker

	// Additional generators that are used in system exclusives and will not be saved
	vibLfoToVolume = 61,
	vibLfoToFilterFc = 62
};

enum { MAX_GENERATOR = vibLfoToFilterFc };

struct tsf_generators { tsf_u16 g[MAX_GENERATOR + 1]; };

struct tsf_region
{
	int loop_mode;
	unsigned int sample_rate;
	unsigned char lokey, hikey, lovel, hivel;
	unsigned int group, offset, end, loop_start, loop_end;
	int transpose, tune, pitch_keycenter, pitch_keytrack;
	float attenuation, pan;
	struct tsf_envelope ampenv, modenv;
	int initialFilterQ, initialFilterFc;
	int modEnvToPitch, modEnvToFilterFc, modLfoToFilterFc, modLfoToVolume;
	float delayModLFO;
	int freqModLFO, modLfoToPitch;
	float delayVibLFO;
	int freqVibLFO, vibLfoToPitch;
	float reverb, chorus;
	struct tsf_modulator* modulators;
	struct tsf_generators generators;
	int modNum;
};

struct tsf_preset
{
	tsf_char40 presetName;
	tsf_u32 preset, bank;
	struct tsf_region* regions;
	struct tsf_modulator* modulators;
	int regionNum, modNum;
};

enum tsf_midi_controller
{
	bankSelect = 0,
	modulationWheel = 1,
	breathController = 2,
	undefinedCC3 = 3,
	footController = 4,
	portamentoTime = 5,
	dataEntryMSB = 6,
	mainVolume = 7,
	balance = 8,
	undefinedCC9 = 9,
	controllerPan = 10,
	expressionController = 11,
	effectControl1 = 12,
	effectControl2 = 13,
	undefinedCC14 = 14,
	undefinedCC15 = 15,
	generalPurposeController1 = 16,
	generalPurposeController2 = 17,
	generalPurposeController3 = 18,
	generalPurposeController4 = 19,
	undefinedCC20 = 20,
	undefinedCC21 = 21,
	undefinedCC22 = 22,
	undefinedCC23 = 23,
	undefinedCC24 = 24,
	undefinedCC25 = 25,
	undefinedCC26 = 26,
	undefinedCC27 = 27,
	undefinedCC28 = 28,
	undefinedCC29 = 29,
	undefinedCC30 = 30,
	undefinedCC31 = 31,
	bankSelectLSB = 32,
	modulationWheelLSB = 33,
	breathControllerLSB = 34,
	undefinedCC3LSB = 35,
	footControllerLSB = 36,
	portamentoTimeLSB = 37,
	dataEntryLSB = 38,
	mainVolumeLSB = 39,
	balanceLSB = 40,
	undefinedCC9LSB = 41,
	panLSB = 42,
	expressionControllerLSB = 43,
	effectControl1LSB = 44,
	effectControl2LSB = 45,
	undefinedCC14LSB = 46,
	undefinedCC15LSB = 47,
	undefinedCC16LSB = 48,
	undefinedCC17LSB = 49,
	undefinedCC18LSB = 50,
	undefinedCC19LSB = 51,
	undefinedCC20LSB = 52,
	undefinedCC21LSB = 53,
	undefinedCC22LSB = 54,
	undefinedCC23LSB = 55,
	undefinedCC24LSB = 56,
	undefinedCC25LSB = 57,
	undefinedCC26LSB = 58,
	undefinedCC27LSB = 59,
	undefinedCC28LSB = 60,
	undefinedCC29LSB = 61,
	undefinedCC30LSB = 62,
	undefinedCC31LSB = 63,
	sustainPedal = 64,
	portamentoOnOff = 65,
	sostenutoPedal = 66,
	softPedal = 67,
	legatoFootswitch = 68,
	hold2Pedal = 69,
	soundVariation = 70,
	filterResonance = 71,
	releaseTime = 72,
	attackTime = 73,
	brightness = 74,
	decayTime = 75,
	vibratoRate = 76,
	vibratoDepth = 77,
	vibratoDelay = 78,
	soundController10 = 79,
	generalPurposeController5 = 80,
	generalPurposeController6 = 81,
	generalPurposeController7 = 82,
	generalPurposeController8 = 83,
	portamentoControl = 84,
	undefinedCC85 = 85,
	undefinedCC86 = 86,
	undefinedCC87 = 87,
	undefinedCC88 = 88,
	undefinedCC89 = 89,
	undefinedCC90 = 90,
	reverbDepth = 91,
	tremoloDepth = 92,
	chorusDepth = 93,
	detuneDepth = 94,
	phaserDepth = 95,
	dataIncrement = 96,
	dataDecrement = 97,
	nonRegisteredParameterLSB = 98,
	nonRegisteredParameterMSB = 99,
	registeredParameterLSB = 100,
	registeredParameterMSB = 101,
	undefinedCC102LSB = 102,
	undefinedCC103LSB = 103,
	undefinedCC104LSB = 104,
	undefinedCC105LSB = 105,
	undefinedCC106LSB = 106,
	undefinedCC107LSB = 107,
	undefinedCC108LSB = 108,
	undefinedCC109LSB = 109,
	undefinedCC110LSB = 110,
	undefinedCC111LSB = 111,
	undefinedCC112LSB = 112,
	undefinedCC113LSB = 113,
	undefinedCC114LSB = 114,
	undefinedCC115LSB = 115,
	undefinedCC116LSB = 116,
	undefinedCC117LSB = 117,
	undefinedCC118LSB = 118,
	undefinedCC119LSB = 119,
	allSoundOff = 120,
	resetAllControllers = 121,
	localControlOnOff = 122,
	allNotesOff = 123,
	omniModeOff = 124,
	omniModeOn = 125,
	monoModeOn = 126,
	polyModeOn = 127
};

enum tsf_modulator_source_type
{
	noController = 0,
	noteOnVelocity = 2,
	noteOnKeyNum = 3,
	polyPressure = 10,
	channelPressure = 13,
	pitchWheel = 14,
	pitchWheelRange = 16,
	modLink = 127
};

enum { NON_CC_INDEX_OFFSET = 128 };
enum { CONTROLLER_TABLE_SIZE = 147 };

enum tsf_modulator_curve_type
{
	curveLinear = 0,
	curveConcave = 1,
	curveConvex = 2,
	curveSwitch = 3
};

enum tsf_modulator_transform_type
{
	transformLinear = 0,
	transformAbsolute = 2
};

struct tsf_modulator_source
{
	tsf_u8 isBipolar : 1, isNegative : 1, index : 7, isCC : 1;
	enum tsf_modulator_curve_type curveType;
};

enum { MODULATOR_RESOLUTION = 16384 };
enum { MOD_CURVE_TYPES_AMOUNT = 4 };
enum { MOD_SOURCE_TRANSFORM_POSSIBILITIES = 4 };

static TSF_BOOL tsf_curves_computed = TSF_FALSE;
static float tsf_concave[MODULATOR_RESOLUTION + 1];
static float tsf_convex[MODULATOR_RESOLUTION + 1];
static float tsf_precomputed_transforms[MODULATOR_RESOLUTION * MOD_SOURCE_TRANSFORM_POSSIBILITIES * MOD_CURVE_TYPES_AMOUNT];

static const float MODENV_PEAK = 1.0;
				
// 1000 should be precise enough
static float CONVEX_ATTACK[1000];

enum { MIN_TIMECENT = -15000 };
enum { MAX_TIMECENT =  15000 };
static double tsf_timecentLookupTable[MAX_TIMECENT - MIN_TIMECENT + 1];

enum { MIN_DECIBELS = -1660 };
enum { MAX_DECIBELS =  1600 };
static double tsf_decibelLookupTable[(MAX_DECIBELS - MIN_DECIBELS) * 100 + 1];

static float tsf_modulator_get_curve_value(tsf_u8 transformType, enum tsf_modulator_curve_type curveType, float value)
{
	const TSF_BOOL isBipolar = !!(transformType & 2);
	const TSF_BOOL isNegative = !!(transformType & 1);
	int index;

	// Inverse the value if needed
	if (isNegative) {
		value = 1.0 - value;
	}
	switch (curveType)
	{
		case curveLinear:
			if (isBipolar)
			{
				// Bipolar curve
				return value * 2.0 - 1.0;
			}
			return value;

		case curveSwitch:
			// Switch
			value = value > 0.5 ? 1.0 : 0.0;
			if (isBipolar)
			{
				// Multiply
				return value * 2.0 - 1.0;
			}
			return value;

		case curveConcave:
			// Look up the value
			if (isBipolar)
			{
				value = value * 2.0 - 1.0;
				if (value < 0)
				{
					index = (int)(value * -MODULATOR_RESOLUTION);
					return -tsf_concave[TSF_MAX(0, TSF_MIN(MODULATOR_RESOLUTION, index))];
				}
			}
			index = (int)(value * MODULATOR_RESOLUTION);
			return tsf_concave[TSF_MAX(0, TSF_MIN(MODULATOR_RESOLUTION, index))];

		case curveConvex:
			// Look up the value
			if (isBipolar)
			{
				value = value * 2.0 - 1.0;
				if (value < 0.0)
				{
					index = (int)(value * -MODULATOR_RESOLUTION);
					return -tsf_convex[TSF_MAX(0, TSF_MIN(MODULATOR_RESOLUTION, index))];
				}
			}
			index = (int)(value * MODULATOR_RESOLUTION);
			return tsf_convex[TSF_MAX(0, TSF_MIN(MODULATOR_RESOLUTION, index))];
	}
}

static void tsf_init_curves(void)
{
	int i, curveType, transformType, value;

	if (tsf_curves_computed) return;

	tsf_concave[0] = 0;
	tsf_concave[MODULATOR_RESOLUTION] = 1;

	tsf_convex[0] = 0;
	tsf_concave[MODULATOR_RESOLUTION] = 1;

	for (i = 1; i < MODULATOR_RESOLUTION; i++)
	{
		const float x =
		(((-200.0 * 2.0) / 960.0) * TSF_LOG10((float)i / (float)MODULATOR_RESOLUTION));
		tsf_convex[i] = 1.0 - x;
		tsf_concave[MODULATOR_RESOLUTION - i] = x;
	}

	for (i = 0; i < TSF_COUNTOF(CONVEX_ATTACK); i++) {
		// This makes the db linear (I think)
		CONVEX_ATTACK[i] = tsf_modulator_get_curve_value(
			0,
			curveConvex,
			(float)i / (float)TSF_COUNTOF(CONVEX_ATTACK)
		);
	}

	for (i = 0; i < TSF_COUNTOF(tsf_timecentLookupTable); i++)
	{
		const double timecents = (double)(MIN_TIMECENT + i);
		tsf_timecentLookupTable[i] = TSF_POW(2.0, timecents / 1200.0);
	}

	for (i = 0; i < TSF_COUNTOF(tsf_decibelLookupTable); i++)
	{
		const double decibels = (double)(MIN_DECIBELS * 100 + i) / 100.0;
		tsf_decibelLookupTable[i] = TSF_POW(10.0, -decibels / 20.0);
	}

	for (curveType = 0; curveType < MOD_CURVE_TYPES_AMOUNT; curveType++)
	{
		for (transformType = 0; transformType < MOD_SOURCE_TRANSFORM_POSSIBILITIES; transformType++)
		{
			int tableIndex =
				MODULATOR_RESOLUTION *
				(curveType * MOD_CURVE_TYPES_AMOUNT + transformType);
			for (value = 0; value < MODULATOR_RESOLUTION; value++) {
				tsf_precomputed_transforms[tableIndex + value] = tsf_modulator_get_curve_value(
					transformType,
					(enum tsf_modulator_curve_type)curveType,
					(float)value / (float)MODULATOR_RESOLUTION
				);
			}
		}
	}

	tsf_curves_computed = TSF_TRUE;
}

static void tsf_modulator_source_setup(struct tsf_modulator_source *s, tsf_u8 index, enum tsf_modulator_curve_type curveType, tsf_u8 isCC, tsf_u8 isBipolar, tsf_u8 isNegative)
{
	s->isBipolar = isBipolar;
	s->isNegative = isNegative;
	s->index = index;
	s->isCC = isCC;
	s->curveType = curveType;
}

static void tsf_modulator_source_setup_from_source_enum(struct tsf_modulator_source *s, tsf_u16 sourceEnum)
{
	const tsf_u8 isBipolar = (sourceEnum >> 9) & 1;
	const tsf_u8 isNegative = (sourceEnum >> 8) & 1;
	const tsf_u8 isCC = (sourceEnum >> 7) & 1;
	const tsf_u8 index = sourceEnum & 127;
	const enum tsf_modulator_curve_type curveType = (enum tsf_modulator_curve_type)((sourceEnum >> 10) & 3);
	tsf_modulator_source_setup(s, index, curveType, isCC, isBipolar, isNegative);
}

static tsf_u16 tsf_modulator_source_to_source_enum(const struct tsf_modulator_source *s)
{
	return (
			(((tsf_u8)s->curveType) << 10) |
			(s->isBipolar << 9) |
			(s->isNegative << 8) |
			(s->isCC << 7) |
			s->index
			);
}

static tsf_u16 tsf_modulator_source_parameters_to_source_enum(enum tsf_modulator_curve_type curveType, tsf_u8 isBipolar, tsf_u8 isNegative, tsf_u8 isCC, tsf_u8 index)
{
	struct tsf_modulator_source s;
	tsf_modulator_source_setup(&s, index, curveType, isCC, isBipolar, isNegative);
	return tsf_modulator_source_to_source_enum(&s);
}

static tsf_u16 tsf_default_resonant_mod_source;

struct tsf_modulator
{
	float currentValue;
	enum tsf_generator_type destination;
	tsf_u16 transformAmount;
	enum tsf_modulator_transform_type transformType;
	tsf_u8 isEffectModulator : 1;
	tsf_u8 isDefaultResonantModulator : 1;
	struct tsf_modulator_source primarySource;
	struct tsf_modulator_source secondarySource;
};

static void tsf_modulator_setup(struct tsf_modulator *m, const struct tsf_modulator_source *ps, const struct tsf_modulator_source *ss, enum tsf_generator_type destination, tsf_u16 amount, enum tsf_modulator_transform_type transformType, tsf_u8 isEffectModulator, tsf_u8 isDefaultResonantModulator)
{
	m->primarySource = *ps;
	m->secondarySource = *ss;

	m->destination = destination;
	m->transformAmount = amount;
	m->transformType = transformType;
	m->isEffectModulator = isEffectModulator;
	m->isDefaultResonantModulator = isDefaultResonantModulator;
}

static int tsf_modulators_copy(struct tsf_modulator **tt, const struct tsf_modulator *s, int modNum)
{
	struct tsf_modulator *t = (struct tsf_modulator *) TSF_MALLOC(modNum * sizeof(*t));
	if (!t) return 0;
	*tt = t;
	TSF_MEMCPY(t, s, modNum * sizeof(*t));
	return 1;
}

static int tsf_modulators_append(struct tsf_modulator **tt, int *modNum, const struct tsf_modulator *s, int sModNum)
{
	if (!s || !sModNum) return 1;
	struct tsf_modulator *t = (struct tsf_modulator *) TSF_REALLOC(*tt, (*modNum + sModNum) * sizeof(*t));
	if (!t) return 0;
	*tt = t;
	TSF_MEMCPY(t + *modNum, s, sModNum * sizeof(*t));
	*modNum += sModNum;
	return 1;
}

static void tsf_modulator_setup_decoded_modulator(struct tsf_modulator *m, tsf_u16 sourceEnum, tsf_u16 secondarySourceEnum, enum tsf_generator_type destination, tsf_u16 amount, tsf_u8 transformType)
{
	const tsf_u8 isEffectModulator =
		(sourceEnum == 0x00db || sourceEnum == 0x00dd) &&
		secondarySourceEnum == 0x0 &&
		(destination == reverbEffectsSend ||
		 destination == chorusEffectsSend);

	const tsf_u8 isDefaultResonantModulator =
		sourceEnum == tsf_default_resonant_mod_source &&
		secondarySourceEnum == 0x0 &&
		destination == initialFilterQ;

	struct tsf_modulator_source primarySource, secondarySource;

	tsf_modulator_source_setup_from_source_enum(&primarySource, sourceEnum);
	tsf_modulator_source_setup_from_source_enum(&secondarySource, secondarySourceEnum);

	tsf_modulator_setup(m, &primarySource, &secondarySource, destination, amount, (enum tsf_modulator_transform_type)transformType, isEffectModulator, isDefaultResonantModulator);

	if (m->destination > MAX_GENERATOR)
		m->destination = INVALID;
}

static const int DEFAULT_ATTENUATION_MOD_AMOUNT = 960;
static const int DEFAULT_ATTENUATION_MOD_CURVE_TYPE = curveConcave;

static int tsf_setup_default_modulators(struct tsf_modulator **mm)
{
	// 9 default standard modulators, plus 6 from SpessaSynth
	struct tsf_modulator *m = (struct tsf_modulator *) TSF_MALLOC(15 * sizeof(*m));
	if (!m) return 0;
	*mm = m;

	tsf_default_resonant_mod_source = tsf_modulator_source_parameters_to_source_enum(curveLinear, TSF_TRUE, TSF_FALSE, TSF_TRUE, filterResonance);

	// Vel to attenuation
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(DEFAULT_ATTENUATION_MOD_CURVE_TYPE, TSF_FALSE, TSF_TRUE, TSF_FALSE, noteOnVelocity), 0x0, initialAttenuation, DEFAULT_ATTENUATION_MOD_AMOUNT, 0);

	// Mod wheel to vibrato
	tsf_modulator_setup_decoded_modulator(m++, 0x0081, 0x0, vibLfoToPitch, 50, 0);

	// Vol to attenuation
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(DEFAULT_ATTENUATION_MOD_CURVE_TYPE, TSF_FALSE, TSF_TRUE, TSF_TRUE, mainVolume), 0x0, initialAttenuation, DEFAULT_ATTENUATION_MOD_AMOUNT, 0);

	// Channel pressure to vibrato
	tsf_modulator_setup_decoded_modulator(m++, 0x000d, 0x0, vibLfoToPitch, 50, 0);

	// Pitch wheel to tuning
	tsf_modulator_setup_decoded_modulator(m++, 0x020e, 0x0010, fineTune, 12700, 0);

	// Pan to uhh, pan
	// Amount is 500 instead of 1000
	tsf_modulator_setup_decoded_modulator(m++, 0x028a, 0x0, generatorPan, 500, 0);

	// Expression to attenuation
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(DEFAULT_ATTENUATION_MOD_CURVE_TYPE, TSF_FALSE, TSF_TRUE, TSF_TRUE, expressionController), 0x0, initialAttenuation, DEFAULT_ATTENUATION_MOD_AMOUNT, 0);

	// Custom modulators heck yeah
	// Poly pressure to vibrato
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(curveLinear, TSF_FALSE, TSF_FALSE, TSF_FALSE, polyPressure), 0x0, vibLfoToPitch, 50, 0);

	// Cc 92 (tremolo) to modLFO volume
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(curveLinear, TSF_FALSE, TSF_FALSE, TSF_TRUE, tremoloDepth) /*Linear forward unipolar cc 92*/, 0x0 /*No controller*/, modLfoToVolume, 24, 0);

	// Cc 73 (attack time) to volEnv attack
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(curveConvex, TSF_TRUE, TSF_FALSE, TSF_TRUE, attackTime) /*Linear forward bipolar cc 73*/, 0x0 /*No controller*/, attackVolEnv, 6000, 0);

	// Cc 72 (release time) to volEnv release
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(curveLinear, TSF_TRUE, TSF_FALSE, TSF_TRUE, releaseTime) /*Linear forward bipolar cc 72*/, 0x0 /*No controller*/, releaseVolEnv, 3600, 0);

	// Cc 74 (brightness) to filterFc
	tsf_modulator_setup_decoded_modulator(m++, tsf_modulator_source_parameters_to_source_enum(curveLinear, TSF_TRUE, TSF_FALSE, TSF_TRUE, brightness) /*Linear forwards bipolar cc 74*/, 0x0 /*No controller*/, initialFilterFc, 6000, 0);

	// Cc 71 (filter Q) to filter Q (default resonant modulator)
	tsf_modulator_setup_decoded_modulator(m++, tsf_default_resonant_mod_source, 0x0 /*No controller*/, initialFilterQ, 250, 0);

	return m - *mm;
}

static int tsf_read_modulators(struct tsf_modulator **mm, const struct tsf_hydra_pmod *pmods, int modNum)
{
	struct tsf_modulator *m = (struct tsf_modulator *) TSF_MALLOC(modNum * sizeof(*m));
	if (!m) return 0;
	*mm = m;

	int i;
	for (i = 0; i < modNum; i++, pmods++)
	{
		tsf_modulator_setup_decoded_modulator(m++, pmods->modSrcOper, pmods->modAmtSrcOper, (enum tsf_generator_type) pmods->modDestOper, pmods->modAmount, pmods->modTransOper);
	}
	
	return modNum;
}

struct tsf_voice
{
	const struct tsf_soundbank* soundbank;
	int playingPreset, playingKey, playingVelocity, playingChannel, pressure, heldSustain;
	struct tsf_preset *preset;
	struct tsf_region region;
	double pitchInputTimecents, pitchOutputFactor;
	double sourceSamplePosition;
	float  noteGainDB, panFactorLeft, panFactorRight, reverbEffectsSend, chorusEffectsSend, resonanceOffset;
	unsigned int playIndex, loopStart, loopEnd;
	struct tsf_volume_envelope ampenv;
	struct tsf_modulation_envelope modenv;
	struct tsf_voice_lowpass lowpass;
	struct tsf_voice_lfo modlfo, viblfo;
	struct tsf_generators generators;
	struct tsf_generators modulatedGenerators;
	struct tsf_modulator *modulators;
	int modNum;
	float voiceTime;
};

struct tsf_channel
{
	const struct tsf_soundbank* soundbank;
	unsigned short presetIndex, bank, pitchWheel, midiPan, midiVolume, midiExpression, pressure, midiRPN, midiData : 14, sustain : 1, reverb : 7, chorus : 7;
	float panOffset, gainDB, pitchRange, tuning;
	tsf_u16 controllers[CONTROLLER_TABLE_SIZE];
};

struct tsf_channels
{
	void (*setupVoice)(tsf* f, struct tsf_voice* voice);
	int channelNum, activeChannel;
	struct tsf_channel channels[1];
};

static float tsf_modulator_source_get_value(const struct tsf_modulator_source *s, const struct tsf_channel *c, const struct tsf_voice *v)
{
	const tsf_u16 *midiControllers = c->controllers;
	tsf_u16 rawValue;
	if (s->isCC) {
		if (s->index < 64)
		{
			rawValue = midiControllers[s->index & 31];
		}
		else
		{
			rawValue = midiControllers[s->index];
		}
	} else {
		switch (s->index) {
			case noController:
				rawValue = 16383; // Equals to 1
				break;

			case noteOnKeyNum:
				rawValue = v->playingKey << 7;
				break;

			case noteOnVelocity:
				rawValue = v->playingVelocity << 7;
				break;

			case polyPressure:
				rawValue = v->pressure << 7;
				break;

			case channelPressure:
				rawValue = c->pressure << 7;
				break;

			case pitchWheelRange:
				rawValue = (int)(TSF_FMOD(c->pitchRange, 1.0) * 100.0) + ((int)c->pitchRange) * 128;
				break;

			default:
				if ((s->index + NON_CC_INDEX_OFFSET) >= CONTROLLER_TABLE_SIZE)
					rawValue = 0;
				else
					rawValue = midiControllers[s->index + NON_CC_INDEX_OFFSET];
				break;
		}
	}

	rawValue = TSF_MIN(16383, rawValue);

	// Transform the value
	// 2-bit number as in 0bPD
	const int transformType =
		(s->isBipolar ? 2 : 0) | (s->isNegative ? 1 : 0);

	return tsf_precomputed_transforms[
		(MODULATOR_RESOLUTION *
			(((tsf_u8)s->curveType) * MOD_CURVE_TYPES_AMOUNT + transformType)) +
		 rawValue
	];
}

static const int EFFECT_MODULATOR_TRANSFORM_MULTIPLIER = 1000 / 200;

static float tsf_modulator_compute(struct tsf_modulator *m, const struct tsf_channel *c, struct tsf_voice *v)
{
	if (m->transformAmount == 0) {
		m->currentValue = 0;
		return 0;
	}
	const float sourceValue = tsf_modulator_source_get_value(&m->primarySource, c, v);
	const float secondSrcValue = tsf_modulator_source_get_value(&m->secondarySource, c, v);

	tsf_u16 transformAmount = m->transformAmount;
	if (m->isEffectModulator && transformAmount <= 1000)
	{
		transformAmount *= EFFECT_MODULATOR_TRANSFORM_MULTIPLIER;
		transformAmount = TSF_MIN(transformAmount, 1000);
	}

	// Compute the modulator
	float computedValue = sourceValue * secondSrcValue * transformAmount;

	if (m->transformType == transformAbsolute)
	{
		// Abs value
		computedValue = TSF_FABS(computedValue);
	}

	// Resonant modulator: take its value and ensure that it won't change the final gain
	if (m->isDefaultResonantModulator)
	{
		v->resonanceOffset = TSF_MAX(0.0, computedValue / 2.0);
	}

	m->currentValue = computedValue;
	return computedValue;
}

static double tsf_timecents2Secsd(double timecents) { if (timecents < MIN_TIMECENT) return 0.0; else if (timecents > MAX_TIMECENT) timecents = MAX_TIMECENT; return tsf_timecentLookupTable[(int)(timecents) - MIN_TIMECENT]; }
static float tsf_timecents2Secsf(float timecents) { return (float) tsf_timecents2Secsd((double)timecents); }
static float tsf_timecents2Samples(float timecents, float sampleRate) { float outputValue = TSF_FLOOR(tsf_timecents2Secsd(timecents) * sampleRate); return TSF_MAX(0, outputValue); }
static float tsf_cents2Hertz(float cents) { return (float)(8.176 * tsf_timecents2Secsd((double)cents)); }
static double tsf_decibelsToGain(double db) { if (db > (double)MAX_DECIBELS) return 0.0; else if (db < (double)MIN_DECIBELS) db = (double)MIN_DECIBELS; return tsf_decibelLookupTable[(int)TSF_FLOOR((db - MIN_DECIBELS) * 100.0)]; }
static float tsf_gainToDecibels(float gain) { return (gain <= .00001f ? -100.f : (float)(20.0 * (double)TSF_LOG10(gain))); }

static TSF_BOOL tsf_riffchunk_read(struct tsf_riffchunk* parent, struct tsf_riffchunk* chunk, struct tsf_stream* stream, int* riff64)
{
	TSF_BOOL IsRiff, IsRiff64, IsList;
	if (parent && sizeof(tsf_fourcc) > parent->size) return TSF_FALSE;
	if (!tsf_read_endian(stream, &chunk->id, sizeof(tsf_fourcc)) || *chunk->id <= ' ' || *chunk->id >= 'z') return TSF_FALSE;
	IsRiff = TSF_FourCCEquals(chunk->id, "RIFF");
	IsRiff64 = TSF_FourCCEquals(chunk->id, "RIFS");
	if (IsRiff64) *riff64 = 1;
	const int sizeSize = *riff64 ? sizeof(tsf_u64) : sizeof(tsf_u32);
	if (parent && sizeof(tsf_fourcc) + sizeSize > parent->size) return TSF_FALSE;
	if (!(chunk->size = tsf_read_riff_size(stream, *riff64))) return TSF_FALSE;
	if (chunk->size & 1) ++chunk->size;
	if (parent && sizeof(tsf_fourcc) + sizeSize + chunk->size > parent->size) return TSF_FALSE;
	if (parent) parent->size -= (tsf_u32)(sizeof(tsf_fourcc) + sizeSize + chunk->size);
	IsList = TSF_FourCCEquals(chunk->id, "LIST");
	if ((IsRiff || IsRiff64) && parent) return TSF_FALSE; //not allowed
	if (!IsRiff && !IsRiff64 && !IsList) return TSF_TRUE; //custom type without sub type
	if (!tsf_read_endian(stream, &chunk->id, sizeof(tsf_fourcc)) || *chunk->id <= ' ' || *chunk->id >= 'z') return TSF_FALSE;
	chunk->size -= (tsf_u32)(sizeof(tsf_fourcc));
	return TSF_TRUE;
}

static float tsf_region_operator(struct tsf_region* region, tsf_u16 genOper, union tsf_hydra_genamount* amount, struct tsf_region* merge_region, struct tsf_generators *generators, TSF_BOOL scale)
{
	enum
	{
		_GEN_TYPE_MASK       = 0x0F,
		GEN_FLOAT            = 0x01,
		GEN_INT              = 0x02,
		GEN_UINT_ADD         = 0x03,
		GEN_UINT_ADD15       = 0x04,
		GEN_KEYRANGE         = 0x05,
		GEN_VELRANGE         = 0x06,
		GEN_LOOPMODE         = 0x07,
		GEN_GROUP            = 0x08,
		GEN_KEYCENTER        = 0x09,

		_GEN_LIMIT_MASK      = 0xF0,
		GEN_INT_LIMIT12K     = 0x10, //min -12000, max 12000
		GEN_INT_LIMITFC      = 0x20, //min 1500, max 13500
		GEN_INT_LIMITQ       = 0x30, //min 0, max 960
		GEN_INT_LIMIT960     = 0x40, //min -960, max 960
		GEN_INT_LIMIT16K4500 = 0x50, //min -16000, max 4500
		GEN_FLOAT_LIMIT12K5K = 0x60, //min -12000, max 5000
		GEN_FLOAT_LIMIT12K8K = 0x70, //min -12000, max 8000
		GEN_FLOAT_LIMIT1200  = 0x80, //min -1200, max 1200
		GEN_FLOAT_LIMITPAN   = 0x90, //* .001f, min -.5f, max .5f,
		GEN_FLOAT_LIMITATTN  = 0xA0, //* .1f, min 0, max 144.0
		GEN_FLOAT_MAX1000    = 0xB0, //min 0, max 1000
		GEN_FLOAT_MAX1440    = 0xC0, //min 0, max 1440
		GEN_FLOAT_LIMIT32K5K = 0xD0, //min -32768, max 5000
		GEN_FLOAT_LIMIT32K8K = 0xE0, //min -32768, max 8000

		_GEN_MAX = 59
	};
#ifndef TSF_NO_STDDEF
	#define _TSFREGIONOFFSET(TYPE, FIELD) (unsigned char)(offsetof(struct tsf_region, FIELD) / sizeof(TYPE))
	#define _TSFREGIONENVOFFSET(TYPE, ENV, FIELD) (unsigned char)((offsetof(struct tsf_region, ENV) + offsetof(struct tsf_envelope, FIELD)) / sizeof(TYPE))
#else
	#define _TSFREGIONOFFSET(TYPE, FIELD) (unsigned char)(((TYPE*)&((struct tsf_region*)TSF_NULL)->FIELD) - (TYPE*)TSF_NULL)
	#define _TSFREGIONENVOFFSET(TYPE, ENV, FIELD) (unsigned char)(((TYPE*)&((&(((struct tsf_region*)TSF_NULL)->ENV))->FIELD)) - (TYPE*)TSF_NULL)
#endif
	static const struct { unsigned char mode, offset; } genMetas[_GEN_MAX] =
	{
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, offset               ) }, // 0 StartAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, end                  ) }, // 1 EndAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, loop_start           ) }, // 2 StartloopAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(unsigned int, loop_end             ) }, // 3 EndloopAddrsOffset
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, offset               ) }, // 4 StartAddrsCoarseOffset
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int, modLfoToPitch        ) }, // 5 ModLfoToPitch
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int, vibLfoToPitch        ) }, // 6 VibLfoToPitch
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int, modEnvToPitch        ) }, // 7 ModEnvToPitch
		{ GEN_INT   | GEN_INT_LIMITFC      , _TSFREGIONOFFSET(         int, initialFilterFc      ) }, // 8 InitialFilterFc
		{ GEN_INT   | GEN_INT_LIMITQ       , _TSFREGIONOFFSET(         int, initialFilterQ       ) }, // 9 InitialFilterQ
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int, modLfoToFilterFc     ) }, //10 ModLfoToFilterFc
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int, modEnvToFilterFc     ) }, //11 ModEnvToFilterFc
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, end                  ) }, //12 EndAddrsCoarseOffset
		{ GEN_INT   | GEN_INT_LIMIT960     , _TSFREGIONOFFSET(         int, modLfoToVolume       ) }, //13 ModLfoToVolume
		{ 0                                , (0                                                  ) }, //   Unused
		{ GEN_FLOAT | GEN_FLOAT_MAX1000    , _TSFREGIONOFFSET(       float, chorus               ) }, //15 ChorusEffectsSend
		{ GEN_FLOAT | GEN_FLOAT_MAX1000    , _TSFREGIONOFFSET(       float, reverb               ) }, //16 ReverbEffectsSend
		{ GEN_FLOAT | GEN_FLOAT_LIMITPAN   , _TSFREGIONOFFSET(       float, pan                  ) }, //17 Pan
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //   Unused
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONOFFSET(       float, delayModLFO          ) }, //21 DelayModLFO
		{ GEN_INT   | GEN_INT_LIMIT16K4500 , _TSFREGIONOFFSET(         int, freqModLFO           ) }, //22 FreqModLFO
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONOFFSET(       float, delayVibLFO          ) }, //23 DelayVibLFO
		{ GEN_INT   | GEN_INT_LIMIT16K4500 , _TSFREGIONOFFSET(         int, freqVibLFO           ) }, //24 FreqVibLFO
		{ GEN_FLOAT | GEN_FLOAT_LIMIT32K5K , _TSFREGIONENVOFFSET(    float, modenv, delay        ) }, //25 DelayModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT32K8K , _TSFREGIONENVOFFSET(    float, modenv, attack       ) }, //26 AttackModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, modenv, hold         ) }, //27 HoldModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, modenv, decay        ) }, //28 DecayModEnv
		{ GEN_FLOAT | GEN_FLOAT_MAX1000    , _TSFREGIONENVOFFSET(    float, modenv, sustain      ) }, //29 SustainModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, modenv, release      ) }, //30 ReleaseModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, modenv, keynumToHold ) }, //31 KeynumToModEnvHold
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, modenv, keynumToDecay) }, //32 KeynumToModEnvDecay
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, ampenv, delay        ) }, //33 DelayVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, attack       ) }, //34 AttackVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, ampenv, hold         ) }, //35 HoldVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, decay        ) }, //36 DecayVolEnv
		{ GEN_FLOAT | GEN_FLOAT_MAX1440    , _TSFREGIONENVOFFSET(    float, ampenv, sustain      ) }, //37 SustainVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, release      ) }, //38 ReleaseVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, ampenv, keynumToHold ) }, //39 KeynumToVolEnvHold
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, ampenv, keynumToDecay) }, //40 KeynumToVolEnvDecay
		{ 0                                , (0                                                  ) }, //   Instrument (special)
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_KEYRANGE                     , (0                                                  ) }, //43 KeyRange
		{ GEN_VELRANGE                     , (0                                                  ) }, //44 VelRange
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, loop_start           ) }, //45 StartloopAddrsCoarseOffset
		{ 0                                , (0                                                  ) }, //46 Keynum (special)
		{ 0                                , (0                                                  ) }, //47 Velocity (special)
		{ GEN_FLOAT | GEN_FLOAT_LIMITATTN  , _TSFREGIONOFFSET(       float, attenuation          ) }, //48 InitialAttenuation
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(unsigned int, loop_end             ) }, //50 EndloopAddrsCoarseOffset
		{ GEN_INT                          , _TSFREGIONOFFSET(         int, transpose            ) }, //51 CoarseTune
		{ GEN_INT                          , _TSFREGIONOFFSET(         int, tune                 ) }, //52 FineTune
		{ 0                                , (0                                                  ) }, //   SampleID (special)
		{ GEN_LOOPMODE                     , _TSFREGIONOFFSET(         int, loop_mode            ) }, //54 SampleModes
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_INT                          , _TSFREGIONOFFSET(         int, pitch_keytrack       ) }, //56 ScaleTuning
		{ GEN_GROUP                        , _TSFREGIONOFFSET(unsigned int, group                ) }, //57 ExclusiveClass
		{ GEN_KEYCENTER                    , _TSFREGIONOFFSET(         int, pitch_keycenter      ) }, //58 OverridingRootKey
	};
	#undef _TSFREGIONOFFSET
	#undef _TSFREGIONENVOFFSET
	if (amount)
	{
		int offset;
		if (genOper >= _GEN_MAX) return 0;
		if (!generators) generators = &region->generators;
		generators->g[genOper] = amount->wordAmount;
		offset = genMetas[genOper].offset;
		switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
		{
			case GEN_FLOAT:      ((       float*)region)[offset]  = amount->shortAmount;     break;
			case GEN_INT:        ((         int*)region)[offset]  = amount->shortAmount;     break;
			case GEN_UINT_ADD:   ((unsigned int*)region)[offset] += (unsigned)amount->shortAmount;         return amount->wordAmount;
			case GEN_UINT_ADD15: ((unsigned int*)region)[offset] += (unsigned)amount->shortAmount * 32768; return ((unsigned int*)region)[offset];
			case GEN_KEYRANGE:   region->lokey = amount->range.lo; region->hikey = amount->range.hi; return amount->wordAmount;
			case GEN_VELRANGE:   region->lovel = amount->range.lo; region->hivel = amount->range.hi; return amount->wordAmount;
			case GEN_LOOPMODE:   region->loop_mode       = ((amount->wordAmount&3) == 3 ? TSF_LOOPMODE_SUSTAIN : ((amount->wordAmount&3) == 1 ? TSF_LOOPMODE_CONTINUOUS : TSF_LOOPMODE_NONE)); return amount->wordAmount;
			case GEN_GROUP:      region->group           = amount->wordAmount;  return amount->wordAmount;
			case GEN_KEYCENTER:  region->pitch_keycenter = amount->shortAmount; return amount->shortAmount;
		}
		if (!scale) return amount->shortAmount;
		switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
		{
			case GEN_FLOAT:
			{
				float *val = &((float*)region)[offset], vfactor, vmin, vmax;
				switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
				{
					case GEN_FLOAT_LIMIT12K5K: vfactor =   1.0f; vmin = -12000.0f; vmax = 5000.0f; break;
					case GEN_FLOAT_LIMIT32K5K: vfactor =   1.0f; vmin = -32768.0f; vmax = 5000.0f; break;
					case GEN_FLOAT_LIMIT12K8K: vfactor =   1.0f; vmin = -12000.0f; vmax = 8000.0f; break;
					case GEN_FLOAT_LIMIT32K8K: vfactor =   1.0f; vmin = -32768.0f; vmax = 8000.0f; break;
					case GEN_FLOAT_LIMIT1200:  vfactor =   1.0f; vmin =  -1200.0f; vmax = 1200.0f; break;
					case GEN_FLOAT_LIMITPAN:   vfactor = 0.001f; vmin =     -0.5f; vmax =    0.5f; break;
					case GEN_FLOAT_LIMITATTN:  vfactor =   0.1f; vmin =      0.0f; vmax =  144.0f; break;
					case GEN_FLOAT_MAX1000:    vfactor =   1.0f; vmin =      0.0f; vmax = 1000.0f; break;
					case GEN_FLOAT_MAX1440:    vfactor =   1.0f; vmin =      0.0f; vmax = 1440.0f; break;
					default: return 0;
				}
				*val *= vfactor;
				if      (*val < vmin) *val = vmin;
				else if (*val > vmax) *val = vmax;
				return *val;
			}
			case GEN_INT:
			{
				int *val = &((int*)region)[offset], vmin, vmax;
				switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
				{
					case GEN_INT_LIMIT12K:     vmin = -12000; vmax = 12000; break;
					case GEN_INT_LIMITFC:      vmin =   1500; vmax = 13500; break;
					case GEN_INT_LIMITQ:       vmin =      0; vmax =   960; break;
					case GEN_INT_LIMIT960:     vmin =   -960; vmax =   960; break;
					case GEN_INT_LIMIT16K4500: vmin = -16000; vmax =  4500; break;
					default: return *val;
				}
				if      (*val < vmin) *val = vmin;
				else if (*val > vmax) *val = vmax;
				return *val;
			}
		}
	}
	else //merge regions and clamp values
	{
		for (genOper = 0; genOper != _GEN_MAX; genOper++)
		{
			int offset = genMetas[genOper].offset;
			switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
			{
				case GEN_FLOAT:
				{
					float *val = &((float*)region)[offset], vfactor, vmin, vmax;
					*val += ((float*)merge_region)[offset];
					switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
					{
						case GEN_FLOAT_LIMIT12K5K: vfactor =   1.0f; vmin = -12000.0f; vmax = 5000.0f; break;
						case GEN_FLOAT_LIMIT32K5K: vfactor =   1.0f; vmin = -32768.0f; vmax = 5000.0f; break;
						case GEN_FLOAT_LIMIT12K8K: vfactor =   1.0f; vmin = -12000.0f; vmax = 8000.0f; break;
						case GEN_FLOAT_LIMIT32K8K: vfactor =   1.0f; vmin = -32768.0f; vmax = 8000.0f; break;
						case GEN_FLOAT_LIMIT1200:  vfactor =   1.0f; vmin =  -1200.0f; vmax = 1200.0f; break;
						case GEN_FLOAT_LIMITPAN:   vfactor = 0.001f; vmin =     -0.5f; vmax =    0.5f; break;
						case GEN_FLOAT_LIMITATTN:  vfactor =   0.1f; vmin =      0.0f; vmax =  144.0f; break;
						case GEN_FLOAT_MAX1000:    vfactor =   1.0f; vmin =      0.0f; vmax = 1000.0f; break;
						case GEN_FLOAT_MAX1440:    vfactor =   1.0f; vmin =      0.0f; vmax = 1440.0f; break;
						default: continue;
					}
					*val *= vfactor;
					if      (*val < vmin) *val = vmin;
					else if (*val > vmax) *val = vmax;
					continue;
				}
				case GEN_INT:
				{
					int *val = &((int*)region)[offset], vmin, vmax;
					*val += ((int*)merge_region)[offset];
					switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
					{
						case GEN_INT_LIMIT12K:     vmin = -12000; vmax = 12000; break;
						case GEN_INT_LIMITFC:      vmin =   1500; vmax = 13500; break;
						case GEN_INT_LIMITQ:       vmin =      0; vmax =   960; break;
						case GEN_INT_LIMIT960:     vmin =   -960; vmax =   960; break;
						case GEN_INT_LIMIT16K4500: vmin = -16000; vmax =  4500; break;
						default: continue;
					}
					if      (*val < vmin) *val = vmin;
					else if (*val > vmax) *val = vmax;
					continue;
				}
				case GEN_UINT_ADD:
				{
					((unsigned int*)region)[offset] += ((unsigned int*)merge_region)[offset];
					continue;
				}
			}
		}
	}
	return 0;
}

static void tsf_region_clear(struct tsf_region* i, TSF_BOOL for_relative)
{
	union tsf_hydra_genamount amount;
	TSF_MEMSET(i, 0, sizeof(struct tsf_region));
	amount.range.lo = 0; amount.range.hi = 127;
	tsf_region_operator(i, keyRange, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, velRange, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	amount.shortAmount = 60; // C4;
	tsf_region_operator(i, overridingRootKey, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	if (for_relative) return;

	amount.shortAmount = 100;
	tsf_region_operator(i, scaleTuning, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);

	amount.shortAmount = -1;
	tsf_region_operator(i, overridingRootKey, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);

	// SF2 defaults in timecents.
	amount.shortAmount = -12000;
	tsf_region_operator(i, delayVolEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, attackVolEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, holdVolEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, decayVolEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, releaseVolEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);

	tsf_region_operator(i, holdModEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, decayModEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, releaseModEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);

	// -32768 = instant, done to prevent clicks with the lowpass filter
	amount.shortAmount = -32768;
	tsf_region_operator(i, delayModEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, attackModEnv, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);

	amount.shortAmount = 13500;
	tsf_region_operator(i, initialFilterFc, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);

	amount.shortAmount = -12000;
	tsf_region_operator(i, delayModLFO, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
	tsf_region_operator(i, delayVibLFO, &amount, TSF_NULL, TSF_NULL, TSF_TRUE);
}

static int tsf_soundbank_load_presets(tsf_soundbank* res, struct tsf_hydra *hydra, struct tsf_hydra *xydra, unsigned int fontSampleCount)
{
	enum { GenInstrument = 41, GenKeyRange = 43, GenVelRange = 44, GenSampleID = 53 };
	// Read each preset.
	struct tsf_hydra_phdr *pphdr, *pphdrMax, *pxphdr;
	res->presetNum = hydra->phdrNum - 1;
	res->presets = (struct tsf_preset*)TSF_MALLOC(res->presetNum * sizeof(struct tsf_preset));
	if (!res->presets) return 0;
	else { int i; for (i = 0; i != res->presetNum; i++) { res->presets[i].regions = TSF_NULL; res->presets[i].modulators = TSF_NULL; } }
	for (pphdr = hydra->phdrs, pphdrMax = pphdr + hydra->phdrNum - 1, pxphdr = xydra->phdrs; pphdr != pphdrMax; pphdr++, pxphdr += !!pxphdr)
	{
		int sortedIndex = 0, region_index = 0;
		struct tsf_hydra_phdr *otherphdr, *otherxphdr;
		struct tsf_preset* preset;
		struct tsf_hydra_pbag *ppbag, *ppbagEnd, *pxpbag;
		struct tsf_region globalRegion;
		for (otherphdr = hydra->phdrs, otherxphdr = xydra->phdrs; otherphdr != pphdrMax; otherphdr++, otherxphdr += !!otherxphdr)
		{
			if (otherphdr == pphdr || tsf_hydra_phdr_read_bank(otherphdr, otherxphdr) > tsf_hydra_phdr_read_bank(pphdr, pxphdr)) continue;
			else if (tsf_hydra_phdr_read_bank(otherphdr, otherxphdr) < tsf_hydra_phdr_read_bank(pphdr, pxphdr)) sortedIndex++;
			else if (tsf_hydra_phdr_read_preset(otherphdr, otherxphdr) > tsf_hydra_phdr_read_preset(pphdr, pxphdr)) continue;
			else if (tsf_hydra_phdr_read_preset(otherphdr, otherxphdr) < tsf_hydra_phdr_read_preset(pphdr, pxphdr)) sortedIndex++;
			else if (otherphdr < pphdr) sortedIndex++;
		}

		preset = &res->presets[sortedIndex];
		TSF_MEMCPY(preset->presetName, pphdr->presetName, sizeof(pphdr->presetName));
		if (pxphdr)
		{
			TSF_MEMCPY(preset->presetName + 20, pxphdr->presetName, sizeof(pxphdr->presetName));
			preset->presetName[sizeof(preset->presetName)-1] = '\0'; //should be zero terminated in source file but make sure
		}
		else
			preset->presetName[sizeof(pphdr->presetName)] = '\0';
		preset->bank = tsf_hydra_phdr_read_bank(pphdr, pxphdr);
		preset->preset = tsf_hydra_phdr_read_preset(pphdr, pxphdr);
		preset->regionNum = 0;

		//count regions covered by this preset
		for (ppbag = hydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr, pxphdr), ppbagEnd = hydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr + 1, pxphdr + !!pxphdr), pxpbag = xydra->pbags ? xydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr, pxphdr) : TSF_NULL; ppbag != ppbagEnd; ppbag++, pxpbag += !!pxpbag)
		{
			tsf_u16 instBagNdx, nextInstBagNdx;
			unsigned char plokey = 0, phikey = 127, plovel = 0, phivel = 127;
			struct tsf_hydra_pgen *ppgen, *ppgenEnd; struct tsf_hydra_inst *pinst, *pxinst; struct tsf_hydra_ibag *pibag, *pibagEnd, *pxibag; struct tsf_hydra_igen *pigen, *pigenEnd;
			for (ppgen = hydra->pgens + tsf_hydra_pbag_read_genNdx(ppbag, pxpbag), ppgenEnd = hydra->pgens + tsf_hydra_pbag_read_genNdx(ppbag + 1, pxpbag + !!pxpbag); ppgen != ppgenEnd; ppgen++)
			{
				if (ppgen->genOper == GenKeyRange) { plokey = ppgen->genAmount.range.lo; phikey = ppgen->genAmount.range.hi; continue; }
				if (ppgen->genOper == GenVelRange) { plovel = ppgen->genAmount.range.lo; phivel = ppgen->genAmount.range.hi; continue; }
				if (ppgen->genOper != GenInstrument) continue;
				if (ppgen->genAmount.wordAmount >= hydra->instNum) continue;
				pinst = hydra->insts + ppgen->genAmount.wordAmount;
				pxinst = xydra->insts ? xydra->insts + ppgen->genAmount.wordAmount : TSF_NULL;
				for (instBagNdx = tsf_hydra_inst_read_instBagNdx(pinst, pxinst), nextInstBagNdx = tsf_hydra_inst_read_instBagNdx(pinst + 1, pxinst + !!pxinst), pibag = hydra->ibags + instBagNdx, pibagEnd = hydra->ibags + nextInstBagNdx, pxibag = xydra->ibags ? xydra->ibags + instBagNdx : TSF_NULL; pibag != pibagEnd; pibag++, pxibag += !!pxibag)
				{
					unsigned char ilokey = 0, ihikey = 127, ilovel = 0, ihivel = 127;
					for (pigen = hydra->igens + tsf_hydra_ibag_read_instGenNdx(pibag, pxibag), pigenEnd = hydra->igens + tsf_hydra_ibag_read_instGenNdx(pibag + 1, pxibag + !!pxibag); pigen != pigenEnd; pigen++)
					{
						if (pigen->genOper == GenKeyRange) { ilokey = pigen->genAmount.range.lo; ihikey = pigen->genAmount.range.hi; continue; }
						if (pigen->genOper == GenVelRange) { ilovel = pigen->genAmount.range.lo; ihivel = pigen->genAmount.range.hi; continue; }
						if (pigen->genOper == GenSampleID && ihikey >= plokey && ilokey <= phikey && ihivel >= plovel && ilovel <= phivel) preset->regionNum++;
					}
				}
			}
		}

		preset->regions = (struct tsf_region*)TSF_MALLOC(preset->regionNum * sizeof(struct tsf_region));
		if (!preset->regions)
		{
			int i; for (i = 0; i != res->presetNum; i++)
			{
				TSF_FREE(res->presets[i].regions);
				TSF_FREE(res->presets[i].modulators);
			}
			TSF_FREE(res->presets);
			return 0;
		}
		tsf_region_clear(&globalRegion, TSF_TRUE);

		// Zones.
		for (ppbag = hydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr, pxphdr), ppbagEnd = hydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr + 1, pxphdr + !!pxphdr), pxpbag = xydra->pbags ? xydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr, pxphdr) : TSF_NULL; ppbag != ppbagEnd; ppbag++, pxpbag += !!pxpbag)
		{
			struct tsf_hydra_pgen *ppgen, *ppgenEnd; struct tsf_hydra_inst *pinst, *pxinst; struct tsf_hydra_ibag *pibag, *pibagEnd, *pxibag; struct tsf_hydra_igen *pigen, *pigenEnd;
			struct tsf_region presetRegion = globalRegion;
			int hadGenInstrument = 0;
			tsf_u16 instBagNdx, nextInstBagNdx;
			int pModIdx, pModEnd;

			// Generators.
			for (ppgen = hydra->pgens + tsf_hydra_pbag_read_genNdx(ppbag, pxpbag), ppgenEnd = hydra->pgens + tsf_hydra_pbag_read_genNdx(ppbag + 1, pxpbag + !!pxpbag); ppgen != ppgenEnd; ppgen++)
			{
				// Instrument.
				if (ppgen->genOper == GenInstrument)
				{
					struct tsf_region instRegion;
					tsf_u16 whichInst = ppgen->genAmount.wordAmount;
					if (whichInst >= hydra->instNum) continue;

					tsf_region_clear(&instRegion, TSF_FALSE);
					pinst = &hydra->insts[whichInst];
					pxinst = xydra->insts ? &xydra->insts[whichInst] : TSF_NULL;
					for (instBagNdx = tsf_hydra_inst_read_instBagNdx(pinst, pxinst), nextInstBagNdx = tsf_hydra_inst_read_instBagNdx(pinst + 1, pxinst + !!pxinst), pibag = hydra->ibags + instBagNdx, pibagEnd = hydra->ibags + nextInstBagNdx, pxibag = xydra->ibags ? xydra->ibags + instBagNdx : TSF_NULL; pibag != pibagEnd; pibag++, pxibag += !!pxibag)
					{
						// Generators.
						struct tsf_region zoneRegion = instRegion;
						int hadSampleID = 0;
						int iModIdx, iModEnd;
						for (pigen = hydra->igens + tsf_hydra_ibag_read_instGenNdx(pibag, pxibag), pigenEnd = hydra->igens + tsf_hydra_ibag_read_instGenNdx(pibag + 1, pxibag + !!pxibag); pigen != pigenEnd; pigen++)
						{
							if (pigen->genOper == GenSampleID)
							{
								struct tsf_hydra_shdr* pshdr;

								//preset region key and vel ranges are a filter for the zone regions
								if (zoneRegion.hikey < presetRegion.lokey || zoneRegion.lokey > presetRegion.hikey) continue;
								if (zoneRegion.hivel < presetRegion.lovel || zoneRegion.lovel > presetRegion.hivel) continue;
								if (presetRegion.lokey > zoneRegion.lokey) zoneRegion.lokey = presetRegion.lokey;
								if (presetRegion.hikey < zoneRegion.hikey) zoneRegion.hikey = presetRegion.hikey;
								if (presetRegion.lovel > zoneRegion.lovel) zoneRegion.lovel = presetRegion.lovel;
								if (presetRegion.hivel < zoneRegion.hivel) zoneRegion.hivel = presetRegion.hivel;

								//sum regions
								tsf_region_operator(&zoneRegion, 0, TSF_NULL, &presetRegion, TSF_NULL, TSF_FALSE);

								// LFO times need to be converted from timecents to seconds.
								zoneRegion.delayModLFO = (zoneRegion.delayModLFO < -11950.0f ? 0.0f : tsf_timecents2Secsf(zoneRegion.delayModLFO));
								zoneRegion.delayVibLFO = (zoneRegion.delayVibLFO < -11950.0f ? 0.0f : tsf_timecents2Secsf(zoneRegion.delayVibLFO));

								// Fixup sample positions
								pshdr = &hydra->shdrs[pigen->genAmount.wordAmount];
								zoneRegion.offset += pshdr->start;
								zoneRegion.end += pshdr->end;
								zoneRegion.loop_start += pshdr->startLoop;
								zoneRegion.loop_end += pshdr->endLoop;
								if (pshdr->endLoop > 0) zoneRegion.loop_end -= 1;
								if (zoneRegion.loop_end > fontSampleCount) zoneRegion.loop_end = fontSampleCount;
								if (zoneRegion.pitch_keycenter == -1) zoneRegion.pitch_keycenter = pshdr->originalPitch;
								zoneRegion.tune += pshdr->pitchCorrection;
								zoneRegion.sample_rate = pshdr->sampleRate;
								if (zoneRegion.end && zoneRegion.end < fontSampleCount) zoneRegion.end++;
								else zoneRegion.end = fontSampleCount;

								preset->regions[region_index] = zoneRegion;
								if (zoneRegion.modulators &&
									!tsf_modulators_copy(&preset->regions[region_index].modulators, zoneRegion.modulators, zoneRegion.modNum))
									return 0;
								region_index++;
								hadSampleID = 1;
							}
							else tsf_region_operator(&zoneRegion, pigen->genOper, &pigen->genAmount, TSF_NULL, TSF_NULL, TSF_FALSE);
						}

						// Handle instrument's global zone.
						if (pibag == hydra->ibags + tsf_hydra_inst_read_instBagNdx(pinst, pxinst) && !hadSampleID)
						{
							if (instRegion.modulators != zoneRegion.modulators)
								TSF_FREE(instRegion.modulators);
							instRegion = zoneRegion;
						}

						// Modulators
						iModIdx = tsf_hydra_ibag_read_instModNdx(pibag, pxibag);
						iModEnd = tsf_hydra_ibag_read_instModNdx(pibag + 1, pxibag + !!pxibag);
						if (iModIdx < iModEnd)
						{
							TSF_FREE(instRegion.modulators); instRegion.modulators = TSF_NULL;
							const int iModNum = iModEnd - iModIdx;
							struct tsf_hydra_imod *imods = hydra->imods + iModIdx;
							instRegion.modNum = tsf_read_modulators(&instRegion.modulators, (struct tsf_hydra_pmod *)imods, iModNum);
						}
					}
					hadGenInstrument = 1;
					TSF_FREE(instRegion.modulators); instRegion.modulators = TSF_NULL;
				}
				else tsf_region_operator(&presetRegion, ppgen->genOper, &ppgen->genAmount, TSF_NULL, TSF_NULL, TSF_FALSE);
			}

			// Modulators
			pModIdx = tsf_hydra_pbag_read_modNdx(ppbag, pxpbag);
			pModEnd = tsf_hydra_pbag_read_modNdx(ppbag + 1, pxpbag + !!pxpbag);
			if (pModIdx < pModEnd)
			{
				const int pModNum = pModEnd - pModIdx;
				struct tsf_hydra_pmod *pmods = hydra->pmods + pModIdx;
				preset->modNum = tsf_read_modulators(&preset->modulators, pmods, pModNum);
			}

			// Handle preset's global zone.
			if (ppbag == hydra->pbags + tsf_hydra_phdr_read_presetBagNdx(pphdr, pxphdr) && !hadGenInstrument)
				globalRegion = presetRegion;
		}
	}
	return 1;
}

#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
static int tsf_decode_ogg(const tsf_u8 *pSmpl, const tsf_u8 *pSmplEnd, float** pRes, tsf_u32* pResNum, tsf_u32* pResMax, tsf_u32 resInitial)
{
	float *res = *pRes, *oldres; tsf_u32 resNum = *pResNum; tsf_u32 resMax = *pResMax; stb_vorbis *v;

	// Use whatever stb_vorbis API that is available (either pull or push)
	#if !defined(STB_VORBIS_NO_PULLDATA_API) && !defined(STB_VORBIS_NO_FROMMEMORY)
	v = stb_vorbis_open_memory(pSmpl, (int)(pSmplEnd - pSmpl), TSF_NULL, TSF_NULL);
	#else
	{ int use, err; v = stb_vorbis_open_pushdata(pSmpl, (int)(pSmplEnd - pSmpl), &use, &err, TSF_NULL); pSmpl += use; }
	#endif
	if (v == TSF_NULL) return 0;

	for (;;)
	{
		float** outputs; int n_samples;

		// Decode one frame of vorbis samples with whatever stb_vorbis API that is available
		#if !defined(STB_VORBIS_NO_PULLDATA_API) && !defined(STB_VORBIS_NO_FROMMEMORY)
		n_samples = stb_vorbis_get_frame_float(v, TSF_NULL, &outputs);
		if (!n_samples) break;
		#else
		if (pSmpl >= pSmplEnd) break;
		{ int use = stb_vorbis_decode_frame_pushdata(v, pSmpl, (int)(pSmplEnd - pSmpl), TSF_NULL, &outputs, &n_samples); pSmpl += use; }
		if (!n_samples) continue;
		#endif

		// Expand our output buffer if necessary then copy over the decoded frame samples
		resNum += n_samples;
		if (resNum > resMax)
		{
			do { resMax += (resMax ? (resMax < 1048576 ? resMax : 1048576) : resInitial); } while (resNum > resMax);
			oldres = res;
			res = (float*)TSF_REALLOC(res, resMax * sizeof(float));
			if (!res) { TSF_FREE(oldres); stb_vorbis_close(v); return 0; }
		}
		TSF_MEMCPY(res + resNum - n_samples, outputs[0], n_samples * sizeof(float));
	}
	stb_vorbis_close(v);
	*pRes = res; *pResNum = resNum; *pResMax = resMax;
	return 1;
}

static int tsf_decode_sf3_samples(const void* rawBuffer, float** pFloatBuffer, tsf_u64* pSmplCount, struct tsf_hydra *hydra)
{
	const tsf_u8* smplBuffer = (const tsf_u8*)rawBuffer;
	tsf_u64 smplLength = *pSmplCount, resNum = 0, resMax = 0, resInitial = (smplLength > 0x100000 ? (smplLength & ~0xFFFFF) : 65536);
	float *res = TSF_NULL, *oldres;
	int i, shdrLast = hydra->shdrNum - 1, is_sf3 = 0;
	for (i = 0; i <= shdrLast; i++)
	{
		struct tsf_hydra_shdr *shdr = &hydra->shdrs[i];
		if (shdr->sampleType & 0x30) // compression flags (sometimes Vorbis flag)
		{
			const tsf_u8 *pSmpl = smplBuffer + shdr->start, *pSmplEnd = smplBuffer + shdr->end;
			if (pSmpl + 4 > pSmplEnd || !TSF_FourCCEquals(pSmpl, "OggS"))
			{
				shdr->start = shdr->end = shdr->startLoop = shdr->endLoop = 0;
				continue;
			}

			// Fix up sample indices in shdr (end index is set after decoding)
			shdr->start = resNum;
			shdr->startLoop += resNum;
			shdr->endLoop += resNum;
			if (!tsf_decode_ogg(pSmpl, pSmplEnd, &res, &resNum, &resMax, resInitial)) { TSF_FREE(res); return 0; }
			shdr->end = resNum;
			is_sf3 = 1;
		}
		else // raw PCM sample
		{
			float *out; short *in = (short*)smplBuffer + resNum, *inEnd; tsf_u32 oldResNum = resNum;
			if (is_sf3) // Fix up sample indices in shdr
			{
				tsf_u32 fix_offset = resNum - shdr->start;
				in -= fix_offset;
				shdr->start = resNum;
				shdr->end += fix_offset;
				shdr->startLoop += fix_offset;
				shdr->endLoop += fix_offset;
			}
			inEnd = in + ((shdr->end >= shdr->endLoop ? shdr->end : shdr->endLoop) - resNum);
			if (i == shdrLast || (tsf_u8*)inEnd > (smplBuffer + smplLength)) inEnd = (short*)(smplBuffer + smplLength);
			if (inEnd <= in) continue;

			// expand our output buffer if necessary then convert the PCM data from short to float
			resNum += (tsf_u32)(inEnd - in);
			if (resNum > resMax)
			{
				do { resMax += (resMax ? (resMax < 1048576 ? resMax : 1048576) : resInitial); } while (resNum > resMax);
				oldres = res;
				res = (float*)TSF_REALLOC(res, resMax * sizeof(float));
				if (!res) { TSF_FREE(oldres); return 0; }
			}

			// Convert the samples from short to float
			if (!_tsf_big_endian)
			{
				for (out = res + oldResNum; in < inEnd;)
					*(out++) = (float)(*(in++) / 32768.0);
			}
			else
			{
				for (out = res + oldResNum; in < inEnd;)
				{
					tsf_u8 tmp;
					union { tsf_u8 bytes[2]; short word; } sample;
					sample.word = *(in++);
					tmp = sample.bytes[0]; sample.bytes[0] = sample.bytes[1]; sample.bytes[1] = tmp;
					*(out++) = (float)(sample.word / 32768.0);
				}
			}
		}
	}

	// Trim the sample buffer down then return success (unless out of memory)
	if (!(*pFloatBuffer = (float*)TSF_REALLOC(res, resNum * sizeof(float)))) *pFloatBuffer = res;
	*pSmplCount = resNum;
	return (res ? 1 : 0);
}
#endif

static int tsf_load_samples(void** pRawBuffer, float** pFloatBuffer, tsf_u64* pSmplCount, struct tsf_riffchunk *chunkSmpl, struct tsf_stream* stream)
{
	#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
	// With OGG Vorbis support we cannot pre-allocate the memory for tsf_decode_sf3_samples
	tsf_u64 resNum, resMax; float* oldres;
	*pSmplCount = chunkSmpl->size;
	*pRawBuffer = (void*)TSF_MALLOC(*pSmplCount);
	if (!*pRawBuffer || !stream->read(stream->data, *pRawBuffer, chunkSmpl->size)) return 0;
	if (chunkSmpl->id[3] != 'o') return 1;

	// Decode custom .sfo 'smpo' format where all samples are in a single ogg stream
	resNum = resMax = 0;
	if (!tsf_decode_ogg((tsf_u8*)*pRawBuffer, (tsf_u8*)*pRawBuffer + chunkSmpl->size, pFloatBuffer, &resNum, &resMax, 65536)) return 0;
	oldres = *pFloatBuffer;
	if (!(*pFloatBuffer = (float*)TSF_REALLOC(*pFloatBuffer, resNum * sizeof(float)))) *pFloatBuffer = oldres;
	*pSmplCount = resNum;
	return (*pFloatBuffer ? 1 : 0);
	#else
	(void)pRawBuffer;
	// Inline convert the samples from short to float
	float *res, *out; const short *in;
	(void)pRawBuffer;
	*pSmplCount = chunkSmpl->size / sizeof(short);
	*pFloatBuffer = (float*)TSF_MALLOC(*pSmplCount * sizeof(float));
	if (!*pFloatBuffer || !stream->read(stream->data, *pFloatBuffer, chunkSmpl->size)) return 0;
	if (!_tsf_big_endian)
	{
		for (res = *pFloatBuffer, out = res + *pSmplCount, in = (short*)res + *pSmplCount; out != res;)
			*(--out) = (float)(*(--in) / 32768.0);
	}
	else
	{
		for (res = *pFloatBuffer, out = res + *pSmplCount, in = (short*)res + *pSmplCount; out != res;)
		{
			tsf_u8 tmp;
			union { tsf_u8 bytes[2]; short word; } sample;
			sample.word = *(--in);
			tmp = sample.bytes[0]; sample.bytes[0] = sample.bytes[1]; sample.bytes[1] = tmp;
			*(--out) = (float)(sample.word / 32768.0);
		}
	}
	return 1;
	#endif
}

static int tsf_load_samples_24(void* pRawBuffer, void** pRaw24Buffer, float** pFloatBuffer, tsf_u64* pSmplCount, struct tsf_riffchunk *chunkSm24, struct tsf_stream* stream)
{
	tsf_u64 resMax;
	#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
	resMax = *pSmplCount / (unsigned int)sizeof(short);
	#else
	resMax = *pSmplCount;
	#endif
	tsf_u64 maxSm24Count = TSF_MIN(resMax, chunkSm24->size);
	float *out, *outEnd; const short *in; const unsigned char *in24;

	if (!pRawBuffer) return 0;
	*pRaw24Buffer = (void*)TSF_MALLOC(chunkSm24->size);
	if (!*pRaw24Buffer || !stream->read(stream->data, *pRaw24Buffer, chunkSm24->size)) return 0;

	if (!*pFloatBuffer)
	{
		// Due to how the STB Vorbis may or may not be imported, the samples
		// may not already be converted to float
		*pFloatBuffer = (float *) TSF_MALLOC(resMax * sizeof(float));
		if (!*pFloatBuffer) return 0;
		for (out = *pFloatBuffer, outEnd = out + maxSm24Count, in = (short *)pRawBuffer, in24 = (unsigned char *)*pRaw24Buffer; out != outEnd;)
		{
			*out++ = (((int)(*in++) * 256) + (*in24++)) / 16777216.0;
		}
		if (resMax > maxSm24Count)
			for (outEnd = out + resMax; out != outEnd;)
			{
				*out++ = (*in++) / 32768.0;
			}
	}
	else
	{
		// Previous function already converted the samples, so add on the fractions
		for (out = *pFloatBuffer, outEnd = out + maxSm24Count, in24 = (unsigned char *)pRaw24Buffer; out != outEnd;)
		{
			*out++ += *in24++ / 16777216.0;
		}
	}
	*pSmplCount = resMax;
	return 1;
}

static const float VOLUME_ENVELOPE_SMOOTHING_FACTOR = 0.01;

static const float DB_SILENCE = 100.0;
static const float PERCEIVED_DB_SILENCE = 90.0;
// Around 96 dB of attenuation
static const float PERCEIVED_GAIN_SILENCE = 0.000015; // Can't go lower than that

static int tsf_volume_envelope_release_samples(const struct tsf_volume_envelope *e)
{
	return e->currentSampleTime - e->releaseStartDb;
}

static void tsf_volume_envelope_recalculate(struct tsf_voice *v, float outSampleRate)
{
	struct tsf_volume_envelope* e = &v->ampenv;
	e->parameters = v->region.ampenv;

	// construct based on original inline defaults
	if (e->state == TSF_SEGMENT_NONE)
	{
		e->inRelease = TSF_FALSE;
		e->currentAttenuationDb = DB_SILENCE;
		e->attenuation = 0;
		e->attenuationTargetGain = 0;
		e->currentSampleTime = 0;
		e->releaseStartDb = DB_SILENCE;
		e->releaseStartTimeSamples = 0;
		e->currentReleaseGain = 1;
		e->attackDuration = 0;
		e->decayDuration = 0;
		e->releaseDuration = 0;
		e->attenuationTarget = 0;
		e->sustainDbRelative = 0;
		e->delayEnd = 0;
		e->attackEnd = 0;
		e->holdEnd = 0;
		e->decayEnd = 0;
		e->state = TSF_SEGMENT_DELAY;
	}

	e->attenuationTarget = v->region.attenuation;
	e->attenuationTargetGain = tsf_decibelsToGain(e->attenuationTarget);
	e->sustainDbRelative = TSF_MIN(DB_SILENCE, e->parameters.sustain / 10.0);
	const float sustainDb = TSF_MIN(DB_SILENCE, e->sustainDbRelative);

	e->canEndOnSilentSustain = e->parameters.sustain / 10.0 >= PERCEIVED_DB_SILENCE;

	// Calculate durations
	e->attackDuration = tsf_timecents2Samples(e->parameters.attack, outSampleRate);
	
	// Decay: sf spec page 35: the time is for change from attenuation to -100dB
	// Therefore, we need to calculate the real time
	// (changing from attenuation to sustain instead of -100dB)
	const float fullChange = e->parameters.decay;
	const float keyNumAddition = (60.0 - (float)v->playingKey) * e->parameters.keynumToDecay;
	const float fraction = sustainDb / DB_SILENCE;
	e->decayDuration = tsf_timecents2Samples(fullChange + keyNumAddition, outSampleRate) * fraction;

	// Min is set to -7200 prevent clicks
	e->releaseDuration = tsf_timecents2Samples(TSF_MAX(-7200.0, e->parameters.release), outSampleRate);

	// Calculate absolute end times for the values
	e->delayEnd = tsf_timecents2Samples(e->parameters.delay, outSampleRate);
	e->attackEnd = e->attackDuration + e->delayEnd;

	// Make sure to take keyNumToVolEnvHold into account!
	const float holdExcursion = (60.0 - v->playingKey) * e->parameters.keynumToHold;
	e->holdEnd = tsf_timecents2Samples(e->parameters.hold + holdExcursion, outSampleRate) + e->attackEnd;

	e->decayEnd = e->decayDuration + e->holdEnd;
	
	// If this is the first recalculation and the voice has no attack or delay time, set current dB to peak
	if (e->state == TSF_SEGMENT_DELAY && e->attackEnd == 0)
	{
		// currentAttenuationDb = attenuationTarget
		e->state = TSF_SEGMENT_HOLD;
	}

	// Check if voice is in release
	if (e->inRelease)
	{
		// No interpolation this time: force update to actual attenuation and calculate release start from there
		// attenuation = TSF_MIN(DB_SILENCE, attenuationTarget)
		const float sustainDb = TSF_MAX(0, TSF_MIN(DB_SILENCE, e->sustainDbRelative));
		const float fraction = sustainDb / DB_SILENCE;
		e->decayDuration = tsf_timecents2Samples(fullChange + keyNumAddition, outSampleRate) * fraction;

		switch (e->state)
		{
			case TSF_SEGMENT_NONE:
			case TSF_SEGMENT_DELAY:
				e->releaseStartDb = DB_SILENCE;
				break;
				
			case TSF_SEGMENT_ATTACK: {
				// Attack phase: get linear gain of the attack phase when release started
				// And turn it into db as we're ramping the db up linearly
				// (to make volume go down exponentially)
				// Attack is linear (in gain) so we need to do get db from that
				const float elapsed = 1.0 - (float)(e->attackEnd - e->releaseStartTimeSamples) / (float)e->attackDuration;
				// Calculate the gain that the attack would have, so
				// Turn that into db
				e->releaseStartDb = 20.0 * TSF_LOG10(elapsed) * -1.0;
				break;
			}
				
			case TSF_SEGMENT_HOLD:
				e->releaseStartDb = 0;
				break;
				
			case TSF_SEGMENT_DECAY:
				e->releaseStartDb = (1.0 - (float)(e->decayEnd - e->releaseStartTimeSamples) / (float)e->decayDuration) * sustainDb;
				break;
				
			case TSF_SEGMENT_SUSTAIN:
				e->releaseStartDb = sustainDb;
				break;
		}
		e->releaseStartDb = TSF_MAX(0.0, TSF_MIN(e->releaseStartDb, DB_SILENCE));
		if (e->releaseStartDb >= PERCEIVED_DB_SILENCE)
		{
			e->state = TSF_SEGMENT_DONE;
		}
		e->currentReleaseGain = tsf_decibelsToGain(e->releaseStartDb);

		// Release: sf spec page 35: the time is for change from attenuation to -100dB,
		// Therefore, we need to calculate the real time
		// (changing from release start to -100dB instead of from peak to -100dB)
		const float releaseFraction = (DB_SILENCE - e->releaseStartDb) / DB_SILENCE;
		e->releaseDuration *= releaseFraction;
	}
}

static void tsf_volume_envelope_release(struct tsf_voice *v, float outSampleRate)
{
	struct tsf_volume_envelope* e = &v->ampenv;
	if (!e->inRelease)
	{
		e->inRelease = TSF_TRUE;
		e->releaseStartTimeSamples = e->currentSampleTime;
		e->currentReleaseGain = tsf_decibelsToGain(e->currentAttenuationDb);
		tsf_volume_envelope_recalculate(v, outSampleRate);
	}
}

static float tsf_modulation_envelope_get_value(struct tsf_voice *v, float currentTime, TSF_BOOL ignoreRelease)
{
	struct tsf_modulation_envelope* e = &v->modenv;

	if (e->state >= TSF_SEGMENT_DONE)
	{
		return 0;
	}

	if (e->inRelease && !ignoreRelease) {
		// If the voice is still in the delay phase,
		// Start level will be 0 that will result in divide by zero
		if (e->releaseStartLevel == 0) {
			return 0;
		}
		return TSF_MAX(0.0, (1.0 - (currentTime - e->releaseStartTime) / e->releaseDuration) * e->releaseStartLevel);
	}

	if (currentTime < e->delayEnd)
	{
		e->currentValue = 0; // Delay
	}
	else if (currentTime < e->attackEnd)
	{
		// Modulation envelope uses convex curve for attack
		e->currentValue = CONVEX_ATTACK[(int) TSF_MIN(999, TSF_MAX(0, (1.0 - (e->attackEnd - currentTime) / e->attackDuration) * 1000.0))];
	}
	else if (currentTime < e->holdEnd)
	{
		// Hold: stay at 1
		e->currentValue = MODENV_PEAK;
	}
	else if (currentTime < e->decayEnd)
	{
		// Decay: linear ramp from 1 to sustain level
		e->currentValue = (1.0 - (e->decayEnd - currentTime) / e->decayDuration) * (e->sustainLevel - MODENV_PEAK) + MODENV_PEAK;
	}
	else
	{
		// Sustain: stay at the sustain level
		e->currentValue = e->sustainLevel;
	}
	return e->currentValue;
}

static void tsf_modulation_envelope_recalculate(struct tsf_voice *v)
{
	struct tsf_modulation_envelope* e = &v->modenv;
	e->parameters = v->region.modenv;

	// Constructor values
	if (e->state == TSF_SEGMENT_NONE)
	{
		e->inRelease = TSF_FALSE;
		e->attackDuration = 0;
		e->decayDuration = 0;
		e->holdDuration = 0;
		e->releaseDuration = 0;
		e->sustainLevel = 0;
		e->delayEnd = 0;
		e->attackEnd = 0;
		e->holdEnd = 0;
		e->decayEnd = 0;
		e->releaseStartLevel = 0;
		e->currentValue = 0;
		e->state = TSF_SEGMENT_DELAY;
	}

	// In release? Might need to recalculate the value as it can be modulated
	if (e->inRelease)
	{
		e->releaseStartLevel = tsf_modulation_envelope_get_value(v, e->releaseStartTime, TSF_TRUE);
	}

	e->sustainLevel = 1.0 - e->parameters.sustain / 1000.0;

	e->attackDuration = tsf_timecents2Secsf(e->parameters.attack);

	const float decayKeyExcursionCents = (60.0 - (float)v->playingKey) * e->parameters.keynumToDecay;
	const float decayTime = tsf_timecents2Secsf(e->parameters.decay + decayKeyExcursionCents);

	// According to the specification, the decay time is the time it takes to reach 0% from 100%
	// Calculate the time to reach actual sustain level,
	// For example, sustain 0.6 will be 0.4 of the decay time
	e->decayDuration = decayTime * (1.0 - e->sustainLevel);

	const float holdKeyExcursionCents = (60.0 - (float)v->playingKey) * e->parameters.keynumToHold;
	e->holdDuration = tsf_timecents2Secsf(holdKeyExcursionCents + e->parameters.hold);

	// Min is set to -7200 to prevent lowpass clicks
	const float releaseTime = tsf_timecents2Secsf(TSF_MAX(e->parameters.release, -7200));
	// Release time is from the full level to 0%
	// To get the actual time, multiply by the release start level
	e->releaseDuration = releaseTime * e->releaseStartLevel;

	e->delayEnd = tsf_timecents2Secsf(e->parameters.delay);
	e->attackEnd = e->delayEnd + e->attackDuration;
	e->holdEnd = e->attackEnd + e->holdDuration;
	e->decayEnd = e->holdEnd + e->decayDuration;
}

static void tsf_modulation_envelope_release(struct tsf_voice *v)
{
	struct tsf_modulation_envelope* e = &v->modenv;
	if (!e->inRelease)
	{
		e->inRelease = TSF_TRUE;
		e->releaseStartTime = v->voiceTime;
		tsf_modulation_envelope_recalculate(v);
	}
}

static void tsf_modulators_compute(tsf *f, const struct tsf_channel *c, struct tsf_voice *v, tsf_s8 sourceUsesCC, tsf_u8 sourceIndex)
{
	if (!v->modulators || !v->modNum) return;

	struct tsf_generators *generators = &v->generators;
	struct tsf_generators *modulatedGenerators = &v->modulatedGenerators;
	struct tsf_modulator *m, *mEnd, *nm;
	union tsf_hydra_genamount amount;
	int i;

	// Using these as flags
	struct tsf_generators computedDestinations;
	TSF_MEMSET(&computedDestinations, 0, sizeof(computedDestinations));

	if (sourceUsesCC == -1)
	{
		// All modulators mode: compute all modulators
		*modulatedGenerators = *generators;

		for (m = v->modulators, mEnd = m ? m + v->modNum : TSF_NULL; m != mEnd; m++)
		{
			computedDestinations.g[m->destination] = 1;
			amount.wordAmount = modulatedGenerators->g[m->destination];
			float outputValue = tsf_region_operator(&v->region, m->destination, &amount, TSF_NULL, modulatedGenerators, TSF_TRUE) + tsf_modulator_compute(m, c, v);
			amount.shortAmount = (tsf_s16) TSF_MIN(32767, TSF_MAX(-32768, outputValue));
			tsf_region_operator(&v->region, m->destination, &amount, TSF_NULL, modulatedGenerators, TSF_TRUE);
		}

		tsf_volume_envelope_recalculate(v, f->outSampleRate);
		tsf_modulation_envelope_recalculate(v);

		return;
	}

	const tsf_u8 sourceCC = !!sourceUsesCC;

	for (m = v->modulators, mEnd = m ? m + v->modNum : TSF_NULL; m != mEnd; m++)
	{
		if (
			(m->primarySource.isCC == sourceCC &&
			m->primarySource.index == sourceIndex) ||
			(m->secondarySource.isCC == sourceCC &&
			 m->secondarySource.index == sourceIndex)
			)
		{
			const enum tsf_generator_type destination = m->destination;
			if (!computedDestinations.g[destination])
			{
				// Reset this destination
				amount.wordAmount = modulatedGenerators->g[destination] = generators->g[destination];
				float outputValue = tsf_region_operator(&v->region, destination, &amount, TSF_NULL, modulatedGenerators, TSF_TRUE);
				// Compute our modulator
				tsf_modulator_compute(m, c, v);
				// Sum the values of all modulators for this destination
				for (nm = v->modulators; nm != mEnd; nm++)
				{
					if (nm->destination == destination)
					{
						outputValue += nm->currentValue;
					}
				}

				amount.shortAmount = (short) TSF_MIN(32767, TSF_MAX(-32768, outputValue));
				tsf_region_operator(&v->region, destination, &amount, TSF_NULL, modulatedGenerators, TSF_TRUE);

				computedDestinations.g[destination] = 1;
			}
		}
	}

	if (computedDestinations.g[initialAttenuation] ||
		computedDestinations.g[delayVolEnv] ||
		computedDestinations.g[attackVolEnv] ||
		computedDestinations.g[holdVolEnv] ||
		computedDestinations.g[decayVolEnv] ||
		computedDestinations.g[sustainVolEnv] ||
		computedDestinations.g[releaseVolEnv] ||
		computedDestinations.g[keyNumToVolEnvHold] ||
		computedDestinations.g[keyNumToVolEnvDecay])
	{
		tsf_volume_envelope_recalculate(v, f->outSampleRate);
	}

	tsf_modulation_envelope_recalculate(v);
}

static float tsf_volume_envelope_apply(struct tsf_voice* v, float val, float centibelOffset, float smoothingFactor)
{
	struct tsf_volume_envelope *e = &v->ampenv;
	if (e->state >= TSF_SEGMENT_DONE)
	{
		return 0;
	}

	const float decibelOffset = centibelOffset / 10.0;

	const float attenuationSmoothing = smoothingFactor;

	// RELEASE PHASE
	if (e->inRelease)
	{
		int elapsedRelease = e->currentSampleTime - e->releaseStartTimeSamples;
		if (elapsedRelease >= e->releaseDuration)
		{
			e->state = TSF_SEGMENT_DONE;
			return 0;
		}
		const float dbDifference = DB_SILENCE - e->releaseStartDb;
		e->attenuation += (e->attenuationTargetGain - e->attenuation) * attenuationSmoothing;
		const float db = (elapsedRelease / (float)e->releaseDuration) * dbDifference + e->releaseStartDb;
		e->currentReleaseGain = e->attenuation * tsf_decibelsToGain(db + decibelOffset);
		val *= e->currentReleaseGain;
		e->currentSampleTime++;

		if (e->currentReleaseGain <= PERCEIVED_GAIN_SILENCE) {
			e->state = TSF_SEGMENT_DONE;
		}

		return val;
	}

	switch (e->state) {
		case TSF_SEGMENT_NONE:
		case TSF_SEGMENT_DELAY:
			// Delay phase, no sound is produced
			if (e->currentSampleTime < e->delayEnd)
			{
				e->currentAttenuationDb = DB_SILENCE;
				val = 0;
				
				e->currentSampleTime++;

				return 0;
			}

			e->state = TSF_SEGMENT_ATTACK;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
			[[fallthrough]];
#endif

		case TSF_SEGMENT_ATTACK:
			// Attack phase: ramp from 0 to attenuation
			if (e->currentSampleTime < e->attackEnd)
			{
				// Attenuation interpolation
				e->attenuation += (e->attenuationTargetGain - e->attenuation) * attenuationSmoothing;

				// Special case: linear gain ramp instead of linear db ramp
				const float linearAttenuation = 1.0 - (e->attackEnd - e->currentSampleTime) / (float)e->attackDuration; // 0 to 1
				val *= linearAttenuation * e->attenuation * tsf_decibelsToGain(decibelOffset);
				// Set current attenuation to peak as its invalid during this phase
				e->currentAttenuationDb = 0;

				e->currentSampleTime++;

				return val;
			}

			e->state = TSF_SEGMENT_HOLD;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
			[[fallthrough]];
#endif
		case TSF_SEGMENT_HOLD:
			// Hold/peak phase: stay at attenuation
			if (e->currentSampleTime < e->holdEnd)
			{
				// Attenuation interpolation
				e->attenuation += (e->attenuationTargetGain - e->attenuation) * attenuationSmoothing;

				val *= e->attenuation * tsf_decibelsToGain(decibelOffset);
				e->currentAttenuationDb = 0;

				e->currentSampleTime++;

				return val;
			}

			e->state = TSF_SEGMENT_DECAY;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
			[[fallthrough]];
#endif

		case TSF_SEGMENT_DECAY:
			// Decay phase: linear ramp from attenuation to sustain
			if (e->currentSampleTime < e->decayEnd)
			{
				// Attenuation interpolation
				e->attenuation += (e->attenuationTargetGain - e->attenuation) * attenuationSmoothing;

				e->currentAttenuationDb = (1.0 - (e->decayEnd - e->currentSampleTime) / (float)e->decayDuration) * e->sustainDbRelative;
				val *= e->attenuation * tsf_decibelsToGain(e->currentAttenuationDb + decibelOffset);

				e->currentSampleTime++;

				return val;
			}

			e->state = TSF_SEGMENT_SUSTAIN;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
			[[fallthrough]];
#endif

		case TSF_SEGMENT_SUSTAIN: {
			// Sustain phase: stay at sustain
			if (e->canEndOnSilentSustain &&
				e->sustainDbRelative >= PERCEIVED_DB_SILENCE)
			{
				e->state = TSF_SEGMENT_DONE;
			}
			// Attenuation interpolation
			e->attenuation += (e->attenuationTargetGain - e->attenuation) * attenuationSmoothing;

			val *= e->attenuation * tsf_decibelsToGain(e->sustainDbRelative + decibelOffset);
			e->currentAttenuationDb = e->sustainDbRelative;

			e->currentSampleTime++;

			return val;
		}
	}

	return val;
}

static void tsf_voice_lowpass_setup(struct tsf_voice_lowpass* e, float Fc)
{
	// Lowpass filter from http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
	double K = TSF_TAN(TSF_PI * (double)Fc), KK = K * K;
	double norm = 1 / (1 + K * e->QInv + KK);
	e->a0 = KK * norm;
	e->a1 = 2 * e->a0;
	e->b1 = 2 * (KK - 1) * norm;
	e->b2 = (1 - K * e->QInv + KK) * norm;
}

static float tsf_voice_lowpass_process(struct tsf_voice_lowpass* e, double In)
{
	double Out = In * e->a0 + e->z1; e->z1 = In * e->a1 + e->z2 - e->b1 * Out; e->z2 = In * e->a0 - e->b2 * Out; return (float)Out;
}

static int tsf_delay_line_setup(struct tsf_delay_line* e, unsigned int maxDelay)
{
	e->feedback = 0;
	e->gain = 1;
	e->buffer = (float *) TSF_MALLOC(maxDelay * sizeof(float));
	if (!e->buffer) return 0;
	TSF_MEMSET(e->buffer, 0, maxDelay * sizeof(float));
	e->bufferLength = maxDelay;
	e->writeIndex = 0;
	e->time = maxDelay - 5;
	return 1;
}

static void tsf_delay_line_process(struct tsf_delay_line* e, const float* In, float* Out, int samples)
{
	unsigned int writeIndex = e->writeIndex;
	const unsigned int delay = e->time;
	float *buffer = e->buffer;
	const unsigned int bufferLength = e->bufferLength;
	const float feedback = e->feedback;
	const float gain = e->gain;

	int i;
	for (i = 0; i < samples; i++)
	{
		int readIndex = (signed)writeIndex - (signed)delay;
		if (readIndex < 0) readIndex += bufferLength;

		const float delayed = buffer[readIndex];
		Out[i] = delayed * gain;

		buffer[writeIndex] = In[i] + delayed * feedback;

		if (++writeIndex >= bufferLength) writeIndex = 0;
	}

	e->writeIndex = writeIndex;
}

static void tsf_delay_line_clear(struct tsf_delay_line* e)
{
	TSF_MEMSET(e->buffer, 0, e->bufferLength * sizeof(float));
}

static void tsf_delay_line_free(struct tsf_delay_line* e)
{
	TSF_FREE(e->buffer); e->buffer = TSF_NULL;
}

static int tsf_dattorro_delay_line_setup(struct tsf_dattorro_delay_line* e, double Delay, float sampleRate)
{
	const unsigned int len = (unsigned)(TSF_ROUND(Delay * (double)sampleRate));
	const unsigned int nextPow2 = (unsigned)TSF_POW(2.0, TSF_CEIL(TSF_LOG2((float)len)));
	e->buffer = (float *) TSF_MALLOC(nextPow2 * sizeof(float));
	if (!e->buffer) return 0;
	TSF_MEMSET(e->buffer, 0, nextPow2 * sizeof(float));
	e->writeIndex = len - 1;
	e->readIndex = 0;
	e->writeMask = nextPow2 - 1;
	return 1;
}

static void tsf_dattorro_delay_line_free(struct tsf_dattorro_delay_line* e)
{
	TSF_FREE(e->buffer); e->buffer = TSF_NULL;
}

static float tsf_dattorro_delay_line_write(struct tsf_dattorro_reverb* e, int index, float Input)
{
	struct tsf_dattorro_delay_line* d = &e->delays[index];
	return (d->buffer[d->writeIndex] = Input);
}

static float tsf_dattorro_delay_line_read(struct tsf_dattorro_reverb* e, int index)
{
	struct tsf_dattorro_delay_line* d = &e->delays[index];
	return d->buffer[d->readIndex];
}

static float tsf_dattorro_delay_line_read_at(struct tsf_dattorro_reverb* e, int index, int offset)
{
	struct tsf_dattorro_delay_line* d = &e->delays[index];
	return d->buffer[(d->readIndex + offset) & d->writeMask];
}

static float tsf_dattorro_delay_line_read_cubic_at(struct tsf_dattorro_reverb* e, int index, float offset) {
	struct tsf_dattorro_delay_line* d = &e->delays[index];
	const float frac = offset - (float)((int)offset);
	const unsigned int mask = d->writeMask;

	unsigned int intOffset = ((int)offset) + d->readIndex - 1;

	const float x0 = d->buffer[intOffset++ & mask],
		x1 = d->buffer[intOffset++ & mask],
		x2 = d->buffer[intOffset++ & mask],
		x3 = d->buffer[intOffset & mask];

	const float a = (3.0 * (x1 - x2) - x0 + x3) / 2.0,
		b = 2.0 * x2 + x0 - (5.0 * x1 + x3) / 2.0,
		c = (x2 - x0) / 2.0;

	return ((a * frac + b) * frac + c) * frac + x1;
}

static int tsf_dattorro_reverb_setup(struct tsf_dattorro_reverb* e, float sampleRate)
{
	static const double templateDelays[] = {
		0.004771345, 0.003595309, 0.012734787, 0.009307483,
		0.022579886, 0.149625349, 0.060481839, 0.1249958,
		0.030509727, 0.141695508, 0.089244313, 0.106280031
	};

	static const double templateTaps[] = {
		0.008937872, 0.099929438, 0.064278754, 0.067067639,
		0.066866033, 0.006283391, 0.035818689, 0.011861161,
		0.121870905, 0.041262054, 0.08981553, 0.070931756,
		0.011256342, 0.004065724
	};

	int i;

	e->preDelay = 0;
	e->preLPF = 0.5f;
	e->inputDiffusion[0] = 0.75f;
	e->inputDiffusion[1] = 0.625f;
	e->decay = 0.5f;
	e->decayDiffusion[0] = 0.7f;
	e->decayDiffusion[1] = 0.5f;
	e->damping = 0.005f;
	e->excursionRate = 0.1f;
	e->excursionDepth = 0.2f;
	e->gain = 1.0f;
	e->sampleRate = sampleRate;
	e->lp[0] = 0;
	e->lp[1] = 0;
	e->lp[2] = 0;
	e->excPhase = 0;
	e->pDWrite = 0;

	e->delays = TSF_NULL;
	e->taps = TSF_NULL;

	e->pDLength = (unsigned int) TSF_ROUND(sampleRate);
	e->pDelay = (float *) TSF_MALLOC(e->pDLength * sizeof(float));
	if (!e->pDelay) return 0;
	TSF_MEMSET(e->pDelay, 0, e->pDLength * sizeof(float));

	e->delays = (struct tsf_dattorro_delay_line *) TSF_MALLOC(12 * sizeof(*e->delays));
	if (!e->delays) return 0;
	TSF_MEMSET(e->delays, 0, 12 * sizeof(*e->delays));

	for (i = 0; i < 12; i++)
	{
		if (!tsf_dattorro_delay_line_setup(&e->delays[i], templateDelays[i], sampleRate))
			return 0;
	}

	e->taps = (short *) TSF_MALLOC(14 * sizeof(short));
	if (!e->taps) return 0;

	for (i = 0; i < 14; i++)
		e->taps[i] = (short)(TSF_ROUND(templateTaps[i] * (double)sampleRate));

	return 1;
}

static void tsf_dattorro_reverb_process(struct tsf_dattorro_reverb* e, const float* Input, float* outputL, float* outputR, int samples, int channels)
{
	const unsigned int pd = e->preDelay;
	const float fi = e->inputDiffusion[0];
	const float si = e->inputDiffusion[1];
	const float dc = e->decay;
	const float ft = e->decayDiffusion[0];
	const float st = e->decayDiffusion[1];
	const float dp = 1.0 - e->damping;
	const float ex = e->excursionRate / e->sampleRate;
	const float ed = (e->excursionDepth * e->sampleRate) / 1000.0;
	const unsigned int blockStart = e->pDWrite;
	const unsigned int blockLength = e->pDLength;
	const float gain = e->gain;

	int i, j;

	for (i = 0; i < samples; i++)
	{
		e->pDelay[(blockStart + i) % blockLength] = Input[i];
	}

	for (i = 0; i < samples; i++)
	{
		e->lp[0] +=
		e->preLPF *
		(e->pDelay[
			(blockLength + blockStart - pd + i) % blockLength
		] -
		 e->lp[0]);

#define delayWrite(n, v)   tsf_dattorro_delay_line_write(e, n, v)
#define delayRead(n)       tsf_dattorro_delay_line_read(e, n)
#define delayReadAt(n, p)  tsf_dattorro_delay_line_read_at(e, n, p)
#define delayReadCAt(n, p) tsf_dattorro_delay_line_read_cubic_at(e, n, p)

		// Pre-tank
		float pre = delayWrite(0, e->lp[0] - fi * delayRead(0));
		pre = delayWrite(1, fi * (pre - delayRead(1)) + delayRead(0));
		pre = delayWrite(2, fi *  pre + delayRead(1)  - si * delayRead(2));
		pre = delayWrite(3, si * (pre - delayRead(3)) + delayRead(2));

		const float split = si * pre + delayRead(3);

		// Excursions
		// Could be optimized?
		const float exc = ed * (1.0 + TSF_COS(e->excPhase * 6.28f));
		const float exc2 = ed * (1.0 + TSF_SIN(e->excPhase * 6.2847f));

		// Left loop
		float temp = delayWrite(4, split + dc * delayRead(11) + ft * delayReadCAt(4, exc)); // Tank diffuse 1
		delayWrite(5, delayReadCAt(4, exc) - ft * temp); // Long delay 1
		e->lp[1] += dp * (delayRead(5) - e->lp[1]); // Damp 1
		temp = delayWrite(6, dc * e->lp[1] - st * delayRead(6)); // Tank diffuse 2
		delayWrite(7, delayRead(6) + st * temp); // Long delay 2

		// Right loop
		temp = delayWrite(8, split + dc * delayRead(7) + ft * delayReadCAt(8, exc2)); // Tank diffuse 3
		delayWrite(9, delayReadCAt(8, exc2) - ft * temp); // Long delay 3
		e->lp[2] += dp * (delayRead(9) - e->lp[2]); // Damp 2
		temp = delayWrite(10, dc * e->lp[2] - st * delayRead(10)); // Tank diffuse 4
		delayWrite(11, delayRead(10) + st * temp); // Long delay 4

		// Mix down
		const float leftSample =
			delayReadAt(9, e->taps[0]) +
			delayReadAt(9, e->taps[1]) -
			delayReadAt(10, e->taps[2]) +
			delayReadAt(11, e->taps[3]) -
			delayReadAt(5, e->taps[4]) -
			delayReadAt(6, e->taps[5]) -
			delayReadAt(7, e->taps[6]);

		const float rightSample =
			delayReadAt(5, e->taps[7]) +
			delayReadAt(5, e->taps[8]) -
			delayReadAt(6, e->taps[9]) +
			delayReadAt(7, e->taps[10]) -
			delayReadAt(9, e->taps[11]) -
			delayReadAt(10, e->taps[12]) -
			delayReadAt(11, e->taps[13]);

		if (outputL && !outputR)
		{
			if (channels == 2)
			{
				*outputL++ += leftSample * gain;
				*outputL++ += rightSample * gain;
			}
			else
			{
				*outputL++ += ((rightSample + rightSample) / 2.0) * e->gain;
			}
		}
		else if (outputL && outputR)
		{
			*outputL++ += leftSample * gain;
			*outputR++ += rightSample * gain;
		}

#undef delayWrite
#undef delayRead
#undef delayReadAt
#undef delayReadCAt

		e->excPhase += ex;

		// Advance delays
		for (j = 0; j < 12; j++)
		{
			struct tsf_dattorro_delay_line* d = &e->delays[j];
			d->writeIndex = (d->writeIndex + 1) & d->writeMask;
			d->readIndex = (d->readIndex + 1) & d->writeMask;
		}
	}
	e->pDWrite = (blockStart + samples) % blockLength;
}

static void tsf_dattorro_reverb_free(struct tsf_dattorro_reverb* e)
{
	TSF_FREE(e->pDelay); e->pDelay = TSF_NULL;
	TSF_FREE(e->taps);   e->taps = TSF_NULL;
	if (e->delays)
	{
		int i;
		for (i = 0; i < 12; i++)
			tsf_dattorro_delay_line_free(&e->delays[i]);
	}
	TSF_FREE(e->delays); e->delays = TSF_NULL;
}

static int tsf_reverb_setup(struct tsf_reverb** ee, float sampleRate, int maxBufferSize)
{
	struct tsf_reverb* e = (struct tsf_reverb *) TSF_MALLOC(sizeof(*e));
	if (!e) return 0;

	*ee = e;

	e->maxBufferSize = maxBufferSize;
	e->sampleRate = sampleRate;

	e->parameters.delayFeedback = 0;
	e->parameters.character = 0;
	e->parameters.time = 0;
	e->parameters.preDelayTime = 0;
	e->parameters.level = 0;
	e->parameters.preLowpass = 0;

	e->preLPFfc = 8000.0;
	e->preLPFa = 0;
	e->preLPFz = 0;

	e->characterTimeCoefficient = 1.0;
	e->characterGainCoefficient = 1.0;
	e->characterLPFCoefficient = 0;

	e->delayGain = 1;

	e->panDelayFeedback = 0;

	e->delayLeftOutput = TSF_NULL;
	e->delayRightOutput = TSF_NULL;
	e->delayLeftInput = TSF_NULL;
	e->delayPreLPF = TSF_NULL;

	e->delayLeft.buffer = TSF_NULL;
	e->delayRight.buffer = TSF_NULL;

	e->delayLeftOutput = (float *) TSF_MALLOC(maxBufferSize * sizeof(float));
	if (!e->delayLeftOutput) return 0;
	TSF_MEMSET(e->delayLeftOutput, 0, maxBufferSize * sizeof(float));
	e->delayRightOutput = (float *) TSF_MALLOC(maxBufferSize * sizeof(float));
	if (!e->delayRightOutput) return 0;
	TSF_MEMSET(e->delayRightOutput, 0, maxBufferSize * sizeof(float));
	e->delayLeftInput = (float *) TSF_MALLOC(maxBufferSize * sizeof(float));
	if (!e->delayLeftInput) return 0;
	TSF_MEMSET(e->delayLeftInput, 0, maxBufferSize * sizeof(float));
	e->delayPreLPF = (float *) TSF_MALLOC(maxBufferSize * sizeof(float));
	if (!e->delayPreLPF) return 0;
	TSF_MEMSET(e->delayPreLPF, 0, maxBufferSize * sizeof(float));
	if (!tsf_dattorro_reverb_setup(&e->dattorro, sampleRate)) return 0;
	if (!tsf_delay_line_setup(&e->delayLeft, (unsigned)sampleRate)) return 0;
	if (!tsf_delay_line_setup(&e->delayRight, (unsigned)sampleRate)) return 0;
	return 1;
}

static void tsf_reverb_free(struct tsf_reverb* e)
{
	if (!e) return;
	TSF_FREE(e->delayLeftOutput); e->delayLeftOutput = TSF_NULL;
	TSF_FREE(e->delayRightOutput); e->delayRightOutput = TSF_NULL;
	TSF_FREE(e->delayLeftInput); e->delayLeftInput = TSF_NULL;
	TSF_FREE(e->delayPreLPF); e->delayPreLPF = TSF_NULL;
	tsf_dattorro_reverb_free(&e->dattorro);
	tsf_delay_line_free(&e->delayLeft);
	tsf_delay_line_free(&e->delayRight);
	TSF_FREE(e);
}

static void tsf_reverb_update_feedback(struct tsf_reverb* e)
{
	const float x = (float)e->parameters.delayFeedback / 127.0;
	const float exp = 1.0 - TSF_POW(1.0 - x, 1.9f);
	if (e->parameters.character == 6)
	{
		e->delayLeft.feedback = exp * 0.73f;
	}
	else
	{
		e->delayLeft.feedback = e->delayRight.feedback = 0;
		e->panDelayFeedback = exp * 0.73f;
	}
}

static void tsf_reverb_update_lowpass(struct tsf_reverb* e)
{
	const float preLPF = 0.1 + (float)(7 - e->parameters.preLowpass) / 14.0 + e->characterLPFCoefficient;
	e->dattorro.preLPF = (preLPF < 1.0) ? preLPF : 1.0;
}

static void tsf_reverb_update_gain(struct tsf_reverb* e)
{
	e->dattorro.gain = ((float)e->parameters.level / 348.0) * e->characterGainCoefficient;
	e->delayGain = ((float)e->parameters.level / 127.0) * 1.5;
}

static void tsf_reverb_update_time(struct tsf_reverb* e)
{
	const float t = (float)e->parameters.time / 127.0;
	e->dattorro.decay = e->characterTimeCoefficient * (0.05 + 0.65 * t);
	// Delay at 127 is exactly 0.4468 seconds
	// The minimum value (delay 0) seems to be 21 samples
	const unsigned int calcSamples = (unsigned int)(t * e->sampleRate * 0.4468);
	const unsigned int timeSamples = (calcSamples > 21) ? calcSamples : 21;
	if (e->parameters.character == 7)
	{
		// Half the delay time
		e->delayRight.time = e->delayLeft.time = timeSamples / 2;
	}
	else
	{
		e->delayLeft.time = timeSamples;
	}
}

static void tsf_reverb_set_delay_feedback(struct tsf_reverb* e, unsigned char delayFeedback)
{
	e->parameters.delayFeedback = delayFeedback;
	tsf_reverb_update_feedback(e);
}

static void tsf_reverb_set_character(struct tsf_reverb* e, unsigned char character)
{
	e->parameters.character = character;
	e->dattorro.damping = 0.005;
	e->characterTimeCoefficient = 1;
	e->characterGainCoefficient = 1;
	e->characterLPFCoefficient = 0;
	e->dattorro.inputDiffusion[0] = 0.75;
	e->dattorro.inputDiffusion[1] = 0.625;
	e->dattorro.decayDiffusion[0] = 0.7;
	e->dattorro.decayDiffusion[1] = 0.5;
	e->dattorro.excursionRate = 0.5;
	e->dattorro.excursionDepth = 0.7;

	// Tested all characters on level = 64, preset: Hall2
	// File: gs_reverb_character_test.ts, compare spessasynth to SC-VA
	// Tuned by me, though I'm not very good at it :-)
	switch (character)
	{
		case 0: {
			// Room1
			e->dattorro.damping = 0.85;
			e->characterTimeCoefficient = 0.9;
			e->characterGainCoefficient = 0.7;
			e->characterLPFCoefficient = 0.2;
			break;
		}

		case 1: {
			// Room2
			e->dattorro.damping = 0.2;
			e->characterGainCoefficient = 0.5;
			e->characterTimeCoefficient = 1;
			e->dattorro.decayDiffusion[1] = 0.64;
			e->dattorro.decayDiffusion[0] = 0.6;
			e->characterLPFCoefficient = 0.2;
			break;
		}

		case 2: {
			// Room3
			e->dattorro.damping = 0.56;
			e->characterGainCoefficient = 0.55;
			e->characterTimeCoefficient = 1;
			e->dattorro.decayDiffusion[1] = 0.64;
			e->dattorro.decayDiffusion[0] = 0.6;
			e->characterLPFCoefficient = 0.1;
			break;
		}

		case 3: {
			// Hall1
			e->dattorro.damping = 0.6;
			e->characterGainCoefficient = 1;
			e->characterLPFCoefficient = 0;
			e->dattorro.decayDiffusion[1] = 0.7;
			e->dattorro.decayDiffusion[0] = 0.66;
			break;
		}

		case 4: {
			// Hall2
			e->characterGainCoefficient = 0.75;
			e->dattorro.damping = 0.2;
			e->characterLPFCoefficient = 0.2;
			break;
		}

		case 5: {
			// Plate
			e->characterGainCoefficient = 0.55;
			e->dattorro.damping = 0.65;
			e->characterTimeCoefficient = 0.5;
			break;
		}
	}

	// Update values
	tsf_reverb_update_time(e);
	tsf_reverb_update_gain(e);
	tsf_reverb_update_lowpass(e);
	tsf_reverb_update_feedback(e);
	tsf_delay_line_clear(&e->delayLeft);
	tsf_delay_line_clear(&e->delayRight);
}

static void tsf_reverb_set_time(struct tsf_reverb* e, unsigned char time)
{
	e->parameters.time = time;
	tsf_reverb_update_time(e);
}

static void tsf_reverb_set_pre_delay_time(struct tsf_reverb* e, unsigned char preDelayTime)
{
	e->parameters.preDelayTime = preDelayTime;
	e->dattorro.preDelay = ((float)preDelayTime / 1000.0) * e->sampleRate;
}

static void tsf_reverb_set_level(struct tsf_reverb* e, unsigned char level)
{
	e->parameters.level = level;
	tsf_reverb_update_gain(e);
}

static void tsf_reverb_set_pre_lowpass(struct tsf_reverb* e, unsigned char preLowpass)
{
	e->parameters.preLowpass = preLowpass;
	e->preLPFfc = 8000.0 * TSF_POW(0.63, (float)preLowpass);
	const float decay = TSF_EXPF((-2.0 * TSF_PI * e->preLPFfc) / e->sampleRate);
	e->preLPFa = 1.0 - decay;
	tsf_reverb_update_lowpass(e);
}

static void tsf_reverb_set_macro(struct tsf_reverb* e, unsigned char value)
{
	if (!e) return;

	// SC-8850 manual page 81
	tsf_reverb_set_level(e, 64);
	tsf_reverb_set_pre_delay_time(e, 0);
	tsf_reverb_set_character(e, value);
	switch (value)
	{
			/**
			 * REVERB MACRO is a macro parameter that allows global setting of reverb parameters.
			 * When you select the reverb type with REVERB MACRO, each reverb parameter will be set to their most
			 * suitable value.
			 *
			 * Room1, Room2, Room3
			 * These reverbs simulate the reverberation of a room. They provide a well-defined
			 * spacious reverberation.
			 * Hall1, Hall2
			 * These reverbs simulate the reverberation of a concert hall. They provide a deeper
			 * reverberation than the Room reverbs.
			 * Plate
			 * This simulates a plate reverb (a studio device using a metal plate).
			 * Delay
			 * This is a conventional delay that produces echo effects.
			 * Panning Delay
			 * This is a special delay in which the delayed sounds move left and right.
			 * It is effective when you are listening in stereo.
			 */
		case 0: {
			// Room1
			tsf_reverb_set_pre_lowpass(e, 3);
			tsf_reverb_set_time(e, 80);
			tsf_reverb_set_delay_feedback(e, 0);
			tsf_reverb_set_pre_delay_time(e, 0);
			break;
		}

		case 1: {
			// Room2
			tsf_reverb_set_pre_lowpass(e, 4);
			tsf_reverb_set_time(e, 56);
			tsf_reverb_set_delay_feedback(e, 0);
			break;
		}

		case 2: {
			// Room3
			tsf_reverb_set_pre_lowpass(e, 0);
			tsf_reverb_set_time(e, 72);
			tsf_reverb_set_delay_feedback(e, 0);
			break;
		}

		case 3: {
			// Hall1
			tsf_reverb_set_pre_lowpass(e, 4);
			tsf_reverb_set_time(e, 72);
			tsf_reverb_set_delay_feedback(e, 0);
			break;
		}

		case 4: {
			// Hall2
			tsf_reverb_set_pre_lowpass(e, 0);
			tsf_reverb_set_time(e, 64);
			tsf_reverb_set_delay_feedback(e, 0);
			break;
		}

		case 5: {
			// Plate
			tsf_reverb_set_pre_lowpass(e, 0);
			tsf_reverb_set_time(e, 88);
			tsf_reverb_set_delay_feedback(e, 0);
			break;
		}

		case 6: {
			// Delay
			tsf_reverb_set_pre_lowpass(e, 0);
			tsf_reverb_set_time(e, 32);
			tsf_reverb_set_delay_feedback(e, 40);
			break;
		}

		case 7: {
			// Panning delay
			tsf_reverb_set_pre_lowpass(e, 0);
			tsf_reverb_set_time(e, 64);
			tsf_reverb_set_delay_feedback(e, 32);
			break;
		}

		default: {
			// Check for invalid macros
			// Testcase: 18 - Dichromatic Lotus Butterfly ~ Ancients (ZUN).mid
			return;
		}
	}
}

static void tsf_reverb_process(struct tsf_reverb* e, const float* Input, float* OutputL, float* OutputR, int samples, int channels)
{
	int i;
	switch (e->parameters.character)
	{
		default: {
			// Reverb
			tsf_dattorro_reverb_process(&e->dattorro, Input, OutputL, OutputR, samples, channels);
			return;
		}

		case 6: {
			// Delay
			// Process pre-lowpass
			const float *delayIn;
			if (e->parameters.preLowpass > 0)
			{
				float *preLPF = e->delayPreLPF;
				float z = e->preLPFz;
				const float a = e->preLPFa;
				for (i = 0; i < samples; i++)
				{
					const float x = Input[i];
					z += a * (x - z);
					preLPF[i] = z;
				}
				e->preLPFz = z;
				delayIn = preLPF;
			}
			else
			{
				delayIn = Input;
			}

			tsf_delay_line_process(&e->delayLeft, delayIn, e->delayLeftOutput, samples);

			// Mix down
			const float g = e->delayGain;
			const float* delay = e->delayLeftOutput;
			for (i = 0; i < samples; i++)
			{
				const float sample = delay[i] * g;
				if (OutputL)
				{
					*OutputL += sample;
					OutputL += channels;
				}
				if (OutputR)
				{
					*OutputR++ += sample;
				}
			}
			return;
		}

		case 7: {
			// Panning Delay
			// Process pre-lowpass
			const float *delayIn;
			if (e->parameters.preLowpass > 0)
			{
				float *preLPF = e->delayPreLPF;
				float z = e->preLPFz;
				const float a = e->preLPFa;
				for (i = 0; i < samples; i++)
				{
					const float x = Input[i];
					z += a * (x - z);
					preLPF[i] = z;
				}
				e->preLPFz = z;
				delayIn = preLPF;
			}
			else
			{
				delayIn = Input;
			}

			// Mix right into left
			const float fb = e->panDelayFeedback;
			float *delayLeftInput = e->delayLeftInput;
			float *delayLeftOutput = e->delayLeftOutput;
			float *delayRightOutput = e->delayRightOutput;
			for (i = 0; i < samples; i++)
			{
				delayLeftInput[i] = delayIn[i] + delayRightOutput[i] * fb;
			}
			// Process left
			tsf_delay_line_process(&e->delayLeft, delayLeftInput, delayLeftOutput, samples);
			// Process right
			tsf_delay_line_process(&e->delayRight, delayLeftOutput, delayRightOutput, samples);
			// Mix
			const float g = e->delayGain;
			for (i = 0; i < samples; i++)
			{
				if (OutputL)
				{
					*OutputL += delayLeftOutput[i] * g;
					OutputL += channels;
				}
				if (OutputR)
				{
					*OutputR++ += delayRightOutput[i] * g;
				}
			}
			return;
		}
	}
}

static int tsf_chorus_setup(struct tsf_chorus** ee, float sampleRate, int maxBufferSize)
{
	struct tsf_chorus *e = (struct tsf_chorus *) TSF_MALLOC(sizeof(*e));
	if (!e) return 0;

	*ee = e;

	e->parameters.sendLevelToReverb = 0;
	e->parameters.sendLevelToDelay = 0;
	e->parameters.preLowpass = 0;
	e->parameters.depth = 0;
	e->parameters.delay = 0;
	e->parameters.feedback = 0;
	e->parameters.rate = 0;
	e->parameters.level = 64;

	e->preLPFfc = 8000;
	e->preLPFa = 0;
	e->preLPFz = 0;

	e->phase = 0;
	e->write = 0;
	e->gain = 0.5;
	e->reverbGain = 0;
	e->delayGain = 0;
	e->depthSamples = 0;
	e->delaySamples = 1;
	e->rateInc = 0;
	e->feedbackGain = 0;

	e->leftDelayBuffer = TSF_NULL;
	e->rightDelayBuffer = TSF_NULL;

	e->sampleRate = sampleRate;

	// Override
	maxBufferSize = (unsigned int) TSF_ROUND(sampleRate);

	e->maxBufferSize = maxBufferSize;

	e->leftDelayBuffer = (float *) TSF_MALLOC(maxBufferSize * sizeof(float));
	if (!e->leftDelayBuffer) return 0;
	TSF_MEMSET(e->leftDelayBuffer, 0, maxBufferSize * sizeof(float));
	e->rightDelayBuffer = (float *) TSF_MALLOC(maxBufferSize * sizeof(float));
	if (!e->rightDelayBuffer) return 0;
	TSF_MEMSET(e->rightDelayBuffer, 0, maxBufferSize * sizeof(float));

	return 1;
}

static void tsf_chorus_free(struct tsf_chorus* e)
{
	if (!e) return;
	TSF_FREE(e->leftDelayBuffer); e->leftDelayBuffer = TSF_NULL;
	TSF_FREE(e->rightDelayBuffer); e->rightDelayBuffer = TSF_NULL;
	TSF_FREE(e);
}

static void tsf_chorus_set_send_level_to_reverb(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.sendLevelToReverb = value;
	e->reverbGain = (float)value / 127.0;
}

static void tsf_chorus_set_send_level_to_delay(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.sendLevelToDelay = value;
	e->delayGain = (float)value / 127.0;
}

static void tsf_chorus_set_pre_lowpass(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.preLowpass = value;
	// GS sure loves weird mappings, huh?
	// Maps to around 8000-300 Hz
	e->preLPFfc = 8000.0 * pow(0.63, (float)value);
	const float decay = TSF_EXPF((-2.0 * TSF_PI * e->preLPFfc) / e->sampleRate);
	e->preLPFa = 1.0 - decay;
}

static void tsf_chorus_set_depth(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.depth = value;
	e->depthSamples = (unsigned int) TSF_ROUND(((float)value / 127.0) * 0.025 * e->sampleRate);
}

static void tsf_chorus_set_delay(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.delay = value;
	const unsigned int delaySamples = (unsigned int) TSF_ROUND(((float)value / 127.0) * 0.025 * e->sampleRate);
	e->delaySamples = delaySamples > 1 ? delaySamples : 1;
}

static void tsf_chorus_set_feedback(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.feedback = value;
	e->feedbackGain = (float)value / 128.0;
}

static void tsf_chorus_set_rate(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.rate = value;
	const float rate = 15.5 * ((float)value / 127.0);
	e->rateInc = rate / e->sampleRate;
}

static void tsf_chorus_set_level(struct tsf_chorus* e, unsigned char value)
{
	e->parameters.level = value;
	e->gain = ((float)value / 127.0) * 1.3;
}

static void tsf_chorus_set_macro(struct tsf_chorus* e, unsigned char value)
{
	tsf_chorus_set_level(e, 64);
	tsf_chorus_set_pre_lowpass(e, 0);
	tsf_chorus_set_delay(e, 127);
	tsf_chorus_set_send_level_to_delay(e, 0);
	tsf_chorus_set_send_level_to_reverb(e, 0);
	switch (value)
	{
			/**
			 * CHORUS MACRO is a macro parameter that allows global setting of chorus parameters.
			 * When you select the chorus type with CHORUS MACRO, each chorus parameter will be set to their
			 * most suitable value.
			 *
			 * Chorus1, Chorus2, Chorus3, Chorus4
			 * These are conventional chorus effects that add spaciousness and depth to the
			 * sound.
			 * Feedback Chorus
			 * This is a chorus with a flanger-like effect and a soft sound.
			 * Flanger
			 * This is an effect sounding somewhat like a jet airplane taking off and landing.
			 * Short Delay
			 * This is a delay with a short delay time.
			 * Short Delay (FB)
			 * This is a short delay with many repeats.
			 */
		case 0: {
			// Chorus1
			tsf_chorus_set_feedback(e, 0);
			tsf_chorus_set_delay(e, 112);
			tsf_chorus_set_rate(e, 3);
			tsf_chorus_set_depth(e, 5);
			break;
		}

		case 1: {
			// Chorus2
			tsf_chorus_set_feedback(e, 5);
			tsf_chorus_set_delay(e, 80);
			tsf_chorus_set_rate(e, 9);
			tsf_chorus_set_depth(e, 19);
			break;
		}

		case 2: {
			// Chorus3
			tsf_chorus_set_feedback(e, 8);
			tsf_chorus_set_delay(e, 80);
			tsf_chorus_set_rate(e, 3);
			tsf_chorus_set_depth(e, 19);
			break;
		}

		case 3: {
			// Chorus4
			tsf_chorus_set_feedback(e, 16);
			tsf_chorus_set_delay(e, 64);
			tsf_chorus_set_rate(e, 9);
			tsf_chorus_set_depth(e, 16);
			break;
		}

		case 4: {
			// FbChorus
			tsf_chorus_set_feedback(e, 64);
			tsf_chorus_set_delay(e, 127);
			tsf_chorus_set_rate(e, 2);
			tsf_chorus_set_depth(e, 24);
			break;
		}

		case 5: {
			// Flanger
			tsf_chorus_set_feedback(e, 112);
			tsf_chorus_set_delay(e, 127);
			tsf_chorus_set_rate(e, 1);
			tsf_chorus_set_depth(e, 5);
			break;
		}

		case 6: {
			// SDelay
			tsf_chorus_set_feedback(e, 0);
			tsf_chorus_set_delay(e, 127);
			tsf_chorus_set_rate(e, 0);
			tsf_chorus_set_depth(e, 127);
			break;
		}

		case 7: {
			// SDelayFb
			tsf_chorus_set_feedback(e, 80);
			tsf_chorus_set_delay(e, 127);
			tsf_chorus_set_rate(e, 0);
			tsf_chorus_set_depth(e, 127);
			break;
		}

		default: {
			return;
		}
	}
}

static void tsf_chorus_process(struct tsf_chorus* e, const float* Input, float* OutputL, float* OutputR, float* OutputReverb, float* OutputDelay, int samples, int channels)
{
	float *bufferL = e->leftDelayBuffer;
	float *bufferR = e->rightDelayBuffer;
	const float rateInc = e->rateInc;
	const unsigned int bufferLen = e->maxBufferSize;
	const unsigned int depth = e->depthSamples;
	const unsigned int delay = e->delaySamples;
	const float gain = e->gain;
	const float reverbGain = e->reverbGain;
	const float delayGain = e->delayGain;
	const float feedback = e->feedbackGain;

	const TSF_BOOL preLPF = e->parameters.preLowpass > 0;
	float phase = e->phase;
	unsigned int write = e->write;
	float z = e->preLPFz;
	const float a = e->preLPFa;

	const TSF_BOOL outReverb = OutputReverb && reverbGain > 0.0;
	const TSF_BOOL outDelay = OutputDelay && delayGain > 0.0;

	int i;
	for (i = 0; i < samples; i++)
	{
		float inputSample = Input[i];
		// Pre lowpass filter
		if (preLPF)
		{
			z += a * (inputSample - z);
			inputSample = z;
		}

		// Triangle LFO (GS uses triangle)
		const float lfo = 2.0 * TSF_FABS(phase - 0.5);

		// Read position
		const float dL = TSF_MAX(1.0, TSF_MIN(delay + lfo * depth, bufferLen));
		float readPosL = (float)write - dL;
		if (readPosL < 0.0) readPosL += (float)bufferLen;

		// Linear interpolation
		unsigned int x0 = (unsigned int) readPosL;
		unsigned int x1 = x0 + 1;
		if (x1 >= bufferLen) x1 -= bufferLen;
		float frac = readPosL - (float)x0;
		const float outL = bufferL[x0] * (1.0 - frac) + bufferL[x1] * frac;

		// Write input sample
		bufferL[write] = inputSample + outL * feedback;

		// Same for the right line (shared buffer for now for testing)
		const float dR = TSF_MAX(1.0, TSF_MIN(delay + (1.0 - lfo) * depth, bufferLen));
		float readPosR = (float)write - dR;
		if (readPosR < 0.0) readPosR += (float)bufferLen;

		// Linear interpolation
		x0 = (unsigned int) readPosR;
		x1 = x0 + 1;
		if (x1 >= bufferLen) x1 -= bufferLen;
		frac = readPosR - (float)x0;
		const float outR = bufferR[x0] * (1.0 - frac) + bufferR[x1] * frac;

		// Write input sample
		bufferR[write] = inputSample + outR * feedback;

		// Mix outputs
		float mono;
		if ((!OutputR && channels == 1) || outReverb || outDelay)
			mono = (outL + outR) / 2.0;
		if (OutputL && !OutputR)
		{
			if (channels == 2)
			{
				*OutputL++ += outL * gain;
				*OutputL++ += outR * gain;
			}
			else
			{
				*OutputL++ += mono * gain;
			}
		}
		else if (OutputL && OutputR)
		{
			*OutputL++ += outL * gain;
			*OutputR++ += outR * gain;
		}

		// Mix other effects outputs
		if (outReverb)
		{
			*OutputReverb++ += mono * reverbGain;
		}
		if (outDelay)
		{
			*OutputDelay++ += mono * delayGain;
		}

		// Advance pointers
		if (++write >= bufferLen) write = 0;

		if ((phase += rateInc) >= 1.0) phase -= 1.0;
	}
	e->write = write;
	e->phase = phase;
	e->preLPFz = z;
}

static void tsf_voice_lfo_setup(struct tsf_voice_lfo* e, float delay, int freqCents, float outSampleRate)
{
	e->samplesUntil = (int)(delay * outSampleRate);
	e->delta = (4.0f * tsf_cents2Hertz((float)freqCents) / outSampleRate);
	e->level = 0;
}

static void tsf_voice_lfo_process(struct tsf_voice_lfo* e, int blockSamples)
{
	if (e->samplesUntil > blockSamples) { e->samplesUntil -= blockSamples; return; }
	e->level += e->delta * (float)blockSamples;
	if      (e->level >  1.0f) { e->delta = -e->delta; e->level =  2.0f - e->level; }
	else if (e->level < -1.0f) { e->delta = -e->delta; e->level = -2.0f - e->level; }
}

static void tsf_voice_kill(struct tsf_voice* v)
{
	v->playingPreset = -1;
	TSF_FREE(v->modulators); v->modulators = TSF_NULL;
	v->modNum = 0;
}

static void tsf_voice_end(tsf* f, struct tsf_voice* v)
{
	// if maxVoiceNum is set, assume that voice rendering and note queuing are on separate threads
	// so to minimize the chance that voice rendering would advance the segment at the same time
	// we just do it twice here and hope that it sticks
	int repeats = (f->maxVoiceNum ? 2 : 1);
	while (repeats--)
	{
		tsf_volume_envelope_release(v, f->outSampleRate);
		tsf_modulation_envelope_release(v);
		if (v->region.loop_mode == TSF_LOOPMODE_SUSTAIN)
		{
			// Continue playing, but stop looping.
			v->loopEnd = v->loopStart;
		}
	}
}

static void tsf_voice_endquick(tsf* f, struct tsf_voice* v)
{
	// if maxVoiceNum is set, assume that voice rendering and note queuing are on separate threads
	// so to minimize the chance that voice rendering would advance the segment at the same time
	// we just do it twice here and hope that it sticks
	int repeats = (f->maxVoiceNum ? 2 : 1);
	while (repeats--)
	{
		v->ampenv.parameters.release = 0.0f;
		tsf_volume_envelope_release(v, f->outSampleRate);
		v->modenv.parameters.release = 0.0f;
		tsf_modulation_envelope_release(v);
	}
}

static void tsf_voice_calcpitchratio(struct tsf_voice* v, float pitchShift, float outSampleRate)
{
	double note = v->playingKey + v->region.transpose + v->region.tune / 100.0;
	double adjustedPitch = v->region.pitch_keycenter + (note - v->region.pitch_keycenter) * (v->region.pitch_keytrack / 100.0);
	if (pitchShift!=0.0f) adjustedPitch += (double)pitchShift;
	v->pitchInputTimecents = adjustedPitch * 100.0;
	v->pitchOutputFactor = v->region.sample_rate / (tsf_timecents2Secsd(v->region.pitch_keycenter * 100.0) * (double)outSampleRate);
}

static void tsf_voice_render_separate(tsf* f, struct tsf_voice* v, float* outputBufferL, float* outputBufferR, int numSamples)
{
	struct tsf_region* region = &v->region;
	float* input = v->soundbank->bankSamples;
	float* outL = outputBufferL;
	float* outR = outputBufferR;

	// Cache some values, to give them at least some chance of ending up in registers.
	TSF_BOOL updateModEnv = (region->modEnvToPitch || region->modEnvToFilterFc);
	TSF_BOOL updateModLFO = (v->modlfo.delta!=0.0f && (region->modLfoToPitch || region->modLfoToFilterFc || region->modLfoToVolume));
	TSF_BOOL updateVibLFO = (v->viblfo.delta!=0.0f && (region->vibLfoToPitch));
	TSF_BOOL isLooping    = (v->loopStart < v->loopEnd);
	unsigned int tmpLoopStart = v->loopStart, tmpLoopEnd = v->loopEnd;
	double tmpSampleEndDbl = (double)region->end, tmpLoopEndDbl = (double)tmpLoopEnd + 1.0;
	double tmpSourceSamplePosition = v->sourceSamplePosition;
	struct tsf_voice_lowpass tmpLowpass = v->lowpass;

	TSF_BOOL dynamicLowpass = (region->modLfoToFilterFc || region->modEnvToFilterFc);
	float tmpSampleRate = f->outSampleRate, tmpInitialFilterFc, tmpModLfoToFilterFc, tmpModEnvToFilterFc;

	TSF_BOOL dynamicPitchRatio = (region->modLfoToPitch || region->modEnvToPitch || region->vibLfoToPitch);
	double pitchRatio;
	float tmpModLfoToPitch, tmpVibLfoToPitch, tmpModEnvToPitch;

	TSF_BOOL dynamicGain = (region->modLfoToVolume != 0);
	float noteGain = 0, tmpModLfoToVolume;

	float *reverbInput = f->reverbInput;
	float *chorusInput = f->chorusInput;

	if (dynamicLowpass)
	{
		tmpInitialFilterFc = (float)region->initialFilterFc;
		tmpModLfoToFilterFc = (float)region->modLfoToFilterFc;
		 tmpModEnvToFilterFc = (float)region->modEnvToFilterFc;
	}
	else
	{
		tmpInitialFilterFc = 0;
		tmpModLfoToFilterFc = 0;
		 tmpModEnvToFilterFc = 0;
	}

	if (dynamicPitchRatio)
	{
		pitchRatio = 0;
		tmpModLfoToPitch = (float)region->modLfoToPitch;
		tmpVibLfoToPitch = (float)region->vibLfoToPitch;
		tmpModEnvToPitch = (float)region->modEnvToPitch;
	}
	else
	{
		pitchRatio = tsf_timecents2Secsd(v->pitchInputTimecents) * v->pitchOutputFactor;
		tmpModLfoToPitch = 0;
		tmpVibLfoToPitch = 0;
		tmpModEnvToPitch = 0;
	}

	if (dynamicGain) tmpModLfoToVolume = (float)region->modLfoToVolume * 0.1f;
	else
	{
		noteGain = tsf_decibelsToGain(-v->noteGainDB);
		tmpModLfoToVolume = 0;
	}

	while (numSamples)
	{
		float gainMono, gainLeft, gainRight, volumeExcursionCentibels = 0;
		int blockSamples = (numSamples > TSF_RENDER_EFFECTSAMPLEBLOCK ? TSF_RENDER_EFFECTSAMPLEBLOCK : numSamples);
		int samplesDone = blockSamples;
		numSamples -= blockSamples;

		float modEnvLevel = 0;
		if (updateModEnv)
		{
			modEnvLevel = tsf_modulation_envelope_get_value(v, v->voiceTime, TSF_FALSE);
		}

		if (dynamicLowpass)
		{
			float fres = tmpInitialFilterFc + v->modlfo.level * tmpModLfoToFilterFc + modEnvLevel * tmpModEnvToFilterFc;
			float lowpassFc = (fres <= 13500 ? tsf_cents2Hertz(fres) / tmpSampleRate : 1.0f);
			tmpLowpass.active = (lowpassFc < 0.499f);
			if (tmpLowpass.active) tsf_voice_lowpass_setup(&tmpLowpass, lowpassFc);
		}

		if (dynamicPitchRatio)
			pitchRatio = tsf_timecents2Secsd(v->pitchInputTimecents + (double)(v->modlfo.level * tmpModLfoToPitch + v->viblfo.level * tmpVibLfoToPitch + modEnvLevel * tmpModEnvToPitch)) * v->pitchOutputFactor;

		if (dynamicGain)
			noteGain = tsf_decibelsToGain(-v->noteGainDB - (v->modlfo.level * tmpModLfoToVolume));

		gainMono = noteGain;

		// Update LFOs.
		if (updateModLFO) tsf_voice_lfo_process(&v->modlfo, blockSamples);
		if (updateVibLFO) tsf_voice_lfo_process(&v->viblfo, blockSamples);

		if (updateVibLFO) volumeExcursionCentibels += -(v->viblfo.level) * v->viblfo.delta;
		if (updateModLFO) volumeExcursionCentibels += -(v->modlfo.level) * v->modlfo.delta;
		volumeExcursionCentibels -= v->resonanceOffset;

		float reverbGain = v->reverbEffectsSend * gainMono;
		float chorusGain = v->chorusEffectsSend * gainMono;

		switch (f->outputmode)
		{
			case TSF_STEREO_INTERLEAVED:
				gainLeft = gainMono * v->panFactorLeft;
				gainRight = gainMono * v->panFactorRight;
				while (blockSamples-- && tmpSourceSamplePosition < tmpSampleEndDbl)
				{
					unsigned int pos = (unsigned int)tmpSourceSamplePosition, nextPos = (pos >= tmpLoopEnd && isLooping ? tmpLoopStart : pos + 1);

					// Simple linear interpolation.
					float alpha = (float)(tmpSourceSamplePosition - pos), val = (input[pos] * (1.0f - alpha) + input[nextPos] * alpha);

					// Low-pass filter.
					if (tmpLowpass.active) val = tsf_voice_lowpass_process(&tmpLowpass, (double)val);

					val = tsf_volume_envelope_apply(v, val, volumeExcursionCentibels, f->envelopeSmoothingFactor);

					*reverbInput++ += val * reverbGain;
					*chorusInput++ += val * chorusGain;

					*outL++ += val * gainLeft;
					*outL++ += val * gainRight;

					// Next sample.
					tmpSourceSamplePosition += pitchRatio;
					if (tmpSourceSamplePosition >= tmpLoopEndDbl && isLooping) tmpSourceSamplePosition -= (tmpLoopEnd - tmpLoopStart + 1.0);
				}
				break;

			case TSF_STEREO_UNWEAVED:
				gainLeft = gainMono * v->panFactorLeft;
				gainRight = gainMono * v->panFactorRight;
				while (blockSamples-- && tmpSourceSamplePosition < tmpSampleEndDbl)
				{
					unsigned int pos = (unsigned int)tmpSourceSamplePosition, nextPos = (pos >= tmpLoopEnd && isLooping ? tmpLoopStart : pos + 1);

					// Simple linear interpolation.
					float alpha = (float)(tmpSourceSamplePosition - pos), val = (input[pos] * (1.0f - alpha) + input[nextPos] * alpha);

					// Low-pass filter.
					if (tmpLowpass.active) val = tsf_voice_lowpass_process(&tmpLowpass, (double)val);

					val = tsf_volume_envelope_apply(v, val, volumeExcursionCentibels, f->envelopeSmoothingFactor);

					*reverbInput++ += val * reverbGain;
					*chorusInput++ += val * chorusGain;

					*outL++ += val * gainLeft;
					*outR++ += val * gainRight;

					// Next sample.
					tmpSourceSamplePosition += pitchRatio;
					if (tmpSourceSamplePosition >= tmpLoopEndDbl && isLooping) tmpSourceSamplePosition -= (tmpLoopEnd - tmpLoopStart + 1.0);
				}
				break;

			case TSF_MONO:
				while (blockSamples-- && tmpSourceSamplePosition < tmpSampleEndDbl)
				{
					unsigned int pos = (unsigned int)tmpSourceSamplePosition, nextPos = (pos >= tmpLoopEnd && isLooping ? tmpLoopStart : pos + 1);

					// Simple linear interpolation.
					float alpha = (float)(tmpSourceSamplePosition - pos), val = (input[pos] * (1.0f - alpha) + input[nextPos] * alpha);

					// Low-pass filter.
					if (tmpLowpass.active) val = tsf_voice_lowpass_process(&tmpLowpass, (double)val);

					val = tsf_volume_envelope_apply(v, val, volumeExcursionCentibels, f->envelopeSmoothingFactor);

					*reverbInput++ += val * reverbGain;
					*chorusInput++ += val * chorusGain;

					*outL++ += val * gainMono;

					// Next sample.
					tmpSourceSamplePosition += pitchRatio;
					if (tmpSourceSamplePosition >= tmpLoopEndDbl && isLooping) tmpSourceSamplePosition -= (tmpLoopEnd - tmpLoopStart + 1.0);
				}
				break;
		}

		v->voiceTime += (float)samplesDone / f->outSampleRate;

		if (tmpSourceSamplePosition >= tmpSampleEndDbl || v->ampenv.state == TSF_SEGMENT_DONE)
		{
			tsf_voice_kill(v);
			return;
		}
	}

	v->sourceSamplePosition = tmpSourceSamplePosition;
	if (tmpLowpass.active || dynamicLowpass) v->lowpass = tmpLowpass;
}

static int tsf_read_hydra(struct tsf_hydra* hydra, struct tsf_riffchunk* chunkList, struct tsf_riffchunk* chunk, struct tsf_stream* stream, int* riff64)
{
	while (tsf_riffchunk_read(chunkList, chunk, stream, riff64))
	{
		#define HandleChunk(chunkName) (TSF_FourCCEquals(chunk->id, #chunkName) && !(chunk->size % chunkName##SizeInFile)) \
			{ \
				int num = chunk->size / chunkName##SizeInFile, i; \
				hydra->chunkName##Num = num; \
				hydra->chunkName##s = (struct tsf_hydra_##chunkName*)TSF_MALLOC(num * sizeof(struct tsf_hydra_##chunkName)); \
				if (!hydra->chunkName##s) return 0; \
				for (i = 0; i < num; ++i) tsf_hydra_read_##chunkName(&hydra->chunkName##s[i], stream); \
			}
		enum
		{
			phdrSizeInFile = 38, pbagSizeInFile =  4, pmodSizeInFile = 10,
			pgenSizeInFile =  4, instSizeInFile = 22, ibagSizeInFile =  4,
			imodSizeInFile = 10, igenSizeInFile =  4, shdrSizeInFile = 46
		};
		if      HandleChunk(phdr) else if HandleChunk(pbag) else if HandleChunk(pmod)
		else if HandleChunk(pgen) else if HandleChunk(inst) else if HandleChunk(ibag)
		else if HandleChunk(imod) else if HandleChunk(igen) else if HandleChunk(shdr)
		else stream->skip(stream->data, chunk->size);
		#undef HandleChunk
	}
	return 1;
}

TSFDEF tsf_soundbank* tsf_soundbank_load(struct tsf_stream* stream)
{
	tsf_soundbank* res = TSF_NULL;
	struct tsf_riffchunk chunkHead;
	struct tsf_riffchunk chunkList;
	struct tsf_hydra hydra, xydra;
	void* rawBuffer = TSF_NULL;
	void* raw24Buffer = TSF_NULL;
	float* floatBuffer = TSF_NULL;
	tsf_u64 smplCount = 0;

	struct tsf_version version, romVersion;
	struct tsf_sfe_version sfeVersion;
	struct tsf_sfe_flags *sfeFlags;

	int i, IsRiff64 = 0;

	tsf_read_init();

	if (!tsf_riffchunk_read(TSF_NULL, &chunkHead, stream, &IsRiff64) || (!TSF_FourCCEquals(chunkHead.id, "sfbk") &&
		// !TSF_FourCCEquals(chunkHead.id, "sfpk") &&
		!TSF_FourCCEquals(chunkHead.id, "sfen")))
	{
		//if (e) *e = TSF_INVALID_NOSF2HEADER;
		return res;
	}

	TSF_MEMSET(&version, 0, sizeof(version));
	TSF_MEMSET(&romVersion, 0, sizeof(romVersion));
	TSF_MEMSET(&sfeVersion, 0, sizeof(sfeVersion));
	sfeFlags = TSF_NULL;

	// Read hydra and locate sample data.
	TSF_MEMSET(&hydra, 0, sizeof(hydra));
	TSF_MEMSET(&xydra, 0, sizeof(xydra));
	while (tsf_riffchunk_read(&chunkHead, &chunkList, stream, &IsRiff64))
	{
		struct tsf_riffchunk chunk;
		if (TSF_FourCCEquals(chunkList.id, "INFO"))
		{
			while (tsf_riffchunk_read(&chunkList, &chunk, stream, &IsRiff64))
			{
				if (TSF_FourCCEquals(chunk.id, "ifil") ||
					TSF_FourCCEquals(chunk.id, "iver"))
				{
					if (chunk.size != 4) goto out_of_memory;
					TSF_BOOL ifil = TSF_FourCCEquals(chunk.id, "ifil");
					tsf_read_version(ifil ? &version : &romVersion, stream);
				}
				else if (TSF_FourCCEquals(chunk.id, "ISFe"))
				{
					struct tsf_riffchunk isfc;
					while (tsf_riffchunk_read(&chunk, &isfc, stream, &IsRiff64))
					{
						if (TSF_FourCCEquals(isfc.id, "SFty"))
						{
							char sftyStr[isfc.size];
							if (!stream->read(stream->data, sftyStr, isfc.size)) goto out_of_memory;
							if (TSF_STRNCMP(sftyStr, "SFe standard", isfc.size - 1) != 0) {
								// Unknown draft
							}
						}
						else if (TSF_FourCCEquals(isfc.id, "SFvx"))
						{
							if (isfc.size != sizeof(sfeVersion)) goto out_of_memory;
							tsf_read_sfe_version(&sfeVersion, stream);
							if (sfeVersion.major >= 5)
							{
								// SFe 5 or later, structurally incompatible
								goto out_of_memory;
							}
							else if (sfeVersion.major == 4 && sfeVersion.minor > 0)
							{
								// SFe 4.1 or later (4.x)
								version.major = 4;
								version.minor = 0;
							}
							else if (sfeVersion.major == 4 && sfeVersion.minor == 0)
							{
								version.major = sfeVersion.major;
								version.minor = sfeVersion.minor;

								if (!TSF_STRNCMP(sfeVersion.specType, "Draft", sizeof(sfeVersion.specType)))
								{
									// Draft specification
								}
								else
								{
									// Release candidate or final
								}
							}
							else
							{
								// If it's below version 4, we treat this as a non-fatal error condition and ignore it
								version.major = 4;
								version.minor = 0;
							}
						}
						else if (TSF_FourCCEquals(isfc.id, "flag"))
						{
							int flagsCount;
							if ((isfc.size % 6) != 0) goto out_of_memory;
							flagsCount = isfc.size / 6;
							sfeFlags = (struct tsf_sfe_flags *) TSF_MALLOC(flagsCount * sizeof(*sfeFlags));
							if (!sfeFlags) goto out_of_memory;
							for (i = 0; i < flagsCount; i++)
							{
								struct tsf_sfe_flags *f = &sfeFlags[i];
								stream->read(stream->data, &f->branch, 1);
								stream->read(stream->data, &f->leaf, 1);
								tsf_read_endian(stream, &f->flags, sizeof(f->flags));
							}
							for (i = 0; i < flagsCount; i++)
							{
								if (!tsf_sfe_check_flags(&sfeFlags[i]))
								{
									// warning, supported flags
								}
							}
						}
						else stream->skip(stream->data, isfc.size);
					}
				}
				else stream->skip(stream->data, chunk.size);
			}
		}
		else if (TSF_FourCCEquals(chunkList.id, "pdta"))
		{
			if (!tsf_read_hydra(&hydra, &chunkList, &chunk, stream, &IsRiff64)) goto out_of_memory;
		}
		else if (TSF_FourCCEquals(chunkList.id, "xdta"))
		{
			if (!tsf_read_hydra(&xydra, &chunkList, &chunk, stream, &IsRiff64)) goto out_of_memory;
		}
		else if (TSF_FourCCEquals(chunkList.id, "sdta"))
		{
			while (tsf_riffchunk_read(&chunkList, &chunk, stream, &IsRiff64))
			{
				if ((TSF_FourCCEquals(chunk.id, "smpl")
						#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
						|| TSF_FourCCEquals(chunk.id, "smpo")
						#endif
					) && !rawBuffer && !floatBuffer && chunk.size >= sizeof(short))
				{
					if (!tsf_load_samples(&rawBuffer, &floatBuffer, &smplCount, &chunk, stream)) goto out_of_memory;
				}
				else if ((TSF_FourCCEquals(chunk.id, "sm24"))
						&& rawBuffer && !raw24Buffer
						&& chunk.size <= (smplCount / sizeof(short)))
				{
					if (!tsf_load_samples_24(rawBuffer, &raw24Buffer, &floatBuffer, &smplCount, &chunk, stream)) goto out_of_memory;
				}
				else stream->skip(stream->data, chunk.size);
			}
		}
		else stream->skip(stream->data, chunkList.size);
	}
	if (!hydra.phdrs || !hydra.pbags || !hydra.pmods || !hydra.pgens || !hydra.insts || !hydra.ibags || !hydra.imods || !hydra.igens || !hydra.shdrs)
	{
		//if (e) *e = TSF_INVALID_INCOMPLETE;
	}
	else if ((xydra.phdrs && xydra.phdrNum != hydra.phdrNum) ||
			 (xydra.pbags && xydra.pbagNum != hydra.pbagNum) ||
			 (xydra.pmods && xydra.pmodNum != hydra.pmodNum) ||
			 (xydra.pgens && xydra.pgenNum != hydra.pgenNum) ||
			 (xydra.insts && xydra.instNum != hydra.instNum) ||
			 (xydra.ibags && xydra.ibagNum != hydra.ibagNum) ||
			 (xydra.imods && xydra.imodNum != hydra.imodNum) ||
			 (xydra.igens && xydra.igenNum != hydra.igenNum) ||
			 (xydra.shdrs && xydra.shdrNum != hydra.shdrNum))
	{
		//if (e) *e = TSF_INVALID_INCOMPLETE;
	}
	else if (!rawBuffer && !floatBuffer)
	{
		//if (e) *e = TSF_INVALID_NOSAMPLEDATA;
	}
	else
	{
		#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
		if (!floatBuffer && !tsf_decode_sf3_samples(rawBuffer, &floatBuffer, &smplCount, &hydra)) goto out_of_memory;
		#endif
		res = (tsf_soundbank*) TSF_MALLOC(sizeof(tsf_soundbank));
		if (res) TSF_MEMSET(res, 0, sizeof(tsf_soundbank));
		if (!res || !tsf_soundbank_load_presets(res, &hydra, &xydra, smplCount)) goto out_of_memory;
		res->bankSamples = floatBuffer;
		floatBuffer = TSF_NULL; // don't free below
		res->version = version;
		res->romVersion = romVersion;
	}
	if (0)
	{
		out_of_memory:
		TSF_FREE(res);
		res = TSF_NULL;
		//if (e) *e = TSF_OUT_OF_MEMORY;
	}
	TSF_FREE(hydra.phdrs); TSF_FREE(hydra.pbags); TSF_FREE(hydra.pmods);
	TSF_FREE(hydra.pgens); TSF_FREE(hydra.insts); TSF_FREE(hydra.ibags);
	TSF_FREE(hydra.imods); TSF_FREE(hydra.igens); TSF_FREE(hydra.shdrs);
	TSF_FREE(xydra.phdrs); TSF_FREE(xydra.pbags); TSF_FREE(xydra.pmods);
	TSF_FREE(xydra.pgens); TSF_FREE(xydra.insts); TSF_FREE(xydra.ibags);
	TSF_FREE(xydra.imods); TSF_FREE(xydra.igens); TSF_FREE(xydra.shdrs);
	TSF_FREE(rawBuffer);   TSF_FREE(raw24Buffer); TSF_FREE(floatBuffer);
	TSF_FREE(sfeFlags);
	return res;
}

TSFDEF void tsf_soundbank_close(tsf_soundbank* sb)
{
	if (!sb) return;
	struct tsf_preset *preset = sb->presets, *presetEnd = preset + sb->presetNum;
	for (; preset != presetEnd; preset++)
	{
		if (preset->regions)
		{
			int i;
			for (i = 0; i < preset->regionNum; i++)
				TSF_FREE(preset->regions[i].modulators);
		}
		TSF_FREE(preset->regions);
		TSF_FREE(preset->modulators);
	}
	TSF_FREE(sb->presets);
	TSF_FREE(sb->bankSamples);
	TSF_FREE(sb);
}

TSFDEF void tsf_close(tsf* f)
{
	int i;
	if (!f) return;
	tsf_reverb_free(f->reverb);
	tsf_chorus_free(f->chorus);
	TSF_FREE(f->modulators);
	TSF_FREE(f->channels);
	if (f->voices)
	{
		for (i = 0; i < f->voiceNum; i++)
			TSF_FREE(f->voices[i].modulators);
	}
	TSF_FREE(f->voices);
	TSF_FREE(f);
}

TSFDEF void tsf_reset(tsf* f)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && (v->ampenv.state < TSF_SEGMENT_RELEASE || v->ampenv.parameters.release!=0.0f))
			tsf_voice_endquick(f, v);
	if (f->channels) { TSF_FREE(f->channels); f->channels = TSF_NULL; }
	if (!f->reverb && f->outSampleRate) tsf_reverb_setup(&f->reverb, f->outSampleRate, TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK);
	if (!f->chorus && f->outSampleRate) tsf_chorus_setup(&f->chorus, f->outSampleRate, TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK);
	tsf_reverb_set_macro(f->reverb, 4); // Hall2
	tsf_chorus_set_macro(f->chorus, 2); // Chorus3
}

TSFDEF int tsf_get_presetindex(const tsf* f, const tsf_soundbank** outsb, int bank, int preset_number)
{
	const struct tsf_soundbank **sb, **sbEnd;
	int res;
	for (sb = f->soundbanks, sbEnd = sb ? sb + f->bankNum : TSF_NULL; sb != sbEnd; sb++)
	{
		res = tsf_soundbank_get_presetindex(*sb, bank, preset_number);
		if (res != -1)
		{
			*outsb = *sb;
			return res;
		}
	}
	return -1;
}

TSFDEF int tsf_soundbank_get_presetindex(const tsf_soundbank* sb, int bank, int preset_number)
{
	const struct tsf_preset *presets;
	int i, iMax;
	if (!sb) return -1;
	for (presets = sb->presets, i = 0, iMax = sb->presetNum; i < iMax; i++)
		if (presets[i].preset == preset_number && presets[i].bank == bank)
			return i;
	return -1;
}

TSFDEF int tsf_soundbank_get_presetcount(const tsf_soundbank* sb)
{
	return sb->presetNum;
}

TSFDEF const char* tsf_soundbank_get_presetname(const tsf_soundbank* sb, int preset)
{
	return (!sb || preset < 0 || preset >= sb->presetNum ? TSF_NULL : sb->presets[preset].presetName);
}

TSFDEF const char* tsf_bank_get_presetname(const tsf* f, int bank, int preset_number)
{
	const struct tsf_soundbank* sb = TSF_NULL;
	int res = tsf_get_presetindex(f, &sb, bank, preset_number);
	return tsf_soundbank_get_presetname(sb, res);
}

TSFDEF tsf* tsf_init(enum TSFOutputMode outputmode, int samplerate, float global_gain_db)
{
	tsf* f = (tsf *) TSF_MALLOC(sizeof(*f));
	if (!f) return f;
	TSF_MEMSET(f, 0, sizeof(*f));
	f->soundbanks = TSF_NULL;
	f->bankNum = 0;
	f->outputmode = outputmode;
	f->outSampleRate = (float)(samplerate >= 1 ? (float)samplerate : 44100.0f);
	f->globalGainDB = global_gain_db;
	if (!tsf_reverb_setup(&f->reverb, f->outSampleRate, TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK))
	{
		tsf_close(f);
		return TSF_NULL;
	}
	if (!tsf_chorus_setup(&f->chorus, f->outSampleRate, TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK))
	{
		tsf_close(f);
		return TSF_NULL;
	}
	tsf_init_curves();
	f->modNum = tsf_setup_default_modulators(&f->modulators);
	f->envelopeSmoothingFactor = VOLUME_ENVELOPE_SMOOTHING_FACTOR * (44100.0 / f->outSampleRate);
	return f;
}

TSFDEF int tsf_add_soundbank(tsf* f, const tsf_soundbank* insb)
{
	if (!f || !insb) return 0;
	const struct tsf_soundbank** sb = f->soundbanks, **sbEnd = sb ? sb + f->bankNum : TSF_NULL;
	for (; sb != sbEnd && *sb; sb++);
	if (sb == sbEnd) {
		f->bankNum += 4;
		sb = (const struct tsf_soundbank **) TSF_REALLOC(f->soundbanks, f->bankNum * sizeof(*sb));
		if (!sb) { f->bankNum -= 4; return 0; }
		sbEnd = sb + f->bankNum;
		TSF_MEMSET(sbEnd - 4, 0, sizeof(*sb) * 4);
		f->soundbanks = sb;
		sb = sbEnd - 4;
	}
	if (!*sb)
	{
		*sb = insb;
		return 1;
	}
	return 0;
}

TSFDEF void tsf_set_volume(tsf* f, float global_volume)
{
	f->globalGainDB = (global_volume == 1.0f ? 0 : -tsf_gainToDecibels(1.0f / global_volume));
}

TSFDEF int tsf_set_max_voices(tsf* f, int max_voices)
{
	int i = f->voiceNum;
	int newVoiceNum = (f->voiceNum > max_voices ? f->voiceNum : max_voices);
	struct tsf_voice *newVoices = (struct tsf_voice*)TSF_REALLOC(f->voices, newVoiceNum * sizeof(struct tsf_voice));
	if (!newVoices) return 0;
	f->voices = newVoices;
	f->voiceNum = f->maxVoiceNum = newVoiceNum;
	for (; i < max_voices; i++)
	{
		f->voices[i].playingPreset = -1;
		f->voices[i].modulators = TSF_NULL;
		f->voices[i].modNum = 0;
	}
	return 1;
}

TSFDEF int tsf_note_on(tsf* f, const tsf_soundbank* sb, int preset_index, int key, float vel)
{
	short midiVelocity = (short)(vel * 127);
	unsigned int voicePlayIndex;
	struct tsf_preset *preset;
	struct tsf_region *region, *regionEnd;

	if (!sb || preset_index < 0 || preset_index >= sb->presetNum) return 1;
	if (vel <= 0.0f) { tsf_note_off(f, sb, preset_index, key); return 1; }

	// Play all matching regions.
	voicePlayIndex = (signed)f->voicePlayIndex++;
	for (preset = &sb->presets[preset_index], region = preset->regions, regionEnd = region + preset->regionNum; region != regionEnd; region++)
	{
		struct tsf_voice *voice, *v, *vEnd; TSF_BOOL doLoop; float lowpassFilterQDB, lowpassFc;
		if (key < region->lokey || key > region->hikey || midiVelocity < region->lovel || midiVelocity > region->hivel) continue;

		voice = TSF_NULL;
		v     = f->voices;
		vEnd  = v ? v + f->voiceNum : TSF_NULL;
		if (region->group)
		{
			for (; v != vEnd; v++)
				if (v->playingPreset == preset_index && v->region.group == region->group) tsf_voice_endquick(f, v);
				else if (v->playingPreset == -1 && !voice) voice = v;
		}
		else for (; v != vEnd; v++) if (v->playingPreset == -1) { voice = v; break; }

		if (!voice)
		{
			if (f->maxVoiceNum)
			{
				// Voices have been pre-allocated and limited to a maximum, try to kill a voice off in its release envelope
				int bestKillReleaseSamplePos = -999999999;
				for (v = f->voices; v != vEnd; v++)
				{
					if (v->ampenv.state == TSF_SEGMENT_RELEASE)
					{
						// We're looking for the voice furthest into its release
						int releaseSamplesDone = tsf_volume_envelope_release_samples(&v->ampenv);
						if (releaseSamplesDone > bestKillReleaseSamplePos)
						{
							bestKillReleaseSamplePos = releaseSamplesDone;
							voice = v;
						}
					}
				}
				if (!voice)
					continue;
				tsf_voice_kill(voice);
			}
			else
			{
				// Allocate more voices so we don't need to kill one off.
				struct tsf_voice* newVoices;
				f->voiceNum += 4;
				newVoices = (struct tsf_voice*)TSF_REALLOC(f->voices, f->voiceNum * sizeof(struct tsf_voice));
				if (!newVoices) { f->voiceNum -= 4; return 0; }
				f->voices = newVoices;
				voice = &f->voices[f->voiceNum - 4];
				voice[1].playingPreset = voice[2].playingPreset = voice[3].playingPreset = -1;
				voice[1].modulators = voice[2].modulators = voice[3].modulators = TSF_NULL;
				voice[1].modNum = voice[2].modNum = voice[3].modNum = 0;
			}
		}

		voice->soundbank = sb;
		voice->preset = preset;
		voice->region = *region;
		voice->playingPreset = preset_index;
		voice->playingKey = key;
		voice->playingVelocity = midiVelocity;
		voice->playIndex = (unsigned)voicePlayIndex;
		voice->heldSustain = 0;
		voice->noteGainDB = f->globalGainDB - region->attenuation;
		voice->modulatedGenerators = voice->generators = region->generators;
		voice->modulators = TSF_NULL;
		voice->modNum = 0;
		voice->voiceTime = 0;

		if (f->channels)
		{
			f->channels->setupVoice(f, voice);
		}
		else
		{
			tsf_voice_calcpitchratio(voice, 0, f->outSampleRate);
			// The SFZ spec is silent about the pan curve, but a 3dB pan law seems common. This sqrt() curve matches what Dimension LE does; Alchemy Free seems closer to sin(adjustedPan * pi/2).
			voice->panFactorLeft     = TSF_SQRTF(0.5f - region->pan);
			voice->panFactorRight    = TSF_SQRTF(0.5f + region->pan);
			voice->reverbEffectsSend = 0;
			voice->chorusEffectsSend = 0;
		}

		// Offset/end.
		voice->sourceSamplePosition = region->offset;

		// Loop.
		doLoop = (region->loop_mode != TSF_LOOPMODE_NONE && region->loop_start < region->loop_end);
		voice->loopStart = (doLoop ? region->loop_start : 0);
		voice->loopEnd = (doLoop ? region->loop_end : 0);

		// Setup envelopes.
		tsf_volume_envelope_recalculate(voice, f->outSampleRate);
		tsf_modulation_envelope_recalculate(voice);

		// Setup lowpass filter.
		lowpassFc = (region->initialFilterFc <= 13500 ? tsf_cents2Hertz((float)region->initialFilterFc) / f->outSampleRate : 1.0f);
		lowpassFilterQDB = (float)region->initialFilterQ / 10.0f;
		voice->lowpass.QInv = 1.0 / TSF_POW(10.0, ((double)lowpassFilterQDB / 20.0));
		voice->lowpass.z1 = voice->lowpass.z2 = 0;
		voice->lowpass.active = (lowpassFc < 0.499f);
		if (voice->lowpass.active) tsf_voice_lowpass_setup(&voice->lowpass, lowpassFc);

		// Setup LFO filters.
		tsf_voice_lfo_setup(&voice->modlfo, region->delayModLFO, region->freqModLFO, f->outSampleRate);
		tsf_voice_lfo_setup(&voice->viblfo, region->delayVibLFO, region->freqVibLFO, f->outSampleRate);

		// Setup pressure
		voice->pressure = 0;
	}
	return 1;
}

TSFDEF int tsf_bank_note_on(tsf* f, int bank, int preset_number, int key, float vel)
{
	const struct tsf_soundbank* sb = TSF_NULL;
	int preset_index = tsf_get_presetindex(f, &sb, bank, preset_number);
	if (preset_index == -1) return 0;
	return tsf_note_on(f, sb, preset_index, key, vel);
}

TSFDEF void tsf_note_off(tsf* f, const tsf_soundbank* sb, int preset_index, int key)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL, *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
	for (; v != vEnd; v++)
	{
		//Find the first and last entry in the voices list with matching preset, key and look up the smallest play index
		if (v->soundbank != sb || v->playingPreset != preset_index || v->playingKey != key || v->ampenv.state >= TSF_SEGMENT_RELEASE) continue;
		else if (!vMatchFirst || v->playIndex < vMatchFirst->playIndex) vMatchFirst = vMatchLast = v;
		else if (v->playIndex == vMatchFirst->playIndex) vMatchLast = v;
	}
	if (!vMatchFirst) return;
	for (v = vMatchFirst; v <= vMatchLast; v++)
	{
		//Stop all voices with matching preset, key and the smallest play index which was enumerated above
		if (v != vMatchFirst && v != vMatchLast &&
			(v->playIndex != vMatchFirst->playIndex || v->soundbank != sb || v->playingPreset != preset_index || v->playingKey != key || v->ampenv.state >= TSF_SEGMENT_RELEASE)) continue;
		tsf_voice_end(f, v);
	}
}

TSFDEF int tsf_bank_note_off(tsf* f, int bank, int preset_number, int key)
{
	const struct tsf_soundbank* sb = TSF_NULL;
	int preset_index = tsf_get_presetindex(f, &sb, bank, preset_number);
	if (preset_index == -1) return 0;
	tsf_note_off(f, sb, preset_index, key);
	return 1;
}

TSFDEF void tsf_note_off_all(tsf* f, int quick)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	if (quick)
	{
		for (; v != vEnd; v++) if (v->playingPreset != -1 && v->ampenv.state < TSF_SEGMENT_RELEASE)
			tsf_voice_endquick(f, v);
	}
	else
	{
		for (; v != vEnd; v++) if (v->playingPreset != -1 && v->ampenv.state < TSF_SEGMENT_RELEASE)
			tsf_voice_end(f, v);
	}
}

TSFDEF int tsf_active_voice_count(tsf* f)
{
	int count = 0;
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++) if (v->playingPreset != -1) count++;
	return count;
}

TSFDEF void tsf_render_short(tsf* f, short* buffer, int samples, int flag_mixing)
{
	float outputSamples[TSF_RENDER_SHORTBUFFERBLOCK];
	int channels = (f->outputmode == TSF_MONO ? 1 : 2), maxChannelSamples = TSF_RENDER_SHORTBUFFERBLOCK / channels;
	while (samples > 0)
	{
		int channelSamples = (samples > maxChannelSamples ? maxChannelSamples : samples);
		short* bufferEnd = buffer + channelSamples * channels;
		float *floatSamples = outputSamples;
		tsf_render_float(f, floatSamples, channelSamples, TSF_FALSE);
		samples -= channelSamples;

		if (flag_mixing)
			while (buffer != bufferEnd)
			{
				float v = *floatSamples++;
				int vi = *buffer + (v < -1.00004566f ? (int)-32768 : (v > 1.00001514f ? (int)32767 : (int)(v * 32767.5f)));
				*buffer++ = (vi < -32768 ? (short)-32768 : (vi > 32767 ? (short)32767 : (short)vi));
			}
		else
			while (buffer != bufferEnd)
			{
				float v = *floatSamples++;
				*buffer++ = (v < -1.00004566f ? (short)-32768 : (v > 1.00001514f ? (short)32767 : (short)(v * 32767.5f)));
			}
	}
}

static void tsf_effects_clear(tsf* f)
{
	if (f && f->channels)
	{
		TSF_MEMSET(f->reverbInput, 0, sizeof(f->reverbInput));
		TSF_MEMSET(f->chorusInput, 0, sizeof(f->chorusInput));
	}
}

static void tsf_effects_process(tsf* f, float* bufferL, float* bufferR, int samples, int channels)
{
	if (!f->channels) return;
	tsf_chorus_process(f->chorus, f->chorusInput, bufferL, bufferR, f->reverbInput, TSF_NULL, samples, channels);
	tsf_reverb_process(f->reverb, f->reverbInput, bufferL, bufferR, samples, channels);
}

static void tsf_render_voices_separate(tsf* f, float* bufferL, float* bufferR, int samples)
{
	int channels = f->outputmode == TSF_STEREO_INTERLEAVED ? 2 : 1;
	while (samples)
	{
		const int maxBatch = TSF_RENDER_GLOBALEFFECTSAMPLEBLOCK;
		const int samplesBatch = samples < maxBatch ? samples : maxBatch;
		struct tsf_voice *v, *vEnd;
		tsf_effects_clear(f);
		for (v = f->voices, vEnd = v ? v + f->voiceNum : TSF_NULL; v != vEnd; v++)
			if (v->playingPreset != -1)
				tsf_voice_render_separate(f, v, bufferL, bufferR, samplesBatch);
		tsf_effects_process(f, bufferL, bufferR, samplesBatch, channels);
		bufferL += samplesBatch * channels;
		if (bufferR) bufferR += samplesBatch;
		samples -= samplesBatch;
	}
}

static void tsf_render_voices(tsf* f, float* buffer, int samples)
{
	tsf_render_voices_separate(f, buffer, f->outputmode == TSF_STEREO_UNWEAVED ? buffer + samples : TSF_NULL, samples);
}

TSFDEF void tsf_render_float(tsf* f, float* buffer, int samples, int flag_mixing)
{
	if (!flag_mixing) TSF_MEMSET(buffer, 0, (f->outputmode == TSF_MONO ? 1 : 2) * sizeof(float) * (unsigned)samples);
	tsf_render_voices(f, buffer, samples);
}

TSFDEF void tsf_render_float_separate(tsf* f, float* bufferL, float* bufferR, int samples, int flag_mixing)
{
	if (!flag_mixing)
	{
		TSF_MEMSET(bufferL, 0, sizeof(float) * samples);
		TSF_MEMSET(bufferR, 0, sizeof(float) * samples);
	}
	tsf_render_voices_separate(f, bufferL, bufferR, samples);
}

static void tsf_channel_setup_voice(tsf* f, struct tsf_voice* v)
{
	struct tsf_channel* c = &f->channels->channels[f->channels->activeChannel];
	float newpan = v->region.pan + c->panOffset;
	float reverb = (v->region.reverb / 1000.0) * ((float)c->reverb / 127.0);
	float chorus = (v->region.chorus / 1000.0) * ((float)c->chorus / 127.0);
	v->playingChannel = f->channels->activeChannel;
	v->noteGainDB += c->gainDB;
	tsf_voice_calcpitchratio(v, (c->pitchWheel == 8192 ? c->tuning : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f) - c->pitchRange + c->tuning)), f->outSampleRate);
	if      (newpan <= -0.5f) { v->panFactorLeft = 1.0f; v->panFactorRight = 0.0f; }
	else if (newpan >=  0.5f) { v->panFactorLeft = 0.0f; v->panFactorRight = 1.0f; }
	else { v->panFactorLeft = TSF_SQRTF(0.5f - newpan); v->panFactorRight = TSF_SQRTF(0.5f + newpan); }
	v->reverbEffectsSend = reverb;
	v->chorusEffectsSend = chorus;

	tsf_modulators_append(&v->modulators, &v->modNum, v->region.modulators, v->region.modNum);
	tsf_modulators_append(&v->modulators, &v->modNum, v->preset->modulators, v->preset->modNum);
	tsf_modulators_append(&v->modulators, &v->modNum, f->modulators, f->modNum);

	tsf_modulators_compute(f, c, v, -1, 0);

	v->ampenv.state = TSF_SEGMENT_NONE;
	v->modenv.state = TSF_SEGMENT_NONE;

	tsf_volume_envelope_recalculate(v, f->outSampleRate);
	tsf_modulation_envelope_recalculate(v);
}

static struct tsf_channel* tsf_channel_init(tsf* f, int channel)
{
	int i;
	if (f->channels && channel < f->channels->channelNum) return &f->channels->channels[channel];
	if (!f->channels)
	{
		f->channels = (struct tsf_channels*)TSF_MALLOC(sizeof(struct tsf_channels) + sizeof(struct tsf_channel) * channel);
		if (!f->channels) return TSF_NULL;
		f->channels->setupVoice = &tsf_channel_setup_voice;
		f->channels->channelNum = 0;
		f->channels->activeChannel = 0;
	}
	else
	{
		struct tsf_channels *newChannels = (struct tsf_channels*)TSF_REALLOC(f->channels, sizeof(struct tsf_channels) + sizeof(struct tsf_channel) * channel);
		if (!newChannels) return TSF_NULL;
		f->channels = newChannels;
	}
	i = f->channels->channelNum;
	f->channels->channelNum = channel + 1;
	for (; i <= channel; i++)
	{
		struct tsf_channel* c = &f->channels->channels[i];
		c->soundbank = f->soundbanks ? f->soundbanks[0] : TSF_NULL;
		c->presetIndex = c->bank = 0;
		c->pitchWheel = c->midiPan = 8192;
		c->midiVolume = c->midiExpression = 16383;
		c->pressure = 0;
		c->midiRPN = 0xFFFF;
		c->midiData = c->sustain = 0;
		c->panOffset = 0.0f;
		c->gainDB = 0.0f;
		c->pitchRange = 2.0f;
		c->tuning = 0.0f;
		c->reverb = 40;
		c->chorus = 40;
		c->controllers[pitchWheel + NON_CC_INDEX_OFFSET] = 8192;
		c->controllers[pitchWheelRange + NON_CC_INDEX_OFFSET] = 2 * 128;
		c->controllers[generatorPan + NON_CC_INDEX_OFFSET] = 8192;
		c->controllers[mainVolume] = c->controllers[expressionController] = 16383;
		c->controllers[reverbDepth] = 40 << 7;
		c->controllers[chorusDepth] = 40 << 7;
	}
	return &f->channels->channels[channel];
}

static void tsf_channel_applypitch(tsf* f, int channel, struct tsf_channel* c)
{
	struct tsf_voice *v, *vEnd;
	float pitchShift = (c->pitchWheel == 8192 ? c->tuning : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f) - c->pitchRange + c->tuning));
	for (v = f->voices, vEnd = v ? v + f->voiceNum : TSF_NULL; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			tsf_voice_calcpitchratio(v, pitchShift, f->outSampleRate);
}

TSFDEF int tsf_channel_set_presetindex(tsf* f, int channel, const tsf_soundbank* sb, int preset_index)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	c->soundbank = sb;
	c->presetIndex = (unsigned short)preset_index;
	return 1;
}

TSFDEF int tsf_channel_set_presetnumber(tsf* f, int channel, int preset_number, int flag_mididrums)
{
	const struct tsf_soundbank* sb = TSF_NULL;
	int preset_index;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (flag_mididrums)
	{
		preset_index = tsf_get_presetindex(f, &sb, 128 | (c->bank & 0x7FFF), preset_number);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, &sb, 128, preset_number);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, &sb, 128, 0);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, &sb, (c->bank & 0x7FFF), preset_number);
	}
	else preset_index = tsf_get_presetindex(f, &sb, (c->bank & 0x7FFF), preset_number);
	if (preset_index == -1) preset_index = tsf_get_presetindex(f, &sb, 0, preset_number);
	if (preset_index != -1)
	{
		c->soundbank = sb;
		c->presetIndex = (unsigned short)preset_index;
		return 1;
	}
	return 0;
}

TSFDEF int tsf_channel_set_bank(tsf* f, int channel, int bank)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	c->bank = (unsigned short)bank;
	return 1;
}

TSFDEF int tsf_channel_set_bank_preset(tsf* f, int channel, int bank, int preset_number)
{
	const struct tsf_soundbank* sb = TSF_NULL;
	int preset_index;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	preset_index = tsf_get_presetindex(f, &sb, bank, preset_number);
	if (preset_index == -1) return 0;
	c->soundbank = sb;
	c->presetIndex = (unsigned short)preset_index;
	c->bank = (unsigned short)bank;
	return 1;
}

TSFDEF int tsf_channel_set_pan(tsf* f, int channel, float pan)
{
	struct tsf_voice *v, *vEnd;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	for (v = f->voices, vEnd = v ? v + f->voiceNum : TSF_NULL; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
		{
			float newpan = v->region.pan + pan - 0.5f;
			if      (newpan <= -0.5f) { v->panFactorLeft = 1.0f; v->panFactorRight = 0.0f; }
			else if (newpan >=  0.5f) { v->panFactorLeft = 0.0f; v->panFactorRight = 1.0f; }
			else { v->panFactorLeft = TSF_SQRTF(0.5f - newpan); v->panFactorRight = TSF_SQRTF(0.5f + newpan); }
		}
	c->panOffset = pan - 0.5f;
	return 1;
}

TSFDEF int tsf_channel_set_volume(tsf* f, int channel, float volume)
{
	float gainDB = tsf_gainToDecibels(volume), gainDBChange;
	struct tsf_voice *v, *vEnd;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (gainDB == c->gainDB) return 1;
	for (v = f->voices, vEnd = v ? v + f->voiceNum : TSF_NULL, gainDBChange = gainDB - c->gainDB; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			v->noteGainDB += gainDBChange;
	c->gainDB = gainDB;
	return 1;
}

TSFDEF int tsf_channel_set_pitchwheel(tsf* f, int channel, int pitch_wheel)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->pitchWheel == pitch_wheel) return 1;
	c->pitchWheel = (unsigned short)pitch_wheel;
	tsf_channel_applypitch(f, channel, c);
	c->controllers[NON_CC_INDEX_OFFSET + pitchWheel] = pitch_wheel;
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			tsf_modulators_compute(f, c, v, 0, pitchWheel);
	return 1;
}

TSFDEF int tsf_channel_set_pitchrange(tsf* f, int channel, float pitch_range)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->pitchRange == pitch_range) return 1;
	c->pitchRange = pitch_range;
	if (c->pitchWheel != 8192) tsf_channel_applypitch(f, channel, c);
	c->controllers[NON_CC_INDEX_OFFSET + pitchWheelRange] = ((int)(TSF_FMOD(pitch_range, 1.0) * 100.0)) + ((int)pitch_range) * 128;
	return 1;
}

TSFDEF int tsf_channel_set_tuning(tsf* f, int channel, float tuning)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->tuning == tuning) return 1;
	c->tuning = tuning;
	tsf_channel_applypitch(f, channel, c);
	return 1;
}

TSFDEF int tsf_channel_set_sustain(tsf* f, int channel, int flag_sustain)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (!c->sustain == !flag_sustain) return 1;
	c->sustain = (unsigned short)(flag_sustain != 0);
	//Turning on sustain does no action now, just starts note_off behaving differently
	if (flag_sustain) return 1;
	//Turning off sustain, actually end voices that got a note_off and were set to heldSustain status
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && v->ampenv.state < TSF_SEGMENT_RELEASE && v->heldSustain)
			tsf_voice_end(f, v);
	return 1;
}

TSFDEF int tsf_channel_set_reverb_send(tsf* f, int channel, int reverb_send)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->reverb == reverb_send) return 1;
	c->reverb = (unsigned char)(reverb_send);
	const float reverbSend = ((float)c->reverb / 127.0);
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
		{
			const float reverb = (v->region.reverb / 1000.0) * reverbSend;
			v->reverbEffectsSend = reverb;
		}
	return 1;
}

TSFDEF int tsf_channel_set_chorus_send(tsf* f, int channel, int chorus_send)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->chorus == chorus_send) return 1;
	c->chorus = (unsigned char)(chorus_send);
	const float chorusSend = ((float)c->chorus / 127.0);
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
		{
			const float chorus = (v->region.chorus / 1000.0) * chorusSend;
			v->chorusEffectsSend = chorus;
		}
	return 1;
}

TSFDEF int tsf_channel_note_on(tsf* f, int channel, int key, float vel)
{
	if (!f->channels || channel >= f->channels->channelNum) return 1;
	f->channels->activeChannel = channel;
	if (!vel)
	{
		tsf_channel_note_off(f, channel, key);
		return 1;
	}
	return tsf_note_on(f, f->channels->channels[channel].soundbank, f->channels->channels[channel].presetIndex, key, vel);
}

TSFDEF void tsf_channel_note_off(tsf* f, int channel, int key)
{
	unsigned sustain;
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL, *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
	for (; v != vEnd; v++)
	{
		//Find the first and last entry in the voices list with matching channel, key and look up the smallest play index
		if (v->playingPreset == -1 || v->playingChannel != channel || v->playingKey != key || v->ampenv.state >= TSF_SEGMENT_RELEASE || v->heldSustain) continue;
		else if (!vMatchFirst || v->playIndex < vMatchFirst->playIndex) vMatchFirst = vMatchLast = v;
		else if (v->playIndex == vMatchFirst->playIndex) vMatchLast = v;
	}
	if (!vMatchFirst) return;
	for (sustain = f->channels->channels[channel].sustain, v = vMatchFirst; v <= vMatchLast; v++)
	{
		//Stop all voices with matching channel, key and the smallest play index which was enumerated above
		if (v != vMatchFirst && v != vMatchLast &&
			(v->playIndex != vMatchFirst->playIndex || v->playingPreset == -1 || v->playingChannel != channel || v->playingKey != key || v->ampenv.state >= TSF_SEGMENT_RELEASE)) continue;
		//Don't turn off if sustain is active, just mark as held by sustain so we don't forget it
		if (sustain)
			v->heldSustain = 1;
		else
			tsf_voice_end(f, v);
	}
}

TSFDEF void tsf_channel_note_off_all(tsf* f, int channel)
{
	//Ignore sustain channel settings, note_off_all overrides
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && v->ampenv.state < TSF_SEGMENT_RELEASE)
			tsf_voice_end(f, v);
}

TSFDEF void tsf_channel_sounds_off_all(tsf* f, int channel)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && (v->ampenv.state < TSF_SEGMENT_RELEASE || v->ampenv.parameters.release!=0.0f))
			tsf_voice_endquick(f, v);
}

TSFDEF int tsf_channel_midi_control(tsf* f, int channel, int controller, int control_value)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	struct tsf_channel* c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (controller < 32) c->controllers[controller] = control_value << 7;
	else if (controller < 64) c->controllers[controller - 32] |= control_value;
	else if (controller < 128) c->controllers[controller] = control_value << 7;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			tsf_modulators_compute(f, c, v, 1, controller >= 32 && controller < 64 ? controller - 32 : controller);

	switch (controller)
	{
		case   7 /*VOLUME_MSB*/      : c->midiVolume     = (unsigned short)((c->midiVolume     & 0x7F  ) | (control_value << 7)); goto TCMC_SET_VOLUME;
		case  39 /*VOLUME_LSB*/      : c->midiVolume     = (unsigned short)((c->midiVolume     & 0x3F80) |  control_value);       goto TCMC_SET_VOLUME;
		case  11 /*EXPRESSION_MSB*/  : c->midiExpression = (unsigned short)((c->midiExpression & 0x7F  ) | (control_value << 7)); goto TCMC_SET_VOLUME;
		case  43 /*EXPRESSION_LSB*/  : c->midiExpression = (unsigned short)((c->midiExpression & 0x3F80) |  control_value);       goto TCMC_SET_VOLUME;
		case  10 /*PAN_MSB*/         : c->midiPan        = (unsigned short)((c->midiPan        & 0x7F  ) | (control_value << 7)); goto TCMC_SET_PAN;
		case  42 /*PAN_LSB*/         : c->midiPan        = (unsigned short)((c->midiPan        & 0x3F80) |  control_value);       goto TCMC_SET_PAN;
		case   6 /*DATA_ENTRY_MSB*/  : c->midiData       = (unsigned short)((c->midiData       & 0x7F)   | (control_value << 7)); goto TCMC_SET_DATA;
		case  38 /*DATA_ENTRY_LSB*/  : c->midiData       = (unsigned short)((c->midiData       & 0x3F80) |  control_value);       goto TCMC_SET_DATA;
		case   0 /*BANK_SELECT_MSB*/ : c->bank = (unsigned short)(0x8000 | control_value); return 1; //bank select MSB alone acts like LSB
		case  32 /*BANK_SELECT_LSB*/ : c->bank = (unsigned short)((c->bank & 0x8000 ? ((c->bank & 0x7F) << 7) : 0) | control_value); return 1;
		case 101 /*RPN_MSB*/         : c->midiRPN = (unsigned short)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN) & 0x7F  ) | (control_value << 7)); return 1;
		case 100 /*RPN_LSB*/         : c->midiRPN = (unsigned short)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN) & 0x3F80) |  control_value); return 1;
		case  98 /*NRPN_LSB*/        : c->midiRPN = 0xFFFF; return 1;
		case  99 /*NRPN_MSB*/        : c->midiRPN = 0xFFFF; return 1;
		case  64 /*SUSTAIN*/         : tsf_channel_set_sustain(f, channel, (int)(control_value >= 64)); return 1;
		case  91 /*REVERB_SEND*/     : tsf_channel_set_reverb_send(f, channel, control_value); return 1;
		case  93 /*CHORUS_SEND*/     : tsf_channel_set_chorus_send(f, channel, control_value); return 1;
		case 120 /*ALL_SOUND_OFF*/   : tsf_channel_sounds_off_all(f, channel); return 1;
		case 123 /*ALL_NOTES_OFF*/   : tsf_channel_note_off_all(f, channel);   return 1;
		case 121 /*ALL_CTRL_OFF*/    :
			c->midiVolume = c->midiExpression = 16383;
			c->midiPan = 8192;
			c->bank = 0;
			c->midiRPN = 0xFFFF;
			c->midiData = 0;
			tsf_channel_set_volume(f, channel, 1.0f);
			tsf_channel_set_pan(f, channel, 0.5f);
			tsf_channel_set_pitchrange(f, channel, 2.0f);
			tsf_channel_set_pitchwheel(f, channel, 8192);
			tsf_channel_set_tuning(f, channel, 0);
			tsf_channel_set_pitchwheel(f, channel, 8192);
			tsf_channel_set_reverb_send(f, channel, 40);
			tsf_channel_set_chorus_send(f, channel, 40);
			return 1;
	}
	return 1;
TCMC_SET_VOLUME:
	//Raising to the power of 3 seems to result in a decent sounding volume curve for MIDI
	tsf_channel_set_volume(f, channel, TSF_POWF((c->midiVolume / 16383.0f) * (c->midiExpression / 16383.0f), 3.0f));
	return 1;
TCMC_SET_PAN:
	tsf_channel_set_pan(f, channel, c->midiPan / 16383.0f);
	return 1;
TCMC_SET_DATA:
	if      (c->midiRPN == 0) { c->controllers[NON_CC_INDEX_OFFSET + pitchWheelRange] = c->midiData; tsf_channel_set_pitchrange(f, channel, (c->midiData >> 7) + 0.01f * (c->midiData & 0x7F)); }
	else if (c->midiRPN == 1) tsf_channel_set_tuning(f, channel, (float)((int)c->tuning) + ((float)c->midiData - 8192.0f) / 8192.0f); //fine tune
	else if (c->midiRPN == 2 && controller == 6) tsf_channel_set_tuning(f, channel, ((float)control_value - 64.0f) + (c->tuning - (float)((int)c->tuning))); //coarse tune
	return 1;
}

TSFDEF int tsf_channel_set_channel_pressure(tsf* f, int channel, int value)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	if (c->pressure == value) return 1;
	c->pressure = value;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			tsf_modulators_compute(f, c, v, 0, channelPressure);
	return 1;
}

TSFDEF int tsf_channel_set_key_pressure(tsf* f, int channel, int key, int value)
{
	struct tsf_voice *v = f->voices, *vEnd = v ? v + f->voiceNum : TSF_NULL;
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (!c) return 0;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && v->playingKey == key)
		{
			v->pressure = value;
			tsf_modulators_compute(f, c, v, 0, polyPressure);
		}
	return 1;
}

TSFDEF int tsf_channel_get_preset_index(tsf* f, const tsf_soundbank** sb, int channel)
{
	if (f->channels && channel < f->channels->channelNum)
	{
		*sb = f->channels->channels[channel].soundbank;
		return f->channels->channels[channel].presetIndex;
	}
	*sb = TSF_NULL;
	return 0;
}

TSFDEF int tsf_channel_get_preset_bank(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? (f->channels->channels[channel].bank & 0x7FFF) : 0);
}

TSFDEF int tsf_channel_get_preset_number(tsf* f, int channel)
{
	if (f->channels && channel < f->channels->channelNum)
	{
		const struct tsf_soundbank* sb = f->channels->channels[channel].soundbank;
		return sb->presets[f->channels->channels[channel].presetIndex].preset;
	}
	return 0;
}

TSFDEF float tsf_channel_get_pan(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].panOffset - 0.5f : 0.5f);
}

TSFDEF float tsf_channel_get_volume(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? tsf_decibelsToGain(-(f->channels->channels[channel].gainDB)) : 1.0f);
}

TSFDEF int tsf_channel_get_pitchwheel(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].pitchWheel : 8192);
}

TSFDEF float tsf_channel_get_pitchrange(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].pitchRange : 2.0f);
}

TSFDEF float tsf_channel_get_tuning(tsf* f, int channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].tuning : 0.0f);
}

#ifdef __cplusplus
}
#endif

#endif //TSF_IMPLEMENTATION
