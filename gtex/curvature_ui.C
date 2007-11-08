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
/*!
 *  \file curvature_ui.C
 *
 *  \brief Contains the implementation of the class for the UI for manipulating
 *  curvature related GTextures.
 *
 *  \sa curvature_ui.H
 *
 */

#include <map>

using namespace std;

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "glui/glui.h"

#include "gtex/line_drawing.H"
#include "gtex/curvature_texture.H"

#include "curvature_ui.H"

/*!
 *  \brief Controls the UI for curvature related GTextures.
 *
 */
class CurvatureUI {
   
   public:
   
      CurvatureUI(VIEWptr v);
      
      ~CurvatureUI();
      
      bool is_vis();
      
      bool show();
      bool hide();
      
      bool update();
      
      static void checkbox_cb(int id);
      static void slider_cb(int id);
      static void radiogroup_cb(int id);
   
   private:
   
      void build();
      void destroy();
   
      VIEWptr view;
      
      GLUI *glui;
      
      enum rollout_ids_t {
         ROLLOUT_LINE_DRAWING,
         ROLLOUT_CURVATURE_VIS,
         ROLLOUT_CURVATURE_GAUSSIAN_FILTER,
         ROLLOUT_CURVATURE_MEAN_FILTER,
         ROLLOUT_CURVATURE_RADIAL_FILTER
      };
      
      typedef map<rollout_ids_t, GLUI_Rollout*> rollout_map_t;
      rollout_map_t rollouts;
      
      enum checkbox_ids_t {
         CHECKBOX_DRAW_CONTOURS,
         CHECKBOX_DRAW_SUGCONTOURS,
         CHECKBOX_DRAW_COLOR,
         CHECKBOX_DRAW_GAUSSIAN_CURVATURE,
         CHECKBOX_DRAW_MEAN_CURVATURE,
         CHECKBOX_DRAW_RADIAL_CURVATURE
      };
      
      typedef map<checkbox_ids_t, GLUI_Checkbox*> checkbox_map_t;
      checkbox_map_t checkboxes;
      
      enum slider_ids_t {
         SLIDER_SC_THRESH
      };
      
      typedef map<slider_ids_t, GLUI_Slider*> slider_map_t;
      slider_map_t sliders;
      
      enum radiogroup_ids_t {
         RADIOGROUP_GAUSSIAN_FILTER,
         RADIOGROUP_MEAN_FILTER,
         RADIOGROUP_RADIAL_FILTER
      };
      
      typedef map<radiogroup_ids_t, GLUI_RadioGroup*> radiogroup_map_t;
      radiogroup_map_t radiogroups;
      
      enum radiobutton_ids_t {
         RADIOBUTTON_GAUSSIAN_FILTER_NONE,
         RADIOBUTTON_GAUSSIAN_FILTER_GAUSSIAN,
         RADIOBUTTON_GAUSSIAN_FILTER_MEAN,
         RADIOBUTTON_GAUSSIAN_FILTER_RADIAL,
         RADIOBUTTON_MEAN_FILTER_NONE,
         RADIOBUTTON_MEAN_FILTER_GAUSSIAN,
         RADIOBUTTON_MEAN_FILTER_MEAN,
         RADIOBUTTON_MEAN_FILTER_RADIAL,
         RADIOBUTTON_RADIAL_FILTER_NONE,
         RADIOBUTTON_RADIAL_FILTER_GAUSSIAN,
         RADIOBUTTON_RADIAL_FILTER_MEAN,
         RADIOBUTTON_RADIAL_FILTER_RADIAL
      };
      
      typedef map<radiobutton_ids_t, GLUI_RadioButton*> radiobutton_map_t;
      radiobutton_map_t radiobuttons;
   
      CurvatureUI(const CurvatureUI &);
      CurvatureUI &operator=(const CurvatureUI &);
   
};

//----------------------------------------------------------------------------//

CurvatureUISingleton::CurvatureUISingleton()
   : _sc_thresh(0.05),
     _line_drawing_draw_contours(true),
     _line_drawing_draw_sugcontours(true),
     _line_drawing_draw_color(false),
     _curvature_draw_gaussian_curv(true),
     _curvature_draw_mean_curv(true),
     _curvature_draw_radial_curv(true),
     _curvature_gaussian_filter(CurvatureTexture::FILTER_NONE),
     _curvature_mean_filter(CurvatureTexture::FILTER_NONE),
     _curvature_radial_filter(CurvatureTexture::FILTER_NONE)
{
   
}
      
CurvatureUISingleton::~CurvatureUISingleton()
{
   
   view2ui_map_t::iterator itor = view2ui_map.begin();
   
   for(;itor != view2ui_map.end(); ++itor){
      
      delete itor->second;
      
   }
   
}

bool
CurvatureUISingleton::is_vis(CVIEWptr& v)
{
   
   CurvatureUI *cui = fetch(v);

   if (!cui){
      
      err_msg("CurvatureUISingleton::is_vis() - Error! Failed to fetch CurvatureUI!");
      return false;
      
   }

   return cui->is_vis();
   
}
      
bool
CurvatureUISingleton::show(CVIEWptr& v)
{
   
   CurvatureUI *cui = fetch(v);

   if(!cui){
      
      err_msg("CurvatureUISingleton::show() - Error! Failed to fetch CurvatureUI!");
      return false;
      
   }

   if(!cui->show()){
      
      err_msg("CurvatureUISingleton::show() - Error! Failed to show CurvatureUI!");
      return false;
      
   } else {
      
      err_msg("CurvatureUISingleton::show() - Sucessfully showed CurvatureUI.");
      return true;
      
   }
   
}

bool
CurvatureUISingleton::hide(CVIEWptr& v)
{
   
   CurvatureUI *cui = fetch(v);

   if(!cui){
      
      err_msg("CurvatureUISingleton::hide() - Error! Failed to fetch CurvatureUI!");
      return false;
      
   }

   if(!cui->hide()){
      
      err_msg("CurvatureUISingleton::hide() - Error! Failed to hide CurvatureUI!");
      return false;
      
   } else {
      
      err_msg("CurvatureUISingleton::hide() - Sucessfully hid CurvatureUI.");
      return true;
      
   }
   
}

bool
CurvatureUISingleton::update(CVIEWptr& v)
{
   
   CurvatureUI *cui = fetch(v);

   if (!cui){
      
      err_msg("CurvatureUISingleton::update() - Error! Failed to fetch CurvatureUI!");
      return false;
      
   }

   if (!cui->update()){
      
      err_msg("CurvatureUISingleton::update() - Error! Failed to update CurvatureUI!");
      return false;
      
   } else {
      
      err_msg("CurvatureUISingleton::update() - Sucessfully updated CurvatureUI.");
      return true;
      
   }
   
}   

CurvatureUI*
CurvatureUISingleton::fetch(CVIEWptr& v)
{
   
   if (!v) {
      err_msg("CurvatureUISingleton::fetch() - Error! view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg("CurvatureUISingleton::fetch() - Error! view->impl() is nil");
      return 0;
   }
   
   view2ui_map_t::iterator v2ui_itor = view2ui_map.find(v->impl());
   
   if(v2ui_itor != view2ui_map.end()){
      
      return v2ui_itor->second;
      
   } else {
      
      CurvatureUI *cui = new CurvatureUI(v);
      
      view2ui_map.insert(make_pair(v->impl(), cui));
      
      return cui;
      
   }
   
}

//----------------------------------------------------------------------------//

CurvatureUI::CurvatureUI(VIEWptr v)
   : view(v), glui(0)
{
   
}
      
CurvatureUI::~CurvatureUI()
{
   
   if(glui) destroy();
   
}

bool
CurvatureUI::is_vis()
{
   
   return glui != 0;
   
}

bool
CurvatureUI::show()
{
   
   if(glui){
      
      cerr << "CurvatureUI::show() - Error! CurvatureUI is already shown!" << endl;
      return false;
      
   } else {
      
      build();

      if(!glui){
         
         cerr << "CurvatureUI::show() - Error! CurvatureUI failed to build GLUI object!"
              << endl;
         return false;
         
      } else {
         
         glui->show();

         // Update the controls that don't use
         // 'live' variables
         //update_non_lives();

         glui->sync_live();

         return true;
         
      }
      
   }
   
}

bool
CurvatureUI::hide()
{
   
   if(!glui){
      
      cerr << "CurvatureUI::hide() - Error! CurvatureUI is already hidden!" << endl;
      return false;
      
   } else {

      glui->hide();
 
      destroy();

      assert(!glui);

      return true;
      
   }
   
}

bool
CurvatureUI::update()
{
   
   if(!glui){
      
      cerr << "CurvatureUI::update() - Error! No GLUI object to update (not showing)!"
           << endl;
      return false;
      
   } else {
      
      // Update the controls that don't use
      // 'live' variables
      //update_non_lives();

      glui->sync_live();

      return true;
      
   }
   
}

void CurvatureUI::checkbox_cb(int id)
{
   
   checkbox_ids_t enum_id = static_cast<checkbox_ids_t>(id);
   
   CurvatureUI *cui = CurvatureUISingleton::Instance().fetch(VIEW::peek());
   
   // Make sure a checkbox with the given ID exists:
   assert(cui->checkboxes.count(enum_id) != 0);
   
   bool checkbox_val = (cui->checkboxes[enum_id]->get_int_val() != 0);
   
   switch(enum_id){
      
      case CHECKBOX_DRAW_CONTOURS:
      
         LineDrawingTexture::set_draw_contours(checkbox_val);
      
         break;
      
      case CHECKBOX_DRAW_SUGCONTOURS:
      
         LineDrawingTexture::set_draw_sugcontours(checkbox_val);
      
         break;
      
      case CHECKBOX_DRAW_COLOR:
      
         LineDrawingTexture::set_draw_in_color(checkbox_val);
      
         break;
      
      case CHECKBOX_DRAW_GAUSSIAN_CURVATURE:
      
         CurvatureTexture::set_draw_gaussian_curv(checkbox_val);
      
         break;
      
      case CHECKBOX_DRAW_MEAN_CURVATURE:
      
         CurvatureTexture::set_draw_mean_curv(checkbox_val);
      
         break;
      
      case CHECKBOX_DRAW_RADIAL_CURVATURE:
      
         CurvatureTexture::set_draw_radial_curv(checkbox_val);
      
         break;
         
      default:
      
         cerr << "CurvatureUI::checkbox_cb() - Error:  Invalid checkbox ID!"
              << endl;
      
         break;
      
   }
   
}

void CurvatureUI::slider_cb(int id)
{
   
   slider_ids_t enum_id = static_cast<slider_ids_t>(id);
   
   CurvatureUI *cui = CurvatureUISingleton::Instance().fetch(VIEW::peek());
   
   // Make sure a slider with the given ID exists:
   assert(cui->sliders.count(enum_id) != 0);
   
   float slider_val = cui->sliders[enum_id]->get_float_val();
   
   switch(enum_id){
      
      case SLIDER_SC_THRESH:
      
         LineDrawingTexture::set_sugcontour_thresh(slider_val);
         CurvatureTexture::set_sugcontour_thresh(slider_val);
      
         break;
         
      default:
      
         cerr << "CurvatureUI::slider_cb() - Error:  Invalid slider ID!"
              << endl;
      
         break;
      
   }
   
}

void CurvatureUI::radiogroup_cb(int id)
{
   
   radiogroup_ids_t enum_id = static_cast<radiogroup_ids_t>(id);
   
   CurvatureUI *cui = CurvatureUISingleton::Instance().fetch(VIEW::peek());
   
   // Make sure a radiogroup with the given ID exists:
   assert(cui->radiogroups.count(enum_id) != 0);
   
   int radiogroup_val = cui->radiogroups[enum_id]->get_int_val();
   
   CurvatureTexture::curvature_filter_t filter_val;
   
   switch(radiogroup_val){
      
      case 0:
      
         filter_val = CurvatureTexture::FILTER_NONE;
      
         break;
      
      case 1:
      
         filter_val = CurvatureTexture::FILTER_GAUSSIAN;
      
         break;
      
      case 2:
      
         filter_val = CurvatureTexture::FILTER_MEAN;
      
         break;
      
      case 3:
      
         filter_val = CurvatureTexture::FILTER_RADIAL;
      
         break;
      
      default:
      
         cerr << "CurvatureUI::radiogroup_cb() - Error:  Invalid curvature filter type!"
              << endl;
      
         filter_val = CurvatureTexture::FILTER_NONE;
      
         break;
      
   }
   
   switch(enum_id){
      
      case RADIOGROUP_GAUSSIAN_FILTER:
      
         CurvatureTexture::set_gaussian_filter(filter_val);
      
         break;
      
      case RADIOGROUP_MEAN_FILTER:
      
         CurvatureTexture::set_mean_filter(filter_val);
      
         break;
      
      case RADIOGROUP_RADIAL_FILTER:
      
         CurvatureTexture::set_radial_filter(filter_val);
      
         break;
         
      default:
      
         cerr << "CurvatureUI::radiogroup_cb() - Error:  Invalid radiogroup ID!"
              << endl;
      
         break;
      
   }
   
}

void
CurvatureUI::build()
{
   
   assert(!glui);

   int root_x, root_y, root_w, root_h;
   view->win()->size(root_w,root_h);
   view->win()->position(root_x,root_y);

   glui = GLUI_Master.create_glui("Curvature gTexture Controls", 0,
                                  root_x + root_w + 10, root_y);
   glui->set_main_gfx_window(view->win()->id());
   
   // General Controls:
   
   sliders[SLIDER_SC_THRESH]
      = glui->add_slider("Suggestive Contour Threshold",
         GLUI_SLIDER_FLOAT, 0.0f, 0.1f,
         &(CurvatureUISingleton::Instance()._sc_thresh),
         SLIDER_SC_THRESH, &CurvatureUI::slider_cb);
   
   // Line Drawing gTexture Controls:
   
   rollouts[ROLLOUT_LINE_DRAWING]
      = glui->add_rollout("Line Drawing Controls", true);
   
   checkboxes[CHECKBOX_DRAW_CONTOURS]
      = glui->add_checkbox_to_panel(rollouts[ROLLOUT_LINE_DRAWING],
         "Draw Contours",
         &(CurvatureUISingleton::Instance()._line_drawing_draw_contours),
         CHECKBOX_DRAW_CONTOURS, &CurvatureUI::checkbox_cb);
   checkboxes[CHECKBOX_DRAW_SUGCONTOURS]
      = glui->add_checkbox_to_panel(rollouts[ROLLOUT_LINE_DRAWING],
         "Draw Suggestive Contours",
         &(CurvatureUISingleton::Instance()._line_drawing_draw_sugcontours),
         CHECKBOX_DRAW_SUGCONTOURS, &CurvatureUI::checkbox_cb);
   checkboxes[CHECKBOX_DRAW_COLOR]
      = glui->add_checkbox_to_panel(rollouts[ROLLOUT_LINE_DRAWING],
         "Draw in Color",
         &(CurvatureUISingleton::Instance()._line_drawing_draw_color),
         CHECKBOX_DRAW_COLOR, &CurvatureUI::checkbox_cb);
   
   // Curvature gTexture Controls:
   
   rollouts[ROLLOUT_CURVATURE_VIS]
      = glui->add_rollout("Curvature Controls", true);
   
   checkboxes[CHECKBOX_DRAW_GAUSSIAN_CURVATURE]
      = glui->add_checkbox_to_panel(rollouts[ROLLOUT_CURVATURE_VIS],
         "Draw Gaussian Curvature",
         &(CurvatureUISingleton::Instance()._curvature_draw_gaussian_curv),
         CHECKBOX_DRAW_GAUSSIAN_CURVATURE, &CurvatureUI::checkbox_cb);
   rollouts[ROLLOUT_CURVATURE_GAUSSIAN_FILTER]
      = glui->add_rollout_to_panel(rollouts[ROLLOUT_CURVATURE_VIS],
         "Gaussian Curvature Filter", true);
   radiogroups[RADIOGROUP_GAUSSIAN_FILTER]
      = glui->add_radiogroup_to_panel(rollouts[ROLLOUT_CURVATURE_GAUSSIAN_FILTER],
         &(CurvatureUISingleton::Instance()._curvature_gaussian_filter),
         RADIOGROUP_GAUSSIAN_FILTER, &CurvatureUI::radiogroup_cb);
   radiobuttons[RADIOBUTTON_GAUSSIAN_FILTER_NONE]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_GAUSSIAN_FILTER],
         "None");
   radiobuttons[RADIOBUTTON_GAUSSIAN_FILTER_GAUSSIAN]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_GAUSSIAN_FILTER],
         "By Derivative of Gaussian Curvature");
   radiobuttons[RADIOBUTTON_GAUSSIAN_FILTER_MEAN]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_GAUSSIAN_FILTER],
         "By Derivative of Mean Curvature");
   radiobuttons[RADIOBUTTON_GAUSSIAN_FILTER_RADIAL]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_GAUSSIAN_FILTER],
         "By Derivative of Radial Curvature");
   
   checkboxes[CHECKBOX_DRAW_MEAN_CURVATURE]
      = glui->add_checkbox_to_panel(rollouts[ROLLOUT_CURVATURE_VIS],
         "Draw Mean Curvature",
         &(CurvatureUISingleton::Instance()._curvature_draw_mean_curv),
         CHECKBOX_DRAW_MEAN_CURVATURE, &CurvatureUI::checkbox_cb);
   rollouts[ROLLOUT_CURVATURE_MEAN_FILTER]
      = glui->add_rollout_to_panel(rollouts[ROLLOUT_CURVATURE_VIS],
         "Mean Curvature Filter", true);
   radiogroups[RADIOGROUP_MEAN_FILTER]
      = glui->add_radiogroup_to_panel(rollouts[ROLLOUT_CURVATURE_MEAN_FILTER],
         &(CurvatureUISingleton::Instance()._curvature_mean_filter),
         RADIOGROUP_MEAN_FILTER, &CurvatureUI::radiogroup_cb);
   radiobuttons[RADIOBUTTON_MEAN_FILTER_NONE]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_MEAN_FILTER],
         "None");
   radiobuttons[RADIOBUTTON_MEAN_FILTER_GAUSSIAN]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_MEAN_FILTER],
         "By Derivative of Gaussian Curvature");
   radiobuttons[RADIOBUTTON_MEAN_FILTER_MEAN]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_MEAN_FILTER],
         "By Derivative of Mean Curvature");
   radiobuttons[RADIOBUTTON_MEAN_FILTER_RADIAL]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_MEAN_FILTER],
         "By Derivative of Radial Curvature");
   
   checkboxes[CHECKBOX_DRAW_RADIAL_CURVATURE]
      = glui->add_checkbox_to_panel(rollouts[ROLLOUT_CURVATURE_VIS],
         "Draw Radial Curvature",
         &(CurvatureUISingleton::Instance()._curvature_draw_radial_curv),
         CHECKBOX_DRAW_RADIAL_CURVATURE, &CurvatureUI::checkbox_cb);
   rollouts[ROLLOUT_CURVATURE_RADIAL_FILTER]
      = glui->add_rollout_to_panel(rollouts[ROLLOUT_CURVATURE_VIS],
         "Radial Curvature Filter", true);
   radiogroups[RADIOGROUP_RADIAL_FILTER]
      = glui->add_radiogroup_to_panel(rollouts[ROLLOUT_CURVATURE_RADIAL_FILTER],
         &(CurvatureUISingleton::Instance()._curvature_radial_filter),
         RADIOGROUP_RADIAL_FILTER, &CurvatureUI::radiogroup_cb);
   radiobuttons[RADIOBUTTON_RADIAL_FILTER_NONE]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_RADIAL_FILTER],
         "None");
   radiobuttons[RADIOBUTTON_RADIAL_FILTER_GAUSSIAN]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_RADIAL_FILTER],
         "By Derivative of Gaussian Curvature");
   radiobuttons[RADIOBUTTON_RADIAL_FILTER_MEAN]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_RADIAL_FILTER],
         "By Derivative of Mean Curvature");
   radiobuttons[RADIOBUTTON_RADIAL_FILTER_RADIAL]
      = glui->add_radiobutton_to_group(radiogroups[RADIOGROUP_RADIAL_FILTER],
         "By Derivative of Radial Curvature");
   
}

void
CurvatureUI::destroy()
{
   
   assert(glui);
   
   rollouts.clear();
   checkboxes.clear();
   sliders.clear();
   
   //Recursively kills off all controls, and itself
   glui->close();

   glui = 0;
   
}
