# Toolchain (pinned)

```text
MSXgl:
  version: v1.5.0 (git tag)
  commit:  be3278424ec6a1dcd23563564c857e93aaec090d   (2026-07-08)
  source:  https://github.com/aoineko-fr/MSXgl  (cloned by scripts/setup.sh, checked against the commit)

SDCC:
  version: 4.6.0 #16555 (Linux)  -- the copy bundled in MSXgl tools/sdcc (no separate install)

openMSX:
  version: 19.1 (Ubuntu 24.04 package 19.1+dfsg-1ubuntu3)
  BIOS:    C-BIOS 0.29a (Ubuntu package "cbios"), machine C-BIOS_MSX1_EU (50 Hz) / C-BIOS_MSX1 (60 Hz)

Host build tools:
  OS:           Ubuntu 24.04.4 LTS
  architecture: x86_64
  Node.js:      v22.22.0   (MSXgl build scripts)
  Python:       3.11 + Pillow + NumPy (asset conversion), ffmpeg (music transcription)

Build date: 2026-10-04
```

Notes

* `MSXgl v1.5.0` was the newest release tag on 2026-10-04 (HEAD of `main` was newer; a release
  was preferred over an unreleased commit as required by the porting brief). The bundled SDCC
  4.6.0 is exactly what that release is tested with, so no other compiler is used.
* No patches were applied to MSXgl or SDCC.
* `msxgl_config.h` is the stock MSXgl template with only the module switches changed
  (MSX1 / Graphic 2 / no sub-ROM / direct PSG access); see the diff against
  `MSXgl/projects/template/msxgl_config.h`.
* Versions are pinned in `scripts/env.sh`; do not bump them silently.
