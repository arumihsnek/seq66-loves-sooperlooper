#if ! defined SEQ66_SOOPERLOOPER_AUDIO_PERSISTENCE_HPP
#define SEQ66_SOOPERLOOPER_AUDIO_PERSISTENCE_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_persistence.hpp
 *
 *  Transactional audio persistence for SooperLooper slots.
 *
 *  Design principles:
 *  - Save is transactional: write to temp, fsync, rename.
 *  - Failed save never replaces the prior valid project.
 *  - Schema version is explicit; migration is forward-compatible.
 *  - Clip identity is UUID; runtime indexes are never persisted.
 *  - Missing media is visible and recoverable.
 *  - No Qt dependencies; pure C++17.
 */

#include "audio/sooperlooper_audio_slot_widget.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace seq66
{

/**
 *  Schema version for the audio persistence format.
 */
static constexpr uint32_t AUDIO_PERSISTENCE_SCHEMA_VERSION = 1;

/**
 *  A single persisted audio slot entry.
 *
 *  Mirrors audio_slot_model but is designed for serialization.
 *  Runtime index is deliberately absent — UUID is stable identity.
 */
struct persisted_audio_slot
{
    std::string clip_uuid;
    int loop_number{-1};
    bool active{false};
    int tempo_mode{0};          /* 0=free, 1=tape, 2=elastic */
    bool transport_playing{false};
    double recorded_tempo{120.0};

    bool operator== (const persisted_audio_slot & rhs) const;
    bool operator!= (const persisted_audio_slot & rhs) const
    {
        return ! (*this == rhs);
    }
};

/**
 *  The complete persisted audio project state.
 */
struct audio_project_state
{
    uint32_t schema_version{AUDIO_PERSISTENCE_SCHEMA_VERSION};
    uint32_t sequence_number{0};    /* Monotonic, bumped on each save. */
    std::string project_uuid;
    std::vector<persisted_audio_slot> slots;

    bool operator== (const audio_project_state & rhs) const;
    bool operator!= (const audio_project_state & rhs) const
    {
        return ! (*this == rhs);
    }
};

/**
 *  Result codes for persistence operations.
 */
enum class persist_result
{
    ok,
    file_error,         /* I/O error opening/reading/writing. */
    parse_error,        /* JSON parsing failed. */
    schema_mismatch,    /* Schema version unsupported. */
    corruption_detected,/* Checksum or structural mismatch. */
    missing_media,      /* Referenced media file not found. */
    rollback_failed     /* Could not restore prior valid state. */
};

/**
 *  Detailed result of a persistence operation.
 */
struct persist_outcome
{
    persist_result code{persist_result::ok};
    std::string detail;
    uint32_t schema_version{0};
    uint32_t sequence_number{0};

    bool ok () const { return code == persist_result::ok; }

    static persist_outcome success (uint32_t seq)
    {
        persist_outcome o;
        o.code = persist_result::ok;
        o.sequence_number = seq;
        return o;
    }

    static persist_outcome error (persist_result c, const std::string & msg)
    {
        persist_outcome o;
        o.code = c;
        o.detail = msg;
        return o;
    }
};

/**
 *  Audio persistence manager.
 *
 *  All methods are pure functions operating on the project state
 *  and file paths.  No I/O is performed in the constructor.
 */
class audio_persistence_manager
{
public:
    audio_persistence_manager () = default;

    /**
     *  Serialize project state to JSON string.
     */
    std::string serialize (const audio_project_state & state) const;

    /**
     *  Deserialize JSON string to project state.
     */
    persist_outcome deserialize (
        const std::string & json_str,
        audio_project_state & state) const;

    /**
     *  Save project state to file using transactional write.
     *
     *  Writes to path.tmp, fsyncs, then atomically renames to path.
     *  On failure, the original file is never modified.
     */
    persist_outcome save (
        const audio_project_state & state,
        const std::string & path) const;

    /**
     *  Load project state from file.
     */
    persist_outcome load (
        const std::string & path,
        audio_project_state & state) const;

    /**
     *  Create a backup of the current file before saving.
     *
     *  Returns the backup path, or empty on failure.
     */
    std::string create_backup (const std::string & path) const;

    /**
     *  Restore from backup if save failed.
     */
    persist_outcome restore_backup (
        const std::string & backup_path,
        const std::string & target_path) const;

    /**
     *  Check if a media file exists for the given UUID.
     */
    bool media_exists (const std::string & media_path) const;

    /**
     *  Validate that all slots have valid UUIDs.
     */
    bool validate_state (const audio_project_state & state) const;

    /**
     *  Compute a simple checksum for corruption detection.
     */
    static uint32_t compute_checksum (const std::string & data);

private:
    /**
     *  Write content to file and fsync.
     */
    persist_outcome write_and_fsync (
        const std::string & path,
        const std::string & content) const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_AUDIO_PERSISTENCE_HPP

/*
 * sooperlooper_audio_persistence.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
