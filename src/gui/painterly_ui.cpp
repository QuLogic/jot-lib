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
// PainterlyUI
////////////////////////////////////////////

#include "std/support.H"
#include <GL/glew.h>
#include <fstream>

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui_jot.H"
#include "std/config.H"

#include "proxy_pattern/proxy_pen_ui.H"
#include "painterly_ui.H"
#include "color_ui.H"
#include "light_ui.H"

#include "presets_ui.H"
#include "tone_shader_ui.H"
#include "gui.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)


/*****************************************************************
 * PainterlyUI
 *****************************************************************/

vector<PainterlyUI*> PainterlyUI::_ui;

const static int MAX_LAYERS   = 4;


PainterlyUI::PainterlyUI(BaseUI* parent) :
     BaseUI(parent,"Haching UI"),
        _color_sel(0)
{
   _ui.push_back(this);
   _id = (_ui.size()-1); 
   _color_ui = new ColorUI(this);
   _presets_ui = new PresetsUI(this, "nprdata/painterly_presets/", ".paint");
   _texture_selection_ui = new PatchSelectionUI(this, true);
   _tone_shader_ui = new ToneShaderUI(this);
   _current_painterly = nullptr;
}


void     
PainterlyUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

    _rollout[ROLLOUT_HATCHING] = (base) ? new GLUI_Rollout(base, "Painterly UI", open)
                                    : new GLUI_Rollout(glui, "Painterly UI", open);
   
    
   _panel[PANEL_MAIN] = new GLUI_Panel(_rollout[ROLLOUT_HATCHING], "");
   
  
   _texture_selection_ui->build(glui, _panel[PANEL_MAIN], true);
   
   new GLUI_Column(_panel[PANEL_MAIN], true);

   _presets_ui->build(glui,_panel[PANEL_MAIN], true); 

   // Proparties   
   _panel[PANEL_PROPERTIES] = new GLUI_Panel(_rollout[ROLLOUT_HATCHING], "");

   _listbox[LIST_PATTERN] = new GLUI_Listbox(
      _panel[PANEL_PROPERTIES], 
      "Pattern", nullptr,
      id+LIST_PATTERN, listbox_cb);
   
   fill_directory_listbox(_listbox[LIST_PATTERN], _pattern_filenames, Config::JOT_ROOT() + "/nprdata/painterly_textures/", ".png", false);
                           
    _listbox[LIST_PAPER] = new GLUI_Listbox(
      _panel[PANEL_PROPERTIES], 
      "Paper", nullptr,
      id+LIST_PAPER, listbox_cb);
   
   fill_directory_listbox(_listbox[LIST_PAPER], _paper_filenames, Config::JOT_ROOT() + "/nprdata/paper_textures/", ".png", false);

   
    _checkbox[CHECK_ENABLE]=new GLUI_Checkbox(
                              _panel[PANEL_PROPERTIES],
                              "Enable",
                              nullptr,
                              id+CHECK_ENABLE,
                              checkbox_cb);
   _checkbox[CHECK_HIGHLIGHT]=new GLUI_Checkbox(
                              _panel[PANEL_PROPERTIES],
                              "Highlight",
                              nullptr,
                              id+CHECK_HIGHLIGHT,
                              checkbox_cb);

   _slider[SLIDER_ANGLE] = new GLUI_Slider(
      _panel[PANEL_PROPERTIES], "Angle", 
      id+SLIDER_ANGLE, slider_cb,
      GLUI_SLIDER_INT, 
      0, 360, 
      nullptr);
   _slider[SLIDER_ANGLE]->set_w(150);
   _slider[SLIDER_ANGLE]->set_num_graduations(360);

   _slider[SLIDER_SCALE] = new GLUI_Slider(
      _panel[PANEL_PROPERTIES], "Scale", 
      id+SLIDER_SCALE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      1.0,
      100.0, 
      nullptr);
   _slider[SLIDER_SCALE]->set_w(150);
   _slider[SLIDER_SCALE]->set_num_graduations(200);

   _slider[SLIDER_PAPER_CONTRAST] = new GLUI_Slider(
      _panel[PANEL_PROPERTIES], "Paper Contrast", 
      id+SLIDER_PAPER_CONTRAST, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0,
      5.0, 
      nullptr);
   _slider[SLIDER_PAPER_CONTRAST]->set_w(150);
   _slider[SLIDER_PAPER_CONTRAST]->set_num_graduations(100);

   _slider[SLIDER_PAPER_SCALE] = new GLUI_Slider(
      _panel[PANEL_PROPERTIES], "Paper Scale", 
      id+SLIDER_PAPER_SCALE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      1.0,
      100.0, 
      nullptr);
   _slider[SLIDER_PAPER_SCALE]->set_w(150);
   _slider[SLIDER_PAPER_SCALE]->set_num_graduations(100);

   _slider[SLIDER_TONE_PUSH] = new GLUI_Slider(
      _panel[PANEL_PROPERTIES], "Tone Push", 
      id+SLIDER_TONE_PUSH, slider_cb,
      GLUI_SLIDER_FLOAT, 
      -1.0,
      1.0, 
      nullptr);
   _slider[SLIDER_TONE_PUSH]->set_w(150);
   _slider[SLIDER_TONE_PUSH]->set_num_graduations(100);
   
   //Color Choose
   _panel[PANEL_COLOR_SWITCH] = new GLUI_Panel(_panel[PANEL_PROPERTIES], "Color");
    
   _radgroup[RADGROUP_COLOR_SEL] = new GLUI_RadioGroup(
                                     _panel[PANEL_COLOR_SWITCH],
                                     nullptr,
                                     id+RADGROUP_COLOR_SEL, radiogroup_cb);
   
   _radbutton[RADBUT_STROKE_COL] = new GLUI_RadioButton(
                                   _radgroup[RADGROUP_COLOR_SEL],
                                   "Stroke");
     
   _radbutton[RADBUT_BASE_COL] = new GLUI_RadioButton(
                                   _radgroup[RADGROUP_COLOR_SEL],
                                   "Base Color");
   _radbutton[RADBUT_BACKGROUND] = new GLUI_RadioButton(
                                   _radgroup[RADGROUP_COLOR_SEL],
                                   "Background");
   

   new GLUI_Column(_panel[PANEL_PROPERTIES], true);
   
   _tone_shader_ui->build(glui,_panel[PANEL_PROPERTIES], true);

   _color_ui->build(glui,_rollout[ROLLOUT_HATCHING], false);

   // Cleanup sizes

   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }

   update_non_lives();
}

void
PainterlyUI::update_non_lives()
{
   toggle_enable_me();
   _texture_selection_ui->update_non_lives();
   _presets_ui->update_non_lives();
   _texture_selection_ui->fill_my_texture_listbox<Painterly>();
   _current_painterly = GUI::get_current_texture<Painterly>();
   _color_ui->update_non_lives();
   _tone_shader_ui->update_non_lives();

   if(!_current_painterly || !_texture_selection_ui)
      return;
  
   layer_paint_t* layer = _current_painterly->get_layer(_texture_selection_ui->get_layer_num());
   if(!layer)
      return;

   if (_parent->class_name() == ProxyPenUI::static_name())
      ((ProxyPenUI*)_parent)->get_light_ui()->set_current_light(layer->_channel);

   _texture_selection_ui->set_channel_num(layer->_channel);


   _checkbox[CHECK_ENABLE]->set_int_val(layer->_mode);
   _checkbox[CHECK_HIGHLIGHT]->set_int_val(layer->_highlight);
   _slider[SLIDER_ANGLE]->set_int_val((int)layer->_angle);
   _slider[SLIDER_SCALE]->set_float_val(layer->_pattern_scale);
   _slider[SLIDER_PAPER_CONTRAST]->set_float_val(layer->_paper_contrast);
   _slider[SLIDER_PAPER_SCALE]->set_float_val(layer->_paper_scale);
   _slider[SLIDER_TONE_PUSH]->set_float_val(layer->_tone_push);

   if(_color_sel == 0){
      _color_ui->set_current_color(layer->_ink_color, true);
   } else if(_color_sel == 1){
      COLOR c = _current_painterly->get_base_color();
      _color_ui->set_current_color(c, true);
   } else {
      COLOR c = VIEW::peek()->color();
      _color_ui->set_current_color(c, true);
   }

   // Set defaults if layer is not set up:
   if (layer->_mode == 1)
      _current_painterly->init_default(
         _texture_selection_ui->get_layer_num()
         );
   
  
   if (!layer->_pattern_name.empty()) {
      GLUI_Listbox_Item* pattern_item =
         _listbox[LIST_PATTERN]->get_item_ptr(layer->_pattern_name.c_str());
      if(pattern_item)
         _listbox[LIST_PATTERN]->set_int_val(pattern_item->id);
   }
   if (!_current_painterly->paper_name().empty()) {
      GLUI_Listbox_Item* paper_item =
         _listbox[LIST_PAPER]->get_item_ptr(
            _current_painterly->paper_name().c_str()
            );
      if(paper_item){
         _listbox[LIST_PAPER]->set_int_val(paper_item->id);
         cerr << "paper_item->id " << paper_item->id << endl;
      }
   }
}

void
PainterlyUI::toggle_enable_me()
{
   if(GUI::exists_in_the_world<Painterly>()){
      _rollout[ROLLOUT_HATCHING]->enable();
      if(GUI::get_current_texture<Painterly>()){
         _rollout[ROLLOUT_HATCHING]->open();
      }else {        
         _rollout[ROLLOUT_HATCHING]->close();
      }
   } else {
      _rollout[ROLLOUT_HATCHING]->close();
      _rollout[ROLLOUT_HATCHING]->disable();        
   }  
}


void
PainterlyUI::listbox_cb(int id)
{
   switch(id&ID_MASK)
   {
      case LIST_PATTERN: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_PATTERN_TEXTURE, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
      case LIST_PAPER:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_PAPER_TEXTURE, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
   }
    _ui[id >> ID_SHIFT]->update();
}

void
PainterlyUI::button_cb(int id)
{
   
}

void
PainterlyUI::slider_cb(int id)
{
   switch(id&ID_MASK)
   {
      case SLIDER_ANGLE:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_ANGLE, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
      case SLIDER_SCALE:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_SCALE, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
      case SLIDER_PAPER_CONTRAST:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_PAPER_CONTRAST, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
      case SLIDER_PAPER_SCALE:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_PAPER_SCALE, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
      case SLIDER_TONE_PUSH:
          _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_TONE_PUSH, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
   }
}

void 
PainterlyUI::spinner_cb(int id)
{  
   
}

void
PainterlyUI::radiogroup_cb(int id)
{   
   switch(id&ID_MASK)
   {
      case RADGROUP_COLOR_SEL:
         _ui[id >> ID_SHIFT]->_color_sel = _ui[id >> ID_SHIFT]->_radgroup[RADGROUP_COLOR_SEL]->get_int_val();
         _ui[id >> ID_SHIFT]->update();
         break;
   }
}

void
PainterlyUI::checkbox_cb(int id)
{
   switch(id&ID_MASK)
   {
      case CHECK_ENABLE: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_ENABLE, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         _ui[id >> ID_SHIFT]->update();
         break;
      case CHECK_HIGHLIGHT:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_HIGHLIGHT, 
                                                      _ui[id >> ID_SHIFT]->_current_painterly,
                                                      _ui[id >> ID_SHIFT]->_texture_selection_ui->get_layer_num());
         break;
   }
}

void
PainterlyUI::rotation_cb(int id)
{    
  
}

bool
PainterlyUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;  
   if (sender->class_name() == PatchSelectionUI::static_name()) {
      switch(event)
      {
      case PatchSelectionUI::SELECT_FILL_PATCHES:
         _texture_selection_ui->fill_my_texture_listbox<Painterly>();
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
                                  _current_painterly, 
                                  _texture_selection_ui->get_layer_num());
         update();
         break;     
      case PatchSelectionUI::SELECT_APPLY_CHANGE:
         apply_changes();
         break;
      } 
   }    
   if (sender->class_name() == ColorUI::static_name()) {
      apply_changes_to_texture(OP_COLOR, _current_painterly, _texture_selection_ui->get_layer_num());
   }
   if (sender->class_name() == PresetsUI::static_name()) {
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
   if (sender->class_name() == ToneShaderUI::static_name()) {
       _last_op = OP_TONE_SHADER;
   }     
   
   return s;
}

void 
PainterlyUI::apply_changes()
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
   // Just this layer
   if(!whole_mesh && !all_meshes && !all_layers){
      apply_changes_to_texture(my_op, _current_painterly, current_layer);
      return;
   }
      
   int times = (all_layers) ? MAX_LAYERS : 1;
  
   for(int i=0; i < times; ++i)
   {
      if(all_meshes && whole_mesh){
            for(int j=0; j < patches.num(); ++j){
               Painterly* tex = GUI::get_current_texture<Painterly>(patches[j]);
               int curr_l = (all_layers) ? i : current_layer;
               //update the current layer so that info is good 
               if(!last_operation){
                  _texture_selection_ui->set_layer_num(i);
                  if(_parent)
                     _parent->update();
                  else
                     update();                  
               }
               if(tex){                  
                  apply_changes_to_texture(my_op, tex, curr_l); 
               }
            }
      }else if (whole_mesh){
            // Apply to patches only within the selected model
            for(int j=0; j < patches.num(); ++j){
               if(BMESH::is_focus(patches[j]->mesh()))
               {                 
                  Painterly* tex = GUI::get_current_texture<Painterly>(patches[j]);
                  int curr_l = (all_layers) ? i : current_layer;
                  //update the current layer so that info is good 
                  if(!last_operation){
                     _texture_selection_ui->set_layer_num(i);
                     if(_parent)
                        _parent->update();
                     else
                        update();                     
                  }                     
                  if(tex){                    
                     apply_changes_to_texture(my_op, tex, curr_l); 
                  }
               }
               

            }

      }else if(!all_meshes && !whole_mesh){
         //Apply changes to all layers
         if(_current_painterly)
            apply_changes_to_texture(my_op,_current_painterly,i);      
      }
   }
   
}

void
PainterlyUI::apply_changes_to_texture(operation_id_t op, Painterly* tex, int layer)
{
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
      tex->get_layer(layer)->_mode = _checkbox[CHECK_ENABLE]->get_int_val();      
   }
   if((op==OP_ALL) || (op==OP_HIGHLIGHT)){
      tex->get_layer(layer)->_highlight = (_checkbox[CHECK_HIGHLIGHT]->get_int_val()==1);      
   }
   if((op==OP_ALL) || (op==OP_ANGLE)){
      tex->get_layer(layer)->_angle = _slider[SLIDER_ANGLE]->get_int_val();  
   }
   if((op==OP_ALL) || (op==OP_SCALE)){
      tex->get_layer(layer)->_pattern_scale = _slider[SLIDER_SCALE]->get_float_val();  
   }
   if((op==OP_ALL) || (op==OP_PATTERN_TEXTURE)){
      if(tex->get_layer(layer)->_mode){
       string pattern = _listbox[LIST_PATTERN]->curr_text; //_pattern_filenames[_listbox[LIST_PATTERN]->get_int_val()];
       string paper   = _listbox[LIST_PAPER]->curr_text;//_paper_filenames[_listbox[LIST_PAPER]->get_int_val()];
       // cerr << " ----------- " << pattern << " ---------- " <<  paper << endl;
       tex->init_layer(layer, pattern, paper);
      }
   }
   if((op==OP_ALL) || (op==OP_PAPER_TEXTURE)){
      if(tex->get_layer(layer)->_mode){ 
       string pattern = _listbox[LIST_PATTERN]->curr_text; //_pattern_filenames[_listbox[LIST_PATTERN]->get_int_val()];
       string paper   = _listbox[LIST_PAPER]->curr_text;//_paper_filenames[_listbox[LIST_PAPER]->get_int_val()];
       //cerr << " ----------- " << pattern << " ---------- " <<  paper << endl;
       tex->init_layer(layer, pattern, paper);
      }
   }   
   if((op==OP_ALL) || (op==OP_PAPER_CONTRAST)){
        tex->get_layer(layer)->_paper_contrast = _slider[SLIDER_PAPER_CONTRAST]->get_float_val();  
   } 
   if((op==OP_ALL) || (op==OP_PAPER_SCALE)){
        tex->get_layer(layer)->_paper_scale = _slider[SLIDER_PAPER_SCALE]->get_float_val();  
   }    
   if((op==OP_ALL) || (op==OP_TONE_PUSH)){
         tex->get_layer(layer)->_tone_push = _slider[SLIDER_TONE_PUSH]->get_float_val();  
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
PainterlyUI::load_preset()
{   
   const char* f = _presets_ui->get_filename().c_str();
   if(!f){
      err_msg("PainterlyUI::save_preset - file not specified");
      return false;
   }

   ifstream fin(f, ifstream::in);

   if (!fin)    {
      err_msg("PainterlyUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }
   string str;

   STDdstream s(&fin);
   s >> str;
   if (str != Painterly::static_name()) {
      err_msg("PainterlyUI::load_preset() - Not 'Painterly': '%s'!!", str.c_str());
   } else {
      if(!_current_painterly){
         err_msg("PainterlyUI::load_preset() - Error! there is no painterly presets to populate");
         return false; 
      }
      _current_painterly->decode(s);      
   }
   fin.close();
   return true;
}

bool         
PainterlyUI::save_preset()
{ 
   cerr << "PainterlyUI::save_preset()" << endl;
   const char* f = _presets_ui->get_filename().c_str();
   if(!f){
      err_msg("PainterlyUI::save_preset - file not specified");
      return false;
   }
   fstream fout;
   fout.open(f,ios::out);
   //If this fails, then dump the null string
   if (!fout) 
   {
      err_msg("PainterlyUI::save_preset -  Could not open: '%s'!", f);   
      return false;
   }
   //Otherwise, do the right thing
   else
   {      
      STDdstream stream(&fout);
      if(!_current_painterly){
         err_msg("PainterlyUI::load_preset() - Error! there is no painterly presets to save");
         return false; 
      }
      _current_painterly->format(stream);
   }  
   return true;
}


// painterly_ui.C
