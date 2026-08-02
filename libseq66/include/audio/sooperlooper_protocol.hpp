#if ! defined SEQ66_SOOPERLOOPER_PROTOCOL_HPP
#define SEQ66_SOOPERLOOPER_PROTOCOL_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_protocol.hpp
 *
 *  Central typed SooperLooper OSC protocol identifiers, payload kinds,
 *  ranges and bidirectional string mappings.
 *
 *  This header is the single source of truth for OSC string identifiers
 *  used by the fork.  It is consumed by `sooperlooper_client` (outbound
 *  commands, controls and global controls) and by the future inbound
 *  receiver (observed-state parsing).  Integration code MUST NOT
 *  introduce new raw protocol string literals; it must use the enums
 *  and accessors declared here.
 *
 *  Coverage is intentionally limited to identifiers already used by the
 *  outbound adapter and documented in
 *  `doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md`.  Unknown identifiers
 *  are rejected on the outbound side and preserved as raw integers on
 *  the inbound side, per spec.
 */

#include <string>

namespace seq66
{

/**
 *  Typed SooperLooper command verbs, sent via `/sl/<index>/hit`.
 *
 *  The names match the canonical SooperLooper command names declared
 *  in pinned source `src/command_map.cpp` and in the OSC contract
 *  document.  Range/signature is `s` (string).
 */
enum class sooperlooper_command
{
    record,
    overdub,
    multiply,
    insert,
    replace,
    reverse,
    mute,
    undo,
    redo,
    one_shot,
    trigger,
    substitute,
    pause,
    solo,
    mute_on,
    mute_off
};

/**
 *  Per-loop OSC control identifiers.  Each value maps to a single
 *  canonical string used by `/sl/<index>/set` and `/sl/<index>/get`.
 */
enum class loop_control
{
    rec_thresh,
    feedback,
    use_feedback_play,
    dry,
    wet,
    input_gain,
    use_safety_feedback,
    rate,
    use_rate,
    stretch_ratio,
    pitch_shift,
    tempo_stretch,
    round_integer_tempo,
    quantize,
    round,
    sync,
    playback_sync,
    relative_sync,
    mute_quantized,
    overdub_quantized,
    replace_quantized,
    redo_is_tap,
    jack_timebase_master,
    fade_samples,
    input_latency,
    output_latency,
    trigger_latency,
    autoset_latency,
    discrete_prefader,
    use_common_ins,
    use_common_outs,
    pan_1,
    pan_2,
    pan_3,
    pan_4,
    scratch_pos,
    delay_trigger,
    waiting,
    state,
    next_state,
    loop_len,
    loop_pos,
    cycle_len,
    free_time,
    total_time,
    rate_output,
    has_discrete_io,
    channel_count,
    in_peak_meter,
    out_peak_meter,
    is_soloed
};

/**
 *  Global OSC control identifiers.  Each value maps to a single
 *  canonical string used by `/set` and `/get` at the engine root.
 */
enum class global_control
{
    tempo,
    eighth_per_cycle,
    sync_source,
    tap_tempo,
    save_loop,
    auto_disable_latency,
    select_next_loop,
    select_prev_loop,
    select_all_loops,
    selected_loop_num,
    output_midi_clock,
    smart_eighths,
    use_midi_start,
    use_midi_stop,
    send_midi_start_on_trigger,
    global_cycle_len,
    global_cycle_pos
};

/**
 *  Coarse payload classification used by validation and observed-state
 *  parsing.
 */
enum class payload_kind
{
    boolean,                  /**< 0 or 1, finite.                            */
    integer_index,            /**< -1..N or -3, finite.                       */
    integer_raw,              /**< Finite, signed, used for state integers.   */
    ratio,                    /**< 0..1, finite (and signed -1..1 for pan).   */
    tempo_bpm,                /**< 0..1000, finite.                           */
    semitones,                /**< -12..12, finite.                           */
    playback_rate,            /**< 0.25..4, finite.                           */
    stretch_ratio,            /**< 0.5..4, finite.                            */
    eighths_per_cycle,        /**< 0..2048, finite.                           */
    seconds,                  /**< >= 0, finite.                              */
    samples,                  /**< 0..32768, finite.                          */
    peak_meter,               /**< >= 0, finite, may exceed 1.                */
    sync_source_index,        /**< -3..N, finite.                             */
    channel_count,            /**< 1..16, finite.                             */
    indexed_quantize,         /**< Tested named quantize modes.               */
    unconstrained             /**< No range guard; engine validates.          */
};

/**
 *  Returns the canonical OSC string for a `sooperlooper_command`.
 *  Returns an empty string if `command` is outside the enumerated
 *  range, allowing the caller to reject unknown outbound identifiers.
 */
const char * to_string (sooperlooper_command command);

/**
 *  Returns the canonical OSC string for a `loop_control`.
 *  Returns an empty string for out-of-range identifiers.
 */
const char * to_string (loop_control control);

/**
 *  Returns the canonical OSC string for a `global_control`.
 *  Returns an empty string for out-of-range identifiers.
 */
const char * to_string (global_control control);

/**
 *  Returns the payload classification for a `loop_control`.
 */
payload_kind metadata_for (loop_control control);

/**
 *  Returns the payload classification for a `global_control`.
 */
payload_kind metadata_for (global_control control);

/**
 *  Validates that `value` is finite and inside the documented range
 *  for `control`.  Returns `true` for `payload_kind::unconstrained`
 *  (provided the value is finite) and `false` for non-finite values
 *  regardless of kind.
 */
bool is_in_range (loop_control control, float value);

/**
 *  Validates that `value` is finite and inside the documented range
 *  for `control`.
 */
bool is_in_range (global_control control, float value);

/**
 *  Looks up a `loop_control` from a canonical OSC string.
 *
 *  \param[in] name
 *      The OSC identifier as it would appear on the wire.
 *
 *  \param[out] control
 *      Set on success.  Unchanged on failure.
 *
 *  \return
 *      `true` if `name` matched a canonical identifier.
 *
 *  Unknown inbound control names are NOT mapped here; the receiving
 *  side must preserve them as raw state until M1-002 introduces the
 *  strict parser.
 */
bool try_parse (const std::string & name, loop_control & control);

/**
 *  Looks up a `global_control` from a canonical OSC string.
 */
bool try_parse (const std::string & name, global_control & control);

/**
 *  Result of parsing a raw observed engine-state integer.
 */
struct state_parse_result
{
    bool known {false};           /**< True when raw matched a named state.   */
    const char * label {nullptr};/**< Canonical name when known, else null.   */
    int raw {0};                  /**< Original integer, always preserved.    */
};

/**
 *  Parses a raw engine-state integer into a canonical label.
 *
 *  Per `doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md`, public state
 *  values are -1 and 0..14 and 20.  Unknown future values are
 *  preserved in `result.raw` and reported with `known == false`, so
 *  the receiver never crashes nor silently collapses an unknown value
 *  to `off`.
 *
 *  \param[in] raw
 *      The raw integer as received from the engine.
 *
 *  \param[out] result
 *      Populated on every call.
 */
void parse_state_int (int raw, state_parse_result & result);

}           // namespace seq66

#endif      // SEQ66_SOOPERLOOPER_PROTOCOL_HPP

/*
 * sooperlooper_protocol.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
