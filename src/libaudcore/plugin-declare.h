/*
 * plugin-declare.h
 * Copyright 2014 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

/* Required: AUD_PLUGIN_NAME */

#ifndef AUD_PLUGIN_DOMAIN
#define AUD_PLUGIN_DOMAIN PACKAGE
#endif
#ifndef AUD_PLUGIN_ABOUT
#define AUD_PLUGIN_ABOUT nullptr
#endif
#ifndef AUD_PLUGIN_PREFS
#define AUD_PLUGIN_PREFS nullptr
#endif
#ifndef AUD_PLUGIN_INIT
#define AUD_PLUGIN_INIT nullptr
#endif
#ifndef AUD_PLUGIN_CLEANUP
#define AUD_PLUGIN_CLEANUP nullptr
#endif
#ifndef AUD_PLUGIN_TAKE_MESSAGE
#define AUD_PLUGIN_TAKE_MESSAGE nullptr
#endif
#ifndef AUD_PLUGIN_ABOUTWIN
#define AUD_PLUGIN_ABOUTWIN nullptr
#endif
#ifndef AUD_PLUGIN_CONFIGWIN
#define AUD_PLUGIN_CONFIGWIN nullptr
#endif

#ifdef AUD_DECLARE_TRANSPORT

/* Required: AUD_TRANSPORT_SCHEMES
 *           AUD_TRANSPORT_VTABLE */

AUD_TRANSPORT_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_TRANSPORT_SCHEMES,
    AUD_TRANSPORT_VTABLE
)

#endif // AUD_DECLARE_TRANSPORT

#ifdef AUD_DECLARE_PLAYLIST

/* Required: AUD_PLAYLIST_EXTS
 *           AUD_PLAYLIST_LOAD */

#ifndef AUD_PLAYLIST_SAVE
#define AUD_PLAYLIST_SAVE nullptr
#endif

AUD_PLAYLIST_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_PLAYLIST_EXTS,
    AUD_PLAYLIST_LOAD,
    AUD_PLAYLIST_SAVE
)

#endif // AUD_DECLARE_PLAYLIST

#ifdef AUD_DECLARE_INPUT

#ifndef AUD_INPUT_SUBTUNES
#define AUD_INPUT_SUBTUNES false
#endif
#ifndef AUD_INPUT_EXTS
#define AUD_INPUT_EXTS nullptr
#endif
#ifndef AUD_INPUT_MIMES
#define AUD_INPUT_MIMES nullptr
#endif
#ifndef AUD_INPUT_SCHEMES
#define AUD_INPUT_SCHEMES nullptr
#endif
#ifndef AUD_INPUT_PRIORITY
#define AUD_INPUT_PRIORITY 0
#endif

/* Required: AUD_INPUT_IS_OUR_FILE
 *           AUD_INPUT_READ_TUPLE
 *           AUD_INPUT_PLAY */

#ifndef AUD_INPUT_WRITE_TUPLE
#define AUD_INPUT_WRITE_TUPLE nullptr
#endif
#ifndef AUD_INPUT_READ_IMAGE
#define AUD_INPUT_READ_IMAGE nullptr
#endif
#ifndef AUD_INPUT_INFOWIN
#define AUD_INPUT_INFOWIN nullptr
#endif

AUD_INPUT_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_INPUT_SUBTUNES,
    AUD_INPUT_EXTS,
    AUD_INPUT_MIMES,
    AUD_INPUT_SCHEMES,
    AUD_INPUT_PRIORITY,
    AUD_INPUT_IS_OUR_FILE,
    AUD_INPUT_READ_TUPLE,
    AUD_INPUT_PLAY,
    AUD_INPUT_WRITE_TUPLE,
    AUD_INPUT_READ_IMAGE,
    AUD_INPUT_INFOWIN
)

#endif // AUD_DECLARE_INPUT

#ifdef AUD_DECLARE_EFFECT

#ifndef AUD_EFFECT_START
#define AUD_EFFECT_START nullptr
#endif

/* Required: AUD_EFFECT_PROCESS */

#ifndef AUD_EFFECT_FLUSH
#define AUD_EFFECT_FLUSH nullptr
#endif
#ifndef AUD_EFFECT_FINISH
#define AUD_EFFECT_FINISH nullptr
#endif
#ifndef AUD_EFFECT_ADJ_DELAY
#define AUD_EFFECT_ADJ_DELAY nullptr
#endif
#ifndef AUD_EFFECT_ORDER
#define AUD_EFFECT_ORDER 0
#endif
#ifndef AUD_EFFECT_SAME_FMT
#define AUD_EFFECT_SAME_FMT false
#endif

AUD_EFFECT_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_EFFECT_START,
    AUD_EFFECT_PROCESS,
    AUD_EFFECT_FLUSH,
    AUD_EFFECT_FINISH,
    AUD_EFFECT_ADJ_DELAY,
    AUD_EFFECT_ORDER,
    AUD_EFFECT_SAME_FMT
)

#endif // AUD_DECLARE_EFFECT

#ifdef AUD_DECLARE_OUTPUT

#ifndef AUD_OUTPUT_PRIORITY
#define AUD_OUTPUT_PRIORITY 0
#endif
#ifndef AUD_OUTPUT_GET_VOLUME
#define AUD_OUTPUT_GET_VOLUME nullptr
#endif
#ifndef AUD_OUTPUT_SET_VOLUME
#define AUD_OUTPUT_SET_VOLUME nullptr
#endif

/* Required: AUD_OUTPUT_OPEN
 *           AUD_OUTPUT_CLOSE
 *           AUD_OUTPUT_GET_FREE
 *           AUD_OUTPUT_WAIT_FREE
 *           AUD_OUTPUT_WRITE
 *           AUD_OUTPUT_DRAIN
 *           AUD_OUTPUT_GET_TIME
 *           AUD_OUTPUT_PAUSE
 *           AUD_OUTPUT_FLUSH */

#ifndef AUD_OUTPUT_REOPEN
#define AUD_OUTPUT_REOPEN false
#endif

AUD_OUTPUT_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_OUTPUT_PRIORITY,
    AUD_OUTPUT_GET_VOLUME,
    AUD_OUTPUT_SET_VOLUME,
    AUD_OUTPUT_OPEN,
    AUD_OUTPUT_CLOSE,
    AUD_OUTPUT_GET_FREE,
    AUD_OUTPUT_WAIT_FREE,
    AUD_OUTPUT_WRITE,
    AUD_OUTPUT_DRAIN,
    AUD_OUTPUT_GET_TIME,
    AUD_OUTPUT_PAUSE,
    AUD_OUTPUT_FLUSH,
    AUD_OUTPUT_REOPEN
)

#endif // AUD_DECLARE_OUTPUT

#ifdef AUD_DECLARE_VIS

/* Required: AUD_VIS_CLEAR */

#ifndef AUD_VIS_RENDER_MONO
#define AUD_VIS_RENDER_MONO nullptr
#endif
#ifndef AUD_VIS_RENDER_MULTI
#define AUD_VIS_RENDER_MULTI nullptr
#endif
#ifndef AUD_VIS_RENDER_FREQ
#define AUD_VIS_RENDER_FREQ nullptr
#endif
#ifndef AUD_VIS_GET_WIDGET
#define AUD_VIS_GET_WIDGET nullptr
#endif

AUD_VIS_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_VIS_CLEAR,
    AUD_VIS_RENDER_MONO,
    AUD_VIS_RENDER_MULTI,
    AUD_VIS_RENDER_FREQ,
    AUD_VIS_GET_WIDGET
)

#endif // AUD_DECLARE_VIS

#ifdef AUD_DECLARE_GENERAL

#ifndef AUD_GENERAL_AUTO_ENABLE
#define AUD_GENERAL_AUTO_ENABLE  false
#endif
#ifndef AUD_GENERAL_GET_WIDGET
#define AUD_GENERAL_GET_WIDGET  nullptr
#endif

AUD_GENERAL_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_GENERAL_AUTO_ENABLE,
    AUD_GENERAL_GET_WIDGET
)

#endif // AUD_DECLARE_GENERAL

#ifdef AUD_DECLARE_IFACE

/* Required: AUD_IFACE_SHOW
 *           AUD_IFACE_RUN
 *           AUD_IFACE_QUIT */

#ifndef AUD_IFACE_SHOW_ABOUT
#define AUD_IFACE_SHOW_ABOUT nullptr
#endif

#ifndef AUD_IFACE_HIDE_ABOUT
#define AUD_IFACE_HIDE_ABOUT nullptr
#endif

#ifndef AUD_IFACE_SHOW_FILEBROWSER
#define AUD_IFACE_SHOW_FILEBROWSER nullptr
#endif

#ifndef AUD_IFACE_HIDE_FILEBROWSER
#define AUD_IFACE_HIDE_FILEBROWSER nullptr
#endif

#ifndef AUD_IFACE_SHOW_JUMP_TO_SONG
#define AUD_IFACE_SHOW_JUMP_TO_SONG nullptr
#endif

#ifndef AUD_IFACE_HIDE_JUMP_TO_SONG
#define AUD_IFACE_HIDE_JUMP_TO_SONG nullptr
#endif

#ifndef AUD_IFACE_SHOW_SETTINGS
#define AUD_IFACE_SHOW_SETTINGS nullptr
#endif

#ifndef AUD_IFACE_HIDE_SETTINGS
#define AUD_IFACE_HIDE_SETTINGS nullptr
#endif

#ifndef AUD_IFACE_MENU_ADD
#define AUD_IFACE_MENU_ADD nullptr
#endif

#ifndef AUD_IFACE_MENU_REMOVE
#define AUD_IFACE_MENU_REMOVE nullptr
#endif

AUD_IFACE_PLUGIN (
    AUD_PLUGIN_NAME,
    AUD_PLUGIN_DOMAIN,
    AUD_PLUGIN_ABOUT,
    AUD_PLUGIN_PREFS,
    AUD_PLUGIN_INIT,
    AUD_PLUGIN_CLEANUP,
    AUD_PLUGIN_TAKE_MESSAGE,
    AUD_PLUGIN_ABOUTWIN,
    AUD_PLUGIN_CONFIGWIN,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    AUD_IFACE_SHOW,
    AUD_IFACE_RUN,
    AUD_IFACE_QUIT,
    AUD_IFACE_SHOW_ABOUT,
    AUD_IFACE_HIDE_ABOUT,
    AUD_IFACE_SHOW_FILEBROWSER,
    AUD_IFACE_HIDE_FILEBROWSER,
    AUD_IFACE_SHOW_JUMP_TO_SONG,
    AUD_IFACE_HIDE_JUMP_TO_SONG,
    AUD_IFACE_SHOW_SETTINGS,
    AUD_IFACE_HIDE_SETTINGS,
    AUD_IFACE_MENU_ADD,
    AUD_IFACE_MENU_REMOVE
)

#endif // AUD_DECLARE_IFACE
