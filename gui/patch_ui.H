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
#ifndef _PATCH_UI_H_IS_INCLUDED_
#define _PATCH_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// PatchUI
////////////////////////////////////////////

#include "mesh/patch.H"
#include "gui/base_ui.H"

#include <vector>

class PatchSelectionUI;
class RefImageUI;
class DetailCtrlUI;


/*****************************************************************
 * PatchUI
 *****************************************************************/
class PatchUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_SPS_APPLY=0,
      BUT_NUM 
   };
   enum rotation_id_t {      
      ROT_NUM
   };
   enum listbox_id_t { 
      LIST_TEX_SEL = 0,
      LIST_NUM
   };
   enum slider_id_t {
      SLIDE_REMAP_POWER,
      SLIDE_SHADOW_SCALE,
      SLIDE_SHADOW_SOFT,
      SLIDE_SHADOW_OFFSET,
      SLIDE_NUM
   };
   enum spinner_id_t{
      SPINNER_SPS_HIGHT=0,
      SPINNER_SPS_MIN_DIST,
      SPINNER_SPS_REG,
      SPINNER_N_RING,
      SPINNER_NUM
   };
   enum panel_id_t {
      PANEL_SPS=0,
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_PATCH = 0,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t {      
      RADGROUP_NUM
   };
   enum radiobutton_id_t {      
      RADBUT_NUM
   };
   enum checkbox_id_t {      
      CHECK_TRACKING,
      CHECK_LOD,
      CHECK_ROTATION,
      CHECK_TIMED_LOD,
      CHECK_USE_DIRECTION,
      CHECK_USE_WEIGHTED_LS,
      CHECK_USE_VIS_TEST,
      CHECK_SHOW_BLEND,
      CHECK_OCCLUDER,
      CHECK_RECIEVER,
      CHECK_HALO,
      CHECK_NUM
   }; 
   enum operation_id_t {
      OP_ALL=0,
      OP_TRACKING,
      OP_LOD,
      OP_ROTATION,
      OP_TIMED_LOD,
      OP_TEX_CHANGED,
      OP_USE_DIRECTION,
      OP_USE_WEIGHTED_LS,
      OP_USE_VIS_TEST,
      OP_SHOW_BLEND,
      OP_NRING,
      OP_REMAP_POWER,
      OP_SPS_APPLY, 
      OP_OCCLUDER,
      OP_RECIEVER,
      OP_SSCALE,
      OP_SSOFTNESS,
      OP_SOFFSET,
      OP_HALO,
      OP_NUM
   };
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<PatchUI*>  _ui;       
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PatchUI", PatchUI*, PatchUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   PatchUI(BaseUI* parent=0);
   virtual ~PatchUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   virtual bool child_callback(BaseUI* sender, int event);
 protected:   
   /******** MEMBERS VARS ********/
   int                _id;
   Patch*             _patch;
   Patch_list         _patches;
   operation_id_t     _last_op;

   PatchSelectionUI*  _patch_selection_ui;
   RefImageUI*        _ref_image_ui;
   DetailCtrlUI*      _detail_ctrl_ui;

 protected:     
   /******** Convenience Methods ********/
   void update_dynamic_params();

   void apply_changes();
   void apply_changes_to_patch(operation_id_t op, Patch* p);
   
   void  fill_texture_listbox();
   char* get_current_tex_name();
   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);
   static void  listbox_cb(int id);  
   static void  checkbox_cb(int id);
   static void  spinner_cb(int id);
   static void  slider_cb(int id);
};


#endif // _PATCH_UI_H_IS_INCLUDED_

/* end of file patch_ui.H */
