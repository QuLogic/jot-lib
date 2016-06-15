/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/

#include "config.H"
#include "error.H"

/*****************************************************************
 * Config
 *****************************************************************/

//////////////////////////////////////////////////////
// Config Static Variables Initialization
//////////////////////////////////////////////////////

int          ConfigInit::count = 0;
Config*      Config::_instance = nullptr;
bool         Config::_replace  = true;
bool         Config::_loaded   = false;
set<string> *Config::_no_warn  = nullptr;

//////////////////////////////////////////////////////
// Config Methods
//////////////////////////////////////////////////////

/////////////////////////////////////
// Constructor
/////////////////////////////////////
Config::Config(const string& j) : _jot_root(j)
{ 
   assert(!_instance);  
   err_mesg(ERR_LEV_SPAM, "Config::Config() - Init...");
   _instance = this; 
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
Config::~Config()                          
{ 
   err_mesg(ERR_LEV_SPAM, "Config::~Config() - Closing down...");
   _instance = nullptr;
}


bool
Config::load_config(const string &f, bool rep)
{ 
   bool old_rep = _replace;
   _replace = rep;
   bool ret = ((_instance)?(_instance->load(f)):(false)); 
      
   _loaded |= ret;
   _replace = old_rep;
   return ret;
}

/////////////////////////////////////
// get_var_is_set()
/////////////////////////////////////
bool
Config::get_var_is_set(const string& var)
{
   return getenv(var.c_str()) ? true : false;
}

/////////////////////////////////////
// set_var_int()
/////////////////////////////////////
void
Config::set_var_int(const string& var, int int_val)
{
   if (_replace || !get_var_is_set(var))
   {
      char tmp[64];
      sprintf(tmp, "%d", int_val);
      setenv(var.c_str(), tmp, 1);
   }
   else
   {
      err_mesg(ERR_LEV_WARN,
               "Config::set_var_int() - ***Variable '%s' is already set. Ignoring new value...***",
               var.c_str());
   }
}

/////////////////////////////////////
// set_var_dbl()
/////////////////////////////////////
void
Config::set_var_dbl(const string& var, double dbl_val)
{
   if (_replace || !get_var_is_set(var))
   {
      char tmp[64];
      sprintf(tmp, "%g", dbl_val);
      setenv(var.c_str(), tmp, 1);
   }
   else
   {
      err_mesg(ERR_LEV_WARN,
               "Config::set_var_dbl() - ***Variable '%s' is already set. Ignoring new value...***",
               var.c_str());
   }
}
 
/////////////////////////////////////
// set_var_str()
/////////////////////////////////////
void
Config::set_var_str(const string& var, const string& str_val)
{
   if (_replace || !get_var_is_set(var))
   {
      setenv(var.c_str(), str_val.c_str(), 1);
   }
   else
   {
      err_mesg(ERR_LEV_WARN,
               "Config::set_var_str() - ***Variable '%s' is already set. Ignoring new value...***",
               var.c_str());
   }
}

/////////////////////////////////////
// set_var_bool()
/////////////////////////////////////
void
Config::set_var_bool(const string& var, bool bool_val)
{
   if (_replace || !get_var_is_set(var))
   {
      setenv(var.c_str(), bool_val?"true":"false", 1);
   }
   else
   {
      err_mesg(ERR_LEV_WARN,
               "Config::set_var_bool() - ***Variable '%s' is already set. Ignoring new value...***",
               var.c_str());
   }
}

/////////////////////////////////////
// get_var_int()
/////////////////////////////////////
int
Config::get_var_int(const string& var, int def_int_val, bool store)
{
   if (!_instance)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_int() - ***WARNING*** Variable [%s] accessed without an existing _instance!!",
                    var.c_str());
   if (!_loaded)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_int() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.",
                    var.c_str());

   int int_val;

   if (!get_var_is_set(var))
   {
      int_val = def_int_val;
      if (store) set_var_int(var, def_int_val);
   }
   else
   {
      int_val = atoi(getenv(var.c_str()));
   }

   return int_val;   
}

/////////////////////////////////////
// get_var_dbl()
/////////////////////////////////////
double
Config::get_var_dbl(const string& var, double def_dbl_val, bool store)
{

   if (!_instance)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_dbl() - ***WARNING*** Variable [%s] accessed without an existing _instance!!",
                    var.c_str());
   if (!_loaded)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_dbl() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.",
                    var.c_str());
   
   double dbl_val;

   if (!get_var_is_set(var))
   {
      dbl_val = def_dbl_val;
      if (store) set_var_dbl(var, def_dbl_val);
   }
   else
   {
      dbl_val = atof(getenv(var.c_str()));
   }

   return dbl_val;   
}


/////////////////////////////////////
// get_var_str()
/////////////////////////////////////
string
Config::get_var_str(const string& var, const string& def_str_val, bool store)
{
   if (!_instance)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_str() - ***WARNING*** Variable [%s] accessed without an existing _instance!!",
                    var.c_str());
   if (!_loaded)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_str() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.",
                    var.c_str());

   string str_val;

   if (!get_var_is_set(var))
   {
      str_val = def_str_val;
      if (store) set_var_str(var, def_str_val);
   }
   else
   {
      str_val = string(getenv(var.c_str()));
   }

   return str_val;   
}

/////////////////////////////////////
// get_var_bool()
/////////////////////////////////////
bool
Config::get_var_bool(const string& var, bool def_bool_val, bool store)
{
   if (!_instance)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_bool() - ***WARNING*** Variable [%s] accessed without an existing _instance!!",
                    var.c_str());
   if (!_loaded)
      err_mesg_cond(_no_warn->find(var) == _no_warn->end(),
                    ERR_LEV_WARN,
                    "Config::get_var_bool() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.",
                    var.c_str());
   
   bool bool_val;

   if (!get_var_is_set(var))
   {
      bool_val = def_bool_val;
      if (store) set_var_bool(var, def_bool_val);
   }
   else
   {
      string foo = string(getenv(var.c_str()));

      if (foo == "true")
      {
         bool_val = true;
      }
      else if (foo == "false")
      {
         bool_val = false;
      }
      else
      {
         err_mesg(ERR_LEV_WARN,
                  "Config::get_var_bool - ERROR! Boolean environment variable '%s' should be either 'true' or 'false'. Changing to 'false'.",
                  var.c_str());
         bool_val = false;
         set_var_bool(var, bool_val);
      }
   }

   return bool_val;   
}
