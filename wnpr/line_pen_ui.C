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
// LinePenUI
////////////////////////////////////////////


//This is relative to JOT_ROOT, and should
//contain ONLY the texture dots for hatching strokes
#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "wnpr/line_pen.H"
#include "wnpr/line_pen_ui.H"
#include "tess/tex_body.H"
#include "std/config.H"

#include "npr/npr_texture.H"
#include "npr/sil_and_crease_texture.H"

using namespace mlib;

/*****************************************************************
 * LinePenUI
 *****************************************************************/

/////////////////////////////////////
// Static variables
/////////////////////////////////////

ARRAY<LinePenUI*>          LinePenUI::_ui;

/////////////////////////////////////
// Constructor
/////////////////////////////////////

LinePenUI::LinePenUI(LinePen *p) :
   _id(0),
   _init(false),
   _pen(p)
{
   _ui += this;
   _id = (_ui.num()-1);
    
   // Defer init() until the first build()

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

LinePenUI::~LinePenUI()
{
   // XXX - Need to clean up? Nah, we never destroy these

   cerr << "LinePenUI::~LinePenUI - Error!!! Destructor not implemented.\n";
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
LinePenUI::init()
{
   assert(!_init);

   _init = true;

}

/////////////////////////////////////
// changed()
/////////////////////////////////////

void
LinePenUI::changed()
{
   apply_stroke();

   
}

/////////////////////////////////////
// show()
/////////////////////////////////////

void
LinePenUI::show()
{

   if (!StrokeUI::capture(_pen->view(),this))
      {
         cerr << "LinePenUI::show() - Error! Failed to capture StrokeUI.\n";
         return;
      }
   else
      {
         CBaseStroke *s = StrokeUI::get_params(_pen->view(),this); assert(s); 
         //_pen->params().stroke(*s);
         if (!StrokeUI::show(_pen->view(),this))
            cerr << "LinePenUI::show() - Error! Failed to show StrokeUI.\n";
      }

}


/////////////////////////////////////
// hide()
/////////////////////////////////////

void
LinePenUI::hide()
{

   if (!StrokeUI::hide(_pen->view(),this))
      {
         cerr << "LinePenUI::hide() - Error! Failed to hide StrokeUI.\n";
         return;
      }

   if (!StrokeUI::release(_pen->view(),this))
      cerr << "LinePenUI::hide() - Error! Failed to release StrokeUI.\n";

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
LinePenUI::update()
{
   update_non_lives();

   //Synchs lives and updates the widgets
   if (!StrokeUI::update(_pen->view(),this))
      cerr << "LinePenUI::update() - Error! Failed to update StrokeUI.\n";

}



/////////////////////////////////////
// build()
/////////////////////////////////////

void
LinePenUI::build(GLUI* glui, GLUI_Panel *p, int w) 
{

   int i;
   int id = _id << ID_SHIFT;

    //Init the control arrays
   assert(_button.num()==0);       for (i=0; i<BUT_NUM; i++)      _button.add(0);
   assert(_slider.num()==0);       for (i=0; i<SLIDE_NUM; i++)    _slider.add(0);
   assert(_panel.num()==0);        for (i=0; i<PANEL_NUM; i++)    _panel.add(0);
   assert(_rollout.num()==0);      for (i=0; i<ROLLOUT_NUM; i++)  _rollout.add(0);
   assert(_radgroup.num()==0);     for (i=0; i<RADGROUP_NUM; i++) _radgroup.add(0);
   assert(_radbutton.num()==0);    for (i=0; i<RADBUT_NUM; i++)   _radbutton.add(0);
   assert(_checkbox.num()==0);     for (i=0; i<CHECK_NUM; i++)    _checkbox.add(0);
   assert(_statictext.num()==0);   for (i=0; i<TEXT_NUM; i++)     _statictext.add(0);

   //Sub-panel containing silhouette flag controls
   _rollout[ROLLOUT_FLAGS] = glui->add_rollout_to_panel(p,"Line Types",true);
   assert(_rollout[ROLLOUT_FLAGS]);

   glui->add_separator_to_panel(_rollout[ROLLOUT_FLAGS]);
   _checkbox[CHECK_FLAG_SEE_THRU] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "See Thru",
      NULL,
      id+CHECK_FLAG_SEE_THRU,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SEE_THRU]);
   glui->add_separator_to_panel(_rollout[ROLLOUT_FLAGS]);
  
   glui->add_column_to_panel(_rollout[ROLLOUT_FLAGS],false);

   _checkbox[CHECK_FLAG_SIL_VIS] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Sil - Vis",
      NULL,
      id+CHECK_FLAG_SIL_VIS,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SIL_VIS]);

   _checkbox[CHECK_FLAG_SIL_HID] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Sil - Hid",
      NULL,
      id+CHECK_FLAG_SIL_HID,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SIL_HID]);

   _checkbox[CHECK_FLAG_SIL_OCC] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Sil - Occ",
      NULL,
      id+CHECK_FLAG_SIL_OCC,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SIL_OCC]);

   glui->add_column_to_panel(_rollout[ROLLOUT_FLAGS],false);

   _checkbox[CHECK_FLAG_SILBF_VIS] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Bak - Vis",
      NULL,
      id+CHECK_FLAG_SILBF_VIS,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SILBF_VIS]);

   _checkbox[CHECK_FLAG_SILBF_HID] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Bak - Hid",
      NULL,
      id+CHECK_FLAG_SILBF_HID,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SILBF_HID]);

   _checkbox[CHECK_FLAG_SILBF_OCC] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Bak - Occ",
      NULL,
      id+CHECK_FLAG_SILBF_OCC,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_SILBF_OCC]);

   glui->add_column_to_panel(_rollout[ROLLOUT_FLAGS],false);

   _checkbox[CHECK_FLAG_BORDER_VIS] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Bor - Vis",
      NULL,
      id+CHECK_FLAG_BORDER_VIS,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_BORDER_VIS]);

   _checkbox[CHECK_FLAG_BORDER_HID] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Bor - Hid",
      NULL,
      id+CHECK_FLAG_BORDER_HID,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_BORDER_HID]);

   _checkbox[CHECK_FLAG_BORDER_OCC] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Bor - Occ",
      NULL,
      id+CHECK_FLAG_BORDER_OCC,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_BORDER_OCC]);

   glui->add_column_to_panel(_rollout[ROLLOUT_FLAGS],false);

   _checkbox[CHECK_FLAG_CREASE_VIS] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Cre - Vis",
      NULL,
      id+CHECK_FLAG_CREASE_VIS,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_CREASE_VIS]);

   _checkbox[CHECK_FLAG_CREASE_HID] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Cre - Hid",
      NULL,
      id+CHECK_FLAG_CREASE_HID,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_CREASE_HID]);

   _checkbox[CHECK_FLAG_CREASE_OCC] = glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_FLAGS],
      "Cre - Occ",
      NULL,
      id+CHECK_FLAG_CREASE_OCC,
      checkbox_cb);
   assert(_checkbox[CHECK_FLAG_CREASE_OCC]);

   glui->add_column_to_panel(_rollout[ROLLOUT_FLAGS],false);


   //Sub-panel containing silhouette coherence controls
   _rollout[ROLLOUT_COHER] = glui->add_rollout_to_panel(p,"Coherence",true);
   assert(_rollout[ROLLOUT_COHER]);

//
   _panel[PANEL_COHER_OPTS] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_COHER],
      "");
   assert(_panel[PANEL_COHER_OPTS]);

   _checkbox[CHECK_COHER_GLOBAL] = glui->add_checkbox_to_panel(
      _panel[PANEL_COHER_OPTS],
      "Global",
      NULL,
      id+CHECK_COHER_GLOBAL,
      checkbox_cb);
   assert(_checkbox[CHECK_COHER_GLOBAL]);

   _checkbox[CHECK_COHER_SIG_1] = glui->add_checkbox_to_panel(
      _panel[PANEL_COHER_OPTS],
      "NoSig",
      NULL,
      id+CHECK_COHER_SIG_1,
      checkbox_cb);
   assert(_checkbox[CHECK_COHER_SIG_1]);

   _radgroup[RADGROUP_COHER_COVER] = glui->add_radiogroup_to_panel(
      _panel[PANEL_COHER_OPTS],
      NULL,
      id+RADGROUP_COHER_COVER, radiogroup_cb);
   assert(_radgroup[RADGROUP_COHER_COVER]);

   _radbutton[RADBUT_COHER_COVER_MAJ] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_COVER],
      "Major");
   assert(_radbutton[RADBUT_COHER_COVER_MAJ]);
   _radbutton[RADBUT_COHER_COVER_MAJ]->set_w(5);

   _radbutton[RADBUT_COHER_COVER_1_TO_1] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_COVER],
      "1 to 1");
   assert(_radbutton[RADBUT_COHER_COVER_1_TO_1]);
   _radbutton[RADBUT_COHER_COVER_1_TO_1]->set_w(5);

   _radbutton[RADBUT_COHER_COVER_TRIM] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_COVER],
      "Hybrid");
   assert(_radbutton[RADBUT_COHER_COVER_TRIM]);
   _radbutton[RADBUT_COHER_COVER_TRIM]->set_w(5);


   glui->add_column_to_panel(_rollout[ROLLOUT_COHER],false);

   _panel[PANEL_COHER_FIT] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_COHER],
      "");
   assert(_panel[PANEL_COHER_FIT]);

   _radgroup[RADGROUP_COHER_FIT] = glui->add_radiogroup_to_panel(
      _panel[PANEL_COHER_FIT],
      NULL,
      id+RADGROUP_COHER_FIT, radiogroup_cb);
   assert(_radgroup[RADGROUP_COHER_FIT]);

   _radbutton[RADBUT_COHER_FIT_RAND] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_FIT],
      "Rand");
   assert(_radbutton[RADBUT_COHER_FIT_RAND]);
   _radbutton[RADBUT_COHER_FIT_RAND]->set_w(5);

   _radbutton[RADBUT_COHER_FIT_ARC] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_FIT],
      "ArcL");
   assert(_radbutton[RADBUT_COHER_FIT_ARC]);
   _radbutton[RADBUT_COHER_FIT_ARC]->set_w(5);

   _radbutton[RADBUT_COHER_FIT_PHASE] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_FIT],
      "Phase");
   assert(_radbutton[RADBUT_COHER_FIT_PHASE]);
   _radbutton[RADBUT_COHER_FIT_PHASE]->set_w(5);

   _radbutton[RADBUT_COHER_FIT_INTERP] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_FIT],
      "Interp");
   assert(_radbutton[RADBUT_COHER_FIT_INTERP]);
   _radbutton[RADBUT_COHER_FIT_INTERP]->set_w(5);

   _radbutton[RADBUT_COHER_FIT_OPTIM] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COHER_FIT],
      "Optim");
   assert(_radbutton[RADBUT_COHER_FIT_OPTIM]);
   _radbutton[RADBUT_COHER_FIT_OPTIM]->set_w(5);


   glui->add_column_to_panel(_rollout[ROLLOUT_COHER],false);

   _slider[SLIDE_COHER_PIX] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Pix", 
      GLUI_SLIDER_FLOAT, 
      6, 200,
      NULL,
      id+SLIDE_COHER_PIX, slider_cb);
   assert(_slider[SLIDE_COHER_PIX]);
   _slider[SLIDE_COHER_PIX]->set_num_graduations(195);

   _slider[SLIDE_COHER_MV] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Mv", 
      GLUI_SLIDER_INT, 
      1, 20,
      NULL,
      id+SLIDE_COHER_MV, slider_cb);
   assert(_slider[SLIDE_COHER_MV]);
   _slider[SLIDE_COHER_MV]->set_num_graduations(20);

   glui->add_column_to_panel(_rollout[ROLLOUT_COHER],false);

   _slider[SLIDE_COHER_WF] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Wf", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      NULL,
      id+SLIDE_COHER_WF, slider_cb);
   assert(_slider[SLIDE_COHER_WF]);
   _slider[SLIDE_COHER_WF]->set_num_graduations(201);

   _slider[SLIDE_COHER_MP] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Mp", 
      GLUI_SLIDER_INT, 
      0, 100,
      NULL,
      id+SLIDE_COHER_MP, slider_cb);
   assert(_slider[SLIDE_COHER_MP]);
   _slider[SLIDE_COHER_MP]->set_num_graduations(101);


   glui->add_column_to_panel(_rollout[ROLLOUT_COHER],false);

   _slider[SLIDE_COHER_WS] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Ws", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      NULL,
      id+SLIDE_COHER_WS, slider_cb);
   assert(_slider[SLIDE_COHER_WS]);
   _slider[SLIDE_COHER_WS]->set_num_graduations(201);

   _slider[SLIDE_COHER_M5] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "M%", 
      GLUI_SLIDER_INT, 
      0, 100,
      NULL,
      id+SLIDE_COHER_M5, slider_cb);
   assert(_slider[SLIDE_COHER_M5]);
   _slider[SLIDE_COHER_M5]->set_num_graduations(101);

   glui->add_column_to_panel(_rollout[ROLLOUT_COHER],false);

   _slider[SLIDE_COHER_WB] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Wb", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      NULL,
      id+SLIDE_COHER_WB, slider_cb);
   assert(_slider[SLIDE_COHER_WB]);
   _slider[SLIDE_COHER_WB]->set_num_graduations(201);

   _slider[SLIDE_COHER_HJ] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Hj", 
      GLUI_SLIDER_INT, 
      1, 100,
      NULL,
      id+SLIDE_COHER_HJ, slider_cb);
   assert(_slider[SLIDE_COHER_HJ]);
   _slider[SLIDE_COHER_HJ]->set_num_graduations(100);


   glui->add_column_to_panel(_rollout[ROLLOUT_COHER],false);

   _slider[SLIDE_COHER_WH] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Wh", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      NULL,
      id+SLIDE_COHER_WH, slider_cb);
   assert(_slider[SLIDE_COHER_WH]);
   _slider[SLIDE_COHER_WH]->set_num_graduations(201);

   _slider[SLIDE_COHER_HT] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_COHER], 
      "Ht", 
      GLUI_SLIDER_INT, 
      1, 1000,
      NULL,
      id+SLIDE_COHER_HT, slider_cb);
   assert(_slider[SLIDE_COHER_HT]);
   _slider[SLIDE_COHER_HT]->set_num_graduations(1000);

   
   //Mesh/Crease rollout
   _rollout[ROLLOUT_MESH] = glui->add_rollout_to_panel(p,"Mesh",true);
   assert(_rollout[ROLLOUT_MESH]);

   glui->add_separator_to_panel(_rollout[ROLLOUT_MESH]);
   _button[BUT_MESH_RECREASE] = glui->add_button_to_panel(
      _rollout[ROLLOUT_MESH],
      "Regen. Creases",
      id+BUT_MESH_RECREASE,
      button_cb);
   assert(_button[BUT_MESH_RECREASE]);
   _button[BUT_MESH_RECREASE]->set_w(1);
   glui->add_separator_to_panel(_rollout[ROLLOUT_MESH]);

   _slider[SLIDE_MESH_CREASE_VIS_STEP] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MESH], 
      "Crease Visibility Step", 
      GLUI_SLIDER_FLOAT, 
      1, 200,
      NULL,
      id+SLIDE_MESH_CREASE_VIS_STEP, slider_cb);
   assert(_slider[SLIDE_MESH_CREASE_VIS_STEP]);
   _slider[SLIDE_MESH_CREASE_VIS_STEP]->set_w(150);
   _slider[SLIDE_MESH_CREASE_VIS_STEP]->set_num_graduations(200);

   glui->add_column_to_panel(_rollout[ROLLOUT_MESH],false);

   _slider[SLIDE_MESH_CREASE_DETECT_ANGLE] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MESH], 
      "Crease Detect Angle", 
      GLUI_SLIDER_FLOAT, 
      0, 180,
      NULL,
      id+SLIDE_MESH_CREASE_DETECT_ANGLE, slider_cb);
   assert(_slider[SLIDE_MESH_CREASE_DETECT_ANGLE]);
   _slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->set_w(150);
   _slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->set_num_graduations(361);

   _slider[SLIDE_MESH_CREASE_JOINT_ANGLE] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MESH], 
      "Crease Joint Angle", 
      GLUI_SLIDER_FLOAT, 
      0, 180,
      NULL,
      id+SLIDE_MESH_CREASE_JOINT_ANGLE, slider_cb);
   assert(_slider[SLIDE_MESH_CREASE_JOINT_ANGLE]);
   _slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->set_w(150);
   _slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->set_num_graduations(361);

   glui->add_column_to_panel(_rollout[ROLLOUT_MESH],false);

   _slider[SLIDE_MESH_POLY_FACTOR] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MESH], 
      "Polygon Offset Factor", 
      GLUI_SLIDER_FLOAT, 
      0, 30,
      NULL,
      id+SLIDE_MESH_POLY_FACTOR, slider_cb);
   assert(_slider[SLIDE_MESH_POLY_FACTOR]);
   _slider[SLIDE_MESH_POLY_FACTOR]->set_w(150);
   _slider[SLIDE_MESH_POLY_FACTOR]->set_num_graduations(301);

   _slider[SLIDE_MESH_POLY_UNITS] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MESH], 
      "Polygon Offset Units", 
      GLUI_SLIDER_FLOAT, 
      0, 30,
      NULL,
      id+SLIDE_MESH_POLY_UNITS, slider_cb);
   assert(_slider[SLIDE_MESH_POLY_UNITS]);
   _slider[SLIDE_MESH_POLY_UNITS]->set_w(150);
   _slider[SLIDE_MESH_POLY_UNITS]->set_num_graduations(301);

   //Noise rollout
   _rollout[ROLLOUT_NOISE] = glui->add_rollout_to_panel(p,"Noise",true);
   assert(_rollout[ROLLOUT_NOISE]);

   _panel[PANEL_NOISE_PROTOTYPE] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_NOISE],
      "Per-Line-Type Prototype Controls");
   assert(_panel[PANEL_NOISE_PROTOTYPE]);

   _panel[PANEL_NOISE_PROTOTYPE_TEXT] = glui->add_panel_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE],
      "Selected");
   assert(_panel[PANEL_NOISE_PROTOTYPE_TEXT]);

   _statictext[TEXT_NOISE_PROTOTYPE] = glui->add_statictext_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE_TEXT],
      "00 of 00");
   assert(_statictext[TEXT_NOISE_PROTOTYPE]);
   _statictext[TEXT_NOISE_PROTOTYPE]->set_w(50);
   _statictext[TEXT_NOISE_PROTOTYPE]->set_alignment(GLUI_ALIGN_CENTER);
   _panel[PANEL_NOISE_PROTOTYPE_TEXT]->set_w(50);

   glui->add_column_to_panel(_panel[PANEL_NOISE_PROTOTYPE],false);

   _panel[PANEL_NOISE_PROTOTYPE_CONTROLS] = glui->add_panel_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE],
      "Editing");
   assert(_panel[PANEL_NOISE_PROTOTYPE_CONTROLS]);

   _button[BUT_NOISE_PROTOTYPE_NEXT] = glui->add_button_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE_CONTROLS],
      "Next",
      id+BUT_NOISE_PROTOTYPE_NEXT,
      button_cb);
   assert(_button[BUT_NOISE_PROTOTYPE_NEXT]);

   glui->add_column_to_panel(_panel[PANEL_NOISE_PROTOTYPE_CONTROLS],false);

   _button[BUT_NOISE_PROTOTYPE_DEL] = glui->add_button_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE_CONTROLS],
      "Del",
      id+BUT_NOISE_PROTOTYPE_DEL,
      button_cb);
   assert(_button[BUT_NOISE_PROTOTYPE_DEL]);

   glui->add_column_to_panel(_panel[PANEL_NOISE_PROTOTYPE_CONTROLS],false);

   _button[BUT_NOISE_PROTOTYPE_ADD] = glui->add_button_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE_CONTROLS],
      "Add",
      id+BUT_NOISE_PROTOTYPE_ADD,
      button_cb);
   assert(_button[BUT_NOISE_PROTOTYPE_ADD]);

   glui->add_column_to_panel(_panel[PANEL_NOISE_PROTOTYPE_CONTROLS],false);

   _checkbox[CHECK_NOISE_PROTOTYPE_LOCK] = glui->add_checkbox_to_panel(
      _panel[PANEL_NOISE_PROTOTYPE_CONTROLS],
      "Lock",
      NULL,
      id+CHECK_NOISE_PROTOTYPE_LOCK,
      checkbox_cb);
   assert(_checkbox[CHECK_NOISE_PROTOTYPE_LOCK]);

   _panel[PANEL_NOISE_OBJECT] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_NOISE],
      "Per-Patch Noise Controls");
   assert(_panel[PANEL_NOISE_OBJECT]);

   glui->add_separator_to_panel(_panel[PANEL_NOISE_OBJECT]);
   _checkbox[CHECK_NOISE_OBJECT_MOTION] = glui->add_checkbox_to_panel(
      _panel[PANEL_NOISE_OBJECT],
      "Motion Only",
      NULL,
      id+CHECK_NOISE_OBJECT_MOTION,
      checkbox_cb);
   assert(_checkbox[CHECK_NOISE_OBJECT_MOTION]);
   glui->add_separator_to_panel(_panel[PANEL_NOISE_OBJECT]);

   glui->add_column_to_panel(_panel[PANEL_NOISE_OBJECT],false);

   _slider[SLIDE_NOISE_OBJECT_FREQUENCY] = glui->add_slider_to_panel(
      _panel[PANEL_NOISE_OBJECT], "Frequency", 
      GLUI_SLIDER_FLOAT, 
      0, 30,
      NULL,
      id+SLIDE_NOISE_OBJECT_FREQUENCY, slider_cb);
   assert(_slider[SLIDE_NOISE_OBJECT_FREQUENCY]);
   _slider[SLIDE_NOISE_OBJECT_FREQUENCY]->set_num_graduations(61);

   glui->add_column_to_panel(_panel[PANEL_NOISE_OBJECT],false);

   _slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER] = glui->add_slider_to_panel(
      _panel[PANEL_NOISE_OBJECT], "Rand. Order", 
      GLUI_SLIDER_FLOAT, 
      0, 1,
      NULL,
      id+SLIDE_NOISE_OBJECT_RANDOM_ORDER, slider_cb);
   assert(_slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]);
   _slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->set_num_graduations(101);

   glui->add_column_to_panel(_panel[PANEL_NOISE_OBJECT],false);

   _slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION] = glui->add_slider_to_panel(
      _panel[PANEL_NOISE_OBJECT], "Rand. Duration", 
      GLUI_SLIDER_FLOAT, 
      0, 1,
      NULL,
      id+SLIDE_NOISE_OBJECT_RANDOM_DURATION, slider_cb);
   assert(_slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]);
   _slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->set_num_graduations(101);

   //Edit rollout
   _rollout[ROLLOUT_EDIT] = glui->add_rollout_to_panel(p,"Stylization",true);
   assert(_rollout[ROLLOUT_EDIT]);

   _panel[PANEL_EDIT_STATUS] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Status");
   assert(_panel[PANEL_EDIT_STATUS]);
   
   _statictext[TEXT_EDIT_STATUS_1] = glui->add_statictext_to_panel(
      _panel[PANEL_EDIT_STATUS],
      "STATUS");
   assert(_statictext[TEXT_EDIT_STATUS_1]);
   _statictext[TEXT_EDIT_STATUS_1]->set_w(125);
   //_statictext[TEXT_EDIT_STATUS_1]->set_alignment(GLUI_ALIGN_CENTER);

   _statictext[TEXT_EDIT_STATUS_2] = glui->add_statictext_to_panel(
      _panel[PANEL_EDIT_STATUS],
      "STATUS");
   assert(_statictext[TEXT_EDIT_STATUS_2]);
   _statictext[TEXT_EDIT_STATUS_2]->set_w(125);
   //_statictext[TEXT_EDIT_STATUS_2]->set_alignment(GLUI_ALIGN_CENTER);

   _panel[PANEL_EDIT_CYCLE] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Cycle Select");
   assert(_panel[PANEL_EDIT_CYCLE]);

   _button[BUT_EDIT_CYCLE_LINE_TYPES] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_CYCLE],
      "Line Types",
      id+BUT_EDIT_CYCLE_LINE_TYPES,
      button_cb);
   assert(_button[BUT_EDIT_CYCLE_LINE_TYPES]);
   _button[BUT_EDIT_CYCLE_LINE_TYPES]->set_w(125);

   glui->add_separator_to_panel(_panel[PANEL_EDIT_CYCLE]);

   _button[BUT_EDIT_CYCLE_DECAL_GROUPS] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_CYCLE],
      "Decal Marks",
      id+BUT_EDIT_CYCLE_DECAL_GROUPS,
      button_cb);
   assert(_button[BUT_EDIT_CYCLE_DECAL_GROUPS]);
   _button[BUT_EDIT_CYCLE_DECAL_GROUPS]->set_w(125);

   glui->add_separator_to_panel(_panel[PANEL_EDIT_CYCLE]);

   _button[BUT_EDIT_CYCLE_CREASE_PATHS] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_CYCLE],
      "Crease Paths",
      id+BUT_EDIT_CYCLE_CREASE_PATHS,
      button_cb);
   assert(_button[BUT_EDIT_CYCLE_CREASE_PATHS]);
   _button[BUT_EDIT_CYCLE_CREASE_PATHS]->set_w(125);

   _button[BUT_EDIT_CYCLE_CREASE_STROKES] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_CYCLE],
      "Crease Strokes",
      id+BUT_EDIT_CYCLE_CREASE_STROKES,
      button_cb);
   assert(_button[BUT_EDIT_CYCLE_CREASE_STROKES]);
   _button[BUT_EDIT_CYCLE_CREASE_STROKES]->set_w(125);

   glui->add_column_to_panel(_rollout[ROLLOUT_EDIT],false);

   _panel[PANEL_EDIT_OFFSETS] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Offsets");
   assert(_panel[PANEL_EDIT_OFFSETS]);

   _button[BUT_EDIT_OFFSET_EDIT] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_OFFSETS],
      "Edit Selected",
      id+BUT_EDIT_OFFSET_EDIT,
      button_cb);
   assert(_button[BUT_EDIT_OFFSET_EDIT]);
   _button[BUT_EDIT_OFFSET_EDIT]->set_w(110);

   _button[BUT_EDIT_OFFSET_CLEAR] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_OFFSETS],
      "Clear Selected",
      id+BUT_EDIT_OFFSET_CLEAR,
      button_cb);
   assert(_button[BUT_EDIT_OFFSET_CLEAR]);
   _button[BUT_EDIT_OFFSET_CLEAR]->set_w(110);

   glui->add_separator_to_panel(_panel[PANEL_EDIT_OFFSETS]);

   _button[BUT_EDIT_OFFSET_UNDO] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_OFFSETS],
      "Undo Sketch",
      id+BUT_EDIT_OFFSET_UNDO,
      button_cb);
   assert(_button[BUT_EDIT_OFFSET_UNDO]);
   _button[BUT_EDIT_OFFSET_UNDO]->set_w(110);

   _button[BUT_EDIT_OFFSET_APPLY] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_OFFSETS],
      "Apply Sketch",
      id+BUT_EDIT_OFFSET_APPLY,
      button_cb);
   assert(_button[BUT_EDIT_OFFSET_APPLY]);
   _button[BUT_EDIT_OFFSET_APPLY]->set_w(110);

   glui->add_separator_to_panel(_rollout[ROLLOUT_EDIT]);   

   _panel[PANEL_EDIT_STYLE] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Style");
   assert(_panel[PANEL_EDIT_STYLE]);

   _button[BUT_EDIT_STYLE_APPLY] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_STYLE],
      "Apply Current",
      id+BUT_EDIT_STYLE_APPLY,
      button_cb);
   assert(_button[BUT_EDIT_STYLE_APPLY]);
   _button[BUT_EDIT_STYLE_APPLY]->set_w(110);

   _button[BUT_EDIT_STYLE_GET] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_STYLE],
      "Get Selected",
      id+BUT_EDIT_STYLE_GET,
      button_cb);
   assert(_button[BUT_EDIT_STYLE_GET]);
   _button[BUT_EDIT_STYLE_GET]->set_w(110);

   glui->add_column_to_panel(_rollout[ROLLOUT_EDIT],false);

   _panel[PANEL_EDIT_PRESSURE] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Pressure");
   assert(_panel[PANEL_EDIT_PRESSURE]);

   _checkbox[CHECK_EDIT_PRESSURE_WIDTH] = glui->add_checkbox_to_panel(
      _panel[PANEL_EDIT_PRESSURE],
      "Vary Width",
      NULL,
      id+CHECK_EDIT_PRESSURE_WIDTH,
      checkbox_cb);
   assert(_checkbox[CHECK_EDIT_PRESSURE_WIDTH]);
   
   _checkbox[CHECK_EDIT_PRESSURE_ALPHA] = glui->add_checkbox_to_panel(
      _panel[PANEL_EDIT_PRESSURE],
      "Vary Alpha",
      NULL,
      id+CHECK_EDIT_PRESSURE_ALPHA,
      checkbox_cb);
   assert(_checkbox[CHECK_EDIT_PRESSURE_ALPHA]);

   glui->add_separator_to_panel(_rollout[ROLLOUT_EDIT]);   

   _panel[PANEL_EDIT_OVERSKETCH] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Baseline");
   assert(_panel[PANEL_EDIT_OVERSKETCH]);

   _radgroup[RADGROUP_EDIT_OVERSKETCH] = glui->add_radiogroup_to_panel(
      _panel[PANEL_EDIT_OVERSKETCH],
      NULL,
      id+RADGROUP_EDIT_OVERSKETCH, radiogroup_cb);
   assert(_radgroup[RADGROUP_EDIT_OVERSKETCH]);

   _radbutton[RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_EDIT_OVERSKETCH],
      "Virtual");
   assert(_radbutton[RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE]);

   _radbutton[RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE] = glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_EDIT_OVERSKETCH],
      "Selected");
   assert(_radbutton[RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE]);

   glui->add_separator_to_panel(_rollout[ROLLOUT_EDIT]);   

   _panel[PANEL_EDIT_STROKES] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Strokes");
   assert(_panel[PANEL_EDIT_STROKES]);

   _button[BUT_EDIT_STROKE_ADD] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_STROKES],
      "Add New",
      id+BUT_EDIT_STROKE_ADD,
      button_cb);
   assert(_button[BUT_EDIT_STROKE_ADD]);
   _button[BUT_EDIT_STROKE_ADD]->set_w(90);

   _button[BUT_EDIT_STROKE_DEL] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_STROKES],
      "Delete",
      id+BUT_EDIT_STROKE_DEL,
      button_cb);
   assert(_button[BUT_EDIT_STROKE_DEL]);
   _button[BUT_EDIT_STROKE_DEL]->set_w(90);

   glui->add_column_to_panel(_rollout[ROLLOUT_EDIT],false);

   _panel[PANEL_EDIT_SYNTHESIS] = glui->add_panel_to_panel(
      _rollout[ROLLOUT_EDIT],
      "Synthesis");
   assert(_panel[PANEL_EDIT_SYNTHESIS]);

   _button[BUT_EDIT_SYNTH_RUBBER] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Rubberstamp",
      id+BUT_EDIT_SYNTH_RUBBER,
      button_cb);
   assert(_button[BUT_EDIT_SYNTH_RUBBER]);
   _button[BUT_EDIT_SYNTH_RUBBER]->set_w(110);

   _button[BUT_EDIT_SYNTH_SYNTHESIZE] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Synthesize",
      id+BUT_EDIT_SYNTH_SYNTHESIZE,
      button_cb);
   assert(_button[BUT_EDIT_SYNTH_SYNTHESIZE]);
   _button[BUT_EDIT_SYNTH_SYNTHESIZE]->set_w(110);

   glui->add_separator_to_panel(_panel[PANEL_EDIT_SYNTHESIS]);

   _statictext[TEXT_EDIT_SYNTH_COUNT] = glui->add_statictext_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Protos: X");
   assert(_statictext[TEXT_EDIT_SYNTH_COUNT]);
   _statictext[TEXT_EDIT_SYNTH_COUNT]->set_w(5);
   _statictext[TEXT_EDIT_SYNTH_COUNT]->set_alignment(GLUI_ALIGN_CENTER);

   _button[BUT_EDIT_SYNTH_EX_ADD] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Add Proto",
      id+BUT_EDIT_SYNTH_EX_ADD,
      button_cb);
   assert(_button[BUT_EDIT_SYNTH_EX_ADD]);
   _button[BUT_EDIT_SYNTH_EX_ADD]->set_w(110);

   _button[BUT_EDIT_SYNTH_EX_DEL] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Clear Last",
      id+BUT_EDIT_SYNTH_EX_DEL,
      button_cb);
   assert(_button[BUT_EDIT_SYNTH_EX_DEL]);
   _button[BUT_EDIT_SYNTH_EX_DEL]->set_w(110);

   _button[BUT_EDIT_SYNTH_EX_CLEAR] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Clear All",
      id+BUT_EDIT_SYNTH_EX_CLEAR,
      button_cb);
   assert(_button[BUT_EDIT_SYNTH_EX_CLEAR]);
   _button[BUT_EDIT_SYNTH_EX_CLEAR]->set_w(110);


   glui->add_separator_to_panel(_panel[PANEL_EDIT_SYNTHESIS]);

   _button[BUT_EDIT_SYNTH_ALL_CLEAR] = glui->add_button_to_panel(
      _panel[PANEL_EDIT_SYNTHESIS],
      "Clear Creases",
      id+BUT_EDIT_SYNTH_ALL_CLEAR,
      button_cb);
   assert(_button[BUT_EDIT_SYNTH_ALL_CLEAR]);
   _button[BUT_EDIT_SYNTH_ALL_CLEAR]->set_w(110);

   cleanup_sizes(p,w);

   // One-time inits...
   if (!_init) init();

   _rollout[ROLLOUT_MESH]->close(); 
   _rollout[ROLLOUT_NOISE]->close();

}



/////////////////////////////////////
// cleanup_sizes()
/////////////////////////////////////

void
LinePenUI::cleanup_sizes(GLUI_Panel *p, int w) 
{

   int delta;
   
   delta = (p->get_w() - w)/5;
   _slider[SLIDE_COHER_PIX]->set_w(_slider[SLIDE_COHER_PIX]->get_w() - delta);
   _slider[SLIDE_COHER_WF]->set_w(_slider[SLIDE_COHER_WF]->get_w() - delta);
   _slider[SLIDE_COHER_WS]->set_w(_slider[SLIDE_COHER_WS]->get_w() - delta);
   _slider[SLIDE_COHER_WB]->set_w(_slider[SLIDE_COHER_WB]->get_w() - delta);
   _slider[SLIDE_COHER_WH]->set_w(_slider[SLIDE_COHER_WH]->get_w() - delta);
   _slider[SLIDE_COHER_MV]->set_w(_slider[SLIDE_COHER_MV]->get_w() - delta);
   _slider[SLIDE_COHER_MP]->set_w(_slider[SLIDE_COHER_MP]->get_w() - delta);
   _slider[SLIDE_COHER_M5]->set_w(_slider[SLIDE_COHER_M5]->get_w() - delta);
   _slider[SLIDE_COHER_HT]->set_w(_slider[SLIDE_COHER_HT]->get_w() - delta);
   _slider[SLIDE_COHER_HJ]->set_w(_slider[SLIDE_COHER_HJ]->get_w() - delta);

   delta = (_rollout[ROLLOUT_FLAGS]->get_w() - _rollout[ROLLOUT_COHER]->get_w())/6;
   _checkbox[CHECK_FLAG_SEE_THRU]->set_w(_checkbox[CHECK_FLAG_SEE_THRU]->get_w() - delta);   
   _checkbox[CHECK_FLAG_SIL_VIS]->set_w(_checkbox[CHECK_FLAG_SIL_VIS]->get_w() - delta);   
   _checkbox[CHECK_FLAG_SIL_HID]->set_w(_checkbox[CHECK_FLAG_SIL_HID]->get_w() - delta);   
   _checkbox[CHECK_FLAG_SIL_OCC]->set_w(_checkbox[CHECK_FLAG_SIL_OCC]->get_w() - delta);   
   _checkbox[CHECK_FLAG_SILBF_VIS]->set_w(_checkbox[CHECK_FLAG_SILBF_VIS]->get_w() - delta);   
   _checkbox[CHECK_FLAG_SILBF_HID]->set_w(_checkbox[CHECK_FLAG_SILBF_HID]->get_w() - delta);   
   _checkbox[CHECK_FLAG_SILBF_OCC]->set_w(_checkbox[CHECK_FLAG_SILBF_OCC]->get_w() - delta);   
   _checkbox[CHECK_FLAG_BORDER_VIS]->set_w(_checkbox[CHECK_FLAG_BORDER_VIS]->get_w() - delta);   
   _checkbox[CHECK_FLAG_BORDER_HID]->set_w(_checkbox[CHECK_FLAG_BORDER_HID]->get_w() - delta);   
   _checkbox[CHECK_FLAG_BORDER_OCC]->set_w(_checkbox[CHECK_FLAG_BORDER_OCC]->get_w() - delta);   
   _checkbox[CHECK_FLAG_CREASE_VIS]->set_w(_checkbox[CHECK_FLAG_CREASE_VIS]->get_w() - delta);   
   _checkbox[CHECK_FLAG_CREASE_HID]->set_w(_checkbox[CHECK_FLAG_CREASE_HID]->get_w() - delta);   
   _checkbox[CHECK_FLAG_CREASE_OCC]->set_w(_checkbox[CHECK_FLAG_CREASE_OCC]->get_w() - delta);   

   delta = (_rollout[ROLLOUT_MESH]->get_w() - _rollout[ROLLOUT_COHER]->get_w())/3;
   _slider[SLIDE_MESH_CREASE_VIS_STEP]->set_w(_slider[SLIDE_MESH_CREASE_VIS_STEP]->get_w() - delta);   
   _slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->set_w(_slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->get_w() - delta);   
   _slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->set_w(_slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->get_w() - delta);   
   _slider[SLIDE_MESH_POLY_FACTOR]->set_w(_slider[SLIDE_MESH_POLY_FACTOR]->get_w() - delta);   
   _slider[SLIDE_MESH_POLY_UNITS]->set_w(_slider[SLIDE_MESH_POLY_UNITS]->get_w() - delta);   
   _button[BUT_MESH_RECREASE]->set_w(_slider[SLIDE_MESH_CREASE_VIS_STEP]->get_w());   

   delta = (_rollout[ROLLOUT_NOISE]->get_w() - _rollout[ROLLOUT_COHER]->get_w())/3;
   _button[BUT_NOISE_PROTOTYPE_NEXT]->set_w(_button[BUT_NOISE_PROTOTYPE_NEXT]->get_w() - delta);   
   _button[BUT_NOISE_PROTOTYPE_DEL]->set_w(_button[BUT_NOISE_PROTOTYPE_DEL]->get_w() - delta);   
   _button[BUT_NOISE_PROTOTYPE_ADD]->set_w(_button[BUT_NOISE_PROTOTYPE_ADD]->get_w() - delta);   
   delta = (_panel[PANEL_NOISE_OBJECT]->get_w() - _panel[PANEL_NOISE_PROTOTYPE]->get_w())/3;
   _slider[SLIDE_NOISE_OBJECT_FREQUENCY]->set_w(_slider[SLIDE_NOISE_OBJECT_FREQUENCY]->get_w() - delta);   
   _slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->set_w(_slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->get_w() - delta);   
   _slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->set_w(_slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->get_w() - delta);   


   delta = (_rollout[ROLLOUT_EDIT]->get_w() - _rollout[ROLLOUT_COHER]->get_w())/3;
   _button[BUT_EDIT_CYCLE_LINE_TYPES]->set_w(_button[BUT_EDIT_CYCLE_LINE_TYPES]->get_w() - delta);   
   _button[BUT_EDIT_CYCLE_DECAL_GROUPS]->set_w(_button[BUT_EDIT_CYCLE_DECAL_GROUPS]->get_w() - delta);   
   _button[BUT_EDIT_CYCLE_CREASE_PATHS]->set_w(_button[BUT_EDIT_CYCLE_CREASE_PATHS]->get_w() - delta);   
   _button[BUT_EDIT_CYCLE_CREASE_STROKES]->set_w(_button[BUT_EDIT_CYCLE_CREASE_STROKES]->get_w() - delta);   
   _button[BUT_EDIT_OFFSET_EDIT]->set_w(_button[BUT_EDIT_OFFSET_EDIT]->get_w() - delta);   
   _button[BUT_EDIT_OFFSET_CLEAR]->set_w(_button[BUT_EDIT_OFFSET_CLEAR]->get_w() - delta);   
   _button[BUT_EDIT_OFFSET_UNDO]->set_w(_button[BUT_EDIT_OFFSET_UNDO]->get_w() - delta);   
   _button[BUT_EDIT_OFFSET_APPLY]->set_w(_button[BUT_EDIT_OFFSET_APPLY]->get_w() - delta);   
   _button[BUT_EDIT_STYLE_APPLY]->set_w(_button[BUT_EDIT_STYLE_APPLY]->get_w() - delta);   
   _button[BUT_EDIT_STYLE_GET]->set_w(_button[BUT_EDIT_STYLE_GET]->get_w() - delta);   
   //_button[BUT_EDIT_STROKE_ADD]->set_w(_button[BUT_EDIT_STROKE_ADD]->get_w() - delta);   
   //_button[BUT_EDIT_STROKE_DEL]->set_w(_button[BUT_EDIT_STROKE_DEL]->get_w() - delta);   
   _button[BUT_EDIT_SYNTH_RUBBER]->set_w(_button[BUT_EDIT_SYNTH_RUBBER]->get_w() - delta);   
   _button[BUT_EDIT_SYNTH_SYNTHESIZE]->set_w(_button[BUT_EDIT_SYNTH_SYNTHESIZE]->get_w() - delta);   
   _button[BUT_EDIT_SYNTH_EX_ADD]->set_w(_button[BUT_EDIT_SYNTH_EX_ADD]->get_w() - delta);   
   _button[BUT_EDIT_SYNTH_EX_DEL]->set_w(_button[BUT_EDIT_SYNTH_EX_DEL]->get_w() - delta);   
   _button[BUT_EDIT_SYNTH_EX_CLEAR]->set_w(_button[BUT_EDIT_SYNTH_EX_CLEAR]->get_w() - delta);   
   _button[BUT_EDIT_SYNTH_ALL_CLEAR]->set_w(_button[BUT_EDIT_SYNTH_ALL_CLEAR]->get_w() - delta);   
   delta = (_panel[PANEL_EDIT_STATUS]->get_w() - _panel[PANEL_EDIT_CYCLE]->get_w());
   _statictext[TEXT_EDIT_STATUS_1]->set_w(_statictext[TEXT_EDIT_STATUS_1]->get_w() - delta);   
   _statictext[TEXT_EDIT_STATUS_2]->set_w(_statictext[TEXT_EDIT_STATUS_2]->get_w() - delta);   
   delta = (_panel[PANEL_EDIT_PRESSURE]->get_w() - _panel[PANEL_EDIT_STROKES]->get_w());
   _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->set_w(_checkbox[CHECK_EDIT_PRESSURE_WIDTH]->get_w() - delta);   
   _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->set_w(_checkbox[CHECK_EDIT_PRESSURE_ALPHA]->get_w() - delta);   
   delta = (_panel[PANEL_EDIT_OVERSKETCH]->get_w() - _panel[PANEL_EDIT_STROKES]->get_w());
   _radbutton[RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE]->set_w(_radbutton[RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE]->get_w() - delta);   
   _radbutton[RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE]->set_w(_radbutton[RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE]->get_w() - delta);   

}


/////////////////////////////////////
// destroy
/////////////////////////////////////

void
LinePenUI::destroy(GLUI*,GLUI_Panel *) 
{

   //Hands off these soon to be bad things

   _button.clear();
   _slider.clear();
   _panel.clear(); 
   _rollout.clear();
   _radgroup.clear();
   _radbutton.clear();
   _checkbox.clear();
   _statictext.clear();

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
LinePenUI::update_non_lives()
{

   //update_stroke();
   update_flags();
   update_coherence();
   update_mesh();
   update_noise();
   update_edit();
   
}

/////////////////////////////////////
// update_stroke()
/////////////////////////////////////
void
LinePenUI::update_stroke()
{
   COutlineStroke *s;

   s = _pen->retrieve_active_prototype();

   if (s)
   {
      bool ret;
      ret = StrokeUI::set_params(_pen->view(), this, s);  assert(ret);
      ret = StrokeUI::update(_pen->view(),this); assert(ret);

   }
}

/////////////////////////////////////
// update_flags()
/////////////////////////////////////
void
LinePenUI::update_flags()
{
//    static bool DOUG = Config::get_var_bool("DOUG",false,false);
   NPRTexture* curr_tex = _pen->curr_tex();

   if (curr_tex)
   {
      bool see_thru = curr_tex->get_see_thru();
         
      _checkbox[CHECK_FLAG_SEE_THRU]->enable();

      _checkbox[CHECK_FLAG_SILBF_VIS]->enable();
      _checkbox[CHECK_FLAG_BORDER_VIS]->enable();
      _checkbox[CHECK_FLAG_CREASE_VIS]->enable();
      _checkbox[CHECK_FLAG_SIL_VIS]->enable();

      if (see_thru)
      {
         _checkbox[CHECK_FLAG_SIL_HID]->enable();
         _checkbox[CHECK_FLAG_SIL_OCC]->enable();
         _checkbox[CHECK_FLAG_SILBF_HID]->enable();
         _checkbox[CHECK_FLAG_SILBF_OCC]->enable();
         _checkbox[CHECK_FLAG_BORDER_HID]->enable();
         _checkbox[CHECK_FLAG_BORDER_OCC]->enable();
         _checkbox[CHECK_FLAG_CREASE_HID]->enable();
         _checkbox[CHECK_FLAG_CREASE_OCC]->enable();
      }
      else
      {
         _checkbox[CHECK_FLAG_SIL_HID]->disable();
         _checkbox[CHECK_FLAG_SIL_OCC]->disable();
         _checkbox[CHECK_FLAG_SILBF_HID]->disable();
         _checkbox[CHECK_FLAG_SILBF_OCC]->disable();
         _checkbox[CHECK_FLAG_BORDER_HID]->disable();
         _checkbox[CHECK_FLAG_BORDER_OCC]->disable();
         _checkbox[CHECK_FLAG_CREASE_HID]->disable();
         _checkbox[CHECK_FLAG_CREASE_OCC]->disable();
      }

      _checkbox[CHECK_FLAG_SEE_THRU]->set_int_val(((see_thru)?(1):(0)));
      
      _checkbox[CHECK_FLAG_SIL_VIS]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_SIL_VISIBLE))?(1):(0)));
      _checkbox[CHECK_FLAG_SIL_HID]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_SIL_HIDDEN))?(1):(0)));
      _checkbox[CHECK_FLAG_SIL_OCC]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_SIL_OCCLUDED))?(1):(0)));
      _checkbox[CHECK_FLAG_SILBF_VIS]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_BF_SIL_VISIBLE))?(1):(0)));
      _checkbox[CHECK_FLAG_SILBF_HID]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_BF_SIL_HIDDEN))?(1):(0)));
      _checkbox[CHECK_FLAG_SILBF_OCC]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_BF_SIL_OCCLUDED))?(1):(0)));
      _checkbox[CHECK_FLAG_BORDER_VIS]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_BORDER_VISIBLE))?(1):(0)));
      _checkbox[CHECK_FLAG_BORDER_HID]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_BORDER_HIDDEN))?(1):(0)));
      _checkbox[CHECK_FLAG_BORDER_OCC]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_BORDER_OCCLUDED))?(1):(0)));
      _checkbox[CHECK_FLAG_CREASE_VIS]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_CREASE_VISIBLE))?(1):(0)));
      _checkbox[CHECK_FLAG_CREASE_HID]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_CREASE_HIDDEN))?(1):(0)));
      _checkbox[CHECK_FLAG_CREASE_OCC]->set_int_val(((curr_tex->get_see_thru_flag(ZXFLAG_CREASE_OCCLUDED))?(1):(0)));

   }
   else
   {
      _checkbox[CHECK_FLAG_SEE_THRU]->disable();
      _checkbox[CHECK_FLAG_SIL_VIS]->disable();
      _checkbox[CHECK_FLAG_SIL_HID]->disable();
      _checkbox[CHECK_FLAG_SIL_OCC]->disable();
      _checkbox[CHECK_FLAG_SILBF_VIS]->disable();
      _checkbox[CHECK_FLAG_SILBF_HID]->disable();
      _checkbox[CHECK_FLAG_SILBF_OCC]->disable();
      _checkbox[CHECK_FLAG_BORDER_VIS]->disable();
      _checkbox[CHECK_FLAG_BORDER_HID]->disable();
      _checkbox[CHECK_FLAG_BORDER_OCC]->disable();
      _checkbox[CHECK_FLAG_CREASE_VIS]->disable();
      _checkbox[CHECK_FLAG_CREASE_HID]->disable();
      _checkbox[CHECK_FLAG_CREASE_OCC]->disable();
   }

}

/////////////////////////////////////
// update_coherence()
/////////////////////////////////////
void
LinePenUI::update_coherence()
{

   BStrokePool*          curr_pool = _pen->curr_pool();
   LinePen::edit_mode_t  curr_mode = _pen->curr_mode();

   if (curr_mode == LinePen::EDIT_MODE_SIL)
   {
      assert(curr_pool && (curr_pool->class_name() == SilStrokePool::static_name()));
      
      SilStrokePool* sil_pool = (SilStrokePool*)curr_pool;
      
      bool coher_global = sil_pool->get_coher_global();
         
      _checkbox[CHECK_COHER_GLOBAL]->enable();

      if (!coher_global)
      {
         _checkbox[CHECK_COHER_SIG_1]->enable();
         _radgroup[RADGROUP_COHER_COVER]->enable();
         _radgroup[RADGROUP_COHER_FIT]->enable();
         _slider[SLIDE_COHER_PIX]->enable();
         _slider[SLIDE_COHER_WF]->enable();
         _slider[SLIDE_COHER_WS]->enable();
         _slider[SLIDE_COHER_WB]->enable();
         _slider[SLIDE_COHER_WH]->enable();
         _slider[SLIDE_COHER_MV]->enable();
         _slider[SLIDE_COHER_MP]->enable();
         _slider[SLIDE_COHER_M5]->enable();
         _slider[SLIDE_COHER_HJ]->enable();
         _slider[SLIDE_COHER_HT]->enable();
      }
      else
      {
         _checkbox[CHECK_COHER_SIG_1]->disable();
         _radgroup[RADGROUP_COHER_COVER]->disable();
         _radgroup[RADGROUP_COHER_FIT]->disable();
         _slider[SLIDE_COHER_PIX]->disable();
         _slider[SLIDE_COHER_WF]->disable();
         _slider[SLIDE_COHER_WS]->disable();
         _slider[SLIDE_COHER_WB]->disable();
         _slider[SLIDE_COHER_WH]->disable();
         _slider[SLIDE_COHER_MV]->disable();
         _slider[SLIDE_COHER_MP]->disable();
         _slider[SLIDE_COHER_M5]->disable();
         _slider[SLIDE_COHER_HJ]->disable();
         _slider[SLIDE_COHER_HT]->disable();
      }
 
      _checkbox[CHECK_COHER_GLOBAL]->set_int_val(((coher_global)?(1):(0)));

      _checkbox[CHECK_COHER_SIG_1]->set_int_val(((sil_pool->get_coher_sigma_one())?(1):(0)));

      int cover_type;
      switch(sil_pool->get_coher_cover_type())
      {
         case SilAndCreaseTexture::SIL_COVER_MAJORITY:   cover_type = RADBUT_COHER_COVER_MAJ;   break;
         case SilAndCreaseTexture::SIL_COVER_ONE_TO_ONE: cover_type = RADBUT_COHER_COVER_1_TO_1;break;
         case SilAndCreaseTexture::SIL_COVER_TRIMMED:    cover_type = RADBUT_COHER_COVER_TRIM;  break;
         default: assert(0); break;
      }
      _radgroup[RADGROUP_COHER_COVER]->set_int_val(cover_type - RADBUT_COHER_COVER_MAJ);

      int fit_type;
      switch(sil_pool->get_coher_fit_type())
      {
         case SilAndCreaseTexture::SIL_FIT_RANDOM:       fit_type = RADBUT_COHER_FIT_RAND;    break;
         case SilAndCreaseTexture::SIL_FIT_SIGMA:        fit_type = RADBUT_COHER_FIT_ARC;     break;
         case SilAndCreaseTexture::SIL_FIT_PHASE:        fit_type = RADBUT_COHER_FIT_PHASE;   break;
         case SilAndCreaseTexture::SIL_FIT_INTERPOLATE:  fit_type = RADBUT_COHER_FIT_INTERP;  break;
         case SilAndCreaseTexture::SIL_FIT_OPTIMIZE:     fit_type = RADBUT_COHER_FIT_OPTIM;   break;
         default: assert(0); break;
      }
      _radgroup[RADGROUP_COHER_FIT]->set_int_val(fit_type - RADBUT_COHER_FIT_RAND);

      _slider[SLIDE_COHER_PIX]->set_float_val(sil_pool->get_coher_pix());
      _slider[SLIDE_COHER_WF]->set_float_val(sil_pool->get_coher_wf());
      _slider[SLIDE_COHER_WS]->set_float_val(sil_pool->get_coher_ws());
      _slider[SLIDE_COHER_WB]->set_float_val(sil_pool->get_coher_wb());
      _slider[SLIDE_COHER_WH]->set_float_val(sil_pool->get_coher_wh());
      _slider[SLIDE_COHER_MV]->set_int_val(sil_pool->get_coher_mv());
      _slider[SLIDE_COHER_MP]->set_int_val(sil_pool->get_coher_mp());
      _slider[SLIDE_COHER_M5]->set_int_val(sil_pool->get_coher_m5());
      _slider[SLIDE_COHER_HJ]->set_int_val(sil_pool->get_coher_hj());
      _slider[SLIDE_COHER_HT]->set_int_val(sil_pool->get_coher_ht());
   }
   else
   {
      _checkbox[CHECK_COHER_GLOBAL]->disable();
      _checkbox[CHECK_COHER_SIG_1]->disable();
      _radgroup[RADGROUP_COHER_COVER]->disable();
      _radgroup[RADGROUP_COHER_FIT]->disable();
      _slider[SLIDE_COHER_PIX]->disable();
      _slider[SLIDE_COHER_WF]->disable();
      _slider[SLIDE_COHER_WS]->disable();
      _slider[SLIDE_COHER_WB]->disable();
      _slider[SLIDE_COHER_WH]->disable();
      _slider[SLIDE_COHER_MV]->disable();
      _slider[SLIDE_COHER_MP]->disable();
      _slider[SLIDE_COHER_M5]->disable();
      _slider[SLIDE_COHER_HJ]->disable();
      _slider[SLIDE_COHER_HT]->disable();
   }
}


/////////////////////////////////////
// update_mesh()
/////////////////////////////////////
void
LinePenUI::update_mesh()
{
   NPRTexture* curr_tex = _pen->curr_tex();

   if (curr_tex)
   {
      _button[BUT_MESH_RECREASE]->enable();
      _slider[SLIDE_MESH_POLY_FACTOR]->enable();
      _slider[SLIDE_MESH_POLY_UNITS]->enable();
      _slider[SLIDE_MESH_CREASE_VIS_STEP]->enable();
      _slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->enable();
      _slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->enable();
     
      _slider[SLIDE_MESH_POLY_FACTOR]->set_float_val(curr_tex->get_polygon_offset_factor()); 
      _slider[SLIDE_MESH_POLY_UNITS]->set_float_val(curr_tex->get_polygon_offset_units()); 
      _slider[SLIDE_MESH_CREASE_VIS_STEP]->set_float_val(curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_vis_step_size());
      _slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->set_float_val(curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_max_bend_angle());
      _slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->set_float_val(rad2deg(Acos(curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_thresh())));
   }
   else
   {
      _button[BUT_MESH_RECREASE]->disable();
      _slider[SLIDE_MESH_POLY_FACTOR]->disable();
      _slider[SLIDE_MESH_POLY_UNITS]->disable();
      _slider[SLIDE_MESH_CREASE_VIS_STEP]->disable();
      _slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->disable();
      _slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->disable();

   }

}

/////////////////////////////////////
// update_noise()
/////////////////////////////////////
void
LinePenUI::update_noise()
{
   NPRTexture*          curr_tex  = _pen->curr_tex();
   BStrokePool*         curr_pool = _pen->curr_pool();
   LinePen::edit_mode_t curr_mode = _pen->curr_mode();

   str_ptr text;

   if (curr_tex)
   {
      assert(curr_pool);
      if (curr_mode == LinePen::EDIT_MODE_SIL)
      {
         assert(curr_pool->class_name() == SilStrokePool::static_name());
         SilStrokePool* sil_pool = (SilStrokePool*)curr_pool;
      
         text = str_ptr(sil_pool->get_edit_proto()+1) + " of " + str_ptr(sil_pool->get_num_protos());

         if (sil_pool->get_num_protos()>1)
         {
            _button[BUT_NOISE_PROTOTYPE_NEXT]->enable();
            _button[BUT_NOISE_PROTOTYPE_DEL]->enable();
         }
         else
         {
            _button[BUT_NOISE_PROTOTYPE_NEXT]->disable();
            _button[BUT_NOISE_PROTOTYPE_DEL]->disable();
         }

         _button[BUT_NOISE_PROTOTYPE_ADD]->enable();
         _checkbox[CHECK_NOISE_PROTOTYPE_LOCK]->enable();

         _checkbox[CHECK_NOISE_PROTOTYPE_LOCK]->set_int_val(((sil_pool->get_lock_proto())?(1):(0)));
      }
      else
      {
         assert(curr_pool->class_name() != SilStrokePool::static_name());

         text = " N/A";

         _button[BUT_NOISE_PROTOTYPE_NEXT]->disable();
         _button[BUT_NOISE_PROTOTYPE_DEL]->disable();
         _button[BUT_NOISE_PROTOTYPE_ADD]->disable();
         _checkbox[CHECK_NOISE_PROTOTYPE_LOCK]->disable();
      }

      _checkbox[CHECK_NOISE_OBJECT_MOTION]->enable();
      _slider[SLIDE_NOISE_OBJECT_FREQUENCY]->enable();
      _slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->enable();
      _slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->enable();

      _checkbox[CHECK_NOISE_OBJECT_MOTION]->set_int_val(((curr_tex->stroke_tex()->sil_and_crease_tex()->get_noise_motion())?(1):(0)));
      _slider[SLIDE_NOISE_OBJECT_FREQUENCY]->set_float_val(curr_tex->stroke_tex()->sil_and_crease_tex()->get_noise_frequency());
      _slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->set_float_val(curr_tex->stroke_tex()->sil_and_crease_tex()->get_noise_order());
      _slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->set_float_val(curr_tex->stroke_tex()->sil_and_crease_tex()->get_noise_duration());
   }
   else
   {
      text = " N/A";

      _button[BUT_NOISE_PROTOTYPE_NEXT]->disable();
      _button[BUT_NOISE_PROTOTYPE_DEL]->disable();
      _button[BUT_NOISE_PROTOTYPE_ADD]->disable();
      _checkbox[CHECK_NOISE_PROTOTYPE_LOCK]->disable();

      _checkbox[CHECK_NOISE_OBJECT_MOTION]->disable();
      _slider[SLIDE_NOISE_OBJECT_FREQUENCY]->disable();
      _slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->disable();
      _slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->disable();
   }
   
   _statictext[TEXT_NOISE_PROTOTYPE]->set_text(**text);
}



/////////////////////////////////////
// update_edit()
/////////////////////////////////////

void
LinePenUI::update_edit()
{

   NPRTexture*          curr_tex    = _pen->curr_tex();
   BStrokePool*         curr_pool   = _pen->curr_pool();
   OutlineStroke*       curr_stroke = _pen->curr_stroke();
   LinePen::edit_mode_t curr_mode   = _pen->curr_mode();

   str_ptr text1, text2;

   if (curr_mode == LinePen::EDIT_MODE_SIL)
   {
      assert(curr_tex);
      assert(curr_pool && (curr_pool->class_name() == SilStrokePool::static_name()));
      SilStrokePool* sil_pool = (SilStrokePool*)curr_pool;

      OutlineStroke *p = sil_pool->get_prototype();

      _button[BUT_EDIT_CYCLE_LINE_TYPES]->enable();
      _button[BUT_EDIT_CYCLE_DECAL_GROUPS]->enable();
      if (curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_stroke_pools()->num()>0)
      {
         _button[BUT_EDIT_CYCLE_CREASE_PATHS]->enable();
      }
      else
      {
         _button[BUT_EDIT_CYCLE_CREASE_PATHS]->disable();
      }
      _button[BUT_EDIT_CYCLE_CREASE_STROKES]->disable();

      if (p->get_offsets())
      {
         _button[BUT_EDIT_OFFSET_EDIT]->enable();
         _button[BUT_EDIT_OFFSET_CLEAR]->enable();
      }
      else
      {
         _button[BUT_EDIT_OFFSET_EDIT]->disable();
         _button[BUT_EDIT_OFFSET_CLEAR]->disable();
      }
      if (!_pen->easel_is_empty())
      {
         _button[BUT_EDIT_OFFSET_UNDO]->enable();
         _button[BUT_EDIT_OFFSET_APPLY]->enable();
      }
      else
      {
         _button[BUT_EDIT_OFFSET_UNDO]->disable();
         _button[BUT_EDIT_OFFSET_APPLY]->disable();
      }
      _button[BUT_EDIT_STYLE_APPLY]->enable();
      _button[BUT_EDIT_STYLE_GET]->enable();
      _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->enable();
      _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->enable();
    
      _button[BUT_EDIT_STROKE_ADD]->disable();
      _button[BUT_EDIT_STROKE_DEL]->disable();
      _button[BUT_EDIT_SYNTH_RUBBER]->disable();
      _button[BUT_EDIT_SYNTH_SYNTHESIZE]->disable();
      _button[BUT_EDIT_SYNTH_EX_ADD]->disable();
      _button[BUT_EDIT_SYNTH_EX_DEL]->disable();
      _button[BUT_EDIT_SYNTH_EX_CLEAR]->disable();
      _button[BUT_EDIT_SYNTH_ALL_CLEAR]->disable();

      if (curr_stroke)
      {
         _radgroup[RADGROUP_EDIT_OVERSKETCH]->enable();         
      }
      else
      {
         _radgroup[RADGROUP_EDIT_OVERSKETCH]->disable();         
      }

      
      SilAndCreaseTexture::sil_stroke_pool_t sil_type;

      for (int i=0; i<SilAndCreaseTexture::SIL_STROKE_POOL_NUM; i++) 
      {
          if (curr_pool == curr_tex->stroke_tex()->sil_and_crease_tex()->
                              get_sil_stroke_pool((SilAndCreaseTexture::sil_stroke_pool_t)i))
          {
            sil_type = (SilAndCreaseTexture::sil_stroke_pool_t)i;
          }
      }

      text1 = str_ptr("Edit: Line ") + SilAndCreaseTexture::sil_stroke_pool(sil_type);
      text2 = str_ptr("Proto: ") + str_ptr(sil_pool->get_edit_proto()+1) + " of " + str_ptr(sil_pool->get_num_protos());

      _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->set_int_val((p->get_press_vary_width()?1:0));
      _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->set_int_val((p->get_press_vary_alpha()?1:0));
   }
   else if (curr_mode == LinePen::EDIT_MODE_CREASE)
   {
      assert(curr_tex);
      assert(curr_pool && (curr_pool->class_name() == EdgeStrokePool::static_name()));
      EdgeStrokePool* edge_pool = (EdgeStrokePool*)curr_pool;

      _button[BUT_EDIT_CYCLE_LINE_TYPES]->enable();
      _button[BUT_EDIT_CYCLE_DECAL_GROUPS]->enable();
      assert(curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_stroke_pools()->num()>0);
      _button[BUT_EDIT_CYCLE_CREASE_PATHS]->enable();
      
      if (edge_pool->num_strokes()>0)
      {
         _button[BUT_EDIT_CYCLE_CREASE_STROKES]->enable();
      }
      else
      {
         _button[BUT_EDIT_CYCLE_CREASE_STROKES]->disable();
      }

      _button[BUT_EDIT_STROKE_ADD]->enable();
      
      _button[BUT_EDIT_SYNTH_RUBBER]->disable();
      _button[BUT_EDIT_SYNTH_SYNTHESIZE]->disable();
      _button[BUT_EDIT_SYNTH_EX_ADD]->disable();
      _button[BUT_EDIT_SYNTH_EX_DEL]->disable();
      _button[BUT_EDIT_SYNTH_EX_CLEAR]->disable();
      _button[BUT_EDIT_SYNTH_ALL_CLEAR]->disable();

      ARRAY<EdgeStrokePool*>* pools = curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_stroke_pools(); assert(pools);
      int i = pools->get_index(edge_pool); assert(i != BAD_IND);

      text1 = str_ptr("Edit: Crease ") + str_ptr(i+1) + " of " + str_ptr(pools->num());

      if (curr_stroke)
      {
         if (curr_stroke->get_offsets())
         {
            _button[BUT_EDIT_OFFSET_EDIT]->enable();
            _button[BUT_EDIT_OFFSET_CLEAR]->enable();
         }
         else
         {
            _button[BUT_EDIT_OFFSET_EDIT]->disable();
            _button[BUT_EDIT_OFFSET_CLEAR]->disable();
         }
         if (!_pen->easel_is_empty())
         {
            _button[BUT_EDIT_OFFSET_UNDO]->enable();
            _button[BUT_EDIT_OFFSET_APPLY]->enable();
         }
         else
         {
            _button[BUT_EDIT_OFFSET_UNDO]->disable();
            _button[BUT_EDIT_OFFSET_APPLY]->disable();
         }

         _button[BUT_EDIT_STYLE_APPLY]->enable();
         _button[BUT_EDIT_STYLE_GET]->enable();

         _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->enable();
         _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->enable();

         _radgroup[RADGROUP_EDIT_OVERSKETCH]->enable();         
         
         _button[BUT_EDIT_STROKE_DEL]->enable();


         text2 = str_ptr("Stroke: ") + str_ptr(1+curr_pool->get_selected_stroke_index()) 
                                                      + " of " + str_ptr(curr_pool->num_strokes());

         _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->set_int_val((curr_stroke->get_press_vary_width()?1:0));
         _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->set_int_val((curr_stroke->get_press_vary_alpha()?1:0));

      }
      else
      {
         _button[BUT_EDIT_OFFSET_EDIT]->disable();
         _button[BUT_EDIT_OFFSET_CLEAR]->disable();
            
         _button[BUT_EDIT_OFFSET_APPLY]->enable();

         assert(_pen->easel_is_empty());
         _button[BUT_EDIT_OFFSET_UNDO]->disable();
         
         _button[BUT_EDIT_STYLE_APPLY]->disable();
         _button[BUT_EDIT_STYLE_GET]->disable();

         _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->disable();
         _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->disable();

         _radgroup[RADGROUP_EDIT_OVERSKETCH]->disable();         
         
         _button[BUT_EDIT_STROKE_DEL]->disable();

         text2 = str_ptr("Stroke: ") + "None" + " of " + str_ptr(curr_pool->num_strokes());      
      }
   }
   else if (curr_mode == LinePen::EDIT_MODE_DECAL)
   {
      assert(curr_tex);
      assert(curr_pool && (curr_pool->class_name() == DecalStrokePool::static_name()));
//      DecalStrokePool* decal_pool = (DecalStrokePool*)curr_pool;

      _button[BUT_EDIT_CYCLE_LINE_TYPES]->enable();
      _button[BUT_EDIT_CYCLE_DECAL_GROUPS]->enable();
      if (curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_stroke_pools()->num()>0)
      {
         _button[BUT_EDIT_CYCLE_CREASE_PATHS]->enable();
      }
      else
      {
         _button[BUT_EDIT_CYCLE_CREASE_PATHS]->disable();
      }

      _button[BUT_EDIT_CYCLE_CREASE_STROKES]->disable();

      _button[BUT_EDIT_OFFSET_EDIT]->disable();

      _radgroup[RADGROUP_EDIT_OVERSKETCH]->disable();         

      _button[BUT_EDIT_STROKE_ADD]->disable();
      _button[BUT_EDIT_STROKE_DEL]->disable();

      _button[BUT_EDIT_SYNTH_RUBBER]->disable();
      _button[BUT_EDIT_SYNTH_SYNTHESIZE]->disable();
      _button[BUT_EDIT_SYNTH_EX_ADD]->disable();
      _button[BUT_EDIT_SYNTH_EX_DEL]->disable();
      _button[BUT_EDIT_SYNTH_EX_CLEAR]->disable();
      _button[BUT_EDIT_SYNTH_ALL_CLEAR]->disable();

      text1 = str_ptr("Edit: Decal");
      
      if (curr_stroke)
      {
         _button[BUT_EDIT_OFFSET_CLEAR]->enable();
         assert(_pen->easel_is_empty());
         _button[BUT_EDIT_OFFSET_UNDO]->disable();
         _button[BUT_EDIT_OFFSET_APPLY]->disable();

         _button[BUT_EDIT_STYLE_APPLY]->enable();
         _button[BUT_EDIT_STYLE_GET]->enable();

         _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->enable();
         _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->enable();

         text2 = str_ptr("Mark: ") + str_ptr(1+curr_pool->get_selected_stroke_index()) 
                                                      + " of " + str_ptr(curr_pool->num_strokes());

         _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->set_int_val((curr_stroke->get_press_vary_width()?1:0));
         _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->set_int_val((curr_stroke->get_press_vary_alpha()?1:0));
      }
      else
      {
         _button[BUT_EDIT_OFFSET_CLEAR]->disable();

         if (!_pen->easel_is_empty()) 
         {
            _button[BUT_EDIT_OFFSET_UNDO]->enable();
            _button[BUT_EDIT_OFFSET_APPLY]->enable();
         }
         else
         {
            _button[BUT_EDIT_OFFSET_UNDO]->disable();
            _button[BUT_EDIT_OFFSET_APPLY]->disable();
         }


         _button[BUT_EDIT_STYLE_APPLY]->disable();
         _button[BUT_EDIT_STYLE_GET]->disable();

         _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->disable();
         _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->disable();

         text2 = str_ptr("Mark: ") + "None" + " of " + str_ptr(curr_pool->num_strokes());            
      }
   }
   else
   {
      assert(!curr_tex);
      assert(!curr_pool);
      assert(!curr_stroke);
      assert(curr_mode == LinePen::EDIT_MODE_NONE);

      _button[BUT_EDIT_CYCLE_LINE_TYPES]->disable();
      _button[BUT_EDIT_CYCLE_DECAL_GROUPS]->disable();
      _button[BUT_EDIT_CYCLE_CREASE_PATHS]->disable();
      _button[BUT_EDIT_CYCLE_CREASE_STROKES]->disable();

      _button[BUT_EDIT_OFFSET_EDIT]->disable();
      _button[BUT_EDIT_OFFSET_UNDO]->disable();
      assert(_pen->easel_is_empty());
      _button[BUT_EDIT_OFFSET_CLEAR]->disable();
      _button[BUT_EDIT_OFFSET_APPLY]->disable();

      _radgroup[RADGROUP_EDIT_OVERSKETCH]->disable();         

      _button[BUT_EDIT_STROKE_ADD]->disable();
      _button[BUT_EDIT_STROKE_DEL]->disable();

      _button[BUT_EDIT_SYNTH_RUBBER]->disable();
      _button[BUT_EDIT_SYNTH_SYNTHESIZE]->disable();
      _button[BUT_EDIT_SYNTH_EX_ADD]->disable();
      _button[BUT_EDIT_SYNTH_EX_DEL]->disable();
      _button[BUT_EDIT_SYNTH_EX_CLEAR]->disable();
      _button[BUT_EDIT_SYNTH_ALL_CLEAR]->disable();

      _button[BUT_EDIT_STYLE_APPLY]->disable();
      _button[BUT_EDIT_STYLE_GET]->disable();

      _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->disable();
      _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->disable();

      text1 = "Edit: Nothing!";
      text2 = "";
   }

   _statictext[TEXT_EDIT_STATUS_1]->set_text(**text1);
   _statictext[TEXT_EDIT_STATUS_2]->set_text(**text2);

   _radgroup[RADGROUP_EDIT_OVERSKETCH]->set_int_val(
      (_pen->get_virtual_baseline() ? RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE : 
                                      RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE)
                                             - RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE);

}

/////////////////////////////////////
// apply_stroke()
/////////////////////////////////////
void
LinePenUI::apply_stroke()
{
   CBaseStroke *bsp = StrokeUI::get_params(_pen->view(),this);    assert(bsp); 
   
   OutlineStroke os;

   os.set_propagate_mesh_size(false);   
   os.set_propagate_offsets(false);

   COutlineStroke *osp = _pen->retrieve_active_prototype();

   if (osp)
   {
      //Dup the stroke OutlineStroke we're about to modify...
      os.copy(*osp);

      //Apply the BaseStroke attributes...
      os.copy(*bsp);

      //And use the result to modify the original...
      _pen->modify_active_prototype(&os);
   }

   _pen->update_gesture_drawer();

}

/////////////////////////////////////
// apply_flags()
/////////////////////////////////////
void
LinePenUI::apply_flags()
{
   
   NPRTexture* curr_tex = _pen->curr_tex();

   assert(curr_tex);

   curr_tex->set_see_thru(_checkbox[CHECK_FLAG_SEE_THRU]->get_int_val()==1);

   curr_tex->set_see_thru_flag(ZXFLAG_SIL_VISIBLE, _checkbox[CHECK_FLAG_SIL_VIS]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_SIL_HIDDEN,  _checkbox[CHECK_FLAG_SIL_HID]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_SIL_OCCLUDED,_checkbox[CHECK_FLAG_SIL_OCC]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_BF_SIL_VISIBLE, _checkbox[CHECK_FLAG_SILBF_VIS]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_BF_SIL_HIDDEN,  _checkbox[CHECK_FLAG_SILBF_HID]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_BF_SIL_OCCLUDED,_checkbox[CHECK_FLAG_SILBF_OCC]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_BORDER_VISIBLE, _checkbox[CHECK_FLAG_BORDER_VIS]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_BORDER_HIDDEN,  _checkbox[CHECK_FLAG_BORDER_HID]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_BORDER_OCCLUDED,_checkbox[CHECK_FLAG_BORDER_OCC]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_CREASE_VISIBLE, _checkbox[CHECK_FLAG_CREASE_VIS]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_CREASE_HIDDEN,  _checkbox[CHECK_FLAG_CREASE_HID]->get_int_val()==1);
   curr_tex->set_see_thru_flag(ZXFLAG_CREASE_OCCLUDED,_checkbox[CHECK_FLAG_CREASE_OCC]->get_int_val()==1);

   curr_tex->stroke_tex()->sil_and_crease_tex()->mark_sils_dirty();

}

/////////////////////////////////////
// apply_coherence()
/////////////////////////////////////
void
LinePenUI::apply_coherence()
{
   NPRTexture*  curr_tex  = _pen->curr_tex();
   BStrokePool* curr_pool = _pen->curr_pool();
   LinePen::edit_mode_t  curr_mode = _pen->curr_mode();

   assert(curr_mode == LinePen::EDIT_MODE_SIL);
   assert(curr_tex);
   assert(curr_pool && (curr_pool->class_name() == SilStrokePool::static_name()));

   SilStrokePool* sil_pool = (SilStrokePool*)curr_pool;

   sil_pool->set_coher_global(_checkbox[CHECK_COHER_GLOBAL]->get_int_val()==1);
   sil_pool->set_coher_sigma_one(_checkbox[CHECK_COHER_SIG_1]->get_int_val()==1);

   int cover_type;
   switch(_radgroup[RADGROUP_COHER_COVER]->get_int_val() + RADBUT_COHER_COVER_MAJ)
   {
      case RADBUT_COHER_COVER_MAJ:     cover_type = SilAndCreaseTexture::SIL_COVER_MAJORITY;    break;
      case RADBUT_COHER_COVER_1_TO_1:  cover_type = SilAndCreaseTexture::SIL_COVER_ONE_TO_ONE;  break;
      case RADBUT_COHER_COVER_TRIM:    cover_type = SilAndCreaseTexture::SIL_COVER_TRIMMED;     break;
      default: assert(0); break;
   }
   sil_pool->set_coher_cover_type(cover_type);

   int fit_type;
   switch(_radgroup[RADGROUP_COHER_FIT]->get_int_val() + RADBUT_COHER_FIT_RAND)
   {
      case RADBUT_COHER_FIT_RAND:   fit_type = SilAndCreaseTexture::SIL_FIT_RANDOM;       break;
      case RADBUT_COHER_FIT_ARC:    fit_type = SilAndCreaseTexture::SIL_FIT_SIGMA;        break;
      case RADBUT_COHER_FIT_PHASE:  fit_type = SilAndCreaseTexture::SIL_FIT_PHASE;        break;
      case RADBUT_COHER_FIT_INTERP: fit_type = SilAndCreaseTexture::SIL_FIT_INTERPOLATE;  break;
      case RADBUT_COHER_FIT_OPTIM:  fit_type = SilAndCreaseTexture::SIL_FIT_OPTIMIZE;     break;
      default: assert(0); break;
   }
   sil_pool->set_coher_fit_type(fit_type);


   sil_pool->set_coher_pix(_slider[SLIDE_COHER_PIX]->get_float_val());
   sil_pool->set_coher_wf(_slider[SLIDE_COHER_WF]->get_float_val());
   sil_pool->set_coher_ws(_slider[SLIDE_COHER_WS]->get_float_val());
   sil_pool->set_coher_wb(_slider[SLIDE_COHER_WB]->get_float_val());
   sil_pool->set_coher_wh(_slider[SLIDE_COHER_WH]->get_float_val());
   sil_pool->set_coher_mv(_slider[SLIDE_COHER_MV]->get_int_val());
   sil_pool->set_coher_mp(_slider[SLIDE_COHER_MP]->get_int_val());
   sil_pool->set_coher_m5(_slider[SLIDE_COHER_M5]->get_int_val());
   sil_pool->set_coher_hj(_slider[SLIDE_COHER_HJ]->get_int_val());
   sil_pool->set_coher_ht(_slider[SLIDE_COHER_HT]->get_int_val());

   curr_tex->stroke_tex()->sil_and_crease_tex()->mark_sils_dirty();

}


/////////////////////////////////////
// apply_mesh()
/////////////////////////////////////
void
LinePenUI::apply_mesh()
{
   NPRTexture* curr_tex = _pen->curr_tex();

   assert(curr_tex);

   curr_tex->set_polygon_offset_factor(_slider[SLIDE_MESH_POLY_FACTOR]->get_float_val()); 
   curr_tex->set_polygon_offset_units(_slider[SLIDE_MESH_POLY_UNITS]->get_float_val()); 

   curr_tex->stroke_tex()->sil_and_crease_tex()->set_crease_vis_step_size(_slider[SLIDE_MESH_CREASE_VIS_STEP]->get_float_val());
   curr_tex->stroke_tex()->sil_and_crease_tex()->set_crease_max_bend_angle(_slider[SLIDE_MESH_CREASE_JOINT_ANGLE]->get_float_val());
   curr_tex->stroke_tex()->sil_and_crease_tex()->set_crease_thresh(cos(deg2rad(_slider[SLIDE_MESH_CREASE_DETECT_ANGLE]->get_float_val())));

   curr_tex->stroke_tex()->sil_and_crease_tex()->mark_sils_dirty();
}

/////////////////////////////////////
// apply_noise()
/////////////////////////////////////
void
LinePenUI::apply_noise()
{

   NPRTexture*  curr_tex  = _pen->curr_tex();
   BStrokePool* curr_pool = _pen->curr_pool();
   LinePen::edit_mode_t  curr_mode = _pen->curr_mode();
   
   assert(curr_tex);
   assert(curr_pool);

   if (curr_mode == LinePen::EDIT_MODE_SIL)
   {
      assert(curr_pool->class_name() == SilStrokePool::static_name());
      SilStrokePool* sil_pool = (SilStrokePool*)curr_pool;
      sil_pool->set_lock_proto(_checkbox[CHECK_NOISE_PROTOTYPE_LOCK]->get_int_val() == 1);
   }
   else
   {
      assert(curr_pool->class_name() != SilStrokePool::static_name());
   }

   curr_tex->stroke_tex()->sil_and_crease_tex()->set_noise_motion(_checkbox[CHECK_NOISE_OBJECT_MOTION]->get_int_val()==1);
   curr_tex->stroke_tex()->sil_and_crease_tex()->set_noise_frequency(_slider[SLIDE_NOISE_OBJECT_FREQUENCY]->get_float_val());
   curr_tex->stroke_tex()->sil_and_crease_tex()->set_noise_order(_slider[SLIDE_NOISE_OBJECT_RANDOM_ORDER]->get_float_val());
   curr_tex->stroke_tex()->sil_and_crease_tex()->set_noise_duration(_slider[SLIDE_NOISE_OBJECT_RANDOM_DURATION]->get_float_val());

   //XXX - Not req'd
   //curr_tex->stroke_tex()->sil_and_crease_tex()->mark_sils_dirty();

}

/////////////////////////////////////
// apply_edit()
/////////////////////////////////////
void
LinePenUI::apply_edit()
{

   NPRTexture*          curr_tex    = _pen->curr_tex();
   BStrokePool*         curr_pool   = _pen->curr_pool();
   OutlineStroke*       curr_stroke = _pen->curr_stroke();
   LinePen::edit_mode_t curr_mode   = _pen->curr_mode();
   
   assert(curr_tex);
   assert(curr_pool);

   bool press_width = _checkbox[CHECK_EDIT_PRESSURE_WIDTH]->get_int_val()==1;
   bool press_alpha = _checkbox[CHECK_EDIT_PRESSURE_ALPHA]->get_int_val()==1;

   if (curr_mode == LinePen::EDIT_MODE_SIL)
   {
      assert(curr_pool->class_name() == SilStrokePool::static_name());
      SilStrokePool* sil_pool = (SilStrokePool*)curr_pool;

      OutlineStroke *p = sil_pool->get_prototype();

      if ((press_width != p->get_press_vary_width()) ||
          (press_alpha != p->get_press_vary_alpha())   )
      {
         p->set_press_vary_width(press_width);
         p->set_press_vary_alpha(press_alpha);
         sil_pool->set_prototype(p);
      }
   }
   else if (curr_mode == LinePen::EDIT_MODE_CREASE)
   {
      assert(curr_pool->class_name() == EdgeStrokePool::static_name());
//      EdgeStrokePool* edge_pool = (EdgeStrokePool*)curr_pool;

      assert(curr_stroke);
      
      if ((press_width != curr_stroke->get_press_vary_width()) ||
          (press_alpha != curr_stroke->get_press_vary_alpha())   )
      {
         curr_stroke->set_press_vary_width(press_width);
         curr_stroke->set_press_vary_alpha(press_alpha);
      }
   }
   else if (curr_mode == LinePen::EDIT_MODE_DECAL)
   {
      assert(curr_pool->class_name() == DecalStrokePool::static_name());
//      DecalStrokePool* decal_pool = (DecalStrokePool*)curr_pool;

      assert(curr_stroke);
      
      if ((press_width != curr_stroke->get_press_vary_width()) ||
          (press_alpha != curr_stroke->get_press_vary_alpha())   )
      {
         curr_stroke->set_press_vary_width(press_width);
         curr_stroke->set_press_vary_alpha(press_alpha);
      }
   }
   else
   {
      assert(0);
   }

   //Also write thru pressure flags to the UI's stroke
   //and the undo stroke, so that these flags always
   //stay consistent with the check boxes. (i.e. we
   //don't want these flags to be affected when the
   //StrokeUI proto is applied, or when we undo.)
   //These flags only respond to the checkboxes!

   bool ret;
   CBaseStroke *bsp;
   BaseStroke  bs;

   bsp = StrokeUI::get_params(_pen->view(), this);        assert(bsp);
   bs.copy(*bsp);
   bs.set_press_vary_width(press_width);
   bs.set_press_vary_alpha(press_alpha);
   ret = StrokeUI::set_params(_pen->view(), this, &bs);   assert(ret);

   OutlineStroke *osp = _pen->undo_stroke();

   osp->set_press_vary_width(press_width);
   osp->set_press_vary_alpha(press_alpha);

   switch(_radgroup[RADGROUP_EDIT_OVERSKETCH]->get_int_val() + RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE )
   {
      case RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE:
         _pen->set_virtual_baseline(true);
      break;
      case RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE:
         _pen->set_virtual_baseline(false);
      break;
   }
   if (!_pen->easel_is_empty())
      _pen->easel_update_baseline();

}



/////////////////////////////////////
// button_cb()
/////////////////////////////////////

void
LinePenUI::button_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case BUT_MESH_RECREASE:
         _ui[id >> ID_SHIFT]->_pen->button_mesh_recrease();
      break;
      case BUT_NOISE_PROTOTYPE_NEXT:
         _ui[id >> ID_SHIFT]->_pen->button_noise_prototype_next();
      break;
      case BUT_NOISE_PROTOTYPE_DEL:
         _ui[id >> ID_SHIFT]->_pen->button_noise_prototype_del();
      break;
      case BUT_NOISE_PROTOTYPE_ADD:
         _ui[id >> ID_SHIFT]->_pen->button_noise_prototype_add();
      break;
      case BUT_EDIT_CYCLE_LINE_TYPES:
         _ui[id >> ID_SHIFT]->_pen->button_edit_cycle_line_types();
      break;
      case BUT_EDIT_CYCLE_DECAL_GROUPS:
         _ui[id >> ID_SHIFT]->_pen->button_edit_cycle_decal_groups();
      break;
      case BUT_EDIT_CYCLE_CREASE_PATHS:
         _ui[id >> ID_SHIFT]->_pen->button_edit_cycle_crease_paths();
      break;
      case BUT_EDIT_CYCLE_CREASE_STROKES:
         _ui[id >> ID_SHIFT]->_pen->button_edit_cycle_crease_strokes();
      break;
      case BUT_EDIT_OFFSET_EDIT:
         _ui[id >> ID_SHIFT]->_pen->button_edit_offset_edit();
      break;
      case BUT_EDIT_OFFSET_CLEAR:
         _ui[id >> ID_SHIFT]->_pen->button_edit_offset_clear();
      break;
      case BUT_EDIT_OFFSET_UNDO:
         _ui[id >> ID_SHIFT]->_pen->button_edit_offset_undo();
      break;
      case BUT_EDIT_OFFSET_APPLY:
         _ui[id >> ID_SHIFT]->_pen->button_edit_offset_apply();
      break;
      case BUT_EDIT_STYLE_APPLY:
         _ui[id >> ID_SHIFT]->_pen->button_edit_style_apply();
      break;
      case BUT_EDIT_STYLE_GET:
         _ui[id >> ID_SHIFT]->_pen->button_edit_style_get();
      break;
      case BUT_EDIT_STROKE_ADD:
         _ui[id >> ID_SHIFT]->_pen->button_edit_stroke_add();
      break;
      case BUT_EDIT_STROKE_DEL:
         _ui[id >> ID_SHIFT]->_pen->button_edit_stroke_del();
      break;
      case BUT_EDIT_SYNTH_RUBBER:
         _ui[id >> ID_SHIFT]->_pen->button_edit_synth_rubber();
      break;
      case BUT_EDIT_SYNTH_SYNTHESIZE:
         _ui[id >> ID_SHIFT]->_pen->button_edit_synth_synthesize();
      break;
      case BUT_EDIT_SYNTH_EX_ADD:
         _ui[id >> ID_SHIFT]->_pen->button_edit_synth_ex_add();
      break;
      case BUT_EDIT_SYNTH_EX_DEL:
         _ui[id >> ID_SHIFT]->_pen->button_edit_synth_ex_del();
      break;
      case BUT_EDIT_SYNTH_EX_CLEAR:
         _ui[id >> ID_SHIFT]->_pen->button_edit_synth_ex_clear();
      break;
      case BUT_EDIT_SYNTH_ALL_CLEAR:
         _ui[id >> ID_SHIFT]->_pen->button_edit_synth_all_clear();
      break;
   }
}

/////////////////////////////////////
// slider_cb()
/////////////////////////////////////

void
LinePenUI::slider_cb(int id)
{

   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case SLIDE_COHER_PIX:
      case SLIDE_COHER_WF:
      case SLIDE_COHER_WS:
      case SLIDE_COHER_WB:
      case SLIDE_COHER_WH:
      case SLIDE_COHER_MV:
      case SLIDE_COHER_MP:
      case SLIDE_COHER_M5:
      case SLIDE_COHER_HT:
      case SLIDE_COHER_HJ:
         _ui[id >> ID_SHIFT]->apply_coherence();
      break;
      case SLIDE_MESH_POLY_UNITS:
      case SLIDE_MESH_POLY_FACTOR:
      case SLIDE_MESH_CREASE_DETECT_ANGLE:
      case SLIDE_MESH_CREASE_JOINT_ANGLE:
      case SLIDE_MESH_CREASE_VIS_STEP:
         _ui[id >> ID_SHIFT]->apply_mesh();
      break;
      case SLIDE_NOISE_OBJECT_FREQUENCY:
      case SLIDE_NOISE_OBJECT_RANDOM_ORDER:
      case SLIDE_NOISE_OBJECT_RANDOM_DURATION:
         _ui[id >> ID_SHIFT]->apply_noise();
      break;
   }
}

/////////////////////////////////////
// radiogroup_cb()
/////////////////////////////////////

void
LinePenUI::radiogroup_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());
   switch(id&ID_MASK)
   {
      case RADGROUP_COHER_FIT:
      case RADGROUP_COHER_COVER:
         _ui[id >> ID_SHIFT]->apply_coherence();
      break;
      case RADGROUP_EDIT_OVERSKETCH:
         _ui[id >> ID_SHIFT]->apply_edit();
      break;
   }
}

/////////////////////////////////////
// checkbox_cb()
/////////////////////////////////////

void
LinePenUI::checkbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case CHECK_FLAG_SIL_VIS:
      case CHECK_FLAG_SIL_HID:
      case CHECK_FLAG_SIL_OCC:
      case CHECK_FLAG_SILBF_VIS:
      case CHECK_FLAG_SILBF_HID:
      case CHECK_FLAG_SILBF_OCC:
      case CHECK_FLAG_BORDER_VIS:
      case CHECK_FLAG_BORDER_HID:
      case CHECK_FLAG_BORDER_OCC:
      case CHECK_FLAG_CREASE_VIS:
      case CHECK_FLAG_CREASE_HID:
      case CHECK_FLAG_CREASE_OCC:
         _ui[id >> ID_SHIFT]->apply_flags();
      break;
      case CHECK_FLAG_SEE_THRU:
         _ui[id >> ID_SHIFT]->apply_flags();
         _ui[id >> ID_SHIFT]->update_flags();
      break;
      case CHECK_COHER_SIG_1:
         _ui[id >> ID_SHIFT]->apply_coherence();
      break;
      case CHECK_COHER_GLOBAL:
         _ui[id >> ID_SHIFT]->apply_coherence();
         _ui[id >> ID_SHIFT]->update_coherence();
      break;
      case CHECK_NOISE_PROTOTYPE_LOCK:
      case CHECK_NOISE_OBJECT_MOTION:
         _ui[id >> ID_SHIFT]->apply_noise();
      break;
      case CHECK_EDIT_PRESSURE_WIDTH:
      case CHECK_EDIT_PRESSURE_ALPHA:
         _ui[id >> ID_SHIFT]->apply_edit();
      break;
   }
}
