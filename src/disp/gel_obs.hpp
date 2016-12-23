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

#include "disp/gel.hpp"
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
      // needs to be a pointer, see gel.cpp
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
   // needs to be a pointer, see gel.cpp
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
   // needs to be a pointer, see gel.cpp
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


#endif // GEL_OBS_H_HAS_BEEN_INCLUDED
