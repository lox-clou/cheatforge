#!/bin/bash
set -e
# --- windows offsets ---
U=https://raw.githubusercontent.com/a2x/cs2-dumper/main/output
dl(){ curl -fsSL "$U/$1" -o "$2" 2>/dev/null || curl -fsSL "${U/main/master}/$1" -o "$2" 2>/dev/null || : > "$2"; }
dl offsets.hpp o.hpp; dl client.dll.hpp c.hpp; dl buttons.hpp b.hpp
g(){ local v=$(grep -hm1 "$2" "$1" 2>/dev/null | sed -E 's/.*= (0x[0-9a-fA-F]+).*/\1/');
     [ -z "$v" ] && v=$(grep -hm1 "$2" o.hpp b.hpp c.hpp 2>/dev/null | head -1 | sed -E 's/.*= (0x[0-9a-fA-F]+).*/\1/');
     echo "${v:-0}"; }
{ echo "#pragma once"
echo "#define OFF_ENTLIST   $(g o.hpp dwEntityList)"; echo "#define OFF_PAWN      $(g o.hpp dwLocalPlayerPawn)"
echo "#define OFF_VIEWMAT   $(g o.hpp dwViewMatrix)"; echo "#define OFF_FORCEJUMP $(g b.hpp dwForceJump)"
echo "#define OFF_HP      $(g c.hpp m_iHealth)"; echo "#define OFF_TEAM    $(g c.hpp m_iTeamNum)"
echo "#define OFF_ORIGIN  $(g c.hpp m_vOldOrigin)"; echo "#define OFF_FLAGS   $(g c.hpp m_fFlags)"
echo "#define OFF_PUNCH   $(g c.hpp m_AimPunchAngle)"; echo "#define OFF_SHOTS   $(g c.hpp m_iShotsFired)"
echo "#define OFF_CROSS   $(g c.hpp m_iIDEntIndex)"; echo "#define OFF_FLASH   $(g c.hpp m_flFlashMaxAlpha)"
echo "#define OFF_ANG     $(g c.hpp m_angEyeAngles)"; } > offsets_gen.h
mkdir -p ../builds
D10="-DFEAT_D_FOVC -DFEAT_D_CROSS -DFEAT_D_HITM -DFEAT_D_WATER -DFEAT_D_GRID -DFEAT_D_PULSE -DFEAT_D_CORNER -DFEAT_D_TRAC -DFEAT_D_TIMER -DFEAT_D_FPS"
C8="-DFEAT_C_ESP -DFEAT_C_AIM -DFEAT_C_HEAD -DFEAT_C_SMOOTH -DFEAT_C_FOV -DFEAT_C_TRIG -DFEAT_C_TDLY -DHAS_COLOR"
build(){ local g=$1; shift; x86_64-w64-mingw32-g++ -O2 -s -static -o ../builds/CheatForge_$g.exe main.cpp -DGAME_${g^^} $@ -luser32 -lgdi32 && echo "ok win/$g" || echo "FAIL win/$g"; }
build cs2 -DFEAT_M_ESP -DFEAT_M_HP -DFEAT_M_DIST -DFEAT_M_RADAR -DFEAT_M_NOFLASH -DFEAT_M_AIM -DFEAT_M_HEAD -DFEAT_M_SMOOTH -DFEAT_M_FOV -DFEAT_M_TRIG -DFEAT_M_TDLY -DFEAT_M_BHOP -DFEAT_M_STRAFE -DFEAT_M_RCS -DFEAT_M_RCSH -DFEAT_M_ASTOP $D10 -DFEAT_I_RAPID -DFEAT_I_CROUCH -DFEAT_I_BURST -DFEAT_I_JUMP
build valorant $C8 -DFEAT_C_RADAR $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_RAPID -DFEAT_I_RSLOW -DFEAT_I_BURST -DFEAT_I_CROUCH -DFEAT_I_JUMP
build rust -DHAS_COLOR -DFEAT_C_ORE $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_RAK -DFEAT_I_RM4 -DFEAT_I_RBOLT -DFEAT_I_RAPID -DFEAT_I_RSLOW -DFEAT_I_BURST -DFEAT_I_BHOP -DFEAT_I_STRAFE -DFEAT_I_TAP -DFEAT_I_CROUCH -DFEAT_I_LEAN -DFEAT_I_PRONE -DFEAT_I_JUMP -DFEAT_I_WALK
build apex $C8 -DFEAT_C_RADAR $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_TAP -DFEAT_I_BHOP -DFEAT_I_STRAFE -DFEAT_I_RAPID -DFEAT_I_BURST
build pubg $C8 -DFEAT_C_RADAR $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_RAK -DFEAT_I_RM4 -DFEAT_I_RAPID -DFEAT_I_RSLOW -DFEAT_I_CROUCH -DFEAT_I_PRONE -DFEAT_I_LEAN
build fortnite $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_RAK -DFEAT_I_RM4 -DFEAT_I_RAPID -DFEAT_I_RSLOW -DFEAT_I_BURST -DFEAT_I_BHOP -DFEAT_I_STRAFE -DFEAT_I_TAP -DFEAT_I_CROUCH -DFEAT_I_JUMP -DFEAT_I_WALK -DFEAT_I_MELEE -DFEAT_I_AIMKEY
build gta5 $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_RAPID -DFEAT_I_RSLOW -DFEAT_I_BURST -DFEAT_I_JUMP -DFEAT_I_CROUCH -DFEAT_I_WALK -DFEAT_I_MELEE -DFEAT_I_AIMKEY -DFEAT_I_RELOAD -DFEAT_I_COVER -DFEAT_I_WEP -DFEAT_I_ENTER -DFEAT_I_STRAFE
build minecraft -DHAS_COLOR -DFEAT_C_ESP -DFEAT_C_AIM -DFEAT_C_HEAD -DFEAT_C_SMOOTH -DFEAT_C_FOV -DFEAT_C_TRIG -DFEAT_C_TDLY -DFEAT_C_RADAR -DFEAT_C_ORE $D10 -DFEAT_I_RAPID -DFEAT_I_BURST -DFEAT_I_BHOP -DFEAT_I_JUMP -DFEAT_I_CROUCH -DFEAT_I_STRAFE
build tf2 $C8 -DFEAT_C_RADAR $D10 -DFEAT_I_REC -DFEAT_I_RECH -DFEAT_I_BHOP -DFEAT_I_STRAFE -DFEAT_I_TAP -DFEAT_I_CROUCH -DFEAT_I_JUMP -DFEAT_I_RAPID

# --- android: download NDK, cross-compile arm64 injector ---
echo "--- android arm64 ---"
NDK=/tmp/android-ndk-r26b
if [ ! -d "$NDK" ]; then
  echo "downloading NDK (~1GB)..."
  curl -fsSL https://dl.google.com/android/repository/android-ndk-r26b-linux.zip -o /tmp/ndk.zip
  unzip -q /tmp/ndk.zip -d /tmp/ && rm /tmp/ndk.zip
fi
TC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
CC="$TC/aarch64-linux-android21-clang++"
[ -x "$CC" ] || { echo "NDK clang not found at $CC"; exit 1; }
AND_GAMES="standoff2:com.axlebolt.standoff2 pubgm:com.tencent.ig freefire:com.dts.freefireth codm:com.activision.callofduty.shooter mcpe:com.mojang.minecraftpe"
for entry in $AND_GAMES; do
  g=${entry%%:*}; pkg=${entry##*:}
  $CC -O2 -static-libstdc++ -o ../builds/cf_inject_$g main.cpp -DPKG=\"$pkg\" -llog && echo "ok and/$g ($pkg)" || echo "FAIL and/$g"
done
ls -lh ../builds/ | tail -16
