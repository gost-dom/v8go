# Backlog

Deferred work items. Nothing here is done yet — captured so we don't lose it
while focusing on functionality and runtime performance (req/s).

## Module rename: `github.com/tommie/v8go` → `github.com/iquirino/v8go`

Do NOT do this yet. When we do, update every occurrence of the module path:

- `go.mod` — the `require` directives for `github.com/tommie/v8go/deps/*`.
- `deps/*_*/go.mod` — the `module github.com/tommie/v8go/deps/<os>_<arch>` lines
  (these are generated, see below).
- `cgo_*_*.go` — generated `import _ "github.com/tommie/v8go/deps/<os>_<arch>"`.
- `deps/update_cgo.py` — the `--root-module` default (`github.com/tommie/v8go`);
  regenerating with the new value rewrites the go.mod/cgo files above.
- `README.md` / docs references.

Open question: the root module is currently `github.com/gost-dom/v8go` while the
deps modules live under `github.com/tommie/v8go`. Decide whether the root module
also moves to `github.com/iquirino/v8go` for consistency, or only the deps paths
change.

## Security / sandbox (revisit after functionality + perf baseline)

Currently built with `v8_enable_sandbox=false`. Enabling it is coupled to
performance and to the build toolchain:

- Requires the hardened custom libc++ (`use_custom_libcxx=true` + safe libcxx),
  i.e. switching the Linux build from GCC + system libstdc++ to clang + bundled
  libc++. Cascades to the other platforms and needs `-DV8_ENABLE_SANDBOX` back
  in `cgo.go`.
- Upside: on x64/arm64 the sandbox config also unlocks builtins PGO
  (`v8_enable_builtins_optimization`), which is otherwise off in this standalone
  build. PGO can speed up builtins-heavy workloads.
- Downside / risk: the sandbox reserves a large virtual-address region per
  isolate and adds external-pointer-table indirection — may raise memory and CPU
  and affect execution time. MUST be benchmarked (req/s, RSS, CPU) before
  committing to it; do not assume it's a net win.

## Builtins PGO (standalone build)

`v8_enable_builtins_optimization` defaults to false for standalone (non-Chromium)
builds, so we ship without builtins PGO today. The profiles are already
downloaded by the `checkout_v8_builtins_pgo_profiles` gclient hook. On x64/arm64
applying them requires `v8_enable_sandbox=true` (see above), so PGO is coupled to
the sandbox decision. Benchmark alongside it.

## Remaining platform binaries

Only `linux/amd64` static libraries were rebuilt locally for the V8 15.x upgrade.
Before release, rebuild `darwin` (amd64/arm64), `android` (amd64/arm64), and
`linux/arm64` via the `V8 Build` workflow — the shared headers/bindings now
target the 15.x API and will not link against the old 11.x libraries.

## API surface / feature work (post-upgrade)

Not exposed by the current binding; candidates once the upgrade is fully green:

- Typed arrays / ArrayBuffer / DataView creation and element access.
- `ValueSerializer` / `ValueDeserializer` (structured clone).
- Map / Set entry access; Date creation + value extraction.
- Dynamic `import()` + `import.meta` host callbacks.
- Explicit `TryCatch` wrapper (stack traces, rethrow, HasTerminated).
- Startup snapshots (`SnapshotCreator`) for faster context creation.
- `MeasureMemory`, external-memory accounting, `RequestInterrupt`.
