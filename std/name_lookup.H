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
#ifndef NAME_LOOKUP_H_IS_INCLUDED
#define NAME_LOOKUP_H_IS_INCLUDED

#include "std/support.H" // XXX - for str_ptr, should be removed

#include <string>
#include <map>

using namespace std;

/*****************************************************************
 * NameLookup:
 *
 *   Templated base class that lets derived types be looked up
 *   by uniquely assigned names.
 *
 *   Template parameter T is the derived type.
 *
 *****************************************************************/
template <typename T>
class NameLookup {
 public:
   //******** MANAGERS ********
   NameLookup(const string base="") {
      if (base != "")
         set_unique_name(base);
   }
   virtual ~NameLookup() { clear_name(); }

   //******** ACCESSORS ********

   // Return the name of this object:
   const string& get_name() const { return _name; }

   // Set the name of this object, using a unique name
   // constructed from the given base name:
   void set_unique_name(const string& base) {
      set_name(unique_name(base));
   }

   // Tell whether name is in use
   // (meaning the object has a valid name and can be looked up):
   bool has_name() const { return _name != ""; }

   // Clear our name and remove object from list:
   void clear_name() { set_name(""); }

   //******** STATICS ********

//   typedef map<string,T*>::const_iterator citer_t;

   // Lookup an object from its name:
   static T* lookup(const string& obj_name) {
      // #@%$!!^ compiler &^$%#@! *&$%#!!! (swear words bleeped out)
      // Why can't I declare an iterator of type map<string,T*>::iterator???
//      map<string,T*>::iterator pos = _map.find(obj_name);
//       citer_t pos = _map.find(obj_name); // doesn't work either
//       return (pos == _map.end()) ? 0 : pos->second;
      // &^%$#$%$@!!! this makes the slow part twice as %$#@!!@# slow:
      return (_map.find(obj_name) == _map.end()) ? 0 :
         _map.find(obj_name)->second; // &^%#$@@! compiler
   }

   // Return a name that can be used for an object
   // (and that is not already in use).
   // Exception: returns "" when base == "".
   static string unique_name(const string& base) {
      if (base == "")
         return base;
      string ret;
      int i = 0;
      do {
         // XXX - how to create string from int
         //       without using str_ptr?
         ret = base + **str_ptr(i++); 
      } while (lookup(ret) != 0);
      return ret;
   }

 protected:

   //******** MEMBER DATA ********

   string                       _name;  // name of this instance
   static map<string,T*>        _map;   // used in name lookups for class T
   static bool                  _debug; // for printing debug info

   //******** DEBUG ********

   // For debugging want to print the name of the derived class.
   // All derived types have member function class_name(), but
   // the compiler won't let it be called here in the base class,
   // so we're falling back on using a virtual method to return
   // the name.
   virtual str_ptr class_id() = 0;

   //******** UTILITIES ********

   // Set the name of this object;
   // myname must not already be in use:
   void set_name(const string& myname) {

      // Desired name already set? Do nothing:
      if (_name == myname) {
         // Print debug info if new name == old name
         // (and they're not the empty string):
         if (has_name() && _debug)
            cerr << class_id() << "::set_name: name \""
                 << myname << "\" already set, doing nothing..."
                 << endl;
         return;
      }

      // Already have a name?
      if (has_name()) {
         assert(lookup(_name) == this);
         if (_debug) {
            cerr << class_id() << "::set_name: existing name \""
                 << _name << "\" already set, erasing..."
                 << endl;
         }
         // Old name was in use, remove our entry:
         _map[_name] = 0;
      }

      // Error if myname is already in use:
      assert(lookup(myname) == 0);

      _name = myname;      // record our name

      // Record this object in the table,
      // unless the name is empty:
      if (_name != "") {
         _map[myname] = (T*)this; // add self to list of named objects
         assert(lookup(myname) == this);
      }

      if (_debug) {
         cerr << class_id() << "::set_name: name is now: \""
              << myname << "\"" << endl;
      }
   }
};

#endif // NAME_LOOKUP_H_IS_INCLUDED

// name_lookup.H
