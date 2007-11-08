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

int        ConfigInit::count     = 0;
Config*        Config::_instance = NULL;
bool           Config::_replace  = true;
bool           Config::_loaded   = false;
str_list*      Config::_no_warn  = NULL;

//////////////////////////////////////////////////////
// Config Methods
//////////////////////////////////////////////////////

/////////////////////////////////////
// Constructor
/////////////////////////////////////
Config::Config(Cstr_ptr& j) : _jot_root(j) 
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
   _instance = NULL;  
}


bool
Config::load_config(Cstr_ptr &f, bool rep) 
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
Config::get_var_is_set(Cstr_ptr& var) 
{
   return getenv(**var) ? true : false;
}

/////////////////////////////////////
// set_var_int()
/////////////////////////////////////
void
Config::set_var_int(Cstr_ptr& var, int int_val) 
{
   if (_replace || !get_var_is_set(var))
   {
      putenv(**(var + "=" + str_ptr(int_val)));
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "Config::set_var_int() - ***Variable '%s' is already set. Ignoring new value...***", **var);
   }
}

/////////////////////////////////////
// set_var_dbl()
/////////////////////////////////////
void
Config::set_var_dbl(Cstr_ptr& var, double dbl_val) 
{
   if (_replace || !get_var_is_set(var))
   {
      putenv(**(var + "=" + str_ptr(dbl_val)));
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "Config::set_var_dbl() - ***Variable '%s' is already set. Ignoring new value...***", **var);
   }
}
 
/////////////////////////////////////
// set_var_str()
/////////////////////////////////////
void
Config::set_var_str(Cstr_ptr& var, Cstr_ptr& str_val) 
{
   if (_replace || !get_var_is_set(var))
   {
      putenv(**(var + "=" + str_val));
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "Config::set_var_str() - ***Variable '%s' is already set. Ignoring new value...***", **var);
   }
}

/////////////////////////////////////
// set_var_bool()
/////////////////////////////////////
void
Config::set_var_bool(Cstr_ptr& var, bool bool_val) 
{
   if (_replace || !get_var_is_set(var))
   {
      putenv(**(var + "=" + ((bool_val)?("true"):("false"))));
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "Config::set_var_bool() - ***Variable '%s' is already set. Ignoring new value...***", **var);
   }

}

/////////////////////////////////////
// get_var_int()
/////////////////////////////////////
int
Config::get_var_int(Cstr_ptr& var, int def_int_val, bool store) 
{
   if (!_instance)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_int() - ***WARNING*** Variable [%s] accessed without an existing _instance!!", **var);
   if (!_loaded)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_int() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.", **var);

   int int_val;

   if (!get_var_is_set(var))
   {
      int_val = def_int_val;
      if (store) set_var_int(var,def_int_val);
   }
   else
   {
      int_val = atoi(getenv(**var));
   }

   return int_val;   
}

/////////////////////////////////////
// get_var_dbl()
/////////////////////////////////////
double
Config::get_var_dbl(Cstr_ptr& var, double def_dbl_val, bool store) 
{

   if (!_instance)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_dbl() - ***WARNING*** Variable [%s] accessed without an existing _instance!!", **var);
   if (!_loaded)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_dbl() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.", **var);
   
   double dbl_val;

   if (!get_var_is_set(var))
   {
      dbl_val = def_dbl_val;
      if (store) set_var_dbl(var,def_dbl_val);
   }
   else
   {
      dbl_val = atof(getenv(**var));
   }

   return dbl_val;   
}


/////////////////////////////////////
// get_var_str()
/////////////////////////////////////
str_ptr
Config::get_var_str(Cstr_ptr& var, Cstr_ptr& def_str_val, bool store) 
{
   if (!_instance)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_str() - ***WARNING*** Variable [%s] accessed without an existing _instance!!", **var);
   if (!_loaded)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_str() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.", **var);

   str_ptr str_val;

   if (!get_var_is_set(var))
   {
      str_val = def_str_val;
      if (store) set_var_str(var,def_str_val);
   }
   else
   {
      str_val = str_ptr(getenv(**var));
   }

   return str_val;   
}

/////////////////////////////////////
// get_var_bool()
/////////////////////////////////////
bool
Config::get_var_bool(Cstr_ptr& var, bool def_bool_val, bool store) 
{
   if (!_instance)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_bool() - ***WARNING*** Variable [%s] accessed without an existing _instance!!", **var);
   if (!_loaded)
      err_mesg_cond(!_no_warn->contains(var), ERR_LEV_WARN, "Config::get_var_bool() - ***WARNING*** Variable [%s] accessed before a Config::load() was completed.", **var);
   
   bool bool_val;

   if (!get_var_is_set(var))
   {
      bool_val = def_bool_val;
      if (store) set_var_bool(var,def_bool_val);
   }
   else
   {
      str_ptr foo = str_ptr(getenv(**var));

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
         err_mesg(ERR_LEV_WARN, "Config::get_var_bool - ERROR! Boolean environment variable '%s' should be either 'true' or 'false'. Changing to 'false'.", **var);
         bool_val = false;
         set_var_bool(var,bool_val);
      }
   }

   return bool_val;   
}

/* end of file config.C */
