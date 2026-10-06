#pragma once

namespace ReyesCuadra
{
    enum class SoundId
    {
        Select = 0,
        Card,
        Dice,
        Success,
        Fail,
        Crit,
        Hit,
        Victory,
        Count
    };

    enum class MusicTrack
    {
        None = 0,
        Explore,
        Combat
    };

    void LoadAudio();
    void UnloadAudio();
    void UpdateAudio();

    void PlaySfx(SoundId id);
    void PlaySfxDelayed(SoundId id, float delay);

    void PlayTrack(MusicTrack track);
    void StopTrack();

    void ToggleMute();
    bool IsAudioMuted();
    bool IsAudioReady();
}
