Game_Music_Emu 0.3.0: Game Music Emulators
------------------------------------------
Game_Music_Emu is a collection of portable video game music emulators. Its
modular design allows elimination of any unneeded emulators and features.
Modules are included supporting the following file formats:

GBS     Nintendo Game Boy
VGM/VGZ Sega Master System/Genesis/Mega Drive/Mark III/BBC Micro
GYM     Sega Genesis
SPC     Super Nintendo
NSF     Nintendo NES (with VRC6, N106, and FME-7 sound)

This library has been used in game music players for Win32, Linux x86-32/64,
Mac OS X, Mac OS Classic, MorphOS (Amiga), PlayStation Portable, and GP2X.

Author : Shay Green <hotpop.com@blargg>
Website: http://www.slack.net/~ant/
Forum  : http://groups.google.com/group/blargg-sound-libs
License: GNU Lesser General Public License (LGPL)


Getting Started
---------------
Build a program consisting of demo/basics.cpp, demo/Wave_Writer.cpp, and all
source files in gme/ except Gzip_File.cpp. Be sure "test.nsf" is in the same
directory. Running the program should generate a WAVE sound file "out.wav" of
music.

See notes.txt for more information, and respective header (.h) files for
reference. Post to the discussion forum for assistance.


Files
-----
notes.txt               General notes about the library
changes.txt             Changes made since previous releases
design.txt              Library design notes
LGPL.txt                GNU Lesser General Public License

test.nsf                Test file for NSF emulator

demo/
  basics.cpp            Loads game music file and records to wave sound file
  info_fields.cpp       Reads information tags from files
  multi_format.cpp      Handles multiple game music types
  custom_reader.cpp     Loads music data from gzip file and memory block
  stereo_effects.cpp    Uses Effects_Buffer to add stereo echo

  simple_player.cpp     Uses Music_Player to make simple player
  Music_Player.cpp      Simple game music player module using SDL sound
  Music_Player.h

  Wave_Writer.h         WAVE sound file writer used for demo output
  Wave_Writer.cpp

gme/
  Effects_Buffer.h      Sound buffer with adjustable stereo echo and panning
  Effects_Buffer.cpp

  Gzip_File.h           Gzip reader for transparent access to gzipped files
  Gzip_File.cpp

  Music_Emu.h           Game music emulator interface

  Nsf_Emu.h             Nintendo NES NSF emulator
  Nsf_Emu.cpp
  Nes_Apu.cpp
  Nes_Apu.h
  Nes_Cpu.cpp
  Nes_Cpu.h
  Nes_Oscs.cpp
  Nes_Oscs.h
  Nes_Fme7_Apu.cpp
  Nes_Fme7_Apu.h
  Nes_Namco_Apu.cpp
  Nes_Namco_Apu.h
  Nes_Vrc6_Apu.cpp
  Nes_Vrc6_Apu.h

  Gbs_Emu.h             Nintendo Game Boy GBS emulator
  Gbs_Emu.cpp
  Gb_Apu.cpp
  Gb_Apu.h
  Gb_Cpu.cpp
  Gb_Cpu.h
  Gb_Oscs.cpp
  Gb_Oscs.h

  Spc_Emu.h             Super Nintendo SPC emulator
  Spc_Emu.cpp
  Snes_Spc.cpp
  Snes_Spc.h
  Spc_Cpu.cpp
  Spc_Cpu.h
  Spc_Dsp.cpp
  Spc_Dsp.h
  Fir_Resampler.cpp
  Fir_Resampler.h

  Gym_Emu.h             Sega Genesis GYM emulator
  Gym_Emu.cpp
  Vgm_Emu.h             Sega VGM emulator
  Vgm_Emu_Impl.cpp
  Vgm_Emu_Impl.h
  Vgm_Emu.cpp
  Ym2413_Emu.cpp
  Ym2413_Emu.h
  Sms_Apu.cpp           Common Sega emulator files
  Sms_Apu.h
  Sms_Oscs.h
  Ym2612_Emu.cpp
  Ym2612_Emu.h
  Dual_Resampler.cpp
  Dual_Resampler.h
  Fir_Resampler.cpp
  Fir_Resampler.h
  
  blargg_common.h       Common files
  blargg_endian.h
  blargg_source.h
  Blip_Buffer.cpp
  Blip_Buffer.h
  Music_Emu.cpp
  Classic_Emu.h
  Classic_Emu.cpp
  Multi_Buffer.h
  Multi_Buffer.cpp
  abstract_file.cpp
  abstract_file.h


Legal
-----
Game_Music_Emu library copyright (C) 2003-2006 Shay Green.
SNES SPC DSP emulator based on OpenSPC, copyright (C) 2002 Brad Martin.
Sega Genesis YM2612 emulator copyright (C) 2002 Stephane Dallongeville.
