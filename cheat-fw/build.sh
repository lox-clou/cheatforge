#!/bin/bash
set -e
U=https://raw.githubusercontent.com/a2x/cs2-dumper/main/output
grab(){ local v=$(curl -fsS "$1" | grep -m1 "$2 " | sed -E 's/.*= (0x[0-9a-fA-F]+).*/\1/'); echo "${v:-0}"; }
{
echo "#pragma once"
echo "#define OFF_ENTLIST   $(grab $U/offsets.hpp dwEntityList)"
echo "#define OFF_PAWN      $(grab $U/offsets.hpp dwLocalPlayerPawn)"
echo "#define OFF_VIEWMAT   $(grab $U/offsets.hpp dwViewMatrix)"
echo "#define OFF_FORCEJUMP $(grab $U/buttons.hpp dwForceJump)"
echo "#define OFF_HP      $(grab $U/client.dll.hpp m_iHealth)"
echo "#define OFF_TEAM    $(grab $U/client.dll.hpp m_iTeamNum)"
echo "#define OFF_ORIGIN  $(grab $U/client.dll.hpp m_vOldOrigin)"
echo "#define OFF_DORMANT $(grab $U/client.dll.hpp m_bDormant)"
echo "#define OFF_FLAGS   $(grab $U/client.dll.hpp m_fFlags)"
echo "#define OFF_PUNCH   $(grab $U/client.dll.hpp m_AimPunchAngle)"
echo "#define OFF_SHOTS   $(grab $U/client.dll.hpp m_iShotsFired)"
echo "#define OFF_CROSS   $(grab $U/client.dll.hpp m_iIDEntIndex)"
echo "#define OFF_FLASH   $(grab $U/client.dll.hpp m_flFlashMaxAlpha)"
echo "#define OFF_ANG     $(grab $U/client.dll.hpp m_angEyeAngles)"
} > offsets_gen.h
cat offsets_gen.h
x86_64-w64-mingw32-g++ -O2 -s -static -o CheatForge.exe main.cpp -luser32 -lgdi32 -lshell32
mkdir -p ../builds
for g in cs2 tf2 valorant rust apex pubg fortnite gta5 minecraft; do cp CheatForge.exe ../builds/CheatForge_$g.exe; done
ls -lh ../builds/
