/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_persistence.cpp
 *
 *  Transactional audio persistence implementation.
 *
 *  Uses a minimal JSON format without external dependencies.
 *  The format is:
 *
 *  {
 *    "schema_version": 1,
 *    "sequence_number": 42,
 *    "project_uuid": "abc-123",
 *    "slots": [
 *      {
 *        "clip_uuid": "clip-aaa",
 *        "loop_number": 0,
 *        "active": true,
 *        "tempo_mode": 2,
 *        "transport_playing": true,
 *        "recorded_tempo": 120.0
 *      }
 *    ]
 *  }
 */

#include "audio/sooperlooper_audio_persistence.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>

namespace seq66
{

/* ------------------------------------------------------------------ */
/*  persisted_audio_slot comparison                                    */
/* ------------------------------------------------------------------ */

bool
persisted_audio_slot::operator== (const persisted_audio_slot & rhs) const
{
    return clip_uuid == rhs.clip_uuid &&
           loop_number == rhs.loop_number &&
           active == rhs.active &&
           tempo_mode == rhs.tempo_mode &&
           transport_playing == rhs.transport_playing &&
           std::abs(recorded_tempo - rhs.recorded_tempo) < 0.001;
}

/* ------------------------------------------------------------------ */
/*  audio_project_state comparison                                    */
/* ------------------------------------------------------------------ */

bool
audio_project_state::operator== (const audio_project_state & rhs) const
{
    return schema_version == rhs.schema_version &&
           sequence_number == rhs.sequence_number &&
           project_uuid == rhs.project_uuid &&
           slots == rhs.slots;
}

/* ------------------------------------------------------------------ */
/*  Simple JSON helpers (no external library)                         */
/* ------------------------------------------------------------------ */

static std::string
json_escape (const std::string & s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string
json_unescape (const std::string & s)
{
    std::string out;
    out.reserve(s.size());
    bool escaped = false;
    for (char c : s)
    {
        if (escaped)
        {
            switch (c)
            {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                default:   out += c;    break;
            }
            escaped = false;
        }
        else if (c == '\\')
            escaped = true;
        else
            out += c;
    }
    return out;
}

/**
 *  Extract a JSON string value for the given key.
 */
static std::string
json_get_string (const std::string & json, const std::string & key)
{
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
        return "";

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos)
        return "";

    auto start = json.find('"', pos + 1);
    if (start == std::string::npos)
        return "";

    auto end = json.find('"', start + 1);
    if (end == std::string::npos)
        return "";

    return json_unescape(json.substr(start + 1, end - start - 1));
}

/**
 *  Extract a JSON integer value for the given key.
 */
static int64_t
json_get_int (const std::string & json, const std::string & key,
              int64_t def = 0)
{
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
        return def;

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos)
        return def;

    /* Skip whitespace */
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        ++pos;

    if (pos >= json.size())
        return def;

    /* Handle negative numbers */
    bool negative = false;
    if (json[pos] == '-')
    {
        negative = true;
        ++pos;
    }

    int64_t result = 0;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
    {
        result = result * 10 + (json[pos] - '0');
        ++pos;
    }
    return negative ? -result : result;
}

/**
 *  Extract a JSON double value for the given key.
 */
static double
json_get_double (const std::string & json, const std::string & key,
                 double def = 0.0)
{
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
        return def;

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos)
        return def;

    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        ++pos;

    if (pos >= json.size())
        return def;

    std::string num;
    while (pos < json.size() &&
           (json[pos] == '-' || json[pos] == '.' ||
            json[pos] == 'e' || json[pos] == 'E' ||
            json[pos] == '+' ||
            (json[pos] >= '0' && json[pos] <= '9')))
    {
        num += json[pos];
        ++pos;
    }

    if (num.empty())
        return def;

    char * end = nullptr;
    double result = std::strtod(num.c_str(), &end);
    return (end != num.c_str()) ? result : def;
}

/**
 *  Extract a JSON boolean value for the given key.
 */
static bool
json_get_bool (const std::string & json, const std::string & key,
               bool def = false)
{
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
        return def;

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos)
        return def;

    auto true_pos = json.find("true", pos + 1);
    auto false_pos = json.find("false", pos + 1);

    if (true_pos != std::string::npos &&
        (false_pos == std::string::npos || true_pos < false_pos) &&
        true_pos - pos < 10)
        return true;

    return false;
}

/**
 *  Extract a JSON array of slot objects.
 *  Returns the substring between [ and ] for the "slots" key.
 */
static std::vector<std::string>
json_get_slot_objects (const std::string & json)
{
    std::vector<std::string> result;
    std::string needle = "\"slots\"";
    auto arr_start = json.find(needle);
    if (arr_start == std::string::npos)
        return result;

    auto bracket = json.find('[', arr_start + needle.size());
    if (bracket == std::string::npos)
        return result;

    auto bracket_end = json.find(']', bracket + 1);
    if (bracket_end == std::string::npos)
        return result;

    std::string arr = json.substr(bracket + 1, bracket_end - bracket - 1);

    /* Extract each { ... } object */
    size_t pos = 0;
    while (pos < arr.size())
    {
        auto start = arr.find('{', pos);
        if (start == std::string::npos)
            break;

        /* Find matching closing brace */
        int depth = 0;
        size_t end = start;
        for (; end < arr.size(); ++end)
        {
            if (arr[end] == '{') ++depth;
            if (arr[end] == '}') --depth;
            if (depth == 0) break;
        }

        if (depth == 0)
            result.push_back(arr.substr(start, end - start + 1));

        pos = end + 1;
    }

    return result;
}

/* ------------------------------------------------------------------ */
/*  Simple CRC32 checksum                                             */
/* ------------------------------------------------------------------ */

static uint32_t
crc32_table_entry (int i)
{
    uint32_t crc = static_cast<uint32_t>(i);
    for (int j = 0; j < 8; ++j)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xEDB88320u;
        else
            crc = crc >> 1;
    }
    return crc;
}

static const uint32_t *
crc32_table ()
{
    static uint32_t table[256];
    static bool initialized = false;
    if (! initialized)
    {
        for (int i = 0; i < 256; ++i)
            table[i] = crc32_table_entry(i);
        initialized = true;
    }
    return table;
}

uint32_t
audio_persistence_manager::compute_checksum (const std::string & data)
{
    const uint32_t * table = crc32_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : data)
        crc = (crc >> 8) ^ table[(crc ^ c) & 0xFFu];
    return crc ^ 0xFFFFFFFFu;
}

/* ------------------------------------------------------------------ */
/*  Serialize                                                         */
/* ------------------------------------------------------------------ */

std::string
audio_persistence_manager::serialize (const audio_project_state & state) const
{
    std::ostringstream os;
    os << "{\n";
    os << "  \"schema_version\": " << state.schema_version << ",\n";
    os << "  \"sequence_number\": " << state.sequence_number << ",\n";
    os << "  \"project_uuid\": \"" << json_escape(state.project_uuid) << "\",\n";
    os << "  \"slots\": [\n";

    for (size_t i = 0; i < state.slots.size(); ++i)
    {
        const auto & s = state.slots[i];
        os << "    {\n";
        os << "      \"clip_uuid\": \"" << json_escape(s.clip_uuid) << "\",\n";
        os << "      \"loop_number\": " << s.loop_number << ",\n";
        os << "      \"active\": " << (s.active ? "true" : "false") << ",\n";
        os << "      \"tempo_mode\": " << s.tempo_mode << ",\n";
        os << "      \"transport_playing\": "
           << (s.transport_playing ? "true" : "false") << ",\n";
        os << "      \"recorded_tempo\": " << s.recorded_tempo << "\n";
        os << "    }";
        if (i + 1 < state.slots.size())
            os << ",";
        os << "\n";
    }

    os << "  ]\n";
    os << "}\n";
    return os.str();
}

/* ------------------------------------------------------------------ */
/*  Deserialize                                                       */
/* ------------------------------------------------------------------ */

persist_outcome
audio_persistence_manager::deserialize (
    const std::string & json_str,
    audio_project_state & state) const
{
    /* Schema version check */
    int64_t sv = json_get_int(json_str, "schema_version", 0);
    if (sv < 1 || sv > static_cast<int64_t>(AUDIO_PERSISTENCE_SCHEMA_VERSION))
    {
        return persist_outcome::error(
            persist_result::schema_mismatch,
            "Unsupported schema version: " + std::to_string(sv));
    }
    state.schema_version = static_cast<uint32_t>(sv);
    state.sequence_number = static_cast<uint32_t>(
        json_get_int(json_str, "sequence_number", 0));
    state.project_uuid = json_get_string(json_str, "project_uuid");

    /* Parse slots */
    auto slot_jsons = json_get_slot_objects(json_str);
    state.slots.clear();
    state.slots.reserve(slot_jsons.size());

    for (const auto & sj : slot_jsons)
    {
        persisted_audio_slot slot;
        slot.clip_uuid = json_get_string(sj, "clip_uuid");
        slot.loop_number = static_cast<int>(json_get_int(sj, "loop_number", -1));
        slot.active = json_get_bool(sj, "active", false);
        slot.tempo_mode = static_cast<int>(json_get_int(sj, "tempo_mode", 0));
        slot.transport_playing = json_get_bool(sj, "transport_playing", false);
        slot.recorded_tempo = json_get_double(sj, "recorded_tempo", 120.0);
        state.slots.push_back(slot);
    }

    return persist_outcome::success(state.sequence_number);
}

/* ------------------------------------------------------------------ */
/*  File I/O helpers                                                  */
/* ------------------------------------------------------------------ */

persist_outcome
audio_persistence_manager::write_and_fsync (
    const std::string & path,
    const std::string & content) const
{
    std::ofstream ofs(path, std::ios::binary);
    if (! ofs.is_open())
    {
        return persist_outcome::error(
            persist_result::file_error,
            "Cannot open for writing: " + path);
    }

    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (! ofs.good())
    {
        return persist_outcome::error(
            persist_result::file_error,
            "Write failed: " + path);
    }

    ofs.flush();
    if (! ofs.good())
    {
        return persist_outcome::error(
            persist_result::file_error,
            "Flush failed: " + path);
    }

    /* fsync via fileno */
    FILE * fp = nullptr;
    ofs.close();
    fp = std::fopen(path.c_str(), "r+b");
    if (fp)
    {
        int fd = fileno(fp);
        if (fsync(fd) != 0)
        {
            std::fclose(fp);
            return persist_outcome::error(
                persist_result::file_error,
                "fsync failed: " + path);
        }
        std::fclose(fp);
    }

    return persist_outcome::success(0);
}

std::string
audio_persistence_manager::create_backup (const std::string & path) const
{
    std::string backup = path + ".bak";

    /* Check if original exists */
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return "";  /* No file to back up */

    /* Read original */
    std::ifstream ifs(path, std::ios::binary);
    if (! ifs.is_open())
        return "";

    std::string content(
        (std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    /* Write backup */
    auto result = write_and_fsync(backup, content);
    return result.ok() ? backup : "";
}

persist_outcome
audio_persistence_manager::restore_backup (
    const std::string & backup_path,
    const std::string & target_path) const
{
    struct stat st;
    if (stat(backup_path.c_str(), &st) != 0)
    {
        return persist_outcome::error(
            persist_result::rollback_failed,
            "Backup not found: " + backup_path);
    }

    std::ifstream ifs(backup_path, std::ios::binary);
    if (! ifs.is_open())
    {
        return persist_outcome::error(
            persist_result::rollback_failed,
            "Cannot read backup: " + backup_path);
    }

    std::string content(
        (std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    /* Write to tmp, fsync, rename */
    std::string tmp = target_path + ".restore-tmp";
    auto wr = write_and_fsync(tmp, content);
    if (! wr.ok())
    {
        (void) std::remove(tmp.c_str());
        return persist_outcome::error(
            persist_result::rollback_failed,
            "Restore write failed: " + wr.detail);
    }

    if (std::rename(tmp.c_str(), target_path.c_str()) != 0)
    {
        (void) std::remove(tmp.c_str());
        return persist_outcome::error(
            persist_result::rollback_failed,
            "Restore rename failed: " + target_path);
    }

    return persist_outcome::success(0);
}

bool
audio_persistence_manager::media_exists (const std::string & media_path) const
{
    struct stat st;
    return stat(media_path.c_str(), &st) == 0;
}

bool
audio_persistence_manager::validate_state (const audio_project_state & state) const
{
    for (const auto & slot : state.slots)
    {
        if (slot.clip_uuid.empty())
            return false;
        if (slot.loop_number < 0)
            return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/*  Save (transactional)                                              */
/* ------------------------------------------------------------------ */

persist_outcome
audio_persistence_manager::save (
    const audio_project_state & state,
    const std::string & path) const
{
    /* Validate before writing */
    if (! validate_state(state))
    {
        return persist_outcome::error(
            persist_result::parse_error,
            "Invalid state: empty UUID or negative loop number");
    }

    /* Create backup if file exists */
    std::string backup = create_backup(path);

    /* Serialize */
    std::string json_str = serialize(state);

    /* Write to temp file */
    std::string tmp = path + ".tmp";
    auto wr = write_and_fsync(tmp, json_str);
    if (! wr.ok())
    {
        (void) std::remove(tmp.c_str());
        /* Try to restore backup */
        if (! backup.empty())
        {
            auto rb = restore_backup(backup, path);
            if (! rb.ok())
            {
                return persist_outcome::error(
                    persist_result::rollback_failed,
                    "Save failed and rollback failed: " + wr.detail);
            }
        }
        return wr;
    }

    /* Atomic rename */
    if (std::rename(tmp.c_str(), path.c_str()) != 0)
    {
        (void) std::remove(tmp.c_str());
        if (! backup.empty())
        {
            auto rb = restore_backup(backup, path);
            if (! rb.ok())
            {
                return persist_outcome::error(
                    persist_result::rollback_failed,
                    "Rename failed and rollback failed");
            }
        }
        return persist_outcome::error(
            persist_result::file_error,
            "Rename failed: " + path);
    }

    return persist_outcome::success(state.sequence_number);
}

/* ------------------------------------------------------------------ */
/*  Load                                                              */
/* ------------------------------------------------------------------ */

persist_outcome
audio_persistence_manager::load (
    const std::string & path,
    audio_project_state & state) const
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return persist_outcome::error(
            persist_result::file_error,
            "File not found: " + path);
    }

    std::ifstream ifs(path, std::ios::binary);
    if (! ifs.is_open())
    {
        return persist_outcome::error(
            persist_result::file_error,
            "Cannot open: " + path);
    }

    std::string content(
        (std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    if (content.empty())
    {
        return persist_outcome::error(
            persist_result::parse_error,
            "Empty file: " + path);
    }

    /* Deserialize */
    auto result = deserialize(content, state);
    if (! result.ok())
    {
        /* Try backup if available */
        std::string backup = path + ".bak";
        struct stat bst;
        if (stat(backup.c_str(), &bst) == 0)
        {
            auto rb = restore_backup(backup, path);
            if (rb.ok())
            {
                /* Retry load from restored file */
                return load(path, state);
            }
        }
        return result;
    }

    return persist_outcome::success(state.sequence_number);
}

} // namespace seq66

/*
 * sooperlooper_audio_persistence.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
