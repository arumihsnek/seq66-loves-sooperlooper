# Summary of SooperLooper Interfaces

## 1. Engine Monitor API (`sooperlooper_engine_monitor.hpp`)
- **Purpose**: Tracks engine lifecycle (ping, readiness, health) via a state machine.
- **States**: `disabled` → `starting` → `reconciling` → `ready` ↔ `stale` → `engine_offline` → `restarting`
- **Key Methods**:
  - `ping_sent()`, `ping_reply(version, loop_count)`, `e recorded ping reply
 
 0_state()`: current engine_version()`: get_version: `int`ng_count_last ping
-() `:bool`: Returns if engine is ready (`state == ready`)
-()`:Bool returns if engine is unreachable (`state` == `stale` or `engine_offline`)
-():`Void reset()`: Resets monitor to `disabled` state
- **Thread Safety**: Assumes single-threaded coordinator access (no internal locking)

## 2. Client API Relevant to Launch/Discovery (`sooperlooper_client.hpp`)
- **Purpose**: Non-realtime OSC adapter for headless SooperLooper engine; handles discovery, commands, subscriptions.
- **Key Methods**:
  - `ping(std::string& version, int& loop_count, int timeout_ms=1000)`: Sends ping, fills version/loop_count on success, returns `bool`.
  - `set_receiver(sooperlooper_receiver* receiver)`: Binds a receiver for subscription callbacks (caller manages lifetime).
  - `set_generation(uint64_t)` / `generation()`: Manages engine generation token; subscriptions tied to generation; invalidated on restart.
  - `cancel_subscriptions()`: Sends unregister messages for all active subscriptions (called on shutdown/restart).
  - `subscribe_loop*()`, `subscribe_global*()`, `unsubscribe_*()`: Full OSC subscription API with return_url/path.
  - `callback_url()`: Constructs return URL from bound receiver's port.
  - `endpoint()`, `last_error()`, `ready()`: Standard client configuration/status.

## 3. Generation Tracking in Observed State Cache (`sooperlooper_observed_state.hpp`)
- **Purpose**: Thread-safe cache separating desired vs. observed SooperLooper state; tracks freshness and presence.
- **Generation Handling**:
  - `set_generation(uint64_t new_generation)`: Equivalent to `clear()` + setting generation token; invalidates all prior loop indexes.
  - `generation()`: Returns current generation token.
  - `apply_event(const receiver_event& event)`: Rejects events with mismatched generation (returns `false`).
  - `clear()`: Resets all observed fields to absent/stale state (used on generation change).
- **State Tracking**:
  - Each observed field stores: value, presence flag, freshness timestamp.
  - High-frequency fields (meters/positions) use latest-value coalescing; state/error events are never coalesced.
  - `snapshot()`: Returns immutable copy of current observed state (safe for lock-free reads).
  - `dirty()`: True if any field updated since construction/last clear.
  - `is_present(loop_index, control)`: Checks if a specific loop field exists and has been set.

## 4. Backend Probe Result Structure (`sooperlooper_backend_probe.hpp`)
- **Purpose**: Side-effect-free probe to classify audio backend capability (JACK/PipeWire) without launching processes.
- **`backend_capability` Enum**:
  - `unknown = -1`
  - `usable_native_jack = 0`
  - `usable_pipewire_jack = 1`
  - `backend_unavailable = 2`
  - `probe_error = 3`
- **`backend_probe_result` Struct**:
  - `capability`: Backend capability enum (default `unknown`).
  - `client_library`: `"jack"` or `"pipewire-jack"`.
  - `server_info`: Server name/version if available.
  - `implementation`: `"native"`, `"pipewire"`, or `"unknown"`.
  - `error`: Non-empty string on `probe_error`.
  - `server_reachable`: `true` if JACK server detected.
- **Helper Methods**:
  - `is_usable()`: True for `usable_native_jack` or `usable_pipewire_jack`.
  - `is_unavailable()`: True for `backend_unavailable`.
  - `has_error()`: True for `probe_error`.
- **Usage**: 
  - `probe_backend_capability(backend_detect_fn detect = nullptr)`: Runs probe; uses default system detection if `detect` is null (checks JACK dlopen, server reachability via `jack_client_open(NoStart)`, PipeWire JACK impl).

## Interconnections for M2-004 (Engine Launch & OSC Reconciliation)
1. **Backend Probe** (`sooperlooper_backend_probe`) runs first to check audio backend availability.
2. **Engine Monitor** (`sooperlooper_engine_monitor`) manages engine lifecycle:
   - On `starting`: Sends ping via `sooperlooper_client::ping()`.
   - On ping reply: Records version/loop count, advances to `reconciling`.
   - Periodically calls `evaluate()` to check timeouts and transition states (e.g., to `ready` after successful ping and subscription setup).
3. **Client** (`sooperlooper_client`):
   - Used by monitor to send ping (discovery).
   - After engine is ready, client sets up subscriptions (via `subscribe_*()`) tied to current generation.
   - Binds a `sooperlooper_receiver` to receive OSC feedback.
4. **Observed State Cache** (`sooperlooper_observed_state`):
   - Receiver events are fed via `apply_event()`.
   - On engine restart/generation change: Monitor triggers client to `cancel_subscriptions()`, then cache `set_generation(new_gen)` to clear old state.
   - Cache provides snapshots for reconciliation (comparing desired vs. observed state).
5. **Generation Coordination**:
   - Client's `set_generation()` and cache's `set_generation()` must be kept in sync (typically on engine restart).
   - Mismatched generations cause `apply_event()` to reject events, preventing stale state from interfering.