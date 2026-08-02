# Development setup

## Supported development environment

The audio-loop integration is developed first on Linux with one of these audio
graphs:

- PipeWire with JACK compatibility;
- native JACK;
- JACK dummy for headless CI.

ALSA may still be used for MIDI, but ALSA-only audio is intentionally
unsupported for SooperLooper tracks in the initial implementation.

## Core dependencies

For the focused audio model and OSC contract tests:

- C++17 compiler;
- liblo development headers/library;
- pthread support.

For real-engine tests:

- JACK server and tools;
- SooperLooper build dependencies;
- autoconf/automake/libtool toolchain;
- libsigc++ 2, libxml2, libsndfile, libsamplerate, Rubber Band, FFTW and
  ncurses development packages.

The GitHub workflows are the executable reference for Ubuntu package names.
Other distributions should map those packages rather than copying names
blindly.

## Repository workflow

Current integration branch:

```text
feature/sooperlooper-audio-clips
```

Current integration PR remains a draft until the relevant phase gate is met.
Do not push integration changes directly to `master` and do not merge without
explicit human instruction.

## Focused local build

The current audio core can be compiled without a full Seq66 build. Create a
minimal generated configuration header:

```sh
mkdir -p .ci/include .ci/bin
cat > .ci/include/seq66-config.h <<'EOF'
#define SEQ66_HAVE_LIBLO 1
#define SEQ66_SOOPERLOOPER_SUPPORT 1
EOF
```

Compile the model test:

```sh
g++ \
  -std=c++17 \
  -Wall -Wextra -Wpedantic -Werror \
  -I.ci/include \
  -Ilibseq66/include \
  libseq66/src/audio/audio_clip.cpp \
  libseq66/src/audio/sooperlooper_client.cpp \
  tests/audio/audio_clip_test.cpp \
  -llo \
  -o .ci/bin/audio_clip_test

.ci/bin/audio_clip_test
```

Compile the fake-engine OSC contract test:

```sh
g++ \
  -std=c++17 \
  -Wall -Wextra -Wpedantic -Werror \
  -pthread \
  -I.ci/include \
  -Ilibseq66/include \
  libseq66/src/audio/audio_clip.cpp \
  libseq66/src/audio/sooperlooper_client.cpp \
  tests/audio/sooperlooper_osc_contract_test.cpp \
  -llo \
  -o .ci/bin/sooperlooper_osc_contract_test

.ci/bin/sooperlooper_osc_contract_test
```

The contract test binds UDP port 19951. Do not run multiple copies on the same
network namespace until the fixture supports dynamic ports.

## Pinned SooperLooper engine

The real-engine workflow currently pins:

```text
c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec
```

A revision change requires protocol/source review and a real-engine test run.
Do not replace the pin with an unrecorded moving branch.

Build the pinned engine without its GUI:

```sh
git clone https://github.com/essej/sooperlooper.git .ci/sooperlooper
git -C .ci/sooperlooper checkout --detach \
  c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec

cd .ci/sooperlooper
./autogen.sh
./configure --without-gui --prefix="$OLDPWD/.ci/sooperlooper-install"
make -j2
make install
cd "$OLDPWD"
```

Use the CI workflow as the current source for the complete package list.

## JACK dummy fixture

Start a non-realtime dummy server for tests:

```sh
jackd -r -d dummy -r 48000 -p 256 > .ci/jack.log 2>&1 &
echo $! > .ci/jack.pid
```

Wait for readiness by polling `jack_lsp`; do not assume that a fixed sleep means
the server is ready.

Start SooperLooper with deterministic names and no initial loops:

```sh
.ci/sooperlooper-install/bin/sooperlooper \
  --quiet \
  --loopcount=0 \
  --osc-port=19953 \
  --jack-name=seq66-sl-ci \
  > .ci/sooperlooper.log 2>&1 &
echo $! > .ci/sooperlooper.pid
```

Compile and run the real-engine smoke probe:

```sh
g++ \
  -std=c++17 \
  -Wall -Wextra -Wpedantic -Werror \
  -pthread \
  tests/audio/sooperlooper_real_engine_smoke.cpp \
  -llo \
  -o .ci/bin/sooperlooper_real_engine_smoke

SEQ66_SL_TEST_URL=osc.udp://127.0.0.1:19953/ \
  .ci/bin/sooperlooper_real_engine_smoke
```

The probe binds callback port 19952. It verifies ping feedback, loop-count
changes, loop creation, value feedback, control set/get, loop removal and
engine quit.

## Cleanup

Tests must kill only fixture processes they own:

```sh
kill "$(cat .ci/sooperlooper.pid)" 2>/dev/null || true
kill "$(cat .ci/jack.pid)" 2>/dev/null || true
```

The smoke probe normally requests `/quit`; cleanup remains necessary for failed
runs.

## Full Seq66 build

The focused tests do not replace a normal Meson build. When build wiring,
headers or shared interfaces change, also configure and compile relevant Seq66
CLI/GUI targets with audio support enabled and disabled.

The current implementation temporarily derives SooperLooper support from
Seq66's liblo availability. A later phase must introduce a dedicated option and
capability model so NSM and SooperLooper are not conceptually coupled.

## PipeWire development

When using PipeWire, run normal JACK clients through PipeWire's JACK
compatibility environment for the distribution. Verify that:

- SooperLooper appears as the controlled JACK client name;
- expected ports exist;
- connections are created through the graph actually used by Seq66;
- no separate native JACK server is accidentally started.

Do not add distribution-specific launch commands to product code without a
capability abstraction and tests.

## ALSA-only negative development

To test the hard block, ensure no JACK-compatible server/socket is available.
Expected behaviour:

- MIDI-only Seq66 remains usable;
- audio-loop creation is disabled;
- loaded audio clips remain visible as `backend_unavailable`;
- mute/unmute/launch/record and automation cannot unlock them;
- no OSC loop packet is emitted;
- no JACK server is launched implicitly.

## Diagnostics to preserve

For a failed real-engine run preserve:

- Seq66 commit;
- pinned SooperLooper SHA and printed version;
- JACK command, sample rate and period size;
- `jack_lsp` output;
- JACK and SooperLooper stderr logs;
- OSC endpoint/callback ports;
- test assertion or timeout;
- process exit status;
- temporary session/WAV files when relevant.

Do not commit large generated logs or audio captures. Upload them as CI
artifacts or attach them to the relevant issue/PR when needed.
