#include "AudioManager.h"
#include "../Utils/Math.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include <vorbis/vorbisfile.h>
#include <mpg123.h>

AudioManager::AudioManager()
    : device(nullptr)
    , context(nullptr)
    , musicSource(0)
    , masterVolume(1.0f)
    , musicVolume(0.7f)
    , sfxVolume(1.0f)
{
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open audio device" << std::endl;
        return false;
    }
    
    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        return false;
    }
    
    alcMakeContextCurrent(context);
    
    // Create music source
    alGenSources(1, &musicSource);
    
    // Initialize mpg123
    if (mpg123_init() != MPG123_OK) {
        std::cerr << "Warning: Failed to initialize mpg123" << std::endl;
    }
    
    return true;
}

void AudioManager::Shutdown() {
    // Stop all sounds
    StopMusic();
    
    // Delete sources
    if (musicSource) {
        alDeleteSources(1, &musicSource);
    }
    
    for (ALuint source : activeSources) {
        alDeleteSources(1, &source);
    }
    activeSources.clear();
    
    // Delete buffers
    for (auto& pair : soundBuffers) {
        alDeleteBuffers(1, &pair.second.bufferId);
    }
    soundBuffers.clear();
    
    // Cleanup context
    if (context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
    }
    
    if (device) {
        alcCloseDevice(device);
    }
    
    // Cleanup mpg123
    mpg123_exit();
}

void AudioManager::PlayMusic(const std::string& path, bool loop) {
    ALuint buffer = LoadSound(path);
    if (buffer == 0) return;
    
    alSourcei(musicSource, AL_BUFFER, buffer);
    alSourcei(musicSource, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcef(musicSource, AL_GAIN, musicVolume * masterVolume);
    alSourcePlay(musicSource);
}

void AudioManager::StopMusic() {
    alSourceStop(musicSource);
}

void AudioManager::SetMusicVolume(float volume) {
    musicVolume = volume;
    alSourcef(musicSource, AL_GAIN, musicVolume * masterVolume);
}

void AudioManager::PlaySound(const std::string& path, float volume) {
    ALuint buffer = LoadSound(path);
    if (buffer == 0) return;
    
    ALuint source = CreateSource();
    if (source == 0) return;
    
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, volume * sfxVolume * masterVolume);
    alSourcePlay(source);
    
    activeSources.push_back(source);
}

void AudioManager::PlaySoundAt(const std::string& path, const Vector2& position, 
                              float volume) {
    ALuint buffer = LoadSound(path);
    if (buffer == 0) return;
    
    ALuint source = CreateSource();
    if (source == 0) return;
    
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, volume * sfxVolume * masterVolume);
    alSource3f(source, AL_POSITION, position.x, 0.0f, position.y);
    alSourcePlay(source);
    
    activeSources.push_back(source);
}

void AudioManager::SetMasterVolume(float volume) {
    masterVolume = volume;
}

void AudioManager::SetSFXVolume(float volume) {
    sfxVolume = volume;
}

void AudioManager::Update(const Vector2& listenerPosition) {
    alListener3f(AL_POSITION, listenerPosition.x, 0.0f, listenerPosition.y);
    
    // Remove finished sources
    activeSources.erase(
        std::remove_if(activeSources.begin(), activeSources.end(),
            [](ALuint source) {
                ALint state;
                alGetSourcei(source, AL_SOURCE_STATE, &state);
                if (state != AL_PLAYING) {
                    alDeleteSources(1, &source);
                    return true;
                }
                return false;
            }),
        activeSources.end()
    );
}

bool AudioManager::HasExtension(const std::string& path, const std::string& ext) {
    if (path.size() < ext.size()) return false;
    return path.compare(path.size() - ext.size(), ext.size(), ext) == 0;
}

ALuint AudioManager::LoadSound(const std::string& path) {
    auto it = soundBuffers.find(path);
    if (it != soundBuffers.end()) {
        return it->second.bufferId;
    }
    
    ALuint buffer = 0;
    
    if (HasExtension(path, ".ogg")) {
        buffer = LoadOGG(path);
    } else if (HasExtension(path, ".mp3")) {
        buffer = LoadMP3(path);
    } else {
        std::cerr << "Unsupported audio format: " << path << std::endl;
        return 0;
    }
    
    if (buffer != 0) {
        soundBuffers[path] = {buffer, path};
    }
    
    return buffer;
}

ALuint AudioManager::LoadOGG(const std::string& path) {
    OggVorbis_File vf;
    
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open OGG file: " << path << std::endl;
        return 0;
    }
    
    if (ov_open(fp, &vf, nullptr, 0) < 0) {
        std::cerr << "Failed to open OGG stream: " << path << std::endl;
        fclose(fp);
        return 0;
    }
    
    // Get file info
    vorbis_info* info = ov_info(&vf, -1);
    if (!info) {
        std::cerr << "Failed to get OGG info: " << path << std::endl;
        ov_clear(&vf);
        return 0;
    }
    
    ALenum format;
    if (info->channels == 1) {
        format = AL_FORMAT_MONO16;
    } else {
        format = AL_FORMAT_STEREO16;
    }
    
    ALsizei sampleRate = info->rate;
    
    // Read all PCM data
    std::vector<char> pcmData;
    char buffer[4096];
    int bitStream = 0;
    long bytesRead;
    
    while ((bytesRead = ov_read(&vf, buffer, sizeof(buffer), 0, 2, 1, &bitStream)) > 0) {
        pcmData.insert(pcmData.end(), buffer, buffer + bytesRead);
    }
    
    ov_clear(&vf);
    
    if (pcmData.empty()) {
        std::cerr << "No PCM data decoded from OGG: " << path << std::endl;
        return 0;
    }
    
    // Upload to OpenAL
    ALuint bufferId;
    alGenBuffers(1, &bufferId);
    alBufferData(bufferId, format, pcmData.data(), static_cast<ALsizei>(pcmData.size()), sampleRate);
    
    std::cout << "Loaded OGG: " << path 
              << " (" << (pcmData.size() / 1024) << " KB, "
              << sampleRate << " Hz, "
              << info->channels << " channels)" << std::endl;
    
    return bufferId;
}

ALuint AudioManager::LoadMP3(const std::string& path) {
    mpg123_handle* mh = mpg123_new(nullptr, nullptr);
    if (!mh) {
        std::cerr << "Failed to create mpg123 handle" << std::endl;
        return 0;
    }
    
    if (mpg123_open(mh, path.c_str()) != MPG123_OK) {
        std::cerr << "Failed to open MP3 file: " << path 
                  << " - " << mpg123_strerror(mh) << std::endl;
        mpg123_delete(mh);
        return 0;
    }
    
    // Get file format
    long rate;
    int channels, encoding;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        std::cerr << "Failed to get MP3 format: " << path << std::endl;
        mpg123_close(mh);
        mpg123_delete(mh);
        return 0;
    }
    
    // Force output to signed 16-bit
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, MPG123_ENC_SIGNED_16);
    
    ALenum format;
    if (channels == 1) {
        format = AL_FORMAT_MONO16;
    } else {
        format = AL_FORMAT_STEREO16;
    }
    
    // Read all PCM data
    std::vector<char> pcmData;
    unsigned char buffer[4096];
    size_t bytesRead;
    
    while (mpg123_read(mh, buffer, sizeof(buffer), &bytesRead) == MPG123_OK) {
        pcmData.insert(pcmData.end(), buffer, buffer + bytesRead);
    }
    
    mpg123_close(mh);
    mpg123_delete(mh);
    
    if (pcmData.empty()) {
        std::cerr << "No PCM data decoded from MP3: " << path << std::endl;
        return 0;
    }
    
    // Upload to OpenAL
    ALuint bufferId;
    alGenBuffers(1, &bufferId);
    alBufferData(bufferId, format, pcmData.data(), static_cast<ALsizei>(pcmData.size()), static_cast<ALsizei>(rate));
    
    std::cout << "Loaded MP3: " << path 
              << " (" << (pcmData.size() / 1024) << " KB, "
              << rate << " Hz, "
              << channels << " channels)" << std::endl;
    
    return bufferId;
}

ALuint AudioManager::CreateSource() {
    ALuint source;
    alGenSources(1, &source);
    return source;
}

void AudioManager::ReleaseSource(ALuint source) {
    alDeleteSources(1, &source);
}