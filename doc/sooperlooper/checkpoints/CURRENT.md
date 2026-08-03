# Current checkpoint — Phase 1 corrective gate

Checkpoint ID: `CP-015`  
Checkpoint date: 2026-08-03  
Phase: `phase-1-protocol-core`  
Active task: `M1-008`  
Status: `review`  
Branch: `gate/m1-008-phase-1`  
Draft PR: `#14`

The M1-005B, M1-006B and M1-007B correctives are published.
Local ad-hoc verification passed, but the first remote gate run exposed
project-control and focused-linking defects that must be corrected before
the Phase 1 human decision.

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-03-CP-015-correctives-merged.md`

Next executable action: repair the gate workflow and control files, run
fresh CI on the exact resulting head, then execute the pinned real-engine
smoke. PR #14 remains draft and Phase 2 is not authorized.
