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
#ifndef _LINE_PEN_H_IS_INCLUDED_
#define _LINE_PEN_H_IS_INCLUDED_

////////////////////////////////////////////
// LinePen
////////////////////////////////////////////

#include "wnpr/stroke_ui.H"
#include "stroke/gesture_stroke_drawer.H"
#include "gest/pen.H"

#define COutlineStroke const OutlineStroke

class LinePenUI;
class NPRTexture;
class BStrokePool;
class OutlineStroke;
class DecalLineStroke;

/*****************************************************************
 * STROKE_GEL
 *****************************************************************/

#define CBASELINE_GELptr const BASELINE_GELptr
MAKE_PTR_SUBC(BASELINE_GEL,GEL);

class BASELINE_GEL : public GEL {
 protected:

	BaseStroke        _stroke;
   mlib::NDCZpt_list       _pts;

 public:
   BASELINE_GEL()            
   { 
      _stroke.set_vis(VIS_TYPE_SCREEN); 
      _stroke.set_gen_t(true);
      _pts.clear(); 
   }
   ~BASELINE_GEL()           { }

	BaseStroke*       stroke()	{ return &_stroke;	}
	mlib::NDCZpt_list&		pts()	   { return _pts;			}

 public:
    virtual int       draw(CVIEWptr &v) { return 0; }
   virtual int			draw_final(CVIEWptr &v) 
	{ 
		int i;

      _pts.update_length();

      _stroke.clear();
      _stroke.set_min_t(0.0);
      _stroke.set_max_t(_pts.length()*VIEW::peek()->ndc2pix_scale()/
                                            (max(_stroke.get_angle(),1.0f)));

      for (i=0;i<_pts.num();i++) _stroke.add(_pts[i]);

      _stroke.draw_start(); 
		i = _stroke.draw(v); 
		_stroke.draw_end(); 
		return i;
	}
	virtual DATA_ITEM*	dup()	const			{ return 0; }

};

/*****************************************************************
 * LinePen
 *****************************************************************/
class LinePen : public Pen, public CAMobs {
 public:
   /******** MEMBERS TYPES ********/    
    enum edit_mode_t {
      EDIT_MODE_NONE = 0,
      EDIT_MODE_DECAL,
      EDIT_MODE_CREASE,
      EDIT_MODE_SIL
   };

    enum line_pen_selection_t {
      LINE_PEN_SELECTION_CHANGED__SIL_TRACKING = 0
   };

    
 protected:

   /******** MEMBERS VARS ********/

   NPRTexture*             _curr_tex;		
   BStrokePool*            _curr_pool;
   OutlineStroke*          _curr_stroke;
   edit_mode_t             _curr_mode;

   bool                    _virtual_baseline;

   OutlineStroke*          _undo_prototype;

   LIST<GESTUREptr>        _gestures;   
   BASELINE_GELptr         _baseline_gel;

   GestureStrokeDrawer*		_gesture_drawer;		
   GestureDrawer*          _blank_gesture_drawer;

   GestureDrawer*				_prev_gesture_drawer;	

   LinePenUI*              _ui;

   /******** MEMBERS METHODS ********/

   typedef		CallMeth_t<LinePen,GESTUREptr> draw_cb_t;
   draw_cb_t*	drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

   bool        pick_stroke(NPRTexture*, mlib::CNDCpt&, double, BStrokePool*&, OutlineStroke*&, edit_mode_t&);

   void        perform_selection(NPRTexture*, BStrokePool*, OutlineStroke*, edit_mode_t);

   void        display_mode();

   void        observe();
   void        unobserve();

   void        apply_undo_prototype();
   void        store_undo_prototype();

   int         create_stroke(CGESTUREptr&);   

   void        set_decal_stroke_verts(mlib::CPIXEL_list&, CARRAY<double>&, DecalLineStroke*);

   void        easel_add(CGESTUREptr&);
   void        easel_clear();
   void        easel_pop();
   void        easel_populate();
 public:
   bool        easel_is_empty();
   void        easel_update_baseline();

   void        update_gesture_drawer();

   //******** CONSTRUCTOR/DECONSTRUCTOR ********

	 LinePen(CGEST_INTptr &gest_int,
            CEvent &d, CEvent &m, CEvent &u,
	         CEvent &shift_down, CEvent &shift_up,
	         CEvent &ctrl_down, CEvent &ctrl_up);

   virtual ~LinePen();

   //******** UI Stuff ********

   NPRTexture*    curr_tex()     { return _curr_tex; }
   edit_mode_t    curr_mode()    { return _curr_mode; }
   BStrokePool*   curr_pool()    { return _curr_pool; }   
   OutlineStroke* curr_stroke()  { return _curr_stroke; }
   OutlineStroke* undo_stroke()  { return _undo_prototype; }

   void           set_virtual_baseline(bool v)     { _virtual_baseline = v;      }
   bool           get_virtual_baseline() const     { return _virtual_baseline;   }


   void           button_mesh_recrease();
   void           button_noise_prototype_next();
   void           button_noise_prototype_del();
   void           button_noise_prototype_add();
   void           button_edit_cycle_line_types();
   void           button_edit_cycle_decal_groups();
   void           button_edit_cycle_crease_paths();
   void           button_edit_cycle_crease_strokes();
   void           button_edit_offset_edit();
   void           button_edit_offset_clear();
   void           button_edit_offset_undo();
   void           button_edit_offset_apply();
   void           button_edit_style_apply();
   void           button_edit_style_get();
   void           button_edit_stroke_add();
   void           button_edit_stroke_del();
   void           button_edit_synth_rubber();
   void           button_edit_synth_synthesize();
   void           button_edit_synth_ex_add();
   void           button_edit_synth_ex_del();
   void           button_edit_synth_ex_clear();
   void           button_edit_synth_all_clear();

   void           modify_active_prototype(COutlineStroke *);
   COutlineStroke*retrieve_active_prototype();

   void           selection_changed(line_pen_selection_t t);
   //******** GestObs METHODS ********
   virtual int    handle_event(CEvent&, State* &);

   //******** GESTURE METHODS ********

   virtual int    tap_cb      (CGESTUREptr& gest, DrawState*&);
   virtual int    scribble_cb (CGESTUREptr& gest, DrawState*&);
   virtual int    lasso_cb    (CGESTUREptr& gest, DrawState*&);
   virtual int    line_cb     (CGESTUREptr& gest, DrawState*&);
   virtual int    stroke_cb   (CGESTUREptr& gest, DrawState*&);
   virtual int    slash_cb    (CGESTUREptr& gest, DrawState*&);

   // ******** PEN METHODS ********

   virtual void   key(CEvent &e);

   virtual void   activate(State *);
	virtual bool   deactivate(State *);

   virtual int    erase_down  (CEvent &,State *&) {cerr << "Edown\n"; return 0;}
   virtual int    erase_move  (CEvent &,State *&) {cerr << "Emove\n";return 0;}
   virtual int    erase_up    (CEvent &,State *&) {cerr << "Eup  \n";return 0;}

   virtual int    ctrl_down(CEvent &,State *&)  {cerr << "Cdown\n";return 0;}
   virtual int    ctrl_move(CEvent &,State *&)  {cerr << "Cmove\n";return 0;}
   virtual int    ctrl_up  (CEvent &,State *&)  {cerr << "Cup  \n";return 0;}   

   virtual int    down(CEvent &,State *&) {cerr << "Sdown\n"; return 0;}
   virtual int    move(CEvent &,State *&) {cerr << "Smove\n"; return 0;}
   virtual int    up  (CEvent &,State *&) {cerr << "Sup  \n"; return 0;}

   // ******** CAMobs METHODS ********
   virtual void notify(CCAMdataptr &data);

};

#endif // _LINE_PEN_H_IS_INCLUDED_

/* end of file line_pen.H */

