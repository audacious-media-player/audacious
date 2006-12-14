#ifndef CONFIGDB_HH
#define CONFIGDB_HH

#include <glib.h>

#include <string>

#ifdef _AUDACIOUS_CORE
# include "libaudacious/configdb.h"
#else
# include <audacious/configdb.h>
#endif

namespace Audacious
{

	/*
	 * Usage example:
	 *
	 * {
	 *     Audacious::ConfigDb foo;
	 *     Audacious::ConfValue *bar;
	 *
	 *     bar = foo.GetValue("bar", "filter");
	 *     std::string filter = bar->asString();
	 *     delete bar;
	 *
	 *     foo.SetValue("bar", "filter", "none");
	 *
	 *     foo.RemoveEntry("bar", "baz");
	 * }
	 */

	class ConfValue
	{
	public:
		gchar *strval;
		gint intval;
		bool boolval;
		gfloat floatval;
		gdouble dblval;

		std::string asString(void);
		gint asInt(void);
		bool asBool(void);
		gfloat asFloat(void);
		gdouble asDouble(void);

		~ConfValue(void);
	};

	class ConfigDB
	{
	private:
		ConfigDb *db;

	public:
		enum ValueType { String, Int, Bool, Float, Double };

		ConfValue *GetValue(std::string &section, std::string &name, ConfigDB::ValueType type);

		void SetValue(std::string &section, std::string &name, std::string &value);
		void SetValue(std::string &section, std::string &name, gint value);
		void SetValue(std::string &section, std::string &name, bool value);
		void SetValue(std::string &section, std::string &name, gfloat value);
		void SetValue(std::string &section, std::string &name, gdouble value);

		void RemoveEntry(std::string &section, std::string &value);

		ConfigDB(void);
		~ConfigDB(void);
	};
};

#endif
