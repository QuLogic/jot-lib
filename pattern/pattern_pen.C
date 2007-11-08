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
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "std/support.H"
#include "glew/glew.H"

#include "std/stop_watch.H"
#include "gtex/buffer_ref_image.H"
#include "gtex/ref_image.H"
#include "geom/command.H"
#include "mesh/bmesh.H"
#include "mesh/mesh_global.H"
#include "gtex/smooth_shade.H"
#include "pattern/pattern_texture.H"
//#include "pattern/pattern_grid.H"
//#include "pattern/quad_cell.H"
//#include "pattern/pattern_stroke.H"
#include "stroke/gesture_stroke_drawer.H"
#include "gest/gesture_box_drawer.H"
#include "pattern/bbox_cell.H"
#include "pattern/rect_cell.H"
#include "pattern/path_cell.H"
#include "pattern/carriers_cell.H"
#include "pattern/pattern_pen.H"
#include "pattern/pattern_pen_ui.H"
#include "pattern/stroke_group.H"
#include <list> 
#include <stack>

using namespace mlib;



/////////////////////////////////////////////
// Pattern Pen Class
/////////////////////////////////////////////

int          PatternPen::VARIATION       = 1;
int          PatternPen::AUTO_GRID_SYNTH = 0;
const double PatternPen::ICON_SIZE       = 50.0;
const double PatternPen::ICON_OFFSET     = 5.0;

PatternPen::PatternPen(
   CGEST_INTptr &gest_int,
   CEvent& d, CEvent& m, CEvent& u,
   CEvent& shift_down, CEvent& shift_up,
   CEvent& ctrl_down,  CEvent& ctrl_up) :
   Pen(str_ptr("Pattern Mode"),
       gest_int, d, m, u,
       shift_down, shift_up,
       ctrl_down, ctrl_up), 
   _mode(STROKE), 
   _example_strokes(new GELset()), _example_icon_strokes(new GELset()),  
   _synthesized_strokes(new GestureCellDrawer()), _live_strokes(new GELset()),
   _geometry(new GELset()), 
   _picture_bbox(), 
   _current_strokes(), 
   _current_group_type(StrokeGroup::FREE), _current_reference_frame(0), 
   _stroke_pattern(), _display_structure(false), _display_ref_frame(true),
   _current_cell_type(0), _current_synth_mode(StrokeGroup::MIMIC),
   _current_distrib(StrokeGroup::LLOYD),
   _show_icons(true), _current_global_scale(1.0),
   _init_proxy(false){

   // gestures we recognize:
   _draw_start += DrawArc(new TapGuard,      drawCB(&PatternPen::tap_cb));
   _draw_start += DrawArc(new SlashGuard,    drawCB(&PatternPen::slash_cb));
   _draw_start += DrawArc(new LineGuard,     drawCB(&PatternPen::line_cb));
   _draw_start += DrawArc(new ScribbleGuard, drawCB(&PatternPen::scribble_cb));
   _draw_start += DrawArc(new LassoGuard,    drawCB(&PatternPen::lasso_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&PatternPen::stroke_cb));
  
   _blank_gesture_drawer = new GestureDrawer();
   _box_gesture_drawer = new GestureBoxDrawer();
   _rect_gesture_drawer = new GestureRectDrawer();
   _path_gesture_drawer = new GesturePathDrawer();
   _carrier_gesture_drawer = new GestureCarrierDrawer();
   _gesture_preset = new BaseStroke();
   _gesture_preset->set_width(2.0);
   _gesture_preset->set_press_vary_width(true);
//   _gesture_preset->set_press_vary_alpha(true);
   _gesture_drawer = new GestureStrokeDrawer();
   _gesture_drawer->set_base_stroke_proto(_gesture_preset);
  
   load_attribute_presets();
   _ui = new PatternPenUI(this);

   _stroke_pattern_drawer = new StrokePatternDrawer(&_stroke_pattern);
   _ref_frame_drawer = new RefFrameDrawer(_current_reference_frame);

   assert(_ui);      
}


void 
PatternPen::load_attribute_presets(){
   // first load a default preset that varies with width
   BaseStroke* default_width_preset = new BaseStroke();
   default_width_preset->set_press_vary_width(true);
   default_width_preset->set_press_vary_alpha(false);

   GestureStrokeDrawer* stroke_width_drawer = new GestureStrokeDrawer();
   stroke_width_drawer->set_base_stroke_proto(default_width_preset);
   _stroke_drawers += stroke_width_drawer;

   _attribute_presets_names += "default_width";

   // test: load another default preset that varies with alpha
   BaseStroke* default_alpha_preset = new BaseStroke();
   default_alpha_preset->set_press_vary_width(false);
   default_alpha_preset->set_press_vary_alpha(true);

   GestureStrokeDrawer* stroke_alpha_drawer = new GestureStrokeDrawer();
   stroke_alpha_drawer->set_base_stroke_proto(default_alpha_preset);
   _stroke_drawers += stroke_alpha_drawer;

   _attribute_presets_names += "default_alpha";

   // and assign the first preset
   _current_attribute_id = 0;
}   


void
PatternPen::init_proxy(Bface* f) {
   if(!_init_proxy){
      Patch* p = get_ctrl_patch(f);
      assert(p);
    
      p->set_texture(PatternTexture::static_name());
      GTexture* cur_texture = p->cur_tex();
      if(!cur_texture->is_of_type(PatternTexture::static_name())) return;
      _pattern_texture = (PatternTexture*)cur_texture;
      _pattern_texture->set_patch(p);
      _ui->update();

      _view->set_rendering(PatternTexture::static_name());
    
      _init_proxy = true;  
   }       
}


PatternPen::~PatternPen() {
   if(_ui)
      delete _ui;
  
   if(_gesture_drawer)
      delete _gesture_drawer;
}

void
PatternPen::activate(State *s) { 
   if(_ui) {
      _ui->show();
   }
   Pen::activate(s);

   // fill geometry display set
   _geometry = GELsetptr(new GELset);
   unsigned int nb_drawn_gel = DRAWN.num();
   for (unsigned int i=0 ; i<nb_drawn_gel ; i++){
      if (gel_to_bmesh(DRAWN[i])) {
         *_geometry += DRAWN[i];
      }
   }
  
   // then replace geometry elements from DRAWN with the GELset
   unsigned int nb_geom_gel = _geometry->num();
   for (unsigned int i=0 ; i<nb_geom_gel ; i++){
      WORLD::undisplay((*_geometry)[i]);
   }  
  
   // default mode is STROKE
   change_mode(STROKE);  

   if (_view)
      _view->set_rendering(GTexture::static_name());     

}

bool
PatternPen::deactivate(State* s) {
   cerr << "PatternPen::deactivate" << endl;
   if(_ui)
      _ui->hide();
  
   bool ret = Pen::deactivate(s);
   assert(ret);

   // first remove GELsets
   WORLD::undisplay(_ref_frame_drawer);
   WORLD::undisplay(_stroke_pattern_drawer);
   WORLD::undisplay(_example_strokes);
   WORLD::undisplay(_example_icon_strokes);
   WORLD::undisplay(_live_strokes);
   WORLD::undisplay(_synthesized_strokes);
   WORLD::undisplay(_geometry);

   // put geometry back.
   unsigned int nb_geom_gel = _geometry->num();
   for (unsigned int i=0 ; i<nb_geom_gel ; i++){
      WORLD::display((*_geometry)[i]);
   }  
   _geometry->clear();

   return true;
}

void
PatternPen::change_mode(int new_mode) {
   switch(new_mode){
    case STROKE: cerr << "Changed to STROKE mode" << endl;
      if (_gest_int && _gesture_drawer){
         _gest_int->set_drawer(_gesture_drawer);
      }

      _mode = STROKE;
      update_display();
      // TODO: bypass camera motions
      break;

    case SYNTH: cerr << "Changed to SYNTH mode" << endl;
      if (_gest_int) {
         if (_current_cell_type==BBOX_CELL && _box_gesture_drawer){
            _gest_int->set_drawer(_box_gesture_drawer);
         } else if (_current_cell_type==RECT_CELL && _rect_gesture_drawer) {
            _gest_int->set_drawer(_rect_gesture_drawer);
         } else if (_current_cell_type==PATH_CELL && _path_gesture_drawer) {
            _gest_int->set_drawer(_path_gesture_drawer);
         } else if (_current_cell_type==CARRIER_CELL && _carrier_gesture_drawer) {
            _gest_int->set_drawer(_carrier_gesture_drawer);
         }
      }

      _mode = SYNTH;
      update_display();
      // TODO: bypass camera motions
      break;

    case PATH: cerr << "Changed to PATH mode" << endl;
      if (_gest_int && _blank_gesture_drawer){
         _gest_int->set_drawer(_blank_gesture_drawer);
      }

      _mode = PATH;
      update_display();
      break;

    case PROXY: cerr << "Changed to PROXY mode" << endl;
      if (_gest_int && _blank_gesture_drawer){
         _gest_int->set_drawer(_gesture_drawer);
      }

      _mode = PROXY;
      update_display();
      break;

    case ELLIPSE: cerr << "Changed to ELLIPSE mode" << endl;
      if (_gest_int && _blank_gesture_drawer){
         _gest_int->set_drawer(_blank_gesture_drawer);
      }

      _mode = ELLIPSE;
      update_display();
      break; 
  
   }
}


void 
PatternPen::create_new_group(int type, int ref){
   update_stroke_pattern();
   _current_group_type = type;
   _current_reference_frame = ref;
   _ref_frame_drawer->set_ref_frame(ref);
}


void 
PatternPen::update_display(){
   _view->set_rendering(SmoothShadeTexture::static_name());

   if (_mode==STROKE || _mode==SYNTH){
      // show relevant strokes
      if (_mode==STROKE){
         if (_display_ref_frame){
            WORLD::display(_ref_frame_drawer);
         }
         WORLD::display(_example_strokes);
         if (_display_structure){
            WORLD::display(_stroke_pattern_drawer);
         }

         WORLD::undisplay(_example_icon_strokes);
         WORLD::undisplay(_live_strokes);
         WORLD::undisplay(_synthesized_strokes);
      } else {
         WORLD::undisplay(_ref_frame_drawer);
         WORLD::undisplay(_example_strokes);
         WORLD::undisplay(_stroke_pattern_drawer);

         update_stroke_pattern();
         WORLD::display(_live_strokes);
         WORLD::display(_synthesized_strokes);
         update_icon_strokes();
         if (_show_icons){
            WORLD::display(_example_icon_strokes);
         }
      }

      // hide geometry
      WORLD::undisplay(_geometry);
     
   } else {
      // show relevant strokes
      update_stroke_pattern();
      WORLD::undisplay(_ref_frame_drawer);
      WORLD::undisplay(_example_strokes);    
      WORLD::undisplay(_stroke_pattern_drawer);
      WORLD::undisplay(_live_strokes);
      WORLD::undisplay(_synthesized_strokes);

      update_icon_strokes();
      WORLD::display(_example_icon_strokes);
    
      // show geometry and change to patch's desired texture
      WORLD::display(_geometry);
      if (_view && _mode==PROXY){
         //_view->set_rendering(PatternTexture::static_name());
         _view->set_rendering(SmoothShadeTexture::static_name());
      } else {
         _view->set_rendering(SmoothShadeTexture::static_name());
      }

   }
}


int
PatternPen::stroke_cb(CGESTUREptr& gest, DrawState*&) {
//   err_msg("PatternPen::stroke_cb()");   
   if (_mode == STROKE){
      add_example_stroke(gest);
   } else if (_mode == SYNTH){
      if (compute_target_cell(gest)){
         synthesize_strokes();
      }
   } else if (_mode == PATH){
   } else if (_mode == PROXY){
      //populate_grid(gest);
      //Draw a stroke onto a proxy surface
      add_stroke_to_proxy(gest);
   } else if (_mode == ELLIPSE){
   }
   return 0;
}


int
PatternPen::tap_cb(CGESTUREptr& gest, DrawState*& s){
//   err_msg("PatternPen::tap_cb()");
   if (_mode == PROXY){
      Bface* f;
      f = VisRefImage::Intersect(gest->center());
      if(f)
         init_proxy(f);
   } else {
      return stroke_cb(gest, s);  
   }
   return 1;
}


int
PatternPen::line_cb(CGESTUREptr& gest, DrawState*& s) {
//   err_msg("PatternPen::line_cb()");
   return stroke_cb(gest, s);
}


int
PatternPen::slash_cb(CGESTUREptr& gest, DrawState*& s) {
//   err_msg("PatternPen::slash_cb()");
   return stroke_cb(gest, s);
}


int
PatternPen::scribble_cb(CGESTUREptr& gest, DrawState*& s) {
//   err_msg("PatternPen::scribble_cb()");
   return stroke_cb(gest, s);
}


int
PatternPen::lasso_cb(CGESTUREptr& gest, DrawState*& s) {
//   err_msg("PatternPen::lasso_cb()");   
   return stroke_cb(gest, s);
}





///////////////////
// Stroke methods
///////////////////



void 
PatternPen::set_epsilon(double eps){
   _stroke_pattern.set_epsilon(eps);
}

void 
PatternPen::set_style_adjust(double sa){
   _stroke_pattern.set_style_adjust(sa);
}

void 
PatternPen::set_ref_frame(int id){
   _ref_frame_drawer->set_ref_frame(id);
}


void 
PatternPen::set_analyze_style(bool b){
   _stroke_pattern.set_analyze_style(b);
}

void 
PatternPen::display_structure(bool b){
   _display_structure = b;
   if (b){
      WORLD::display(_stroke_pattern_drawer);
   } else {
      WORLD::undisplay(_stroke_pattern_drawer);
   }
}

void 
PatternPen::display_reference_frame(bool b){
   _display_ref_frame = b;
   if (b){
      WORLD::display(_ref_frame_drawer);
   } else {
      WORLD::undisplay(_ref_frame_drawer);
   }
}

void 
PatternPen::add_example_stroke(CGESTUREptr& gest){
   // first add gesture to display sets
   *_example_strokes += gest;
 
   // then add the gesture as a stroke to the current group
   GestureStroke s (gest, _gesture_drawer);
   _current_strokes.push_back(s);
}


void
PatternPen::pop_stroke(){
   if (!_current_strokes.empty()){
      _current_strokes.pop_back();
      _example_strokes->pop();
   }
}

void
PatternPen::clear_strokes(){
   _stroke_pattern.clear();
   _current_strokes.clear();
   _example_strokes->clear();
}



void
PatternPen::update_stroke_pattern(){
   // old synthesized strokes are irrelevant
   //_synthesized_strokes->clear();

   // finalize current group
   if (!_current_strokes.empty()){
      _stroke_pattern.add_group(_current_group_type, _current_reference_frame, _current_strokes);
      _current_strokes.clear();
   }
}


void
PatternPen::update_icon_strokes(){
   // first clear icon gestures
   _example_icon_strokes->clear();  
   compute_icon_cell();
   if (_icon_cell.valid()){
      // TODO: use COPY mode when it works
//     _stroke_pattern.synthesize(StrokePattern::REPEAT, false, &_icon_cell);
   } else {
      cerr << "icon cell invalid !" << endl;
   }

   unsigned int nb_icon_strokes = _icon_cell.nb_strokes();
   for (unsigned int i=0 ; i<nb_icon_strokes ; i++){
      GESTUREptr icon_gest = _icon_cell.gesture(i);
      icon_gest->set_drawer(_gesture_drawer);
      *_example_icon_strokes += icon_gest;
   }  

}


const UVvec
PatternPen::compute_icon_cell(){
   double w = _stroke_pattern.bbox().width();
   double h = _stroke_pattern.bbox().height();
   double ratio = 1.0;
//   if (w > h){
//     ratio = ICON_SIZE / w;
//   } else {
//     ratio = ICON_SIZE / h;
//   }
   PIXEL icon_min (ICON_OFFSET, ICON_OFFSET);
   PIXEL icon_max (ICON_OFFSET+w*ratio, ICON_OFFSET+h*ratio);
   _icon_cell = BBoxCell(icon_min, icon_max);  
  
   return UVvec(ratio, ratio);
}






//////////////////////
// Synthesis methods
//////////////////////

void 
PatternPen::set_current_cell_type(int type)  { 
   _current_cell_type = type; 
   if (_current_cell_type == BBOX_CELL && _box_gesture_drawer){
      _gest_int->set_drawer(_box_gesture_drawer);
   } else if (_current_cell_type == RECT_CELL && _rect_gesture_drawer){
      _rect_gesture = GESTUREptr();
      _rect_gesture_drawer->set_axis(_rect_gesture);
      _gest_int->set_drawer(_rect_gesture_drawer);
   } else if (_current_cell_type == PATH_CELL && _path_gesture_drawer) {
      _path_gesture = GESTUREptr();    
      _path_gesture_drawer->set_path(_path_gesture);
      _gest_int->set_drawer(_path_gesture_drawer);
   } else if (_current_cell_type == CARRIER_CELL && _carrier_gesture_drawer) {
      _carrier_gesture = GESTUREptr();
      _carrier_gesture_drawer->set_first_carrier(_carrier_gesture);
      _gest_int->set_drawer(_carrier_gesture_drawer);
   }
}

void 
PatternPen::show_example_strokes(bool b){
   _show_icons = b;
   if (b){
      WORLD::display(_example_icon_strokes);
   } else {
      WORLD::undisplay(_example_icon_strokes);
   }  
}


void 
PatternPen::set_current_global_scale(double scale){
   _current_global_scale = scale;
}

void 
PatternPen::set_current_synth_mode(int mode)  { 
   _current_synth_mode = mode; 
}

void 
PatternPen::set_current_distribution(int distrib)  { 
   _current_distrib = distrib; 
}

void 
PatternPen::set_current_stretching_state(bool b)  { 
   _current_stretching_state = b; 
}


void 
PatternPen::set_current_ring_nb(int n)  { 
   _stroke_pattern.set_ring_nb(n);
}

void 
PatternPen::set_current_correction(double d)  { 
   _stroke_pattern.set_correction_amount(d);
   if (_target_cell){
      _stroke_pattern.render_synthesized_strokes(_target_cell);
      render_target_cell();
   }
}

void 
PatternPen::rerender_target_cell()  { 
  
   if (_target_cell){
      _stroke_pattern.render_synthesized_strokes(_target_cell);
      render_target_cell();
   }
}


bool
PatternPen::compute_target_cell(CGESTUREptr& gest){
   if (_target_cell) {
      print_target_cell();
   }

   int nb_pts = gest->pts().num();
   if (nb_pts < 2) return false;
  
   if (_current_cell_type == BBOX_CELL){
      _target_cell = new BBoxCell(gest->pts()[0], gest->pts()[nb_pts-1]);
   } else if (_current_cell_type == RECT_CELL){
      double thickness;
      GESTUREptr axis_gesture;
      update_rect_drawer(gest, axis_gesture, thickness);
      if (axis_gesture) {
         _target_cell = new RectCell(axis_gesture, thickness);
      } else {
         return false;
      }
   } else if (_current_cell_type == PATH_CELL){
      double thickness;
      GESTUREptr axis_gesture;
      update_path_drawer(gest, axis_gesture, thickness);
      if (axis_gesture) {
         _target_cell = new PathCell(axis_gesture, thickness);
      } else {
         return false;
      }
   } else if (_current_cell_type == CARRIER_CELL){
      GESTUREptr first_gesture;
      GESTUREptr second_gesture;
      update_carrier_drawer(gest, first_gesture, second_gesture);
      if (first_gesture) {
         _target_cell = new CarriersCell(first_gesture, second_gesture);
      } else {
         return false;
      }
   } else {
      return false;    
   }

   _target_cell->set_global_scale(_current_global_scale);
   return true;
}




void
PatternPen::update_rect_drawer(CGESTUREptr& gest, GESTUREptr& axis_gesture, double& thickness){
   if (_rect_gesture){
      thickness = gest->endpoint_dist();
      axis_gesture = new GESTURE(*_rect_gesture);

      _rect_gesture = GESTUREptr();
   } else {
      _rect_gesture = new GESTURE(*gest);
      _rect_gesture->set_drawer(_blank_gesture_drawer);
   }

   _rect_gesture_drawer->set_axis(_rect_gesture);
}

void
PatternPen::update_path_drawer(CGESTUREptr& gest, GESTUREptr& axis_gesture, double& thickness){
   if (_path_gesture){
      thickness = gest->endpoint_dist();
      axis_gesture = new GESTURE(*_path_gesture);

      _path_gesture = GESTUREptr();
   } else {
      _path_gesture = new GESTURE(*gest);
      _path_gesture->set_drawer(_blank_gesture_drawer);
   }

   _path_gesture_drawer->set_path(_path_gesture);
}


void
PatternPen::update_carrier_drawer(CGESTUREptr& gest, GESTUREptr& first_gesture, GESTUREptr& second_gesture){
   if (_carrier_gesture){
      first_gesture = new GESTURE(*_carrier_gesture);
      second_gesture = new GESTURE(*gest);

      _carrier_gesture = GESTUREptr();
   } else {
      _carrier_gesture = new GESTURE(*gest);
      _carrier_gesture->set_drawer(_blank_gesture_drawer);
   }

   _carrier_gesture_drawer->set_first_carrier(_carrier_gesture);
}

bool get_back_color(CPIXEL& pix, COLOR& col) {
   CTEXTUREptr back_t = VIEW::peek()->get_bkg_tex();
  
   if(back_t && PatternPenUI::use_image_color){
	
      Image back_image = back_t->image();
      //cerr << "looking at file " << VIEW::peek()->get_bkg_file() << endl;
      uint r = back_image.pixel_r((uint)pix[0],(uint)pix[1]);
      uint g = back_image.pixel_g((uint)pix[0],(uint)pix[1]);
      uint b = back_image.pixel_b((uint)pix[0],(uint)pix[1]);        
   
      col[0] = r/255.0; col[1] = g/255.0; col[2] = b/255.0;
      return true;
   }
   return false;
	
}

void PatternPen::adjust_color(COLOR& col) {
 
   HSVCOLOR tmp(col);
   tmp[0] += _color_adjust[0];
   tmp[1] += _color_adjust[1];
   tmp[2] += _color_adjust[2]; 
   tmp[0] = (tmp[0] > 1.0) ? 1.0 : (tmp[0] < 0.0) ? 0.0 : tmp[0];
   tmp[1] = (tmp[1] > 1.0) ? 1.0 : (tmp[1] < 0.0) ? 0.0 : tmp[1];
   tmp[2] = (tmp[2] > 1.0) ? 1.0 : (tmp[2] < 0.0) ? 0.0 : tmp[2];
   COLOR tmp2(tmp);
   col = tmp2;
}

void
PatternPen::synthesize_strokes(){
   if (_target_cell && _target_cell->valid()){
      _stroke_pattern.synthesize(_current_synth_mode, _current_distrib, _current_stretching_state, _target_cell); 
   } else {
      cerr << "target cell invalid !" << endl;
   }

   render_target_cell();
}

void
PatternPen::render_target_cell(){
   _live_strokes->clear();
   unsigned int nb_target_strokes = _target_cell->nb_strokes();
   for (unsigned int i=0 ; i<nb_target_strokes ; i++){
      GESTUREptr target_gest = _target_cell->gesture(i);
    
      //XXX - oh yea memory leak here I come
    
      //GestureStrokeDrawer* target_drawer = _stroke_drawers[_target_cell->id(i)];
      GestureStrokeDrawer* target_drawer = new GestureStrokeDrawer();
    
      BaseStroke* t_preset = new BaseStroke();
      t_preset->copy(*(_gesture_drawer->base_stroke_proto()));
    
      // Lets get a color from Back Image is we want so
      COLOR tmp;
      if(get_back_color(target_gest->center(), tmp)) {		
         adjust_color(tmp);
         t_preset->set_color(tmp);		
      }
    
      target_drawer->set_base_stroke_proto(t_preset);
    
      target_gest->set_drawer(target_drawer);
      *_live_strokes += target_gest;
    
   }  
   _target_cell->clear();
}


void
PatternPen::print_target_cell(){  
   if (_live_strokes->num()==0){
      return;
   }

   // Hack, Lets ensure that we have something in GELset so that draw
   // will execute
   if(_synthesized_strokes->num() < 1) {
      GESTUREptr dummy_gest = new GESTURE(NULL, -1, PIXEL(0.0, 0.0), 1.0);
      *_synthesized_strokes += dummy_gest;
   }

   // add the strokes to the drawing and clear the current strokes
   _synthesized_strokes->add(*_live_strokes, _target_cell);
   _live_strokes->clear();
}

void
PatternPen::resynthesize()
{
   //Take care of the life cell
   //_live_strokes->clear(); 
   //_target_cell->set_global_scale(_current_global_scale);
   //_target_cell->clear();
   //synthesize_strokes();
	
   //Lets now take care of the other strokes
   std::vector<GestureCell*> cells;
   for(int i=0; i < _synthesized_strokes->size(); ++i) {
      cells.push_back(_synthesized_strokes->get_cell(i));
      cerr << "Target cell added " << i  << endl;
   }
	
   _synthesized_strokes->clear();
    
   for(uint k=0; k < cells.size(); ++k) {
      _target_cell = cells[k];
      if(_target_cell){
         _target_cell->clear();
         _target_cell->set_global_scale(_current_global_scale);
      }
      synthesize_strokes();
      print_target_cell();
   }
}



void 
PatternPen::pop_synthesized() {
   _live_strokes->clear();
}

void 

PatternPen::clear_synthesized(){
   _live_strokes->clear(); 
   _synthesized_strokes->clear();

}

///////////////////
// Proxy methods
///////////////////
void 
PatternPen::add_stroke_to_proxy(CGESTUREptr& gest)
{
   if (gest->pts().num() < 2) {
      WORLD::message("Failed to generate hatch stroke (too few samples)...");
      return;
   }
   //NDCpt_list ndcpts = gest->pts();
   //if (_pattern_texture)
   //_pattern_texture->proxy_surface()->add(
   //   gest->pts(),gest->pressures(), _gesture_drawer->base_stroke_proto());
}

// end of file pattern_pen.C
