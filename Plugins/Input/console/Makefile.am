lib_LTLIBRARIES = libconsole.la

libdir = $(plugin_dir)/$(INPUT_PLUGIN_DIR)

libconsole_la_SOURCES = \
	Blip_Buffer.cpp		\
	Classic_Emu.cpp		\
	Effects_Buffer.cpp	\
	Fir_Resampler.cpp	\
	Gb_Apu.cpp		\
	Gb_Cpu.cpp		\
	Gb_Oscs.cpp		\
	Gbs_Emu.cpp		\
	Gym_Emu.cpp		\
	Multi_Buffer.cpp	\
	Music_Emu.cpp		\
	Nes_Apu.cpp		\
	Nes_Cpu.cpp		\
	Nes_Namco.cpp		\
	Nes_Oscs.cpp		\
	Nes_Vrc6.cpp		\
	Nsf_Emu.cpp		\
	Panning_Buffer.cpp	\
	Sms_Apu.cpp		\
	Snes_Spc.cpp		\
	Spc_Cpu.cpp		\
	Spc_Dsp.cpp		\
	Spc_Emu.cpp		\
	Vgm_Emu.cpp		\
	abstract_file.cpp	\
	ym2612.cpp		\
	Audacious_Driver.cpp

libconsole_la_LDFLAGS = $(PLUGIN_LDFLAGS)
libconsole_la_LIBADD = $(GTK_LIBS) $(top_builddir)/libaudacious/libaudacious.la
INCLUDES = $(GTK_CFLAGS) $(ARCH_DEFINES) -I$(top_srcdir)/intl -I$(top_srcdir) -Iboost/

