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
#ifndef FLOOR_H_IS_INCLUDED
#define FLOOR_H_IS_INCLUDED

/*!
 *  \file floor.H
 *  \brief Contains the declaration of the FLOOR interactor.
 *
 *  \ingroup group_FFS
 *  \sa floor.C
 *
 */
 
#include "geom/command.H"
#include "geom/geom.H"
#include "geom/world.H"
#include "gest/draw_widget.H"
#include "manip/manip.H"
#include "map3d/map2d3d.H"
#include "mesh/bmesh.H"
#include "std/config.H"
#include "std/stop_watch.H"

/*!
 * The FLOOR is under development; the basic idea is to provide a
 * drawing plane used to help create objects, especially when starting
 * from scratch. Ideally the FLOOR should be fully interactive and let
 * you position and orient it (e.g. vertically) as you see fit.
 */

MAKE_PTR_SUBC(FLOOR,DrawWidget);
typedef const FLOORptr CFLOORptr;
class FLOOR : public DrawWidget {
 public:

   //******** MANAGERS ********

   FLOOR(CEvent& down, CEvent& move, CEvent& up);
   virtual ~FLOOR() {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("FLOOR", FLOOR*, DrawWidget, CDATA_ITEM*);
   
   //******** ACCESSORS ********

   PlaneMap* get_map()          { return _map; }

   //******** COORDINATE FRAME ********

   Wpt  o() const { return _map->map(UVpt(0,0)); }
   Wvec t() const { return _map->du(UVpt(0,0)); }
   Wvec b() const { return _map->dv(UVpt(0,0)); }
   Wvec n() const { return _map->norm(UVpt(0,0)); }

   Wplane plane() const { return Wplane(o(), n()); }

   //******** INTERACTOR METHODS ********

   virtual int down_cb             (CEvent&, State*&);
   virtual int up_cb               (CEvent&, State*&);
   virtual int resize_uniform_cb   (CEvent&, State*&);
   virtual int resize_nonuniform_cb(CEvent&, State*&);

   double sizeX() { return _sx; }
   double sizeY() { return _sy; }
   double sizeZ() { return _sz; }

   //******** STATICS ********

   static bool is_active() {
      return _instance && DRAWN.contains(_instance);
   }

   static FLOORptr lookup()      { return is_active() ? _instance : 0; }
   static BMESHptr lookup_mesh() {
      return lookup() ? lookup()->mesh() : BMESHptr(0);
   }

   //! display or undisplay the floor:
   static void show();
   static void hide();
   static void toggle();

   //! If it's active, displace the floor to be "under" the given mesh:
   static void realign(BMESH* m, MULTI_CMDptr cmd);

   //! If it's active, transform the floor
   static void transform(CWtransf& xf, MULTI_CMDptr cmd) {
      if (is_active())
         _instance->_transform(xf, cmd);
   }

   //! Added to provide a method to kill off the statically
   //! stored version of the floor during scheduled cleanup
   //! as exit(0) is called.  The order of static data cleanup
   //! is (just as with static data allocation) undefined.  On
   //! WIN32, after exit(0), the static HASH of BMESHobs was axed
   //! before the floor, so FLOOR tanks when it deletes its
   //! _instance and cannot inform its observers without referring
   //! to non-existant data.  WORLD::clean_on_exit is already setup 
   //! to axe the ref-counted pointers to all GELS (EXIST), so we must
   //! clear _instance to axe the last count and delete the floor.  
   //! This is registered with onexit() in the FLOOR constructor, 
   //! and is called when exit() occurs. Other onexit() routines 
   //! are also executed in LIFO fashion.

   //! Other widgets that hold static pointers to an instance
   //! may need to register some cleanup with onexit.
   //! This is the only reasonable way to ensure clean shutdown,
   //! as GLUT will call exit when the window is closed without
   //! providing any callbacks...!

   static void clean_on_exit() {
      if (Config::get_var_bool("DEBUG_CLEAN_ON_EXIT",false))
         cerr << "FLOOR::clean_on_exit()\n";
      _instance = 0;
   }

   // prevent warnings
   static  DATA_ITEM  *lookup(Cstr_ptr& d) { return DATA_ITEM::lookup(d); }

   static FLOOR* null;

   //******** GEOM methods ********

   virtual void    write_xform(CWtransf&, CWtransf&, CMOD &);
   virtual RAYhit &intersect  (RAYhit&, CWtransf& =Identity, int=0)const;
   virtual int     interactive(CEvent&, State*&, RAYhit* =0) const;
   virtual int     draw       (CVIEWptr &v);
   virtual BBOX    bbox       (int i=0) const;

   //******** GEL METHODS ********

   virtual DATA_ITEM* dup() const { return 0; } // there's only one

   //******** RefImageClient METHODS ********

   virtual int draw_vis_ref() {
      return should_draw() ? GEOM::draw_vis_ref() : 0;
   }

 protected:

   enum intersect_t {
      INTERSECT_NONE = 0,
      INTERSECT_SIDE,
      INTERSECT_CORNER
   };

   enum update_t {
      CLEAN = 0,
      DIRTY
   };

   //! floor description
   double _sx;
   double _sy;
   double _sz;

   // FSA states (for resizing w/ middle button)
   State        _start;
   State        _resize_uniform;
   State        _resize_nonuniform;

   intersect_t  _down_intersect;  //!< records last intersect result
   PIXEL  _last_pt;         //!< pixel location of last mouse event
   stop_watch   _clock;
   PlaneMap*    _map;

   static FLOORptr _instance;

   //******** PROTECTED METHODS ********

   //! for defining callbacks for regular events
   typedef CallMeth_t<FLOOR,Event> dcb_t;
   dcb_t* DACb(dcb_t::_method m)           { return new dcb_t(this, m); }
   dcb_t* DACb(dcb_t::_method m, State* s) { return new dcb_t(this, m, s); }

   intersect_t intersect(CPIXEL& p, Wpt& hit, Wvec& norm) const;
   intersect_t intersect(CPIXEL& p) const {
      Wpt hit; Wvec norm; return intersect(p, hit, norm);
   }

   //! Transform the floor by applying the given matrix.
   void _transform(CWtransf& xf, MULTI_CMDptr cmd);

   //! Center of floor:
   Wpt  _center() const { return _map->map(UVpt(0,0)); }

   //! Floor plane normal:
   Wvec _normal() const { return _map->norm(UVpt(0,0)); }

   //! Okay to draw FLOOR?
   bool should_draw() const;

   BMESHptr mesh() const { return BMESH::upcast(body()); }

   //******** GEOM VIRTUAL METHODS ********

   // animate camera to look straight onto floor
   virtual bool do_cam_focus(CVIEWptr& view, CRAYhit& r);

   // polygon offset:
   // the floor needs greater amounts
   virtual float po_factor() const { return 2.0; }
   virtual float po_units()  const { return 8.0; }
};

#endif // FLOOR_H_IS_INCLUDED

// end of file floor.H
