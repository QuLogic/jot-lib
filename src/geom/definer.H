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
#ifndef DEFINER_H
#define DEFINER_H

#include "std/support.H"
#include "net/data_item.H"
#include "net/stream.H"
#include "mlib/points.H"
#include "mod.H"
#include "body.H"

class GEOM;
class BODY;

/* --------------------------------*/
// This file contains objects that support directed acyclic data networks.  
// 
// The overview is that data networks consist of two elemental 
// components : DATA and DEFINERs.    A third compound object, a GEOM,
// is used to contain DATA and DEFINER's.
//
//                                 GEOM 
//  --------------       --------------------------
//               |                   ^     |       
//    DATA       |       DEFINER     |     ->  DATA
//  "outputs"    ->  "inputs"  "outputs"       "outputs"  ->  ...
//
// Briefly, DATA objects are values and DEFINERs are functions that consume
// DATA to produce new DATA.  Thus, data networks are networks of 
// DATA that have "outputs lists" that point to DEFINERs which have 
// "output lists" pointing to the GEOMs whose data they "define".  (DEFINERS 
// also have "input lists" that point to the GEOMs where their input
// DATA comes from).   DATA objects do not have "input lists" since the 
// "input lists" would be equivalent to the input lists of the DATA's 
// corresponding DEFINER.   DATA's do have a GEOM that indicates that
// object that controls the DATA.
// 
//
/*----------------------------------*/



/* ------------------ class definitions --------------------- */

//-------------------------------------------------------------
//  BASIC_DATA - wrapper around data used in a dependency graph.
//  The data contains the object that controls the DATA, and a
//  a modification sequence number.
//-------------------------------------------------------------
#define CBASIC_DATA const BASIC_DATA
template<class T>
class BASIC_DATA {
  protected:
   T     _data;
   MOD   _id;
   GEOM *_geom;

  public :
               BASIC_DATA()                                        {}
               BASIC_DATA(const BASIC_DATA<T> &d):_data(d._data),_id(d._id),_geom(0) {}
   virtual    ~BASIC_DATA() {}
   const MOD  &id()   const           { return  _id; }
   MOD        &id()                   { return  _id; }
   const T    &data() const           { return  _data; }
   T          &data()                 { return  _data; }
   const GEOM *geom() const           { return  _geom; }
   GEOM *      geom()                 { return  _geom; }

   void        write(GEOM *g, T d, MOD m) { _geom = g; _data = d; _id = m; }
};


//---------------------------------------------------------------
//  DEFINER - contains a list of input and a list of output GEOMs
//  Based on the inputs, the definer computes how to modify the
//  outputs.  The definer can then write a modification delta
//  "request" to its output GEOMs.  These are requests because the
//  output GEOM may have multiple of its inputs change and will
//  itself determine how to comine all of its input deltas into a
//  final value.
//---------------------------------------------------------------
#define CDEFINER const DEFINER
class DEFINER : public DATA_ITEM {
  public :
   enum data_mask {
      XFORM               = 1,
      ORIG_BODY           = 2,
      XFORM_AND_ORIG_BODY = 3,
      CSG_BODY            = 4
   };
  protected :
   int                 _reached; 
   data_mask           _out_mask;
   BASIC_DATA<mlib::Wtransf> _xf_delt;
   BASIC_DATA<int>     _ob_delt;
   ARRAY<GEOM*>        _inputs;
   ARRAY<GEOM*>        _outputs;
   static TAGlist     *_def_tags;
  public :
           DEFINER();
           DEFINER(CDEFINER *d, GEOM *g): _reached(0), _out_mask(d->_out_mask),
                                          _inputs(d->_inputs) { set_geom(g); }
   virtual~DEFINER();
   virtual DEFINER  *copy     (GEOM *g) const   { return new DEFINER(this,g); }

   virtual CTAGlist &tags     ()        const;

   virtual void      add_input(GEOM *o);
   virtual void      rem_input(GEOM *o);

   virtual GEOM     *feat_geom()          const { return 0; }
     const GEOM     *geom     (int i = 0) const { return _outputs[i]; }
           GEOM     *geom     (int i = 0)       { return _outputs[i]; }
   virtual void      rem_geom (GEOM *g)         { _outputs -= g; }
   virtual void      add_geom (GEOM *g)         { _outputs += g; }
   // for backward compatibility
   virtual void      clear_geom()               { _outputs.clear(); }
   virtual void      set_geom (GEOM *g)         { _outputs += g; }

           void      clear_deltas()             { _xf_delt.data() = mlib::Identity;}
           void      set_delta(GEOM *g, CBODYptr&, CMOD&m)
                                                {_ob_delt.write(g,0,m);reach();}
           void      set_delta(GEOM *g, mlib::CWtransf&d,CMOD&m)
                                                {_xf_delt.write(g,
                                m == _xf_delt.id() ? d : d * _xf_delt.data(),m);
                                                  reach(); }
           mlib::Wtransf   xf_delta ()                { return _xf_delt.data(); }
CBASIC_DATA<mlib::Wtransf> &xf_delt () const          { return _xf_delt;}


           void      reach    (int r=1)         { _reached = r; }
           int       reached  ()                { return _reached; }
           int       mask     (data_mask d)     { return (int)_out_mask&(int)d;}

           int       num_outputs()   const      { return _outputs.num(); }
           int       num_inputs()    const      { return _inputs.num(); }
           data_mask out_mask()      const      { return _out_mask; }
           GEOM     *operator[](int i)          { return _inputs[i]; }
   virtual ARRAY<GEOM *> required()  const      { return ARRAY<GEOM *>(); }

   virtual void      visit     (MOD &mod);
   virtual void      print     (ostream &os)    { os << class_name(); }
   virtual DATA_ITEM*dup      ()     const      { return new DEFINER;}
   virtual void      put_outmask(TAGformat &d) const { d.id()<< (int)_out_mask;}
   virtual void      put_inputs (TAGformat &d) const;
   virtual void      get_outmask(TAGformat &d)       { *d >> (int&)_out_mask; }
   virtual void      get_inputs (TAGformat &d);

   DEFINE_RTTI_METHODS2("DEFINER", DATA_ITEM, CDEFINER *);

};

inline STDdstream &operator<<(STDdstream &ds, CDEFINER *p) 
                      { return p ? p->format(ds) : ds; }

#define CDATA const DATA
template<class T>
class DATA : public BASIC_DATA<T> {
   ARRAY<DEFINER*> _outputs;

   public :
         DATA()                                   {}
         // Don't copy _outputs because inputs corresponding to _outputs
         DATA(const DATA<T> &d):BASIC_DATA<T>(d)    {}
         ARRAY<DEFINER*> *operator->()           { return &_outputs; }
   const ARRAY<DEFINER*> *operator->()  const    { return &_outputs; }
         ARRAY<DEFINER*> &operator *()           { return  _outputs; }
   const ARRAY<DEFINER*> &operator *()  const    { return  _outputs; }
   void                   operator+=(DEFINER *d) { _outputs += d; }
   void                   operator-=(DEFINER *d) { _outputs -= d; }
   DEFINER               *operator[](int i)      { return  _outputs[i]; }

   void                   write(T d)             { data()  = d; }
   void                   write(GEOM *g, T d, MOD m)      { write(g,d,d,m); }
   void                   write(GEOM *g, T x, T d, MOD m) { data() = x; id() = m;
                                            for (int i=0; i<_outputs.num(); i++)
                                               _outputs[i]->set_delta(g,d,m); }
friend
ostream &operator << (ostream &os, const DATA<T> &d) { return os<<d.data(); }

   using BASIC_DATA<T>::id;
   using BASIC_DATA<T>::data;
};

#define CCOMPOSITE_DEF const COMPOSITE_DEF
class COMPOSITE_DEF : public DEFINER {
   public :
                COMPOSITE_DEF ()           { }
                COMPOSITE_DEF (CCOMPOSITE_DEF *d, GEOM *g):DEFINER(d,g) { }

   virtual void      visit         (MOD &mod);
   virtual void      inputs_changed(int all_eq, int any_body_mod, MOD &m) = 0;

   DEFINE_RTTI_METHODS2("COMPOSITE", DEFINER, CDEFINER *);
};

#endif

