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
#ifndef GEL_H_IS_INCLUDED
#define GEL_H_IS_INCLUDED

#include "std/support.H"
#include "net/data_item.H"
#include "net/stream.H"
#include "mlib/points.H"
#include "disp/bbox.H"
#include "disp/ref_img_client.H"


class   RAYhit;
#define CRAYhit const RAYhit
class   RAYnear;
#define CRAYnear const RAYnear
class VIEWptr;
#define CVIEWptr  const VIEWptr
class VIEW;


#define CFRAMEobslist const FRAMEobslist
#define CFRAMEobsptr  const FRAMEobsptr
//--------------------------------------------
// FRAMEobs, FRAMEobslist  -
//   An object that can be notified when each
// frame occurs.  The return value of the tick()
// function determines whether the observer
// should be put back onto the observing queue
// or not.  A value of -1 indicates that the
// observer should be removed from the queue.
//--------------------------------------------
MAKE_PTR_BASEC(FRAMEobs);
class FRAMEobs: public REFcounter {
   protected:
      // index of this FRAMEobs in
      // SCHEDULER::_scheduled list (below):
      int _index;      

   public :
      FRAMEobs() : _index(-1) {}
      virtual     ~FRAMEobs()        { }

      virtual int  tick()            { return -1; }
      virtual void setIndex(int idx) { _index = idx; }
      virtual int  getIndex() const  { return _index; }
};

typedef LIST<FRAMEobsptr> FRAMEobslist;


#define CSCHEDULERptr const SCHEDULERptr
#define CSCHEDULER    const SCHEDULER
//---------------------------------------------
// SCHEDULER, SCHEDULEptr
// This class is a subclass of GEL, which implements calling the
// GEL::tick() methods on objects that have been schedule()'d on it.
//
// Usually the WORLD::tick() function is called from a window system
// callback, and all the other schedulers and GEL objects are called
// from there in a cascade of tick()'s...
//---------------------------------------------
MAKE_PTR_SUBC(SCHEDULER, FRAMEobs);
class SCHEDULER: public FRAMEobs {
   protected:
      FRAMEobslist  _scheduled;  // List of scheduled objects
      FRAMEobslist  _unscheduled;// list of objects to unschedule 
                                 //   at the end of this tick
      bool          _ticking;    // currently ticking

   public :
   SCHEDULER() : _ticking(0) { }
   virtual ~SCHEDULER() {}

   virtual int tick(void);

   bool is_scheduled(CFRAMEobsptr& o) const;
   int  get_index   (CFRAMEobsptr& o) const;

   virtual void    schedule  (CFRAMEobsptr& o);
   virtual void    unschedule(CFRAMEobsptr& o);

   static  STAT_STR_RET static_name() {RET_STAT_STR("SCHEDULER");} 
   virtual STAT_STR_RET class_name ()          const{ return static_name(); }
   virtual int          is_of_type (Cstr_ptr &n)const{ return IS (n);}
   static  int          isa        (CSCHEDULERptr &o){ return ISA(o); }
};

// This defines the GELptr class and the auxiliary GELsubc class
// that is used for subclassing GELptr's.
MAKE_PTR_BASEC(GEL);
typedef const GELptr CGELptr;

// -------------------------------------------------------------
/*! GEL, GELptr:
 *  The virtual base class for "graphic elements" that can be 
 *  displayed in a view.   These objects are intended to be 
 *  lightweight, ref-counted geometric elements that support
 *  a minimal but important interface including methods for:
 *   intersection, nearest, transformations, naming, serializing,
 *   and drawing
 */
// -------------------------------------------------------------
class GEL: public REFcounter, 
           public DATA_ITEM,
           public RefImageClient 
{ 
 public:  
   GEL();
   virtual ~GEL();

   DEFINE_RTTI_METHODS3("GEL", GEL*, DATA_ITEM, CDATA_ITEM*);

   virtual CTAGlist& tags() const {
      return _gel_tags ? *_gel_tags : *(_gel_tags = new TAGlist);
   }
   virtual RAYhit& intersect
   (RAYhit  &r,mlib::CWtransf&m=mlib::Identity,int uv=0)const;
   virtual RAYnear& nearest
   (RAYnear &r,mlib::CWtransf&m=mlib::Identity)         const;
   virtual bool inside(mlib::CXYpt_list&)const  { return false; }
   virtual bool needs_blend()           const   { return false; }
   virtual BBOX bbox(int =0)            const   { return BBOX(); }
   virtual bool cull(const VIEW *)      const   { return 0; }
   virtual int  draw(CVIEWptr &)                { return 0; }

   virtual bool is_3D()                 const   { return false; }
   virtual bool can_do_halo()           const   { return false; }
   virtual bool do_viewall()            const   { return true; }

   virtual Cstr_ptr&  name ()           const   { return _name; }

   virtual ostream&   print(ostream &s) const   { return s; }
   virtual DATA_ITEM* dup  ()           const = 0;

 private:
   static Cstr_ptr _name;
 protected:
   GEL(CGELptr &g);
   static TAGlist* _gel_tags;
};

STDdstream     &operator>>(STDdstream &d,      GELptr &p);
STDdstream     &operator<<(STDdstream &d,const GEL    *p);
inline ostream &operator<<(ostream    &d,const GEL *p) {return p?p->print(d):d;}

/*****************************************************************
 * GEL_list:
 *
 *    A templated list class to be used with GELs, 
 *    or with types derived from GEL.
 *****************************************************************/
template <class T>
class GEL_list : public RIC_list<T> { 
public :
  //******** MANAGERS ********
  GEL_list(const RIC_list<T>& l) : RIC_list<T>(l) {}
  GEL_list(int n=0)              : RIC_list<T>(n) {}
  
  //******** CONVENIENCE ********
  bool cull(const VIEW * v) const{
    bool ret = true;
    for (int i=0; i<num(); i++)
      ret = ret && (*this)[i]->cull(v);
    return ret;    
  }

  int draw(CVIEWptr& v) const {
    int ret = 0;
    for (int i=0; i<num(); i++)
      ret += (*this)[i]->draw(v);
    return ret;
  }

   BBOX bbox() const {
      BBOX ret;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->bbox();
      return ret;
   }

   // return true if any need blend
   bool needs_blend() const {
      for (int i=0; i<num(); i++)
         if ((*this)[i]->needs_blend())
            return true;
      return false;
   }

   // defined in ray.H to placate VS .NET compiler:
   RAYhit& intersect(RAYhit& r, mlib::CWtransf& m=mlib::Identity) const;

  using RIC_list<T>::num;
};

/*****************************************************************
 * GELlist:
 *
 *    Instantiation of above, for GELs.
 *****************************************************************/
class GELFILT;
class GELlist;
typedef const GELlist CGELlist;
class GELlist : public GEL_list<GELptr> {
 public :
   //******** MANAGERS ********
   GELlist(const GEL_list<GELptr>& l) : GEL_list<GELptr>(l) {}
   GELlist(int n=16)                  : GEL_list<GELptr>(n) {}
   GELlist(CGELptr &g) { add(g); }
   GELlist(GEL     *g) { add(g); }

   // Returns the subset of this list made up of GELs 
   // that are accepted by the given filter:
   GELlist filter(GELFILT&) const;        
};

// ------- define additional variables

class DrawnList : public GELlist {   
   protected :
     class DispOp {
        public:
           GELptr _gel;
           bool   _op;
           DispOp(CGELptr &g, bool op = true) : _gel(g), _op(op) {}
           DispOp() : _op(false) {}
           int operator==(const DispOp &c) const {
              return _gel == c._gel && _op == c._op;
           }
     };
     ARRAY<DispOp> _dispq;
     bool          _buffering;
   public :
            DrawnList() { }
    void    add   (CGELptr &g);
    void    rem   (CGELptr &g);
    GELptr  lookup(Cstr_ptr &n) const;

   virtual void buffer()      { _buffering = 1;}
   virtual int  buffering()   { return _buffering; }
   virtual void flush();
};

class ExistList : public GELlist {   
   public :
            ExistList() { }
    void    add           (CGELptr &g);
    void    rem           (CGELptr &g);
    GELptr  lookup        (Cstr_ptr &n) const;
    str_ptr unique_dupname(Cstr_ptr &) const;
    str_ptr unique_name   (Cstr_ptr &) const;
    bool    allocated() const { return _array != 0; }
};

extern ExistList    EXIST;
extern DrawnList    DRAWN;

// -------- define observers for GEL operations
#include "disp/gel_obs.H" 
#include "disp/hash_types.H"

// -------- define hash variables for GEL objects
extern GrabVar      GRABBED;
extern hashvar<int> NO_COLOR_MOD, NO_XFORM_MOD, NO_DISP_MOD, NO_COPY,
                              PICKABLE,
                              NETWORK,       // list of networked objects
                              NO_CONSTRAINT_MOD,
                              NO_SCALE_MOD,  // Don't scale object
                              NO_EXTNETWORK, // Don't do external geometry 
                                             // for this obj
                              NO_SAVE;       // Network, but don't send whole
                                             // object
extern hashvar<mlib::Wpt>  CONSTRAINT_POINT;
extern hashvar<mlib::Wvec> CONSTRAINT_VECTOR;

#endif // GEL_H_IS_INCLUDED


