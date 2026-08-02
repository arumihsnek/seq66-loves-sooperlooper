#if ! defined SEQ66_AUDIO_CLIP_HPP
#define SEQ66_AUDIO_CLIP_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          audio_clip.hpp
 *
 *  Provides the transport-independent model for a SooperLooper-backed audio
 *  slot. Audio data remains owned and processed by the external engine.
 */

#include <string>

namespace seq66
{

enum class audio_sync_mode
{
    free,                       /**< Keep original duration and pitch.       */
    tape,                       /**< Follow tempo by rate; pitch follows.    */
    elastic                     /**< Follow tempo while preserving pitch.   */
};

enum class audio_clip_state : int
{
    unknown       = -1,
    off           = 0,
    wait_start    = 1,
    recording     = 2,
    wait_stop     = 3,
    playing       = 4,
    overdubbing   = 5,
    multiplying   = 6,
    inserting     = 7,
    replacing     = 8,
    delay         = 9,
    muted         = 10,
    scratching    = 11,
    one_shot      = 12,
    substituting  = 13,
    paused        = 14,
    off_muted     = 20
};

struct audio_timing
{
    int bars {1};
    int beats_per_bar {4};
    int beat_width {4};
    double recorded_bpm {120.0};

    bool valid () const;
    double eighths_per_bar () const;
    double total_quarter_notes () const;
    double duration_seconds (double bpm) const;
};

class audio_clip
{

private:

    std::string m_stable_id;
    int m_runtime_loop_index;
    int m_channels;
    audio_timing m_timing;
    audio_sync_mode m_sync_mode;
    audio_clip_state m_state;
    double m_pitch_shift;

public:

    audio_clip ();
    explicit audio_clip (const std::string & stable_id, int channels = 2);

    const std::string & stable_id () const
    {
        return m_stable_id;
    }

    int runtime_loop_index () const
    {
        return m_runtime_loop_index;
    }

    int channels () const
    {
        return m_channels;
    }

    const audio_timing & timing () const
    {
        return m_timing;
    }

    audio_sync_mode sync_mode () const
    {
        return m_sync_mode;
    }

    audio_clip_state state () const
    {
        return m_state;
    }

    double pitch_shift () const
    {
        return m_pitch_shift;
    }

    void stable_id (const std::string & value)
    {
        m_stable_id = value;
    }

    void runtime_loop_index (int value)
    {
        m_runtime_loop_index = value;
    }

    bool channels (int value);
    bool timing (const audio_timing & value);

    void sync_mode (audio_sync_mode value)
    {
        m_sync_mode = value;
    }

    void state (audio_clip_state value)
    {
        m_state = value;
    }

    bool pitch_shift (double semitones);

    double playback_rate (double target_bpm) const;
    double time_stretch_ratio (double target_bpm) const;
    bool supports_tempo (double target_bpm) const;
};

}           // namespace seq66

#endif      // SEQ66_AUDIO_CLIP_HPP

/*
 * audio_clip.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
