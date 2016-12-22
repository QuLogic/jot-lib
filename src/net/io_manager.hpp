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

#include "disp/gel.hpp"

#define CSAVEobs_list const SAVEobs_list
#define CSAVEobs      const SAVEobs
class SAVEobs;
typedef set<SAVEobs*> SAVEobs_list;
//--------------------------------------------
// SAVEobs -
//   An object that can be notified when some
// other object is saved
//--------------------------------------------
class SAVEobs {
 public:
   enum save_status_t {
      SAVE_ERROR_NONE=0,   //no problems
      SAVE_ERROR_STREAM,   //bad stream
      SAVE_ERROR_WRITE,    //good stream, failed writing
      SAVE_ERROR_CWD       //good stream, good write, failed changing cwd
   };

 protected:
   static SAVEobs_list *_save_obs;
   static SAVEobs_list *_presave_obs;
   static SAVEobs_list *_postsave_obs;

   static SAVEobs_list *saveobs_list()    { if (!_save_obs) _save_obs=new SAVEobs_list; return _save_obs; }
   static SAVEobs_list *presaveobs_list() { if (!_presave_obs) _presave_obs=new SAVEobs_list; return _presave_obs; }
   static SAVEobs_list *postsaveobs_list(){ if (!_postsave_obs) _postsave_obs=new SAVEobs_list; return _postsave_obs; }
 public:
   virtual ~SAVEobs() {}
   virtual void notify_presave (STDdstream &s, save_status_t &status, bool full_scene) {}
   virtual void notify_postsave(STDdstream &s, save_status_t &status, bool full_scene) {}
   virtual void notify_save    (STDdstream &s, save_status_t &status, bool full_scene) {}

   static  void notify_save_obs(STDdstream &s, save_status_t &status, bool full_scene);

   /* ---  object save observer --- */
   void   presave_obs  ()  { presaveobs_list()->insert(this); }
   void   postsave_obs  () { postsaveobs_list()->insert(this); }
   void   save_obs  ()     { saveobs_list()->insert(this); }
   void   unobs_save()     { saveobs_list()->erase(this); }
   void   unobs_presave()  { presaveobs_list()->erase(this); }
   void   unobs_postsave() { postsaveobs_list()->erase(this); }
};


#define CLOADobs_list const LOADobs_list
#define CLOADobs      const LOADobs
class LOADobs;
typedef set<LOADobs*> LOADobs_list;
//--------------------------------------------
// LOADobs -
//   An object that can be notified when some
// other object is loaded
//--------------------------------------------
class  LOADobs {
 public:
   enum load_status_t {
      LOAD_ERROR_NONE= 0,  //no problems
      LOAD_ERROR_STREAM,   //bad stream
      LOAD_ERROR_JOT,      //good stream, good header, failed conventional load
      LOAD_ERROR_CWD,      //good stream, good header, good conventional load, failed cwd change
      LOAD_ERROR_AUX,      //good stream, no header or failed conv. load, but succeeded aux load
      LOAD_ERROR_READ      //good stream, no header, failed aux load
   };
 protected:
   static LOADobs_list *_load_obs;
   static LOADobs_list *_preload_obs;
   static LOADobs_list *_postload_obs;

   static LOADobs_list *loadobs_list()    { if (!_load_obs) _load_obs=new LOADobs_list; return _load_obs; }
   static LOADobs_list *preloadobs_list() { if (!_preload_obs) _preload_obs=new LOADobs_list; return _preload_obs; }
   static LOADobs_list *postloadobs_list(){ if (!_postload_obs) _postload_obs=new LOADobs_list; return _postload_obs; }
 public:
   virtual ~LOADobs() {}
   virtual void notify_preload (STDdstream &s, load_status_t &status, bool full_scene) {}
   virtual void notify_postload(STDdstream &s, load_status_t &status, bool full_scene) {}
   virtual void notify_load    (STDdstream &s, load_status_t &status, bool full_scene) {}

   static  void notify_load_obs(STDdstream &s, load_status_t &status, bool full_scene);

   /* ---  object load observer --- */
   void   preload_obs ()        { preloadobs_list()->insert(this); }
   void   postload_obs()        { postloadobs_list()->insert(this); }
   void   load_obs    ()        { loadobs_list()->insert(this); }
   void   unobs_load  ()        { loadobs_list()->erase(this); }
   void   unobs_preload()       { preloadobs_list()->erase(this); }
   void   unobs_postload()      { postloadobs_list()->erase(this); }
};

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

   static void    add_state(state_t s) { instance()->_state.push_back(s); }
   static state_t state()           { return instance()->state_(); }
   static string  basename()        { return instance()->basename_(); }
   static string  cwd()             { return instance()->cwd_(); }
   static string  cached_prefix()   { return instance()->cached_prefix_(); }
   static string  current_prefix()  { return instance()->current_prefix_(); }
   static string  load_prefix()     { return instance()->load_prefix_(); }
   static string  save_prefix()     { return instance()->save_prefix_(); }

   /******** LOADobs METHODS ********/
   virtual void notify_preload (STDdstream &, load_status_t &, bool);
   virtual void notify_postload(STDdstream &, load_status_t &, bool);

   /******** SAVEobs METHODS ********/
   virtual void notify_presave (STDdstream &, save_status_t &, bool);
   virtual void notify_postsave(STDdstream &, save_status_t &, bool);

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("IOManager", CDATA_ITEM*);
   virtual DATA_ITEM*   dup() const      { return new IOManager; }
   virtual CTAGlist&    tags() const;

 protected:
   /******** MEMBER VARIABLES ********/
   vector<state_t>      _state;

   string               _basename;
   string               _cached_cwd_plus_basename;

   string               _old_cwd;
   string               _old_basename;

   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist*      _io_tags;
   static IOManager*    _instance;

   /******** INTERNAL MEMBER METHODS ********/
   bool split_filename(const string &, string &, string &, string &);

   /******** MEMBER METHODS FOR STATIC ACCESSORS********/

   state_t state_() const { assert(_state.size()>0); return _state.back(); }
   
   string  basename_() { return _basename; }
   
   string  cwd_();

   string  cached_prefix_();
   string  current_prefix_();

   string  load_prefix_();
   string  save_prefix_();

   /******** I/O Methods ********/
   virtual void get_basename(TAGformat &d);
   virtual void put_basename(TAGformat &d) const;
};

#endif // IO_MANAGER_H_IS_INCLUDED
