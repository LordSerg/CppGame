#include "AudioManager.h"
#include "../Utils/Math.h"
#include <iostream>
#include <vector>

// Vorbis decoding for OGG files would need additional library
// For now, this is a simplified implementation

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

ALuint AudioManager::LoadSound(const std::string& path) {
    auto it = soundBuffers.find(path);
    if (it != soundBuffers.end()) {
        return it->second.bufferId;
    }
    
    // This would need actual OGG file loading
    // Using vorbisfile or similar library
    // Simplified for now
    
    ALuint buffer;
    alGenBuffers(1, &buffer);
    
    // Load OGG file data
    // ...
    
    soundBuffers[path] = {buffer, path};
    return buffer;
}

ALuint AudioManager::CreateSource() {
    ALuint source;
    alGenSources(1, &source);
    return source;
}

void AudioManager::ReleaseSource(ALuint source) {
    alDeleteSources(1, &source);
}