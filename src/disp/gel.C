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
#include "geom/distrib.H"
#include "std/config.H"
#include "gel_filt.H"
#include "ray.H"
#include "jot_vars.H"

using namespace mlib;

#define MAKE_HASHVAR(NAME, TYPE, VAL) hashvar<TYPE>  NAME(#NAME, VAL, 0)

GrabVar GRABBED;
MAKE_HASHVAR    (NETWORK,           int,  0);
MAKE_NET_HASHVAR(NO_COLOR_MOD,      int,  0);
MAKE_NET_HASHVAR(NO_XFORM_MOD,      int,  0);
MAKE_NET_HASHVAR(NO_DISP_MOD,       int,  0);
MAKE_NET_HASHVAR(NO_SCALE_MOD,      int,  0);
MAKE_NET_HASHVAR(NO_COPY,           int,  0);
MAKE_NET_HASHVAR(NO_EXTNETWORK,     int,  0);
MAKE_NET_HASHVAR(PICKABLE,          int,  1);
MAKE_NET_HASHVAR(NO_SAVE,           int,  0);
MAKE_NET_HASHVAR(NO_CONSTRAINT_MOD, int,  0);
MAKE_NET_HASHVAR(CONSTRAINT_VECTOR, Wvec, Wvec(0,1,0));
MAKE_NET_HASHVAR(CONSTRAINT_POINT,  Wpt,  Wpt (0,0,0));

const string GEL::_name("work'er");
TAGlist  *GEL::_gel_tags = nullptr;

// Static constructors can be the spawn of satan.
// We don't know when someone will reference distobs_list or existobs_list,
// so we have to protect by making the class allocate these objects as
// soon as they are referenced (during static initialization).
GELdistobs_list *GELdistobs::_dist_obs = nullptr;
EXISTobs_list   *EXISTobs::_exist_obs  = nullptr;
DUPobs_list     *DUPobs::_dup_obs      = nullptr;
GRABobs_list    *GRABobs::_grab_obs    = nullptr;
SAVEobs_list    *SAVEobs::_save_obs    = nullptr;
SAVEobs_list    *SAVEobs::_presave_obs = nullptr;
SAVEobs_list    *SAVEobs::_postsave_obs= nullptr;
LOADobs_list    *LOADobs::_load_obs    = nullptr;
LOADobs_list    *LOADobs::_preload_obs = nullptr;
LOADobs_list    *LOADobs::_postload_obs= nullptr;
HASHobs_list    *HASHobs::_hash_obs_list=nullptr;
JOTvar_obs_list *JOTvar_obs::_jot_var_obs_list = nullptr;


GEL::GEL():DATA_ITEM(1) 
{ 
  // XXX -- we hate this, potential memory leak
  REFlock lock(this);
  if (EXIST.allocated())
    EXIST.add(this); 
}

GEL::GEL(CGELptr &oldg)
{
   // XXX -- we hate this, potential memory leak
   REFlock lock(this);
   if (EXIST.allocated())
      EXIST.add(this); 
   DUPobs::notify_dup_obs(oldg, this);
}

GEL::~GEL()
{
   static bool debug =
      Config::get_var_bool("JOT_REPORT_GEL_DESTRUCTOR",false,true);
   if (debug)
      cerr << "~GEL" << endl;
}

RAYhit &
GEL::intersect(
   RAYhit    &r,
   CWtransf  &,
   int        
   ) const
{
   return r; 
}

RAYnear &
GEL::nearest  (
   RAYnear &r,
   CWtransf   &
   ) const 
{
   return r; 
}

STDdstream &
operator>>(STDdstream &ds, GELptr &p) 
{
   DATA_ITEM *d = DATA_ITEM::Decode(ds);
   if (d && GEL::isa(d))
      p = (GEL *)d;
   else  {
      cerr << "operator >> Couldn't find GEL in stream" << endl;
      p = nullptr;
   }
      
   return ds;
}

STDdstream &
operator<<(STDdstream &d, const GEL *p) 
{
   return d << p->name();
}

static const bool debug = Config::get_var_bool("DEBUG_SCHEDULER",false);

int
SCHEDULER::tick(void) 
{
   // This deals with the possibility that inside of a tick, one or
   // more objects may be unscheduled.  Thus, we can't remove them
   // from the list until the end of the tick.
   _ticking = 1;
   for (int i=_scheduled.num()-1; i>=0; i--) {
      if (!_unscheduled.contains(_scheduled[i])) {
         if (_scheduled[i]->tick() == -1) {
            unschedule(_scheduled[i]);
         }
      }
   }
   _ticking = 0; 

   while (!_unscheduled.empty()) {
      unschedule(_unscheduled.pop());
   }

   return _scheduled.empty() ? -1 : 1;
}

bool 
SCHEDULER::is_scheduled(CFRAMEobsptr& o) const
{
   return get_index(o) >= 0;
}

int
SCHEDULER::get_index(CFRAMEobsptr& o) const
{
   // XXX - 
   //
   //   The stored index value is incorrect because SCHEDULER
   //   code has bugs that don't keep the value up to date...
   //
   //   Use the "slow" way of accessing the index instead:
   return _scheduled.get_index(o);
}

void
SCHEDULER::schedule(CFRAMEobsptr &o)  
{
   if (_unscheduled.contains(o)) {
      if (debug) {
         cerr << "SCHEDULER::schedule: object " << o
              << " was in unscheduled list" << endl;
      }
      _unscheduled -= o;
   }
   if (is_scheduled(o)) {
      if (debug) {
         cerr << "SCHEDULER::schedule: object " << o
              << " was already scheduled " << endl;
      }
   } else {
      // not currently scheduled, so schedule it:
      _scheduled += o;
      if (debug) {
         cerr << "SCHEDULER::schedule: scheduled object " << o << endl;
      }
   }
   if (debug) {
      cerr << "scheduled list: " << endl
           << _scheduled << endl;
   }
}

void
SCHEDULER::unschedule(CFRAMEobsptr &o)  
{ 
   // do nothing if nothing to do:
   if (!is_scheduled(o))
      return;

   // If we're ticking, we can't remove it.  
   // We put it in the list, to be removed later.
   if (_ticking) {
      _unscheduled.add_uniquely(o);
      return;
   }

   // do it:
   int idx = get_index(o);
   assert(idx >= 0);
   // setIndex() must be done here in case the remove() call destroys o
   ((FRAMEobsptr&)o)->setIndex(-1);
   _scheduled.remove(idx);
   if (_scheduled.valid_index(idx))
      _scheduled[idx]->setIndex(idx);
}


GELlist 
GELlist::filter(GELFILT& f) const
{
   GELlist ret;
   for (int i=0; i<_num; i++)
      if (f.accept(_array[i]))
         ret += _array[i];
   return ret;
}

// ---------------- Define the DrawnLists's functions 

DrawnList  DRAWN;

void    
DrawnList::add(
   CGELptr &g
   )
{
   if (g) {
      if (buffering())
         _dispq.push_back(DispOp(g, true)); // Displayed
      else {
         GELlist::add_uniquely(g);
         DISPobs::notify_disp_obs(g, 1);
      }
   }
}

void    
DrawnList::rem(
   CGELptr &g
   )
{
   if (g) {
      if (buffering())
         _dispq.push_back(DispOp(g, 0)); // Undisplay
      else {
         GELlist::rem(g);
         DISPobs::notify_disp_obs(g, 0);
      }
   }
}

GELptr
DrawnList::lookup(
   const string &s
   ) const
{
   for (int i = 0; i < num(); i++)
      if (s == (*this)[i]->name())
          return (*this)[i];
   return GELptr();
}

void
DrawnList::flush()
{
   _buffering = 0;
   vector<DispOp>::size_type i;
   for (i = 0; i < _dispq.size(); i++) {
      if (_dispq[i]._op) add(_dispq[i]._gel);
      else               rem(_dispq[i]._gel);
   }
   _dispq.clear();
}

// ---------------- Define the ExistList's functions 

ExistList  EXIST;

void    
ExistList::add(
   CGELptr &g
   )
{
   if (g)
      GELlist::add_uniquely(g);
}

void    
ExistList::rem(
   CGELptr &g
   )
{
   if (g)
      GELlist::rem(g);
}

GELptr
ExistList::lookup(
   const string &s
   ) const
{
   for (int i = 0; i < num(); i++)
      if (s == (*this)[i]->name())
          return (*this)[i];
   return GELptr();
}

//
// Gets a unique name for a duplicate of an object
//
string
ExistList::unique_dupname(
   const string &buff
   ) const
{
   static char nbuff[255];
   int count = 0;

   sprintf(nbuff, "C%d_%s", count++, buff.c_str());

   while (lookup(nbuff))
      sprintf(nbuff, "C%d_%s", count++, buff.c_str());

   return nbuff;
}

//
// Gets a unique name for an object
//
string
ExistList::unique_name(
   const string &pref
   ) const
{
   static char nbuff[255];
   static char buff [255];
   int count = 1;
   gethostname(buff, 255);

   sprintf(nbuff, "%s_%d(%s)", pref.c_str(), count++, buff);

   while (lookup(nbuff))
      sprintf(nbuff, "%s_%d(%s)", pref.c_str(), count++, buff);

   return nbuff;
}

/* ---- DISPobs routines----- */

DISPobs_list DISPobs::_all_disp;
map<CGELptr,DISPobs_list*> DISPobs::_hash_disp;
int          DISPobs::_suspend_disp = 0;

/* -------------------------------------------------------------
 *
 * DISPobs
 *
 *   This class provides callbacks when an object's displayed or
 * undisplayed.
 *
 * ------------------------------------------------------------- */
void
DISPobs::notify_disp_obs(
   CGELptr &g, 
   int      disp
   ) 
{
   if (_suspend_disp || WORLD::is_over())
       return;
   DISPobs_list::iterator i;
   // Notify observers who want to know about all undisplays
   for (i = _all_disp.begin(); i != _all_disp.end(); ++i)
      (*i)->notify(g, disp);

   // Notify observers who want to know about undisplays of this object
   CDISPobs_list obj_obs = disp_obs_list((GEL *)&*g);
   for (i = obj_obs.begin(); i != obj_obs.end(); ++i)
      (*i)->notify(g, disp);
}


void 
SAVEobs::notify_save_obs(
   NetStream &s, 
   save_status_t &status,
   bool to_file, 
   bool full_scene) 
{
   SAVEobs_list::iterator i;
   distrib();
   status = SAVE_ERROR_NONE;
   for (i=presaveobs_list()->begin(); i!=presaveobs_list()->end(); ++i)
      (*i)->notify_presave(s, status, to_file, full_scene);
   for (i=saveobs_list()->begin(); i!=saveobs_list()->end(); ++i)
      (*i)->notify_save(s, status, to_file, full_scene);
   for (i=postsaveobs_list()->begin(); i!=postsaveobs_list()->end(); ++i)
      (*i)->notify_postsave(s, status, to_file, full_scene);
}

void
LOADobs::notify_load_obs(
   NetStream &s,
   load_status_t &status,
   bool to_file,
   bool full_scene )
{
   LOADobs_list::iterator i;
   distrib();
   status = LOAD_ERROR_NONE;
   for (i=preloadobs_list()->begin(); i!=preloadobs_list()->end(); ++i)
      (*i)->notify_preload(s, status, to_file, full_scene);
   for (i=loadobs_list()->begin(); i!=loadobs_list()->end(); ++i)
      (*i)->notify_load(s, status, to_file, full_scene);
   for (i=postloadobs_list()->begin(); i!=postloadobs_list()->end(); ++i)
      (*i)->notify_postload(s, status, to_file, full_scene);
}
