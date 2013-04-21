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

#define CGELdistobs_list  const GELdistobs_list
#define CGELdistobs       const GELdistobs
class   GELdistobs;
typedef ARRAY<GELdistobs *> GELdistobs_list;
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
                                     for (int i=0; i<distobs_list()->num(); i++)
                                       (*_dist_obs)[i]->notify_distrib(d,gel); }
   virtual void notify_distrib    (STDdstream &d, CGELptr &gel) = 0;

           void distrib_obs  ()   { distobs_list()->add_uniquely(this);}
           void unobs_distrib()   { distobs_list()->rem         (this);}
};


#define CEXISTobs_list const EXISTobs_list
#define CEXISTobs      const EXISTobs
class   EXISTobs;
typedef ARRAYptrs<EXISTobs*> EXISTobs_list;
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
                                          if (!_exist_obs) return; int i;
                                          for (i=0;i<existobs_list()->num();i++)
                                            (*_exist_obs)[i]->notify_exist(o,f); }
   virtual void notify_exist   (CGELptr &, int flag) = 0;
   
           void exist_obs()           { existobs_list()->add_uniquely(this); }
           void unobs_exist()         { existobs_list()->rem         (this); }
};

#define CDISPobs_list const DISPobs_list
#define CDISPobs      const DISPobs
class DISPobs;
typedef ARRAYptrs<DISPobs*> DISPobs_list;
//--------------------------------------------
// DISPobs -
//   An object that can be notified when some
// other object is transformed
//--------------------------------------------
class DISPobs {
    static  HASH         _hash_disp;
    static  int          _suspend_disp;
    static  DISPobs_list _all_disp;
  public:
   virtual ~DISPobs() {}
   virtual void notify(CGELptr &g, int) = 0;

   static   void notify_disp_obs(CGELptr &g, int disp);

   /* ---  object xform observer --- */
   void        disp_obs     (CGELptr &g) { disp_obs_list(g).add_uniquely(this);}
   void        unobs_display(CGELptr &g) { disp_obs_list(g).rem(this); }
   void        disp_obs     ()           { _all_disp.add_uniquely(this); }
   void        unobs_display()           { _all_disp.rem(this); }
   static void suspend_disp_obs()        { _suspend_disp = 1; }
   static void activate_disp_obs()       { _suspend_disp = 0; }
   protected :
      static DISPobs_list &disp_obs_list(CGELptr &g)  {
         DISPobs_list *list = (DISPobs_list *) _hash_disp.find((long ) &*g);
         if (!list)
            _hash_disp.add((long)&* g, (void *)(list = new DISPobs_list));
         return *list;
      }
};

#define CDUPobs_list const DUPobs_list
#define CDUPobs      const DUPobs
class   DUPobs;
typedef ARRAYptrs<DUPobs*> DUPobs_list;
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
   static  void notify_dup_obs(CGELptr &o, CGELptr &newg)
      {if (!_dup_obs) return;
       for (int i=0;i<_dup_obs->num();i++) (*_dup_obs)[i]->notify_dup(o,newg);}
   virtual void notify_dup    (CGELptr &old, CGELptr &newg) = 0;
   
           void dup_obs()           { dupobs_list()->add_uniquely(this); }
           void unobs_dup()         { dupobs_list()->rem         (this); }
};


#define CGRABobs_list const GRABobs_list
#define CGRABobs      const GRABobs
class GRABobs;
typedef ARRAYptrs<GRABobs*> GRABobs_list;
//--------------------------------------------
// GRABobs -
//   An object that can be notified when some
// other object is grabbed
//--------------------------------------------
class GRABobs {
      static  GRABobs_list *_grab_obs;
      static GRABobs_list *grabobs_list() { if (!_grab_obs) 
                                               _grab_obs=new GRABobs_list;
                                            return _grab_obs; }
   public:
      virtual ~GRABobs() {}
      virtual void notify_grab    (CGELptr &,  int flag) = 0;
      static  void notify_grab_obs(CGELptr &g, int flag) {
                                        if (!_grab_obs) return;  int i;
                                        for (i=0; i<grabobs_list()->num(); i++)
                                          (*_grab_obs)[i]->notify_grab(g,flag);}

   /* ---  object xform observer --- */
      void   grab_obs  ()            { grabobs_list()->add_uniquely(this); }
      void   unobs_grab()            { grabobs_list()->rem(this); }
};


#define CSAVEobs_list const SAVEobs_list
#define CSAVEobs      const SAVEobs
class SAVEobs;
typedef ARRAYptrs<SAVEobs*> SAVEobs_list;
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
   virtual void notify_presave (NetStream &s, save_status_t &status, bool to_file, bool full_scene) {}
   virtual void notify_postsave(NetStream &s, save_status_t &status, bool to_file, bool full_scene) {}
   virtual void notify_save    (NetStream &s, save_status_t &status, bool to_file, bool full_scene) {}

   static  void notify_save_obs(NetStream &s, save_status_t &status, bool to_file, bool full_scene); 

   /* ---  object save observer --- */
   void   presave_obs  ()  { presaveobs_list()->add_uniquely(this); }
   void   postsave_obs  () { postsaveobs_list()->add_uniquely(this); }
   void   save_obs  ()     { saveobs_list()->add_uniquely(this); }
   void   unobs_save()     { saveobs_list()->rem(this); }
   void   unobs_presave()  { presaveobs_list()->rem(this); }
   void   unobs_postsave() { postsaveobs_list()->rem(this); }
};


#define CLOADobs_list const LOADobs_list
#define CLOADobs      const LOADobs
class LOADobs;
typedef ARRAYptrs<LOADobs*> LOADobs_list;
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
   virtual void notify_preload (NetStream &s, load_status_t &status, bool from_file, bool full_scene) {}
   virtual void notify_postload(NetStream &s, load_status_t &status, bool from_file, bool full_scene) {}
   virtual void notify_load    (NetStream &s, load_status_t &status, bool from_file, bool full_scene) {}
      
   static  void notify_load_obs(NetStream &s, load_status_t &status, bool to_file, bool full_scene);

   /* ---  object load observer --- */
   void   preload_obs ()        { preloadobs_list()->add_uniquely(this); }
   void   postload_obs()        { postloadobs_list()->add_uniquely(this); }
   void   load_obs    ()        { loadobs_list()->add_uniquely(this); }
   void   unobs_load  ()        { loadobs_list()->rem(this); }
   void   unobs_preload()       { preloadobs_list()->rem(this); }
   void   unobs_postload()      { postloadobs_list()->rem(this); }
};

#endif // GEL_OBS_H_HAS_BEEN_INCLUDED

// end of file gel_obs.H
