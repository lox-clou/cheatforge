#!/bin/bash
set -e
U=https://raw.githubusercontent.com/a2x/cs2-dumper/main/output
dl(){ curl -fsSL "$U/$1" -o "$2" 2>/dev/null || curl -fsSL "${U/main/master}/$1" -o "$2" 2>/dev/null || : > "$2"; }
dl offsets.hpp o.hpp
dl client.dll.hpp c.hpp
dl buttons.hpp b.hpp
g(){ local v=$(grep -hm1 "$2" "$1" 2>/dev/null | sed -E 's/.*= (0x[0-9a-fA-F]+).*/\1/');
     [ -z "$v" ] && v=$(grep -hm1 "$2" o.hpp b.hpp c.hpp 2>/dev/null | head -1 | sed -E 's/.*= (0x[0-9a-fA-F]+).*/\1/');
     echo "${v:-0}"; }
{
echo "#pragma once"
echo "#define OFF_ENTLIST   $(g o.hpp dwEntityList)"
echo "#define OFF_PAWN      $(g o.hpp dwLocalPlayerPawn)"
echo "#define OFF_VIEWMAT   $(g o.hpp dwViewMatrix)"
echo "#define OFF_FORCEJUMP $(g b.hpp dwForceJump)"
echo "#define OFF_HP      $(g c.hpp m_iHealth)"
echo "#define OFF_TEAM    $(g c.hpp m_iTeamNum)"
echo "#define OFF_ORIGIN  $(g c.hpp m_vOldOrigin)"
echo "#define OFF_FLAGS   $(g c.hpp m_fFlags)"
echo "#define OFF_PUNCH   $(g c.hpp m_AimPunchAngle)"
echo "#define OFF_SHOTS   $(g c.hpp m_iShotsFired)"
echo "#define OFF_CROSS   $(g c.hpp m_iIDEntIndex)"
echo "#define OFF_FLASH   $(g c.hpp m_flFlashMaxAlpha)"
echo "#define OFF_ANG     $(g c.hpp m_angEyeAngles)"
} > offsets_gen.h
echo "--- offsets_gen.h ---"; cat offsets_gen.h; echo "---------------------"
mkdir -p ../builds
build(){ local g=$1; shift
  x86_64-w64-mingw32-g++ -O2 -s -static -o ../builds/CheatForge_$g.exe main.cpp -DGAME_${g^^} $@ -luser32 -lgdi32 && echo "ok $g" || echo "FAIL $g"; }
build cs2      -DFEAT_MEMESP -DFEAT_MEMAIM -DFEAT_TRIGMEM -DFEAT_BHOPMEM -DFEAT_RCSMEM -DFEAT_RADARMEM -DFEAT_NOFLASH
build valorant -DFEAT_COLESP -DFEAT_TRIGCOL -DFEAT_RECOIL
build rust     -DFEAT_RECOIL -DFEAT_RAPID -DFEAT_BHOPIN
build apex     -DFEAT_COLESP -DFEAT_AIMCOL -DFEAT_TRIGCOL
build pubg     -DFEAT_COLESP -DFEAT_TRIGCOL -DFEAT_RECOIL
build fortnite -DFEAT_RECOIL -DFEAT_RAPID -DFEAT_BHOPIN
build gta5     -DFEAT_RAPID -DFEAT_RECOIL
build minecraft -DFEAT_COLESP -DFEAT_TRIGCOL -DFEAT_RAPID -DFEAT_BHOPIN
build tf2      -DFEAT_COLESP -DFEAT_TRIGCOL -DFEAT_BHOPIN
ls -lh ../builds/
