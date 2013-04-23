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
   typedef void (*file_cb_t) (void *,int,int,string,string);

 protected:
   /******** STATIC MEMBER VARIABLES ********/

 public :    
   /******** STATIC MEMBER METHODS ********/

 protected:
   /******** MEMBER VARIABLES ********/
     
   string         _title;
   string         _action;
   icon_t         _icon;
   string         _path;
   string         _file;
   vector<string> _filters;
   set<string>    _filter_set;
   vector<string>::size_type _filter;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public  :
   FileSelect() : _title(""), _action("OK"), _icon(NO_ICON), _path(Config::JOT_ROOT()), _file("")
   { 
      _filters.push_back("*");
      _filter_set.insert("*");
      _filter = 0;
   }
     virtual ~FileSelect() {}
   
   /******** MEMBER METHODS ********/

   virtual bool      set_title(const string &s)  { if (is_displaying()) return false; _title = s;   return true; }
   virtual bool      set_action(const string &s) { if (is_displaying()) return false; _action = s;  return true; }
   virtual bool      set_icon(icon_t i)          { if (is_displaying()) return false; _icon = i;    return true; }
   virtual bool      set_path(const string &s)   { if (is_displaying()) return false; _path = s;    return true; }
   virtual bool      set_file(const string &s)   { if (is_displaying()) return false; _file = s;    return true; }
   virtual bool      set_filter(const string &s) { if (is_displaying()) return false;
                                                   add_filter(s);
                                                   _filter = std::find(_filters.begin(), _filters.end(), s) - _filters.begin();
                                                   return true;
                                                 }
   virtual bool      add_filter(const string &s) { if (is_displaying()) return false;
                                                   if (_filter_set.find(s) == _filter_set.end()) {
                                                   _filters.push_back(s);
                                                   _filter_set.insert(s);
                                                   }
                                                   return true;
                                                 }
   
   virtual bool      clear_title()               { if (is_displaying()) return false; _title = "";                       return true; }
   virtual bool      clear_action()              { if (is_displaying()) return false; _action = "OK";                    return true; }
   virtual bool      clear_icon()                { if (is_displaying()) return false; _icon = NO_ICON;                   return true; }
   virtual bool      clear_path()                { if (is_displaying()) return false; _path = Config::JOT_ROOT();        return true; }
   virtual bool      clear_file()                { if (is_displaying()) return false; _file = "";                        return true; }
   virtual bool      clear_filter()              { if (is_displaying()) return false; set_filter("*");                   return true; }
   virtual bool      clear_filters()             { if (is_displaying()) return false; _filters.clear(); set_filter("*"); return true; }

   virtual string         get_title()            { return _title;              }
   virtual string         get_action()           { return _action;             }
   virtual icon_t         get_icon()             { return _icon;               }
   virtual string         get_path()             { return _path;               }
   virtual string         get_file()             { return _file;               }
   virtual string         get_filter()           { return _filters[_filter];   }
   virtual vector<string> get_filters()          { return _filters;            }

   /******** PURE VIRTUAL METHODS ********/
 public:
   virtual bool      is_displaying() = 0;
   virtual bool      display(bool blocking, file_cb_t cb, void *vp, int idx) = 0;
 protected:
   virtual bool      undisplay(int button, string path, string file) = 0;

};

#endif
