#include "configdb.hh"

using namespace Audacious;

ConfValue::~ConfValue(void)
{
	// make sure we don't leak any string data
	if (this->strval != NULL)
		g_free(this->strval);
}

std::string ConfValue::asString(void)
{
	return this->strval;
}

gint ConfValue::asInt(void)
{
	return this->intval;
}

bool ConfValue::asBool(void)
{
	return this->boolval;
}

gfloat ConfValue::asFloat(void)
{
	return this->floatval;
}

gdouble ConfValue::asDouble(void)
{
	return this->dblval;
}

// *************************************************************************

ConfigDB::ConfigDB(void)
{
	this->db = bmp_cfg_db_open();
}

ConfigDB::~ConfigDB(void)
{
	bmp_cfg_db_close(this->db);
}

ConfValue *ConfigDB::GetValue(std::string &section, std::string &value, ConfigDB::ValueType type)
{
	ConfValue *val = new ConfValue;

	switch(type)
	{
		case String:
			bmp_cfg_db_get_string(this->db, section.c_str(), value.c_str(), &val->strval);
			break;

		case Int:
			bmp_cfg_db_get_int(this->db, section.c_str(), value.c_str(), &val->intval);
			break;

		case Bool:
			gboolean tmp;

			bmp_cfg_db_get_bool(this->db, section.c_str(), value.c_str(), &tmp);

			if (tmp != 0)
				val->boolval = true;
			else
				val->boolval = false;

			break;

		case Float:
			bmp_cfg_db_get_float(this->db, section.c_str(), value.c_str(), &val->floatval);
			break;

		case Double:
			bmp_cfg_db_get_double(this->db, section.c_str(), value.c_str(), &val->dblval);
			break;

		default:
			g_warning("Unknown value passed to Audacious::ConfigDB::GetValue!");
			break;
	}

	return val;
}

void ConfigDB::SetValue(std::string &section, std::string &name, std::string &value)
{
	// XXX bmp_cfg_db_set_string should be const char
	// because it's not, we have to do this:

	gchar *foo = g_strdup(value.c_str());

	bmp_cfg_db_set_string(this->db, section.c_str(), name.c_str(), foo);

	g_free(foo);
}

void ConfigDB::SetValue(std::string &section, std::string &name, gint value)
{
	bmp_cfg_db_set_int(this->db, section.c_str(), name.c_str(), value);
}

void ConfigDB::SetValue(std::string &section, std::string &name, bool value)
{
	bmp_cfg_db_set_bool(this->db, section.c_str(), name.c_str(), value);
}

void ConfigDB::SetValue(std::string &section, std::string &name, gfloat value)
{
	bmp_cfg_db_set_float(this->db, section.c_str(), name.c_str(), value);
}

void ConfigDB::SetValue(std::string &section, std::string &name, gdouble value)
{
	bmp_cfg_db_set_double(this->db, section.c_str(), name.c_str(), value);
}
