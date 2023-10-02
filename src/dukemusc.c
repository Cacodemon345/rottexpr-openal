/*
 * A reimplementation of Jim Dose's FX_MAN routines, using  SDL_mixer 1.2.
 *   Whee. FX_MAN is also known as the "Apogee Sound System", or "ASS" for
 *   short. How strangely appropriate that seems.
 *
 * Written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <fluidsynth.h>

#define AL_ALEXT_PROTOTYPES 1
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#define ROTT

#define cdecl

#include "SDL2/SDL.h"
#include "SDL2/SDL_mixer.h"
#ifdef ROTT
#include "rt_def.h"      // ROTT music hack
#include "rt_cfg.h"      // ROTT music hack
#include "rt_util.h"     // ROTT music hack
#endif
#include "music.h"

#define __FX_TRUE  (1 == 1)
#define __FX_FALSE (!__FX_TRUE)

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

// FluidSynth stuff.
static fluid_settings_t* settings;
static fluid_synth_t* synth;
static fluid_player_t* player;
static fluid_audio_driver_t* audiodrv;
static ALuint buffer, source;

ALsizei AL_APIENTRY audio_buffer_callback(ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes)
{
    fluid_synth_write_s16(synth, numbytes / 2, sampledata, 0, 2, sampledata, 1, 2);
    return numbytes;
}

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

char *MUSIC_ErrorString(int ErrorNumber)
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
static Mix_Music *music_musicchunk = NULL;

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

    settings = new_fluid_settings();
    if (!settings) return MUSIC_Error;
    synth = new_fluid_synth(settings);

    if (!synth) {
        fprintf(stderr, "Failed to load synth\n");
        delete_fluid_settings(settings);
        return MUSIC_Error;
    }

    int res = fluid_synth_sfload(synth, "./soundfont.sf2", 1);
    if (res == FLUID_FAILED)
    {
        fprintf(stderr, "Failed to load soundfont\n");
    }

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
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
    synth = NULL;
    settings = NULL;
    alDeleteBuffers(1, &buffer);
    alDeleteSources(1, &source);

    return(MUSIC_Ok);
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int channel)
{
    musdebug("STUB ... MUSIC_SetMaxFMMidiChannel(%d).\n", channel);
} // MUSIC_SetMaxFMMidiChannel


void MUSIC_SetVolume(int volume)
{
    fluid_synth_set_gain(synth, (float)volume / 255.f);
    //Mix_VolumeMusic(volume >> 1);  // convert 0-255 to 0-128.
} // MUSIC_SetVolume


void MUSIC_SetMidiChannelVolume(int channel, int volume)
{
    musdebug("STUB ... MUSIC_SetMidiChannelVolume(%d, %d).\n", channel, volume);
} // MUSIC_SetMidiChannelVolume


void MUSIC_ResetMidiChannelVolumes(void)
{
    musdebug("STUB ... MUSIC_ResetMidiChannelVolumes().\n");
} // MUSIC_ResetMidiChannelVolumes


int MUSIC_GetVolume(void)
{
    return fluid_synth_get_gain(synth) * 255.f;
    //return(Mix_VolumeMusic(-1) << 1);  // convert 0-128 to 0-255.
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int loopflag)
{
    music_loopflag = loopflag;
} // MUSIC_SetLoopFlag


int MUSIC_SongPlaying(void)
{
    return((fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) ? __FX_TRUE : __FX_FALSE);
} // MUSIC_SongPlaying


void MUSIC_Continue(void)
{
#if 1
    if (player && (fluid_player_get_status(player) == FLUID_PLAYER_DONE || fluid_player_get_status(player) == FLUID_PLAYER_READY))
        fluid_player_play(player);
    else if (music_songdata)
        MUSIC_PlaySong(music_songdata, MUSIC_PlayOnce);
#endif
} // MUSIC_Continue


void MUSIC_Pause(void)
{
    fluid_player_stop(player);
} // MUSIC_Pause


int MUSIC_StopSong(void)
{
#if 0
    //if (!fx_initialized)
    if (!Mix_QuerySpec(NULL, NULL, NULL))
    {
        setErrorMessage("Need FX system initialized, too. Sorry.");
        return(MUSIC_Error);
    } // if

    if ( (Mix_PlayingMusic()) || (Mix_PausedMusic()) )
        Mix_HaltMusic();

    if (music_musicchunk)
        Mix_FreeMusic(music_musicchunk);

    music_songdata = NULL;
    music_musicchunk = NULL;
#endif
    if (player)
    {
        fluid_player_stop(player);
        fluid_player_seek(player, 0);

        // Detach and stop everything before deleting it.
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        delete_fluid_audio_driver(audiodrv);
        delete_fluid_player(player);
        fluid_synth_system_reset(synth);
        player = NULL;
    }
    return(MUSIC_Ok);
} // MUSIC_StopSong


int MUSIC_PlaySong(char *song, int loopflag)
{
    double samplerate = 44100;
    //SDL_RWops *rw;

    MUSIC_StopSong();

    music_songdata = song;
    music_loopflag = loopflag;

    player = new_fluid_player(synth);
    audiodrv = new_fluid_audio_driver(settings, synth);

    fluid_player_set_loop(player, (loopflag == MUSIC_PlayOnce) ? 0 : -1);
    fluid_player_add_mem(player, song, music_size);
    fluid_settings_getnum(fluid_synth_get_settings(synth), "synth.sample-rate", &samplerate);
    //alSourcei(source, AL_BUFFER, buffer);
    //alBufferCallbackSOFT(buffer, AL_FORMAT_STEREO16, samplerate, audio_buffer_callback, NULL);
    //alSourcePlay(source);
    fluid_player_play(player);

    return(MUSIC_Ok);
} // MUSIC_PlaySong


extern char ApogeePath[256];

#ifdef ROTT
// ROTT Special - SBF
int MUSIC_PlaySongROTT(char *song, int size, int loopflag)
{
    double samplerate = 44100;
    MUSIC_StopSong();

    player = new_fluid_player(synth);
    audiodrv = new_fluid_audio_driver(settings, synth);

    music_songdata = song;
    music_size = size;
    music_loopflag = loopflag;

    fluid_player_set_loop(player, (loopflag == MUSIC_PlayOnce) ? 0 : -1);
    fluid_player_add_mem(player, song, music_size);
    fluid_settings_getnum(fluid_synth_get_settings(synth), "synth.sample-rate", &samplerate);
    //alSourcei(source, AL_BUFFER, buffer);
    //alBufferCallbackSOFT(buffer, AL_FORMAT_STEREO16, samplerate, audio_buffer_callback, NULL);
    fluid_player_play(player);
    //alSourcePlay(source);

    return(MUSIC_Ok);
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
    fluid_player_stop(player);
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


// end of fx_man.c ...
