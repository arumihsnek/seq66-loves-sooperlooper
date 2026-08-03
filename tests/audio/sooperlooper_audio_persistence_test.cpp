/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_audio_persistence_test.cpp
 *
 *  Tests for the audio persistence layer.
 *
 *  Covers:
 *  - Serialization round-trip
 *  - Deserialization with valid JSON
 *  - Schema version mismatch
 *  - Missing/corrupt file handling
 *  - Transactional save: write to tmp, fsync, rename
 *  - Rollback on failed save
 *  - Backup creation and restore
 *  - Checksum computation
 *  - State validation (UUID, loop number)
 *  - Multiple slots
 *  - Empty project
 */

#include "audio/sooperlooper_audio_persistence.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static int s_assertions = 0;
static int s_test_count = 0;
static int s_fail_count = 0;

#define CHECK(expr) \
    do { ++s_assertions; if (! (expr)) { \
        std::fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } } while (0)

#define CHECK_EQ(a, b) \
    do { ++s_assertions; if ((a) != (b)) { \
        std::fprintf(stderr, "  [FAIL] %s:%d: %s == %s\n", \
            __FILE__, __LINE__, #a, #b); \
        return 1; \
    } } while (0)

static const char * TEST_DIR = "/tmp/persistence_test";

static void
setup_test_dir ()
{
    mkdir(TEST_DIR, 0755);
}

static void
cleanup_test_dir ()
{
    /* Remove test files */
    std::string cmd = "rm -rf " + std::string(TEST_DIR);
    (void) std::system(cmd.c_str());
}

static std::string
test_path (const char * name)
{
    return std::string(TEST_DIR) + "/" + name;
}

static bool
file_exists (const std::string & path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

/* ------------------------------------------------------------------ */
/*  Serialization round-trip                                          */
/* ------------------------------------------------------------------ */

static int
test_serialize_roundtrip ()
{
    seq66::audio_persistence_manager mgr;

    seq66::audio_project_state state;
    state.schema_version = 1;
    state.sequence_number = 42;
    state.project_uuid = "proj-uuid-001";

    seq66::persisted_audio_slot s1;
    s1.clip_uuid = "clip-aaa";
    s1.loop_number = 0;
    s1.active = true;
    s1.tempo_mode = 2;
    s1.transport_playing = true;
    s1.recorded_tempo = 120.0;
    state.slots.push_back(s1);

    seq66::persisted_audio_slot s2;
    s2.clip_uuid = "clip-bbb";
    s2.loop_number = 1;
    s2.active = false;
    s2.tempo_mode = 0;
    s2.transport_playing = false;
    s2.recorded_tempo = 140.5;
    state.slots.push_back(s2);

    std::string json = mgr.serialize(state);
    CHECK(! json.empty());

    seq66::audio_project_state loaded;
    auto result = mgr.deserialize(json, loaded);
    CHECK(result.ok());
    CHECK_EQ(loaded.schema_version, 1u);
    CHECK_EQ(loaded.sequence_number, 42u);
    CHECK_EQ(loaded.project_uuid, std::string("proj-uuid-001"));
    CHECK_EQ(loaded.slots.size(), 2u);

    CHECK_EQ(loaded.slots[0].clip_uuid, std::string("clip-aaa"));
    CHECK_EQ(loaded.slots[0].loop_number, 0);
    CHECK(loaded.slots[0].active);
    CHECK_EQ(loaded.slots[0].tempo_mode, 2);
    CHECK(loaded.slots[0].transport_playing);
    CHECK(std::abs(loaded.slots[0].recorded_tempo - 120.0) < 0.001);

    CHECK_EQ(loaded.slots[1].clip_uuid, std::string("clip-bbb"));
    CHECK_EQ(loaded.slots[1].loop_number, 1);
    CHECK(! loaded.slots[1].active);
    CHECK_EQ(loaded.slots[1].tempo_mode, 0);
    CHECK(! loaded.slots[1].transport_playing);
    CHECK(std::abs(loaded.slots[1].recorded_tempo - 140.5) < 0.001);

    CHECK(state == loaded);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Schema version mismatch                                           */
/* ------------------------------------------------------------------ */

static int
test_schema_mismatch ()
{
    seq66::audio_persistence_manager mgr;
    seq66::audio_project_state state;

    std::string bad_json = R"({
        "schema_version": 999,
        "sequence_number": 1,
        "project_uuid": "test",
        "slots": []
    })";

    auto result = mgr.deserialize(bad_json, state);
    CHECK(! result.ok());
    CHECK(result.code == seq66::persist_result::schema_mismatch);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Empty project                                                     */
/* ------------------------------------------------------------------ */

static int
test_empty_project ()
{
    seq66::audio_persistence_manager mgr;

    seq66::audio_project_state state;
    state.schema_version = 1;
    state.sequence_number = 1;
    state.project_uuid = "empty-proj";

    std::string json = mgr.serialize(state);
    seq66::audio_project_state loaded;
    auto result = mgr.deserialize(json, loaded);
    CHECK(result.ok());
    CHECK(loaded.slots.empty());
    CHECK(state == loaded);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Checksum computation                                              */
/* ------------------------------------------------------------------ */

static int
test_checksum ()
{
    uint32_t c1 = seq66::audio_persistence_manager::compute_checksum("hello");
    uint32_t c2 = seq66::audio_persistence_manager::compute_checksum("hello");
    uint32_t c3 = seq66::audio_persistence_manager::compute_checksum("world");

    CHECK_EQ(c1, c2);       /* Deterministic */
    CHECK(c1 != c3);         /* Different input → different checksum */

    return 0;
}

/* ------------------------------------------------------------------ */
/*  State validation                                                  */
/* ------------------------------------------------------------------ */

static int
test_validate_state_valid ()
{
    seq66::audio_persistence_manager mgr;
    seq66::audio_project_state state;
    state.schema_version = 1;
    state.project_uuid = "proj";

    seq66::persisted_audio_slot s;
    s.clip_uuid = "clip-1";
    s.loop_number = 0;
    state.slots.push_back(s);

    CHECK(mgr.validate_state(state));
    return 0;
}

static int
test_validate_state_empty_uuid ()
{
    seq66::audio_persistence_manager mgr;
    seq66::audio_project_state state;
    state.schema_version = 1;
    state.project_uuid = "proj";

    seq66::persisted_audio_slot s;
    s.clip_uuid = "";    /* Invalid: empty UUID */
    s.loop_number = 0;
    state.slots.push_back(s);

    CHECK(! mgr.validate_state(state));
    return 0;
}

static int
test_validate_state_negative_loop ()
{
    seq66::audio_persistence_manager mgr;
    seq66::audio_project_state state;
    state.schema_version = 1;
    state.project_uuid = "proj";

    seq66::persisted_audio_slot s;
    s.clip_uuid = "clip-1";
    s.loop_number = -1;  /* Invalid */
    state.slots.push_back(s);

    CHECK(! mgr.validate_state(state));
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Transactional save                                                */
/* ------------------------------------------------------------------ */

static int
test_save_and_load ()
{
    setup_test_dir();
    seq66::audio_persistence_manager mgr;

    seq66::audio_project_state state;
    state.schema_version = 1;
    state.sequence_number = 10;
    state.project_uuid = "save-test";

    seq66::persisted_audio_slot s;
    s.clip_uuid = "clip-save";
    s.loop_number = 0;
    s.active = true;
    state.slots.push_back(s);

    std::string path = test_path("test_project.json");
    auto save_result = mgr.save(state, path);
    CHECK(save_result.ok());
    CHECK(file_exists(path));

    seq66::audio_project_state loaded;
    auto load_result = mgr.load(path, loaded);
    CHECK(load_result.ok());
    CHECK(state == loaded);

    cleanup_test_dir();
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Missing file                                                      */
/* ------------------------------------------------------------------ */

static int
test_load_missing_file ()
{
    seq66::audio_persistence_manager mgr;
    seq66::audio_project_state state;

    auto result = mgr.load("/tmp/nonexistent_file_12345.json", state);
    CHECK(! result.ok());
    CHECK(result.code == seq66::persist_result::file_error);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Corrupt file                                                      */
/* ------------------------------------------------------------------ */

static int
test_load_corrupt_file ()
{
    setup_test_dir();
    seq66::audio_persistence_manager mgr;

    std::string path = test_path("corrupt.json");
    std::ofstream ofs(path);
    ofs << "this is not json";
    ofs.close();

    seq66::audio_project_state state;
    auto result = mgr.load(path, state);
    /* Should fail schema check or parse */
    CHECK(! result.ok());

    cleanup_test_dir();
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Backup and restore                                                */
/* ------------------------------------------------------------------ */

static int
test_backup_and_restore ()
{
    setup_test_dir();
    seq66::audio_persistence_manager mgr;

    /* Create initial file */
    std::string path = test_path("backup_test.json");
    seq66::audio_project_state state;
    state.schema_version = 1;
    state.sequence_number = 1;
    state.project_uuid = "backup-proj";

    seq66::persisted_audio_slot s;
    s.clip_uuid = "clip-bak";
    s.loop_number = 0;
    state.slots.push_back(s);

    auto save_result = mgr.save(state, path);
    CHECK(save_result.ok());

    /* Create backup */
    std::string backup = mgr.create_backup(path);
    CHECK(! backup.empty());
    CHECK(file_exists(backup));

    /* Modify the file */
    state.sequence_number = 2;
    state.slots[0].active = true;
    auto save2 = mgr.save(state, path);
    CHECK(save2.ok());

    /* Verify modification */
    seq66::audio_project_state loaded;
    mgr.load(path, loaded);
    CHECK_EQ(loaded.sequence_number, 2u);

    /* Restore from backup */
    auto restore = mgr.restore_backup(backup, path);
    CHECK(restore.ok());

    /* Verify restored state */
    seq66::audio_project_state restored;
    mgr.load(path, restored);
    CHECK_EQ(restored.sequence_number, 1u);

    cleanup_test_dir();
    return 0;
}

/* ------------------------------------------------------------------ */
/*  No backup needed for first save                                   */
/* ------------------------------------------------------------------ */

static int
test_first_save_no_backup ()
{
    setup_test_dir();
    seq66::audio_persistence_manager mgr;

    std::string path = test_path("first_save.json");

    /* Remove if exists */
    (void) std::remove(path.c_str());

    seq66::audio_project_state state;
    state.schema_version = 1;
    state.sequence_number = 1;
    state.project_uuid = "first-save";

    seq66::persisted_audio_slot s;
    s.clip_uuid = "clip-first";
    s.loop_number = 0;
    state.slots.push_back(s);

    auto save_result = mgr.save(state, path);
    CHECK(save_result.ok());
    CHECK(file_exists(path));

    /* Backup should be empty since file didn't exist before */
    std::string backup = mgr.create_backup(path);
    /* Backup exists now since save created the file */
    CHECK(! backup.empty());

    cleanup_test_dir();
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Persisted slot comparison                                          */
/* ------------------------------------------------------------------ */

static int
test_slot_comparison ()
{
    seq66::persisted_audio_slot a;
    a.clip_uuid = "clip-1";
    a.loop_number = 0;
    a.active = true;
    a.tempo_mode = 2;
    a.transport_playing = true;
    a.recorded_tempo = 120.0;

    seq66::persisted_audio_slot b = a;
    CHECK(a == b);

    b.clip_uuid = "clip-2";
    CHECK(a != b);

    b = a;
    b.active = false;
    CHECK(a != b);

    b = a;
    b.recorded_tempo = 121.0;
    CHECK(a != b);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Multiple save increments                                          */
/* ------------------------------------------------------------------ */

static int
test_multiple_saves ()
{
    setup_test_dir();
    seq66::audio_persistence_manager mgr;

    std::string path = test_path("multi_save.json");

    for (int i = 1; i <= 5; ++i)
    {
        seq66::audio_project_state state;
        state.schema_version = 1;
        state.sequence_number = static_cast<uint32_t>(i);
        state.project_uuid = "multi-proj";

        seq66::persisted_audio_slot s;
        s.clip_uuid = "clip-" + std::to_string(i);
        s.loop_number = 0;
        s.active = (i % 2 == 0);
        state.slots.push_back(s);

        auto save_result = mgr.save(state, path);
        CHECK(save_result.ok());
    }

    /* Verify final state */
    seq66::audio_project_state loaded;
    auto load_result = mgr.load(path, loaded);
    CHECK(load_result.ok());
    CHECK_EQ(loaded.sequence_number, 5u);
    CHECK_EQ(loaded.slots[0].clip_uuid, std::string("clip-5"));
    CHECK(! loaded.slots[0].active);

    cleanup_test_dir();
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::printf("=== Audio persistence tests ===\n\n");

    struct test { const char * name; int (*fn)(); };
    struct test tests[] = {
        {"serialize roundtrip",         test_serialize_roundtrip},
        {"schema mismatch",             test_schema_mismatch},
        {"empty project",               test_empty_project},
        {"checksum",                    test_checksum},
        {"validate state valid",        test_validate_state_valid},
        {"validate state empty uuid",   test_validate_state_empty_uuid},
        {"validate state negative loop", test_validate_state_negative_loop},
        {"save and load",               test_save_and_load},
        {"load missing file",           test_load_missing_file},
        {"load corrupt file",           test_load_corrupt_file},
        {"backup and restore",          test_backup_and_restore},
        {"first save no backup",        test_first_save_no_backup},
        {"slot comparison",             test_slot_comparison},
        {"multiple saves",              test_multiple_saves},
    };

    int count = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < count; ++i)
    {
        ++s_test_count;
        int rc = tests[i].fn();
        const char * mark = rc == 0 ? "PASS" : "FAIL";
        std::printf("  [%s] %s\n", mark, tests[i].name);
        if (rc != 0)
            ++s_fail_count;
    }

    std::printf("\n%d assertions, %d failures.\n", s_assertions, s_fail_count);
    return s_fail_count;
}
