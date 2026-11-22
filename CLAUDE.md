# Hardware verification pipeline — working notes

This file is for whoever (human or Claude Code session) picks up the
hardware-verification pipeline next. It does NOT describe the wrapper
itself — see `README.md`/`docs/architecture.md` for that. Nothing here
authorizes editing `stm/`, `nrf/`, `silicon_labs/`, `ti/`, or
`include/` — a test finding a wrapper bug gets reported, never fixed,
without the maintainer's explicit go-ahead.

## Status (as of this writing)

**Phase 1/2 (build-only, no hardware) are written but not yet
validated by a real build anywhere.** This development sandbox's
network egress proxy refuses all `silabs.com` connections (confirmed:
`curl` gets `403 connect_rejected ... host is not allowed for this
session`; same result through Docker's build network and through an
independent fetch tool). That blocks `slt-cli`/the Simplicity SDK
download, which is the one thing `docker/efr32/Dockerfile` needs most.
GitHub Actions runners have ordinary internet access and are expected
to reach `silabs.com` fine — so the actual first real signal on
whether this Dockerfile/build script work comes from a pushed commit's
Actions run, not from this sandbox. See `docs/verification/efr32.md`
for the running log of what broke and what was fixed, each time.

**Phase 3 (flash + capture through HF·NODE) has not started.** It
needs, from the maintainer, none of which exist yet in this repo or
session:
- `HFNODE_API_URL` / `HFNODE_API_KEY` environment variables.
- A description of the HF·NODE API itself (flash/logs/status
  endpoints, auth header shape, request/response bodies) — the hard
  rule from the original brief is "never guess HF·NODE endpoints,"
  so `scripts/hfnode.sh`/`fetch_build.sh`/`verify.sh` are not written
  until this is supplied.
- The HF·NODE target id for the BRD2601B board being used.

**Phase 4 (I2C baseline vs. wrapper)** additionally needs the exact
I2C sensor + 7-bit address on the board — not yet supplied.

## Hard rules (apply to every future session working on this pipeline)

- Never modify `stm/`, `nrf/`, `silicon_labs/`, `ti/`, `include/`
  (the wrapper). A test exposing a wrapper bug is reported with
  evidence, not patched — the maintainer decides whether/how.
- Never edit files inside a vendor SDK or example repo. Copy what's
  needed into `tests/<vendor>/<test>/`, and keep the original vendor
  license header on anything genuinely copied from a vendor example.
  Do NOT add a vendor license header to hand-written automation code
  that wasn't actually copied from the vendor (a real mistake made and
  fixed once already in this pipeline's own `tests/efr32/smoke/
  main.c` — see its header comment).
- Never commit a vendor SDK, a binary, or a secret. `HFNODE_API_KEY`
  only ever comes from the environment variable — never printed,
  logged, or echoed by any script here.
- The GitHub Actions workflow (`.github/workflows/build.yml`) builds
  firmware only. No secrets referenced anywhere in it, no flashing
  step, ever — flashing/verification is a local script run by a human
  or Claude Code session with HF·NODE credentials, never CI.
- Never change a test's expectation or a golden log to make a test
  pass. A mismatch between captured output and golden is a finding to
  report, not something to edit away.
- Test firmware logs over the board's own native serial/iostream path
  — never through `portable-mcu-hal`'s own UART adapter — so a
  peripheral bug in the thing being tested can't masquerade as (or
  hide behind) a logging bug in that same wrapper.
- An SDK version pin (`verify/targets.yml`'s `sdk_version`) changes
  only when a human deliberately decides to bump it — never silently.

## Environment / commands

- `HFNODE_API_URL`, `HFNODE_API_KEY` — not yet set in any session that
  has worked on this; Phase 3 needs them.
- `verify/targets.yml` — the one file that describes every vendor:
  board, SDK version, Docker image, target macro, test list. Adding a
  vendor should only ever mean: a new `docker/<vendor>/Dockerfile`, new
  `tests/<vendor>/<test>/` folders, and a new entry here — no script
  changes.
- `scripts/build.sh <vendor> <test> <short_sha>` — builds one test's
  firmware inside its pinned Docker image, writes
  `out/<vendor>/<test>.{hex,elf,map}`. Runs identically locally and in
  CI (CI sets `DOCKER_BUILD_EXTRA_ARGS` for layer caching; everything
  else is the same command).
- `.github/workflows/build.yml` — matrix over `verify/targets.yml`,
  runs `scripts/build.sh` per vendor × test, uploads the artifact. No
  secrets, no flashing, runs on `push` and `pull_request`.
- Phase 3+ commands (`scripts/fetch_build.sh`, `scripts/hfnode.sh`,
  `scripts/compare.sh`, `scripts/verify.sh <vendor> <test>`) don't
  exist yet — see Status above.

## Adding a new peripheral test (once Phase 3/4 exist)

1. `tests/<vendor>/<test_name>/` — hand-written `main.c` (or adapted
   from a real vendor example, with its license header kept), the
   `[BOOT]`/structured-op/`[DONE]` log-line conventions from the
   original brief, calling the wrapper API being verified.
2. Add the test to its vendor's `tests:` list in `verify/targets.yml`.
3. `scripts/build.sh <vendor> <test_name> <sha>` to confirm it builds.
4. Once it builds in CI, run `scripts/verify.sh <vendor> <test_name>`
   on real hardware, show the maintainer the captured log, and only
   after their explicit approval save it as
   `tests/<vendor>/golden/<test_name>.log`.

## Adding a new vendor (STM32 / nRF52 / TI MSPM0)

1. `docker/<vendor>/Dockerfile` — pinned vendor SDK + toolchain,
   mirroring `docker/efr32/Dockerfile`'s structure.
2. A new entry under `targets:` in `verify/targets.yml`.
3. `tests/<vendor>/smoke/` following the same conventions as
   `tests/efr32/smoke/`.
4. No changes to `scripts/build.sh`/the workflow should be needed —
   if one turns out to be, that's a sign something was hardcoded to
   `efr32` that shouldn't have been; fix the script, not the pattern.



# Kickoff: Local build → HF·NODE verify → commit only on PASS (Silicon Labs first)

## Fill these in before pasting (delete this section after)
- Board / part number: <e.g. EFR32xG24 Dev Kit, BRD2601B>
- I2C sensor + 7-bit address: <e.g. on-board Si7021 @ 0x40>
- Local Simplicity SDK path + tag: <e.g. C:/sdk/simplicity_sdk @ v2025.x.x>
- slc-cli path, GNU Arm toolchain path: <...>
- HF·NODE API docs: <path to docs / OpenAPI spec / backend repo>
- HF·NODE target ID for the EFR32 board: <id>
- Env vars already set: HFNODE_API_URL, HFNODE_API_KEY

---

## Change of plan
We are dropping the GitHub Actions build (the SDK/container downloads were too heavy). New flow, all run by you locally:

```
edit → build.sh (local SDK) → verify.sh (flash + stimulus + logs via HF·NODE API)
     → PASS: ship.sh commits exactly the tested code and pushes
     → FAIL / INVALID: nothing is committed; report or fix, then rerun
```

First, list what the previous CI attempt left in the repo (workflows, Dockerfiles, scripts) and propose what to delete or reuse. Ask me before deleting anything.

Work in phases. **Stop after each phase, show me the evidence, wait for my OK.** Start in plan mode.

## Hard rules
- **Nothing is committed or pushed unless `verify.sh` returned PASS for that exact code.** Commit and push only through `scripts/ship.sh`. Never use `git commit` directly for code changes, never use `--no-verify`, never force-push.
- Only change files in `src/` (the wrapper) when I've asked for that change. If a test exposes a wrapper bug while you're building the pipeline, stop and report it with evidence.
- Never edit the vendor SDK. Copy examples into `tests/<vendor>/` with original license headers. Never commit SDKs, build outputs, or secrets.
- `HFNODE_API_KEY` comes from the environment only; never print, log, or commit it.
- Never change an expected log or stimulus file to make a test pass. A mismatch is a finding.
- Test logging uses the vendor's native serial/iostream code, not this library's serial adapter.
- If anything about the HF·NODE API is unclear, ask me. Do not guess endpoints.

## Layout
```
verify/targets.yml          # per vendor: SDK path/tag, toolchain, HF·NODE target id, tests
tests/<vendor>/<test>/      # test firmware
tests/<vendor>/<test>/expected.log   # what the device must print
tests/<vendor>/<test>/stimulus.yml   # what HF·NODE sends/actuates during the test, and when (optional)
scripts/build.sh            # build.sh <vendor> <test> <build_id> → out/<vendor>/<test>.{hex,elf,map}
scripts/hfnode.sh           # flash | send | logs | status — wraps HF·NODE API, non-zero on error
scripts/compare.sh          # log vs expected.log, prints differing lines
scripts/verify.sh           # build → flash → stimulus → capture → BUILD_ID check → compare
scripts/ship.sh             # gate: verify the staged code, then commit + push only on PASS
.githooks/pre-push          # refuses to push if HEAD has no PASS record
verify/results/             # committed PASS reports (evidence)
CLAUDE.md
```

## Proving the tested code is the committed code
The code is tested before it's committed, so there's no commit SHA yet. Use the git tree hash instead:

1. `ship.sh` stages the changes (`git add -A` within the repo, respecting .gitignore) and takes `BUILD_ID = git write-tree` (short form). This hash identifies exactly the staged content.
2. Firmware is built with `BUILD_ID` as a define and prints it first after boot:
   `[BOOT] build=<BUILD_ID> vendor=<vendor> test=<test>`
3. `verify.sh` reads that line from the HF·NODE logs. Missing or different → **INVALID** (wrong or stale firmware on the board). Stop.
4. On PASS, `ship.sh` writes `verify/results/<BUILD_ID>.md` (vendor, test, HF·NODE target, timestamp, the captured log, stimulus sent, comparison result), stages it, commits, then checks that the committed code tree matches the tested tree (excluding the results file). Commit message trailer:
   `Verified-on: <vendor>/<test> build=<BUILD_ID> hfnode=<target_id> result=PASS`
5. If anything changes in the working tree between verify and commit, abort and start over.
6. `.githooks/pre-push` (enable with `git config core.hooksPath .githooks`) blocks the push if the HEAD commit has no `Verified-on` trailer with `result=PASS`.

For changes that touch no firmware (docs, scripts only), `ship.sh --no-firmware` may commit without a hardware run, but must refuse if any file under `src/` or `tests/` changed.

## What "PASS" means
All of these must hold:
- BOOT line present and `BUILD_ID` matches.
- Every stimulus in `stimulus.yml` was sent successfully by HF·NODE (API confirmed), at the specified points.
- Device output matches `expected.log` (`[I2C]`, `[GPIO]`, `[APP]` lines; timestamps ignored; stated numeric tolerance for sensor values).
- `[DONE] test=<test>` received within the timeout; no unexpected resets or hangs.

## Phase 1 — Local build
- `build.sh` using the local SDK and slc-cli from `verify/targets.yml` (`slc generate` Makefile export + `make`). Handle `slc` SDK configuration and signature trust once, documented.
- Smoke test `tests/efr32/smoke/`: BOOT line, toggle LED via the wrapper's GPIO API with log lines, DONE.
- Done when: smoke test builds locally and the BUILD_ID define is in the binary.

## Phase 2 — HF·NODE flash, stimulus, logs, verify
- `hfnode.sh` and `verify.sh` as above. Show me the captured smoke log; after I approve it, it becomes `expected.log`.
- Done when: `verify.sh efr32 smoke` gives PASS on the board, and deliberately flashing an old build gives INVALID.

## Phase 3 — The commit gate
- `ship.sh`, results file, commit trailer, pre-push hook.
- Prove it: (a) a passing change gets committed and pushed with its results file; (b) a deliberately failing change is NOT committed; (c) a manual push of an unverified commit is blocked by the hook. Show me all three.

## Phase 4 — I2C test
- `tests/efr32/i2c_baseline/`: the Silicon Labs I2C example, unmodified except for structured log lines and BOOT/DONE. Verify it; after I approve the log it becomes the reference for the wrapper test.
- `tests/efr32/i2c_wrapper/`: same app with only the vendor I2C calls replaced by the wrapper API. Show me the diff.
- Failure cases, expected lines written BEFORE running: no-device address → NACK without hanging; repeated-start write-read; max-length burst read; bus timeout (use HF·NODE stimulus if it can drive this; otherwise tell me when to disconnect the sensor).
- Report as a table: case / expected / actual / PASS-FAIL. Ship only what passes; report what doesn't.

## Phase 5 — Make it repeatable
- `CLAUDE.md`: SDK paths and tags, env vars, the exact commands, these hard rules, the definition of PASS, and how to add a new vendor and a new peripheral test.
- Done when: a fresh Claude Code session can run edit → verify → ship from `CLAUDE.md` alone.