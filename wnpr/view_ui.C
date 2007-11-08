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
// ViewUI
////////////////////////////////////////////



//This is relative to JOT_ROOT, and should
//contain ONLY the texture dots for hatching strokes
#define BKGTEX_DIRECTORY         "nprdata/background_textures/"
#define PAPER_DIRECTORY          "nprdata/paper_textures/"
#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "gtex/paper_effect.H"
#include "std/config.H"

#include "view_ui.H"

using namespace mlib;

/*****************************************************************
 * ViewUI
 *****************************************************************/

/////////////////////////////////////
// Static variables
/////////////////////////////////////

ARRAY<ViewUI*>          ViewUI::_ui;
HASH                    ViewUI::_hash(16);

/////////////////////////////////////
// Constructor
/////////////////////////////////////

ViewUI::ViewUI(VIEWptr v) :
      _id(0),
      _init(false),
      _view(v),
      _glui(0)
{
   _ui += this;
   _id = (_ui.num()-1);

   // Defer init() until the first build()

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

ViewUI::~ViewUI()
{
   // XXX - Need to clean up? Nah, we never destroy these

   cerr << "ViewUI::~ViewUI - Error!!! Destructor not implemented.\n";
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
ViewUI::init()
{
   assert(!_init);

   _init = true;

}

/////////////////////////////////////
// fetch() - Implicit Constructor
/////////////////////////////////////

ViewUI*
ViewUI::fetch(CVIEWptr& v)
{
   if (!v) {
      err_msg("ViewUI::fetch() - Error! view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg("ViewUI::fetch() - Error! view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself

   ViewUI* vui = (ViewUI*)_hash.find((long)v->impl());

   if (!vui) {
      vui = new ViewUI(v);
      assert(vui);
      _hash.add((long)v->impl(), (void *)vui);

   }

   return vui;
}

/////////////////////////////////////
// is_vis()
/////////////////////////////////////

bool
ViewUI::is_vis(CVIEWptr& v)
{

   ViewUI* vui;

   if (!(vui = ViewUI::fetch(v))) {
      err_msg("ViewUI::show - Error! Failed to fetch ViewUI!");
      return false;
   }

   return vui->internal_is_vis();

}


/////////////////////////////////////
// show()
/////////////////////////////////////

bool
ViewUI::show(CVIEWptr& v)
{

   ViewUI* vui;

   if (!(vui = ViewUI::fetch(v))) {
      err_msg("ViewUI::show - Error! Failed to fetch ViewUI!");
      return false;
   }

   if (!vui->internal_show()) {
      err_msg("ViewUI::show() - Error! Failed to show ViewUI!");
      return false;
   } else {
      err_msg("ViewUI::show() - ViewUI sucessfully showed ViewUI.");
      return true;
   }

}

/////////////////////////////////////
// hide()
/////////////////////////////////////

bool
ViewUI::hide(CVIEWptr& v)
{

   ViewUI* vui;

   if (!(vui = ViewUI::fetch(v))) {
      err_msg("ViewUI::hide - Error! Failed to fetch ViewUI!");
      return false;
   }

   if (!vui->internal_hide()) {
      err_msg("ViewUI::hide() - Error! Failed to hide ViewUI!");
      return false;
   } else {
      err_msg("ViewUI::hide() - ViewUI sucessfully hid ViewUI.");
      return true;
   }

}

/////////////////////////////////////
// update()
/////////////////////////////////////

bool
ViewUI::update(CVIEWptr& v)
{

   ViewUI* vui;

   if (!(vui = ViewUI::fetch(v))) {
      err_msg("ViewUI::update - Error! Failed to fetch ViewUI!");
      return false;
   }

   if (!vui->internal_update()) {
      err_msg("ViewUI::update() - Error! Failed to update ViewUI!");
      return false;
   } else {
      err_msg("ViewUI::update() - ViewUI sucessfully updated ViewUI.");
      return true;
   }

}

/////////////////////////////////////
// internal_is_vis()
/////////////////////////////////////

bool
ViewUI::internal_is_vis()
{
   if (_glui) {
      return true;
   } else {
      return false;
   }
}



/////////////////////////////////////
// internal_show()
/////////////////////////////////////

bool
ViewUI::internal_show()
{
   if (_glui) {
      cerr << "ViewUI::internal_show() - Error! ViewUI is "
      << "already showing!"
      << endl;
      return false;
   } else {
      build();

      assert(_init);

      if (!_glui) {
         cerr << "ViewUI::internal_show() - Error! ViewUI "
         << "failed to build GLUI object!"
         << endl;
         return false;
      } else {
         _glui->show();

         // Update the controls that don't use
         // 'live' variables
         update_non_lives();

         _glui->sync_live();

         return true;
      }
   }
}


/////////////////////////////////////
// internal_hide()
/////////////////////////////////////

bool
ViewUI::internal_hide()
{
   if (!_glui) {
      cerr << "ViewUI::internal_hide() - Error! ViewUI is "
      << "already hidden!"
      << endl;
      return false;
   } else {
      _glui->hide();

      assert(_init);

      destroy();

      assert(!_glui);

      return true;
   }

}

/////////////////////////////////////
// internal_update()
/////////////////////////////////////
//
// -Forces GLUI to look at live variables
//  and repost the widgets
//
/////////////////////////////////////

bool
ViewUI::internal_update()
{

   if (!_glui) {
      cerr << "ViewUI::internal_update() - Error! "
      << " No GLUI object to update (not showing)!"
      << endl;
      return false;
   } else {
      // Update the controls that don't use
      // 'live' variables
      update_non_lives();

      _glui->sync_live();

      return true;
   }

}


/////////////////////////////////////
// build()
/////////////////////////////////////

void
ViewUI::build()
{
   int i;
   int id = _id << ID_SHIFT;

   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   _view->win()->size(root_w,root_h);
   _view->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("Global Parameters", 0, root_x + root_w + 10, root_y);
   _glui->set_main_gfx_window(_view->win()->id());

   //Init the control arrays
   assert(_listbox.num()==0);
   for (i=0; i<LIST_NUM; i++)     _listbox.add(0);
   assert(_button.num()==0);
   for (i=0; i<BUT_NUM; i++)      _button.add(0);
   assert(_slider.num()==0);
   for (i=0; i<SLIDE_NUM; i++)    _slider.add(0);
   assert(_spinner.num()==0);
   for (i=0; i<SPINNER_NUM; i++)    _spinner.add(0);
   assert(_panel.num()==0);
   for (i=0; i<PANEL_NUM; i++)    _panel.add(0);
   assert(_rollout.num()==0);
   for (i=0; i<ROLLOUT_NUM; i++)  _rollout.add(0);
   assert(_rotation.num()==0);
   for (i=0; i<ROT_NUM; i++)      _rotation.add(0);
   assert(_radgroup.num()==0);
   for (i=0; i<RADGROUP_NUM; i++) _radgroup.add(0);
   assert(_radbutton.num()==0);
   for (i=0; i<RADBUT_NUM; i++)   _radbutton.add(0);
   assert(_checkbox.num()==0);
   for (i=0; i<CHECK_NUM; i++)    _checkbox.add(0);


   assert(_bkgtex_filenames.num() == 0);
   assert(_paper_filenames.num() == 0);


   //Lighting
   _rollout[ROLLOUT_LIGHT] = _glui->add_rollout("Lighting",true);
   assert(_rollout[ROLLOUT_LIGHT]);

   //Top panel
   _panel[PANEL_LIGHTTOP] = _glui->add_panel_to_panel(
                               _rollout[ROLLOUT_LIGHT],
                               "");
   assert(_panel[PANEL_LIGHTTOP]);

   //Number
   _radgroup[RADGROUP_LIGHTNUM] = _glui->add_radiogroup_to_panel(
                                     _panel[PANEL_LIGHTTOP],
                                     NULL,
                                     id+RADGROUP_LIGHTNUM, radiogroup_cb);
   assert(_radgroup[RADGROUP_LIGHTNUM]);

   _radbutton[RADBUT_LIGHT0] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "0");
   assert(_radbutton[RADBUT_LIGHT0]);
   _radbutton[RADBUT_LIGHT1] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "1");
   assert(_radbutton[RADBUT_LIGHT1]);
   _radbutton[RADBUT_LIGHT2] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "2");
   assert(_radbutton[RADBUT_LIGHT2]);
   _radbutton[RADBUT_LIGHT3] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "3");
   _radbutton[RADBUT_LIGHT4] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "4");
   _radbutton[RADBUT_LIGHT5] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "5");
   _radbutton[RADBUT_LIGHT6] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "6");
   _radbutton[RADBUT_LIGHT7] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "7");
   assert(_radbutton[RADBUT_LIGHT2]);
   _radbutton[RADBUT_LIGHTG] = _glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "G");
   assert(_radbutton[RADBUT_LIGHTG]);

   _glui->add_column_to_panel(_panel[PANEL_LIGHTTOP],true);

   //Enable, Position/Direction, Amb/Diff
   _checkbox[CHECK_ENABLE] = _glui->add_checkbox_to_panel(
                                _panel[PANEL_LIGHTTOP],
                                "On",
                                NULL,
                                id+CHECK_ENABLE,
                                checkbox_cb);
   assert(_checkbox[CHECK_ENABLE]);

    _checkbox[CHECK_SHOW_WIDGET] = _glui->add_checkbox_to_panel(
                                _panel[PANEL_LIGHTTOP],
                                "Show Widget",
                                NULL,
                                id+CHECK_SHOW_WIDGET,
                                checkbox_cb);

   _checkbox[CHECK_POS] = _glui->add_checkbox_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "Dir",
                             NULL,
                             id+CHECK_POS,
                             checkbox_cb);
   assert(_checkbox[CHECK_POS]);

   _checkbox[CHECK_CAM] = _glui->add_checkbox_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "Cam",
                             NULL,
                             id+CHECK_CAM,
                             checkbox_cb);
   assert(_checkbox[CHECK_CAM]);

   _radgroup[RADGROUP_LIGHTCOL] = _glui->add_radiogroup_to_panel(
                                     _panel[PANEL_LIGHTTOP],
                                     NULL,
                                     id+RADGROUP_LIGHTCOL, radiogroup_cb);
   assert(_radgroup[RADGROUP_LIGHTCOL]);

   _radbutton[RADBUT_DIFFUSE] = _glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_LIGHTCOL],
                                   "Dif");
   assert(_radbutton[RADBUT_DIFFUSE]);
   _radbutton[RADBUT_AMBIENT] = _glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_LIGHTCOL],
                                   "Amb");
   assert(_radbutton[RADBUT_AMBIENT]);

   _glui->add_column_to_panel(_panel[PANEL_LIGHTTOP],true);

   //Rot
   _rotation[ROT_LIGHT] = _glui->add_rotation_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "Pos/Dir",
                             NULL,
                             id+ROT_LIGHT, rotation_cb);
   assert(_rotation[ROT_LIGHT]);
   _glui->add_column_to_panel(_panel[PANEL_LIGHTTOP],false);

   _spinner[SPINNER_LIGHT_DIR_X] = _glui->add_spinner_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "x",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_LIGHT_DIR_X, spinner_cb);
   _spinner[SPINNER_LIGHT_DIR_Y] = _glui->add_spinner_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "y",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_LIGHT_DIR_Y, spinner_cb);
    _spinner[SPINNER_LIGHT_DIR_Z] = _glui->add_spinner_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "z",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_LIGHT_DIR_Z, spinner_cb);

    _rollout[ROLLOUT_SPOT] = _glui->add_rollout_to_panel( _rollout[ROLLOUT_LIGHT], "Point Light",false);
    
    _panel[PANEL_LIGHT_SPOTDIR] = _glui->add_panel_to_panel( _rollout[ROLLOUT_SPOT], "");
    
    _rotation[ROT_SPOT] = _glui->add_rotation_to_panel(
                             _panel[PANEL_LIGHT_SPOTDIR],
                             "Spot Dir",
                             NULL,
                             id+ROT_SPOT, rotation_cb);
     _glui->add_column_to_panel(_panel[PANEL_LIGHT_SPOTDIR],false);
    _spinner[SPINNER_LIGHT_SPOT_X] = _glui->add_spinner_to_panel(
                              _panel[PANEL_LIGHT_SPOTDIR],
                             "x",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_LIGHT_SPOT_X, spinner_cb);
    _spinner[SPINNER_LIGHT_SPOT_Y] = _glui->add_spinner_to_panel(
                              _panel[PANEL_LIGHT_SPOTDIR],
                             "y",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_LIGHT_SPOT_Y, spinner_cb);
    _spinner[SPINNER_LIGHT_SPOT_Z] = _glui->add_spinner_to_panel(
                              _panel[PANEL_LIGHT_SPOTDIR],
                             "z",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_LIGHT_SPOT_Z, spinner_cb);
    //_glui->add_column_to_panel(_rollout[ROLLOUT_SPOT],true);
    _slider[SLIDE_SPOT_EXPONENT] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Spot Exponent",
                          GLUI_SLIDER_FLOAT,
                          0.0, 5.0,
                          NULL,
                          id+SLIDE_SPOT_EXPONENT, slider_cb);
    
    _slider[SLIDE_SPOT_CUTOFF] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Spot Cutoff",
                          GLUI_SLIDER_INT,
                          0, 90,
                          NULL,
                          id+SLIDE_SPOT_CUTOFF, slider_cb);
    _slider[SLIDE_SPOT_CUTOFF]->set_int_val(90);
    _slider[SLIDE_SPOT_K0] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Constatnt Attenuation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 5.0,
                          NULL,
                          id+SLIDE_SPOT_K0, slider_cb);
    _slider[SLIDE_SPOT_K0]->set_float_val(1.0);
    _slider[SLIDE_SPOT_K1] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Linear Attenuation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_SPOT_K1, slider_cb);
   
    _slider[SLIDE_SPOT_K2] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Quadratic Attenuation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 0.5,
                          NULL,
                          id+SLIDE_SPOT_K2, slider_cb);
   

   //Color
    _slider[SLIDE_LH] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_LIGHT],
                          "Hue",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_LH, slider_cb);
   assert(_slider[SLIDE_LH]);
   _slider[SLIDE_LH]->set_num_graduations(201);

   _slider[SLIDE_LS] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_LIGHT],
                          "Saturation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_LS, slider_cb);
   assert(_slider[SLIDE_LS]);
   _slider[SLIDE_LS]->set_num_graduations(201);

   _slider[SLIDE_LV] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_LIGHT],
                          "Value",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_LV, slider_cb);
   assert(_slider[SLIDE_LV]);
   _slider[SLIDE_LV]->set_num_graduations(201);



   // Background
   _rollout[ROLLOUT_BKG] = _glui->add_rollout("Background",true);
   assert(_rollout[ROLLOUT_BKG]);

   _checkbox[CHECK_PAPER] = _glui->add_checkbox_to_panel(
                               _rollout[ROLLOUT_BKG],
                               "Apply Paper to Bkg",
                               NULL,
                               id+CHECK_PAPER,
                               checkbox_cb);
   assert(_checkbox[CHECK_PAPER]);

   //Tex
   _panel[PANEL_BKGTEX] = _glui->add_panel_to_panel(
                             _rollout[ROLLOUT_BKG],
                             "Texture");
   assert(_panel[PANEL_BKGTEX]);

   _listbox[LIST_BKGTEX] = _glui->add_listbox_to_panel(
                              _panel[PANEL_BKGTEX],
                              "", NULL,
                              id+LIST_BKGTEX, listbox_cb);
   assert(_listbox[LIST_BKGTEX]);
   _listbox[LIST_BKGTEX]->add_item(0, "----");
   fill_bkgtex_listbox(_listbox[LIST_BKGTEX], _bkgtex_filenames, Config::JOT_ROOT() + BKGTEX_DIRECTORY);

   //Color
   _slider[SLIDE_BH] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_BKG],
                          "Hue",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_BH, slider_cb);
   assert(_slider[SLIDE_BH]);
   _slider[SLIDE_BH]->set_num_graduations(201);

   _slider[SLIDE_BS] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_BKG],
                          "Saturation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_BS, slider_cb);
   assert(_slider[SLIDE_BS]);
   _slider[SLIDE_BS]->set_num_graduations(201);

   _slider[SLIDE_BV] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_BKG],
                          "Value",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_BV, slider_cb);
   assert(_slider[SLIDE_BV]);
   _slider[SLIDE_BV]->set_num_graduations(201);

   _slider[SLIDE_BA] = _glui->add_slider_to_panel(
                          _rollout[ROLLOUT_BKG], "Alpha",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          id+SLIDE_BA, slider_cb);
   assert(_slider[SLIDE_BA]);
   _slider[SLIDE_BA]->set_num_graduations(201);


   // Paper
   _rollout[ROLLOUT_PAPER] = _glui->add_rollout("Paper",true);
   assert(_rollout[ROLLOUT_PAPER]);

   _checkbox[CHECK_ACTIVE] = _glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_PAPER],
                                "Active",
                                NULL,
                                id+CHECK_ACTIVE,
                                checkbox_cb);
   assert(_checkbox[CHECK_ACTIVE]);

   _panel[PANEL_PAPER] = _glui->add_panel_to_panel(
                            _rollout[ROLLOUT_PAPER],
                            "Texture");
   assert(_panel[PANEL_PAPER]);

   _listbox[LIST_PAPER] = _glui->add_listbox_to_panel(
                             _panel[PANEL_PAPER],
                             "",
                             NULL,
                             id+LIST_PAPER, listbox_cb);
   assert(_listbox[LIST_PAPER]);
   _listbox[LIST_PAPER]->add_item(0, "----");
   fill_paper_listbox(_listbox[LIST_PAPER], _paper_filenames, Config::JOT_ROOT() );

   _slider[SLIDE_BRIG] = _glui->add_slider_to_panel(
                            _rollout[ROLLOUT_PAPER],
                            "Brightness",
                            GLUI_SLIDER_FLOAT,
                            0.0, 1.0,
                            NULL,
                            id+SLIDE_BRIG, slider_cb);
   assert(_slider[SLIDE_BRIG]);
   _slider[SLIDE_BRIG]->set_num_graduations(101);

   _slider[SLIDE_CONT] = _glui->add_slider_to_panel(
                            _rollout[ROLLOUT_PAPER],
                            "Contrast",
                            GLUI_SLIDER_FLOAT,
                            0.0, 1.0,
                            NULL,
                            id+SLIDE_CONT, slider_cb);
   assert(_slider[SLIDE_CONT]);
   _slider[SLIDE_CONT]->set_num_graduations(101);

   // Antialiasing
   _rollout[ROLLOUT_ANTI] = _glui->add_rollout("Antialiasing",true);
   assert(_rollout[ROLLOUT_ANTI]);

   _checkbox[CHECK_ANTI] = _glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_ANTI],
                              "Active",
                              NULL,
                              id+CHECK_ANTI,
                              checkbox_cb);
   assert(_checkbox[CHECK_ANTI]);

   _panel[PANEL_ANTI] = _glui->add_panel_to_panel(
                           _rollout[ROLLOUT_ANTI],
                           "Mode");
   assert(_panel[PANEL_ANTI]);

   _listbox[LIST_ANTI] = _glui->add_listbox_to_panel(
                            _panel[PANEL_ANTI],
                            "",
                            NULL,
                            id+LIST_ANTI, listbox_cb);
   assert(_listbox[LIST_ANTI]);
   fill_anti_listbox(_listbox[LIST_ANTI]);


   // Suggestive Line detection: image space

   // Check if we're using Pearson & Robinson
   static bool VALLEY = Config::get_var_bool("SUG_VALLEY",false);

   _rollout[ROLLOUT_LINE] =
      _glui->add_rollout(VALLEY ?
                         (char*) "Suggestive - IMG (P&R 5x5)" :
                         (char*) "Suggestive - IMG", true);
   assert(_rollout[ROLLOUT_LINE]);

   // parameters
   _slider[SLIDE_SUGR] = _glui->add_slider_to_panel(
                            _rollout[ROLLOUT_LINE],
                            VALLEY ? (char*) "---" : (char*) "Radius",
                            GLUI_SLIDER_FLOAT,
                            1, 20,
                            NULL,
                            id+SLIDE_SUGR, slider_cb);
   assert(_slider[SLIDE_SUGR]);
   _slider[SLIDE_SUGR]->set_num_graduations(120);

   _slider[SLIDE_SUGT] = _glui->add_slider_to_panel(
                            _rollout[ROLLOUT_LINE],
                            VALLEY ? (char*) "---" : (char*) "Threshold",
                            GLUI_SLIDER_FLOAT,
                            0.0, 2.0,
                            NULL,
                            id+SLIDE_SUGT, slider_cb);
   assert(_slider[SLIDE_SUGT]);
   _slider[SLIDE_SUGT]->set_num_graduations(201);

   _slider[SLIDE_SUGMIN] = _glui->add_slider_to_panel(
                              _rollout[ROLLOUT_LINE],
                              VALLEY ? (char*) "Threshold #1" : (char*) "Min",
                              GLUI_SLIDER_INT,
                              0, 255,
                              NULL,
                              id+SLIDE_SUGMIN, slider_cb);
   assert(_slider[SLIDE_SUGMIN]);
   _slider[SLIDE_SUGMIN]->set_num_graduations(255);

   _slider[SLIDE_SUGMAX] = _glui->add_slider_to_panel(
                              _rollout[ROLLOUT_LINE],
                              VALLEY ? (char*) "Threshold #2" : (char*) "Max",
                              GLUI_SLIDER_INT,
                              0, 255,
                              NULL,
                              id+SLIDE_SUGMAX, slider_cb);
   assert(_slider[SLIDE_SUGMAX]);
   _slider[SLIDE_SUGMAX]->set_num_graduations(255);

   _checkbox[CHECK_SUGLCK] = _glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_LINE],
                                "Check only on lines",
                                NULL,
                                id+CHECK_SUGLCK,
                                checkbox_cb);
   assert(_checkbox[CHECK_SUGLCK]);

   _slider[SLIDE_SUGLWID] = _glui->add_slider_to_panel(
                               _rollout[ROLLOUT_LINE],
                               "Edge width",
                               GLUI_SLIDER_INT,
                               1, 10,
                               NULL,
                               id+SLIDE_SUGLWID, slider_cb);
   assert(_slider[SLIDE_SUGLWID]);
   _slider[SLIDE_SUGLWID]->set_num_graduations(255);

   _checkbox[CHECK_SUGMED] = _glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_LINE],
                                VALLEY ? (char*) "---" : (char*) "Median filter",
                                NULL,
                                id+CHECK_SUGMED,
                                checkbox_cb);
   assert(_checkbox[CHECK_SUGMED]);

   _checkbox[CHECK_SUGKEY] = _glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_LINE],
                                "Keyline",
                                NULL,
                                id+CHECK_SUGKEY,
                                checkbox_cb);
   assert(_checkbox[CHECK_SUGKEY]);

   // -- Suggestive Contours: object space

   _rollout[ROLLOUT_LINE_OBJ] =
      _glui->add_rollout((char*) "Suggestive - OBJ (0 cross)", true);
   assert(_rollout[ROLLOUT_LINE_OBJ]);

   _checkbox[CHECK_SUGOBJ_ENABLE] = _glui->add_checkbox_to_panel(
                                       _rollout[ROLLOUT_LINE_OBJ],
                                       "Enabled",
                                       NULL,
                                       id+CHECK_SUGOBJ_ENABLE,
                                       checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_ENABLE]);

   // parameters

   _checkbox[CHECK_SUGOBJ_C1] = _glui->add_checkbox_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "Apply Tests",
                                   NULL,
                                   id+CHECK_SUGOBJ_C1,
                                   checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_C1]);

   _slider[SLIDE_SUGOBJT2] = _glui->add_slider_to_panel(
                                _rollout[ROLLOUT_LINE_OBJ],
                                (char*) "Grad test threshold",
                                GLUI_SLIDER_FLOAT,
                                -0.1, 0.3,
                                NULL,
                                id+SLIDE_SUGOBJT2, slider_cb);
   assert(_slider[SLIDE_SUGOBJT2]);
   _slider[SLIDE_SUGOBJT2]->set_num_graduations(201);

   _slider[SLIDE_SUGOBJT2a] = _glui->add_slider_to_panel(
                                 _rollout[ROLLOUT_LINE_OBJ],
                                 "Grad test threshold #2",
                                 GLUI_SLIDER_FLOAT,
                                 -0.1, 0.3,
                                 NULL,
                                 id+SLIDE_SUGOBJT2a, slider_cb);
   assert(_slider[SLIDE_SUGOBJT2a]);
   _slider[SLIDE_SUGOBJT2a]->set_num_graduations(201);

   _slider[SLIDE_SUGOBJT3] = _glui->add_slider_to_panel(
                                _rollout[ROLLOUT_LINE_OBJ],
                                (char*) "n dot v thresh",
                                GLUI_SLIDER_FLOAT,
                                0, 1,
                                NULL,
                                id+SLIDE_SUGOBJT3, slider_cb);
   assert(_slider[SLIDE_SUGOBJT3]);
   _slider[SLIDE_SUGOBJT3]->set_num_graduations(201);

   _slider[SLIDE_SUGOBJT3a] = _glui->add_slider_to_panel(
                                 _rollout[ROLLOUT_LINE_OBJ],
                                 "n dot v thresh #2",
                                 GLUI_SLIDER_FLOAT,
                                 0, 1,
                                 NULL,
                                 id+SLIDE_SUGOBJT3a, slider_cb);
   assert(_slider[SLIDE_SUGOBJT3a]);
   _slider[SLIDE_SUGOBJT3a]->set_num_graduations(201);

   _slider[SLIDE_SUGOBJDREG] = _glui->add_slider_to_panel(
                                  _rollout[ROLLOUT_LINE_OBJ],
                                  "Dreg size",
                                  GLUI_SLIDER_INT,
                                  1, 100,
                                  NULL,
                                  id+SLIDE_SUGOBJDREG, slider_cb);
   assert(_slider[SLIDE_SUGOBJDREG]);
   _slider[SLIDE_SUGOBJDREG]->set_num_graduations(101);

   _checkbox[CHECK_SUGOBJ_C2] = _glui->add_checkbox_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "Flip gradient test",
                                   NULL,
                                   id+CHECK_SUGOBJ_C2,
                                   checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_C2]);

   _checkbox[CHECK_SUGOBJ_C5] = _glui->add_checkbox_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "Hysteresis thresholding",
                                   NULL,
                                   id+CHECK_SUGOBJ_C5,
                                   checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_C5]);

   _checkbox[CHECK_SUGOBJ_C3] = _glui->add_checkbox_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "Use curvature normals",
                                   NULL,
                                   id+CHECK_SUGOBJ_C3,
                                   checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_C3]);

   _checkbox[CHECK_SUGOBJ_C6] = _glui->add_checkbox_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "Numerical derivatives",
                                   NULL,
                                   id+CHECK_SUGOBJ_C6,
                                   checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_C6]);


   _slider[SLIDE_SUGOBJWHICH] = _glui->add_slider_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "Kv          MaxK            K               H      n*v",
                                   GLUI_SLIDER_INT,
                                   1, 5,
                                   NULL,
                                   id+SLIDE_SUGOBJWHICH, slider_cb);
   assert(_slider[SLIDE_SUGOBJWHICH]);
   _slider[SLIDE_SUGOBJWHICH]->set_num_graduations(500);

   _checkbox[CHECK_SUGOBJ_C4] = _glui->add_checkbox_to_panel(
                                   _rollout[ROLLOUT_LINE_OBJ],
                                   "grad(f) * V",
                                   NULL,
                                   id+CHECK_SUGOBJ_C4,
                                   checkbox_cb);
   assert(_checkbox[CHECK_SUGOBJ_C4]);

   _slider[SLIDE_SUGOBJT] = _glui->add_slider_to_panel(
                               _rollout[ROLLOUT_LINE_OBJ],
                               (char*) "Zero offset",
                               GLUI_SLIDER_FLOAT,
                               -1,  1,
                               NULL,
                               id+SLIDE_SUGOBJT, slider_cb);
   assert(_slider[SLIDE_SUGOBJT]);
   _slider[SLIDE_SUGOBJT]->set_num_graduations(201);

   _slider[SLIDE_SUGOBJT5] = _glui->add_slider_to_panel(
                                _rollout[ROLLOUT_LINE_OBJ],
                                (char*) "Light thresh",
                                GLUI_SLIDER_FLOAT,
                                -1, 1,
                                NULL,
                                id+SLIDE_SUGOBJT5, slider_cb);
   assert(_slider[SLIDE_SUGOBJT5]);
   _slider[SLIDE_SUGOBJT5]->set_num_graduations(201);

   const int WIN_WIDTH = 300;

   // Cleanup sizes
   int w;

   w = max(_panel[PANEL_LIGHTTOP]->get_w(),  _panel[PANEL_BKGTEX]->get_w());
   w = max(_panel[PANEL_PAPER]->get_w(), w);
   w = max(_panel[PANEL_ANTI]->get_w(), w);
   w = max(WIN_WIDTH, w);

   for (i=0; i<SLIDE_NUM; i++){
      if(_slider[i])   
         _slider[i]->set_w(w);
   }
   /*
        w = max(_listbox[LIST_BKGTEX]->get_w(),_listbox[LIST_PAPER]->get_w());
        w = max(_listbox[LIST_ANTI]->get_w(),w);
   */
   for (i=0; i<LIST_NUM; i++)
      _listbox[i]->set_w(w-20);

//   _rollout[ROLLOUT_LIGHT]->close();
   _rollout[ROLLOUT_LINE]->close();
   _rollout[ROLLOUT_BKG]->close();
   _rollout[ROLLOUT_PAPER]->close();
   _rollout[ROLLOUT_ANTI]->close();

   static bool DOUG = Config::get_var_bool("DOUG",false,false);
   if (!DOUG) {
      _rollout[ROLLOUT_LINE]->disable();
      _rollout[ROLLOUT_LINE_OBJ]->disable();
      _rollout[ROLLOUT_LINE_OBJ]->close();
   } else {
      // close up other rollouts if doing sug stuff
      // (..."Out of my way, I am a motorist!!!")
      _rollout[ROLLOUT_BKG]->close();
      _rollout[ROLLOUT_PAPER]->close();
      _rollout[ROLLOUT_ANTI]->close();
   }

   // Otherwise, just fix up the image size
   if (!_init) init();

   //Show will actually show it...
   _glui->hide();
}

/////////////////////////////////////
// destroy
/////////////////////////////////////

void
ViewUI::destroy()
{

   assert(_glui);

   //Hands off these soon to be bad things

   _listbox.clear();
   _button.clear();
   _slider.clear();
   _panel.clear();
   _rollout.clear();
   _rotation.clear();
   _radgroup.clear();
   _radbutton.clear();
   _checkbox.clear();

   _bkgtex_filenames.clear();
   _paper_filenames.clear();

   //Recursively kills off all controls, and itself
   _glui->close();

   _glui = NULL;


}

/////////////////////////////////////
// fill_anti_listbox()
/////////////////////////////////////

void
ViewUI::fill_anti_listbox(
   GLUI_Listbox *listbox
)
{

   for (int i = 0; i < VIEW::get_jitter_mode_num(); i++) {
      listbox->add_item(i, **(str_ptr("Jittered - ") + str_ptr(VIEW::get_jitter_num(i)) + " Samples"));
   }
}


/////////////////////////////////////
// fill_bkgtex_listbox()
/////////////////////////////////////

void
ViewUI::fill_bkgtex_listbox(
   GLUI_Listbox *listbox,
   str_list     &save_files,
   Cstr_ptr     &full_path
)
{
   int j=0;
   str_list in_files = dir_list(full_path);
   for (int i = 0; i < in_files.num(); i++) {
      int len = in_files[i].len();
      if ( (len>3) &&
           (strcmp(&(**in_files[i])[len-4],".png") == 0)) {
         save_files += full_path + in_files[i];
         listbox->add_item(1+j++, **in_files[i]);
      }
   }
}

/////////////////////////////////////
// fill_paper_listbox()
/////////////////////////////////////
//
// -Retrieve list of files in paper directory
// -Populate the listbox with the filenames
//
/////////////////////////////////////

void
ViewUI::fill_paper_listbox(
   GLUI_Listbox *listbox,
   str_list     &save_files,
   Cstr_ptr     &full_path
)
{
   int j=0;
   str_list in_files = dir_list(full_path + PAPER_DIRECTORY);
   for (int i = 0; i < in_files.num(); i++) {
      int len = in_files[i].len();
      if ( (len>3) &&
           (strcmp(&(**in_files[i])[len-4],".png") == 0)) {
         save_files += str_ptr(PAPER_DIRECTORY) + in_files[i];
         listbox->add_item(1+j++, **in_files[i]);
      }
   }
}


/////////////////////////////////////
// update_non_lives()
/////////////////////////////////////
//
// -Update the controls that changed
//  but don't have 'live' variables
//
/////////////////////////////////////

void
ViewUI::update_non_lives()
{

   update_light();
   update_bkg();
   update_paper();
   update_anti();
   update_lines();
   update_lines_obj();

}

/////////////////////////////////////
// update_lines()
/////////////////////////////////////

void
ViewUI::update_lines()
{
   /*
     int min, max;
     float r, t;
     bool key, med, lck;
     int lwid;
     SugLineTexture::getConfig(r, t, min, max, key, med, lwid, lck);
     _slider[SLIDE_SUGR]->set_float_val(r);
     _slider[SLIDE_SUGT]->set_float_val(t);
     _slider[SLIDE_SUGMIN]->set_int_val(min);
     _slider[SLIDE_SUGMAX]->set_int_val(max);
     _checkbox[CHECK_SUGKEY]->set_int_val(key ? 1 : 0);
     _checkbox[CHECK_SUGMED]->set_int_val(med ? 1 : 0);
     _checkbox[CHECK_SUGLCK]->set_int_val(lck ? 1 : 0);
     _slider[SLIDE_SUGLWID]->set_int_val(lwid);
   */
}


/////////////////////////////////////
// update_lines_obj()
/////////////////////////////////////

void
ViewUI::update_lines_obj()
{
   /*
     SugContoursParam param;

     SugContoursTexture::getConfig(param);
    
     _slider[SLIDE_SUGOBJT]->set_float_val(param.zero_offset);
     _slider[SLIDE_SUGOBJT2]->set_float_val(param.thresh_grad);
     _slider[SLIDE_SUGOBJT3]->set_float_val(param.thresh_ndotv);
     _slider[SLIDE_SUGOBJT5]->set_float_val(param.thresh_light);
     _slider[SLIDE_SUGOBJT2a]->set_float_val(param.thresh_grad2);
     _slider[SLIDE_SUGOBJT3a]->set_float_val(param.thresh_ndotv2);
     _slider[SLIDE_SUGOBJWHICH]->set_int_val(param.which_field);
     _slider[SLIDE_SUGOBJDREG]->set_int_val(param.dregsize);
     _checkbox[CHECK_SUGOBJ_ENABLE]->set_int_val(param.enabled ? 1 : 0);
     _checkbox[CHECK_SUGOBJ_C1]->set_int_val(param.apply_test ? 1 : 0);
     _checkbox[CHECK_SUGOBJ_C2]->set_int_val(param.flip_test ? 1 : 0);
     _checkbox[CHECK_SUGOBJ_C3]->set_int_val(param.curv_normal ? 1 : 0);
     _checkbox[CHECK_SUGOBJ_C4]->set_int_val(param.take_grad ? 1 : 0);
     _checkbox[CHECK_SUGOBJ_C5]->set_int_val(param.canny ? 1 : 0);
     _checkbox[CHECK_SUGOBJ_C6]->set_int_val(param.numerical_diff ? 1 : 0);
   */
}


/////////////////////////////////////
// update_light()
/////////////////////////////////////

void
ViewUI::update_light()
{

   int i = _radgroup[RADGROUP_LIGHTNUM]->get_int_val();

   if (i == RADBUT_LIGHTG) {
      _radgroup[RADGROUP_LIGHTCOL]->disable();
      _checkbox[CHECK_POS]->disable();
      _checkbox[CHECK_CAM]->disable();
      _checkbox[CHECK_ENABLE]->disable();
      _rotation[ROT_LIGHT]->disable();

      COLOR c = _view->light_get_global_ambient();
      HSVCOLOR hsv(c);

      _slider[SLIDE_LH]->set_float_val(hsv[0]);
      _slider[SLIDE_LS]->set_float_val(hsv[1]);
      _slider[SLIDE_LV]->set_float_val(hsv[2]);

   } else {
      _radgroup[RADGROUP_LIGHTCOL]->enable();
      _checkbox[CHECK_POS]->enable();
      _checkbox[CHECK_CAM]->enable();
      _checkbox[CHECK_ENABLE]->enable();     
      _rotation[ROT_LIGHT]->enable();

      _light_dir = _view->light_get_coordinates_v(i);

//      float rad = _light_dir.length();

      _light_dir = _light_dir.normalized();

      _rotation[ROT_LIGHT]->reset();
      _rotation[ROT_SPOT]->reset();

      _spinner[SPINNER_LIGHT_DIR_X]->set_float_val(_light_dir[0]);
      _spinner[SPINNER_LIGHT_DIR_Y]->set_float_val(_light_dir[1]);
      _spinner[SPINNER_LIGHT_DIR_Z]->set_float_val(_light_dir[2]);

      COLOR c = (_radgroup[RADGROUP_LIGHTCOL]->get_int_val()==0)?
                (_view->light_get_diffuse(i)):
                (_view->light_get_ambient(i));

      HSVCOLOR hsv(c);

      _slider[SLIDE_LH]->set_float_val(hsv[0]);
      _slider[SLIDE_LS]->set_float_val(hsv[1]);
      _slider[SLIDE_LV]->set_float_val(hsv[2]);

       _spot_dir =  _view->light_get_spot_direction(i);
         _spinner[SPINNER_LIGHT_SPOT_X]->set_float_val(_spot_dir[0]);
         _spinner[SPINNER_LIGHT_SPOT_Y]->set_float_val(_spot_dir[1]);
         _spinner[SPINNER_LIGHT_SPOT_Z]->set_float_val(_spot_dir[2]);
         _slider[SLIDE_SPOT_EXPONENT]->set_float_val(_view->light_get_spot_exponent(i));
         int cutoff = (int)_view->light_get_spot_cutoff(i);
         cutoff = (cutoff > 90) ? 90 : cutoff;
         _slider[SLIDE_SPOT_CUTOFF]->set_int_val(cutoff);
         _slider[SLIDE_SPOT_K0]->set_float_val(_view->light_get_constant_attenuation(i));
         _slider[SLIDE_SPOT_K1]->set_float_val(_view->light_get_linear_attenuation(i));
         _slider[SLIDE_SPOT_K2]->set_float_val(_view->light_get_quadratic_attenuation(i));

      _checkbox[CHECK_ENABLE]->set_int_val((_view->light_get_enable(i))?(1):(0));
      _checkbox[CHECK_POS]->set_int_val((_view->light_get_positional(i))?(0):(1));

      if(_view->light_get_positional(i)){
         _rollout[ROLLOUT_SPOT]->enable();
         _spinner[SPINNER_LIGHT_SPOT_X]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Y]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Z]->enable();
         _slider[SLIDE_SPOT_EXPONENT]->enable();
         _slider[SLIDE_SPOT_CUTOFF]->enable();
         _slider[SLIDE_SPOT_K0]->enable();
         _slider[SLIDE_SPOT_K1]->enable();
         _slider[SLIDE_SPOT_K2]->enable();
         _rollout[ROLLOUT_SPOT]->open();
        
      }else{
         _rollout[ROLLOUT_SPOT]->disable();
         _rollout[ROLLOUT_SPOT]->close();
      }
      _checkbox[CHECK_CAM]->set_int_val((_view->light_get_in_cam_space(i))?(1):(0));
   }
}

/////////////////////////////////////
// update_bkg()
/////////////////////////////////////

void
ViewUI::update_bkg()
{

   HSVCOLOR hsv(_view->color());

   _slider[SLIDE_BH]->set_float_val(hsv[0]);
   _slider[SLIDE_BS]->set_float_val(hsv[1]);
   _slider[SLIDE_BV]->set_float_val(hsv[2]);

   _slider[SLIDE_BA]->set_float_val(_view->get_alpha());

   _checkbox[CHECK_PAPER]->set_int_val((_view->get_use_paper())?(1):(0));

   int i = _bkgtex_filenames.get_index(_view->get_bkg_file());
   if (i==BAD_IND)
      _listbox[LIST_BKGTEX]->set_int_val(0);
   else
      _listbox[LIST_BKGTEX]->set_int_val(i+1);



}
/////////////////////////////////////
// update_paper()
/////////////////////////////////////

void
ViewUI::update_paper()
{

   int i = _paper_filenames.get_index(PaperEffect::get_paper_tex());
   if (i==BAD_IND)
      _listbox[LIST_PAPER]->set_int_val(0);
   else
      _listbox[LIST_PAPER]->set_int_val(i+1);

   _slider[SLIDE_BRIG]->set_float_val(PaperEffect::get_brig());
   _slider[SLIDE_CONT]->set_float_val(PaperEffect::get_cont());

   _checkbox[CHECK_ACTIVE]->set_int_val((PaperEffect::is_active())?(1):(0));
}

/////////////////////////////////////
// update_anti()
/////////////////////////////////////

void
ViewUI::update_anti()
{

   _listbox[LIST_ANTI]->set_int_val(_view->get_antialias_mode());


   _checkbox[CHECK_ANTI]->set_int_val(_view->get_antialias_enable());
}

/////////////////////////////////////
// apply_light()
/////////////////////////////////////

void
ViewUI::apply_light()
{
   int i = _radgroup[RADGROUP_LIGHTNUM]->get_int_val();

   if (i == RADBUT_LIGHTG) {
      HSVCOLOR hsv(  _slider[SLIDE_LH]->get_float_val(),
                     _slider[SLIDE_LS]->get_float_val(),
                     _slider[SLIDE_LV]->get_float_val());
      COLOR c(hsv);

      _view->light_set_global_ambient(c);
   } else {
      float mat[4][4];
      _rotation[ROT_LIGHT]->get_float_array_val((float *)mat);

      Wvec c0(mat[0][0],mat[0][1],mat[0][2]);
      Wvec c1(mat[1][0],mat[1][1],mat[1][2]);
      Wvec c2(mat[2][0],mat[2][1],mat[2][2]);
      Wtransf t(c0,c1,c2);

      Wvec new_dir = (t*_light_dir).normalized();
      
      _spinner[SPINNER_LIGHT_DIR_X]->set_float_val(new_dir[0]);
      _spinner[SPINNER_LIGHT_DIR_Y]->set_float_val(new_dir[1]);
      _spinner[SPINNER_LIGHT_DIR_Z]->set_float_val(new_dir[2]);

      _view->light_set_coordinates_v(i, new_dir);
      
      HSVCOLOR hsv(  _slider[SLIDE_LH]->get_float_val(),
                     _slider[SLIDE_LS]->get_float_val(),
                     _slider[SLIDE_LV]->get_float_val());

      COLOR c(hsv);

      if (_radgroup[RADGROUP_LIGHTCOL]->get_int_val()==0)
         _view->light_set_diffuse(i,c);
      else
         _view->light_set_ambient(i,c);

      _view->light_set_enable(i,       (_checkbox[CHECK_ENABLE]->get_int_val()) == 1);
      _view->light_set_positional(i,   (_checkbox[CHECK_POS]->get_int_val()) == 0);
      _view->light_set_in_cam_space(i, (_checkbox[CHECK_CAM]->get_int_val()) == 1);

      
      if(_view->light_get_positional(i)){
         _rollout[ROLLOUT_SPOT]->enable();
         _spinner[SPINNER_LIGHT_SPOT_X]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Y]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Z]->enable();
         _slider[SLIDE_SPOT_EXPONENT]->enable();
         _slider[SLIDE_SPOT_CUTOFF]->enable();
         _slider[SLIDE_SPOT_K0]->enable();
         _slider[SLIDE_SPOT_K1]->enable();
         _slider[SLIDE_SPOT_K2]->enable();
         _rollout[ROLLOUT_SPOT]->open();
         
         float mat2[4][4];
         _rotation[ROT_SPOT]->get_float_array_val((float *)mat2);
          Wvec c02(mat2[0][0],mat2[0][1],mat2[0][2]);
          Wvec c12(mat2[1][0],mat2[1][1],mat2[1][2]);
          Wvec c22(mat2[2][0],mat2[2][1],mat2[2][2]);
          Wtransf t2(c02,c12,c22);
          Wvec spot_dir = (t2*_spot_dir).normalized();

          _spinner[SPINNER_LIGHT_SPOT_X]->set_float_val(spot_dir[0]);
          _spinner[SPINNER_LIGHT_SPOT_Y]->set_float_val(spot_dir[1]);
          _spinner[SPINNER_LIGHT_SPOT_Z]->set_float_val(spot_dir[2]);

         _view->light_set_spot_direction(i,spot_dir);
         _view->light_set_spot_exponent(i,_slider[SLIDE_SPOT_EXPONENT]->get_float_val());
         _view->light_set_spot_cutoff(i,_slider[SLIDE_SPOT_CUTOFF]->get_int_val());
         _view->light_set_constant_attenuation(i,_slider[SLIDE_SPOT_K0]->get_float_val());
         _view->light_set_linear_attenuation(i,_slider[SLIDE_SPOT_K1]->get_float_val());
         _view->light_set_quadratic_attenuation(i,_slider[SLIDE_SPOT_K2]->get_float_val());
      }else{
         _view->light_set_spot_direction(i,Wvec(0,0,-1));
         _view->light_set_spot_exponent(i,0);
         _view->light_set_spot_cutoff(i, 180);  
         _view->light_set_constant_attenuation(i,1);
         _view->light_set_linear_attenuation(i,0);
         _view->light_set_quadratic_attenuation(i,0);

         _rollout[ROLLOUT_SPOT]->disable();
         _rollout[ROLLOUT_SPOT]->close();
      }
   }
}


/////////////////////////////////////
// apply_lines()
/////////////////////////////////////

void
ViewUI::apply_lines()
{
   /*
     SugLineTexture::setConfig(_slider[SLIDE_SUGR]->get_float_val(),
     _slider[SLIDE_SUGT]->get_float_val(),
     _slider[SLIDE_SUGMIN]->get_int_val(),
     _slider[SLIDE_SUGMAX]->get_int_val(),
     _checkbox[CHECK_SUGKEY]->get_int_val() == 1,
     _checkbox[CHECK_SUGMED]->get_int_val() == 1,
     _slider[SLIDE_SUGLWID]->get_int_val(),
     _checkbox[CHECK_SUGLCK]->get_int_val() == 1);
   */
}

/////////////////////////////////////
// apply_lines_obj()
/////////////////////////////////////

void
ViewUI::apply_lines_obj()
{
   /*
     SugContoursParam param;

     SugContoursTexture::getConfig(param);
    
     param.enabled = _checkbox[CHECK_SUGOBJ_ENABLE]->get_int_val() == 1;
     param.apply_test = _checkbox[CHECK_SUGOBJ_C1]->get_int_val() == 1;
     param.flip_test = _checkbox[CHECK_SUGOBJ_C2]->get_int_val() == 1;
     param.curv_normal = _checkbox[CHECK_SUGOBJ_C3]->get_int_val() == 1;
     param.take_grad = _checkbox[CHECK_SUGOBJ_C4]->get_int_val() == 1;
     param.canny = _checkbox[CHECK_SUGOBJ_C5]->get_int_val() == 1;
     param.numerical_diff = _checkbox[CHECK_SUGOBJ_C6]->get_int_val() == 1;
     param.which_field = _slider[SLIDE_SUGOBJWHICH]->get_int_val();
     param.dregsize = _slider[SLIDE_SUGOBJDREG]->get_int_val();
     param.zero_offset = _slider[SLIDE_SUGOBJT]->get_float_val();
     param.thresh_grad = _slider[SLIDE_SUGOBJT2]->get_float_val();
     param.thresh_ndotv = _slider[SLIDE_SUGOBJT3]->get_float_val();
     param.thresh_light = _slider[SLIDE_SUGOBJT5]->get_float_val();
     param.thresh_grad2 = _slider[SLIDE_SUGOBJT2a]->get_float_val();
     param.thresh_ndotv2 = _slider[SLIDE_SUGOBJT3a]->get_float_val();

     SugContoursTexture::setConfig(param);
   */
}


/////////////////////////////////////
// apply_bkg()
/////////////////////////////////////

void
ViewUI::apply_bkg()
{

   HSVCOLOR hsv(  _slider[SLIDE_BH]->get_float_val(),
                  _slider[SLIDE_BS]->get_float_val(),
                  _slider[SLIDE_BV]->get_float_val());

   _view->set_color(COLOR(hsv));

   _view->set_alpha( _slider[SLIDE_BA]->get_float_val());

   _view->set_use_paper( (_checkbox[CHECK_PAPER]->get_int_val())==1 );

   int i = _listbox[LIST_BKGTEX]->get_int_val();
   if (i == 0)
      _view->set_bkg_file(NULL_STR);
   else {
      if (_view->get_bkg_file() != _bkgtex_filenames[i-1])
         _view->set_bkg_file(_bkgtex_filenames[i-1]);
   }

}

/////////////////////////////////////
// apply_paper()
/////////////////////////////////////

void
ViewUI::apply_paper()
{
   PaperEffect::set_brig(_slider[SLIDE_BRIG]->get_float_val());
   PaperEffect::set_cont(_slider[SLIDE_CONT]->get_float_val());

   int i = _listbox[LIST_PAPER]->get_int_val();
   if (i == 0)
      PaperEffect::set_paper_tex(NULL_STR);
   else
      PaperEffect::set_paper_tex(_paper_filenames[i-1]);

   i = _checkbox[CHECK_ACTIVE]->get_int_val();

   if (i==1) {
      if (!PaperEffect::is_active())
         PaperEffect::toggle_active();
   } else {
      if (PaperEffect::is_active())
         PaperEffect::toggle_active();
   }
}

/////////////////////////////////////
// apply_anti()
/////////////////////////////////////

void
ViewUI::apply_anti()
{

   _view->set_antialias_mode(_listbox[LIST_ANTI]->get_int_val());

   _view->set_antialias_enable(_checkbox[CHECK_ANTI]->get_int_val()==1);

}

/////////////////////////////////////
// listbox_cb()
/////////////////////////////////////
//
// -Common callback for all listboxes
//
/////////////////////////////////////

void
ViewUI::listbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK) {
   case LIST_BKGTEX:
      cerr << "ViewUI::listbox_cb() - BkgTex\n";
      _ui[id >> ID_SHIFT]->apply_bkg();
      break;
   case LIST_PAPER:
      cerr << "ViewUI::listbox_cb() - Paper\n";
      _ui[id >> ID_SHIFT]->apply_paper();
      break;
   case LIST_ANTI:
      cerr << "ViewUI::listbox_cb() - Antialias\n";
      _ui[id >> ID_SHIFT]->apply_anti();
      _ui[id >> ID_SHIFT]->update_anti();
      break;

   }
}

/////////////////////////////////////
// button_cb()
/////////////////////////////////////

void
ViewUI::button_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK) {
   case BUT_DEBUG:
      cerr << "StrokeUI::button_cb() - Debug\n";
      break;
   }
}

/////////////////////////////////////
// slider_cb()
/////////////////////////////////////

void
ViewUI::slider_cb(int id)
{
//   static bool HACK_EYE_POS = Config::get_var_bool("HACK_EYE_POS",false);
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK) {
   case SLIDE_LH:
   case SLIDE_LS:
   case SLIDE_LV:
   case SLIDE_SPOT_EXPONENT:
   case SLIDE_SPOT_CUTOFF:
   case SLIDE_SPOT_K0:
   case SLIDE_SPOT_K1:
   case SLIDE_SPOT_K2:
      _ui[id >> ID_SHIFT]->apply_light();
      break;
   case SLIDE_BH:
   case SLIDE_BS:
   case SLIDE_BV:
   case SLIDE_BA:
      _ui[id >> ID_SHIFT]->apply_bkg();
      break;
   case SLIDE_BRIG:
   case SLIDE_CONT:
      _ui[id >> ID_SHIFT]->apply_paper();
      break;

   case SLIDE_SUGR:
   case SLIDE_SUGT:
   case SLIDE_SUGMIN:
   case SLIDE_SUGMAX:
   case SLIDE_SUGLWID:
      _ui[id >> ID_SHIFT]->apply_lines();
      break;

   case SLIDE_SUGOBJT:
   case SLIDE_SUGOBJT2:
   case SLIDE_SUGOBJT3:
   case SLIDE_SUGOBJT5:
   case SLIDE_SUGOBJT2a:
   case SLIDE_SUGOBJT3a:
   case SLIDE_SUGOBJWHICH:
   case SLIDE_SUGOBJDREG:
      _ui[id >> ID_SHIFT]->apply_lines_obj();
      break;
   }
}

/////////////////////////////////////
// spinner_cb()
/////////////////////////////////////
void 
ViewUI::spinner_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());
   int i = _ui[id >> ID_SHIFT]->_radgroup[RADGROUP_LIGHTNUM]->get_int_val();
   Wvec new_dir;
   switch (id&ID_MASK) {
      case SPINNER_LIGHT_DIR_X:
         _ui[id >> ID_SHIFT]->_light_dir[0] = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LIGHT_DIR_X]->get_float_val();
         new_dir = (_ui[id >> ID_SHIFT]->_light_dir).normalized();
         _ui[id >> ID_SHIFT]->_view->light_set_coordinates_v(i, new_dir);
         break;
      case SPINNER_LIGHT_DIR_Y:
         _ui[id >> ID_SHIFT]->_light_dir[1] = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LIGHT_DIR_Y]->get_float_val();
         new_dir = (_ui[id >> ID_SHIFT]->_light_dir).normalized();
         _ui[id >> ID_SHIFT]->_view->light_set_coordinates_v(i, new_dir);
         break;
      case SPINNER_LIGHT_DIR_Z:
         _ui[id >> ID_SHIFT]->_light_dir[2] = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LIGHT_DIR_Z]->get_float_val();
         new_dir = (_ui[id >> ID_SHIFT]->_light_dir).normalized();
         _ui[id >> ID_SHIFT]->_view->light_set_coordinates_v(i, new_dir);
         break;
      case SPINNER_LIGHT_SPOT_X:
         _ui[id >> ID_SHIFT]->_spot_dir[0] = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LIGHT_SPOT_X]->get_float_val();
         new_dir = (_ui[id >> ID_SHIFT]->_spot_dir).normalized();
         _ui[id >> ID_SHIFT]->_view->light_set_spot_direction(i, new_dir);
         break;
      case SPINNER_LIGHT_SPOT_Y:
          _ui[id >> ID_SHIFT]->_spot_dir[1] = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LIGHT_SPOT_Y]->get_float_val();
         new_dir = (_ui[id >> ID_SHIFT]->_spot_dir).normalized();
         _ui[id >> ID_SHIFT]->_view->light_set_spot_direction(i, new_dir);
         break;
      case SPINNER_LIGHT_SPOT_Z:
          _ui[id >> ID_SHIFT]->_spot_dir[2] = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LIGHT_SPOT_Z]->get_float_val();
         new_dir = (_ui[id >> ID_SHIFT]->_spot_dir).normalized();
         _ui[id >> ID_SHIFT]->_view->light_set_spot_direction(i, new_dir);
         break;
      }
}

/////////////////////////////////////
// radiogroup_cb()
/////////////////////////////////////

void
ViewUI::radiogroup_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK) {
   case RADGROUP_LIGHTNUM:
   case RADGROUP_LIGHTCOL:
      _ui[id >> ID_SHIFT]->update_light();
      break;
   }
}

/////////////////////////////////////
// checkbox_cb()
/////////////////////////////////////

void
ViewUI::checkbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK) {
   case CHECK_POS:
      _ui[id >> ID_SHIFT]->_rotation[ROT_LIGHT]->reset();
   case CHECK_ENABLE:
   case CHECK_CAM:
      _ui[id >> ID_SHIFT]->apply_light();
      break;
   case CHECK_PAPER:
      _ui[id >> ID_SHIFT]->apply_bkg();
      break;
   case CHECK_ACTIVE:
      _ui[id >> ID_SHIFT]->apply_paper();
      _ui[id >> ID_SHIFT]->update_paper();
      break;
   case CHECK_ANTI:
      _ui[id >> ID_SHIFT]->apply_anti();
      _ui[id >> ID_SHIFT]->update_anti();
      break;
   case CHECK_SUGKEY:
   case CHECK_SUGMED:
   case CHECK_SUGLCK:
      _ui[id >> ID_SHIFT]->apply_lines();
      break;
   case CHECK_SUGOBJ_ENABLE:
   case CHECK_SUGOBJ_C1:
   case CHECK_SUGOBJ_C2:
   case CHECK_SUGOBJ_C3:
   case CHECK_SUGOBJ_C4:
   case CHECK_SUGOBJ_C5:
   case CHECK_SUGOBJ_C6:
      _ui[id >> ID_SHIFT]->apply_lines_obj();
      break;
   }
}

/////////////////////////////////////
// rotation_cb()
/////////////////////////////////////

void
ViewUI::rotation_cb(int id)
{
   static bool HACK_EYE_POS = Config::get_var_bool("HACK_EYE_POS",false);

   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK) {
   case ROT_LIGHT:
      _ui[id >> ID_SHIFT]->apply_light();
      if (HACK_EYE_POS)
         VIEW::peek_cam()->data()->changed();
      break;
   case ROT_SPOT:
      _ui[id >> ID_SHIFT]->apply_light();
      if (HACK_EYE_POS)
         VIEW::peek_cam()->data()->changed();
      break;   
   }
}

// view_ui.C
