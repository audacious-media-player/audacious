include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libconsole$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = \
	Blip_Buffer.cxx     \
	Classic_Emu.cxx     \
	Dual_Resampler.cxx  \
	Fir_Resampler.cxx   \
	Gb_Apu.cxx      \
	Gb_Cpu.cxx      \
	Gb_Oscs.cxx     \
	Gbs_Emu.cxx     \
	Gym_Emu.cxx     \
	Multi_Buffer.cxx    \
	Music_Emu.cxx       \
	Nes_Apu.cxx     \
	Nes_Cpu.cxx     \
	Nes_Fme7_Apu.cxx    \
	Nes_Namco_Apu.cxx   \
	Nes_Oscs.cxx        \
	Nes_Vrc6_Apu.cxx    \
	Nsfe_Emu.cxx        \
	Nsf_Emu.cxx     \
	Sms_Apu.cxx     \
	Snes_Spc.cxx        \
	Spc_Cpu.cxx     \
	Spc_Dsp.cxx     \
	Spc_Emu.cxx     \
	Vgm_Emu.cxx     \
	abstract_file.cxx   \
	Vfs_File.cxx        \
	Gzip_File.cxx       \
	Vgm_Emu_Impl.cxx    \
	Ym2413_Emu.cxx      \
	Ym2612_Emu.cxx      \
	Track_Emu.cxx       \
	Audacious_Config.cxx \
	Audacious_Driver.cxx

OBJECTS = ${SOURCES:.cxx=.o}

LIBADD += -lz $(GTK_LIBS) -lstdc++
CXXFLAGS += $(PICFLAGS) $(GTK_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../..

include ../../../mk/objective.mk
