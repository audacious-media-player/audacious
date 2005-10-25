/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_PLUGIN_H
#define _LV_PLUGIN_H

#include <libvisual/lv_video.h>
#include <libvisual/lv_audio.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_songinfo.h>
#include <libvisual/lv_event.h>
#include <libvisual/lv_param.h>
#include <libvisual/lv_ui.h>
#include <libvisual/lv_random.h>
#include <libvisual/lv_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_PLUGINREF(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisPluginRef))
#define VISUAL_PLUGININFO(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisPluginInfo))
#define VISUAL_PLUGINDATA(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisPluginData))
#define VISUAL_PLUGINENVIRON(obj)			(VISUAL_CHECK_CAST ((obj), 0, VisPluginEnviron))
	
#define VISUAL_PLUGIN_ACTOR(obj)			(VISUAL_CHECK_CAST ((obj), VISUAL_PLUGIN_TYPE_ACTOR_ENUM, VisActorPlugin))
#define VISUAL_PLUGIN_INPUT(obj)			(VISUAL_CHECK_CAST ((obj), VISUAL_PLUGIN_TYPE_INPUT_ENUM, VisInputPlugin))
#define VISUAL_PLUGIN_MORPH(obj)			(VISUAL_CHECK_CAST ((obj), VISUAL_PLUGIN_TYPE_MORPH_ENUM, VisMorphPlugin))
#define VISUAL_PLUGIN_TRANSFORM(obj)			(VISUAL_CHECK_CAST ((obj), VISUAL_PLUGIN_TYPE_TRANSFORM_ENUM, VisTransformPlugin))

/**
 * Indicates at which version the plugin API is.
 */
#define VISUAL_PLUGIN_API_VERSION	2

/**
 * Type defination that should be used in plugins to set the plugin type for a NULL plugin.
 */
#define VISUAL_PLUGIN_TYPE_NULL		"Libvisual:core:null"
/**
 * Type defination that should be used in plugins to set the plugin type for an actor plugin.
 */
#define VISUAL_PLUGIN_TYPE_ACTOR	"Libvisual:core:actor"
/**
 * Type defination that should be used in plugins to set the plugin type for an input  plugin.
 */
#define VISUAL_PLUGIN_TYPE_INPUT	"Libvisual:core:input"
/**
 * Type defination that should be used in plugins to set the plugin type for a morph plugin.
 */
#define VISUAL_PLUGIN_TYPE_MORPH	"Libvisual:core:morph"
/**
 * Type defination that should be used in plugins to set the plugin type for a transform plugin.
 */
#define VISUAL_PLUGIN_TYPE_TRANSFORM	"Libvisual:core:transform"
	
/**
 * Enumerate to define the plugin type. Especially used
 * within the VisPlugin system and for type checking within the core library itself.
 *
 * For plugin type defination use the VISUAL_PLUGIN_TYPE_NULL, VISUAL_PLUGIN_TYPE_ACTOR
 * VISUAL_PLUGIN_TYPE_INPUT or VISUAL_PLUGIN_TYPE_MORPH defines that contain the domain string.
 *
 * There are three different plugins being:
 * 	-# Actor plugins: These are the actual
 * 	visualisation plugins.
 * 	-# Input plugins: These can be used to obtain
 * 	PCM data through, for example different sound servers.
 * 	-# Morph plugins: These are capable of morphing
 * 	between different plugins.
 * 	-# Transform plugins: These are capable of transforming a video or palette
 * 	into something new.
 */
typedef enum {
	VISUAL_PLUGIN_TYPE_NULL_ENUM,		/**< Used when there is no plugin. */
	VISUAL_PLUGIN_TYPE_ACTOR_ENUM,		/**< Used when the plugin is an actor plugin. */
	VISUAL_PLUGIN_TYPE_INPUT_ENUM,		/**< Used when the plugin is an input plugin. */
	VISUAL_PLUGIN_TYPE_MORPH_ENUM,		/**< Used when the plugin is a morph plugin. */
	VISUAL_PLUGIN_TYPE_TRANSFORM_ENUM	/**< Used when the plugin is a transform plugin. */
} VisPluginType;

/**
 * Enumerate to define the plugin flags. Plugin flags can be used to
 * define some of the plugin it's behavior.
 */
typedef enum {
	VISUAL_PLUGIN_FLAG_NONE			= 0,	/**< Used to set no flags. */
	VISUAL_PLUGIN_FLAG_NOT_REENTRANT	= 1,	/**< Used to tell the plugin loader that this plugin
							  * is not reentrant, and can be loaded only once. */
	VISUAL_PLUGIN_FLAG_SPECIAL		= 2	/**< Used to tell the plugin loader that this plugin has
							  * special purpose, like the GdkPixbuf plugin, or a webcam
							  * plugin. */
} VisPluginFlags;

/**
 * Enumerate to check the depth of the type wildcard/defination used, used together with the visual_plugin_type functions.
 */
typedef enum {
	VISUAL_PLUGIN_TYPE_DEPTH_NONE		= 0,	/**< No type found.*/
	VISUAL_PLUGIN_TYPE_DEPTH_DOMAIN		= 1,	/**< Only domain in type. */
	VISUAL_PLUGIN_TYPE_DEPTH_PACKAGE	= 2,	/**< Domain and package in type. */
	VISUAL_PLUGIN_TYPE_DEPTH_TYPE		= 3,	/**< Domain, package and type found in type. */
} VisPluginTypeDepth;

typedef struct _VisPluginRef VisPluginRef;
typedef struct _VisPluginInfo VisPluginInfo;
typedef struct _VisPluginData VisPluginData;
typedef struct _VisPluginEnviron VisPluginEnviron;

typedef struct _VisActorPlugin VisActorPlugin;
typedef struct _VisInputPlugin VisInputPlugin;
typedef struct _VisMorphPlugin VisMorphPlugin;
typedef struct _VisTransformPlugin VisTransformPlugin;

/* Actor plugin methods */

/**
 * An actor plugin needs this signature for the requisition function. The requisition function
 * is used to determine the size required by the plugin for a given width/height value.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg width Pointer to an int containing the width requested, will be altered to the nearest
 * 	supported width.
 * @arg height Pointer to an int containing the height requested, will be altered to the nearest
 * 	supported height.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginActorRequisitionFunc)(VisPluginData *plugin, int *width, int *height);

/**
 * An actor plugin needs this signature for the palette function. The palette function
 * is used to retrieve the desired palette from the plugin.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 *
 * @return Pointer to the VisPalette used by the plugin, this should be a VisPalette with 256
 *	VisColor entries, NULL is also allowed to be returned.
 */
typedef VisPalette *(*VisPluginActorPaletteFunc)(VisPluginData *plugin);

/**
 * An actor plugin needs this signature for the render function. The render function
 * is used to render the frame for the visualisation.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg video Pointer to the VisVideo containing all information about the display surface.
 *	Params like height and width won't suddenly change, this is always notified as an event
 *	so the plugin can adjust to the new dimension.
 * @arg audio Pointer to the VisAudio containing all the data regarding the current audio sample.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginActorRenderFunc)(VisPluginData *plugin, VisVideo *video, VisAudio *audio);

/* Input plugin methods */

/**
 * An input plugin needs this signature for the sample upload function. The sample upload function
 * is used to retrieve sample information when a input is being used to retrieve the
 * audio sample.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg audio Pointer to the VisAudio in which the new sample data is set.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginInputUploadFunc)(VisPluginData *plugin, VisAudio *audio);

/* Morph plugin methods */

/**
 * A morph plugin needs this signature for the palette function. The palette function
 * is used to give a palette for the morph. The palette function isn't mandatory and the
 * VisMorph system will interpolate between the two palettes in VISUAL_VIDEO_DEPTH_8BIT when
 * a palette function isn't set.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg rate A float between 0.0 and 1.0 that tells how far the morph has proceeded.
 * @arg audio Pointer to the VisAudio containing all the data regarding the current audio sample.
 * @arg pal A pointer to the target VisPalette in which the morph between the two palettes is saved. Should have
 * 	256 VisColor entries.
 * @arg src1 A pointer to the first VisVideo source.
 * @arg src2 A pointer to the second VisVideo source.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginMorphPaletteFunc)(VisPluginData *plugin, float rate, VisAudio *audio, VisPalette *pal,
		VisVideo *src1, VisVideo *src2);

/**
 * A morph plugin needs this signature for the apply function. The apply function
 * is used to execute a morph between two VisVideo sources. It's the 'render' function of
 * the morph plugin and here is the morphing done.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg rate A float between 0.0 and 1.0 that tells how far the morph has proceeded.
 * @arg audio Pointer to the VisAudio containing all the data regarding the current audio sample.
 * @arg src1 A pointer to the first VisVideo source.
 * @arg src2 A pointer to the second VisVideo source.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginMorphApplyFunc)(VisPluginData *plugin, float rate, VisAudio *audio, VisVideo *dest,
		VisVideo *src1, VisVideo *src2);

/* Transform plugin methodes */

/**
 * A transform plugin needs this signature to transform VisPalettes.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg pal Pointer to the VisPalette that is to be morphed.
 *	Only 256 entry VisPalettes have to be supported.
 * @arg audio Optionally a pointer to the VisAudio, when requested.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginTransformPaletteFunc)(VisPluginData *plugin, VisPalette *pal, VisAudio *audio);

/**
 * A transform plugin needs this signature to transform VisVideos.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg video Pointer to the VisVideo that needs to be transformed.
 * @arg audio Optionally a pointer to the VisAudio, when requested.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginTransformVideoFunc)(VisPluginData *plugin, VisVideo *video, VisAudio *audio);

/* Plugin standard get_plugin_info method */
/**
 * This is the signature for the 'get_plugin_info' function every libvisual plugin needs to have. The 
 * 'get_plugin_info' function provides libvisual plugin data and all the detailed information regarding
 * the plugin. This function is compulsory without it libvisual won't load the plugin.
 *
 * @arg count An int pointer in which the number of VisPluginData entries within the plugin. Plugins can have
 * 	multiple 'features' and thus the count is needed.
 *
 * @return Pointer to the VisPluginInfo array which contains information about the plugin.
 */
typedef const VisPluginInfo *(*VisPluginGetInfoFunc)(int *count);

/* Standard plugin methods */

/**
 * Every libvisual plugin that is loaded by the libvisual plugin loader needs this signature for it's
 * intialize function.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginInitFunc)(VisPluginData *plugin);

/**
 * Every libvisual plugin that is loaded by the libvisual plugin loader needs this signature for it's
 * cleanup function.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginCleanupFunc)(VisPluginData *plugin);

/**
 * This is the signature for the event handler within libvisual plugins. An event handler is not mandatory because
 * it has no use in some plugin classes but some plugin types require it nonetheless.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg events Pointer to the VisEventQueue that might contain events that need to be handled.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginEventsFunc)(VisPluginData *plugin, VisEventQueue *events);

/**
 * The VisPluginRef data structure contains information about the plugins
 * and does refcounting. It is also used as entries in the plugin registry.
 */
struct _VisPluginRef {
	VisObject		 object;	/**< The VisObject data. */	

	char			*file;		/**< The file location of the plugin. */
	int			 index;		/**< Contains the index number for the entry in the VisPluginInfo table. */
	int			 usecount;	/**< The use count, this indicates how many instances are loaded. */
	VisPluginInfo		*info;		/**< A copy of the VisPluginInfo structure. */
};

/**
 * The VisPluginInfo data structure contains information about a plugin
 * and is filled within the plugin itself.
 */
struct _VisPluginInfo {
	VisObject		 object;	/**< The VisObject data. */

	uint32_t		 struct_size;	/**< Struct size, should always be set for compatability checks. */
	uint32_t		 api_version;	/**< API version, compile plugins always with .api_version = VISUAL_PLUGIN_API_VERSION. */
	char			*type;		/**< Plugin type, in the format of "domain:package:type", as example,
						 * this could be "Libvisual:core:actor". It's adviced to use the defination macros here
						 * instead of filling in the string yourself. */
	char			*plugname;	/**< The plugin name as it's saved in the registry. */

	char			*name;		/**< Long name */
	char			*author;	/**< Author */
	char			*version;	/**< Version */
	char			*about;		/**< About */
	char			*help;		/**< Help */

	VisPluginInitFunc	 init;		/**< The standard init function, every plugin has to implement this. */
	VisPluginCleanupFunc	 cleanup;	/**< The standard cleanup function, every plugin has to implement this. */
	VisPluginEventsFunc	 events;	/**< The standard event function, implementation is optional. */

	int			 flags;		/**< Plugin flags from the VisPluginFlags enumerate. */

	VisObject		*plugin;	/**< Pointer to the plugin specific data structures. */
};

/**
 * The VisPluginData structure is the main plugin structure, every plugin
 * is encapsulated in this.
 */
struct _VisPluginData {
	VisObject		 object;	/**< The VisObject data. */

	VisPluginRef		*ref;		/**< Pointer to the plugin references corresponding to this VisPluginData. */

	VisPluginInfo		*info;		/**< Pointer to the VisPluginInfo that is obtained from the plugin. */

	VisEventQueue		 eventqueue;	/**< The plugin it's VisEventQueue for queueing events. */
	VisParamContainer	*params;	/**< The plugin it's VisParamContainer in which VisParamEntries can be placed. */
	VisUIWidget		*userinterface;	/**< The plugin it's top level VisUIWidget, this acts as the container for the
						  * rest of the user interface. */

	int			 plugflags;	/**< Plugin flags, currently unused but will be used in the future. */

	VisRandomContext	 random;	/**< Pointer to the plugin it's private random context. It's highly adviced to use
						  * the plugin it's randomize functions. The reason is so more advanced apps can
						  * semi reproduce visuals. */

	int			 realized;	/**< Flag that indicates if the plugin is realized. */
	void			*handle;	/**< The dlopen handle */
	VisList			 environ;	/**< Misc environment specific data. */
};

/**
 * The VisPluginEnviron is used to setup a pre realize/init environment for plugins.
 * Some types of plugins might need this internally and thus this system provides this function.
 */
struct _VisPluginEnviron {
	VisObject		 object;	/**< The VisObject data. */
	const char		*type;		/**< Almost the same as _VisPluginInfo.type. */
	VisObject		*environ;	/**< VisObject that contains environ specific data. */
};

/**
 * The VisActorPlugin structure is the main data structure
 * for the actor (visualisation) plugin.
 *
 * The actor plugin is the visualisation plugin.
 */
struct _VisActorPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginActorRequisitionFunc	 requisition;	/**< The requisition function. This is used to
							 * get the desired VisVideo surface size of the plugin. */
	VisPluginActorPaletteFunc	 palette;	/**< Used to retrieve the desired palette from the plugin. */
	VisPluginActorRenderFunc	 render;	/**< The main render loop. This is called to draw a frame. */

	VisSongInfo			 songinfo;	/**< Pointer to VisSongInfo that contains information about
							 *the current playing song. This can be NULL. */

	int				 depth;		/**< The depth flag for the VisActorPlugin. This contains an ORred
							  * value of depths that are supported by the plugin. */
};

/**
 * The VisInputPlugin structure is the main data structure
 * for the input plugin.
 *
 * The input plugin is used to retrieve PCM samples from
 * certain sources.
 */
struct _VisInputPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginInputUploadFunc	 upload;	/**< The sample upload function. This is the main function
							  * of the plugin which uploads sample data into
							  * libvisual. */
};

/**
 * The VisMorphPlugin structure is the main data structure
 * for the morph plugin.
 *
 * The morph plugin is capable of morphing between two VisVideo
 * sources, and thus is capable of morphing between two
 * VisActors.
 */
struct _VisMorphPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginMorphPaletteFunc	 palette;	/**< The plugin's palette function. This can be used
							  * to obtain a palette for VISUAL_VIDEO_DEPTH_8BIT surfaces.
							  * However the function may be set to NULL. In this case the
							  * VisMorph system morphs between palettes itself. */
	VisPluginMorphApplyFunc		 apply;		/**< The plugin it's main function. This is used to morph
							  * between two VisVideo sources. */
	int				 depth;		/**< The depth flag for the VisMorphPlugin. This contains an ORred
							  * value of depths that are supported by the plugin. */
	int				 requests_audio;/**< When set on TRUE this will indicate that the Morph plugin
							  * requires an VisAudio context in order to render properly. */
};

/**
 * The VisTransformPlugin structure is the main data structure
 * for the transform plugin.
 *
 * The transform plugin is used to transform videos and palettes
 * and can be used in visualisation pipelines.
 */
struct _VisTransformPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginTransformPaletteFunc	 palette;	/**< Used to transform a VisPalette. Writes directly into the source. */
	VisPluginTransformVideoFunc	 video;		/**< Used to transform a VisVideo. Writes directly into the source. */

	int				 depth;		/**< The depth flag for the VisActorPlugin. This contains an ORred
							  * value of depths that are supported by the plugin. */
	int				 requests_audio;/**< When set on TRUE this will indicate that the Morph plugin
							  * requires an VisAudio context in order to render properly. */
};


/* prototypes */
VisPluginInfo *visual_plugin_info_new (void);
int visual_plugin_info_copy (VisPluginInfo *dest, VisPluginInfo *src);

int visual_plugin_events_pump (VisPluginData *plugin);
VisEventQueue *visual_plugin_get_eventqueue (VisPluginData *plugin);
int visual_plugin_set_userinterface (VisPluginData *plugin, VisUIWidget *widget);
VisUIWidget *visual_plugin_get_userinterface (VisPluginData *plugin);

VisPluginInfo *visual_plugin_get_info (VisPluginData *plugin);

VisParamContainer *visual_plugin_get_params (VisPluginData *plugin);

VisRandomContext *visual_plugin_get_random_context (VisPluginData *plugin);

void *visual_plugin_get_specific (VisPluginData *plugin);

VisPluginRef *visual_plugin_ref_new (void);

VisPluginData *visual_plugin_new (void);

VisList *visual_plugin_get_registry (void);
VisList *visual_plugin_registry_filter (VisList *pluglist, const char *domain);

const char *visual_plugin_get_next_by_name (VisList *list, const char *name);
const char *visual_plugin_get_prev_by_name (VisList *list, const char *name);

int visual_plugin_unload (VisPluginData *plugin);
VisPluginData *visual_plugin_load (VisPluginRef *ref);
int visual_plugin_realize (VisPluginData *plugin);

VisPluginRef **visual_plugin_get_references (const char *pluginpath, int *count);
VisList *visual_plugin_get_list (const char **paths);

VisPluginRef *visual_plugin_find (VisList *list, const char *name);

int visual_plugin_get_api_version (void);

VisSongInfo *visual_plugin_actor_get_songinfo (VisActorPlugin *actplugin);

const char *visual_plugin_type_get_domain (const char *type);
const char *visual_plugin_type_get_package (const char *type);
const char *visual_plugin_type_get_type (const char *type);
VisPluginTypeDepth visual_plugin_type_get_depth (const char *type);
int visual_plugin_type_member_of (const char *domain, const char *type);

VisPluginEnviron *visual_plugin_environ_new (const char *type, VisObject *environ);
int visual_plugin_environ_add (VisPluginData *plugin, VisPluginEnviron *penve);
int visual_plugin_environ_remove (VisPluginData *plugin, const char *type);
VisObject *visual_plugin_environ_get (VisPluginData *plugin, const char *type);
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_PLUGIN_H */
