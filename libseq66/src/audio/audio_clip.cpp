/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          audio_clip.cpp
 *
 *  Musical timing and rate calculations for external audio-loop slots.
 */

#include <cmath>

#include "audio/audio_clip.hpp"

namespace seq66
{

bool
audio_timing::valid () const
{
    return
    (
        bars > 0 && beats_per_bar > 0 && beat_width > 0 &&
        std::isfinite(recorded_bpm) && recorded_bpm > 0.0
    );
}

double
audio_timing::eighths_per_bar () const
{
    return valid() ? double(beats_per_bar) * 8.0 / double(beat_width) : 0.0;
}

double
audio_timing::total_quarter_notes () const
{
    return valid() ?
        double(bars) * double(beats_per_bar) * 4.0 / double(beat_width) :
        0.0 ;
}

double
audio_timing::duration_seconds (double bpm) const
{
    return valid() && std::isfinite(bpm) && bpm > 0.0 ?
        total_quarter_notes() * 60.0 / bpm :
        0.0 ;
}

audio_clip::audio_clip () :
    m_stable_id           (),
    m_runtime_loop_index  (-1),
    m_channels            (2),
    m_timing              (),
    m_sync_mode           (audio_sync_mode::elastic),
    m_state               (audio_clip_state::unknown),
    m_pitch_shift         (0.0)
{
    // No code needed.
}

audio_clip::audio_clip (const std::string & stable_id, int channels) :
    audio_clip            ()
{
    m_stable_id = stable_id;
    (void) this->channels(channels);
}

bool
audio_clip::channels (int value)
{
    bool result { value >= 1 && value <= 16 };
    if (result)
        m_channels = value;

    return result;
}

bool
audio_clip::timing (const audio_timing & value)
{
    bool result { value.valid() };
    if (result)
        m_timing = value;

    return result;
}

bool
audio_clip::pitch_shift (double semitones)
{
    bool result
    {
        std::isfinite(semitones) && semitones >= -12.0 && semitones <= 12.0
    };
    if (result)
        m_pitch_shift = semitones;

    return result;
}

double
audio_clip::playback_rate (double target_bpm) const
{
    if (! supports_tempo(target_bpm))
        return 1.0;

    return m_sync_mode == audio_sync_mode::tape ?
        target_bpm / m_timing.recorded_bpm :
        1.0 ;
}

double
audio_clip::time_stretch_ratio (double target_bpm) const
{
    if (! supports_tempo(target_bpm))
        return 1.0;

    return m_sync_mode == audio_sync_mode::elastic ?
        m_timing.recorded_bpm / target_bpm :
        1.0 ;
}

bool
audio_clip::supports_tempo (double target_bpm) const
{
    if
    (
        ! m_timing.valid() || ! std::isfinite(target_bpm) ||
        target_bpm <= 0.0
    )
    {
        return false;
    }

    if (m_sync_mode == audio_sync_mode::free)
        return true;

    if (m_sync_mode == audio_sync_mode::tape)
    {
        double rate { target_bpm / m_timing.recorded_bpm };
        return rate >= 0.25 && rate <= 4.0;
    }

    double ratio { m_timing.recorded_bpm / target_bpm };
    return ratio >= 0.5 && ratio <= 4.0;
}

}           // namespace seq66

/*
 * audio_clip.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
