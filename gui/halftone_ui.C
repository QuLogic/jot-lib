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

#include "proxy_pattern/proxy_pen_ui.H"
#include "halftone_ui.H"
#include "color_ui.H"
#include "light_ui.H"

#include "presets_ui.H"
#include "tone_shader_ui.H"
#include "gui.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)


/*****************************************************************
 * HalftoneUI
 *****************************************************************/

vector<HalftoneUI*> HalftoneUI::_ui;

const static int WIN_WIDTH=300; 
const static int MAX_LAYERS   = 4;

HalftoneUI::HalftoneUI(BaseUI* parent) :
     BaseUI(parent,"Haching UI"),
        _color_sel(0)
{
   _ui.push_back(this);
   _id = (_ui.size()-1); 
   _color_ui = new ColorUI(this);
   _presets_ui = new PresetsUI(this, str_ptr("nprdata/halftone_presets/"), str_ptr(".hatch"));
   _texture_selection_ui = new PatchSelectionUI(this, true);
   _tone_shader_ui = new ToneShaderUI(this);
   _current_halftone = 0;  
}


void     
HalftoneUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

    _rollout[ROLLOUT_MAIN] = (base) ? glui->add_rollout_to_panel(base, "Halftone UI",open)
                                    : glui->add_rollout("Halftone UI",open);
   
    
   _panel[PANEL_MAIN] = glui->add_panel_to_panel(_rollout[ROLLOUT_MAIN], "");
   
  
   _texture_selection_ui->build(glui, _panel[PANEL_MAIN], true);
   
   glui->add_column_to_panel(_panel[PANEL_MAIN],true);

   _presets_ui->build(glui,_panel[PANEL_MAIN], true); 

   // Proparties   
   _panel[PANEL_PROPERTIES] = glui->add_panel_to_panel(_rollout[ROLLOUT_MAIN], "");

   _listbox[LIST_PATTERN] = glui->add_listbox_to_panel(
      _panel[PANEL_PROPERTIES], 
      "Pattern", NULL, 
      id+LIST_PATTERN, listbox_cb);
   
   fill_directory_listbox(_listbox[LIST_PATTERN],_pattern_filenames, Config::JOT_ROOT() + "/nprdata/haftone_textures/", ".png", false, true);
                           
   
   
    _checkbox[CHECK_ENABLE]=glui->add_checkbox_to_panel(
                              _panel[PANEL_PROPERTIES],
                              "Enable",
                              NULL,
                              id+CHECK_ENABLE,
                              checkbox_cb);
   _checkbox[CHECK_HIGHLIGHT]=glui->add_checkbox_to_panel(
                              _panel[PANEL_PROPERTIES],
                              "Highlight",
                              NULL,
                              id+CHECK_HIGHLIGHT,
                              checkbox_cb);

  
   _slider[SLIDER_SCALE] = glui->add_slider_to_panel(
      _panel[PANEL_PROPERTIES], "Scale", 
      GLUI_SLIDER_FLOAT, 
      1.0f/8,
      8.0,
      NULL,
      id+SLIDER_SCALE, slider_cb);
   _slider[SLIDER_SCALE]->set_w(150);
   _slider[SLIDER_SCALE]->set_num_graduations(200);

   _button[BUT_PROCEDURAL] = glui->add_button_to_panel(
                             _panel[PANEL_PROPERTIES],  "Procedural", 
                            id+BUT_PROCEDURAL, button_cb);

   
   _radgroup[RADGROUP_TC_ON_OFF] = glui->add_radiogroup_to_panel(
                                     _panel[PANEL_PROPERTIES],
                                     NULL,
                                     id+RADGROUP_TC_ON_OFF, radiogroup_cb); 
   _radbutton[RADBUT_TC_OFF] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_TC_ON_OFF],
                                   "Tone Correction OFF");
   _radbutton[RADBUT_TC_ON] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_TC_ON_OFF],
                                   "Tone Correction ON");

   _radgroup[RADGROUP_TC_MODE] = glui->add_radiogroup_to_panel(
                                     _panel[PANEL_PROPERTIES],
                                     NULL,
                                     id+RADGROUP_TC_MODE, radiogroup_cb); 
   _radbutton[RADBUT_TC_FULL] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_TC_MODE],
                                   "Full tone correction");
   _radbutton[RADBUT_TC_TRANS] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_TC_MODE],
                                   "Transition correction only");
     
   //Color Choose
   _panel[PANEL_COLOR_SWITCH] = glui->add_panel_to_panel(_panel[PANEL_PROPERTIES], "Color");
    
   _radgroup[RADGROUP_COLOR_SEL] = glui->add_radiogroup_to_panel(
                                     _panel[PANEL_COLOR_SWITCH],
                                     NULL,
                                     id+RADGROUP_COLOR_SEL, radiogroup_cb);
   
   _radbutton[RADBUT_STROKE_COL] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_COLOR_SEL],
                                   "Stroke");
     
   _radbutton[RADBUT_BASE_COL] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_COLOR_SEL],
                                   "Base Color");
   _radbutton[RADBUT_BACKGROUND] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_COLOR_SEL],
                                   "Background");
   

   glui->add_column_to_panel(_panel[PANEL_PROPERTIES],true);
   
   _tone_shader_ui->build(glui,_panel[PANEL_PROPERTIES], true);

   _color_ui->build(glui,_rollout[ROLLOUT_MAIN], false);

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
HalftoneUI::update_non_lives()
{
   toggle_enable_me();
   _texture_selection_ui->update_non_lives();
   _texture_selection_ui->fill_my_texture_listbox<Halftone_TX>();
   _current_halftone = GUI::get_current_texture<Halftone_TX>();
   _color_ui->update_non_lives();
   _tone_shader_ui->update_non_lives();
   _presets_ui->update_non_lives();

   if(!_current_halftone || !_texture_selection_ui)
      return;   
   
   halftone_layer_t* layer = (halftone_layer_t*)_current_halftone->get_layer(_texture_selection_ui->get_layer_num());
   if(!layer)
      return;

   if(_parent->class_name() == ProxyPenUI::static_name() )
      ((ProxyPenUI*)_parent)->get_light_ui()->set_current_light(layer->_channel);

   _texture_selection_ui->set_channel_num(layer->_channel);

   _checkbox[CHECK_ENABLE]->set_int_val(layer->_mode);
   _checkbox[CHECK_HIGHLIGHT]->set_int_val(layer->_highlight);
   _slider[SLIDER_SCALE]->set_float_val(layer->_pattern_scale/_current_halftone->get_abs_scale(_texture_selection_ui->get_layer_num()));
 
   _radgroup[RADGROUP_TC_ON_OFF]->set_int_val(layer->_use_tone_correction);
   _radgroup[RADGROUP_TC_MODE]->set_int_val(layer->_use_lod_only);

   if(_color_sel == 0){
      _color_ui->set_current_color(layer->_ink_color[0],layer->_ink_color[1], layer->_ink_color[2], true, true);
   } else if(_color_sel == 1){
      COLOR c = _current_halftone->get_base_color();
      _color_ui->set_current_color(c[0],c[1],c[2], true, true);
   } else {
      COLOR c = VIEW::peek()->color();
      _color_ui->set_current_color(c[0],c[1],c[2], true, true);
   }

   if((layer->_mode == 1) && (!layer->_pattern_name))
      _current_halftone->set_procedural_pattern(0, 0);  //Set defaults if the layers is not set up...
   
  
   if(layer->_pattern_name){
      GLUI_Listbox_Item* pattern_item = _listbox[LIST_PATTERN]->get_item_ptr(**(layer->_pattern_name));
      if(pattern_item)
         _listbox[LIST_PATTERN]->set_int_val(pattern_item->id);
   } 
}

void
HalftoneUI::toggle_enable_me()
{
   if(GUI::exists_in_the_world<Halftone_TX>()){
      _rollout[ROLLOUT_MAIN]->enable();
      if(GUI::get_current_texture<Halftone_TX>()){
         _rollout[ROLLOUT_MAIN]->open();
      }else {        
         _rollout[ROLLOUT_MAIN]->close();
      }
   } else {
      _rollout[ROLLOUT_MAIN]->close();
      _rollout[ROLLOUT_MAIN]->disable();        
   }  
}


void
HalftoneUI::listbox_cb(int id)
{
   switch(id&ID_MASK)
   {
      case LIST_PATTERN: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_PATTERN_TEXTURE, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;      
   }
    _ui[id >> ID_SHIFT]->update();
}

void
HalftoneUI::button_cb(int id)
{
   switch(id&ID_MASK)
   {
      case LIST_PATTERN: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_PROCEDURAL, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;      
   }
}

void
HalftoneUI::slider_cb(int id)
{
   switch(id&ID_MASK)
   {     
      case SLIDER_SCALE:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_SCALE, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;   
   }
}

void 
HalftoneUI::spinner_cb(int id)
{  
   
}

void
HalftoneUI::radiogroup_cb(int id)
{   
   switch(id&ID_MASK)
   {
      case RADGROUP_COLOR_SEL:
         _ui[id >> ID_SHIFT]->_color_sel = _ui[id >> ID_SHIFT]->_radgroup[RADGROUP_COLOR_SEL]->get_int_val();
         _ui[id >> ID_SHIFT]->update();
         break;
      case RADGROUP_TC_ON_OFF:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_TC_ON_OFF, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;   
      case RADGROUP_TC_MODE:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_TC_MODE, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;   
   }
}

void
HalftoneUI::checkbox_cb(int id)
{
   switch(id&ID_MASK)
   {
      case CHECK_ENABLE: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_ENABLE, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         _ui[id >> ID_SHIFT]->update();
         break;
      case CHECK_HIGHLIGHT:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_HIGHLIGHT, 
                                                      _ui[id >> ID_SHIFT]->_current_halftone,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
   }
}

void
HalftoneUI::rotation_cb(int id)
{    
  
}

bool
HalftoneUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;  
   if(sender->class_name() == PatchSelectionUI::static_name()){
      switch(event)
      {
      case PatchSelectionUI::SELECT_FILL_PATCHES:
         _texture_selection_ui->fill_my_texture_listbox<Halftone_TX>();
         break;
      case PatchSelectionUI::SELECT_PATCH_SELECTED:
         if(_parent)
            _parent->update();
         else
            update();
         break;
      case PatchSelectionUI::SELECT_LAYER_CHENGED:
         if(_parent)
            _parent->update();
         else
            update();
         break;
      case PatchSelectionUI::SELECT_CHANNEL_CHENGED:
         apply_changes_to_texture(OP_CHANNEL, 
                                  _current_halftone, 
                                  _texture_selection_ui->get_layer_num());
         update();
         break;     
      case PatchSelectionUI::SELECT_APPLY_CHANGE:
         apply_changes();
         break;
      } 
   }    
   if(sender->class_name() == ColorUI::static_name()){
      apply_changes_to_texture(OP_COLOR, _current_halftone, _texture_selection_ui->get_layer_num());
   }
   if(sender->class_name() == PresetsUI::static_name()){
      switch(event)
      {
      case PresetsUI::PRESET_SELECTED:
         s = load_preset();
         if(_parent)
            _parent->update();
         else
            update();
         break;
      case PresetsUI::PRESET_SAVE:      
      case PresetsUI::PRESET_SAVE_NEW:      
         s = save_preset();
         break;   
      } 
   }     
   if(sender->class_name() == ToneShaderUI::static_name()){
       _last_op = OP_TONE_SHADER;
   }     
   
   return s;
}

void 
HalftoneUI::apply_changes()
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
      apply_changes_to_texture(my_op, _current_halftone, current_layer);
      return;
   }
      
   int times = (all_layers) ? MAX_LAYERS : 1;
   for(int i=0; i < times; ++i)
   {
      if(all_meshes && whole_mesh){
            for(int j=0; j < patches.num(); ++j){
               Halftone_TX* tex = GUI::get_current_texture<Halftone_TX>(patches[j]);
               int curr_l = (all_layers) ? i : current_layer;
               if(!last_operation){
                     _texture_selection_ui->set_layer_num(i);
                     if(_parent)
                        _parent->update();
                     else
                        update();                     
                  }                 
               // Do it only if the layer is not disabled
               if(tex && tex->get_layer(curr_l)->_mode != 0)
                  apply_changes_to_texture(my_op, tex, curr_l); 
            }
      }else if (whole_mesh){
            // Apply to patches only within the selected model
            for(int j=0; j < patches.num(); ++j){
               if(BMESH::is_focus(patches[j]->mesh()))
               {
                  Halftone_TX* tex = GUI::get_current_texture<Halftone_TX>(patches[j]);
                  int curr_l = (all_layers) ? i : current_layer;
                  if(!last_operation){
                     _texture_selection_ui->set_layer_num(i);
                     if(_parent)
                        _parent->update();
                     else
                        update();                     
                  }                 
                  // Do it only if the layer is not disabled
                  if(tex && tex->get_layer(curr_l)->_mode != 0)
                     apply_changes_to_texture(my_op, tex, curr_l); 
               }
            }

      }else if(!all_meshes && !whole_mesh){
         //Apply changes to all layers
         if(_current_halftone && _current_halftone->get_layer(i)->_mode != 0)
            apply_changes_to_texture(my_op,_current_halftone,i);      
      }
   }
   
   
}

void
HalftoneUI::apply_changes_to_texture(operation_id_t op, Halftone_TX* tex, int layer)
{
   if (!tex)
   {
      cerr << "Unable to apply this operation to the texture because the tex is null" << endl;
      return;
   }


   if((op==OP_ALL) || (op==OP_COLOR)){
      COLOR c = _color_ui->get_color();
      if(_color_sel == 0)
         tex->get_layer(layer)->_ink_color = c;
      else if(_color_sel == 1)
          tex->set_base_color(c);
      else
         VIEW::peek()->set_color(c);
   }
   if((op==OP_ALL) || (op==OP_ENABLE)){
      tex->get_layer(layer)->_mode = (_checkbox[CHECK_ENABLE]->get_int_val() == 1);
      if(tex->get_layer(layer)->_mode)
         tex->set_procedural_pattern(layer,0);
   }
   if((op==OP_ALL) || (op==OP_HIGHLIGHT)){
      tex->get_layer(layer)->_highlight = (_checkbox[CHECK_HIGHLIGHT]->get_int_val()==1);      
   }
   
   if((op==OP_ALL) || (op==OP_SCALE)){
      tex->get_layer(layer)->_pattern_scale = _slider[SLIDER_SCALE]->get_float_val() * tex->get_abs_scale(layer);  
   }
   if((op==OP_ALL) || (op==OP_PROCEDURAL)){
      tex->set_procedural_pattern(layer, 0);
   }
   if((op==OP_ALL) || (op==OP_PATTERN_TEXTURE)){

       if (_listbox[LIST_PATTERN]->get_int_val() == 0) //default pattern = procedural dots
       {
          tex->set_procedural_pattern(layer,0);
       }
       else 
       {
         if(tex->get_layer(layer)->_mode){
          str_ptr pattern = (char*)_listbox[LIST_PATTERN]->curr_text;
          tex->set_texture_pattern(layer, pattern);
         }
       }
   }
   if((op==OP_ALL) || (op==OP_TC_ON_OFF)){
      tex->toggle_tone_correction(layer);
   }
   if((op==OP_ALL) || (op==OP_TC_MODE)){
      tex->toggle_lod_only_correction(layer);
   }  
   if((op==OP_ALL) || (op==OP_TONE_SHADER)){
        _tone_shader_ui->apply_changes_to_texture_parent((int)op, tex->get_tone_shader(), layer);
   } 
   if((op==OP_ALL) || (op==OP_CHANNEL)){
        tex->get_layer(layer)->_channel = _texture_selection_ui->get_channel_num();  
   }
   _last_op = op;
}

bool
HalftoneUI::load_preset()
{   
  
   char* f = **(_presets_ui->get_filename());
   if(!f){
      err_msg("HalftoneUI::save_preset - file not specified");
      return false;
   }

   ifstream fin(f, ifstream::in);

   if (!fin)    {
      err_msg("HalftoneUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }
   str_ptr str;

   STDdstream s(&fin);
   s >> str;
   if (str != Halftone_TX::static_name()) 
   {
      err_msg("HalftoneUI::load_preset() - Not 'Halftone_TX': '%s'!!", **str);
   } else {
      if(!_current_halftone){
         err_msg("HalftoneUI::load_preset() - Error! there is no halftone to populate");
         return false; 
      }
      _current_halftone->decode(s);      
   }
   fin.close();
   return true;
 
}

bool         
HalftoneUI::save_preset()
{ 
   
   cerr << "HalftoneUI::save_preset()" << endl;
   char* f = **(_presets_ui->get_filename());
   if(!f){
      err_msg("HalftoneUI::save_preset - file not specified");
      return false;
   }
   fstream fout;
   fout.open(f,ios::out);
   //If this fails, then dump the null string
   if (!fout) 
   {
      err_msg("HalftoneUI::save_preset -  Could not open: '%s'!", f);   
      return false;
   }
   //Otherwise, do the right thing
   else
   {      
      STDdstream stream(&fout);
      if(!_current_halftone){
         err_msg("HalftoneUI::load_preset() - Error! there is no halftone to save");
         return false; 
      }
      _current_halftone->format(stream);
   }  
   return true;

}


// hatching_ui.C
