#include "fx_man.h"

#define AL_ALEXT_PROTOTYPES 1
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <sndfile.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int revstereo = 0;
static ALCdevice* Device;
static ALCcontext* Context;

static void ( *fx_callback )( unsigned long ) = NULL;
bool OpenALInited = false;

static ALuint* Buffers;
static ALuint* source;
static unsigned long* callbackvals;
int voicenum = 0;
int vol = 0;

int FX_ErrorCode = FX_Ok;

#define FX_SetErrorCode( status ) \
   FX_ErrorCode = ( status );

/*---------------------------------------------------------------------
   Function: FX_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void FX_SetReverseStereo
   (
   int setting
   )

   {
   revstereo = setting;
   }


/*---------------------------------------------------------------------
   Function: FX_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int FX_GetReverseStereo
   (
   void
   )

   {
   return revstereo;
   }

void AL_APIENTRY OALCallback(ALenum eventType, ALuint object, ALuint param, ALsizei length, const ALchar *message, ALvoid *userParam)
{
    if (eventType == AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT
    && (param == AL_STOPPED || AL_INITIAL))
    {
        int i = 0;

        for (i = 0; i < voicenum; i++)
        {
            if (source[i] == object && callbackvals[i] != -1)
            {
                fx_callback(callbackvals[i]);
                callbackvals[i] = -1;
            }
        }
    }
}

int
FX_Init( int SoundCard, int numvoices, int numchannels, int samplebits, unsigned mixrate )
{
    int i = 0;
    ALenum event = AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT;

	Device = alcOpenDevice(NULL); // select the "preferred device" 
	if (Device)
	{
		Context = alcCreateContext(Device, NULL);
		alcMakeContextCurrent(Context);
    }
	ALenum error = alGetError(); // clear error code 
    Buffers = (ALuint*)malloc(sizeof(ALuint) * numvoices);
	alGenBuffers(numvoices, Buffers);
	
	if ((error = alGetError()) != AL_NO_ERROR)
    {
        FX_SetErrorCode(FX_Error);
        return FX_Error;
    }

    source = (ALuint*)calloc(sizeof(ALuint), numvoices);
    alGenSources(numvoices, source);
    callbackvals = (unsigned long*)calloc(sizeof(unsigned long), numvoices);
    for (i = 0; i < numvoices; i++)
    {
        callbackvals[i] = -1;
    }
    OpenALInited = true;
    voicenum = numvoices;
    
    alEventCallbackSOFT(OALCallback, NULL);
    alEventControlSOFT(1, &event, AL_TRUE);
    for (i = 0; i < numvoices; i++)
    {
        alSourcef(source[i], AL_MAX_DISTANCE, 255);
    }
    alDistanceModel(AL_LINEAR_DISTANCE);

	return FX_Ok;
}


int FX_Shutdown( void )
{
    int i = 0;
    alEventCallbackSOFT(NULL, NULL);
	for (i = 0; i < voicenum; i++)
	{
		alSourcei(source[i], AL_LOOPING, AL_FALSE);
		alSourceStop(source[i]);
	}
    alDeleteSources(voicenum, source);
    alDeleteBuffers(voicenum, Buffers);
    alcMakeContextCurrent(NULL);
	alcDestroyContext(Context);
	alcCloseDevice(Device);
    
    return FX_Ok;
}

void
FX_SetVolume(int volume)
{
    vol = volume;
    alListenerf(AL_GAIN, (float)(vol) / 255.f);
}

int FX_GetVolume(void)
{
    float val = 1;
    alGetListenerf(AL_GAIN, &val);
    return (int)(vol * 255);
}
int FX_VoiceAvailable(int priority)
{
    int i = 0;
    for (i = 0; i < voicenum; i++)
    {
        int val;
        alGetSourcei(source[i], AL_SOURCE_STATE, &val);
        if (val == AL_STOPPED || val == AL_INITIAL)
            return 1;
    }
    return 0;
}

int FX_SoundActive( int handle )
{
    int val = AL_STOPPED;
    alGetSourcei(handle, AL_SOURCE_STATE, &val);
    return (val == AL_PLAYING);
}

int FX_SoundsPlaying( void )
{
    int i = 0;
    for (i = 0; i < voicenum; i++)
    {
        int val;
        alGetSourcei(source[i], AL_SOURCE_STATE, &val);
        if (val == AL_PLAYING)
            return 1;
    }
    return 0;
}

int FX_StopSound( int handle )
{
    int i = 0;
    alSourceStop(handle);

    for (i = 0; i < voicenum; i++)
    {
        if (source[i] == handle && callbackvals[i] != -1)
        {
            fx_callback(callbackvals[i]);
            callbackvals[i] = -1;
        }
    }
}

int FX_StopAllSounds( void )
{
    int i = 0;
    for (i = 0; i < voicenum; i++)
    {
        alSourceStop(source[i]);
    }
}

int FX_StartDemandFeedPlayback( void ( *function )( char **ptr, unsigned long *length ),
                                int rate, int pitchoffset, int vol, int left, int right,
                                int priority, unsigned long callbackval )
                                {
                                    return 0;
                                }

/*---------------------------------------------------------------------
   Function: FX_StartRecording

   Starts the sound recording engine.
---------------------------------------------------------------------*/

int FX_StartRecording
   (
   int MixRate,
   void ( *function )( char *ptr, int length )
   )

   {
   int status;

   FX_SetErrorCode( FX_InvalidCard );
   status = FX_Warning;

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_StopRecord

   Stops the sound record engine.
---------------------------------------------------------------------*/

void FX_StopRecord(void)
{
}

int FX_SetupCard(int SoundCard, fx_device* device)
{
             device->MaxVoices     = 32;
         device->MaxSampleBits = 0;
         device->MaxChannels   = 0;
    return FX_Ok;
}

void  FX_SetReverb( int reverb ) {}
void  FX_SetFastReverb( int reverb ) {}
int   FX_GetMaxReverbDelay( void ) {}
int   FX_GetReverbDelay( void ) {}
void  FX_SetReverbDelay( int delay ) {}

char *FX_ErrorString
   (
   int ErrorNumber
   )

   {
   char *ErrorString;

   switch( ErrorNumber )
      {
      case FX_Warning :
      case FX_Error :
         ErrorString = FX_ErrorString( FX_ErrorCode );
         break;

      case FX_Ok :
         ErrorString = "Fx ok.";
         break;

      case FX_ASSVersion :
         ErrorString = "Apogee Sound System Version " ASS_VERSION_STRING "  "
            "Programmed by Jim Dose\n"
            "(c) Copyright 1995 James R. Dose.  All Rights Reserved.\n";
         break;

      case FX_InvalidCard :
         ErrorString = "Invalid Sound Fx device.";
         break;

      case FX_DPMI_Error :
         ErrorString = "DPMI Error in FX_MAN.";
         break;

      default :
         ErrorString = "Unknown Fx error code.";
         break;
      }

   return( ErrorString );
   }

#define MAXDETUNE 25

static unsigned long PitchTable[ 12 ][ MAXDETUNE ] =
{
   { 0x10000, 0x10097, 0x1012f, 0x101c7, 0x10260, 0x102f9, 0x10392, 0x1042c,
   0x104c6, 0x10561, 0x105fb, 0x10696, 0x10732, 0x107ce, 0x1086a, 0x10907,
   0x109a4, 0x10a41, 0x10adf, 0x10b7d, 0x10c1b, 0x10cba, 0x10d59, 0x10df8,
   0x10e98 },
   { 0x10f38, 0x10fd9, 0x1107a, 0x1111b, 0x111bd, 0x1125f, 0x11302, 0x113a5,
   0x11448, 0x114eb, 0x1158f, 0x11634, 0x116d8, 0x1177e, 0x11823, 0x118c9,
   0x1196f, 0x11a16, 0x11abd, 0x11b64, 0x11c0c, 0x11cb4, 0x11d5d, 0x11e06,
   0x11eaf },
   { 0x11f59, 0x12003, 0x120ae, 0x12159, 0x12204, 0x122b0, 0x1235c, 0x12409,
   0x124b6, 0x12563, 0x12611, 0x126bf, 0x1276d, 0x1281c, 0x128cc, 0x1297b,
   0x12a2b, 0x12adc, 0x12b8d, 0x12c3e, 0x12cf0, 0x12da2, 0x12e55, 0x12f08,
   0x12fbc },
   { 0x1306f, 0x13124, 0x131d8, 0x1328d, 0x13343, 0x133f9, 0x134af, 0x13566,
   0x1361d, 0x136d5, 0x1378d, 0x13846, 0x138fe, 0x139b8, 0x13a72, 0x13b2c,
   0x13be6, 0x13ca1, 0x13d5d, 0x13e19, 0x13ed5, 0x13f92, 0x1404f, 0x1410d,
   0x141cb },
   { 0x1428a, 0x14349, 0x14408, 0x144c8, 0x14588, 0x14649, 0x1470a, 0x147cc,
   0x1488e, 0x14951, 0x14a14, 0x14ad7, 0x14b9b, 0x14c5f, 0x14d24, 0x14dea,
   0x14eaf, 0x14f75, 0x1503c, 0x15103, 0x151cb, 0x15293, 0x1535b, 0x15424,
   0x154ee },
   { 0x155b8, 0x15682, 0x1574d, 0x15818, 0x158e4, 0x159b0, 0x15a7d, 0x15b4a,
   0x15c18, 0x15ce6, 0x15db4, 0x15e83, 0x15f53, 0x16023, 0x160f4, 0x161c5,
   0x16296, 0x16368, 0x1643a, 0x1650d, 0x165e1, 0x166b5, 0x16789, 0x1685e,
   0x16934 },
   { 0x16a09, 0x16ae0, 0x16bb7, 0x16c8e, 0x16d66, 0x16e3e, 0x16f17, 0x16ff1,
   0x170ca, 0x171a5, 0x17280, 0x1735b, 0x17437, 0x17513, 0x175f0, 0x176ce,
   0x177ac, 0x1788a, 0x17969, 0x17a49, 0x17b29, 0x17c09, 0x17cea, 0x17dcc,
   0x17eae },
   { 0x17f91, 0x18074, 0x18157, 0x1823c, 0x18320, 0x18406, 0x184eb, 0x185d2,
   0x186b8, 0x187a0, 0x18888, 0x18970, 0x18a59, 0x18b43, 0x18c2d, 0x18d17,
   0x18e02, 0x18eee, 0x18fda, 0x190c7, 0x191b5, 0x192a2, 0x19391, 0x19480,
   0x1956f },
   { 0x1965f, 0x19750, 0x19841, 0x19933, 0x19a25, 0x19b18, 0x19c0c, 0x19d00,
   0x19df4, 0x19ee9, 0x19fdf, 0x1a0d5, 0x1a1cc, 0x1a2c4, 0x1a3bc, 0x1a4b4,
   0x1a5ad, 0x1a6a7, 0x1a7a1, 0x1a89c, 0x1a998, 0x1aa94, 0x1ab90, 0x1ac8d,
   0x1ad8b },
   { 0x1ae89, 0x1af88, 0x1b088, 0x1b188, 0x1b289, 0x1b38a, 0x1b48c, 0x1b58f,
   0x1b692, 0x1b795, 0x1b89a, 0x1b99f, 0x1baa4, 0x1bbaa, 0x1bcb1, 0x1bdb8,
   0x1bec0, 0x1bfc9, 0x1c0d2, 0x1c1dc, 0x1c2e6, 0x1c3f1, 0x1c4fd, 0x1c609,
   0x1c716 },
   { 0x1c823, 0x1c931, 0x1ca40, 0x1cb50, 0x1cc60, 0x1cd70, 0x1ce81, 0x1cf93,
   0x1d0a6, 0x1d1b9, 0x1d2cd, 0x1d3e1, 0x1d4f6, 0x1d60c, 0x1d722, 0x1d839,
   0x1d951, 0x1da69, 0x1db82, 0x1dc9c, 0x1ddb6, 0x1ded1, 0x1dfec, 0x1e109,
   0x1e225 },
   { 0x1e343, 0x1e461, 0x1e580, 0x1e6a0, 0x1e7c0, 0x1e8e0, 0x1ea02, 0x1eb24,
   0x1ec47, 0x1ed6b, 0x1ee8f, 0x1efb4, 0x1f0d9, 0x1f1ff, 0x1f326, 0x1f44e,
   0x1f576, 0x1f69f, 0x1f7c9, 0x1f8f3, 0x1fa1e, 0x1fb4a, 0x1fc76, 0x1fda3,
   0x1fed1 }
};

/*---------------------------------------------------------------------
   Function: PITCH_GetScale

   Returns a fixed-point value to scale number the specified amount.
---------------------------------------------------------------------*/

unsigned long PITCH_GetScale
   (
   int pitchoffset
   )

   {
   unsigned long scale;
   int octaveshift;
   int noteshift;
   int note;
   int detune;

//   if ( !PITCH_Installed )
//      {
//      PITCH_Init();
//      }

   if ( pitchoffset == 0 )
      {
      return( PitchTable[ 0 ][ 0 ] );
      }

   noteshift = pitchoffset % 1200;
   if ( noteshift < 0 )
      {
      noteshift += 1200;
      }

   note   = noteshift / 100;
   detune = ( noteshift % 100 ) / ( 100 / MAXDETUNE );
   octaveshift = ( pitchoffset - noteshift ) / 1200;

   if ( detune < 0 )
      {
      detune += ( 100 / MAXDETUNE );
      note--;
      if ( note < 0 )
         {
         note += 12;
         octaveshift--;
         }
      }

   scale = PitchTable[ note ][ detune ];

   if ( octaveshift < 0 )
      {
      scale >>= -octaveshift;
      }
   else
      {
      scale <<= octaveshift;
      }

   return( scale );
   }

double FixedToFloat(unsigned long f)
{
	return f * (1. / (1 << 16));
}


int FX_SetCallBack(void ( *callback )( unsigned long ))
{
    fx_callback = callback;
    return FX_Ok;
}

static int FindVoice(void)
{
    int i = 0;
    for (i = 0; i < voicenum; i++)
    {
        int val;
        alGetSourcei(source[i], AL_SOURCE_STATE, &val);
        if (val == AL_STOPPED || val == AL_INITIAL)
            return i;
    }
    return -1;
}

typedef struct memory_file
{
 	ptrdiff_t FilePos;

    sf_count_t len;
    char* mem;
} memory_file;

sf_count_t get_len(void* user_data)
{
    return ((memory_file*)user_data)->len;
}

sf_count_t get_tell(void* user_data)
{
    return ((memory_file*)user_data)->FilePos;
}

sf_count_t write_data(const void *ptr, sf_count_t count, void* user_data)
{
    memory_file* file = ((memory_file*)user_data);

    return -1;
}

sf_count_t read_data(void *ptr, sf_count_t len, void* user_data)
{
    memory_file* file = ((memory_file*)user_data);

	if (len>file->len - file->FilePos) len = file->len - file->FilePos;
	if (len<0) len = 0;
	memcpy(ptr, file->mem + file->FilePos, len);
	file->FilePos += len;
	return len;
}

sf_count_t seek_data(sf_count_t offset, int origin, void* user_data)
{
    memory_file* file = ((memory_file*)user_data);

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

int FX_PlayVOC3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval, int len )
{
    int sourceNum;
    SF_VIRTUAL_IO io;
    SF_INFO info;
    memory_file file;
    SNDFILE* sndfile = NULL;
    short* framedata = NULL;
    if ( distance < 0 )
    {
        distance  = -distance;
        angle    += 16;
    }

    file.len = len;
    file.FilePos = 0;
    file.mem = ptr;

    io.get_filelen = get_len;
    io.tell = get_tell;
    io.read = read_data;
    io.write = write_data;
    io.seek = seek_data;

    memset((void*)&info, 0, sizeof(SF_INFO));
    info.format = 0;

    sourceNum = FindVoice();
    if (sourceNum == -1) {
        fprintf(stderr, "Failed to find a free voice.\n");
        return -1;
    }
    
    sndfile = sf_open_virtual(&io, SFM_READ, &info, (void*)&file);
    if (!sndfile) {
        fprintf(stderr, "Failed to read sndfile.\n");
        return -1;
    }
    
    framedata = calloc(sizeof(short), info.frames * info.channels);
    sf_readf_short(sndfile, framedata, info.frames);

    alSourcei(source[sourceNum], AL_BUFFER, 0);
    alBufferData(Buffers[sourceNum], AL_FORMAT_MONO16 + (info.channels == 2 ? 2 : 0), framedata, info.frames * info.channels * sizeof(short), info.samplerate);
    free(framedata);
    sf_close(sndfile);

    angle &= 31;

    alSource3f(source[sourceNum], AL_POSITION, cos(0.5235987755982988 * angle) * distance, sin(0.5235987755982988 * angle) * distance, 0);
    alSource3f(source[sourceNum], AL_VELOCITY, 0, 0, 0);
    alSource3f(source[sourceNum], AL_DIRECTION, 0, 0, 0);
    alSourcef(source[sourceNum], AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source[sourceNum], AL_BUFFER, Buffers[sourceNum]);
    alSourcedSOFT(source[sourceNum], AL_PITCH, FixedToFloat(PITCH_GetScale(pitchoffset)));
    
    callbackvals[sourceNum] = callbackval;
    alSourcePlay(source[sourceNum]);
    
    return source[sourceNum];
}

int FX_PlayWAV3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval, int len )
{
    return FX_PlayVOC3D(ptr, pitchoffset, angle, distance, priority, callbackval, len);
}

int FX_SetPitch(int handle, int pitchoffset)
{
    alSourcedSOFT(handle, AL_PITCH, FixedToFloat(PITCH_GetScale(pitchoffset)));
    return FX_Ok;
}