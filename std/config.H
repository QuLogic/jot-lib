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

#include "support.H"

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

   static str_list*  _no_warn;
   
 public:   
   /******** STATIC MEMBER METHODS ********/

   static bool       get_var_bool(Cstr_ptr& var, bool      def=false,     bool store=false);
   static int        get_var_int (Cstr_ptr& var, int       def=0,         bool store=false);
   static str_ptr    get_var_str (Cstr_ptr& var, Cstr_ptr& def=NULL_STR,  bool store=false);
   static double     get_var_dbl (Cstr_ptr& var, double    def=0.0,       bool store=false);

   static void       set_var_bool(Cstr_ptr& var, bool       val);
   static void       set_var_int (Cstr_ptr& var, int        val);
   static void       set_var_str (Cstr_ptr& var, Cstr_ptr&  val);
   static void       set_var_dbl (Cstr_ptr& var, double     val);

   static bool       save_config(Cstr_ptr &f) 
   { 
      return ((_instance)?(_instance->save(f)):(false)); 
   }
   static bool       load_config(Cstr_ptr &f,bool rep=true);

   static Cstr_ptr&  JOT_ROOT()           { assert(_instance); return _instance->_jot_root;  }
 
   static void       no_warn(Cstr_ptr &s) { if (!_no_warn) _no_warn = new str_list; (*_no_warn) += s; }

 protected:
   static bool       get_var_is_set(Cstr_ptr& var);
 
 protected:
   /******** MEMBER VARIABLES ********/
   str_ptr           _jot_root;
 
 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   Config(Cstr_ptr& j); 
   virtual ~Config();

 protected:
   /******** MEMBER METHODS ********/

   virtual bool      load(Cstr_ptr &) { return false;  }
   virtual bool      save(Cstr_ptr &) { return false;  }

};

#endif
