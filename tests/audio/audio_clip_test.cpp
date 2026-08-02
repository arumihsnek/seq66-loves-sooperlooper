/*
 *  Focused tests for the first Seq66/SooperLooper integration layer.
 */

#include <cassert>
#include <cmath>

#include "audio/audio_clip.hpp"
#include "audio/sooperlooper_client.hpp"

namespace
{

bool
near (double left, double right, double epsilon = 1.0e-9)
{
    return std::abs(left - right) <= epsilon;
}

}           // namespace anonymous

int
main ()
{
    seq66::audio_timing timing;
    timing.bars = 4;
    timing.beats_per_bar = 3;
    timing.beat_width = 4;
    timing.recorded_bpm = 120.0;

    assert(timing.valid());
    assert(near(timing.eighths_per_bar(), 6.0));
    assert(near(timing.total_quarter_notes(), 12.0));
    assert(near(timing.duration_seconds(120.0), 6.0));
    assert(near(timing.duration_seconds(60.0), 12.0));

    seq66::audio_clip clip("test-loop", 2);
    assert(clip.channels() == 2);
    assert(clip.timing(timing));
    clip.runtime_loop_index(0);

    clip.sync_mode(seq66::audio_sync_mode::elastic);
    assert(clip.supports_tempo(60.0));
    assert(near(clip.playback_rate(60.0), 1.0));
    assert(near(clip.time_stretch_ratio(60.0), 2.0));

    clip.sync_mode(seq66::audio_sync_mode::tape);
    assert(clip.supports_tempo(60.0));
    assert(near(clip.playback_rate(60.0), 0.5));
    assert(near(clip.time_stretch_ratio(60.0), 1.0));
    assert(! clip.supports_tempo(20.0));

    clip.sync_mode(seq66::audio_sync_mode::free);
    assert(clip.supports_tempo(20.0));
    assert(near(clip.playback_rate(20.0), 1.0));

    assert(clip.pitch_shift(7.0));
    assert(near(clip.pitch_shift(), 7.0));
    assert(! clip.pitch_shift(13.0));
    assert(near(clip.pitch_shift(), 7.0));

    seq66::sooperlooper_client client;
    assert(seq66::sooperlooper_client::compiled_support());
    assert(client.ready());
    assert(client.endpoint() == "osc.udp://127.0.0.1:9951/");

    return 0;
}
