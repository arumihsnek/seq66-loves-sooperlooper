#!/usr/bin/env python3
"""
Audio analyzer for the virtual studio.

Analyzes captured WAV files against expected parameters.
Produces machine-readable analysis results.
"""

import json
import struct
import sys
from pathlib import Path


def read_wav(path):
    """Read WAV file, return (header, samples_interleaved)."""
    with open(path, "rb") as f:
        raw = f.read()

    # Parse header
    riff = raw[0:4]
    if riff != b'RIFF':
        raise ValueError(f"Not a WAV file: missing RIFF header")

    wave = raw[8:12]
    if wave != b'WAVE':
        raise ValueError(f"Not a WAV file: missing WAVE marker")

    fmt = raw[12:16]
    if fmt != b'fmt ':
        raise ValueError(f"Not a WAV file: missing fmt marker")

    fmt_size = struct.unpack_from('<I', raw, 16)[0]
    audio_format = struct.unpack_from('<H', raw, 20)[0]
    num_channels = struct.unpack_from('<H', raw, 22)[0]
    sample_rate = struct.unpack_from('<I', raw, 24)[0]
    byte_rate = struct.unpack_from('<I', raw, 28)[0]
    block_align = struct.unpack_from('<H', raw, 32)[0]
    bits_per_sample = struct.unpack_from('<H', raw, 34)[0]

    # Find data chunk
    data_offset = 12 + 8 + fmt_size
    while data_offset < len(raw) - 8:
        chunk_id = raw[data_offset:data_offset+4]
        chunk_size = struct.unpack_from('<I', raw, data_offset+4)[0]
        if chunk_id == b'data':
            data_offset += 8
            break
        data_offset += 8 + chunk_size

    data_size = struct.unpack_from('<I', raw, data_offset-4)[0]
    data = raw[data_offset:data_offset+data_size]

    header = {
        "audio_format": audio_format,
        "num_channels": num_channels,
        "sample_rate": sample_rate,
        "byte_rate": byte_rate,
        "block_align": block_align,
        "bits_per_sample": bits_per_sample,
        "data_size": data_size,
    }

    # Parse samples
    if bits_per_sample == 32 and audio_format == 3:  # IEEE float
        n_samples = len(data) // 4
        samples = list(struct.unpack(f'<{n_samples}f', data))
    elif bits_per_sample == 16 and audio_format == 1:  # PCM 16-bit
        n_samples = len(data) // 2
        samples = [s / 32768.0 for s in struct.unpack(f'<{n_samples}h', data)]
    else:
        raise ValueError(f"Unsupported format: {bits_per_sample}bit, fmt={audio_format}")

    return header, samples


def analyze_markers(samples, sample_rate, num_channels, expected_bpm=120,
                    expected_numerator=4, expected_denominator=4):
    """Detect bar/beat markers in the captured audio."""
    frames_per_sample = 1
    frames_per_beat = int((60.0 / expected_bpm) * sample_rate)
    frames_per_bar = frames_per_beat * expected_numerator

    markers = []
    threshold = 0.3  # Marker detection threshold

    for i in range(0, len(samples), num_channels):
        frame = i // num_channels
        # Check for downbeat (strong marker)
        if num_channels >= 2:
            val = (abs(samples[i]) + abs(samples[i+1])) / 2.0
        else:
            val = abs(samples[i])

        if val > threshold:
            frame_in_bar = frame % frames_per_bar if frames_per_bar > 0 else 0
            is_downbeat = frame_in_bar < int(sample_rate * 0.001)  # <1ms into bar
            is_beat = (frame_in_bar % frames_per_beat) < int(sample_rate * 0.001)

            markers.append({
                "frame": frame,
                "value": val,
                "is_downbeat": is_downbeat,
                "is_beat": is_beat,
                "position_in_bar": frame_in_bar,
            })

    return markers


def analyze_continuity(samples, num_channels):
    """Check for continuity issues at loop boundaries."""
    # Find last non-zero frame
    last_nonzero = 0
    for i in range(len(samples) - num_channels, -1, -num_channels):
        if abs(samples[i]) > 0.001:
            last_nonzero = i // num_channels
            break

    # Check first and last samples
    first = [samples[i] for i in range(min(num_channels, len(samples)))]
    last = [samples[last_nonzero * num_channels + j] for j in range(num_channels)
            if last_nonzero * num_channels + j < len(samples)]

    # Calculate discontinuity
    if last:
        discontinuity = sum(abs(f - l) for f, l in zip(first, last)) / num_channels
    else:
        discontinuity = 0.0

    return {
        "first_samples": first,
        "last_samples": last,
        "last_nonzero_frame": last_nonzero,
        "discontinuity": discontinuity,
        "discontinuity_acceptable": discontinuity < 0.5,
    }


def analyze_levels(samples, num_channels, sample_rate):
    """Analyze audio levels and clipping."""
    n_frames = len(samples) // num_channels
    peak_l = 0.0
    peak_r = 0.0
    clipped = 0

    for i in range(0, len(samples), num_channels):
        frame = i // num_channels
        val_l = abs(samples[i]) if i < len(samples) else 0
        val_r = abs(samples[i+1]) if i+1 < len(samples) else 0
        peak_l = max(peak_l, val_l)
        peak_r = max(peak_r, val_r)
        if val_l > 0.99 or val_r > 0.99:
            clipped += 1

    duration_ms = (n_frames / sample_rate) * 1000.0

    return {
        "total_frames": n_frames,
        "duration_ms": duration_ms,
        "peak_left": peak_l,
        "peak_right": peak_r,
        "clipped_frames": clipped,
        "has_silence": peak_l < 0.001 and peak_r < 0.001,
    }


def analyze_wav(wav_path, expected=None):
    """Full analysis of a WAV file."""
    header, samples = read_wav(wav_path)
    n_channels = header["num_channels"]
    sr = header["sample_rate"]

    levels = analyze_levels(samples, n_channels, sr)
    continuity = analyze_continuity(samples, n_channels)

    # Expectations
    if expected:
        expected_frames = expected.get("expected_frames", 0)
        expected_ms = (expected_frames / sr * 1000.0) if sr > 0 else 0
        deviation_frames = levels["total_frames"] - expected_frames
        deviation_ms = deviation_frames / sr * 1000.0 if sr > 0 else 0

        frame_tolerance = expected.get("tolerance_frames", 256)

        result = {
            "header": header,
            "levels": levels,
            "continuity": continuity,
            "expected": {
                "frames": expected_frames,
                "ms": expected_ms,
                "tolerance_frames": frame_tolerance,
            },
            "deviation": {
                "frames": deviation_frames,
                "ms": deviation_ms,
                "within_tolerance": abs(deviation_frames) <= frame_tolerance,
            },
            "verification": "PASS" if (abs(deviation_frames) <= frame_tolerance and
                                        not levels["has_silence"]) else "FAIL",
        }
    else:
        result = {
            "header": header,
            "levels": levels,
            "continuity": continuity,
            "verification": "ANALYZED",
        }

    return result


def main():
    if len(sys.argv) < 2:
        print("Usage: analyzer.py <wav_file> [expected_frames] [tolerance_frames]")
        sys.exit(1)

    wav_path = sys.argv[1]
    expected_frames = int(sys.argv[2]) if len(sys.argv) > 2 else None
    tolerance = int(sys.argv[3]) if len(sys.argv) > 3 else 256

    expected = None
    if expected_frames:
        expected = {"expected_frames": expected_frames, "tolerance_frames": tolerance}

    result = analyze_wav(wav_path, expected)
    print(json.dumps(result, indent=2))
    return 0 if result.get("verification") in ("PASS", "ANALYZED") else 1


if __name__ == "__main__":
    sys.exit(main())
