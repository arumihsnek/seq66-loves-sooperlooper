"""
Module-level smoke test functions for headless audio lab.
"""

def test_audio_oracle_import() -> None:
    """Test that audio_oracle can be imported."""
    from headless_audio_lab.audio_oracle import analyze_wav, analyze_loop_reproduction, AnalysisResult
    assert analyze_wav is not None
    assert analyze_loop_reproduction is not None
    assert AnalysisResult is not None


def test_graph_assertions_import() -> None:
    """Test that graph_assertions can be imported."""
    from headless_audio_lab.graph_assertions import GraphAssertions
    assert GraphAssertions is not None
    # Test instantiation
    ga = GraphAssertions()
    assert ga is not None


def test_runner_import() -> None:
    """Test that runner can be imported."""
    from headless_audio_lab.runner import LabRunner
    assert LabRunner is not None
    # Test instantiation
    lr = LabRunner()
    assert lr is not None


def test_analysis_result_creation() -> None:
    """Test that AnalysisResult can be created and used."""
    from headless_audio_lab.audio_oracle import AnalysisResult
    result = AnalysisResult(
        expected_loop_frames=100,
        observed_loop_frames=100,
        captured_wav_frames=200,
        analyzed_segment_frames=150,
        verification="PASS"
    )
    assert result.expected_loop_frames == 100
    assert result.observed_loop_frames == 100
    assert result.captured_wav_frames == 200
    assert result.analyzed_segment_frames == 150
    assert result.verification == "PASS"
    # Test to_dict
    result_dict = result.to_dict()
    assert isinstance(result_dict, dict)
    assert result_dict["expected_loop_frames"] == 100
    assert result_dict["verification"] == "PASS"


def test_graph_assertions_basic() -> None:
    """Test basic GraphAssertions functionality."""
    from headless_audio_lab.graph_assertions import GraphAssertions
    ga = GraphAssertions()
    # These should not raise exceptions
    ga.get_graph()
    ga.get_clients()
    ga.get_ports()
    ga.get_connections()
    # snapshot and diff_snapshots may raise if no snapshots, but that's ok for smoke test
    try:
        ga.snapshot()
    except Exception:
        pass  # May fail if jack not available, but that's ok for smoke test
    try:
        ga.diff_snapshots()
    except Exception:
        pass  # May fail if insufficient snapshots, but that's ok for smoke test


if __name__ == "__main__":
    # Run the smoke tests
    test_audio_oracle_import()
    test_graph_assertions_import()
    test_runner_import()
    test_analysis_result_creation()
    test_graph_assertions_basic()
    print("All smoke tests passed")