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
#ifndef ALERT_BOX_H_INCLUDED
#define ALERT_BOX_H_INCLUDED

#include "std/support.H"

/*****************************************************************
 * AlertBox
 *****************************************************************/

class AlertBox
{
 public :    
   /******** ENUMS ********/
   enum icon_t {
      NO_ICON = -1,
      JOT_ICON = 0,
      INFO_ICON,
      QUESTION_ICON,
      EXCLAMATION_ICON,
      WARNING_ICON,
      ICON_NUM
   };

   /******** DATA TYPES ********/
   typedef void (*alert_cb_t) (void *,void *,int,int);

 protected:
   /******** STATIC MEMBER VARIABLES ********/

 public :    
   /******** STATIC MEMBER METHODS ********/

 protected:
   /******** MEMBER VARIABLES ********/
     
   str_ptr     _title;

   str_list    _text;
   str_list    _buttons;

   int         _default;

   icon_t      _icon;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public:
    AlertBox() : _title(""), _default(-1), _icon(NO_ICON) { }
    virtual ~AlertBox() {}
   
   /******** MEMBER METHODS ********/

   virtual bool      set_title(Cstr_ptr &s)  { if (is_displaying()) return false; _title = s;      return true; }
   virtual bool      set_icon(icon_t i)      { if (is_displaying()) return false; _icon = i;       return true; }
   virtual bool      set_default(int d)      { if (is_displaying()) return false; _default = d;    return true; }
   virtual bool      add_button(Cstr_ptr &s) { if (is_displaying()) return false; _buttons.add(s); return true; }
   virtual bool      add_text(Cstr_ptr &s)   { if (is_displaying()) return false; _text.add(s);    return true; }
   
   virtual bool      clear_title()           { if (is_displaying()) return false; _title = "";        return true; }
   virtual bool      clear_icon()            { if (is_displaying()) return false; _icon = NO_ICON;    return true; }
   virtual bool      clear_default()         { if (is_displaying()) return false; _default = -1;      return true; }
   virtual bool      clear_buttons()         { if (is_displaying()) return false; _buttons.clear();   return true; } 
   virtual bool      clear_text()            { if (is_displaying()) return false; _text.clear();      return true; }

   virtual str_ptr   get_title()             { return _title;     }
   virtual icon_t    get_icon()              { return _icon;      }
   virtual int       get_default()           { return _default;   }
   virtual str_list  get_buttons()           { return _buttons;   }
   virtual str_list  get_text()              { return _text;      }

   /******** PURE VIRTUAL METHODS ********/
 public:
   virtual bool      is_displaying() = 0;
   virtual bool      display(bool blocking, alert_cb_t cb, void *vp, void *vpd, int idx) = 0;
 protected:
   virtual bool      undisplay(int button) = 0;

};

#endif
