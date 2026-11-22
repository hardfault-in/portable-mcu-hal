# EFR32 hardware verification — results and iteration log

Running record of what was tried, what broke, and what was fixed, for
the Silicon Labs EFR32 (BRD2601B) hardware-verification pipeline. See
`CLAUDE.md` for commands/conventions and current phase status.

## Board / SDK

- Board: EFR32xG24 Dev Kit, BRD2601B, part EFR32MG24B310F1536IM48.
- Target macro: `TARGET_SILABS_SLGPIO` (current `sl_gpio` driver, not
  classic emlib) — confirmed with the maintainer.
- Simplicity SDK pin: `2026.6.1` (latest patch of the June-2026 LTS
  baseline at the time of writing) — confirmed with the maintainer.

## Entry 1 — Phase 1/2 written, not yet validated anywhere

**What was built:** `verify/targets.yml`, `docker/efr32/Dockerfile`,
`docker/efr32/build_in_container.sh`, `tests/efr32/smoke/main.c`,
`scripts/build.sh`, `.github/workflows/build.yml`.

**Why it's unvalidated:** this development sandbox's network egress
proxy refuses all `silabs.com` connections outright —

```
$ curl -sS -o /dev/null -w "%{http_code}\n" https://www.silabs.com/
curl: (56) CONNECT tunnel failed, response 403
[agent-proxy] ... docs.silabs.com:443 — connect_rejected (... host is
not allowed for this session)
```

— confirmed identically via plain `curl`, via the Docker daemon's own
build network (same proxy config), and via an independent fetch tool
(which got a DNS failure on its own separate network path). This
blocks:

1. Downloading `slt-cli` itself (`docker/efr32/Dockerfile`'s first
   `RUN curl ...` step).
2. `slt install`'ing the pinned Simplicity SDK (same Dockerfile).
3. Running `slc generate` locally against a real SDK example to build
   `tests/efr32/smoke/`'s project scaffolding — meaning that file's
   exact include paths/macro names for the board's LED pin
   (`SL_SIMPLE_LED_LED0_PORT`/`_PIN`) are a flagged placeholder guess,
   not a confirmed real symbol (see that file's own header comment).

Checked and ruled out as workarounds, rather than left unconsidered:
- `github.com/SiliconLabs/gecko_sdk` (the pre-2025.12 GitHub SDK
  mirror) is reachable, but Silicon Labs' own docs call it "reference
  only" since the 2025.12 Conan-based delivery change, and it's frozen
  at an older Gecko-SDK-branded release — using it would silently
  substitute a different SDK than the `2026.6.1` pin the maintainer
  confirmed. Not used, for that reason.
- No official or community Docker image bundling Simplicity SDK +
  slc-cli + gcc-arm-none-eabi was found.
- No non-`silabs.com`-hosted Conan/package remote was found.

**Decision (confirmed with the maintainer):** write everything now,
validate via the first real GitHub Actions run — Actions runners have
ordinary internet access this sandbox doesn't. Every genuinely
unconfirmed specific in the files above is marked `EDIT-ME` with the
exact command to run or check once real SDK/slc-cli access exists, so
the next iteration is a short, targeted fix, not a redesign.

## Entry 2 — first real CI run: SLT-CLI itself works; `slt install` flag was wrong

[Run 1](https://github.com/hardfault-in/portable-mcu-hal/actions/runs/39339865558) —
**confirmed working**: the guessed SLT-CLI download URL/version
(`slt-cli-1.2.2-linux-x64.zip`) was right — `slt --version` printed
`SLT version: 1.2.2-107` inside the container, meaning this sandbox's
silabs.com block really was the only thing stopping that step locally.

**Failed at:** `slt install -f /tmp/recipe.toml --accept-licenses --yes`

```
Error: unknown flag: --accept-licenses
...
Global Flags:
      --json                If specified - output will be in JSON format
      --non-interactive     no user interaction, false by default
      --token-file string   path to a file containing an OAuth refresh
                             token; ... Overrides $SLT_SECURE_TOKEN
      --version             print version info
```

**Fix:** dropped the guessed `--accept-licenses --yes`, use the real
global flag `--non-interactive` instead.

**Still open:** whether `--non-interactive` alone gets past any
license/download prompt, or whether `slt install` additionally needs
an OAuth token (`--token-file`/`$SLT_SECURE_TOKEN`) to actually pull
`simplicity-sdk` — unconfirmed until the next run. If it turns out a
token is required, that's an account/credential the maintainer needs
to provide — not something to work around silently.

## Entry 3 — `slt install` fully works; fixed a missing dep and a bad path guess

[Run 2](https://github.com/hardfault-in/portable-mcu-hal/actions/runs/39340042916) —
**major confirmation**: `--non-interactive` alone was enough. No
account, token, or EULA prompt at all. It actually installed
`simplicity-sdk 2026.6.1` (the exact pinned version), `slc-cli 6.0.25`,
`gcc-arm-none-eabi 14.2.rel1`, `cmake 3.30.2`, `ninja 1.12.1`.

**Failed at:** `commander/1.25.0` extraction —
`tar (child): bzip2: Cannot exec: No such file or directory`. The base
image was missing `bzip2`. **Fix:** added `bzip2 xz-utils` to the
`apt-get install` line.

**Also fixed proactively from the same log**, before pushing again:
the real install paths SLT printed are hashed Conan paths like
`/root/.silabs/slt/installs/conan/p/simpleca33d691c539/p` for
`simplicity-sdk` — nothing like the flat `${SLT_HOME}/simplicity-sdk`
this file originally guessed for the `slc configuration --sdk=...
--gcc-toolchain=...` step. Rewrote that step to run `slt install
--json`, parse the real paths out of it with a small Python script,
and write them to `/opt/slt/paths.env` for later steps to source,
instead of hardcoding a path that was already proven wrong.

**Caught and fixed before pushing, by testing locally first** (this
part needed no silabs.com access, so there was no excuse to push
blind): the Python step was originally written as `python3 -c "<multi-
line>"`, which doesn't work — a Dockerfile RUN instruction ends at the
first newline not preceded by `\`, *unless* it's inside a `<<TOKEN`
heredoc (which IS tracked across lines). Confirmed with a disposable
test Dockerfile (`unknown instruction: import`), fixed by switching to
the same heredoc form (`python3 - <<'PYEOF' ... PYEOF`) already used
for the recipe.toml step, and re-verified the fixed version builds
correctly in isolation before editing the real file.

**Also verified locally (as far as this sandbox can go):** `docker
buildx build` against the real, current Dockerfile parses with no
syntax errors and runs every step up through `apt-get install` —
failing, exactly as expected, at the one `curl` to `www.silabs.com`,
with every step after that unreached but now syntax-checked.

**Next entry will be:** the next CI run's result — expecting
`slt install` to fully succeed this time, `slc configuration` to run
against the real discovered paths, and the first real signal on
`slc generate`/the smoke test's own build (Phase 1's actual goal).

## Entry 4 — `slt install` fully succeeded; `--json` doesn't give package details

[Run 3](https://github.com/hardfault-in/portable-mcu-hal/actions/runs/39340851747) —
**`slt install` completed with every package installed**, including
`commander` (confirms the `bzip2`/`xz-utils` fix): `Installation
completed; lock file: /tmp/recipe.lock`.

**Failed at:** the NEXT step, `slc configuration` — `slc: not found`.
Root cause: `slt install -f ... --non-interactive --json` (run a
second time to get machine-readable output) printed only
`{"lock": "/tmp/recipe.lock"}` — a pointer to a lock file, not the
Id/Version/Path block the plain-text run shows. My parser found
nothing, `paths.env` had no real paths in it, so `slc`'s install
directory never reached `PATH`.

**Fix:** stopped relying on `--json` at all. `tee`'d (well — actually
redirected, see below) the *original* `slt install` call's plain-text
output to `/tmp/install.log` and switched the parser to scan that
line-by-line, pairing each `Id:` line with the `Path:` line that
follows it — the exact format confirmed real across two separate runs
now. Tested the parser against the real captured log text from Run 2
locally before pushing; it correctly extracts `simplicity-sdk`,
`gcc-arm-none-eabi`, and `slc-cli`'s real paths.

**Caught and fixed before pushing, by testing locally first (again no
silabs.com access needed for this part):** my first instinct was
`slt install ... | tee /tmp/install.log`, but that makes the RUN
step's exit code `tee`'s (always 0) instead of `slt install`'s — a
real install failure would have been silently swallowed. Switched to
redirecting to a file, capturing `$?` explicitly, catting the log
either way, then re-exiting with the real code — verified the exit
code propagates correctly with a standalone `sh -c` test before
editing the real Dockerfile.

**Still true:** this sandbox's own `docker buildx build` syntax-check
hit an unrelated Docker Hub rate limit (`429 Too Many Requests` on the
`ubuntu:24.04` pull) this round, so this iteration's confidence comes
from the isolated unit tests above, not a local full-file build. The
next CI run is still the real check.

## Entry 5 — the real root cause: `slt`'s summary only prints to a TTY

[Run 4](https://github.com/hardfault-in/portable-mcu-hal/actions/runs/39341402100) —
`slt install` succeeded completely again, but `/tmp/install.log` (the
file Entry 4's fix redirected its plain-text output to) turned out
**empty of package data** — the parser's own debug print showed
`{}`. So the "Installed Packages:" Id/Version/Path summary genuinely
seen twice before in BuildKit's captured RUN output (runs 2 and 3)
isn't ordinary stdout content — `slt` only emits that pretty summary
when it detects a TTY, and silently skips it once stdout is a pipe or
a redirected file (this build's `> /tmp/install.log`). Two guesses in
a row (`--json`, then redirecting plain text) both failed for the same
underlying reason: neither one preserves what only prints to a real
terminal.

**Fix, this time not dependent on slt's output format at all:**
dropped the install-log parsing entirely. Instead, search the
filesystem SLT actually wrote to for concrete marker files/dirs —
`bin/arm-none-eabi-gcc` for the toolchain, an `slc` executable for
slc-cli, and `platform/driver/gpio` for the Simplicity SDK root. That
last marker isn't a fresh guess: it's the exact path
`silicon_labs/gpio/hal_port_silabs_slgpio.c`'s own header comment
already cites as confirmed real against the SDK source, from this
project's original wrapper-development research — reusing a fact this
repo already established, rather than inventing a new one. Also
reverted the install step back to plain (non-redirected) `slt install`
now that nothing downstream needs to parse its output, so the readable
summary shows up in the build log again for human debugging.

Tested the new path-discovery logic locally against a mocked directory
tree matching the exact real hashed paths seen in earlier runs'
logs — `dirname()`-chain math confirmed correct via assertions — before
editing the real Dockerfile. Also updated `docker/efr32/
build_in_container.sh` to source the resulting `/opt/slt/paths.env`
and use the real `$SIMPLICITY_SDK_PATH` instead of its old static
`/opt/slt/simplicity-sdk` guess.

**Next entry will be:** whether this run's `slc configuration` step
succeeds, and if so, the first real signal on `slc generate` against
the guessed `blink_baremetal` example path.

## Entry 6 — Dockerfile fully works end to end; `--load` picked the wrong-arch image

[Run 5](https://github.com/hardfault-in/portable-mcu-hal/actions/runs/39341962056) —
**the whole Dockerfile succeeded.** Path discovery found real values
for all three markers:
```
GCC_ARM_NONE_EABI_PATH=.../gcc-a469753773d4f2/p
SLC_BIN_DIR=.../slc-cli-v6.0.25/slc_cli
SIMPLICITY_SDK_PATH=.../simpleca33d691c539/p/platform_core
```
and `slc configuration --sdk=... --gcc-toolchain=...` ran successfully
(printing a deprecation notice recommending `.slconf` files /
`--sdk-package-path` instead — noted for later, not acted on yet,
since the deprecated form still works).

Note: `SIMPLICITY_SDK_PATH` resolved one level deeper than expected —
`.../p/platform_core`, not `.../p` — because the `platform/driver/gpio`
marker lives under a `platform_core` subdirectory. Under the 2025.12
Conan-based modular package split, "the SDK" may no longer be one flat
tree with `app/` at its root the way the pre-2025.12 Gecko SDK was —
`app/common/example/...` might be a sibling package, not a subfolder
of `platform_core`. Flagging this now rather than assuming the
existing `blink_baremetal` guess in `build_in_container.sh` still
makes sense; the next `slc generate` attempt will show directly
whether it does.

**New failure, one layer out:** after the Docker image built, `docker
run`ning it hit `/usr/bin/bash: cannot execute binary file` (exit
126) — a classic wrong-architecture symptom, immediately after an
unusually long (~6 minute) `exporting to docker image format` →
`sending tarball` → `importing to docker` → `loading layer` sequence
for what should be a simple same-host `--load`. Root cause: modern
BuildKit's default provenance/SBOM attestations produce a multi-
manifest image, and `--load` can end up importing the wrong
manifest's layers. This is a known `docker buildx build --load`
gotcha, not something specific to this Dockerfile.

**Fix:** added `--provenance=false --sbom=false --platform=linux/amd64`
to `scripts/build.sh`'s `docker buildx build` call.

**Not verified locally this round** — Docker Hub rate-limited every
image pull from this sandbox (`429 Too Many Requests`, both
`ubuntu:24.04` and `alpine:latest`), unrelated to the fix itself.
Confidence here comes from this being a well-understood, widely-
documented BuildKit behavior (disabling attestations to get a clean
single-manifest `--load` result), not a fresh guess — the next CI run
is still the real confirmation.

## Entry 7 — switched from CI/Docker to a local build (Phase 1 done)

GitHub Actions and Docker were dropped (SDK/container downloads too
heavy); `.github/workflows/build.yml` and `docker/efr32/` were removed.
`scripts/build.sh <vendor> <test> <build_id>` now builds locally.

**One-time local setup (this machine):**
- `slt` 1.2.2 (Windows) in `C:\SiliconLabs\slt`; `slt install slc-cli`
  put slc-cli 6.0.25 (plus its Python/Java) under
  `~/.silabs/slt/installs/archive/`. The direct slc-cli URL on silabs.com
  returned 403, so slt is the working route. Note `slt` appears hung while
  extracting; the Java step alone took ~13 minutes.
- SDK: Simplicity SDK 2025.6.3, an unmodified clone of
  `SiliconLabs/simplicity_sdk` at tag `v2025.6.3`
  (`C:\Users\sarve\SimplicityStudio\SDKs\simplicity_sdk_2`). The pin in
  `verify/targets.yml` moved from the CI-era 2026.6.1 to 2025.6.3 by the
  maintainer's decision.
- Signature trust, run once after verifying the checkout was clean:
  `slc signature trust --sdk "C:/Users/sarve/SimplicityStudio/SDKs/simplicity_sdk_2/"`
- Toolchain: Simplicity Studio's GNU Arm 12.2.rel1, cmake 3.27 and ninja
  (no GNU make). Build backend is `slc generate -o cmake`.

**Findings:** LED0 on BRD2601B is port D pin 2 (active low), not the
port 1 pin 2 guessed earlier. The generated `toolchain.cmake` needs
`ARM_GCC_DIR`, `NINJA_EXE_PATH` and `POST_BUILD_EXE`, which build.sh sets.

**Result:** `scripts/build.sh efr32 smoke <id>` builds
`out/efr32/smoke.{elf,hex,map}`; the id is in the binary and changes with
each build; `hf_gpio_init/read/toggle` are linked in.
