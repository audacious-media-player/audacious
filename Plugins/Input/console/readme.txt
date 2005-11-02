Game_Music_Emu 0.2.4: Multi-Format Game Music Emulation Library


Game_Music_Emu is a collection of portable video game music emulators for the
following file formats: Nintendo NSF, Game Boy GBS, Sega Master System VGM,
Sega Gensesis GYM, and Super Nintendo SPC.

Licensed under the GNU Lesser General Public License (LGPL); see LGPL.txt.
Copyright (C) 2003-2005 Shay Green. SNES SPC DSP emulator based on OpenSPC,
Copyright (C) 2002 Brad Martin. Sega Genesis YM2612 emulator from Gens project,
Copyright (C) 2002 Stephane Dallongeville.

Website: http://www.slack.net/~ant/libs/
Forum  : http://groups-beta.google.com/group/blargg-sound-libs
Contact: hotpop.com@blargg (swap to e-mail)


Getting Started
---------------

This library is written in somewhat conservative C++ that should compile with
current and older compilers (ANSI/ISO and ARM).

If the Boost library is installed in your environment, delete the included
"boost" compatibility directory, otherwise add the included "boost" directory
to your compiler's search paths.

Build a program consisting of the included source files except demo_effects.cpp
and demo_panning.cpp, and any necessary system libraries. Be sure "test.nsf" is
in the same directory. The program should generate a WAVE sound file "out.wav"
of music.

For a full example of using Game_Music_Emu in a music player, see the Game
Music Box source code: http://www.slack.net/~ant/game-music-box/dev.html

See notes.txt for more information, and respective header (.h) files for
reference. Visit the discussion forum to get assistance.


Files
-----

notes.txt               Collection of notes about the library
changes.txt             Changes made since previous releases
todo.txt                Planned improvements and fixes
design.txt              Library design notes
LGPL.TXT                GNU Lesser General Public License

demo.cpp                Record NSF to WAVE sound file using emulator
demo_effects.cpp        Use Effects_Buffer while recording GBS file
demo_panning.cpp        Use Panning_Buffer while recording VGM file
test.nsf                Test file for NSF emulator

Music_Emu.h             Game music emulator interface
Spc_Emu.h               Super NES SPC emulator
Gym_Emu.h               Sega Genesis GYM emulator

Classic_Emu.h           "Classic" game music emulator interface
Nsf_Emu.h               Nintendo NSF emulator
Gbs_Emu.h               Game Boy GBS emulator
Vgm_Emu.h               Sega Master System VGM emulator

Multi_Buffer.h          Mono and stereo buffers for classic emulators
Effects_Buffer.h        Effects buffer for classic emulators
Panning_Buffer.h        Panning buffer for classic emulators

blargg_common.h         Common library source
blargg_endian.h
blargg_source.h
Blip_Buffer.cpp
Blip_Buffer.h
Blip_Synth.h
Music_Emu.cpp
Classic_Emu.cpp
Multi_Buffer.cpp
Effects_Buffer.cpp
Panning_Buffer.cpp
Fir_Resampler.cpp
Fir_Resampler.h
abstract_file.cpp
abstract_file.h
Nes_Apu.cpp             NSF emulator source
Nes_Apu.h
Nes_Cpu.cpp
Nes_Cpu.h
Nes_Oscs.cpp
Nes_Oscs.h
Nsf_Emu.cpp
Nes_Namco.cpp
Nes_Namco.h
Nes_Vrc6.cpp
Nes_Vrc6.h
Tagged_Data.h
Gbs_Emu.cpp             GBS emulator source
Gb_Apu.cpp
Gb_Apu.h
Gb_Cpu.cpp
Gb_Cpu.h
Gb_Oscs.cpp
Gb_Oscs.h
Sms_Apu.cpp             VGM emulator source
Sms_Apu.h
Sms_Oscs.h
Vgm_Emu.cpp
Gym_Emu.cpp             GYM emulator source
ym2612.cpp
ym2612.h
Spc_Emu.cpp             SPC emulator source
Snes_Spc.cpp
Snes_Spc.h
Spc_Cpu.cpp
Spc_Cpu.h
Spc_Dsp.cpp
Spc_Dsp.h

boost/                  Substitute for boost library if it's unavailable

Wave_Writer.hpp         WAVE sound file writer used for demo output
Wave_Writer.cpp

-- 
Shay Green <hotpop.com@blargg> (swap to e-mail)
