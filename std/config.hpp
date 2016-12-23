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
#ifndef CONFIGCLASS_H
#define CONFIGCLASS_H

#include "support.hpp"
#include <set>

/**********************************************************************
 * Config
 **********************************************************************/

extern void main_config(bool);

class ConfigInit {
   static int count;
 public:
   ConfigInit()   { if (count++ == 0) main_config(true);    }
   ~ConfigInit()  { if (--count == 0) main_config(false);   }
};

static ConfigInit ConfigInitInstance;

class Config {

 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static Config*    _instance;

   static bool       _replace;
   static bool       _loaded;

   static set<string> *_no_warn;
   
 public:   
   /******** STATIC MEMBER METHODS ********/

   static bool       get_var_bool(const string& var, bool          def=false, bool store=false);
   static int        get_var_int (const string& var, int           def=0,     bool store=false);
   static string     get_var_str (const string& var, const string& def="",    bool store=false);
   static double     get_var_dbl (const string& var, double        def=0.0,   bool store=false);

   static void       set_var_bool(const string& var, bool          val);
   static void       set_var_int (const string& var, int           val);
   static void       set_var_str (const string& var, const string& val);
   static void       set_var_dbl (const string& var, double        val);

   static bool       save_config(const string &f)
   { 
      return ((_instance)?(_instance->save(f)):(false)); 
   }
   static bool       load_config(const string &f, bool rep=true);

   static const string& JOT_ROOT()        { assert(_instance); return _instance->_jot_root;  }
 
   static void       no_warn(const string &s) { if (_no_warn == nullptr) _no_warn = new set<string>; _no_warn->insert(s); }

 protected:
   static bool       get_var_is_set(const string& var);
 
 protected:
   /******** MEMBER VARIABLES ********/
   string            _jot_root;
 
 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   Config(const string& j);
   virtual ~Config();

 protected:
   /******** MEMBER METHODS ********/

   virtual bool      load(const string &) { return false;  }
   virtual bool      save(const string &) { return false;  }

};

#endif

