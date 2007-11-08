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
// NPRPenUI
////////////////////////////////////////////

//This is relative to JOT_ROOT, and should
//contain ONLY the texture dots for hatching strokes
#define TOON_DIRECTORY         "nprdata/toon_textures/"
#define OTHER_DIRECTORY        "nprdata/other_textures/"
#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
// #include "draw/draw.H"
#include "base_jotapp/base_jotapp.H"
#include "npr_pen.H"
#include "npr_pen_ui.H"
#include "tess/tex_body.H"
#include "std/config.H"

#include "npr/npr_texture.H"
#include "npr/npr_bkg_texture.H"
#include "npr/npr_solid_texture.H"
#include "gtex/glsl_xtoon.H"
#include "gtex/xtoon_texture.H"

using namespace mlib;

/*****************************************************************
 * NPRPenUI
 *****************************************************************/

/////////////////////////////////////
// Static variables
/////////////////////////////////////

ARRAY<NPRPenUI*>          NPRPenUI::_ui;

/////////////////////////////////////
// Constructor
/////////////////////////////////////

NPRPenUI::NPRPenUI(NPRPen *p) :
   _id(0),
   _init(false),
   _glui(0),
   _pen(p),
   _light(0,0,-1) {


  _ui += this;
  _id = (_ui.num()-1);
  
  for (int i=0 ; i<5 ; i++){
    _old_target_value[i] = 0.0f;
    _old_factor_value[i] = 0.0f;
  }
    
   // Defer init() until the first build()

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

NPRPenUI::~NPRPenUI()
{
   // XXX - Need to clean up? Nah, we never destroy these
   cerr << "NPRPenUI::~NPRPenUI - Error!!! Destructor not implemented.\n";
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
NPRPenUI::init()
{
   assert(!_init);

   _init = true;

}

////////////////////////////////////
// show()
/////////////////////////////////////

void
NPRPenUI::show()
{
   if (_glui) 
   {
      cerr << "NPRPenUI::show() - Error! NPRPenUI is "
           << "already showing!" << endl;
   } 
   else 
   {
      build();
      
      assert(_init);

      if (!_glui) 
      {
         cerr << "NPRPenUI::internal_show() - Error! NPRPenUI "
              << "failed to build GLUI object!" << endl;
      } 
      else 
      {
         _glui->show();

         // Update the controls that don't use
         // 'live' variables
         update_non_lives();

         _glui->sync_live();

      }
   }
}


/////////////////////////////////////
// hide()
/////////////////////////////////////

void
NPRPenUI::hide()
{
   if(!_glui) 
   {
      cerr << "NPRPenUI::internal_hide() - Error! NPRPenUI is "
           << "already hidden!" << endl;
   }
   else
   {
      _glui->hide();

      assert(_init);
 
      destroy();

      assert(!_glui);

   }

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
NPRPenUI::update()
{
   if(!_glui) 
   {
      cerr << "NPRPenUI::update() - Error! "
           << " No GLUI object to update (not showing)!" << endl;
   }
   else 
   {
      // Update the controls that don't use
      // 'live' variables
      update_non_lives();

      _glui->sync_live();
   }

}


/////////////////////////////////////
// build()
/////////////////////////////////////

void
NPRPenUI::build() 
{
   int i;
   int id = _id << ID_SHIFT;

   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   _pen->view()->win()->size(root_w,root_h);
   _pen->view()->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("Basecoat Editor", 0, root_x+root_w+10, root_y);
   _glui->set_main_gfx_window(_pen->view()->win()->id());

   //Init the control arrays
   assert(_listbox.num()==0);      for (i=0; i<LIST_NUM; i++)     _listbox.add(0);
   assert(_button.num()==0);       for (i=0; i<BUT_NUM; i++)      _button.add(0);
   assert(_slider.num()==0);       for (i=0; i<SLIDE_NUM; i++)    _slider.add(0);
   assert(_panel.num()==0);        for (i=0; i<PANEL_NUM; i++)    _panel.add(0);
   assert(_rollout.num()==0);      for (i=0; i<ROLLOUT_NUM; i++)  _rollout.add(0);
   assert(_rotation.num()==0);     for (i=0; i<ROT_NUM; i++)      _rotation.add(0);
   assert(_translation.num()==0);  for (i=0; i<TRAN_NUM; i++)     _translation.add(0);
   assert(_scale.num()==0);        for (i=0; i<SCALE_NUM; i++)    _scale.add(0);
   assert(_radgroup.num()==0);     for (i=0; i<RADGROUP_NUM; i++) _radgroup.add(0);
   assert(_radbutton.num()==0);    for (i=0; i<RADBUT_NUM; i++)   _radbutton.add(0);
   assert(_checkbox.num()==0);     for (i=0; i<CHECK_NUM; i++)    _checkbox.add(0);
   assert(_edittext.num()==0);     for (i=0; i<EDIT_NUM; i++)     _edittext.add(0);

   assert(_toon_filenames.num() == 0);
   assert(_other_filenames.num() == 0);

   _toon_filenames = dir_list(Config::JOT_ROOT()+TOON_DIRECTORY);
   _other_filenames = dir_list(Config::JOT_ROOT()+OTHER_DIRECTORY);

   // Panel containing pen buttons
   _panel[PANEL_PEN] = _glui->add_panel("");
   assert(_panel[PANEL_PEN]);

   //Prev pen
   _button[BUT_PREV_PEN] = _glui->add_button_to_panel(
      _panel[PANEL_PEN], "Previous Mode", 
      id+BUT_PREV_PEN, button_cb);
   assert(_button[BUT_PREV_PEN]);

   _glui->add_column_to_panel(_panel[PANEL_PEN],false);

   //Prev pen
   _button[BUT_NEXT_PEN] = _glui->add_button_to_panel(
      _panel[PANEL_PEN], "Next Mode", 
      id+BUT_NEXT_PEN, button_cb);
   assert(_button[BUT_NEXT_PEN]);

   //Gtex
   _rollout[ROLLOUT_GTEX] = _glui->add_rollout("Basecoat",true);
   assert(_rollout[ROLLOUT_GTEX]);

   //Global panel
   _panel[PANEL_GTEX_GLOBS] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_GTEX],
      "");
   assert(_panel[PANEL_GTEX_GLOBS]);

   //Transparency
   _checkbox[CHECK_TRANS] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_GLOBS],
      "Transparency to Background ",
      NULL,
      id+CHECK_TRANS,
      checkbox_cb);
   assert(_checkbox[CHECK_TRANS]);

   _glui->add_column_to_panel(_panel[PANEL_GTEX_GLOBS],false);

   //Annotatable
   _checkbox[CHECK_ANNOTATE] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_GLOBS],
      "Annotatable ",
      NULL,
      id+CHECK_ANNOTATE,
      checkbox_cb);
   assert(_checkbox[CHECK_ANNOTATE]);

   //Layer Selection
   _panel[PANEL_GTEX_LAYER_NAME] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Layer Editor");
   assert(_panel[PANEL_GTEX_LAYER_NAME]);

   _listbox[LIST_LAYER] = _glui->add_listbox_to_panel(
      _panel[PANEL_GTEX_LAYER_NAME], 
      "", NULL,
      id+LIST_LAYER, listbox_cb);
   assert(_listbox[LIST_LAYER]);
   _listbox[LIST_LAYER]->set_w(172);

   _edittext[EDIT_NAME] = _glui->add_edittext_to_panel(
      _panel[PANEL_GTEX_LAYER_NAME],
      "", GLUI_EDITTEXT_TEXT,
      NULL, 
      id+EDIT_NAME, edittext_cb);
   assert(_edittext[EDIT_NAME]);
   _edittext[EDIT_NAME]->set_w(172);

  ////////////////////////////////////////////
  //RadGroup for selecting which shader to add
  ////////////////////////////////////////////
   _glui->add_column_to_panel(_panel[PANEL_GTEX_LAYER_NAME],false);

   //take out for space
   //_glui->add_separator_to_panel(_panel[PANEL_GTEX_LAYER_NAME]);

   _radgroup[RADGROUP_GTEX] = _glui->add_radiogroup_to_panel(
      _panel[PANEL_GTEX_LAYER_NAME],
      NULL,
      id+RADGROUP_GTEX, radiogroup_cb);
   assert(_radgroup[RADGROUP_GTEX]);

   //Normal NPR shader radbutton
   _radbutton[RADBUT_SOLID] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_GTEX],
      "Norm");
   assert(_radbutton[RADBUT_SOLID]);
   _radbutton[RADBUT_SOLID]->set_w(5);

   //Regular X-Toon shader radbutton
   _radbutton[RADBUT_TOON] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_GTEX],
      "'Toon");
   assert(_radbutton[RADBUT_TOON]);
   _radbutton[RADBUT_TOON]->set_w(5);

   //GLSL X-Toon shader radbutton
   _radbutton[RADBUT_XTOON] = _glui->add_radiobutton_to_group(
   _radgroup[RADGROUP_GTEX],
      "XToon");
    assert(_radbutton[RADBUT_XTOON]);
   _radbutton[RADBUT_XTOON]->set_w(5);

   /////////////////////////////////////////
   //Column for adding and removing textures
   /////////////////////////////////////////
   _glui->add_column_to_panel(_panel[PANEL_GTEX_LAYER_NAME],false);
   _button[BUT_ADD] = _glui->add_button_to_panel(
      _panel[PANEL_GTEX_LAYER_NAME],
      "Add",
      id+BUT_ADD,
      button_cb);
   assert(_button[BUT_ADD]);
   _button[BUT_ADD]->set_w(45);

   _button[BUT_DEL] = _glui->add_button_to_panel(
      _panel[PANEL_GTEX_LAYER_NAME],
      "Del",
      id+BUT_DEL,
      button_cb);
   assert(_button[BUT_DEL]);
   _button[BUT_DEL]->set_w(45);


   //Layer Opts
   _panel[PANEL_GTEX_LAYER_OPTS] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_GTEX],
      "");
   assert(_panel[PANEL_GTEX_LAYER_OPTS]);

   _checkbox[CHECK_PAPER] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_LAYER_OPTS],
      "Apply Paper Effect",
      NULL,
      id+CHECK_PAPER,
      checkbox_cb);
   assert(_checkbox[CHECK_PAPER]);

   _checkbox[CHECK_TRAVEL] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_LAYER_OPTS],
      "Mobile Paper Co-ords",
      NULL,
      id+CHECK_TRAVEL,
      checkbox_cb);
   assert(_checkbox[CHECK_TRAVEL]);

   _glui->add_column_to_panel(_panel[PANEL_GTEX_LAYER_OPTS],false);

   _checkbox[CHECK_LIGHT] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_LAYER_OPTS],
      "Use Global Lighting",
      NULL,
      id+CHECK_LIGHT,
      checkbox_cb);
   assert(_checkbox[CHECK_LIGHT]);

   _checkbox[CHECK_SPEC] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_LAYER_OPTS],
      "Specular Lighting",
      NULL,
      id+CHECK_SPEC,
      checkbox_cb);
   assert(_checkbox[CHECK_SPEC]);


   //Light (toon)
   _rollout[ROLLOUT_LIGHT] = _glui->add_rollout_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Toon Lighting", true);
   assert(_rollout[ROLLOUT_LIGHT]);

   _radgroup[RADGROUP_LIGHT] = _glui->add_radiogroup_to_panel(
      _rollout[ROLLOUT_LIGHT],
      NULL,
      id+RADGROUP_LIGHT, radiogroup_cb);
   assert(_radgroup[RADGROUP_LIGHT]);

   _radbutton[RADBUT_LIGHT0] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_LIGHT],
      "L0");
   assert(_radbutton[RADBUT_LIGHT0]);
   _radbutton[RADBUT_LIGHT0]->set_w(5);

   _radbutton[RADBUT_LIGHT1] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_LIGHT],
      "L1");
   assert(_radbutton[RADBUT_LIGHT1]);
   _radbutton[RADBUT_LIGHT1]->set_w(5);

   _radbutton[RADBUT_LIGHT2] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_LIGHT],
      "L2");
   assert(_radbutton[RADBUT_LIGHT2]);
   _radbutton[RADBUT_LIGHT2]->set_w(5);

   _radbutton[RADBUT_LIGHT3] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_LIGHT],
      "L3");
   assert(_radbutton[RADBUT_LIGHT3]);
   _radbutton[RADBUT_LIGHT3]->set_w(5);

   _radbutton[RADBUT_LIGHTC] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_LIGHT],
      "L?");
   assert(_radbutton[RADBUT_LIGHTC]);
   _radbutton[RADBUT_LIGHTC]->set_w(5);

   _glui->add_column_to_panel(_rollout[ROLLOUT_LIGHT],false);

   _panel[PANEL_GTEX_LIGHT_OPTS] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_LIGHT],
      "");
   assert(_panel[PANEL_GTEX_LIGHT_OPTS]);

   _checkbox[CHECK_DIR] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_LIGHT_OPTS],
      "Direction",
      NULL,
      id+CHECK_DIR,
      checkbox_cb);
   assert(_checkbox[CHECK_DIR]);

   _glui->add_column_to_panel(_panel[PANEL_GTEX_LIGHT_OPTS],false);

   _checkbox[CHECK_CAM] = _glui->add_checkbox_to_panel(
      _panel[PANEL_GTEX_LIGHT_OPTS],
      "Cam Frame",
      NULL,
      id+CHECK_CAM,
      checkbox_cb);
   assert(_checkbox[CHECK_CAM]);

   _slider[SLIDE_R] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_LIGHT], 
      "Radius", 
      GLUI_SLIDER_FLOAT, 
      0, 1000,
      NULL,
      id+SLIDE_R, slider_cb);
   assert(_slider[SLIDE_R]);
   _slider[SLIDE_R]->set_num_graduations(1001);
   _slider[SLIDE_R]->set_w(200);

   _glui->add_column_to_panel(_rollout[ROLLOUT_LIGHT],false);

   //_glui->add_statictext_to_panel(_rollout[ROLLOUT_LIGHT]," ")->set_w(1);

   _rotation[ROT_L] = _glui->add_rotation_to_panel(
      _rollout[ROLLOUT_LIGHT],
      "Dir/Pos",
      NULL,
      id+ROT_L, rotation_cb);
   assert(_rotation[ROT_L]);

   //Toon
    _rollout[ROLLOUT_TOON] = _glui->add_rollout_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Toon Params", true);
   assert(_rollout[ROLLOUT_TOON]);  

   _listbox[LIST_DETAIL] = _glui->add_listbox_to_panel(
      _rollout[ROLLOUT_TOON], 
      "", NULL,
      id+LIST_DETAIL, listbox_cb);
   assert(_listbox[LIST_DETAIL]);
   _listbox[LIST_DETAIL]->set_w(172);
   _listbox[LIST_DETAIL]->add_item(0, "User");
   _listbox[LIST_DETAIL]->add_item(1, "LOD");
   _listbox[LIST_DETAIL]->add_item(2, "Depth-of-field");
   _listbox[LIST_DETAIL]->add_item(3, "Motion blur");
   _listbox[LIST_DETAIL]->add_item(4, "Backlights");
   _listbox[LIST_DETAIL]->add_item(5, "Highlights");

   _slider[SLIDE_T] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_TOON], 
      "Target size", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_T, slider_cb);
   assert(_slider[SLIDE_T]);
   _slider[SLIDE_T]->set_num_graduations(201);


    _slider[SLIDE_F] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_TOON], 
      "Max factor", 
      GLUI_SLIDER_FLOAT, 
      1.1, 50.0,
      NULL,
      id+SLIDE_F, slider_cb);
   assert(_slider[SLIDE_F]);
   _slider[SLIDE_F]->set_num_graduations(201);
   //////////////////////////////////
   // Panel containing normal buttons
   //////////////////////////////////
   _panel[PANEL_NORMALS] = _glui->add_panel_to_panel(_rollout[ROLLOUT_TOON],"Normals");
   assert(_panel[PANEL_NORMALS]);
   _glui->add_column_to_panel(_panel[PANEL_NORMALS],false);
   //----------------------------
   //rollout of normal map to use
   _listbox[LIST_DETAIL2] = _glui->add_listbox_to_panel(
      _rollout[ROLLOUT_TOON], 
      "", NULL,
      id+LIST_DETAIL2, listbox_cb);
   assert(_listbox[LIST_DETAIL2]);
   _listbox[LIST_DETAIL2]->set_w(172);
   _listbox[LIST_DETAIL2]->add_item(0, "User");
   _listbox[LIST_DETAIL2]->add_item(1, "LOD");
   _listbox[LIST_DETAIL2]->add_item(2, "Depth-of-field");
   _listbox[LIST_DETAIL2]->add_item(3, "Motion blur");
   _listbox[LIST_DETAIL2]->add_item(4, "Backlights");
   _listbox[LIST_DETAIL2]->add_item(5, "Highlights");

  //-----------------------------
  //Smooth and Elliptic Button
  _glui->add_column_to_panel(_panel[PANEL_NORMALS],false);

    _button[BUT_SMOOTH] = _glui->add_button_to_panel(
      _panel[PANEL_NORMALS], "Smooth", 
      id+BUT_SMOOTH, button_cb);
    assert(_button[BUT_SMOOTH]);
 
    _button[BUT_ELLIPTIC] = _glui->add_button_to_panel(
      _panel[PANEL_NORMALS], "Elliptic", 
      id+BUT_ELLIPTIC, button_cb);
    assert(_button[BUT_ELLIPTIC]);

  //-----------------------------
  //Spheric and Cylindric Button
    _glui->add_column_to_panel(_panel[PANEL_NORMALS],false);

    _button[BUT_SPHERIC] = _glui->add_button_to_panel(
      _panel[PANEL_NORMALS], "Spheric", 
      id+BUT_SPHERIC, button_cb);
    assert(_button[BUT_SPHERIC]);

    _button[BUT_CYLINDRIC] = _glui->add_button_to_panel(
      _panel[PANEL_NORMALS], "Cylindric", 
      id+BUT_CYLINDRIC, button_cb);
    assert(_button[BUT_CYLINDRIC]);
 

//     _button[BUT_CURV] = _glui->add_button_to_panel(
//       _rollout[ROLLOUT_TOON], "Compute curvatures", 
//       id+BUT_CURV, button_cb);
//    assert(_button[BUT_CURV]);

   _slider[SLIDE_L] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_TOON], 
      "Smooth factor", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_L, slider_cb);
   assert(_slider[SLIDE_L]);
   _slider[SLIDE_L]->set_num_graduations(201);



  //Color
   _slider[SLIDE_H] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_GTEX], 
      "Hue", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_H, slider_cb);
   assert(_slider[SLIDE_H]);
   _slider[SLIDE_H]->set_num_graduations(201);

   _slider[SLIDE_S] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Saturation", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_S, slider_cb);
   assert(_slider[SLIDE_S]);
   _slider[SLIDE_S]->set_num_graduations(201);

   _slider[SLIDE_V] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Value", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_V, slider_cb);
   assert(_slider[SLIDE_V]);
   _slider[SLIDE_V]->set_num_graduations(201);

   _slider[SLIDE_A] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_GTEX], "Alpha", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_A, slider_cb);
   assert(_slider[SLIDE_A]);
   _slider[SLIDE_A]->set_num_graduations(201);

   //Tex
   _panel[PANEL_TEX] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Texture");
   assert(_panel[PANEL_TEX]);

   _checkbox[CHECK_INV] = _glui->add_checkbox_to_panel(
      _panel[PANEL_TEX],
      "Invert detail",
      NULL,
      id+CHECK_INV,
      checkbox_cb);
   _checkbox[CHECK_INV]->set_int_val(0);
   assert(_checkbox[CHECK_INV]);

   _listbox[LIST_TEX] = _glui->add_listbox_to_panel(
      _panel[PANEL_TEX], 
      "", NULL,
      id+LIST_TEX, listbox_cb);
   assert(_listbox[LIST_TEX]);

   _button[BUT_REFRESH] = _glui->add_button_to_panel(
      _rollout[ROLLOUT_GTEX],
      "Refresh",
      id+BUT_REFRESH,
      button_cb);
   assert(_button[BUT_REFRESH]);

   //Xform
   _rollout[ROLLOUT_XFORM] = _glui->add_rollout("Transformation",true);
   assert(_rollout[ROLLOUT_XFORM]);

   _panel[PANEL_XFORM] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_XFORM],
      "");
   assert(_panel[PANEL_XFORM]);

   //Rot
   _rotation[ROT_ROT] = _glui->add_rotation_to_panel(
      _panel[PANEL_XFORM],
      "Rotation",
      NULL,
      id+ROT_ROT, rotation_cb);
   assert(_rotation[ROT_ROT]);

   _glui->add_column_to_panel(_panel[PANEL_XFORM],false);

   _translation[TRAN_X] = _glui->add_translation_to_panel(
      _panel[PANEL_XFORM],
      "X", GLUI_TRANSLATION_X,
      NULL, 
      id+TRAN_X, translation_cb);
   assert(_translation[TRAN_X]);
   _translation[TRAN_X]->set_speed(0.1);

   _glui->add_column_to_panel(_panel[PANEL_XFORM],false);

   _translation[TRAN_Y] = _glui->add_translation_to_panel(
      _panel[PANEL_XFORM],
      "Y", GLUI_TRANSLATION_Y,
      NULL, 
      id+TRAN_Y, translation_cb);
   assert(_translation[TRAN_Y]);
   _translation[TRAN_Y]->set_speed(0.1);

   _glui->add_column_to_panel(_panel[PANEL_XFORM],false);

   _translation[TRAN_Z] = _glui->add_translation_to_panel(
      _panel[PANEL_XFORM],
      "Z", GLUI_TRANSLATION_Z,
      NULL, 
      id+TRAN_Z, translation_cb);
   assert(_translation[TRAN_Z]);
   _translation[TRAN_Z]->set_speed(0.1);   

   _glui->add_column_to_panel(_panel[PANEL_XFORM],false);

   _scale[SCALE_UNIFORM] = _glui->add_translation_to_panel(
      _panel[PANEL_XFORM],
      "Scale", GLUI_TRANSLATION_Y,
      NULL, 
      id+SCALE_UNIFORM, scale_cb);
   assert(_scale[SCALE_UNIFORM]);
   _scale[SCALE_UNIFORM]->set_y(1.0);
   _scale[SCALE_UNIFORM]->set_speed(0.1);


   //Some temp hacks
   _rollout[ROLLOUT_HACK] = _glui->add_rollout_to_panel(
      _rollout[ROLLOUT_XFORM],
      "Hacks", true);
   assert(_rollout[ROLLOUT_HACK]);

   // Center at origin
   _checkbox[CHECK_CENTER] = _glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_HACK],
      "Center at world origin",
      NULL,
      id+CHECK_CENTER,
      NULL);
   assert(_checkbox[CHECK_CENTER]);

   // Editable xforms

   _panel[PANEL_EDIT_XFORM] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_HACK],
      "");
   assert(_panel[PANEL_EDIT_XFORM]);

   // clamp limits for scaling
   float min_scale = .00001;
   float max_scale = 1000;

   _edittext[EDIT_TRAN_X] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Trans X", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_TRAN_X, edit_xform_cb);
   assert(_edittext[EDIT_TRAN_X]);

   _edittext[EDIT_SCALE_X] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Scale X", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_SCALE_X, edit_xform_cb);
   assert(_edittext[EDIT_SCALE_X]);
   _edittext[EDIT_SCALE_X]->set_float_limits(min_scale, max_scale, GLUI_LIMIT_CLAMP);

   _edittext[EDIT_ROT_X] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Rot X", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_ROT_X, edit_xform_cb);
   assert(_edittext[EDIT_ROT_X]);
   _edittext[EDIT_ROT_X]->set_alignment(GLUI_ALIGN_RIGHT);

   _glui->add_column_to_panel(_panel[PANEL_EDIT_XFORM],false);

   _edittext[EDIT_TRAN_Y] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Y", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_TRAN_Y, edit_xform_cb);
   assert(_edittext[EDIT_TRAN_Y]);

   _edittext[EDIT_SCALE_Y] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Y", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_SCALE_Y, edit_xform_cb);
   assert(_edittext[EDIT_SCALE_Y]);
   _edittext[EDIT_SCALE_Y]->set_float_limits(min_scale, max_scale, GLUI_LIMIT_CLAMP);

   _edittext[EDIT_ROT_Y] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Y", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_ROT_Y, edit_xform_cb);
   assert(_edittext[EDIT_ROT_Y]);

   _glui->add_column_to_panel(_panel[PANEL_EDIT_XFORM],false);

   _edittext[EDIT_TRAN_Z] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Z", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_TRAN_Z, edit_xform_cb);
   assert(_edittext[EDIT_TRAN_Z]);

   _edittext[EDIT_SCALE_Z] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Z", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_SCALE_Z, edit_xform_cb);
   assert(_edittext[EDIT_SCALE_Z]);
   _edittext[EDIT_SCALE_Y]->set_float_limits(min_scale, max_scale, GLUI_LIMIT_CLAMP);

   _edittext[EDIT_ROT_Z] = _glui->add_edittext_to_panel(
      _panel[PANEL_EDIT_XFORM],
      "Z", GLUI_EDITTEXT_FLOAT,
      NULL, 
      id+EDIT_ROT_Z, edit_xform_cb);
   assert(_edittext[EDIT_ROT_Z]);



   // Cleanup sizes

   for (i=0; i<EDIT_NUM; i++)  
   {
      if (i != EDIT_NAME)
         _edittext[i]->set_w(80);
   }
  
   int w = _panel[PANEL_XFORM]->get_w();

   for (i=0; i<SLIDE_NUM; i++)
   {
      if (i == SLIDE_R)
         _slider[i]->set_w(_panel[PANEL_GTEX_LIGHT_OPTS]->get_w());
      else
         _slider[i]->set_w(w);
      
   }

   _listbox[LIST_TEX]->set_w(_listbox[LIST_TEX]->get_w() + 
         (_panel[PANEL_XFORM]->get_w() - _panel[PANEL_TEX]->get_w()));

   _button[BUT_REFRESH]->set_w(_listbox[LIST_TEX]->get_w());

   int button_size = (_rollout[ROLLOUT_XFORM]->get_w()-_panel[PANEL_PEN]->get_w())/2;
   _button[BUT_PREV_PEN]->set_w(_button[BUT_PREV_PEN]->get_w()+button_size);
   _button[BUT_NEXT_PEN]->set_w(_button[BUT_NEXT_PEN]->get_w()+button_size);

   // Otherwise, just fix up the image size
   if (!_init) init();

   _rollout[ROLLOUT_LIGHT]->close();
   _rollout[ROLLOUT_TOON]->close();
   _rollout[ROLLOUT_HACK]->close(); 

   //Show will actually show it...
   _glui->hide(); 
}

/////////////////////////////////////
// destroy
/////////////////////////////////////

void
NPRPenUI::destroy() 
{
   assert(_glui);

   //Hands off these soon to be bad things

   _listbox.clear();
   _button.clear();
   _slider.clear();
   _panel.clear(); 
   _rollout.clear();
   _rotation.clear();
   _translation.clear();
   _scale.clear();
   _radgroup.clear();
   _radbutton.clear();
   _checkbox.clear();
   _edittext.clear();

   _toon_filenames.clear();
   _other_filenames.clear();

   _glui->close();

   _glui = NULL;


}

/////////////////////////////////////
// try_layer_name()
/////////////////////////////////////
str_ptr
NPRPenUI::try_layer_name(
   int i,
   str_ptr name)
{
   NPRTexture *npr = _pen->curr_npr_tex();   

   if (!npr)return NULL_STR;

   if (i>=npr->get_basecoat_num()) return NULL_STR;

   GTexture *t = npr->get_basecoat(i);

   if (NPRSolidTexture::isa(t))
      return str_ptr(i) + ":Norm:" + name;
   else if (XToonTexture::isa(t))
      return str_ptr(i) + ":Toon:" + name;
   else if (GLSLXToonShader::isa(t))
      return str_ptr(i) + ":XToon:" + name;
   else
      return NULL_STR;
}

/////////////////////////////////////
// get_layer_name()
/////////////////////////////////////
str_ptr
NPRPenUI::get_layer_name(
   int i)
{
   NPRTexture *npr = _pen->curr_npr_tex();   

   if (!npr) return NULL_STR;

   if (i>=npr->get_basecoat_num()) return NULL_STR;

   GTexture *t = npr->get_basecoat(i);

   if (NPRSolidTexture::isa(t))
      return str_ptr(i) + ":Norm:" + ((NPRSolidTexture*)t)->get_layer_name();
   else if (XToonTexture::isa(t))
      return str_ptr(i) + ":Toon:" + ((XToonTexture*)t)->get_layer_name();
    else if (GLSLXToonShader::isa(t))
      return str_ptr(i) + ":XToon:" + ((GLSLXToonShader*)t)->get_layer_name();
   else
      return NULL_STR;
}

/////////////////////////////////////
// set_layer_name()
/////////////////////////////////////
bool
NPRPenUI::set_layer_name(
   int i,
   str_ptr name
   )
{
   str_ptr test;

   NPRTexture *npr = _pen->curr_npr_tex();   

   if ((!npr) || (i>=npr->get_basecoat_num())) return false;

   GTexture *t = npr->get_basecoat(i);

   if (NPRSolidTexture::isa(t))
   {
      test = str_ptr(i) + ":Norm:" + name;

      if (_listbox[LIST_LAYER]->check_item_fit(**test))
         ((NPRSolidTexture*)t)->set_layer_name(name);
   
      return true;
   }
   else if (XToonTexture::isa(t))
   {
      test = str_ptr(i) + ":Toon:" + name;

      if (_listbox[LIST_LAYER]->check_item_fit(**test))
         ((XToonTexture*)t)->set_layer_name(name);
      return true;
   }
   else if (GLSLXToonShader::isa(t))
   {
      test = str_ptr(i) + ":XToon:" + name;

      if (_listbox[LIST_LAYER]->check_item_fit(**test))
         ((GLSLXToonShader*)t)->set_layer_name(name);
      return true;
   }
   else
      return false;

}


/////////////////////////////////////
// fill_layer_listbox()
/////////////////////////////////////

void
NPRPenUI::fill_layer_listbox()
{
   int i = 0;
   NPRTexture *npr = _pen->curr_npr_tex();   

   while (_listbox[LIST_LAYER]->delete_item(i)) i++;

   _listbox[LIST_LAYER]->add_item(0, "----");

   if (npr)
   {
      for (i = 0; i<npr->get_basecoat_num(); i++) 
      {
         _listbox[LIST_LAYER]->add_item(1+i, **get_layer_name(i));
      }
   }
}

/////////////////////////////////////
// fill_tex_listbox()
/////////////////////////////////////

void
NPRPenUI::fill_tex_listbox(
   GLUI_Listbox *listbox,
   Cstr_list     &files
   )
{
   int m = 1 + max(_toon_filenames.num(), _other_filenames.num());

   
   int j;
   for (j=0; j<=m ; j++)
      _listbox[LIST_TEX]->delete_item(j);

   listbox->add_item(0, "----");
   for (int i = 0; i < files.num(); i++) 
   {
      int len = files[i].len();
      if ( (len>3) && 
            (strcmp(&(**files[i])[len-4],".png") == 0))
      {

         listbox->add_item(1+i, **files[i]);
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
NPRPenUI::update_non_lives()
{
   update_gtex();
   update_xform();
   
}

/////////////////////////////////////
// update_gtex()
/////////////////////////////////////

void
NPRPenUI::update_gtex()
{
   int i;
   HSVCOLOR hsv;
   NPRTexture *npr = _pen->curr_npr_tex();

   i = _listbox[LIST_LAYER]->get_int_val();
   fill_layer_listbox();
   _listbox[LIST_LAYER]->set_int_val(i);

   if (!npr)
   {
      _checkbox[CHECK_TRANS]->disable();
      _checkbox[CHECK_ANNOTATE]->disable();

      _listbox[LIST_LAYER]->disable();
      _edittext[EDIT_NAME]->disable();
      _radgroup[RADGROUP_GTEX]->disable();
      _button[BUT_ADD]->disable();
      _button[BUT_DEL]->disable();

      _checkbox[CHECK_PAPER]->disable();
      _checkbox[CHECK_TRAVEL]->disable();
      _checkbox[CHECK_LIGHT]->disable();
      _checkbox[CHECK_SPEC]->disable();

      _radgroup[RADGROUP_LIGHT]->disable();
      _checkbox[CHECK_DIR]->disable();
      _checkbox[CHECK_CAM]->disable();
      _slider[SLIDE_R]->disable();
      _rotation[ROT_L]->disable();
      
      
      _listbox[LIST_DETAIL]->disable();
      _slider[SLIDE_T]->disable();
      _slider[SLIDE_F]->disable();
      _slider[SLIDE_L]->disable();
	  _listbox[LIST_DETAIL2]->disable();
      _button[BUT_SMOOTH]->disable();
      _button[BUT_ELLIPTIC]->disable();
      _button[BUT_SPHERIC]->disable();
      _button[BUT_CYLINDRIC]->disable();
//       _button[BUT_CURV]->disable();

      _slider[SLIDE_H]->disable();
      _slider[SLIDE_S]->disable();
      _slider[SLIDE_V]->disable();
      _slider[SLIDE_A]->disable();
      _checkbox[CHECK_INV]->disable();
      _listbox[LIST_TEX]->disable();
      _button[BUT_REFRESH]->disable();
      
      fill_tex_listbox(_listbox[LIST_TEX],str_list());
   }
   else
   {

      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);

      _checkbox[CHECK_TRANS]->enable();
      _checkbox[CHECK_ANNOTATE]->enable();

      _listbox[LIST_LAYER]->enable();
      _radgroup[RADGROUP_GTEX]->enable();
      _button[BUT_ADD]->enable();

      _checkbox[CHECK_TRANS]->set_int_val(npr->get_transparent());
      _checkbox[CHECK_ANNOTATE]->set_int_val(npr->get_annotate());

      if (!t)
      {
         assert(_listbox[LIST_LAYER]->get_int_val() == 0);
         
         _rollout[ROLLOUT_LIGHT]->close();

         _edittext[EDIT_NAME]->disable();
         _button[BUT_DEL]->disable();

         _checkbox[CHECK_PAPER]->disable();
         _checkbox[CHECK_TRAVEL]->disable();
         _checkbox[CHECK_LIGHT]->disable();
         _checkbox[CHECK_SPEC]->disable();

         _radgroup[RADGROUP_LIGHT]->disable();
         _checkbox[CHECK_DIR]->disable();
         _checkbox[CHECK_CAM]->disable();
         _slider[SLIDE_R]->disable();
         _rotation[ROT_L]->disable();

         _rollout[ROLLOUT_TOON]->close();
	 _listbox[LIST_DETAIL]->disable();
	 _slider[SLIDE_T]->disable();
	 _slider[SLIDE_F]->disable();
	 _slider[SLIDE_L]->disable();
	 _listbox[LIST_DETAIL2]->disable();
	 _button[BUT_SMOOTH]->disable();
	 _button[BUT_ELLIPTIC]->disable();
	 _button[BUT_SPHERIC]->disable();
	 _button[BUT_CYLINDRIC]->disable();
// 	 _button[BUT_CURV]->disable();

	 _slider[SLIDE_H]->disable();
         _slider[SLIDE_S]->disable();
         _slider[SLIDE_V]->disable();
         _slider[SLIDE_A]->disable();
         _checkbox[CHECK_INV]->disable();
         _listbox[LIST_TEX]->disable();
         _button[BUT_REFRESH]->disable();

      }
      else if (NPRSolidTexture::isa(t))
      {
         NPRSolidTexture *solid = (NPRSolidTexture*)t;
         
         _rollout[ROLLOUT_LIGHT]->close();

         _edittext[EDIT_NAME]->enable();
         _button[BUT_DEL]->enable();

         _checkbox[CHECK_PAPER]->enable();
         if (solid->get_use_paper())
            _checkbox[CHECK_TRAVEL]->enable();
         else
            _checkbox[CHECK_TRAVEL]->disable();

         _checkbox[CHECK_LIGHT]->enable();
         if (solid->get_use_lighting())
            _checkbox[CHECK_SPEC]->enable();
         else
            _checkbox[CHECK_SPEC]->disable();

         _radgroup[RADGROUP_LIGHT]->disable();
         _checkbox[CHECK_DIR]->disable();
         _checkbox[CHECK_CAM]->disable();
         _slider[SLIDE_R]->disable();
         _rotation[ROT_L]->disable();

         _rollout[ROLLOUT_TOON]->close();
	 _listbox[LIST_DETAIL]->disable();
	 _slider[SLIDE_T]->disable();
	 _slider[SLIDE_F]->disable();
	 _slider[SLIDE_L]->disable();
	 _listbox[LIST_DETAIL2]->disable();
	 _button[BUT_SMOOTH]->disable();
	 _button[BUT_ELLIPTIC]->disable();
	 _button[BUT_SPHERIC]->disable();
	 _button[BUT_CYLINDRIC]->disable();
// 	 _button[BUT_CURV]->disable();

	 _slider[SLIDE_H]->enable();
         _slider[SLIDE_S]->enable();
         _slider[SLIDE_V]->enable();
         _slider[SLIDE_A]->enable();
         _checkbox[CHECK_INV]->enable();
         _listbox[LIST_TEX]->enable();
         _button[BUT_REFRESH]->enable();

         _radgroup[RADGROUP_GTEX]->set_int_val(RADBUT_SOLID);

         _checkbox[CHECK_PAPER]->set_int_val(solid->get_use_paper());
         _checkbox[CHECK_TRAVEL]->set_int_val(solid->get_travel_paper());
         _checkbox[CHECK_LIGHT]->set_int_val(solid->get_use_lighting());
         _checkbox[CHECK_SPEC]->set_int_val(solid->get_light_specular());

         hsv = solid->get_color();
   
         _slider[SLIDE_H]->set_float_val(hsv[0]);
         _slider[SLIDE_S]->set_float_val(hsv[1]);
         _slider[SLIDE_V]->set_float_val(hsv[2]);
         _slider[SLIDE_A]->set_float_val(solid->get_alpha());

         fill_tex_listbox(_listbox[LIST_TEX],_other_filenames);

         if (solid->get_tex_name().contains(OTHER_DIRECTORY))
         {
            str_ptr name = solid->get_tex_name();
            str_ptr str;
            for (i=str_ptr(OTHER_DIRECTORY).len(); i<(int)name.len(); i++)
               str = str + str_ptr(name[i]);

            i = _other_filenames.get_index(str);
            if (i==BAD_IND)
               _listbox[LIST_TEX]->set_int_val(0);
            else
               _listbox[LIST_TEX]->set_int_val(i+1);
         }
         else
            _listbox[LIST_TEX]->set_int_val(0);

      }
      else if (XToonTexture::isa(t))
      {
         XToonTexture *toon = (XToonTexture*)t;

         _rollout[ROLLOUT_LIGHT]->open();

         _edittext[EDIT_NAME]->enable();
         _button[BUT_DEL]->enable();

         _checkbox[CHECK_PAPER]->enable();
         if (toon->get_use_paper())
            _checkbox[CHECK_TRAVEL]->enable();
         else
            _checkbox[CHECK_TRAVEL]->disable();

         _checkbox[CHECK_LIGHT]->disable();
         _checkbox[CHECK_SPEC]->disable();

         _radgroup[RADGROUP_LIGHT]->enable();
         if (toon->get_light_index() == -1)
         {
            _checkbox[CHECK_DIR]->enable();
            _checkbox[CHECK_CAM]->enable();
            _slider[SLIDE_R]->enable();
            _rotation[ROT_L]->enable();
         }
         else
         {
            _checkbox[CHECK_DIR]->disable();
            _checkbox[CHECK_CAM]->disable();
            _slider[SLIDE_R]->disable();
            _rotation[ROT_L]->disable();
         }

         _rollout[ROLLOUT_TOON]->open();

	 int detail_id = _listbox[LIST_DETAIL]->get_int_val();
	 _listbox[LIST_DETAIL]->enable();
	 _slider[SLIDE_T]->enable();
	 _slider[SLIDE_T]->set_float_val(_old_target_value[detail_id]);
	 _slider[SLIDE_F]->enable();
	 _slider[SLIDE_F]->set_float_val(_old_factor_value[detail_id]);

	 _listbox[LIST_DETAIL2]->disable();
	 if (toon->normals_smoothed() || toon->normals_elliptic() || 
	     toon->normals_spheric() || toon->normals_cylindric()) {
	   _slider[SLIDE_L]->enable();
	 } else {
	   _slider[SLIDE_L]->disable();
	 }

	 _button[BUT_SMOOTH]->enable();
	 _button[BUT_ELLIPTIC]->enable();
	 _button[BUT_SPHERIC]->enable();
	 _button[BUT_CYLINDRIC]->enable();
// 	 _button[BUT_CURV]->enable();

         _slider[SLIDE_H]->enable();
         _slider[SLIDE_S]->enable();
         _slider[SLIDE_V]->enable();
         _slider[SLIDE_A]->enable();
         _checkbox[CHECK_INV]->enable();
         _listbox[LIST_TEX]->enable();
         _button[BUT_REFRESH]->enable();

         _radgroup[RADGROUP_GTEX]->set_int_val(RADBUT_TOON);

         _checkbox[CHECK_PAPER]->set_int_val(toon->get_use_paper());
         _checkbox[CHECK_TRAVEL]->set_int_val(toon->get_travel_paper());
         _checkbox[CHECK_LIGHT]->set_int_val(1);
         _checkbox[CHECK_SPEC]->set_int_val(0);

         _radgroup[RADGROUP_LIGHT]->set_int_val(
            (toon->get_light_index()==-1)?
                       (RADBUT_LIGHTC-RADBUT_LIGHT0):(toon->get_light_index()));
         _checkbox[CHECK_DIR]->set_int_val(toon->get_light_dir());
         _checkbox[CHECK_CAM]->set_int_val(toon->get_light_cam());
         _light = toon->get_light_coords();
         float rad = _light.length();
         _light = _light.normalized();
         _slider[SLIDE_R]->set_float_val(rad);
         _rotation[ROT_L]->reset();

         hsv = toon->get_color();
   
         _slider[SLIDE_H]->set_float_val(hsv[0]);
         _slider[SLIDE_S]->set_float_val(hsv[1]);
         _slider[SLIDE_V]->set_float_val(hsv[2]);
         _slider[SLIDE_A]->set_float_val(toon->get_alpha());

         fill_tex_listbox(_listbox[LIST_TEX],_toon_filenames);

         if (toon->get_tex_name().contains(TOON_DIRECTORY))
         {
            str_ptr name = toon->get_tex_name();
            str_ptr str;
            for (i=str_ptr(TOON_DIRECTORY).len(); i<(int)name.len(); i++)
               str = str + str_ptr(name[i]);

            i = _toon_filenames.get_index(str);
            if (i==BAD_IND)
               _listbox[LIST_TEX]->set_int_val(0);
            else
               _listbox[LIST_TEX]->set_int_val(i+1);

         }
         else
            _listbox[LIST_TEX]->set_int_val(0);

	  }
	  //GLSL Version of XToon
      else if (GLSLXToonShader::isa(t))
      {
         GLSLXToonShader *xtoon = (GLSLXToonShader*)t;

         _rollout[ROLLOUT_LIGHT]->open();

         _edittext[EDIT_NAME]->enable();
         _button[BUT_DEL]->enable();

		 //robcm - disabling Paper...for now
         _checkbox[CHECK_PAPER]->disable();
        // if (xtoon->get_use_paper())
        //    _checkbox[CHECK_TRAVEL]->enable();
        // else
            _checkbox[CHECK_TRAVEL]->disable();

         _checkbox[CHECK_LIGHT]->disable();
         _checkbox[CHECK_SPEC]->disable();

         _radgroup[RADGROUP_LIGHT]->enable();
		 //if we're not using a GL light, then enable light customizing commands
         if (xtoon->get_light_index() == -1)
         {
            _checkbox[CHECK_DIR]->enable();
            _checkbox[CHECK_CAM]->enable();
            _slider[SLIDE_R]->enable();
            _rotation[ROT_L]->enable();
         }
         else //otherwise disable those things
         {
            _checkbox[CHECK_DIR]->disable();
            _checkbox[CHECK_CAM]->disable();
            _slider[SLIDE_R]->disable();
            _rotation[ROT_L]->disable();
         }

         _rollout[ROLLOUT_TOON]->open();

	 //list box for the different detail maps
	 int detail_id = _listbox[LIST_DETAIL]->get_int_val();
	 _listbox[LIST_DETAIL]->enable();
	 //sliders for the target size and max factor
	 _slider[SLIDE_T]->enable();
	 _slider[SLIDE_T]->set_float_val(_old_target_value[detail_id]);
	 _slider[SLIDE_F]->enable();
	 _slider[SLIDE_F]->set_float_val(_old_factor_value[detail_id]);

	 int detail2_id = _listbox[LIST_DETAIL2]->get_int_val();
	 _listbox[LIST_DETAIL2]->enable();
	 xtoon->set_smooth_detail(detail2_id);
	 if (xtoon->normals_smoothed() || xtoon->normals_elliptic() || 
	     xtoon->normals_spheric() || xtoon->normals_cylindric()) {
	   _slider[SLIDE_L]->enable();
	 } else {
	   _slider[SLIDE_L]->disable();
	 }

	 _button[BUT_SMOOTH]->enable();
	 _button[BUT_ELLIPTIC]->enable();
	 _button[BUT_SPHERIC]->enable();
	 _button[BUT_CYLINDRIC]->enable();
// 	 _button[BUT_CURV]->enable();

         _slider[SLIDE_H]->enable();
         _slider[SLIDE_S]->enable();
         _slider[SLIDE_V]->enable();
         _slider[SLIDE_A]->enable();
         _checkbox[CHECK_INV]->enable();
         _listbox[LIST_TEX]->enable();
         _button[BUT_REFRESH]->enable();

         _radgroup[RADGROUP_GTEX]->set_int_val(RADBUT_XTOON);

		 //robcm - disabling paper...
         _checkbox[CHECK_PAPER]->set_int_val( xtoon->get_use_paper());
         _checkbox[CHECK_TRAVEL]->set_int_val(xtoon->get_travel_paper());
         _checkbox[CHECK_LIGHT]->set_int_val(1);
         _checkbox[CHECK_SPEC]->set_int_val(0);

         _radgroup[RADGROUP_LIGHT]->set_int_val(
            (xtoon->get_light_index()==-1)?
                       (RADBUT_LIGHTC-RADBUT_LIGHT0):(xtoon->get_light_index()));
         _checkbox[CHECK_DIR]->set_int_val(xtoon->get_light_dir());
         _checkbox[CHECK_CAM]->set_int_val(xtoon->get_light_cam());
         _light = xtoon->get_light_coords();
         float rad = _light.length();
         _light = _light.normalized();
         _slider[SLIDE_R]->set_float_val(rad);
         _rotation[ROT_L]->reset();

         hsv = xtoon->get_color();
   
         _slider[SLIDE_H]->set_float_val(hsv[0]);
         _slider[SLIDE_S]->set_float_val(hsv[1]);
         _slider[SLIDE_V]->set_float_val(hsv[2]);
         _slider[SLIDE_A]->set_float_val(xtoon->get_alpha());

		 
         fill_tex_listbox(_listbox[LIST_TEX],_toon_filenames);

         if (xtoon->get_tex_name().contains(TOON_DIRECTORY))
         {
            str_ptr name = xtoon->get_tex_name();
            str_ptr str;
            for (i=str_ptr(TOON_DIRECTORY).len(); i<(int)name.len(); i++)
               str = str + str_ptr(name[i]);
            i = _toon_filenames.get_index(str);
            if (i==BAD_IND)
			   {
               _listbox[LIST_TEX]->set_int_val(0);
			   }
            else
               _listbox[LIST_TEX]->set_int_val(i+1);
         }
         else
		 {
            _listbox[LIST_TEX]->set_int_val(0);
		 }
	  }

      else
      {
         assert(0);
      }
   }
}

/////////////////////////////////////
// update_xform()
/////////////////////////////////////

void
NPRPenUI::update_xform()
{
   _edittext[EDIT_TRAN_X]->set_float_val(0);
   _edittext[EDIT_TRAN_Y]->set_float_val(0);
   _edittext[EDIT_TRAN_Z]->set_float_val(0);

   _edittext[EDIT_SCALE_X]->set_float_val(1);
   _edittext[EDIT_SCALE_Y]->set_float_val(1);
   _edittext[EDIT_SCALE_Z]->set_float_val(1);

   _edittext[EDIT_ROT_X]->set_float_val(0);
   _edittext[EDIT_ROT_Y]->set_float_val(0);
   _edittext[EDIT_ROT_Z]->set_float_val(0);

   if (_pen->curr_npr_tex())
   {
      _rotation[ROT_ROT]->enable();
      _translation[TRAN_X]->enable();
      _translation[TRAN_Y]->enable();
      _translation[TRAN_Z]->enable();
      _scale[SCALE_UNIFORM]->enable();

      _edittext[EDIT_TRAN_X]->enable();
      _edittext[EDIT_TRAN_Y]->enable();
      _edittext[EDIT_TRAN_Z]->enable();

      _edittext[EDIT_SCALE_X]->enable();
      _edittext[EDIT_SCALE_Y]->enable();
      _edittext[EDIT_SCALE_Z]->enable();

      _edittext[EDIT_ROT_X]->enable();
      _edittext[EDIT_ROT_Y]->enable();
      _edittext[EDIT_ROT_Z]->enable();

      _checkbox[CHECK_CENTER]->enable();

      _xform = _pen->curr_npr_tex()->patch()->xform();
      _center = _xform * _pen->curr_npr_tex()->patch()->mesh()->get_bb().center();

      _rotation[ROT_ROT]->reset();
      _translation[TRAN_X]->set_x(0);
      _translation[TRAN_Y]->set_y(0);
      _translation[TRAN_Z]->set_z(0);
      _scale[SCALE_UNIFORM]->set_y(1.0);

   }
   else
   {
      _rotation[ROT_ROT]->disable();
      _translation[TRAN_X]->disable();
      _translation[TRAN_Y]->disable();
      _translation[TRAN_Z]->disable();
      _scale[SCALE_UNIFORM]->disable();

      _edittext[EDIT_TRAN_X]->disable();
      _edittext[EDIT_TRAN_Y]->disable();
      _edittext[EDIT_TRAN_Z]->disable();

      _edittext[EDIT_SCALE_X]->disable();
      _edittext[EDIT_SCALE_Y]->disable();
      _edittext[EDIT_SCALE_Z]->disable();

      _edittext[EDIT_ROT_X]->disable();
      _edittext[EDIT_ROT_Y]->disable();
      _edittext[EDIT_ROT_Z]->disable();

      _checkbox[CHECK_CENTER]->disable();

   }
}

/////////////////////////////////////
// apply_xform()
/////////////////////////////////////

void
NPRPenUI::apply_xform()
{
   if (_pen->curr_npr_tex())
   {

      float mat[4][4];
      _rotation[ROT_ROT]->get_float_array_val((float *)mat);

      float scale_x = _edittext[EDIT_SCALE_X]->get_float_val();
      float scale_y = _edittext[EDIT_SCALE_Y]->get_float_val();
      float scale_z = _edittext[EDIT_SCALE_Z]->get_float_val();

      Wtransf s = Wtransf::scaling(scale_x, scale_y, scale_z);

      Wvec c0(mat[0][0],mat[0][1],mat[0][2]);
      Wvec c1(mat[1][0],mat[1][1],mat[1][2]);
      Wvec c2(mat[2][0],mat[2][1],mat[2][2]);
   
      Wtransf r(c0,c1,c2);

      Wvec delt = Wvec(_translation[TRAN_X]->get_x(),
                       _translation[TRAN_Y]->get_y(),
                       _translation[TRAN_Z]->get_z());
      Wtransf t = Wtransf::translation(delt);


      int center_at_world_origin = _checkbox[CHECK_CENTER]->get_int_val();
  
      if (center_at_world_origin) {
         _pen->curr_npr_tex()->patch()->mesh()->geom()->set_xform(
            t * r * s * _xform );
      }
      else { // use bounding box center
         _pen->curr_npr_tex()->patch()->mesh()->geom()->set_xform(
            t * Wtransf(_center) * r * s * Wtransf(-_center) * _xform );
      }


      XFORMobs::notify_xform_obs(_pen->curr_npr_tex()->mesh()->geom(), XFORMobs::MIDDLE);

   }   
}


/////////////////////////////////////
// select()
/////////////////////////////////////
void
NPRPenUI::select()
{
   cerr << "NPRPenUI::select()\n";

   if (_pen->curr_npr_tex()) {
      XFORMobs::notify_xform_obs(_pen->curr_npr_tex()->mesh()->geom(), XFORMobs::START);

      if (_pen->curr_npr_tex()->get_basecoat_num()>0)
         _listbox[LIST_LAYER]->set_int_val(1);
      else
         _listbox[LIST_LAYER]->set_int_val(0);
   }
}

/////////////////////////////////////
// deselect()
/////////////////////////////////////
void
NPRPenUI::deselect()
{
   cerr << "NPRPenUI::deselect()\n";

   if (_pen->curr_npr_tex()) {
      bool apply = false;
      TEXBODY* t = TEXBODY::upcast(_pen->curr_npr_tex()->mesh()->geom());
      if (t) {
         apply = t->apply_xf();
         static bool APPLY_NPR_TRANSFORMS = Config::get_var_bool("APPLY_NPR_TRANSFORMS",false);
         t->set_apply_xf(APPLY_NPR_TRANSFORMS);
      }
      XFORMobs::notify_xform_obs(_pen->curr_npr_tex()->patch()->mesh()->geom(), XFORMobs::END);
      if (t)
         t->set_apply_xf(apply);
   }

   _listbox[LIST_LAYER]->set_int_val(0);
}

/////////////////////////////////////
// apply_gtex()
/////////////////////////////////////
void
NPRPenUI::apply_gtex()
{
   int i;
   HSVCOLOR hsv;
   NPRTexture *npr = _pen->curr_npr_tex();

   if (!npr)
   {
   }
   else
   {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);

      npr->set_transparent(_checkbox[CHECK_TRANS]->get_int_val());
      npr->set_annotate(_checkbox[CHECK_ANNOTATE]->get_int_val());

      if (!t)
      {
         assert(_listbox[LIST_LAYER]->get_int_val() == 0);
      }
      else if (NPRSolidTexture::isa(t))
      {
         NPRSolidTexture *solid = (NPRSolidTexture*)t;

         solid->set_use_paper(_checkbox[CHECK_PAPER]->get_int_val());
         solid->set_travel_paper(_checkbox[CHECK_TRAVEL]->get_int_val());
         solid->set_use_lighting(_checkbox[CHECK_LIGHT]->get_int_val());
         solid->set_light_specular(_checkbox[CHECK_SPEC]->get_int_val());

         hsv[0] = _slider[SLIDE_H]->get_float_val();
         hsv[1] = _slider[SLIDE_S]->get_float_val();
         hsv[2] = _slider[SLIDE_V]->get_float_val();
        
         solid->set_color(hsv);

         solid->set_alpha(_slider[SLIDE_A]->get_float_val());

         i = _listbox[LIST_TEX]->get_int_val();
         if (i == 0)
            solid->set_tex_name(NULL_STR);
         else
         {
            if (solid->get_tex_name() != (str_ptr(OTHER_DIRECTORY)+_other_filenames[i-1]))
               solid->set_tex_name(str_ptr(OTHER_DIRECTORY) + _other_filenames[i-1]);
         }

	  }
      else if (XToonTexture::isa(t))
      {
         XToonTexture *toon = (XToonTexture*)t;

         toon->set_use_paper(_checkbox[CHECK_PAPER]->get_int_val());
         toon->set_travel_paper(_checkbox[CHECK_TRAVEL]->get_int_val());

         toon->set_light_index(
            (_radgroup[RADGROUP_LIGHT]->get_int_val() == (RADBUT_LIGHTC-RADBUT_LIGHT0)) ?
                  (-1): (_radgroup[RADGROUP_LIGHT]->get_int_val()));         

         toon->set_light_dir(_checkbox[CHECK_DIR]->get_int_val());
         toon->set_light_cam(_checkbox[CHECK_CAM]->get_int_val());
         
         float mat[4][4];
         _rotation[ROT_L]->get_float_array_val((float *)mat);
         Wvec c0(mat[0][0],mat[0][1],mat[0][2]);
         Wvec c1(mat[1][0],mat[1][1],mat[1][2]);
         Wvec c2(mat[2][0],mat[2][1],mat[2][2]);
         Wtransf t(c0,c1,c2);
         Wvec new_dir = (t*_light).normalized();
         toon->set_light_coords(_slider[SLIDE_R]->get_float_val() * new_dir);

	 
// 	 if (_checkbox[CHECK_DEPTH]->get_int_val()){
// 	   toon->set_detail_map(XToonTexture::Depth);
// 	 } else {	   
// 	   toon->set_detail_map(XToonTexture::GaussianCurvature);
// 	 }
	 int detail_id = _listbox[LIST_DETAIL]->get_int_val();
	 switch(detail_id){
	 case 0: toon->set_detail_map(XToonTexture::User); break;
	 case 1: toon->set_detail_map(XToonTexture::Depth); break;
	 case 2: toon->set_detail_map(XToonTexture::Focus); break;
	 case 3: toon->set_detail_map(XToonTexture::Flow); break;
	 case 4: toon->set_detail_map(XToonTexture::Orientation); break;
	 case 5: toon->set_detail_map(XToonTexture::Specularity); break;
	 }
	 _old_target_value[detail_id] = _slider[SLIDE_T]->get_float_val();
	 toon->set_target_length(_old_target_value[detail_id]);
	 _old_factor_value[detail_id] = _slider[SLIDE_F]->get_float_val();
	 toon->set_max_factor(_old_factor_value[detail_id]);

	 toon->set_smooth_factor(_slider[SLIDE_L]->get_float_val());

         hsv[0] = _slider[SLIDE_H]->get_float_val();
         hsv[1] = _slider[SLIDE_S]->get_float_val();
         hsv[2] = _slider[SLIDE_V]->get_float_val();
         
         toon->set_color(hsv);

         toon->set_alpha(_slider[SLIDE_A]->get_float_val());

	 //toon->set_inv_detail(_checkbox[CHECK_INV]->get_int_val());

         i = _listbox[LIST_TEX]->get_int_val();
         if (i == 0)
            toon->set_tex_name(NULL_STR);
         else
         {
            if (toon->get_tex_name() != ( str_ptr(TOON_DIRECTORY) + _toon_filenames[i-1]))
               toon->set_tex_name(str_ptr(TOON_DIRECTORY) + _toon_filenames[i-1]);
         }

	  }
	  //GLSL version of xtoon shader
	  else if(GLSLXToonShader::isa(t)) //robcm
	  {
      GLSLXToonShader *xtoon = (GLSLXToonShader*)t;

	     //robcm - disabling paper for now
	     xtoon->set_use_paper(false);//_checkbox[CHECK_PAPER]->get_int_val());
         xtoon->set_travel_paper(false);//_checkbox[CHECK_TRAVEL]->get_int_val());

         xtoon->set_light_index(
            (_radgroup[RADGROUP_LIGHT]->get_int_val() == (RADBUT_LIGHTC-RADBUT_LIGHT0)) ?
                  (-1): (_radgroup[RADGROUP_LIGHT]->get_int_val()));         

         xtoon->set_light_dir(_checkbox[CHECK_DIR]->get_int_val());
         xtoon->set_light_cam(_checkbox[CHECK_CAM]->get_int_val());
         
         float mat[4][4];
         _rotation[ROT_L]->get_float_array_val((float *)mat);
         Wvec c0(mat[0][0],mat[0][1],mat[0][2]);
         Wvec c1(mat[1][0],mat[1][1],mat[1][2]);
         Wvec c2(mat[2][0],mat[2][1],mat[2][2]);
         Wtransf t(c0,c1,c2);
         Wvec new_dir = (t*_light).normalized();
         xtoon->set_light_coords(_slider[SLIDE_R]->get_float_val() * new_dir);

	 //not used?
 	//if (_checkbox[CHECK_DEPTH]->get_int_val()){
 	//   xtoon->set_detail_map(1); //check depth
 	// } else {	   
 	//   xtoon->set_detail_map(2);  //gausian curvature
 	// }
	 int detail_id = _listbox[LIST_DETAIL]->get_int_val();
	 switch(detail_id){
	 case 0: xtoon->set_detail_map(0); break;  //User Detail
	 case 1: xtoon->set_detail_map(1); break;  //Depth Detail
	 case 2: xtoon->set_detail_map(2); break;  //Focus Detail
	 case 3: xtoon->set_detail_map(3); break;  //Flow Detail
	 case 4: xtoon->set_detail_map(4); break;  //Orientation detail
	 case 5: xtoon->set_detail_map(5); break;  //Specularity detail
	 }

	 int detail2_id = _listbox[LIST_DETAIL2]->get_int_val();
	 xtoon->set_smooth_detail(detail2_id);

	 //set target and max factor
	 _old_target_value[detail_id] = _slider[SLIDE_T]->get_float_val();
	 xtoon->set_target_length(_old_target_value[detail_id]);
	 _old_factor_value[detail_id] = _slider[SLIDE_F]->get_float_val();
	 xtoon->set_max_factor(_old_factor_value[detail_id]);

	 xtoon->set_smooth_factor(_slider[SLIDE_L]->get_float_val());

         hsv[0] = _slider[SLIDE_H]->get_float_val();
         hsv[1] = _slider[SLIDE_S]->get_float_val();
         hsv[2] = _slider[SLIDE_V]->get_float_val();
         
         xtoon->set_color(hsv);

         xtoon->set_alpha(_slider[SLIDE_A]->get_float_val());

	 //robcm
	 xtoon->set_inv_detail(bool(_checkbox[CHECK_INV]->get_int_val()));

         i = _listbox[LIST_TEX]->get_int_val();
         if (i == 0)
            xtoon->set_tex_name(NULL_STR);
         else
         {
            if (xtoon->get_tex_name() != ( str_ptr(TOON_DIRECTORY) + _toon_filenames[i-1]))
               xtoon->set_tex_name(str_ptr(TOON_DIRECTORY) + _toon_filenames[i-1]);
         }
	  }

   }   
}


/////////////////////////////////////
// refresh_tex()
/////////////////////////////////////

void
NPRPenUI::refresh_tex()
{
   int i;
   NPRTexture *npr = _pen->curr_npr_tex();

   if (!npr)
   {
   }
   else
   {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);

      if (!t)
      {
         assert(_listbox[LIST_LAYER]->get_int_val() == 0);
      }
      else if (NPRSolidTexture::isa(t))
      {
         NPRSolidTexture *solid = (NPRSolidTexture*)t;

         // Refill the listbox
         fill_tex_listbox(_listbox[LIST_TEX],str_list());
         _other_filenames = dir_list(Config::JOT_ROOT()+OTHER_DIRECTORY);
         fill_tex_listbox(_listbox[LIST_TEX],_other_filenames);

         if (solid->get_tex_name().contains(OTHER_DIRECTORY))
         {
            str_ptr name = solid->get_tex_name();
            str_ptr str;
            for (i=str_ptr(OTHER_DIRECTORY).len(); i<(int)name.len(); i++)
               str = str + str_ptr(name[i]);

            i = _other_filenames.get_index(str);
            if (i==BAD_IND)
            {
               _listbox[LIST_TEX]->set_int_val(0);
               solid->set_tex_name(NULL_STR);
            }
            else
            {
               _listbox[LIST_TEX]->set_int_val(i+1);
               solid->set_tex_name(name);
            }

         }
         else
         {
            _listbox[LIST_TEX]->set_int_val(0);
            solid->set_tex_name(NULL_STR);
         }

      }
      else if (XToonTexture::isa(t))
      {
         XToonTexture *toon = (XToonTexture*)t;

         // Refill the listbox
         fill_tex_listbox(_listbox[LIST_TEX],str_list());
         _toon_filenames = dir_list(Config::JOT_ROOT()+TOON_DIRECTORY);
         fill_tex_listbox(_listbox[LIST_TEX],_toon_filenames);

         if (toon->get_tex_name().contains(TOON_DIRECTORY))
         {
            str_ptr name = toon->get_tex_name();
            str_ptr str;
            for (i=str_ptr(TOON_DIRECTORY).len(); i<(int)name.len(); i++)
               str = str + str_ptr(name[i]);

            i = _toon_filenames.get_index(str);
            if (i==BAD_IND)
            {
               _listbox[LIST_TEX]->set_int_val(0);
               toon->set_tex_name(NULL_STR);
            }
            else
            {
               _listbox[LIST_TEX]->set_int_val(i+1);
               toon->set_tex_name(name);
            }

         }
         else
         {
            _listbox[LIST_TEX]->set_int_val(0);
            toon->set_tex_name(NULL_STR);
         }

	  }
	  //GLSL Version of XToonShader
      else if (GLSLXToonShader::isa(t))
      {
         GLSLXToonShader *xtoon = (GLSLXToonShader*)t;

         // Refill the listbox
         fill_tex_listbox(_listbox[LIST_TEX],str_list());
         _toon_filenames = dir_list(Config::JOT_ROOT()+TOON_DIRECTORY);
         fill_tex_listbox(_listbox[LIST_TEX],_toon_filenames);

         if (xtoon->get_tex_name().contains(TOON_DIRECTORY))
         {
            str_ptr name = xtoon->get_tex_name();
            str_ptr str;
            for (i=str_ptr(TOON_DIRECTORY).len(); i<(int)name.len(); i++)
               str = str + str_ptr(name[i]);

            i = _toon_filenames.get_index(str);
            if (i==BAD_IND)
            {
               _listbox[LIST_TEX]->set_int_val(0);
               xtoon->set_tex_name(NULL_STR);
            }
            else
            {
               _listbox[LIST_TEX]->set_int_val(i+1);
               xtoon->set_tex_name(name);
            }

         }
         else
         {
            _listbox[LIST_TEX]->set_int_val(0);
            xtoon->set_tex_name(NULL_STR);
         }

	  }
   }
}

/////////////////////////////////////
// rename_layer()
/////////////////////////////////////
void
NPRPenUI::rename_layer()
{
   bool fix,good;
   int i,j,k;
   char *goodchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_!*=+[]{}|,.";
   char *origtext,*newtext;
   int origlen;


   if (_pen->curr_npr_tex())
   {
      k = _listbox[LIST_LAYER]->get_int_val();

      origtext = _edittext[EDIT_NAME]->get_text();
      origlen = strlen(origtext);

      //Here we replace bad chars with _'s,
      //and truncate to fit in the listbox
      //If no adjustments occur, we go ahead...

      if (origlen == 0)
      {
         WORLD::message("Zero length name. Continue editing...");
         return;
      }

      newtext = new char[origlen+1]; 
      assert(newtext);
      strcpy(newtext,origtext);

      fix = false;
      for (i=0; i<origlen; i++)
      {
         good = false;
         for (j=0; j<(int)strlen(goodchars); j++)
         {
            if (newtext[i]==goodchars[j]) good = true;
         }
         if (!good)
         {
            newtext[i] = '_';
            fix = true;
         }
      }

      if (fix) 
      {
         WORLD::message("Replaced bad characters with '_'. Continue editing...");
         _edittext[EDIT_NAME]->set_text(newtext);
         delete(newtext);
         return;
      }

      fix = false;
      while (!_listbox[LIST_LAYER]->check_item_fit(
                           **try_layer_name(k-1,newtext)) && strlen(newtext)>0)
      {
         fix = true;
         newtext[strlen(newtext)-1]=0;
      }

      if (fix) 
      {
         WORLD::message("Truncated name to fit listbox. Continue editing...");
         _edittext[EDIT_NAME]->set_text(newtext);
         delete(newtext);
         return;
      }

      if (!set_layer_name(k-1,newtext))
      {
         WORLD::message("Failed to set name!!!!!!!");
      }
      else
      {
         _edittext[EDIT_NAME]->set_text("");
         update_gtex();
      }
   }
}

/////////////////////////////////////
// add_layer()
/////////////////////////////////////
void
NPRPenUI::add_layer()
{
   int i;
   GTexture *t;
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr)
   {
      i = _listbox[LIST_LAYER]->get_int_val();

	  
      if (_radgroup[RADGROUP_GTEX]->get_int_val() == RADBUT_SOLID)
         t = new NPRSolidTexture(npr->patch()); //add solid NPR texture
      else if (_radgroup[RADGROUP_GTEX]->get_int_val() == RADBUT_TOON)
         t = new XToonTexture(npr->patch()); //add X-Toon texture
	  else if (_radgroup[RADGROUP_GTEX]->get_int_val() == RADBUT_XTOON)
		  t = new GLSLXToonShader(npr->patch()); //add GLSL X-Toon texture

      assert(t);

      npr->insert_basecoat(i,t);

      fill_layer_listbox();
      _listbox[LIST_LAYER]->set_int_val(i+1);
      update_gtex();

   }

}

/////////////////////////////////////
// del_layer()
/////////////////////////////////////
void
NPRPenUI::del_layer()
{
   int i;
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr)
   {
      i = _listbox[LIST_LAYER]->get_int_val();
      npr->remove_basecoat(i-1);

      _listbox[LIST_LAYER]->set_int_val(min(i,npr->get_basecoat_num()));
      update_gtex();
   }

}

/////////////////////////////////////
// smooth_normals()
/////////////////////////////////////
void
NPRPenUI::smooth_normals()
{
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr) {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);
      if (XToonTexture::upcast(t)) {
	 XToonTexture::upcast(t)->update_smoothing(true);
         update_gtex();
      } else if (GLSLXToonShader::upcast(t)) {
        GLSLXToonShader::upcast(t)->set_normals(0);
        update_gtex();
	  }
   }
}

/////////////////////////////////////
// elliptic_normals()
/////////////////////////////////////
void
NPRPenUI::elliptic_normals()
{
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr) {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);
      if (XToonTexture::upcast(t)) {
         XToonTexture::upcast(t)->update_elliptic(true);
         update_gtex();
      } else if (GLSLXToonShader::upcast(t)) { 
         GLSLXToonShader::upcast(t)->set_normals(2);
         update_gtex();
	  }
   }
}

/////////////////////////////////////
// spheric_normals()
/////////////////////////////////////
void
NPRPenUI::spheric_normals()
{
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr) {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);
      if (XToonTexture::upcast(t)) {
         XToonTexture::upcast(t)->update_spheric(true);
         update_gtex();
      } else if (GLSLXToonShader::upcast(t)) {
         GLSLXToonShader::upcast(t)->set_normals(1);
         update_gtex();
	  }
   }
}

/////////////////////////////////////
// cylindric_normals()
/////////////////////////////////////
void
NPRPenUI::cylindric_normals()
{
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr) {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);
      if (XToonTexture::upcast(t)) {
         XToonTexture::upcast(t)->update_cylindric(true);
         update_gtex();
      } else if (GLSLXToonShader::upcast(t)) {
         GLSLXToonShader::upcast(t)->set_normals(3);
         update_gtex();
      }
   }
}

/////////////////////////////////////
// compute_curvatures()
/////////////////////////////////////
void
NPRPenUI::compute_curvatures()
{
   NPRTexture *npr = _pen->curr_npr_tex();

   if (npr) {
      GTexture *t = npr->get_basecoat(_listbox[LIST_LAYER]->get_int_val()-1);
      if (XToonTexture::upcast(t)) {
         XToonTexture::upcast(t)->update_curvatures(true);
         update_gtex();
      }// else if (GLSLXToonShader::isa(t)) {
        // GLSLXToonShader *xtoon = (GLSLXToonShader*)t;
        // xtoon->update_curvatures(true);
        // update_gtex();
      //}
   }
}

/////////////////////////////////////
// listbox_cb()
/////////////////////////////////////

void
NPRPenUI::listbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK)
   {
    case LIST_TEX:
      cerr << "NPRPenUI::listbox_cb() - Tex\n";
      _ui[id >> ID_SHIFT]->apply_gtex();
    break;
    case LIST_LAYER:
      cerr << "NPRPenUI::listbox_cb() - Layer\n";
      _ui[id >> ID_SHIFT]->update_gtex();
    break;
    case LIST_DETAIL:
      cerr << "NPRPenUI::listbox_cb() - detail map\n";
      _ui[id >> ID_SHIFT]->update_gtex();
      _ui[id >> ID_SHIFT]->apply_gtex();
    break; 
    case LIST_DETAIL2:
      cerr << "NPRPenUI::listbox_cb() - smooth detail map\n";
      _ui[id >> ID_SHIFT]->update_gtex();
      _ui[id >> ID_SHIFT]->apply_gtex();
    break;  }
}


/////////////////////////////////////
// edittext_cb()
/////////////////////////////////////

void
NPRPenUI::edittext_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK)
   {
    case EDIT_NAME:
      cerr << "NPRPenUI::edittext_cb() - Layer Name\n";
      _ui[id >> ID_SHIFT]->rename_layer();
    break;
   }
}

/////////////////////////////////////
// button_cb()
/////////////////////////////////////

void
NPRPenUI::button_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case BUT_REFRESH:
         cerr << "StrokeUI::button_cb() - Refresh\n";
         _ui[id >> ID_SHIFT]->refresh_tex();
      break;
      case BUT_NEXT_PEN:
	BaseJOTapp::instance()->next_pen(); 
      break;
      case BUT_PREV_PEN:
	BaseJOTapp::instance()->prev_pen();
      break;
      case BUT_ADD:
         cerr << "StrokeUI::button_cb() - Add Layer\n";
         _ui[id >> ID_SHIFT]->add_layer();
      break;
      case BUT_DEL:
         cerr << "StrokeUI::button_cb() - Delete Layer\n";
         _ui[id >> ID_SHIFT]->del_layer();
      break;
      case BUT_SMOOTH:
         _ui[id >> ID_SHIFT]->smooth_normals();
      break;
      case BUT_ELLIPTIC:
         _ui[id >> ID_SHIFT]->elliptic_normals();
      break;
      case BUT_SPHERIC:
         _ui[id >> ID_SHIFT]->spheric_normals();
      break;
      case BUT_CYLINDRIC:
         _ui[id >> ID_SHIFT]->cylindric_normals();
      break;
//       case BUT_CURV:
//          _ui[id >> ID_SHIFT]->compute_curvatures();
//       break;
   }
}

/////////////////////////////////////
// slider_cb()
/////////////////////////////////////

void
NPRPenUI::slider_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case SLIDE_H:
      case SLIDE_S:
      case SLIDE_V:
      case SLIDE_A:
      case SLIDE_R:
         _ui[id >> ID_SHIFT]->apply_gtex();
      break;
      case SLIDE_T:
         _ui[id >> ID_SHIFT]->apply_gtex();
      break;
      case SLIDE_F:
         _ui[id >> ID_SHIFT]->apply_gtex();
      break;
      case SLIDE_L:
         _ui[id >> ID_SHIFT]->apply_gtex();
      break;
      
   }
}

/////////////////////////////////////
// radiogroup_cb()
/////////////////////////////////////

void
NPRPenUI::radiogroup_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());
   switch(id&ID_MASK)
   {
      case RADGROUP_GTEX:
      break;
      case RADGROUP_LIGHT:
         _ui[id >> ID_SHIFT]->apply_gtex();
         _ui[id >> ID_SHIFT]->update_gtex();
      break;
   }
}

/////////////////////////////////////
// checkbox_cb()
/////////////////////////////////////

void
NPRPenUI::checkbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
       case CHECK_TRANS:
       case CHECK_ANNOTATE:
       case CHECK_TRAVEL:
       case CHECK_SPEC: //
       case CHECK_DIR:  //Toon Lighting::Direction
       case CHECK_CAM:  //Toon Lighting::Cam Frame
          _ui[id >> ID_SHIFT]->apply_gtex();
       break;
       case CHECK_PAPER:  //apply paper efect
       case CHECK_LIGHT:  
	  _ui[id >> ID_SHIFT]->apply_gtex();
          _ui[id >> ID_SHIFT]->update_gtex();
       break;
       case CHECK_INV:    //invert detal
	  _ui[id >> ID_SHIFT]->apply_gtex();
          _ui[id >> ID_SHIFT]->update_gtex();          
       break;
   }
}

/////////////////////////////////////
// rotation_cb()
/////////////////////////////////////

void
NPRPenUI::rotation_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
       case ROT_L:
         _ui[id >> ID_SHIFT]->apply_gtex();
       break;
       case ROT_ROT:
         _ui[id >> ID_SHIFT]->update_xform_edits();    
         _ui[id >> ID_SHIFT]->apply_xform();    
       break;
   }
}

/////////////////////////////////////
// translation_cb()
/////////////////////////////////////

void
NPRPenUI::translation_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
       case TRAN_X:
       case TRAN_Y:
       case TRAN_Z:
         _ui[id >> ID_SHIFT]->update_xform_edits();    
         _ui[id >> ID_SHIFT]->apply_xform();
       break;
   }
}


/////////////////////////////////////
// scale_cb()
/////////////////////////////////////

void
NPRPenUI::scale_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
       case SCALE_UNIFORM:
         _ui[id >> ID_SHIFT]->update_scale_edits();    
         _ui[id >> ID_SHIFT]->apply_xform();
       break;
   }
}


/////////////////////////////////////
// edit_xform_cb()
/////////////////////////////////////

void
NPRPenUI::edit_xform_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   cerr << "edit_xform" << endl;

   switch(id&ID_MASK)
      {
       case EDIT_TRAN_X:
       case EDIT_TRAN_Y:
       case EDIT_TRAN_Z:
       case EDIT_SCALE_X:
       case EDIT_SCALE_Y:
       case EDIT_SCALE_Z:
       case EDIT_ROT_X:
       case EDIT_ROT_Y:
       case EDIT_ROT_Z:
         _ui[id >> ID_SHIFT]->get_xform_edits();
         _ui[id >> ID_SHIFT]->apply_xform();
         break;
      }
}


/////////////////////////////////////
// get_xform_edits()
/////////////////////////////////////

void
NPRPenUI::get_xform_edits()
{
   _translation[TRAN_X]->set_x(_edittext[EDIT_TRAN_X]->get_float_val());
   _translation[TRAN_Y]->set_y(_edittext[EDIT_TRAN_Y]->get_float_val());
   _translation[TRAN_Z]->set_z(_edittext[EDIT_TRAN_Z]->get_float_val());

   // set uniform scaling to average of X,Y,Z scale factors
   float scale = (_edittext[EDIT_SCALE_X]->get_float_val() +
                  _edittext[EDIT_SCALE_Y]->get_float_val() +
                  _edittext[EDIT_SCALE_Z]->get_float_val()) / 3.0;

   _scale[SCALE_UNIFORM]->set_y(scale);

   // get rotations

   float rad_x = deg2rad(_edittext[EDIT_ROT_X]->get_float_val());
   float rad_y = deg2rad(_edittext[EDIT_ROT_Y]->get_float_val());
   float rad_z = deg2rad(_edittext[EDIT_ROT_Z]->get_float_val());

   Wtransf rot_x = Wtransf::rotation(Wvec(1,0,0), rad_x);
   Wtransf rot_y = Wtransf::rotation(Wvec(0,1,0), rad_y);
   Wtransf rot_z = Wtransf::rotation(Wvec(0,0,1), rad_z);

   Wtransf rot = rot_y * rot_x * rot_z;

   Wvec x = rot.X();
   Wvec y = rot.Y();
   Wvec z = rot.Z();

   float mat[4][4];

   mat[0][0] = x[0];   mat[0][1] = x[1];   mat[0][2] = x[2];  mat[0][3] = 0;
   mat[1][0] = y[0];   mat[1][1] = y[1];   mat[1][2] = y[2];  mat[1][3] = 0;
   mat[2][0] = z[0];   mat[2][1] = z[1];   mat[2][2] = z[2];  mat[2][3] = 0;
   mat[3][0] = 0;      mat[3][1] = 0;      mat[3][2] = 0;     mat[3][3] = 1;

   _rotation[ROT_ROT]->set_float_array_val((float *)mat); 

}

/////////////////////////////////////
// update_xform_edits()
/////////////////////////////////////

void
NPRPenUI::update_xform_edits()
{
   _edittext[EDIT_TRAN_X]->set_float_val(_translation[TRAN_X]->get_x());
   _edittext[EDIT_TRAN_Y]->set_float_val(_translation[TRAN_Y]->get_y());
   _edittext[EDIT_TRAN_Z]->set_float_val(_translation[TRAN_Z]->get_z());


   // XXX -- to do: update rotations in edittext box

}


/////////////////////////////////////
// update_scale_edits()
/////////////////////////////////////

// Override nonuniform scaling in edittext boxes only when scaling
// widget is manipulated.
//
void
NPRPenUI::update_scale_edits()
{
   float scale = _scale[SCALE_UNIFORM]->get_y(); 

   if (scale < 1.0 ) {
      scale = 1.0/(2.0-scale);
   }

   _edittext[EDIT_SCALE_X]->set_float_val(scale);
   _edittext[EDIT_SCALE_Y]->set_float_val(scale);
   _edittext[EDIT_SCALE_Z]->set_float_val(scale);
}



