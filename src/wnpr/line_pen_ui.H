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
#ifndef _LINE_PEN_UI_H_IS_INCLUDED_
#define _LINE_PEN_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// ViewUI
////////////////////////////////////////////

#include "disp/view.H"

class LinePen;

class GLUI;
class GLUI_Listbox;
class GLUI_Button;
class GLUI_Slider;
class GLUI_Panel;
class GLUI_Rollout;
class GLUI_Rotation;
class GLUI_Translation;
class GLUI_RadioGroup;
class GLUI_RadioButton;
class GLUI_Checkbox;
class GLUI_EditText;
class GLUI_StaticText;

/*****************************************************************
 * NPRPenUI
 *****************************************************************/
class LinePenUI : public StrokeUIClient {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_MESH_RECREASE=0,
      BUT_NOISE_PROTOTYPE_NEXT,
      BUT_NOISE_PROTOTYPE_DEL,
      BUT_NOISE_PROTOTYPE_ADD,
      BUT_EDIT_CYCLE_LINE_TYPES,
      BUT_EDIT_CYCLE_DECAL_GROUPS,
      BUT_EDIT_CYCLE_CREASE_PATHS,
      BUT_EDIT_CYCLE_CREASE_STROKES,
      BUT_EDIT_OFFSET_EDIT,
      BUT_EDIT_OFFSET_CLEAR,
      BUT_EDIT_OFFSET_UNDO,
      BUT_EDIT_OFFSET_APPLY,
      BUT_EDIT_STYLE_APPLY,
      BUT_EDIT_STYLE_GET,
      BUT_EDIT_STROKE_ADD,
      BUT_EDIT_STROKE_DEL,
      BUT_EDIT_SYNTH_RUBBER,
      BUT_EDIT_SYNTH_SYNTHESIZE,
      BUT_EDIT_SYNTH_EX_ADD,
      BUT_EDIT_SYNTH_EX_DEL,
      BUT_EDIT_SYNTH_EX_CLEAR,
      BUT_EDIT_SYNTH_ALL_CLEAR,
      BUT_NUM
   };

   enum slider_id_t {
      SLIDE_COHER_PIX=0,
      SLIDE_COHER_WF,
      SLIDE_COHER_WS,
      SLIDE_COHER_WB,
      SLIDE_COHER_WH,
      SLIDE_COHER_MV,
      SLIDE_COHER_MP,
      SLIDE_COHER_M5,
      SLIDE_COHER_HT,
      SLIDE_COHER_HJ,
      SLIDE_MESH_POLY_UNITS,
      SLIDE_MESH_POLY_FACTOR,
      SLIDE_MESH_CREASE_DETECT_ANGLE,
      SLIDE_MESH_CREASE_JOINT_ANGLE,
      SLIDE_MESH_CREASE_VIS_STEP,
      SLIDE_NOISE_OBJECT_FREQUENCY,
      SLIDE_NOISE_OBJECT_RANDOM_ORDER,
      SLIDE_NOISE_OBJECT_RANDOM_DURATION,
      SLIDE_NUM
   };

   enum panel_id_t {
      PANEL_COHER_OPTS=0,
      PANEL_COHER_FIT,
      PANEL_NOISE_PROTOTYPE,
      PANEL_NOISE_PROTOTYPE_TEXT,
      PANEL_NOISE_PROTOTYPE_CONTROLS,
      PANEL_NOISE_OBJECT,
      PANEL_EDIT_STATUS,
      PANEL_EDIT_CYCLE,
      PANEL_EDIT_OFFSETS,
      PANEL_EDIT_STYLE,
      PANEL_EDIT_PRESSURE,
      PANEL_EDIT_OVERSKETCH,
      PANEL_EDIT_STROKES,
      PANEL_EDIT_SYNTHESIS,
      PANEL_NUM
   };

   enum rollout_id_t {
      ROLLOUT_FLAGS = 0,
      ROLLOUT_COHER,
      ROLLOUT_MESH,
      ROLLOUT_NOISE,
      ROLLOUT_EDIT,
      ROLLOUT_NUM
   };

   enum radiogroup_id_t {
      RADGROUP_COHER_COVER = 0,
      RADGROUP_COHER_FIT,
      RADGROUP_EDIT_OVERSKETCH,
      RADGROUP_NUM
   };

   enum radiobutton_id_t {
      RADBUT_COHER_COVER_MAJ = 0,
      RADBUT_COHER_COVER_1_TO_1,
      RADBUT_COHER_COVER_TRIM,
      RADBUT_COHER_FIT_RAND,
      RADBUT_COHER_FIT_ARC,
      RADBUT_COHER_FIT_PHASE,
      RADBUT_COHER_FIT_INTERP,
      RADBUT_COHER_FIT_OPTIM,
      RADBUT_EDIT_OVERSKETCH_VIRTUAL_BASELINE,
      RADBUT_EDIT_OVERSKETCH_SELECTED_BASELINE,
      RADBUT_NUM
   };

   enum checkbox_id_t {
      CHECK_FLAG_SIL_VIS=0,
      CHECK_FLAG_SIL_HID,
      CHECK_FLAG_SIL_OCC,
      CHECK_FLAG_SILBF_VIS,
      CHECK_FLAG_SILBF_HID,
      CHECK_FLAG_SILBF_OCC,
      CHECK_FLAG_BORDER_VIS,
      CHECK_FLAG_BORDER_HID,
      CHECK_FLAG_BORDER_OCC,
      CHECK_FLAG_CREASE_VIS,
      CHECK_FLAG_CREASE_HID,
      CHECK_FLAG_CREASE_OCC,
      CHECK_FLAG_SEE_THRU,
      CHECK_COHER_GLOBAL,
      CHECK_COHER_SIG_1,
      CHECK_NOISE_PROTOTYPE_LOCK,
      CHECK_NOISE_OBJECT_MOTION,
      CHECK_EDIT_PRESSURE_WIDTH,
      CHECK_EDIT_PRESSURE_ALPHA,
      CHECK_NUM
   };

   enum text_id_t {
      TEXT_NOISE_PROTOTYPE = 0,
      TEXT_EDIT_STATUS_1,
      TEXT_EDIT_STATUS_2,
      TEXT_EDIT_SYNTH_COUNT,
      TEXT_NUM
   };

 protected:
   /******** STATIC MEMBERS VARS ********/

   static ARRAY<LinePenUI*>      _ui;

   /******** MEMBERS VARS ********/

   int                           _id;
   bool                          _init;

   LinePen*                      _pen;

   ARRAY<GLUI_Button*>           _button;
   ARRAY<GLUI_Slider*>           _slider;
   ARRAY<GLUI_Panel*>            _panel;
   ARRAY<GLUI_Rollout*>          _rollout;
   ARRAY<GLUI_RadioGroup*>       _radgroup;
   ARRAY<GLUI_RadioButton*>      _radbutton;
   ARRAY<GLUI_Checkbox*>         _checkbox;
   ARRAY<GLUI_StaticText*>       _statictext;

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
 public:     
   LinePenUI(LinePen *);
   virtual ~LinePenUI();

   /******** MEMBERS METHODS ********/

   void           show();
   void           hide();
   void           update();

   void           update_stroke();
   void           apply_stroke();

   CBaseStroke*   get_stroke() { return  StrokeUI::get_params(_pen->view(),this); }

 protected:      
   void           init();

   void           cleanup_sizes(GLUI_Panel*, int);

   //Update from things selected in LinePen
   void           update_non_lives();
   
   void           update_flags();
   void           update_coherence();
   void           update_mesh();
   void           update_noise();
   void           update_edit();

   //Apply changes to things selected in LinePen
   void           apply_flags();
   void           apply_coherence();
   void           apply_mesh();
   void           apply_noise();
   void           apply_edit();

   // ******** StrokeUIClient Method ********
	virtual void	changed();
   
   virtual void	build(GLUI* g, GLUI_Panel *p, int w);
	virtual void	destroy(GLUI* g, GLUI_Panel *p);

   virtual const char * plugin_name() { return "Line";}  

   /******** STATIC CALLBACK METHODS ********/
   static void    button_cb(int id);
   static void    slider_cb(int id);
   static void    radiogroup_cb(int id);
   static void    checkbox_cb(int id);

};


#endif // _LINE_PEN_UI_H_IS_INCLUDED_

/* end of file line_pen_ui.H */
