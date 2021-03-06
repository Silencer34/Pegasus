/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	FMODSound.cpp
//! \author	Karolyn Boulanger
//! \date	18th August 2014
//! \brief	Sound system entry point, FMOD implementation

#if PEGASUS_SAPI_FMOD

#include "Pegasus/Sound/Sound.h"
#include "Pegasus/Libs/FMOD/fmod.hpp"

#if PEGASUS_COMPILER_MSVC
#if PEGASUS_DEV
#pragma comment(lib, "FMOD/Win32/fmodL_vc")
#elif PEGASUS_REL
#pragma comment(lib, "FMOD/Win32/fmod_vc")
#endif
#endif

namespace Pegasus {
namespace Sound {


//! Size of a mixing buffer for FMOD. The lower, the more CPU it takes.
//! 1024 (21.5ms) is a recommended value. For longer latencies, it is better to increase the number of buffers
static const int DSP_BUFFER_SIZE = 1024;

//! Number of mixing buffers. The higher, the more stable the sound is, but the more latency.
//! The total latency of the output is DSP_BUFFER_SIZE * DSP_NUM_BUFFERS
static const int DSP_NUM_BUFFERS = 1;

//! Estimated latency in milliseconds between the mixing of the output buffer and the actual sound output.
//! This is completely hardware dependent, and until we have a reliable way to know it,
//! we have to keep an average value
static const int OUTPUT_ESTIMATED_LATENCY = 100;
    
//----------------------------------------------------------------------------------------

//! FMOD system global object
static FMOD::System * gSystem = nullptr;

//! FMOD music object, created by \a LoadMusic()
static FMOD::Sound * gMusic = nullptr;

//! FMOD channel object, created by \a LoadMusic() and used in PlayMusic() to start reading
static FMOD::Channel * gChannel = nullptr;

//----------------------------------------------------------------------------------------

void Initialize()
{
    PG_LOG('SOUN', "Initializing sound system using FMOD");
    PG_ASSERTSTR(gSystem == nullptr, "Trying to initialize FMOD twice");
    
    // Create the FMOD system global object
    FMOD_RESULT result = FMOD::System_Create(&gSystem);
    if (result != FMOD_OK)
    {
        PG_LOG('SOUN', "Unable to create the FMOD sound system");
        gSystem = nullptr;
        return;
    }

    // Check the version of the DLL
    unsigned int version = 0;
    result = gSystem->getVersion(&version);
    if ((result != FMOD_OK) || (version < FMOD_VERSION))
    {
        PG_LOG('SOUN', "Unable to initialize the sound system because the FMOD DLL is the wrong version");
        gSystem->release();
        gSystem = nullptr;
        return;
    }

    //! \todo Give info about the sound card, its name and the number of speakers

    // Set the DSP buffer size to control the latency of the output
    // (the default values result in the music not syncing up well with the timeline)
    result = gSystem->setDSPBufferSize(DSP_BUFFER_SIZE, DSP_NUM_BUFFERS);
    if (result != FMOD_OK)
    {
        PG_LOG('SOUN', "Unable to set the DSP buffer size of the FMOD sound system. Latency will probably be high.");
    }

    // Initialize the FMOD system
    const int MAX_NUM_CHANNELS = 32;
    result = gSystem->init(MAX_NUM_CHANNELS, FMOD_INIT_NORMAL, 0);    // Initialize FMOD.
    if (result != FMOD_OK)
    {
        PG_LOG('SOUN', "Unable to initialize the FMOD sound system");
        gSystem->release();
        gSystem = nullptr;
    }
}

//----------------------------------------------------------------------------------------

void LoadMusic(const char * fileName)
{
    if (gSystem != nullptr)
    {
        PG_LOG('MUSC', "Loading music file %s", fileName);

        // Load the sound file, with or without streaming
#if PEGASUS_SOUND_STREAM_MUSIC
        FMOD_RESULT result = gSystem->createSound(fileName, FMOD_LOOP_OFF | FMOD_2D | FMOD_CREATESTREAM, 0, &gMusic);
#else
        FMOD_RESULT result = gSystem->createSound(fileName, FMOD_LOOP_OFF | FMOD_2D, 0, &gMusic);
#endif
        if (result != FMOD_OK)
        {
            PG_LOG('MUSC', "Unable to load the music file (%s)", fileName);
            return;
        }

        // Preload the music by filling the initial buffer and set the channel to pause
        result = gSystem->playSound(gMusic, 0, true, &gChannel);
        if (result != FMOD_OK)
        {
            PG_LOG('MUSC', "Unable to play the loaded music file (%s)", fileName);
            gChannel = nullptr;
            return;
        }
    }
}

//----------------------------------------------------------------------------------------

void PlayMusic()
{
    if (gChannel != nullptr)
    {
        PG_LOG('MUSC', "Starting playing the music file");

        FMOD_RESULT result = gChannel->setPaused(false);
        if (result != FMOD_OK)
        {
            PG_LOG('MUSC', "Unable to play the music file, the channel cannot be unpaused");
            return;
        }
    }
}

//----------------------------------------------------------------------------------------

bool IsPlayingMusic()
{
    if (gChannel != nullptr)
    {
        bool paused = false;
        FMOD_RESULT result = gChannel->getPaused(&paused);
        return (result == FMOD_OK) && !paused;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------------------

unsigned int GetMusicPosition()
{
    if (gChannel != nullptr)
    {
        unsigned int musicPosition = 0;
        FMOD_RESULT result = gChannel->getPosition(&musicPosition, FMOD_TIMEUNIT_MS);
        if (result == FMOD_OK)
        {
            if (musicPosition > OUTPUT_ESTIMATED_LATENCY)
            {
                return musicPosition - OUTPUT_ESTIMATED_LATENCY;
            }
        }
    }

    return 0;
}

//----------------------------------------------------------------------------------------

void Update()
{
    if (gSystem != nullptr)
    {
        gSystem->update();
    }
}

//----------------------------------------------------------------------------------------

void Release()
{
    if (gSystem != nullptr)
    {
        if (gChannel != nullptr)
        {
            gChannel->stop();
            gChannel = nullptr;
        }

        if (gMusic != nullptr)
        {
            gMusic->release();
            gMusic = nullptr;
        }

        gSystem->release();
        gSystem = nullptr;
    }
}

}   // namespace Sound
}   // namespace Pegasus

#else
PEGASUS_AVOID_EMPTY_FILE_WARNING
#endif  // PEGASUS_SAPI_FMOD

