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

#include "ref_image_ui.H"
#include "npr/npr_view.H"
#include "gtex/halo_ref_image.H"

#include "wnpr/sky_box.H"
#include "npr/skybox_texture.H"

using namespace mlib;

#define ID_SHIFT                     10
#define ID_MASK                      ((1<<ID_SHIFT)-1)



vector<RefImageUI*>         RefImageUI::_ui;



RefImageUI::RefImageUI(BaseUI* parent):    BaseUI(parent, "RefImageUI")
{

   _ui.push_back(this);
   _id = (_ui.size()-1);  
}

void 
RefImageUI::build(GLUI* glui, GLUI_Panel*  base, bool open)
{
   _glui = glui; 
   int id = _id << ID_SHIFT;

  _rollout[ROLLOUT_MAIN] = (base) ? glui->add_rollout_to_panel(base, "Ref Images",open)
                                   : glui->add_rollout("RefImageUI",open);  



  _radgroup[RADGROUP_REF_IMAGES] = glui->add_radiogroup_to_panel(
                                     _rollout[ROLLOUT_MAIN],
                                     NULL,
                                     id+RADGROUP_REF_IMAGES, radiogroup_cb);


   _radbutton[RADBUT_MAIN] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_REF_IMAGES],
                                   "regular");

   _radbutton[RADBUT_TONE0] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_REF_IMAGES],
                                   "tone_0");

   _radbutton[RADBUT_TONE1] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_REF_IMAGES],
                                   "tone_1");

   _radbutton[RADBUT_HALO] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_REF_IMAGES],
                                   "halo");
  
    glui->add_column_to_panel(_rollout[ROLLOUT_MAIN],true);

   _radgroup[RADGROUP_REF_IMAGES]->set_int_val(0);


   _slider[SLIDER_HALO] = glui->add_slider_to_panel(_rollout[ROLLOUT_MAIN],
                                                      "Halo kernel size",1,
                                                      0,50,0,id+SLIDER_HALO,slider_cb);

   _slider[SLIDER_HALO]->set_int_val(HaloRefImage::lookup()->get_kernel_size());



   _listbox[LISTBOX_SKY] = glui->add_listbox_to_panel(_rollout[ROLLOUT_MAIN],
                                                      "Sky",
                                                      0,id+LISTBOX_SKY,
                                                      listbox_cb);

    fill_directory_listbox(_listbox[LISTBOX_SKY],_sky_filenames, Config::JOT_ROOT() + "/nprdata/sky_textures/", ".png", false);


   _checkbox[CHECKBOX_SKY] = glui->add_checkbox_to_panel(_rollout[ROLLOUT_MAIN],
                                                         "draw skybox",
                                                         0,id+CHECKBOX_SKY,
                                                         checkbox_cb);
   _checkbox[CHECKBOX_SKY]->set_int_val(0);
                                                         

  // Cleanup sizes
   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }

}


void  
RefImageUI::radiogroup_cb(int id)
{
   switch(id&ID_MASK)
   {
   case RADGROUP_REF_IMAGES:
      int choice = _ui[id >> ID_SHIFT]->_radgroup[RADGROUP_REF_IMAGES]->get_int_val();

      //do something to change current image
 
      switch(choice)
      {
      case 0 :
         NPRview::_draw_flag = NPRview::SHOW_NONE;
         WORLD::message("Main output");
         break;
      
      case 1 :
         NPRview::_draw_flag = NPRview::SHOW_TEX_MEM0;
         WORLD::message("Tone Image #0");
         break;

      case 2 :
         NPRview::_draw_flag = NPRview::SHOW_TEX_MEM1;
         WORLD::message("Tone Image #1");
         break;

     case 3 :
         NPRview::_draw_flag = NPRview::SHOW_HALO;
         WORLD::message("Halo Image");
         break;
      }
     
      break;
   }
}

void
RefImageUI::slider_cb(int id)
{
   if ((id&ID_MASK)==SLIDER_HALO)
      HaloRefImage::lookup()->set_kernel_size( _ui[0]->_slider[SLIDER_HALO]->get_int_val());
}

void 
RefImageUI::listbox_cb(int id)
{
  if ((id&ID_MASK)==LISTBOX_SKY)
  {
     Skybox_Texture* tex = get_tex<Skybox_Texture>(SKY_BOX::lookup()->get_patch());
     assert(tex);
     str_ptr file =  Config::JOT_ROOT() + "/nprdata/sky_textures/" + _ui[0]->_sky_filenames[_ui[0]->_listbox[LISTBOX_SKY]->get_int_val()];
     cout << "Loading : " << file << endl;
     tex->load_texture( file );
  }
}

void 
RefImageUI::checkbox_cb(int id)
{
   if ((id&ID_MASK)==CHECKBOX_SKY)
   {
      if (_ui[0]->_checkbox[CHECKBOX_SKY]->get_int_val())
      {
         assert(SKY_BOX::lookup());
         SKY_BOX::lookup()->show();
      }
      else
      {
         assert(SKY_BOX::lookup());
         SKY_BOX::lookup()->hide();
      }
   }
}

