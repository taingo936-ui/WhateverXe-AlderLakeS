# WhateverXe Stage 8A — Lenovo LOQ Hybrid internal eDP at 144 Hz

**Date:** 2026-11-03  
**Status:** Experimental / hardware-tested on one laptop; no stable release claim  
**Fork:** [taingo936-ui/WhateverXe-AlderLakeS](https://github.com/taingo936-ui/WhateverXe-AlderLakeS)  
**Upstream:** [b00t0x/WhateverXe](https://github.com/b00t0x/WhateverXe)

## Scope and hardware

This project documents a **machine-specific** attempt to keep the Stage 7M
WhateverXe 0.7.0 Hybrid GPU foundation while reintroducing the older, working
HX144 secondary EDID timing path.

| Component | Observed test configuration |
| --- | --- |
| Laptop | Lenovo LOQ |
| CPU | Intel Core i5-12450HX (4 P-cores + 4 E-cores) |
| iGPU | Intel UHD Graphics, PCI `8086:468B` |
| dGPU | NVIDIA RTX 4050 Laptop GPU, custom NullMoth path; not shipped here |
| Internal display | AUO B156HAN15.K, 1920×1080 eDP, native 144 Hz |
| OS | macOS Tahoe 26.7.1, SMBIOS MacBookPro16,2 |
| WEX | WhateverXe 0.7.0, Stage 7M diagnostic baseline + Stage 8A HX144 EDID branch |
| Graphics dependency | Tested alongside separately modified Graphics 0.4.0 / POSH Stage 7N.4; **not included in this repo** |

CPU spoofing, OpenCore settings, NVIDIA integration, and the graphics package
are independent, experimental parts of this laptop's setup; they are not
universal installation recommendations.

## What Stage 8A changes

The Stage 8A source delta is intentionally small:

1. Add `WhateverXe/hx144_edid.hpp` from the previously tested HX v5 144EDID
   branch, adapted without changing the historical EDID payload.
2. Include that header in `WhateverXe/kern_xe.cpp`.
3. Add an opt-in branch within `XE::wrapTGLPortDDC` gated by
   `checkKernelArgument("-wexhx144edid")`, DDC success, first EDID block,
   DDI 0, and exact panel validation.
4. Add the option to `tests/fixtures/boot_argument_names.json` so the
   `boot_arguments.py` regression test accounts for it.

Stage 8A does **not** intentionally modify lane selection, link training,
PLL programming, PWM, power wells, the GPU scheduler, PCI configuration,
NVIDIA drivers, or the POSH patch.

### EDID timing

| Parameter | Preferred ~60 Hz | Secondary ~144 Hz |
| --- | ---: | ---: |
| Pixel clock | 142,460,000 Hz | 341,900,000 Hz |
| Horizontal total | 2103 | 2103 |
| Vertical total | 1129 | 1129 |
| Derived refresh | ~60.00 Hz | 144.001 Hz |

The helper checks the base EDID against the exact captured panel identity,
validates the source checksum, leaves the original preferred DTD intact, and
replaces the unused descriptor with a secondary 144 Hz DTD, recomputing the
checksum. **This is not a generic EDID injector.**

## Validation — November 3, 2026

### Source and build

- `TEST_OFFLINE.sh`: source integrity, DDC marker, expected EDID output,
  checksum, preferred 60 Hz timing, negative/rejection cases: **PASS**.
- Eight targeted CPU tests, including `boot_arguments.py`: **PASS** after
  updating the new boot-argument fixture.
- Local macOS Release x86_64 build and packaging: **PASS**.
- User-reported Stage 8A executable SHA-256:
  `1961d608bc8c6d431d5702095936bf2b1ad1d4cf73d7f66f85dd6fc066e16314`.
- User-reported Mach-O UUID:
  `40E19745-5F54-305A-BF4F-D6AE1E3A1B16`.

**Important runtime caveat:** the current session verified the new binary in
the staged test EFI and separately verified the display timing after reboot,
but did **not** capture independent proof that the exact Stage 8A Mach-O UUID
was loaded by the running kernel. An independent runtime identity audit is
still pending; do not treat these as cryptographically tied measurements.

### Display and early workload observations

- macOS Displays reported **1920×1080 at 144 Hertz**.
- IORegistry readback reported `IOFBCurrentPixelClock = 341900000`, consistent
  with the panel's 144 Hz timing at 2103×1129 total pixels per frame.
- WindowServer RSS samples: `119804 KiB` (~117.0 MiB) and `124328 KiB`
  (~121.4 MiB), approximately 93 seconds apart.
- `vm.swapusage`: **0.00 MB used** in those samples.
- WindowServer process uptime: ~5 min 27 s and ~7 min at the two samples.
- User reported YouTube **4K with Ambient Light** during early testing and
  was still able to collect terminal diagnostics.
- Targeted `kernel` / `WindowServer` GPU Hang, GPU Restart, BCS searches:
  **no matching messages during their queried time windows**. Empty results do
  not prove that a GPU freeze cannot occur.

These observations are **not** an extended stress or leak test. In particular,
there is no direct 76-KiB POSH allocation count yet at 144 Hz; low WindowServer
RSS alone does not establish that the previous allocation issue is fully fixed.

## Known issues / not yet verified

- Earlier historical HX144 revisions occasionally booted to a lit-but-black
  internal display, with unreliable sleep/wake.
- A separate DDC Guard variant exposed 144 Hz but **hard-froze after 1–3 s**
  of UFO Test. The cause was not established, and Stage 8A should not be
  described as having fixed it without comparable tests.
- Full display sleep/wake, cold boot reproducibility, hotplug, gaming,
  extended 4K Ambient Light, UFO Test and long-duration 144 Hz stress remain
  **unverified for Stage 8A**.
- Stage 8A has only one known panel-specific test configuration; there is no
  basis to claim support across all Alder Lake-S/HX laptops or EDIDs.
- The reported source archive and companion graphics/POSH changes are not
  substitutes for upstream-signed releases.

## Reproduction / development workflow

Build in a separate working copy; never modify a known-good EFI in place.
Keep a known-bootable recovery USB, especially given historical full-system
freezes.

```bash
# From the Stage 8A source/build kit (not generic upstream checkout)
bash TEST_OFFLINE.sh
bash BUILD_ON_MAC.sh
```

The build script fetches pinned development dependencies and creates an
experimental x86_64 artifact. It does not install the kext into EFI.

On a deliberately isolated **test EFI**, the opt-in argument is:

```text
-wexhx144edid
```

The argument only advertises an additional mode; users still select 144 Hz
through macOS Displays. **Do not combine this test path with historical**
`-wex144preferred`, `-wexhx144array`, or `-wexhxpwmfix` experiments. Never
force link lanes or MMIO writes based only on EDID bandwidth calculations.

The currently working boot configuration and system-installed graphics/POSH
bundles must be backed up separately; replacing an EFI does not revert
changes to `/Library` graphics bundles.

## Attribution and license

- Base project: **b00t0x/WhateverXe**; retains its existing notices and
  BSD-3-Clause licensing. This document does not relicense upstream code.
- Stage 7M diagnostic/eDP guard work, historical HX144 panel-specific EDID
  integration, and testing: taingo936-ui fork / Lenovo LOQ experiment.
- WhateverXe depends on the broader Lilu/macOS Hackintosh ecosystem; the
  NVIDIA/NullMoth and modified graphics/POSH components are separate projects.

**All compatibility statements here are limited to the test evidence above.**
