#!/bin/bash
# Headless openMSX tests (throttled = real time, ~2.5 min). Needs a built ROM (scripts/build.sh).
#   smoke    : boot, title, briefing, play screen
#   play     : place articles, discard, pick up, End Day, popup, night, next morning
#   branches : debug-start ROMs for goal pass/fail, rebel path, both endings
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
[ -f "dist/republia-msx-$VERSION.rom" ] || ./scripts/build.sh
rm -f screenshots/*.png
./scripts/shot.sh tests/smoke.tcl > /dev/null 2>&1
./scripts/shot.sh tests/play.tcl  > /dev/null 2>&1
if [ "$1" != "quick" ]; then ./scripts/test_morning.sh; fi
./scripts/test_mouse.sh
python3 tests/check_shots.py 01_title 02_morning 03_play_feed 04_place_big 05_placed 06_two_articles \
  08_endday_selected 09_popup 20_mouse_hover 21_mouse_place 22_mouse_placed 23_mouse_repick 24_mouse_discard 10_night_p1 11_morning_day2 \
  $( [ "$1" != "quick" ] && echo m_d4_p1 m_d4f_p1 m_d6_p1 m_d8_p1 m_d11w_p1 m_d11l_p1 )
