#include "audio.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

static Sound synth(Cue cue) {
    constexpr int rate = 22050;
    constexpr double pi = 3.141592653589793;
    const bool gameplay = cue == Cue::Gameplay;
    const bool theme = cue == Cue::Theme || gameplay;
    const float beat = gameplay ? .3f : .4f;
    const float duration = gameplay            ? 4.8f
                           : theme             ? 3.2f
                           : cue == Cue::Win   ? .65f
                           : cue == Cue::Death ? .45f
                           : cue == Cue::Blast ? .32f
                                               : .16f;
    std::vector<int16_t> pcm(size_t(duration * rate));
    double phase = 0;
    uint32_t noise = 76543;
    const double melody[8] = {261.63, 329.63, 392.00, 523.25, 440.00, 392.00, 329.63, 293.66};
    const double gameMelody[16] = {329.63, 392,    440, 392,    293.66, 329.63, 392,    293.66,
                                   261.63, 329.63, 392, 329.63, 293.66, 392,    329.63, 293.66};
    const double bass[4] = {164.81, 146.83, 130.81, 146.83};
    double bassPhase = 0;
    for (size_t i = 0; i < pcm.size(); ++i) {
        double t = double(i) / rate, progress = t / duration, frequency = 440;
        switch (cue) {
        case Cue::Menu:
            frequency = 660;
            break;
        case Cue::Place:
            frequency = 420 - 260 * progress;
            break;
        case Cue::Blast:
            frequency = 90 - 55 * progress;
            break;
        case Cue::Pickup:
            frequency = 600 + 700 * progress;
            break;
        case Cue::Death:
            frequency = 400 - 310 * progress;
            break;
        case Cue::Win:
            frequency = melody[std::min(3, int(progress * 4))] * 2;
            break;
        case Cue::Theme:
            frequency = melody[std::min(7, int(t / .4))];
            break;
        case Cue::Gameplay:
            frequency = gameMelody[std::min(15, int(t / beat))];
            break;
        default:
            break;
        }
        phase += 2 * pi * frequency / rate;
        double value = std::sin(phase) * .7 + std::sin(phase * 2) * .2;
        if (cue == Cue::Blast) {
            noise ^= noise << 13;
            noise ^= noise >> 17;
            noise ^= noise << 5;
            value = .6 * (double(noise % 20001) / 10000 - 1) + .4 * std::sin(phase);
        }
        double envelope = std::min(1.0, t / .008) * std::pow(1 - progress, 2);
        if (theme) {
            double note = std::fmod(t, double(beat));
            envelope = std::min(1.0, note / .012) * std::max(0.0, 1 - note / beat);
            if (gameplay) {
                bassPhase += 2 * pi * bass[std::min(3, int(t / (beat * 4)))] / rate;
                value = .65 * value + .3 * std::sin(bassPhase);
            }
        }
        pcm[i] = int16_t(std::clamp(value * envelope, -1.0, 1.0) * 19000);
    }
    Wave wave{unsigned(pcm.size()), rate, 16, 1, pcm.data()};
    // raylib copies PCM into its own sound buffer; temporary samples can be freed.
    return LoadSoundFromWave(wave);
}
void Audio::load(bool disabled) {
    if (disabled)
        return;
    InitAudioDevice();
    ready = IsAudioDeviceReady();
    if (!ready)
        return;
    SetMasterVolume(.5f);
    for (int i = 0; i < int(Cue::Count); ++i) {
        sounds[i] = synth(Cue(i));
        if (IsSoundValid(sounds[i]))
            SetSoundVolume(sounds[i],
                           (i == int(Cue::Theme) || i == int(Cue::Gameplay)) ? .22f : .65f);
    }
}
void Audio::play(Cue cue) {
    if (ready && !muted && IsSoundValid(sounds[int(cue)])) {
        PlaySound(sounds[int(cue)]);
        ++played;
    }
}
void Audio::toggle() {
    muted = !muted;
    if (ready && muted)
        for (auto s : sounds)
            if (IsSoundValid(s))
                StopSound(s);
}
void Audio::music(BackgroundMusic mode) {
    if (!ready)
        return;
    if (mode != currentMusic) {
        for (Cue cue : {Cue::Theme, Cue::Gameplay})
            if (IsSoundValid(sounds[int(cue)]))
                StopSound(sounds[int(cue)]);
        currentMusic = mode;
    }
    if (mode == BackgroundMusic::None || muted)
        return;
    Cue cue = mode == BackgroundMusic::Title ? Cue::Theme : Cue::Gameplay;
    if (IsSoundValid(sounds[int(cue)]) && !IsSoundPlaying(sounds[int(cue)]))
        play(cue);
}
void Audio::events(const GameEvents &e) {
    if (e.placed)
        play(Cue::Place);
    if (e.exploded)
        play(Cue::Blast);
    if (e.collected)
        play(Cue::Pickup);
    if (e.died)
        play(Cue::Death);
    if (e.won)
        play(Cue::Win);
}
void Audio::unload() {
    if (ready) {
        for (auto s : sounds)
            if (IsSoundValid(s))
                UnloadSound(s);
        CloseAudioDevice();
    }
    ready = false;
    currentMusic = BackgroundMusic::None;
}
