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
#ifndef GEL_OBS_H_HAS_BEEN_INCLUDED
#define GEL_OBS_H_HAS_BEEN_INCLUDED

#include "disp/gel.H"
#include <map>
#include <set>

#define CGELdistobs_list  const GELdistobs_list
#define CGELdistobs       const GELdistobs
class   GELdistobs;
typedef set<GELdistobs *> GELdistobs_list;
//--------------------------------------------
// GELdistobs -
//   An object that can be notified when some
// other object is distributed over the network
//--------------------------------------------
class GELdistobs {
   protected:
      // needs to be a pointer, see gel.C
      static   GELdistobs_list  *_dist_obs;
      static GELdistobs_list *distobs_list() { if (!_dist_obs)
                            _dist_obs = new GELdistobs_list; return _dist_obs; }
   public:
   virtual ~GELdistobs() {}
   static  void notify_distrib_obs(STDdstream &d, CGELptr &gel) {
                                     GELdistobs_list::iterator i;
                                     for (i=distobs_list()->begin(); i!=distobs_list()->end(); ++i)
                                       (*i)->notify_distrib(d,gel);
                                   }
   virtual void notify_distrib    (STDdstream &d, CGELptr &gel) = 0;

           void distrib_obs  ()   { distobs_list()->insert(this);}
           void unobs_distrib()   { distobs_list()->erase (this);}
};


#define CEXISTobs_list const EXISTobs_list
#define CEXISTobs      const EXISTobs
class   EXISTobs;
typedef set<EXISTobs*> EXISTobs_list;
//--------------------------------------------
// EXISTobs -
//   An object that can be notified when some
// other object is destroyed or created
//--------------------------------------------
class EXISTobs {
   protected :
   static  GELlist         _created;
   // needs to be a pointer, see gel.C
   static  EXISTobs_list  *_exist_obs;
   static EXISTobs_list *existobs_list()  { if (!_exist_obs) 
                                               _exist_obs = new EXISTobs_list;
                                            return _exist_obs; }
   public:
   virtual ~EXISTobs() {}
   static  void notify_exist_obs(CGELptr &o, int f){
                                   if (!_exist_obs) return;
                                   EXISTobs_list::iterator i;
                                   for (i=existobs_list()->begin(); i!=existobs_list()->end(); ++i)
                                     (*i)->notify_exist(o,f);
                                 }
   virtual void notify_exist   (CGELptr &, int flag) = 0;
   
           void exist_obs()           { existobs_list()->insert(this); }
           void unobs_exist()         { existobs_list()->erase (this); }
};

#define CDISPobs_list const DISPobs_list
#define CDISPobs      const DISPobs
class DISPobs;
typedef set<DISPobs*> DISPobs_list;
//--------------------------------------------
// DISPobs -
//   An object that can be notified when some
// other object is transformed
//--------------------------------------------
class DISPobs {
    static  map<CGELptr,DISPobs_list*> _hash_disp;
    static  int          _suspend_disp;
    static  DISPobs_list _all_disp;
  public:
   virtual ~DISPobs() {}
   virtual void notify(CGELptr &g, int) = 0;

   static   void notify_disp_obs(CGELptr &g, int disp);

   /* ---  object xform observer --- */
   void        disp_obs     (CGELptr &g) { disp_obs_list(g).insert(this);}
   void        unobs_display(CGELptr &g) { disp_obs_list(g).erase(this); }
   void        disp_obs     ()           { _all_disp.insert(this); }
   void        unobs_display()           { _all_disp.erase(this); }
   static void suspend_disp_obs()        { _suspend_disp = 1; }
   static void activate_disp_obs()       { _suspend_disp = 0; }
   protected :
      static DISPobs_list &disp_obs_list(CGELptr &g) {
         DISPobs_list *list;
         map<CGELptr,DISPobs_list*>::iterator it = _hash_disp.find(g);
         if (it == _hash_disp.end()) {
           list = new DISPobs_list;
           _hash_disp[g] = list;
         } else
           list = it->second;
         return *list;
      }
};

#define CDUPobs_list const DUPobs_list
#define CDUPobs      const DUPobs
class   DUPobs;
typedef set<DUPobs*> DUPobs_list;
//--------------------------------------------
// DUPobs -
//   An object that can be notified when some
// other object is duplicated
//--------------------------------------------
class DUPobs {
   protected :
   // needs to be a pointer, see gel.C
   static  DUPobs_list  *_dup_obs;
   DUPobs_list *dupobs_list()  { if (!_dup_obs) _dup_obs = new DUPobs_list;
                                   return _dup_obs; }
   public:
   virtual ~DUPobs() {}
   static  void notify_dup_obs(CGELptr &o, CGELptr &newg) {
                                 if (!_dup_obs) return;
                                 DUPobs_list::iterator i;
                                 for (i=_dup_obs->begin(); i!=_dup_obs->end(); ++i)
                                   (*i)->notify_dup(o,newg);
                               }
   virtual void notify_dup    (CGELptr &old, CGELptr &newg) = 0;
   
           void dup_obs()           { dupobs_list()->insert(this); }
           void unobs_dup()         { dupobs_list()->erase (this); }
};


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

#endif // GEL_OBS_H_HAS_BEEN_INCLUDED

// end of file gel_obs.H
