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
////////////////////////////////////////////
// HatchingPenUI
////////////////////////////////////////////
//
// -Manages a GLUI window that handles the interface
// -Actual 'live' variables reside within HatchingPen
// -Selection updates these vars, and update() refreshes the widgets
// -Apply button tells HatchingPen to apply the current settings
//
////////////////////////////////////////////



//This is relative to JOT_ROOT, and should
//contain ONLY the texture dots for hatching strokes
#define ID_SHIFT                     10
#define ID_MASK                      ((1<<ID_SHIFT)-1)

//Big to ensure the panel of sliders is the widest
//during the size fix up computations.
#define HATCHING_SLIDER_W            250

#include "std/support.H"
#include <GL/glew.h>

#include "geom/winsys.H"
#include "glui/glui_jot.H"

#include "wnpr/hatching_pen.H"
#include "wnpr/hatching_pen_ui.H"


/*****************************************************************
 * HatchingPenUI
 *****************************************************************/

/////////////////////////////////////
// Static variables
/////////////////////////////////////

vector<HatchingPenUI*> HatchingPenUI::_ui;

/////////////////////////////////////
// Constructor
/////////////////////////////////////

HatchingPenUI::HatchingPenUI(
   HatchingPen* pen) :
   _pen(nullptr)
{
                        
   assert(pen);
   _pen = pen;

   _ui.push_back(this);

   _id = _ui.size()-1;
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingPenUI::~HatchingPenUI()
{
   //Need to clean up?
}

/////////////////////////////////////
// build
/////////////////////////////////////
//
// -Setup the widgets (callbacks, etc)
// -All widgets pointers are strored in arrays and
//  indexed by id types declared in the header
//
/////////////////////////////////////

void
HatchingPenUI::build(GLUI* glui, GLUI_Panel *p, int panel_width) 
{
   int i;
   int id = _id << ID_SHIFT;

   assert(_button.empty());       _button.resize(BUT_NUM, nullptr);
   assert(_slider.empty());       _slider.resize(SLIDE_NUM, nullptr);
   assert(_checkbox.empty());     _checkbox.resize(CHECK_NUM, nullptr);
   assert(_panel.empty());        _panel.resize(PANEL_NUM, nullptr);
   assert(_rollout.empty());      _rollout.resize(ROLLOUT_NUM, nullptr);

   //Sub-panel containing animation controls
   _rollout[ROLLOUT_ANIM] = new GLUI_Rollout(p, "Animation Settings", true);
   assert(_rollout[ROLLOUT_ANIM]);

   //       COL1  COL2  Col3
   // ROW1  LoThr LoWid Time
   // ROW2  HiThr HiWid MaxLOD
   // ROW3  Glob  Limit Foo

   //COL1
   _slider[SLIDE_LO_THRESH] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Lo Thresh", 
      id+SLIDE_LO_THRESH, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.1, 0.9,
      &(_pen->params().anim_lo_thresh_()));
   assert(_slider[SLIDE_LO_THRESH]);
   _slider[SLIDE_LO_THRESH]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_LO_THRESH]->set_num_graduations(81);

   _slider[SLIDE_HI_THRESH] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Hi Thresh", 
      id+SLIDE_HI_THRESH, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.1, 0.9, 
      &(_pen->params().anim_hi_thresh_()));
   assert(_slider[SLIDE_HI_THRESH]);
   _slider[SLIDE_HI_THRESH]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_HI_THRESH]->set_num_graduations(81);

   _slider[SLIDE_GLOBAL_SCALE] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Global Scale", 
      id+SLIDE_GLOBAL_SCALE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 0.2, 
      &(_pen->params().anim_global_scale_()));
   assert(_slider[SLIDE_GLOBAL_SCALE]);
   _slider[SLIDE_GLOBAL_SCALE]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_GLOBAL_SCALE]->set_num_graduations(201);

   new GLUI_Column(_rollout[ROLLOUT_ANIM], false);

   // COL2
   _slider[SLIDE_LO_WIDTH] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Lo Width", 
      id+SLIDE_LO_WIDTH, slider_cb,
      GLUI_SLIDER_FLOAT,
      0.1, 1.0,
      &(_pen->params().anim_lo_width_()));
   assert(_slider[SLIDE_LO_WIDTH]);
   _slider[SLIDE_LO_WIDTH]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_LO_WIDTH]->set_num_graduations(91);

   _slider[SLIDE_HI_WIDTH] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Hi Width", 
      id+SLIDE_HI_WIDTH, slider_cb,
      GLUI_SLIDER_FLOAT, 
      1.0, 10.0, 
      &(_pen->params().anim_hi_width_()));
   assert(_slider[SLIDE_HI_WIDTH]);
   _slider[SLIDE_HI_WIDTH]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_HI_WIDTH]->set_num_graduations(91);

   _slider[SLIDE_LIMIT_SCALE] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Limit Scale", 
      id+SLIDE_LIMIT_SCALE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0, 
      &(_pen->params().anim_limit_scale_()));
   assert(_slider[SLIDE_LIMIT_SCALE]);
   _slider[SLIDE_LIMIT_SCALE]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_LIMIT_SCALE]->set_num_graduations(1001);

   new GLUI_Column(_rollout[ROLLOUT_ANIM], false);

   //COL3
   _slider[SLIDE_TRANS_TIME] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Trans. Time", 
      id+SLIDE_TRANS_TIME, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 5.0,
      &(_pen->params().anim_trans_time_()));
   assert(_slider[SLIDE_TRANS_TIME]);
   _slider[SLIDE_TRANS_TIME]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_TRANS_TIME]->set_num_graduations(51);

   _slider[SLIDE_MAX_LOD] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Max. LOD", 
      id+SLIDE_MAX_LOD, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 5.0,
      &(_pen->params().anim_max_lod_()));
   assert(_slider[SLIDE_MAX_LOD]);
   _slider[SLIDE_MAX_LOD]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_MAX_LOD]->set_num_graduations(51);

   _slider[SLIDE_FOO] = new GLUI_Slider(
      _rollout[ROLLOUT_ANIM], "Foo", 
      id+SLIDE_FOO, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0, 
      nullptr);
   assert(_slider[SLIDE_FOO]);
   _slider[SLIDE_FOO]->set_w(HATCHING_SLIDER_W);
   _slider[SLIDE_FOO]->set_num_graduations(11);
   _slider[SLIDE_FOO]->disable();

   // Subpanel with control buttons
   _panel[PANEL_CONTROLS] = new GLUI_Panel(p, "");
   assert(_panel[PANEL_CONTROLS]);

   //        COL1     COL2     COL3
   //  ROW1  Type    Curve    Style
   //  ROW2  DelAll  DelLast  Apply

   // COL1
   _panel[PANEL_TYPE] = new GLUI_Panel(_panel[PANEL_CONTROLS],
               (string("Mode: ") + HatchingGroup::name(_pen->hatch_mode())).c_str());
   assert(_panel[PANEL_TYPE]);
   _button[BUT_TYPE] = new GLUI_Button(
      _panel[PANEL_TYPE], "Type", 
      id+BUT_TYPE, button_cb);
   assert(_button[BUT_TYPE]);

   _panel[PANEL_DELETE_ALL] = new GLUI_Panel(_panel[PANEL_CONTROLS], "Delete");
   assert(_panel[PANEL_DELETE_ALL]);
   _button[BUT_DELETE_ALL] = new GLUI_Button(
      _panel[PANEL_DELETE_ALL],  "All", 
      id+BUT_DELETE_ALL, button_cb);
   assert(_button[BUT_DELETE_ALL]);

   new GLUI_Column(_panel[PANEL_DELETE_ALL], false);

   _delete_confirm = 0;
   _checkbox[CHECK_CONFIRM] = new GLUI_Checkbox(
      _panel[PANEL_DELETE_ALL], "", 
      &_delete_confirm,
      id+CHECK_CONFIRM, checkbox_cb);
   assert(_checkbox[CHECK_CONFIRM]);

   _button[BUT_DELETE_ALL]->set_w(_button[BUT_DELETE_ALL]->get_w() - 2*_checkbox[CHECK_CONFIRM]->get_w());

   // COL2
   new GLUI_Column(_panel[PANEL_CONTROLS], false);

   _panel[PANEL_CURVES] = new GLUI_Panel(_panel[PANEL_CONTROLS],
               (string("Mode: ") + HatchingGroup::curve_mode_name(_pen->curve_mode())).c_str());
   assert(_panel[PANEL_CURVES]);
   _button[BUT_CURVES] = new GLUI_Button(
      _panel[PANEL_CURVES], "Curves", 
      id+BUT_CURVES, button_cb);
   assert(_button[BUT_CURVES]);


   _panel[PANEL_UNDO_LAST] = new GLUI_Panel(_panel[PANEL_CONTROLS], "Undo");
   assert(_panel[PANEL_UNDO_LAST]);
   _button[BUT_UNDO_LAST] = new GLUI_Button(
      _panel[PANEL_UNDO_LAST],  "Last", 
      id+BUT_UNDO_LAST, button_cb);
   assert(_button[BUT_UNDO_LAST]);

   
   // COL3
   new GLUI_Column(_panel[PANEL_CONTROLS], false);
   
   _panel[PANEL_STYLE] = new GLUI_Panel(_panel[PANEL_CONTROLS],
               (string("Mode: ") + HatchingGroup::style_mode_name(_pen->params().anim_style())).c_str());
   assert(_panel[PANEL_STYLE]);
   _button[BUT_STYLE] = new GLUI_Button(
      _panel[PANEL_STYLE], "Style", 
      id+BUT_STYLE, button_cb);
   assert(_button[BUT_STYLE]);

   
   _panel[PANEL_APPLY] = new GLUI_Panel(_panel[PANEL_CONTROLS], "Settings");
   assert(_panel[PANEL_APPLY]);
   _button[BUT_APPLY] = new GLUI_Button(
      _panel[PANEL_APPLY],    "Apply", 
      id+BUT_APPLY, button_cb);
   assert(_button[BUT_APPLY]);

   // Fix up sizes

   int new_w = (panel_width - p->get_w() + _slider[SLIDE_LO_THRESH]->get_w() 
                  + _slider[SLIDE_LO_WIDTH]->get_w() + _slider[SLIDE_TRANS_TIME]->get_w() )/3;
   for (i=0; i<SLIDE_NUM; i++)  _slider[i]->set_w(new_w);


   new_w =  (_rollout[ROLLOUT_ANIM]->get_w() - _panel[PANEL_CONTROLS]->get_w() 
               + _button[BUT_TYPE]->get_w() + _button[BUT_CURVES]->get_w() + _button[BUT_STYLE]->get_w())/3;

   for (i=0; i<BUT_NUM; i++) {
      if (i==BUT_DELETE_ALL) {
         _button[i]->set_w(new_w - (_panel[PANEL_DELETE_ALL]->get_w() - _button[BUT_DELETE_ALL]->get_w() 
               - _panel[PANEL_UNDO_LAST]->get_w() + _button[BUT_UNDO_LAST]->get_w()));
      } else {
         _button[i]->set_w(new_w);
      }
   }

   // XXX - Should prolly 'member the old state
   _rollout[ROLLOUT_ANIM]->close();
}

/////////////////////////////////////
// destroy
/////////////////////////////////////

void
HatchingPenUI::destroy(GLUI*,GLUI_Panel *) 
{
   //Hands off these soon to be bad things

   _button.clear();
   _slider.clear();
   _panel.clear(); 
   _checkbox.clear();
   _rollout.clear();
}

/////////////////////////////////////
// changed
/////////////////////////////////////

void
HatchingPenUI::changed()
{
   CBaseStroke *s = StrokeUI::get_params(_pen->view(),this); 
   assert(s); 
   _pen->params().stroke(*s);
}

/////////////////////////////////////
// show()
/////////////////////////////////////

void
HatchingPenUI::show()
{

   if (!StrokeUI::capture(_pen->view(),this))
      {
         cerr << "HatchingPenUI::show() - Error! Failed to capture StrokeUI.\n";
         return;
      }
   else
      {
         CBaseStroke *s = StrokeUI::get_params(_pen->view(),this); assert(s); _pen->params().stroke(*s);
         if (!StrokeUI::show(_pen->view(),this))
            cerr << "HatchingPenUI::show() - Error! Failed to show StrokeUI.\n";
      }

}


/////////////////////////////////////
// hide()
/////////////////////////////////////

void
HatchingPenUI::hide()
{

   if (!StrokeUI::hide(_pen->view(),this))
      {
         cerr << "HatchingPenUI::hide() - Error! Failed to hide StrokeUI.\n";
         return;
      }

   if (!StrokeUI::release(_pen->view(),this))
      cerr << "HatchingPenUI::hide() - Error! Failed to release StrokeUI.\n";

}

/////////////////////////////////////
// update()
/////////////////////////////////////
//
// -Forces GLUI to look at live variables
//  and repost the widgets
//
/////////////////////////////////////

void
HatchingPenUI::update()
{
   _panel[PANEL_TYPE]->set_name( 
         (string("Mode: ") + HatchingGroup::name(_pen->hatch_mode())).c_str());

   _panel[PANEL_STYLE]->set_name( 
         (string("Mode: ") + HatchingGroup::style_mode_name(_pen->params().anim_style())).c_str());

   bool ret = StrokeUI::set_params(_pen->view(), this, &(_pen->params().stroke()));

   assert(ret);

        //Updates the StrokeUI widgets and this UI's widgets
   if (!StrokeUI::update(_pen->view(),this))
      cerr << "HatchingPenUI::update() - Error! Failed to update StrokeUI.\n";

}


/////////////////////////////////////
// button_cb()
/////////////////////////////////////
//
// -Common callback for all buttons
//
/////////////////////////////////////

void
HatchingPenUI::button_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   switch(id&ID_MASK)
   {
      case BUT_TYPE:
         cerr << "HatchingPenUI::button_cb() - Type toggle\n";
         _ui[id >> ID_SHIFT]->_pen->next_hatch_mode();

         _ui[id >> ID_SHIFT]->_panel[PANEL_TYPE]->set_name( 
            (string("Mode: ") + HatchingGroup::name(_ui[id >> ID_SHIFT]->_pen->hatch_mode())).c_str());
         //_ui[id >> ID_SHIFT]->update();
      break;
      case BUT_CURVES:
         cerr << "HatchingPenUI::button_cb() - Curves toggle\n";
         _ui[id >> ID_SHIFT]->_pen->next_curve_mode();

         _ui[id >> ID_SHIFT]->_panel[PANEL_CURVES]->set_name( 
            (string("Mode: ") + HatchingGroup::curve_mode_name(_ui[id >> ID_SHIFT]->_pen->curve_mode())).c_str());
         //_ui[id >> ID_SHIFT]->update();
      break;
      case BUT_STYLE:
         cerr << "HatchingPenUI::button_cb() - Style toggle\n";
         _ui[id >> ID_SHIFT]->_pen->next_style_mode();
         _ui[id >> ID_SHIFT]->_panel[PANEL_STYLE]->set_name(
               (string("Mode: ") + HatchingGroup::style_mode_name(_ui[id >> ID_SHIFT]->_pen->params().anim_style())).c_str());
         //_ui[id >> ID_SHIFT]->update();
      break;
      case BUT_APPLY:
         cerr << "HatchingPenUI::button_cb() - Apply\n";
         _ui[id >> ID_SHIFT]->_pen->apply_parameters();
      break;
      case BUT_DELETE_ALL:
         cerr << "HatchingPenUI::button_cb() - Delete All\n";
         if (_ui[id >> ID_SHIFT]->_delete_confirm == 1) 
         {
            _ui[id >> ID_SHIFT]->_pen->delete_current();
            _ui[id >> ID_SHIFT]->_delete_confirm = 0;
            _ui[id >> ID_SHIFT]->update();
         }
         else
         {
            WORLD::message("You must confirm deletion.");
         }
      break;
      case BUT_UNDO_LAST:
         cerr << "HatchingPenUI::button_cb() - Undo Last\n";
         _ui[id >> ID_SHIFT]->_pen->undo_last();
      break;
   }
}

/////////////////////////////////////
// slider_cb()
/////////////////////////////////////
//
// -Common callback for all spinners
//
/////////////////////////////////////

void
HatchingPenUI::slider_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   float lo,hi;

   switch(id&ID_MASK)
   {
      case SLIDE_LO_THRESH:
         lo = _ui[id >> ID_SHIFT]->_slider[SLIDE_LO_THRESH]->get_float_val();
         hi = _ui[id >> ID_SHIFT]->_slider[SLIDE_HI_THRESH]->get_float_val();
         if (lo > hi)
            _ui[id >> ID_SHIFT]->_slider[SLIDE_LO_THRESH]->set_float_val(hi);
      break;
      case SLIDE_HI_THRESH:
         lo = _ui[id >> ID_SHIFT]->_slider[SLIDE_LO_THRESH]->get_float_val();
         hi = _ui[id >> ID_SHIFT]->_slider[SLIDE_HI_THRESH]->get_float_val();
         if (hi < lo)
            _ui[id >> ID_SHIFT]->_slider[SLIDE_HI_THRESH]->set_float_val(lo);
      break;
      case SLIDE_LO_WIDTH:
      break;
      case SLIDE_HI_WIDTH:
      break;
      case SLIDE_GLOBAL_SCALE:
      break;
      case SLIDE_LIMIT_SCALE:
      break;
      case SLIDE_TRANS_TIME:
      break;
      case SLIDE_MAX_LOD:
      break;
   }
}

/////////////////////////////////////
// checkbox_cb()
/////////////////////////////////////
//
// -Common callback for all checkboxes
//
/////////////////////////////////////

void
HatchingPenUI::checkbox_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   switch(id&ID_MASK) {
    case CHECK_CONFIRM:
      cerr << "HatchingPenUI::checkbox_cb() - Confirm\n";
      break;
   }
}


