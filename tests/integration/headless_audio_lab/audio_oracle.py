"""
Audio oracle for headless audio lab.
"""
from __future__ import annotations

import hashlib
import logging
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Any

logger = logging.getLogger(__name__)

@dataclass
class AnalysisResult:
    expected_loop_frames: int
    observed_loop_frames: int
    captured_wav_frames: int
    analyzed_segment_frames: int
    verification: str  # PASS, FAIL, or ERROR

    def to_dict(self) -> Dict[str, Any]:
        return {
            'expected_loop_frames': self.expected_loop_frames,
            'observed_loop_frames': self.observed_loop_frames,
            'captured_wav_frames': self.captured_wav_frames,
            'analyzed_segment_frames': self.analyzed_segment_frames,
            'verification': self.verification
        }

def read_wav_header(path: Path) -> dict:
    with open(path, 'rb') as f:
        riff = f.read(4)
        if riff != b'RIFF':
            raise ValueError("Not a RIFF file")
        _ = f.read(4)  # chunk size
        wave = f.read(4)
        if wave != b'WAVE':
            raise ValueError("Not a WAVE file")
        while True:
            chunk_id = f.read(4)
            if not chunk_id:
                break
            chunk_size = struct.unpack('<I', f.read(4))[0]
            if chunk_id == b'fmt ':
                audio_format = struct.unpack('<H', f.read(2))[0]
                num_channels = struct.unpack('<H', f.read(2))[0]
                sample_rate = struct.unpack('<I', f.read(4))[0]
                byte_rate = struct.unpack('<I', f.read(4))[0]
                block_align = struct.unpack('<H', f.read(2))[0]
                bits_per_sample = struct.unpack('<H', f.read(2))[0]
                f.seek(chunk_size - 16, 1)
                break
            else:
                f.seek(chunk_size, 1)
        while True:
            chunk_id = f.read(4)
            if not chunk_id:
                break
            chunk_size = struct.unpack('<I', f.read(4))[0]
            if chunk_id == b'data':
                data_size = chunk_size
                break
            else:
                f.seek(chunk_size, 1)
    return {
        'audio_format': audio_format,
        'num_channels': num_channels,
        'sample_rate': sample_rate,
        'byte_rate': byte_rate,
        'block_align': block_align,
        'bits_per_sample': bits_per_sample,
        'data_size': data_size
    }

def read_wav_samples(path: Path) -> tuple[list[float], dict]:
    header = read_wav_header(path)
    num_channels = header['num_channels']
    sample_rate = header['sample_rate']
    bits_per_sample = header['bits_per_sample']
    data_size = header['data_size']
    
    with open(path, 'rb') as f:
        f.seek(0)
        f.read(4)  # RIFF
        f.read(4)  # size
        f.read(4)  # WAVE
        while True:
            chunk_id = f.read(4)
            if not chunk_id:
                break
            chunk_size = struct.unpack('<I', f.read(4))[0]
            if chunk_id == b'data':
                break
            else:
                f.seek(chunk_size, 1)
        raw_data = f.read(data_size)
    
    if bits_per_sample == 16:
        fmt = f'<{data_size // 2}h'
        int_samples = struct.unpack(fmt, raw_data)
        samples = [sample / 32768.0 for sample in int_samples]
    elif bits_per_sample == 32:
        fmt = f'<{data_size // 4}f'
        samples = list(struct.unpack(fmt, raw_data))
    else:
        raise ValueError(f"Unsupported bits per sample: {bits_per_sample}")
    
    if num_channels == 2:
        samples = samples[0::2]
    elif num_channels > 2:
        samples = samples[0::num_channels]
    
    return samples, header

def compute_hash(samples: list[float]) -> str:
    b = b''
    for sample in samples:
        b += struct.pack('<d', sample)
    return hashlib.sha256(b).hexdigest()

def detect_beat_markers(samples: list[float], sample_rate: int, tempo: float, numerator: int) -> list[int]:
    return []

def detect_discontinuities(samples: list[float], threshold: float) -> list[int]:
    discontinuities = []
    for i in range(1, len(samples)):
        if abs(samples[i] - samples[i-1]) > threshold:
            discontinuities.append(i)
    return discontinuities

def analyze_wav(
    wav_path: str | Path,
    expected_loop_frames: int,
    observed_loop_frames: int,
    tempo: float,
    numerator: int,
    sample_rate: int,
    silence_threshold: float = 0.01,
    peak_threshold: float = 0.9
) -> AnalysisResult:
    wav_path = Path(wav_path)
    samples, header = read_wav_samples(wav_path)
    captured_wav_frames = len(samples)
    
    if len(samples) > 0:
        rms = (sum(s * s for s in samples) / len(samples)) ** 0.5
        if rms < silence_threshold:
            return AnalysisResult(
                expected_loop_frames=expected_loop_frames,
                observed_loop_frames=observed_loop_frames,
                captured_wav_frames=captured_wav_frames,
                analyzed_segment_frames=0,
                verification='FAIL'
            )
    
    analyzed_segment_frames = min(captured_wav_frames, expected_loop_frames)
    verification = 'PASS' if observed_loop_frames == expected_loop_frames else 'FAIL'
    
    return AnalysisResult(
        expected_loop_frames=expected_loop_frames,
        observed_loop_frames=observed_loop_frames,
        captured_wav_frames=captured_wav_frames,
        analyzed_segment_frames=analyzed_segment_frames,
        verification=verification
    )

def analyze_loop_reproduction(
    source_wav: str | Path,
    captured_wav: str | Path,
    expected_loop_frames: int,
    tolerance_percent: float = 10.0
) -> dict:
    source_path = Path(source_wav)
    captured_path = Path(captured_wav)
    
    source_samples, _ = read_wav_samples(source_path)
    captured_samples, _ = read_wav_samples(captured_path)
    
    source_len = len(source_samples)
    captured_len = len(captured_samples)
    
    if source_len == 0:
        ratio = 0.0
    else:
        ratio = captured_len / source_len
    
    expected_ratio = 1.0
    tolerance = tolerance_percent / 100.0
    lower_bound = expected_ratio * (1 - tolerance)
    upper_bound = expected_ratio * (1 + tolerance)
    
    verification = 'PASS' if lower_bound <= ratio <= upper_bound else 'FAIL'
    
    return {
        'source_frames': source_len,
        'captured_frames': captured_len,
        'ratio': ratio,
        'verification': verification
    }

def main():
    print("Audio oracle module - not meant to be run directly")
    return 0

if __name__ == '__main__':
    main()
