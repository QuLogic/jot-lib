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
#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "img_line_ui.H"
#include "light_ui.H"
#include "color_ui.H"
#include "detail_ctrl_ui.H" 
#include "basecoat_ui.H" 

#include "presets_ui.H"
#include "tone_shader_ui.H"
#include "gui.H"

#include "npr/img_line.H"
#include "npr/img_line_shader.H"
#include "npr/npr_view.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)


/*****************************************************************
* ImageLineUI
*****************************************************************/

vector<ImageLineUI*> ImageLineUI::_ui;
HASH                 ImageLineUI::_hash(16);

const static int WIN_WIDTH=300; 
const static int MAX_LAYERS = 4;

ImageLineUI::ImageLineUI(BaseUI* parent) :
BaseUI(parent,"Image Line UI"), _color_sel(0)
{
   _ui.push_back(this);
   _id = (_ui.size()-1); 
   _texture_selection_ui = new PatchSelectionUI(this, true);
   _tone_shader_ui = new ToneShaderUI(this);
   _light_ui = new LightUI(VIEW::peek());
   _current_tex = 0;  
   _color_ui = new ColorUI(this);
   _detail_ctrl_ui = new DetailCtrlUI(this); 
   _basecoat_ui = new BasecoatUI(this); 
}

ImageLineShader* 
ImageLineUI::get_current_image_line_shader(CVIEWptr& v) 
{
   return ImageLineShader::upcast(Patch::focus()->cur_tex(v));
}

ImageLineShader*  
ImageLineUI::get_image_line_shader()
{
   if(!_current_tex){
      _current_tex = get_current_image_line_shader(VIEW::peek());
   }

   return _current_tex;
}

ToneShader*  
ImageLineUI::get_tone_shader()
{
   if(!_current_tex){
      _current_tex = get_current_image_line_shader(VIEW::peek());

      if(!_current_tex)
	 return 0;
   }

   return _current_tex->get_tone_shader();
}

int
ImageLineUI::get_current_layer()
{
   return _texture_selection_ui->get_layer_num();
}

void     
ImageLineUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

   _rollout[ROLLOUT_MAIN] 
      = (base) ? glui->add_rollout_to_panel(base, "Image Line UI",open)
	 : glui->add_rollout("Image Line UI",open);

   _panel[PANEL_MAIN] = glui->add_panel_to_panel(_rollout[ROLLOUT_MAIN], "");

   _texture_selection_ui->build(glui, _panel[PANEL_MAIN], true);

   _rollout[ROLLOUT_LINE2] = glui->add_rollout_to_panel(_panel[PANEL_MAIN], "Line Parameters");

   _panel[PANEL_CONFIDENCE] = glui->add_panel_to_panel(_rollout[ROLLOUT_LINE2], "Confidence Tapering");

   _radgroup[RADGROUP_CONFIDENCE] = glui->add_radiogroup_to_panel(
				       _panel[PANEL_CONFIDENCE],
				       NULL,
				       id+RADGROUP_CONFIDENCE, radiogroup_cb);

   _radbutton[RADBUT_CONFIDENCE_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_CONFIDENCE],
					  "None");
   _radbutton[RADBUT_CONFIDENCE_NORMAL] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_CONFIDENCE],
					  "Normal");
   _radbutton[RADBUT_CONFIDENCE_DEBUG] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_CONFIDENCE],
					  "Debug");

   _panel[PANEL_SILHOUETTE] = glui->add_panel_to_panel(_rollout[ROLLOUT_LINE2], "Silhouette Tapering");

   _radgroup[RADGROUP_SILHOUETTE] = glui->add_radiogroup_to_panel(
				       _panel[PANEL_SILHOUETTE],
				       NULL,
				       id+RADGROUP_SILHOUETTE, radiogroup_cb);

   _radbutton[RADBUT_SILHOUETTE_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_SILHOUETTE],
					  "None");
   _radbutton[RADBUT_SILHOUETTE_TAPERING] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_SILHOUETTE],
					  "Tapering");

   _checkbox[CHECK_DRAW_SILHOUETTE] = glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_LINE2],
                                "Draw Silhouettes",
                                NULL,
                                CHECK_DRAW_SILHOUETTE,
                                checkbox_cb);

   _checkbox[CHECK_TAPERING_MODE] = glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_LINE2],
                                "Tapering",
                                NULL,
                                CHECK_TAPERING_MODE,
                                checkbox_cb);

   _checkbox[CHECK_TONE_EFFECT] = glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_LINE2],
                                "Alpha with Tone",
                                NULL,
                                CHECK_TONE_EFFECT,
                                checkbox_cb);

   glui->add_column_to_panel(_rollout[ROLLOUT_LINE2],true);

   _slider[SLIDE_CURV_THRESHOLD0] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Curvature Threshold0", GLUI_SLIDER_FLOAT, 
      0.000, 0.03, NULL, id+SLIDE_CURV_THRESHOLD0, slider_cb);
   _slider[SLIDE_CURV_THRESHOLD0]->set_num_graduations(100);
   _slider[SLIDE_CURV_THRESHOLD0]->set_w(200);

   _slider[SLIDE_CURV_THRESHOLD1] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Curvature Threshold1", GLUI_SLIDER_FLOAT, 
      0.000, 0.03, NULL, id+SLIDE_CURV_THRESHOLD1, slider_cb);
   _slider[SLIDE_CURV_THRESHOLD1]->set_num_graduations(100);
   _slider[SLIDE_CURV_THRESHOLD1]->set_w(200);

   _slider[SLIDE_LIGHT_CONTROL] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Light color control", GLUI_SLIDER_FLOAT, 
      0.5, 2.50, NULL, id+SLIDE_LIGHT_CONTROL, slider_cb);
   _slider[SLIDE_LIGHT_CONTROL]->set_num_graduations(100);
   _slider[SLIDE_LIGHT_CONTROL]->set_w(200);

   _slider[SLIDE_HIGHLIGHT_CONTROL] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Highlight control", GLUI_SLIDER_FLOAT, 
      0.5, 2.50, NULL, id+SLIDE_HIGHLIGHT_CONTROL, slider_cb);
   _slider[SLIDE_HIGHLIGHT_CONTROL]->set_num_graduations(100);
   _slider[SLIDE_HIGHLIGHT_CONTROL]->set_w(200);

   _slider[SLIDE_LINE_WIDTH] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Line Width", GLUI_SLIDER_FLOAT, 
      1.0, 10.0, NULL, id+SLIDE_LINE_WIDTH, slider_cb);
   _slider[SLIDE_LINE_WIDTH]->set_num_graduations(100);
   _slider[SLIDE_LINE_WIDTH]->set_w(200);

   _slider[SLIDE_ALPHA_OFFSET] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Alpha Offset", GLUI_SLIDER_FLOAT, 
      0.0, 1.0, NULL, id+SLIDE_ALPHA_OFFSET, slider_cb);
   _slider[SLIDE_ALPHA_OFFSET]->set_num_graduations(100);
   _slider[SLIDE_ALPHA_OFFSET]->set_w(200);

   _slider[SLIDE_BLUR_SIZE] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Blur Size", GLUI_SLIDER_FLOAT, 
      0, 2.0, NULL, id+SLIDE_BLUR_SIZE, slider_cb);
   _slider[SLIDE_BLUR_SIZE]->set_num_graduations(100);
   _slider[SLIDE_BLUR_SIZE]->set_w(200);

   _slider[SLIDE_MOVING_FACTOR] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Moving Factor", GLUI_SLIDER_FLOAT, 
      0.0, 1.0, NULL, id+SLIDE_MOVING_FACTOR, slider_cb);
   _slider[SLIDE_MOVING_FACTOR]->set_num_graduations(100);
   _slider[SLIDE_MOVING_FACTOR]->set_w(200);

   _slider[SLIDE_SHININESS] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Shininess", GLUI_SLIDER_FLOAT, 
      0.0, 100.0, NULL, id+SLIDE_SHININESS, slider_cb);
   _slider[SLIDE_SHININESS]->set_num_graduations(100);
   _slider[SLIDE_SHININESS]->set_w(200);

   _slider[SLIDE_HT_WIDTH_CONTROL] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_LINE2], "Highlight width control", GLUI_SLIDER_FLOAT, 
      0.0, 2.0, NULL, id+SLIDE_HT_WIDTH_CONTROL, slider_cb);
   _slider[SLIDE_HT_WIDTH_CONTROL]->set_num_graduations(100);
   _slider[SLIDE_HT_WIDTH_CONTROL]->set_w(200);

   _rollout[ROLLOUT_REF_IMG] = glui->add_rollout_to_panel(_panel[PANEL_MAIN], "Show Reference Image", false);

   _radgroup[RADGROUP_REF_IMG] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_REF_IMG],
				       NULL,
				       id+RADGROUP_REF_IMG, radiogroup_cb);

   _radbutton[RADBUT_REF_IMG_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_REF_IMG],
					  "None");

   _radbutton[RADBUT_REF_IMG_TONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_REF_IMG],
					  "Tone Image");
   _radbutton[RADBUT_REF_IMG_BLUR] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_REF_IMG],
					  "Blurred Image");

   glui->add_column_to_panel(_rollout[ROLLOUT_REF_IMG], true);

   _radgroup[RADGROUP_SHOW_CHANNEL] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_REF_IMG],
				       NULL,
				       id+RADGROUP_SHOW_CHANNEL, radiogroup_cb);

   _radbutton[RADBUT_CHANNEL_ALL] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_SHOW_CHANNEL],
					  "All");

   _radbutton[RADBUT_CHANNEL_RED] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_SHOW_CHANNEL],
					  "Red");
   _radbutton[RADBUT_CHANNEL_GREEN] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_SHOW_CHANNEL],
					  "Green");
   _radbutton[RADBUT_CHANNEL_LUMI] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_SHOW_CHANNEL],
					  "Luminance");

   _rollout[ROLLOUT_DEBUG_SHADER] = glui->add_rollout_to_panel(_panel[PANEL_MAIN], "Debug Shader", false);

   _radgroup[RADGROUP_DEBUG_SHADER] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_DEBUG_SHADER],
				       NULL,
				       id+RADGROUP_DEBUG_SHADER, radiogroup_cb);

   _radbutton[RADBUT_DEBUG_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DEBUG_SHADER],
					  "None");

   _radbutton[RADBUT_DEBUG_SIGN] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DEBUG_SHADER],
					  "Sign");
   _radbutton[RADBUT_DEBUG_MOVE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DEBUG_SHADER],
					  "Move");
   _radbutton[RADBUT_DEBUG_CURV] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DEBUG_SHADER],
					  "Curvature");
   _radbutton[RADBUT_DEBUG_DIST] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DEBUG_SHADER],
					  "Distance");
   _radbutton[RADBUT_DEBUG_ANGLE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DEBUG_SHADER],
					  "Angle");


   glui->add_column_to_panel(_panel[PANEL_MAIN],true);

   _tone_shader_ui->build(glui, _panel[PANEL_MAIN], true);

   _detail_ctrl_ui->build(glui, _rollout[ROLLOUT_MAIN], false);

    _rollout[ROLLOUT_LINE] = glui->add_rollout_to_panel( _rollout[ROLLOUT_MAIN], "Line mode",false);
   _radgroup[RADGROUP_LINE] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_LINE],
				       NULL,
				       id+RADGROUP_LINE, radiogroup_cb);

   _radbutton[RADBUT_LINE_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "None");

   _radbutton[RADBUT_LINE_ALL] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "All");

   _radbutton[RADBUT_LINE_DK] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "Only Dark");

   _radbutton[RADBUT_LINE_LT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "Only Light");

   _radbutton[RADBUT_LINE_HT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "Only Highlight");

   _radbutton[RADBUT_LINE_DK_LT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "Dark + Light");

   _radbutton[RADBUT_LINE_DK_HT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "Dark + Hightlight");

   _radbutton[RADBUT_LINE_LT_HT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_LINE],
					  "Light + Hightlight");


   _basecoat_ui->build(glui, _rollout[ROLLOUT_MAIN], false);

   _rollout[ROLLOUT_COLOR_SELECTION] = glui->add_rollout_to_panel( _rollout[ROLLOUT_MAIN], "Color Selection",false);

   _radgroup[RADGROUP_COLOR] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_COLOR_SELECTION],
				       NULL,
				       id+RADGROUP_COLOR, radiogroup_cb);

   _radbutton[RADBUT_COLOR_BASE0] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_COLOR],
					  "Basecoat0");

   _radbutton[RADBUT_COLOR_BASE1] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_COLOR],
					  "Basecoat1");

   _radbutton[RADBUT_COLOR_DARK] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_COLOR],
					  "Dark");

   _radbutton[RADBUT_COLOR_LIGHT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_COLOR],
					  "Light");

   _radbutton[RADBUT_COLOR_HIGHLIGHT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_COLOR],
					  "Highlight");

   _radbutton[RADBUT_COLOR_BACKGROUND] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_COLOR],
					  "Background");

   _color_ui->build(glui, _rollout[ROLLOUT_COLOR_SELECTION], true);

   _light_ui->build(glui,_rollout[ROLLOUT_MAIN], true); 


   // Cleanup sizes

   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }

   int w = _rollout[ROLLOUT_MAIN]->get_w();
   w = max(WIN_WIDTH, w);

   update_non_lives();
}

void
ImageLineUI::update_non_lives()
{
   toggle_enable_me();
   _texture_selection_ui->update_non_lives();
   _current_tex = get_current_image_line_shader(VIEW::peek()); 
   _tone_shader_ui->update_non_lives();
   _detail_ctrl_ui->update_non_lives(); 
   _basecoat_ui->update_non_lives(); 
   _light_ui->update_non_lives();
   _color_ui->update_non_lives();

   if(_current_tex){
      _curv_threshold[0] = _current_tex->get_curv_threshold(0);
      _slider[SLIDE_CURV_THRESHOLD0]->set_float_val(_curv_threshold[0]);

      _curv_threshold[1] = _current_tex->get_curv_threshold(1);
      _slider[SLIDE_CURV_THRESHOLD1]->set_float_val(_curv_threshold[1]);

      _light_control = _current_tex->get_light_control();
      _slider[SLIDE_LIGHT_CONTROL]->set_float_val(_light_control);

      _highlight_control = _current_tex->get_highlight_control();
      _slider[SLIDE_HIGHLIGHT_CONTROL]->set_float_val(_highlight_control);

      _line_width = _current_tex->get_line_width();
      _slider[SLIDE_LINE_WIDTH]->set_float_val(_line_width);

      _slider[SLIDE_BLUR_SIZE]->set_float_val(_current_tex->get_blur_shader()->get_blur_size()/_line_width);

      _slider[SLIDE_ALPHA_OFFSET]->set_float_val(_current_tex->get_alpha_offset());

      _slider[SLIDE_MOVING_FACTOR]->set_float_val(_current_tex->get_moving_factor());
      _slider[SLIDE_SHININESS]->set_float_val(_current_tex->patch()->shininess());
      _slider[SLIDE_HT_WIDTH_CONTROL]->set_float_val(_current_tex->get_ht_width_control());

      _radgroup[RADGROUP_CONFIDENCE]->set_int_val(_current_tex->get_confidence_mode());

      _radgroup[RADGROUP_SILHOUETTE]->set_int_val(_current_tex->get_silhouette_mode());

      _checkbox[CHECK_DRAW_SILHOUETTE]->set_int_val(_current_tex->get_draw_silhouette());

      _checkbox[CHECK_TAPERING_MODE]->set_int_val(_current_tex->get_tapering_mode());

      _checkbox[CHECK_TONE_EFFECT]->set_int_val(_current_tex->get_tone_effect());

      _radgroup[RADGROUP_LINE]->set_int_val(_current_tex->get_line_mode());


      set_colors(_current_tex, _color_sel);

      _radgroup[RADGROUP_REF_IMG]->set_int_val(_current_tex->get_draw_mode());

      _radgroup[RADGROUP_SHOW_CHANNEL]->set_int_val(((MLToneShader*)_current_tex->get_tone_shader())->get_show_channel());


      _radgroup[RADGROUP_DEBUG_SHADER]->set_int_val(_current_tex->get_debug_shader());

   }

}

void
ImageLineUI::toggle_enable_me()
{
   if(ImageLineShader::isa(Patch::focus()->cur_tex(VIEW::peek()))){
      _rollout[ROLLOUT_MAIN]->enable();
      _rollout[ROLLOUT_MAIN]->open();
   } else {
      _rollout[ROLLOUT_MAIN]->close();
      _rollout[ROLLOUT_MAIN]->disable();        
   }  
}


bool
ImageLineUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;  
   if(sender->class_name() == PatchSelectionUI::static_name()){
      switch(event)
      {
	 case PatchSelectionUI::SELECT_FILL_PATCHES:
	    _texture_selection_ui->fill_my_patch_listbox();
	    break;
	 case PatchSelectionUI::SELECT_PATCH_SELECTED:
	    if(_parent)
	       _parent->update();
	    else
	       update();
	    break;
	 case PatchSelectionUI::SELECT_LAYER_CHENGED: {
	    update();
	    break;
	 }
	 case PatchSelectionUI::SELECT_APPLY_CHANGE:
	    apply_changes();
	    break;
      } 
   }    
   if(sender->class_name() == ToneShaderUI::static_name()){
      _last_op = OP_TONE_SHADER;
   }     

   if(sender->class_name() == ColorUI::static_name()){
      apply_changes_to_texture(OP_COLOR, _current_tex, _texture_selection_ui->get_layer_num());
   }

   return s;
}

void 
ImageLineUI::apply_changes()
{
   // Figure out which patches to apply changes to and call
   // apply_changes_to_texture with them
   bool whole_mesh     = (_texture_selection_ui->get_whole_mesh());
   bool all_meshes     = (_texture_selection_ui->get_all_meshes());
   bool all_layers     = (_texture_selection_ui->get_all_layer());
   bool last_operation = (_texture_selection_ui->get_last_operation());
   int  current_layer  = _texture_selection_ui->get_layer_num();


   Patch_list patches = _texture_selection_ui->get_all_patches();

   operation_id_t my_op = (last_operation) ? _last_op : OP_ALL;

   if(!whole_mesh && !all_meshes && !all_layers){
      apply_changes_to_texture(my_op, _current_tex, current_layer);
      return;
   }

   int times = (all_layers) ? MAX_LAYERS : 1;
   ImageLineShader *tex;
   for(int i=0; i < times; ++i)
   {
      if(all_meshes && whole_mesh){
	 for(int j=0; j < patches.num(); ++j){
	    tex = ImageLineShader::upcast(patches[j]->cur_tex(VIEW::peek()));

	    if(!tex)
	       continue;

	    apply_changes_to_texture(my_op, tex,
	       (all_layers) ? i : current_layer);
	 }
      }else if (whole_mesh){
	 // Apply to patches only within the selected model
	 for(int j=0; j < patches.num(); ++j){
	    if(BMESH::is_focus(patches[j]->mesh()))
	       tex = ImageLineShader::upcast(patches[j]->cur_tex(VIEW::peek()));

	       if(!tex)
		  continue;

	    apply_changes_to_texture(my_op, tex,
	       (all_layers) ? i : current_layer);
	 }

      }else if(!all_meshes && !whole_mesh){
	 //Apply changes to all layers
	 apply_changes_to_texture(my_op,_current_tex,i);      
      }
   }

}

void
ImageLineUI::apply_changes_to_texture(operation_id_t op, ImageLineShader* tex, int layer)
{
   if((op==OP_ALL) || (op==OP_TONE_SHADER)){
        _tone_shader_ui->apply_changes_to_texture_parent((int)op, tex->get_tone_shader(), layer);
   }   
   if((op==OP_ALL) || (op==OP_COLOR)){
      get_colors(tex);
   }
   if((op==OP_ALL) || (op==OP_DEBUG_SHADER)){
      int val = _radgroup[RADGROUP_DEBUG_SHADER]->get_int_val();
      tex->set_debug_shader(val);
   }
   if((op==OP_ALL) || (op==OP_CONFIDENCE)){
      int val = _radgroup[RADGROUP_CONFIDENCE]->get_int_val();
      tex->set_confidence_mode(val);
   }
   if((op==OP_ALL) || (op==OP_SILHOUETTE_MODE)){
      int val = _radgroup[RADGROUP_SILHOUETTE]->get_int_val();
      tex->set_silhouette_mode(val);
   }
   if((op==OP_ALL) || (op==OP_DRAW_SILHOUETTE)){
      int val = _checkbox[CHECK_DRAW_SILHOUETTE]->get_int_val();
      tex->set_draw_silhouette(val);
   }
   if((op==OP_ALL) || (op==OP_TONE_EFFECT)){
      int val = _checkbox[CHECK_TONE_EFFECT]->get_int_val();
      tex->set_tone_effect(val);
   }
   if((op==OP_ALL) || (op==OP_TAPERING_MODE)){
      int val = _checkbox[CHECK_TAPERING_MODE]->get_int_val();
      tex->set_tapering_mode(val);
   }
   if((op==OP_ALL) || (op==OP_LINE)){
      int val = _radgroup[RADGROUP_LINE]->get_int_val();
      tex->set_line_mode(val);
   }
   if((op==OP_ALL) || (op==OP_CURVATURE0)){
      _curv_threshold[0] = _slider[SLIDE_CURV_THRESHOLD0]->get_float_val();
      tex->set_curv_threshold(0, _curv_threshold[0]);
   }
   if((op==OP_ALL) || (op==OP_CURVATURE1)){
      _curv_threshold[1] = _slider[SLIDE_CURV_THRESHOLD1]->get_float_val();
      tex->set_curv_threshold(1, _curv_threshold[1]);
   }
   if((op==OP_ALL) || (op==OP_HIGHLIGHT_CONTROL)){
      _highlight_control = _slider[SLIDE_HIGHLIGHT_CONTROL]->get_float_val();
      tex->set_highlight_control(_highlight_control);
   }
   if((op==OP_ALL) || (op==OP_LIGHT_CONTROL)){
      _light_control = _slider[SLIDE_LIGHT_CONTROL]->get_float_val();
      tex->set_light_control(_light_control);
   }
   if((op==OP_ALL) || (op==OP_WIDTH)){
      _line_width = _slider[SLIDE_LINE_WIDTH]->get_float_val();
      tex->set_line_width(_line_width);
      tex->get_blur_shader()->set_blur_size(_line_width*_slider[SLIDE_BLUR_SIZE]->get_float_val());
   }
   if((op==OP_ALL) || (op==OP_BLUR_SIZE)){
      float val = _slider[SLIDE_BLUR_SIZE]->get_float_val();
      tex->get_blur_shader()->set_blur_size(val*tex->get_line_width());
   }
   if((op==OP_ALL) || (op==OP_ALPHA_OFFSET)){
      float val = _slider[SLIDE_ALPHA_OFFSET]->get_float_val();
      tex->set_alpha_offset(val);
   }
   if((op==OP_ALL) || (op==OP_MOVING_FACTOR)){
      float val = _slider[SLIDE_MOVING_FACTOR]->get_float_val();
      tex->set_moving_factor(val);
   }
   if((op==OP_ALL) || (op==OP_SHININESS)){
      float val = _slider[SLIDE_SHININESS]->get_float_val();
      tex->patch()->set_shininess(val);
   }
   if((op==OP_ALL) || (op==OP_HT_WIDTH_CONTROL)){
      float val = _slider[SLIDE_HT_WIDTH_CONTROL]->get_float_val();
      tex->set_ht_width_control(val);
   }
   if((op==OP_ALL) || (op==OP_SHOW_CHANNEL)){
      int val = _radgroup[RADGROUP_SHOW_CHANNEL]->get_int_val();

      ((MLToneShader*)tex->get_tone_shader())->set_show_channel(val);
   }
   if((op==OP_ALL) || (op==OP_REF_IMG)){
      int val = _radgroup[RADGROUP_REF_IMG]->get_int_val();

      bool whole_mesh     = (_texture_selection_ui->get_whole_mesh());
      bool all_meshes     = (_texture_selection_ui->get_all_meshes());
      ImageLineShader *tex2;

      if(whole_mesh && all_meshes){
	 tex->set_draw_mode(val);
      }
      else{
	 Patch_list patches = _texture_selection_ui->get_all_patches();
	 for(int j=0; j < patches.num(); ++j){
	    tex2 = ImageLineShader::upcast(patches[j]->cur_tex(VIEW::peek()));

	    if(!tex2)
	       continue;

	    tex2->set_draw_mode(val);
	 }
      }
   } 

   _last_op = op;
}


void 
ImageLineUI::spinner_cb(int id)
{  
}

void
ImageLineUI::radiogroup_cb(int id)
{
   int val = _ui[id >> ID_SHIFT]->_radgroup[id & ID_MASK]->get_int_val();
   int layer = _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num();
   int i = id >> ID_SHIFT;

   switch (id & ID_MASK) {
      case RADGROUP_LINE:
	 _ui[i]->apply_changes_to_texture(OP_LINE, _ui[i]->_current_tex, layer);
	 break;
      case RADGROUP_COLOR:
	 _ui[i]->set_colors(_ui[i]->_current_tex, val);
	 break;
      case RADGROUP_SHOW_CHANNEL: 
	 _ui[i]->apply_changes_to_texture(OP_SHOW_CHANNEL, _ui[i]->_current_tex, layer);
	 break;
      case RADGROUP_REF_IMG: 
	 _ui[i]->apply_changes_to_texture(OP_REF_IMG, _ui[i]->_current_tex, layer);
/*	 if(val == 0){
	    NPRview::_draw_flag = NPRview::SHOW_NONE;
	 }
	 else if(val == 1){
	    NPRview::_draw_flag = NPRview::SHOW_TEX_MEM0;
	 }
	 else if(val == 2){
	    NPRview::_draw_flag = NPRview::SHOW_TEX_MEM1;
	 }*/
	 break;
      case RADGROUP_DEBUG_SHADER: 
	 _ui[i]->apply_changes_to_texture(OP_DEBUG_SHADER, _ui[i]->_current_tex, layer);
	 break;
      case RADGROUP_CONFIDENCE: 
	 _ui[i]->apply_changes_to_texture(OP_CONFIDENCE, _ui[i]->_current_tex, layer);
      case RADGROUP_SILHOUETTE: 
	 _ui[i]->apply_changes_to_texture(OP_SILHOUETTE_MODE, _ui[i]->_current_tex, layer);
	 break;
   }
}

/////////////////////////////////////
// fetch() - Implicit Constructor
/////////////////////////////////////

ImageLineUI*
ImageLineUI::fetch(CVIEWptr& v)
{
   if (!v) {
      err_msg("ImageLineUI::fetch() - Error! view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg("ImageLineUI::fetch() - Error! view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself

   ImageLineUI* vui = (ImageLineUI*)_hash.find((long)v->impl());

   if (!vui) {
      vui = new ImageLineUI();
      assert(vui);
      _hash.add((long)v->impl(), (void *)vui);

   }

   return vui;
}

/////////////////////////////////////
// is_vis()
/////////////////////////////////////

bool
ImageLineUI::is_vis_external(CVIEWptr& v)
{

   ImageLineUI* vui;

   if (!(vui = ImageLineUI::fetch(v))) {
      err_msg("ImageLineUI::show - Error! Failed to fetch ImageLineUI!");
      return false;
   }

   return vui->is_vis();

}


/////////////////////////////////////
// show()
/////////////////////////////////////

bool
ImageLineUI::show_external(CVIEWptr& v)
{

   ImageLineUI* vui;

   if (!(vui = ImageLineUI::fetch(v))) {
      err_msg("ImageLineUI::show - Error! Failed to fetch ImageLineUI!");
      return false;
   }

   vui->show();

   return true;

}

/////////////////////////////////////
// hide()
/////////////////////////////////////

bool
ImageLineUI::hide_external(CVIEWptr& v)
{

   ImageLineUI* vui;

   if (!(vui = ImageLineUI::fetch(v))) {
      err_msg("ImageLineUI::hide - Error! Failed to fetch ImageLineUI!");
      return false;
   }

   vui->hide();

   return true;
}

/////////////////////////////////////
// update()
/////////////////////////////////////

bool
ImageLineUI::update_external(CVIEWptr& v)
{
   ImageLineUI* vui;

   if (!(vui = ImageLineUI::fetch(v))) {
      err_msg("ImageLineUI::update - Error! Failed to fetch ImageLineUI!");
      return false;
   }

   vui->update();

   return true;

}

void
ImageLineUI::get_colors(ImageLineShader* tex)
{

   COLOR c = _color_ui->get_color(); 

   if (_color_sel == 0) {
      tex->get_basecoat_shader()->set_base_color(0, c);
   }
   else if(_color_sel == 1){
      tex->get_basecoat_shader()->set_base_color(1, c);
   }
   else if(_color_sel == 2){
      tex->set_dark_color(c);
   }
   else if(_color_sel == 3){
      tex->set_light_color(c);
   }
   else if(_color_sel == 4){
      tex->set_highlight_color(c);
   }
   else if(_color_sel == 5){
      VIEW::peek()->set_color(c);
   }
        
}

void
ImageLineUI::set_colors(ImageLineShader* tex, const int i)
{
   if(!tex)
      return;

   COLOR c;

   _color_sel = i;

   if (i == 0) {
      c = tex->get_basecoat_shader()->get_base_color(0);
   }
   else if (i == 1) {
      c = tex->get_basecoat_shader()->get_base_color(1);
   }
   else if(i == 2){
      c = tex->get_dark_color();
   }
   else if(i == 3){
      c = tex->get_light_color();
   }
   else if(i == 4){
      c = tex->get_highlight_color();
   }
   else if(i == 5){
      c = VIEW::peek()->color();
   }
        
   _color_ui->set_current_color(c[0],c[1],c[2], true, true); 

}

void
ImageLineUI::checkbox_cb(int id)
{
   int i = id >> ID_SHIFT;
   int layer = _ui[i]->_texture_selection_ui->get_layer_num();
   ImageLineShader *tex = _ui[i]->_current_tex;

   switch(id&ID_MASK)
   {
      case CHECK_DRAW_SILHOUETTE: 
         _ui[i]->apply_changes_to_texture(OP_DRAW_SILHOUETTE, tex, layer);
         break;
      case CHECK_TAPERING_MODE: 
         _ui[i]->apply_changes_to_texture(OP_TAPERING_MODE, tex, layer);
         break;
      case CHECK_TONE_EFFECT: 
         _ui[i]->apply_changes_to_texture(OP_TONE_EFFECT, tex, layer);
         break;
   }
}

void
ImageLineUI::slider_cb(int id)
{
   int i = id >> ID_SHIFT;
   int layer = _ui[i]->_texture_selection_ui->get_layer_num();
   ImageLineShader *tex = _ui[i]->_current_tex;

   switch(id&ID_MASK)
   {
      case SLIDE_CURV_THRESHOLD0:
	 _ui[i]->apply_changes_to_texture(OP_CURVATURE0, tex, layer);
	 break;
      case SLIDE_CURV_THRESHOLD1:
	 _ui[i]->apply_changes_to_texture(OP_CURVATURE1, tex, layer);
	 break;
      case SLIDE_HIGHLIGHT_CONTROL:
	 _ui[i]->apply_changes_to_texture(OP_HIGHLIGHT_CONTROL, tex, layer);
	 break;
      case SLIDE_LIGHT_CONTROL:
	 _ui[i]->apply_changes_to_texture(OP_LIGHT_CONTROL, tex, layer);
	 break;
      case SLIDE_LINE_WIDTH:
	 _ui[i]->apply_changes_to_texture(OP_WIDTH, tex, layer);
	 break;
      case SLIDE_ALPHA_OFFSET:
	 _ui[i]->apply_changes_to_texture(OP_ALPHA_OFFSET, tex, layer);
	 break;
      case SLIDE_MOVING_FACTOR:
	 _ui[i]->apply_changes_to_texture(OP_MOVING_FACTOR, tex, layer);
	 break;
      case SLIDE_SHININESS:
	 _ui[i]->apply_changes_to_texture(OP_SHININESS, tex, layer);
	 break;
      case SLIDE_HT_WIDTH_CONTROL:
	 _ui[i]->apply_changes_to_texture(OP_HT_WIDTH_CONTROL, tex, layer);
	 break;
      case SLIDE_BLUR_SIZE:
	 _ui[i]->apply_changes_to_texture(OP_BLUR_SIZE, tex, layer);
	 break;
   } 
}

void
ImageLineUI::listbox_cb(int id)
{
}


// img_line_ui.C
