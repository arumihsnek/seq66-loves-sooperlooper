from __future__ import annotations
import wave
class AnalysisResult:
    def __init__(self, expected_loop_frames: int, observed_loop_frames: int, captured_wav_frames: int, analyzed_segment_frames: int, verification: str):
        self.expected_loop_frames=expected_loop_frames; self.observed_loop_frames=observed_loop_frames
        self.captured_wav_frames=captured_wav_frames; self.analyzed_segment_frames=analyzed_segment_frames
        self.verification=verification
    def to_dict(self): return self.__dict__
def analyze_wav(wav_path: str | Path, expected_loop_frames: int, observed_loop_frames: int, tempo: float, numerator: int, sample_rate: int, silence_threshold: float = 0.01, peak_threshold: float = 0.9) -> AnalysisResult:
    with wave.open(str(wav_path), 'rb') as w:
        n = w.getnframes(); data = w.readframes(n)
    peak = max((abs(int.from_bytes(data[i:i+2], 'little', signed=True)) for i in range(0, len(data)-1, 2)), default=0)
    verification = 'FAIL' if peak / 32768.0 < 0.01 else ('PASS' if abs(n - expected_loop_frames) < 2 else 'FAIL')
    return AnalysisResult(expected_loop_frames, observed_loop_frames, n, n, verification)
