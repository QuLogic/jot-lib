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
#ifndef FILE_SELECT_H_INCLUDED
#define FILE_SELECT_H_INCLUDED

#include "std/support.H"
#include "std/config.H"

/*****************************************************************
 * FileSelect
 *****************************************************************/

class FileSelect
{
 public :    
   /******** ENUMS ********/
   enum icon_t {
      NO_ICON = -1,
      LOAD_ICON = 0,
      SAVE_ICON,
      DISC_ICON,
      JOT_ICON,
      ICON_NUM
   };

   enum action_t {
      OK_ACTION = 0,
      CANCEL_ACTION
   };


 public :    
   /******** DATA TYPES ********/
   typedef void (*file_cb_t) (void *,int,int,str_ptr,str_ptr);

 protected:
   /******** STATIC MEMBER VARIABLES ********/

 public :    
   /******** STATIC MEMBER METHODS ********/

 protected:
   /******** MEMBER VARIABLES ********/
     
   str_ptr     _title;
   str_ptr     _action;
   icon_t      _icon;
   str_ptr     _path;
   str_ptr     _file;
   int         _filter;
   str_list    _filters;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public  :
   FileSelect() : _title(""), _action("OK"), _icon(NO_ICON), _path(Config::JOT_ROOT()), _file("")
   { 
      _filter = 0;
      _filters += "*";
   }
     virtual ~FileSelect() {}
   
   /******** MEMBER METHODS ********/

   virtual bool      set_title(Cstr_ptr &s)  { if (is_displaying()) return false; _title = s;   return true; }
   virtual bool      set_action(Cstr_ptr &s) { if (is_displaying()) return false; _action = s;  return true; }
   virtual bool      set_icon(icon_t i)      { if (is_displaying()) return false; _icon = i;    return true; }
   virtual bool      set_path(Cstr_ptr &s)   { if (is_displaying()) return false; _path = s;    return true; }
   virtual bool      set_file(Cstr_ptr &s)   { if (is_displaying()) return false; _file = s;    return true; }
   virtual bool      set_filter(Cstr_ptr &s) { if (is_displaying()) return false; add_filter(s); _filter = _filters.get_index(s); return true; }
   virtual bool      add_filter(Cstr_ptr &s) { if (is_displaying()) return false; _filters.add_uniquely(s);    return true; }
   
   virtual bool      clear_title()           { if (is_displaying()) return false; _title = "";                       return true; }
   virtual bool      clear_action()          { if (is_displaying()) return false; _action = "OK";                    return true; }
   virtual bool      clear_icon()            { if (is_displaying()) return false; _icon = NO_ICON;                   return true; }
   virtual bool      clear_path()            { if (is_displaying()) return false; _path = Config::JOT_ROOT();        return true; }
   virtual bool      clear_file()            { if (is_displaying()) return false; _file = "";                        return true; }
   virtual bool      clear_filter()          { if (is_displaying()) return false; set_filter("*");                   return true; }
   virtual bool      clear_filters()         { if (is_displaying()) return false; _filters.clear(); set_filter("*"); return true; }

   virtual str_ptr   get_title()             { return _title;              }
   virtual str_ptr   get_action()            { return _action;             }
   virtual icon_t    get_icon()              { return _icon;               }
   virtual str_ptr   get_path()              { return _path;               }
   virtual str_ptr   get_file()              { return _file;               }
   virtual str_ptr   get_filter()            { return _filters[_filter];   }
   virtual str_list  get_filters()           { return _filters;            }

   /******** PURE VIRTUAL METHODS ********/
 public:
   virtual bool      is_displaying() = 0;
   virtual bool      display(bool blocking, file_cb_t cb, void *vp, int idx) = 0;
 protected:
   virtual bool      undisplay(int button, str_ptr path, str_ptr file) = 0;

};

#endif
