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
#ifndef IO_MANAGER_H_IS_INCLUDED
#define IO_MANAGER_H_IS_INCLUDED

#include "disp/gel.H"

/*****************************************************************
 * IOManager
 *****************************************************************/
class IOManager : public SAVEobs, public LOADobs, public DATA_ITEM { 
 public:
   /******** MEMBER ENUMS ********/
   enum state_t {
      STATE_IDLE = 0,
      STATE_SCENE_LOAD,
      STATE_PARTIAL_LOAD,
      STATE_SCENE_SAVE,
      STATE_PARTIAL_SAVE
   };

   /******** CONSTRUCTOR/DESTRUCTOR ********/
   IOManager(); 
   virtual ~IOManager();

   /******** STATIC MEMBER METHODS ********/
   static void init() { instance(); }

   static IOManager*    instance() {
      if (!_instance)
         _instance = new IOManager;
      return _instance;
   }

   static void    add_state(state_t s) { instance()->_state.add(s); }
   static state_t state()           { return instance()->state_(); }
   static str_ptr basename()        { return instance()->basename_(); }
   static str_ptr cwd()             { return instance()->cwd_(); }
   static str_ptr cached_prefix()   { return instance()->cached_prefix_(); }
   static str_ptr current_prefix()  { return instance()->current_prefix_(); }
   static str_ptr load_prefix()     { return instance()->load_prefix_(); }
   static str_ptr save_prefix()     { return instance()->save_prefix_(); }

   /******** LOADobs METHODS ********/
   virtual void notify_preload (NetStream &, load_status_t &, bool, bool); 
   virtual void notify_postload(NetStream &, load_status_t &, bool, bool); 

   /******** SAVEobs METHODS ********/
   virtual void notify_presave (NetStream &, save_status_t &, bool, bool); 
   virtual void notify_postsave(NetStream &, save_status_t &, bool, bool); 

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("IOManager", CDATA_ITEM*);
   virtual DATA_ITEM*   dup() const      { return new IOManager; }
   virtual CTAGlist&    tags() const;

 protected:
   /******** MEMBER VARIABLES ********/
   ARRAY<state_t>       _state;

   str_ptr              _basename;
   str_ptr              _cached_cwd_plus_basename;

   str_ptr              _old_cwd;
   str_ptr              _old_basename;

   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist*      _io_tags;
   static IOManager*    _instance;

   /******** INTERNAL MEMBER METHODS ********/
   bool split_filename(Cstr_ptr &, str_ptr &, str_ptr &, str_ptr &);

   /******** MEMBER METHODS FOR STATIC ACCESSORS********/

   state_t state_() const { assert(_state.num()>0); return _state.last(); }
   
   str_ptr basename_() { return _basename; }
   
   str_ptr cwd_();

   str_ptr cached_prefix_();
   str_ptr current_prefix_();

   str_ptr load_prefix_();
   str_ptr save_prefix_();

   /******** I/O Methods ********/
   virtual void get_basename(TAGformat &d);
   virtual void put_basename(TAGformat &d) const;
};

#endif // IO_MANAGER_H_IS_INCLUDED

// end of file io_manager.H

