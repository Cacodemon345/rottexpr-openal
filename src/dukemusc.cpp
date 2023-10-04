/*
 * A reimplementation of Jim Dose's FX_MAN routines, using  SDL_mixer 1.2.
 *   Whee. FX_MAN is also known as the "Apogee Sound System", or "ASS" for
 *   short. How strangely appropriate that seems.
 *
 * Written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

/* Cacodemon345: Changed and rewritten for ZMusic. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <filesystem>

#define AL_ALEXT_PROTOTYPES 1
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <zmusic.h>

#define ROTT

#define cdecl

extern "C"
{
#ifdef ROTT
#include "rt_def.h"      // ROTT music hack
#include "rt_cfg.h"      // ROTT music hack
#include "rt_util.h"     // ROTT music hack
#endif
#include "music.h"
}

#define __FX_TRUE  true
#define __FX_FALSE false

#define DUKESND_DEBUG       "DUKESND_DEBUG"

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

int MUSIC_ErrorCode = MUSIC_Ok;

static char warningMessage[80];
static char errorMessage[80];
static FILE *debug_file = NULL;
static int initialized_debugging = 0;

static ALuint buffer, source;
static ZMusic_MusicStream stream = nullptr;

// This gets called all over the place for information and debugging messages.
//  If the user set the DUKESND_DEBUG environment variable, the messages
//  go to the file that is specified in that variable. Otherwise, they
//  are ignored for the expense of the function call. If DUKESND_DEBUG is
//  set to "-" (without the quotes), then the output goes to stdout.
static void musdebug(const char *fmt, ...)
{
    va_list ap;

    if (debug_file)
    {
        fprintf(debug_file, "DUKEMUS: ");
        va_start(ap, fmt);
        vfprintf(debug_file, fmt, ap);
        va_end(ap);
        fprintf(debug_file, "\n");
        fflush(debug_file);
    } // if
} // musdebug

static void init_debugging(void)
{
    const char *envr;

    if (initialized_debugging)
        return;

    envr = getenv(DUKESND_DEBUG);
    if (envr != NULL)
    {
        if (strcmp(envr, "-") == 0)
            debug_file = stdout;
        else
            debug_file = fopen(envr, "w");

        if (debug_file == NULL)
            fprintf(stderr, "DUKESND: -WARNING- Could not open debug file!\n");
        else
            setbuf(debug_file, NULL);
    } // if

    initialized_debugging = 1;
} // init_debugging

static void setWarningMessage(const char *msg)
{
    strncpy(warningMessage, msg, sizeof (warningMessage));
    // strncpy() doesn't add the null char if there isn't room...
    warningMessage[sizeof (warningMessage) - 1] = '\0';
    musdebug("Warning message set to [%s].", warningMessage);
} // setErrorMessage


static void setErrorMessage(const char *msg)
{
    strncpy(errorMessage, msg, sizeof (errorMessage));
    // strncpy() doesn't add the null char if there isn't room...
    errorMessage[sizeof (errorMessage) - 1] = '\0';
    musdebug("Error message set to [%s].", errorMessage);
} // setErrorMessage

// The music functions...

const char *MUSIC_ErrorString(int ErrorNumber)
{
    switch (ErrorNumber)
    {
    case MUSIC_Warning:
        return(warningMessage);

    case MUSIC_Error:
        return(errorMessage);

    case MUSIC_Ok:
        return("OK; no error.");

    case MUSIC_ASSVersion:
        return("Incorrect sound library version.");

    case MUSIC_SoundCardError:
        return("General sound card error.");

    case MUSIC_InvalidCard:
        return("Invalid sound card.");

    case MUSIC_MidiError:
        return("MIDI error.");

    case MUSIC_MPU401Error:
        return("MPU401 error.");

    case MUSIC_TaskManError:
        return("Task Manager error.");

    case MUSIC_FMNotDetected:
        return("FM not detected error.");

    case MUSIC_DPMI_Error:
        return("DPMI error.");

    default:
        return("Unknown error.");
    } // switch

    assert(0);    // shouldn't hit this point.
    return(NULL);
} // MUSIC_ErrorString


static int music_initialized = 0;
static int music_context = 0;
static int music_loopflag = MUSIC_PlayOnce;
static char *music_songdata = NULL;
static int music_size = 0;

int MUSIC_Init(int SoundCard, int Address)
{
    init_debugging();

    musdebug("INIT! card=>%d, address=>%d...", SoundCard, Address);

    if (music_initialized)
    {
        setErrorMessage("Music system is already initialized.");
        return(MUSIC_Error);
    } // if

    if (SoundCard != SoundScape) // We pretend there's a SoundScape installed.
    {
        setErrorMessage("Card not found.");
        musdebug("We pretend to be an Ensoniq SoundScape only.");
        return(MUSIC_Error);
    } // if

    alGenSources(1, &source);
    alGenBuffers(1, &buffer);

    auto array = ZMusic_GetConfiguration();
    ZMusicConfigurationSetting settingempty = {};
    while (memcmp(array, &settingempty, sizeof(ZMusicConfigurationSetting)))
    {
        switch(array->type)
        {
            case ZMUSIC_VAR_INT:
            case ZMUSIC_VAR_BOOL:
                ChangeMusicSettingInt((EIntConfigKey)array->identifier, nullptr, array->defaultVal, nullptr);
                break;
            case ZMUSIC_VAR_FLOAT:
                ChangeMusicSettingFloat((EFloatConfigKey)array->identifier, nullptr, array->defaultVal, nullptr);
                break;
        }
        array++;
    }

    auto sfpath = std::filesystem::weakly_canonical("./soundfont.sf2").make_preferred();
    ChangeMusicSettingString(EStringConfigKey::zmusic_fluid_patchset, nullptr, sfpath.c_str());
    ChangeMusicSettingInt(EIntConfigKey::zmusic_snd_outputrate, nullptr, 44100, nullptr);
    ChangeMusicSettingFloat(EFloatConfigKey::zmusic_fluid_gain, nullptr, 1, nullptr);
    ChangeMusicSettingFloat(EFloatConfigKey::zmusic_snd_mastervolume, nullptr, 1, nullptr);
    ChangeMusicSettingFloat(EFloatConfigKey::zmusic_snd_musicvolume, nullptr, 1, nullptr);
    ChangeMusicSettingFloat(EFloatConfigKey::zmusic_relative_volume, nullptr, 1, nullptr);

    music_initialized = 1;
    return(MUSIC_Ok);
} // MUSIC_Init


int MUSIC_Shutdown(void)
{
    musdebug("shutting down sound subsystem.");

    if (!music_initialized)
    {
        setErrorMessage("Music system is not currently initialized.");
        return(MUSIC_Error);
    } // if

    MUSIC_StopSong();
    music_context = 0;
    music_initialized = 0;
    music_loopflag = MUSIC_PlayOnce;
    alDeleteBuffers(1, &buffer);
    alDeleteSources(1, &source);

    return(MUSIC_Ok);
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int channel)
{
} // MUSIC_SetMaxFMMidiChannel

float realvol = 1.f;

void MUSIC_SetVolume(int volume)
{
    ChangeMusicSettingFloat(EFloatConfigKey::zmusic_snd_musicvolume, stream, (float)volume / 255.f, &realvol);
    ZMusic_VolumeChanged(stream);
    alSourcef(source, AL_GAIN, (float)volume / 255.f);
} // MUSIC_SetVolume


void MUSIC_SetMidiChannelVolume(int channel, int volume)
{
} // MUSIC_SetMidiChannelVolume


void MUSIC_ResetMidiChannelVolumes(void)
{
} // MUSIC_ResetMidiChannelVolumes


int MUSIC_GetVolume(void)
{
    return realvol * 255;
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int loopflag)
{
    music_loopflag = loopflag;
    
} // MUSIC_SetLoopFlag


int MUSIC_SongPlaying(void)
{
    return ZMusic_IsPlaying(stream);
} // MUSIC_SongPlaying


void MUSIC_Continue(void)
{
    if (stream && ZMusic_IsPlaying(stream))
        ZMusic_Resume(stream);
    else if (music_songdata)
        MUSIC_PlaySong(music_songdata, MUSIC_PlayOnce);

    alSourcef(source, AL_GAIN, realvol);
} // MUSIC_Continue


void MUSIC_Pause(void)
{
    ZMusic_Pause(stream);
    alSourcef(source, AL_GAIN, 0);
} // MUSIC_Pause


int MUSIC_StopSong(void)
{
    if (stream)
    {
        ZMusic_Resume(stream);
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        alSourcef(source, AL_GAIN, 0);
        ZMusic_Stop(stream);
        ZMusic_Close(stream);
        stream = nullptr;
        music_songdata = NULL;
        music_size = 0;
    }
    return(MUSIC_Ok);
} // MUSIC_StopSong

typedef struct memory_file
{
 	ptrdiff_t FilePos;

    int len;
    char* mem;
} memory_file;

static long get_tell(ZMusicCustomReader* zr)
{
    return ((memory_file*)zr->handle)->FilePos;
}

static long read_data(ZMusicCustomReader* zr, void *ptr, int32_t len)
{
    memory_file* file = ((memory_file*)zr->handle);

	if (len>file->len - file->FilePos) len = file->len - file->FilePos;
	if (len<0) len = 0;
	memcpy(ptr, file->mem + file->FilePos, len);
	file->FilePos += len;
	return len;
}

static long seek_data(ZMusicCustomReader* zr, long offset, int origin)
{
    memory_file* file = ((memory_file*)zr->handle);

	switch (origin)
	{
	case SEEK_CUR:
		offset += file->FilePos;
		break;

	case SEEK_END:
		offset += file->len;
		break;

	}
	if (offset < 0 || offset > file->len) return -1;
    if (offset < 0)
        offset = 0;
    if (offset > file->len)
        offset = file->len;
	file->FilePos = offset;

	return 0;
}

static char* gets_data(ZMusicCustomReader* zr, char* strbuf, int len)
{
    memory_file* file = ((memory_file*)zr->handle);
	if (len>file->len - file->FilePos) len = file->len - file->FilePos;
	if (len <= 0) return nullptr;

	char *p = strbuf;
	while (len > 1)
	{
		if (file->mem[file->FilePos] == 0)
		{
			file->FilePos++;
			break;
		}
		if (file->mem[file->FilePos] != '\r')
		{
			*p++ = file->mem[file->FilePos];
			len--;
			if (file->mem[file->FilePos] == '\n')
			{
				file->FilePos++;
				break;
			}
		}
		file->FilePos++;
	}
	if (p == strbuf) return nullptr;
	*p++ = 0;
	return strbuf;
}

void close_data(ZMusicCustomReader* zr)
{
    delete reinterpret_cast<memory_file*>(zr->handle);
    delete zr;
}

int MUSIC_PlaySong(char *song, int loopflag)
{
    SoundStreamInfoEx info;
    
    auto callback = [](ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes) -> int
            {
                ZMusic_MusicStream musStream = stream;

                ZMusic_Update(musStream);
                if (!ZMusic_IsPlaying(musStream))
                    return 0;
                ZMusic_FillStream(musStream, sampledata, numbytes);
                return numbytes;
            };

    MUSIC_StopSong();

    music_songdata = song;
    music_loopflag = loopflag;

    auto zcr = new ZMusicCustomReader;
    zcr->handle = new memory_file{0, music_size, (char*)song};
    zcr->gets = gets_data;
    zcr->close = close_data;
    zcr->read = read_data;
    zcr->tell = get_tell;
    zcr->seek = seek_data;

    //player = new_fluid_player(synth);
    //audiodrv = new_fluid_audio_driver(settings, synth);
    if ((stream = ZMusic_OpenSong(zcr, EMidiDevice::MDEV_FLUIDSYNTH, "")) == nullptr) {
        printf("Failed to open music!\n");
        return MUSIC_Error;
    }

    ZMusic_Start(stream, 0, music_loopflag != MUSIC_PlayOnce);
    ZMusic_GetStreamInfoEx(stream, &info);
    switch (info.mSampleType)
    {
        case SampleType_Float32:
            alBufferCallbackSOFT(buffer, info.mChannelConfig == ChannelConfig_Stereo ? AL_FORMAT_STEREO_FLOAT32 : AL_FORMAT_MONO_FLOAT32, info.mSampleRate, callback, stream);
            break;
        case SampleType_Int16:
            alBufferCallbackSOFT(buffer, info.mChannelConfig == ChannelConfig_Stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, info.mSampleRate, callback, stream);
            break;
        case SampleType_UInt8:
            alBufferCallbackSOFT(buffer, info.mChannelConfig == ChannelConfig_Stereo ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8, info.mSampleRate, callback, stream);
            break;
    }

    alSourcef(source, AL_GAIN, realvol);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    return(MUSIC_Ok);
} // MUSIC_PlaySong

#ifdef ROTT
// ROTT Special - SBF
int MUSIC_PlaySongROTT(char *song, int size, int loopflag)
{
    music_size = size;
    return MUSIC_PlaySong(song, loopflag);
} // MUSIC_PlaySongROTT
#endif


void MUSIC_SetContext(int context)
{
    musdebug("STUB ... MUSIC_SetContext().\n");
    music_context = context;
} // MUSIC_SetContext


int MUSIC_GetContext(void)
{
    return(music_context);
} // MUSIC_GetContext


void MUSIC_SetSongTick(unsigned long PositionInTicks)
{
    musdebug("STUB ... MUSIC_SetSongTick().\n");
} // MUSIC_SetSongTick


void MUSIC_SetSongTime(unsigned long milliseconds)
{
    musdebug("STUB ... MUSIC_SetSongTime().\n");
}// MUSIC_SetSongTime


void MUSIC_SetSongPosition(int measure, int beat, int tick)
{
    musdebug("STUB ... MUSIC_SetSongPosition().\n");
} // MUSIC_SetSongPosition


void MUSIC_GetSongPosition(songposition *pos)
{
    musdebug("STUB ... MUSIC_GetSongPosition().\n");
} // MUSIC_GetSongPosition


void MUSIC_GetSongLength(songposition *pos)
{
    musdebug("STUB ... MUSIC_GetSongLength().\n");
} // MUSIC_GetSongLength


int MUSIC_FadeVolume(int tovolume, int milliseconds)
{
    //Mix_FadeOutMusic(milliseconds);
    MUSIC_StopSong();
    return(MUSIC_Ok);
} // MUSIC_FadeVolume


int MUSIC_FadeActive(void)
{
    //return((Mix_FadingMusic() == MIX_FADING_OUT) ? __FX_TRUE : __FX_FALSE);
    return __FX_FALSE;
} // MUSIC_FadeActive


void MUSIC_StopFade(void)
{
    musdebug("STUB ... MUSIC_StopFade().\n");
} // MUSIC_StopFade


void MUSIC_RerouteMidiChannel(int channel, int cdecl (*function)( int event, int c1, int c2 ))
{
    musdebug("STUB ... MUSIC_RerouteMidiChannel().\n");
} // MUSIC_RerouteMidiChannel


void MUSIC_RegisterTimbreBank(unsigned char *timbres)
{
    musdebug("STUB ... MUSIC_RegisterTimbreBank().\n");
} // MUSIC_RegisterTimbreBank

void MUSIC_UpdateReverbChorus( void )
{
    ChangeMusicSettingInt(EIntConfigKey::zmusic_fluid_reverb, stream, reverbMusic, nullptr);
    ChangeMusicSettingInt(EIntConfigKey::zmusic_fluid_chorus, stream, chorusMusic, nullptr);
}

// end of fx_man.c ...
