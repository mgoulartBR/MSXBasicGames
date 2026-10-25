#!/usr/bin/env bash
# Roda o teste instrumentado no emulador e traz os screenshots para ./shots
set -euo pipefail
cd "$(dirname "$0")/.."
adb shell settings put global window_animation_scale 0 || true
adb shell settings put global transition_animation_scale 0 || true
adb shell settings put global animator_duration_scale 0 || true
status=0
./gradlew --no-daemon connectedDebugAndroidTest || status=$?
rm -rf shots && mkdir -p shots
adb pull /sdcard/Download/sombra/. shots/ || true
ls -la shots
exit $status
