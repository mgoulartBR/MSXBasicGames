# Toolchain (pinned)
- MSXgl **v1.5.0** (be3278424ec6a1dcd23563564c857e93aaec090d), bundled SDCC **4.6.0**
- openMSX **19.1** (distro package) with C-BIOS MSX2, run headless via `xvfb-run`; upstream is newer (21.0), not tried
- Node 22, Python 3.12 + Pillow + NumPy (`python3.12` is auto-detected by `scripts/env.sh`)
- Config: `msxgl_config.h` (Screen 5 only, 16-colour palette, `PSG_DIRECT`), `project_config.js` (ASCII-8, 256 KB, banked calls)
Build variants: `build.sh debug` (debug keys), `selftest`, `clean`, `rebuild`.
Release build: dist/balatro-msx-0.1.rom, SHA256 `edc515bba792da3735cf862026a9ad713e82d56dd703ead21b51ac4e56a9d70b` (local, not committed).
