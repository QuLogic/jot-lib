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
/*****************************************************************
 * crv_sketch.H
 *****************************************************************/
#ifndef CRV_SKETCH_H_IS_INCLUDED
#define CRV_SKETCH_H_IS_INCLUDED

/*!
 *  \file crv_sketch.H
 *  \brief Contains the declaration of the CRV_SKETCH widget.
 *
 *  \ingroup group_FFS
 *  \sa crv_sketch.C
 *
 */

#include "npr/stylized_line3d.H"
#include "tess/bcurve.H"
#include "gest/draw_widget.H"

/*****************************************************************
 * CRV_SKETCH:
 *****************************************************************/
MAKE_PTR_SUBC(CRV_SKETCH,DrawWidget);
typedef const CRV_SKETCH    CCRV_SKETCH;
typedef const CRV_SKETCHptr CCRV_SKETCHptr;

//! \brief Widget that handles oversketching of curves using the shadow and
//! axes in the projection direction as guides. (Also stereo)
class CRV_SKETCH : public DrawWidget {
 public:

   //! interaction mode for setting up constraints
   enum mode_t {
      INACTIVE,
      SHADOW,
      STEREO
   };

   //! the type of component being manipulated
   enum component_t {
      NONE,
      POINT,
      CURVE
   };

   //******** MANAGERS ********
   // no public constructor
   static CRV_SKETCHptr get_instance();
   static CRV_SKETCHptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("CRV_SKETCH", CRV_SKETCH*, DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   //! If the gesture is valid for activating the widget,
   //! activates it and returns true.
   static bool init(CGESTUREptr& g);

 protected:
   bool init_curve(CNDCpt& screen_pt, mode_t mode, int rad=10);
   bool init_point(CNDCpt& screen_pt, mode_t mode, int rad=10);
   void set_shadow_map();

   //! After a successful call to init(), this turns on a CRV_SKETCH
   //! to handle the crv_sketch session:
   static bool go(double dur = default_timeout());

 public:

   //******** DRAW FSA METHODS ********
   virtual int  stroke_cb(CGESTUREptr& gest, DrawState*&);
   virtual int  tap_cb   (CGESTUREptr& gest, DrawState*&);

   //******** DrawWidget METHODS ********

   virtual BMESHptr bmesh() const { return _curve ? _curve->mesh() : 0; }
   virtual LMESH* lmesh()   const { return _curve ? _curve->mesh() : 0; }

   virtual PIXEL_list shadow_pix() const { return _shadow_pixels; }
   virtual Bcurve* curve() const { return _curve; }
   virtual Wpt_listMap* shadow_map() const { return _shadow_map; }
   virtual ARRAY<Wvec> shadow_norms() const { return _shadow_normals; }

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr mode_name()     const { return "curve edit"; }

   //******** CAMobs Method *************

   //! Nofification that the camera just changed:
   virtual void notify(CCAMdataptr &data);

   //******** DISPobs METHODS ********

   //! Used to monitor if curve gets undisplayed:
   virtual void notify(CGELptr &g, int b) { return DrawWidget::notify(g, b); }

   //******** GEL METHODS ********
   virtual int draw(CVIEWptr& v);   

   //*****************************************************************

   // made these methods static since they are used both here
   // and in the STEREO_CRV widget.. I should probably put them
   // in a different class altogether, but this seemed like the
   // quicker solution. (probably wasn't)

   /*static bool do_match(PIXEL_list& drawnlist,
                        Wpt_list& result_list,
                        CARRAY<int>& shadow_cusps,
                        CWpt_list& shadow_pts,
                        CPIXEL_list& shadow_pixels,
                        CARRAY<Wvec>& shadow_normals);
   */

   //! similar to do_match, except that drawnlist is spliced into
   //! upperlist before calling match_spans
   static bool re_match(PIXEL_list& drawnlist,
                        CPIXEL_list& upperlist, // pixel list for existing upper curve
                        Wpt_list& result_list,
                        CARRAY<int>& shadow_cusps,
                        CWpt_list& shadow_pts,
                        CPIXEL_list& shadow_pixels,
                        CARRAY<Wvec>& shadow_normals);

   static bool match_spans(ARRAY<PIXEL_list>& spans,
                           CPIXEL_list& drawnpixels,
                           CARRAY<int>& shadow_cusps,
                           CWpt_list& shadow_pts,
                           CPIXEL_list& shadow_pixels,
                           CARRAY<Wvec>& shadow_normals);

   static bool build_curve(CARRAY<PIXEL_list>& spans, 
                           Wpt_list& ret,
                           CARRAY<int>& shadow_cusps,
                           CWpt_list& shadow_pts,
                           CPIXEL_list& shadow_pixels,
                           CARRAY<Wvec>& shadow_normals);

   static bool is_cusp(CWpt& a, CWpt& p, CWpt& b, CWvec& n);

   static PIXELline normal_to_screen_line(CWvec &v, CWpt& pt) {
      return PIXELline(PIXEL(pt), PIXEL(pt + v));
   }	

   static PIXELline shadow_pix_line(int i, 
                                    CWpt_list& shadow_pts,
                                    CARRAY<Wvec>& shadow_normals) {
      return CRV_SKETCH::normal_to_screen_line(shadow_normals[i], shadow_pts[i]);
   }

 protected:
   LINE3D_list  _guidelines;    //!< (view dependent) guidelines for the curve
   LINE3D_list  _line_pool;     //!< cache guidelines to avoid
                                //!< reallocating them in every frame

   Bcurve*         _curve;        //!< the curve
   Bpoint_list     _points;        //!< points to move

   mode_t          _mode;          //!< current interaction mode
   component_t     _component;     //!< type of component being manipulated
   
   Wvec            _avg_shadow_norm;
   Wpt             _initial_eye_loc;

   uint           _init_stamp;    //!< frame number of init() call

   //! Pixel list of the drawn curve
   PIXEL_list   _drawn_pixels;

   //! Pixel list of shadow curve, read in at time of init
   PIXEL_list   _shadow_pixels;
   //! World point list of shadow, read in at time of init
   Wpt_list     _shadow_pts; 
   Wpt_listMap* _shadow_map;

   //! normals at each pt of _shadow_pts;
   ARRAY<Wvec>  _shadow_normals;
   ARRAY<int>   _shadow_cusp_list;  // indices of cusps in _shadow_pts
   
   static CRV_SKETCHptr      _instance;

   static void clean_on_exit();

   //******** INTERNAL METHODS ********

   //******** MANAGERS ********
   CRV_SKETCH();
   virtual ~CRV_SKETCH() {}

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<CRV_SKETCH, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //******** BUILDING ********

   //! (re)builds the guidelines between curve or points and shadow
   void build_guidelines(CWpt_list& line_locs,
                         CWvec& n,               // projection direction
                         LINE3D_list& guidelines // the result
      );     

   //! builds the guidelines between curve or points and 
   //! a focal point (usually the eye point when curves or points
   //! were originally drawn)
   void build_guidelines(CWpt_list& line_locs,
                         CWpt& focus,            
                         LINE3D_list& guidelines // the result
      );     
   
   //! allocates new guideline and sets LINE3D paramters
   StylizedLine3Dptr new_guideline(CWpt& p, CWvec& n) const; 

   //! if screen point p is near a guideline, move the
   //! point associated with the guideline to the point 
   //! along the guidline nearest to p
   //! (only works if current component type is POINT)
   bool move_point(CNDCpt& p);

   //******** SHADOWMATCH METHODS ********

   //! repositions the upper curve using supplied list of world points
   //! assumes that eachpoint in the list is directly "above" a point
   //! in _shadow_pts with the same index
   bool specify_upper(CWpt_list& list);
   
   //! World normal at ith point of shadow:
   Wvec normal_at_shadow(int i) const { return _curve->shadow_dir(); }

   bool oversketch_shadow(CPIXEL_list& sketch_pixels);
   bool reshape_shadow(const PIXEL_list &new_curve);

   void   update();
   void   cache_curve(Bcurve* bcurve);

   bool grow_shadows(Bpoint* p);
   bool create_shadow(Bpoint* p);
   
   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

/*****************************************************************
 * CRV_SKETCH_CMD
 *****************************************************************/
MAKE_PTR_SUBC(CRV_SKETCH_CMD,COMMAND);
class CRV_SKETCH_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   CRV_SKETCH_CMD(Bcurve* curve, CWpt_list& new_pts) : _map(0) {
      assert(curve);
      _curve = curve;
      _map       = (SurfaceCurveMap*)(curve->map());
      RuledSurfCurveVec* surf = (RuledSurfCurveVec*)(_map->surface());
      s_map = (Wpt_listMap*)(surf->get_curve());
      _old_shadow = s_map->get_wpts();
      _new_shadow = new_pts;
      _type = false;
   }

   CRV_SKETCH_CMD(Bcurve* curve, CUVpt_list& new_pts, Wpt new_b1, Wpt new_b2) : _map(0) {
      assert(curve);
      _curve = curve;
      _map       = (SurfaceCurveMap*)(curve->map());
      _old_uvs = _map->uvs();
      _new_uvs = new_pts;
      _old_b1 = _map->map(0);
      _old_b2 = _map->map(1);
      _new_b1 = new_b1;
      _new_b2 = new_b2;
      _type = true;
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("CRV_SKETCH_CMD",
                        CRV_SKETCH_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bcurve*          _curve;
   SurfaceCurveMap* _map;
   Wpt_listMap*     s_map;
   bool         _type;// if the operation is a shadow sketch, type is false
   Wpt_list     _old_shadow;
   Wpt_list     _new_shadow;
   UVpt_list    _old_uvs;
   UVpt_list    _new_uvs;
   Wpt _old_b1, _old_b2;
   Wpt _new_b1, _new_b2;
};

#endif // CRV_SKETCH_H_IS_INCLUDED

/* end of file crv_sketch.H */
