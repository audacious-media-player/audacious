lib_LTLIBRARIES = libconsole.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libconsole_la_SOURCES = \
	Blip_Buffer.cpp     \
	Classic_Emu.cpp     \
	Dual_Resampler.cpp  \
	Fir_Resampler.cpp   \
	Gb_Apu.cpp      \
	Gb_Cpu.cpp      \
	Gb_Oscs.cpp     \
	Gbs_Emu.cpp     \
	Gym_Emu.cpp     \
	Multi_Buffer.cpp    \
	Music_Emu.cpp       \
	Nes_Apu.cpp     \
	Nes_Cpu.cpp     \
	Nes_Fme7_Apu.cpp    \
	Nes_Namco_Apu.cpp   \
	Nes_Oscs.cpp        \
	Nes_Vrc6_Apu.cpp    \
	Nsfe_Emu.cpp        \
	Nsf_Emu.cpp     \
	Sms_Apu.cpp     \
	Snes_Spc.cpp        \
	Spc_Cpu.cpp     \
	Spc_Dsp.cpp     \
	Spc_Emu.cpp     \
	Vgm_Emu.cpp     \
	abstract_file.cpp   \
	Vfs_File.cpp        \
	Gzip_File.cpp       \
	Vgm_Emu_Impl.cpp    \
	Ym2413_Emu.cpp      \
	Ym2612_Emu.cpp      \
	Track_Emu.cpp       \
	Audacious_Driver.cpp

libconsole_la_LDFLAGS = $(PLUGIN_LDFLAGS)
libconsole_la_LIBADD = -lz $(GTK_LIBS) $(top_builddir)/libaudacious/libaudacious.la
INCLUDES = $(GTK_CFLAGS) $(ARCH_DEFINES) -I$(top_srcdir)/intl -I$(top_srcdir)

