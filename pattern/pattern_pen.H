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
/***************************************************************************
    pattern_pen.H
    
    PatternPen
    * Activates/Shows the PatternPenUI 
    * Derives Gesture callback functions
    * Allows for five different modes: Stroke, Synthesis, Path, Grid and Ellipse

    GELset
    * A GEL container for rapid display/undisplay mechanisms used when changing modes

    -------------------
    Simon Breslav, Pascal Barla
    Fall 2004
 ***************************************************************************/
#ifndef _PATTERN_PEN_H_IS_INCLUDED_
#define _PATTERN_PEN_H_IS_INCLUDED_

#include "disp/bbox.H"
#include "disp/gel_set.H"
#include "stroke/gesture_stroke_drawer.H"
#include "gest/gesture_box_drawer.H"
#include "gest/pen.H"
#include "pattern/stroke.H"
#include "pattern/stroke_pattern.H"
#include "pattern/stroke_pattern_drawer.H"
#include "pattern/ref_frame_drawer.H"
#include "pattern/pattern_texture.H"
#include "pattern/gesture_cell.H"
#include "pattern/bbox_cell.H"
#include "pattern/gesture_rect_drawer.H"
#include "pattern/gesture_path_drawer.H"
#include "pattern/gesture_carrier_drawer.H"
#include "pattern/gesture_cell_drawer.H"
#include "pattern/pattern_texture.H"
#include "tess/tex_body.H"
#include "mlib/points.H"

class PatternPenUI;
class PatternGrid;
class QuadCell;


class PatternPen : public Pen { 

  /*** public methods ***/
public: 
  static int         VARIATION;
  static int         AUTO_GRID_SYNTH;
  
  // Constructor / destructor
  PatternPen(CGEST_INTptr &gest_int,
	     CEvent &d, CEvent &m, CEvent &u,
	     CEvent &shift_down, CEvent &shift_up,
	     CEvent &ctrl_down, CEvent &ctrl_up);
  
  virtual ~PatternPen();
  
  // Pen methods
  virtual void   activate(State *);
  virtual bool   deactivate(State *);

  // mode methods
  enum { STROKE=0, SYNTH, PATH, PROXY, ELLIPSE};
  void change_mode(int m);

  // Gesture methods
  virtual int    tap_cb      (CGESTUREptr& gest, DrawState*&);
  virtual int    scribble_cb (CGESTUREptr& gest, DrawState*&);
  virtual int    lasso_cb    (CGESTUREptr& gest, DrawState*&);
  virtual int    line_cb     (CGESTUREptr& gest, DrawState*&);
  virtual int    stroke_cb   (CGESTUREptr& gest, DrawState*&);
  virtual int    slash_cb    (CGESTUREptr& gest, DrawState*&);


  // Stroke methods
  CARRAY<char*>& attribute_preset_names() {return _attribute_presets_names; }
  void set_epsilon(double eps);
  void set_style_adjust(double sa);
  void pop_stroke();
  void clear_strokes();
  void set_ref_frame(int id);
  void set_analyze_style(bool b);
  void create_new_group(int type, int ref);
  void display_structure(bool b);
  void display_reference_frame(bool b);

  // Synthesis methods
  void set_current_cell_type(int type);
  void show_example_strokes(bool b); 
  void set_current_global_scale(double scale);
  void set_current_synth_mode(int mode); 
  void set_current_distribution(int distrib); 
  void set_current_stretching_state(bool b);
  void set_current_ring_nb(int n);
  void set_current_correction(double d);
  enum {BBOX_CELL=0, RECT_CELL, PATH_CELL, CARRIER_CELL};
  void pop_synthesized();
  void clear_synthesized();
  void set_color_adjust(HSVCOLOR c) {_color_adjust = c;}
  void adjust_color(COLOR& col);
  

  // Proxy methods
  PatternTexture* pattern_texture()    const{ return _pattern_texture; }  
   void    set_gesture_drawer_w(bool w) { _gesture_drawer->base_stroke_proto()->set_press_vary_width(w);}
  void    set_gesture_drawer_a(bool a) { _gesture_drawer->base_stroke_proto()->set_press_vary_alpha(a);}
  bool    get_gesture_drawer_w()  const{ return _gesture_drawer->base_stroke_proto()->get_press_vary_width();}
  bool    get_gesture_drawer_a()  const{ return _gesture_drawer->base_stroke_proto()->get_press_vary_alpha();}
  
  void    set_gesture_drawer_base(BaseStroke* s) { _gesture_drawer->set_base_stroke_proto(s); }
  void    init_proxy(Bface* f);
  void    add_stroke_to_proxy(CGESTUREptr& gest);
  //----------


  void resynthesize();
  void rerender_target_cell();

  
  /*** private methods ***/
protected: 
  // Gestures methods
  typedef     CallMeth_t<PatternPen,GESTUREptr> draw_cb_t;
  draw_cb_t*  drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

  // Display sets methods
  void update_display();

  // Stroke methods
  void load_attribute_presets();
  void apply_current_attributes();
  void add_example_stroke(CGESTUREptr& gest);
  void update_stroke_pattern();
  void update_icon_strokes();
  const mlib::UVvec compute_icon_cell();

  // Synthesis methods 
  bool compute_target_cell(CGESTUREptr& gest);
  void update_rect_drawer(CGESTUREptr& gest, GESTUREptr& axis_gesture, double& height);
  void update_path_drawer(CGESTUREptr& gest, GESTUREptr& axis_gesture, double& thickness);
  void update_carrier_drawer(CGESTUREptr& gest, GESTUREptr& first_gesture, GESTUREptr& second_gesture);
  void synthesize_strokes();
  void render_target_cell();
  void print_target_cell();

  /*** private vars ***/
private:
  PatternPenUI* _ui;

  // mode vars
  int _mode;

  // Gestures vars
  GestureDrawer*       _blank_gesture_drawer; 
  BaseStroke*          _gesture_preset;
  GestureStrokeDrawer* _gesture_drawer;       
  GestureBoxDrawer*    _box_gesture_drawer;
  GestureRectDrawer*   _rect_gesture_drawer;
  GesturePathDrawer*   _path_gesture_drawer;
  GestureCarrierDrawer*   _carrier_gesture_drawer;
  GestureStrokeDrawer* _current_drawer;       

  // Display sets vars
  GELsetptr _example_strokes;
  GELsetptr _example_icon_strokes;
  GestureCellDrawerptr  _synthesized_strokes;
  GELsetptr _live_strokes;
  GELsetptr _geometry;
  BBoxCell  _icon_cell;
  static const double ICON_SIZE;
  static const double ICON_OFFSET;

  // Stroke vars
  BBOXpix                     _picture_bbox;
  ARRAY<char*>                _attribute_presets_names;
  ARRAY<GestureStrokeDrawer*> _stroke_drawers; 
  int                         _current_attribute_id;
  std::vector<GestureStroke>  _current_strokes;
  int                         _current_group_type;
  int                         _current_reference_frame;
  StrokePattern          _stroke_pattern;
  StrokePatternDrawerptr _stroke_pattern_drawer;
  bool                   _display_structure;
  bool                   _display_ref_frame;
  RefFrameDrawerptr      _ref_frame_drawer;

  // Synth vars
  int            _current_cell_type;
  int            _current_synth_mode;
  int            _current_distrib;
  bool           _current_stretching_state;
  bool           _current_correction_state;
  GestureCell*   _target_cell;
  mlib::UVvec    _target_ratio;
  GESTUREptr _rect_gesture;
  GESTUREptr _path_gesture;
  GESTUREptr _carrier_gesture;
  bool       _show_icons;
  double     _current_global_scale;

  HSVCOLOR   _color_adjust;

  // Proxy Surface vars             
  PatternTexture* _pattern_texture;
  bool            _init_proxy;
 
  
};









/*****************************************************************
 * ADD_STROKE_GROUP_CMD
 *
 *      Perform an ADD_STROKE_GROUP_CMD operation.
 *
 *****************************************************************/

/*
MAKE_PTR_SUBC(ADD_STROKE_GROUP_CMD,COMMAND);
class ADD_STROKE_GROUP_CMD : public COMMAND {
 public:
   // ******** MANAGERS ********
   ADD_STROKE_GROUP_CMD(QuadCell* cell, Stroke_List* group) :
       _cell(cell),
       _group(group),
       _pos(-1)
   {}

   // ******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("STROKE_SYNTHESIS_CMD", COMMAND, CCOMMAND*);

   // ******** COMMAND VIRTUAL METHODS ********
   virtual bool doit();
   virtual bool undoit();
   // ******** DIAGNOSTIC ********

   virtual void print() const {
      cerr << class_name();
   }

 protected:
   QuadCell*        _cell;
   Stroke_List*     _group;
   int              _pos;
};
*/

#endif   // _PATTERN_PEN_H_IS_INCLUDED_

/* end of file pattern_pen.H */
