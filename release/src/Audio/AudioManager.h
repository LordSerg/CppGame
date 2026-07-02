#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <string>
#include <unordered_map>
#include <AL/al.h>
#include <AL/alc.h>
#include <vector>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    
    bool Initialize();
    void Shutdown();
    
    // Music
    void PlayMusic(const std::string& path, bool loop = true);
    void StopMusic();
    void SetMusicVolume(float volume);
    
    // Sound effects
    void PlaySound(const std::string& path, float volume = 1.0f);
    void PlaySoundAt(const std::string& path, const class Vector2& position, 
                    float volume = 1.0f);
    
    void SetMasterVolume(float volume);
    void SetSFXVolume(float volume);
    
    void Update(const Vector2& listenerPosition);
    
private:
    ALCdevice* device;
    ALCcontext* context;
    
    struct SoundBuffer {
        ALuint bufferId;
        std::string path;
    };
    
    std::unordered_map<std::string, SoundBuffer> soundBuffers;
    
    ALuint musicSource;
    std::vector<ALuint> activeSources;
    
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    
    ALuint LoadSound(const std::string& path);
    ALuint CreateSource();
    void ReleaseSource(ALuint source);
};

#endif // AUDIOMANAGER_H