#include "Audio.h"
#include "Assets.h"
#include "raylib.h"

namespace ReyesCuadra
{
    constexpr int   SOUND_COUNT    = static_cast<int>(SoundId::Count);
    constexpr int   PENDING_SLOTS  = 8;
    constexpr float MUSIC_VOLUME   = 0.35f;
    constexpr float SFX_VOLUME     = 0.7f;

    struct PendingSfx
    {
        SoundId id    = SoundId::Select;
        float   delay = 0.0f;
        bool    armed = false;
    };

    static Sound      uiSounds[SOUND_COUNT];
    static bool       soundLoaded[SOUND_COUNT] = {};
    static Music      exploreMusic;
    static Music      combatMusic;
    static bool       exploreLoaded = false;
    static bool       combatLoaded  = false;
    static MusicTrack currentTrack  = MusicTrack::None;
    static bool       audioReady    = false;
    static bool       muted         = false;
    static PendingSfx pending[PENDING_SLOTS];

    static const char* SOUND_FILES[SOUND_COUNT] =
    {
        "assets/audio/select.wav",
        "assets/audio/card.wav",
        "assets/audio/dice.wav",
        "assets/audio/success.wav",
        "assets/audio/fail.wav",
        "assets/audio/crit.wav",
        "assets/audio/hit.wav",
        "assets/audio/victory.wav"
    };

    static Music* TrackPointer(MusicTrack track)
    {
        if (track == MusicTrack::Explore && exploreLoaded) return &exploreMusic;
        if (track == MusicTrack::Combat  && combatLoaded)  return &combatMusic;
        return nullptr;
    }

    void LoadAudio()
    {
        InitAudioDevice();
        audioReady = IsAudioDeviceReady();
        if (!audioReady) return;

        for (int i = 0; i < SOUND_COUNT; ++i)
        {
            const char* path = FindAssetPath(SOUND_FILES[i]);
            if (path == nullptr) continue;

            uiSounds[i] = LoadSound(path);
            soundLoaded[i] = (uiSounds[i].frameCount > 0);
            if (soundLoaded[i]) SetSoundVolume(uiSounds[i], SFX_VOLUME);
        }

        const char* explorePath = FindAssetPath("assets/audio/music_explore.ogg");
        if (explorePath != nullptr)
        {
            exploreMusic = LoadMusicStream(explorePath);
            exploreLoaded = (exploreMusic.frameCount > 0);
            if (exploreLoaded)
            {
                exploreMusic.looping = true;
                SetMusicVolume(exploreMusic, MUSIC_VOLUME);
            }
        }

        const char* combatPath = FindAssetPath("assets/audio/music_combat.ogg");
        if (combatPath != nullptr)
        {
            combatMusic = LoadMusicStream(combatPath);
            combatLoaded = (combatMusic.frameCount > 0);
            if (combatLoaded)
            {
                combatMusic.looping = true;
                SetMusicVolume(combatMusic, MUSIC_VOLUME);
            }
        }
    }

    void UnloadAudio()
    {
        if (!audioReady) return;

        StopTrack();

        for (int i = 0; i < SOUND_COUNT; ++i)
        {
            if (soundLoaded[i]) UnloadSound(uiSounds[i]);
            soundLoaded[i] = false;
        }

        if (exploreLoaded) UnloadMusicStream(exploreMusic);
        if (combatLoaded)  UnloadMusicStream(combatMusic);
        exploreLoaded = false;
        combatLoaded  = false;

        CloseAudioDevice();
        audioReady = false;
    }

    void PlaySfx(SoundId id)
    {
        if (!audioReady || muted) return;

        int index = static_cast<int>(id);
        if (index < 0 || index >= SOUND_COUNT) return;
        if (!soundLoaded[index]) return;

        PlaySound(uiSounds[index]);
    }

    void PlaySfxDelayed(SoundId id, float delay)
    {
        if (delay <= 0.0f) { PlaySfx(id); return; }

        for (int i = 0; i < PENDING_SLOTS; ++i)
        {
            if (pending[i].armed) continue;

            pending[i].id    = id;
            pending[i].delay = delay;
            pending[i].armed = true;
            return;
        }
    }

    void UpdateAudio()
    {
        if (!audioReady) return;

        float delta = GetFrameTime();

        for (int i = 0; i < PENDING_SLOTS; ++i)
        {
            if (!pending[i].armed) continue;

            pending[i].delay -= delta;
            if (pending[i].delay <= 0.0f)
            {
                pending[i].armed = false;
                PlaySfx(pending[i].id);
            }
        }

        Music* music = TrackPointer(currentTrack);
        if (music != nullptr && !muted) UpdateMusicStream(*music);
    }

    void PlayTrack(MusicTrack track)
    {
        if (!audioReady) return;
        if (track == currentTrack) return;

        StopTrack();
        currentTrack = track;

        Music* music = TrackPointer(track);
        if (music != nullptr && !muted) PlayMusicStream(*music);
    }

    void StopTrack()
    {
        if (!audioReady) return;

        Music* music = TrackPointer(currentTrack);
        if (music != nullptr) StopMusicStream(*music);

        currentTrack = MusicTrack::None;
    }

    void ToggleMute()
    {
        muted = !muted;
        if (!audioReady) return;

        Music* music = TrackPointer(currentTrack);
        if (music == nullptr) return;

        if (muted) StopMusicStream(*music);
        else       PlayMusicStream(*music);
    }

    bool IsAudioMuted() { return muted; }
    bool IsAudioReady() { return audioReady; }
}
