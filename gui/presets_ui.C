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
// PresetsUI
////////////////////////////////////////////

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "presets_ui.H"

#include <vector>
using namespace mlib;

#define ID_SHIFT                     10
#define ID_MASK                      ((1<<ID_SHIFT)-1)

/*****************************************************************
 * PresetsUI
 *****************************************************************/
vector<PresetsUI*>         PresetsUI::_ui;
static int MY_WIDTH = 100;

PresetsUI::PresetsUI(BaseUI* parent, str_ptr dir, str_ptr ext) :
     BaseUI(parent, "PresetsUI"),
     _directory(dir),
     _extension(ext)
{
   _ui.push_back(this);
   _id = (_ui.size()-1);  
}

void     
PresetsUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
    _glui = glui; 
   int id = _id << ID_SHIFT;
      
   _panel[PANEL_PRESET] = (base) ? glui->add_panel_to_panel(base,"Presets")
                                 : glui->add_panel("Presets");

   assert(_panel[PANEL_PRESET]);           

   //Preset list
   _listbox[LIST_PRESET] = glui->add_listbox_to_panel(
      _panel[PANEL_PRESET], 
      "", NULL, 
      id+LIST_PRESET, listbox_cb);
   assert(_listbox[LIST_PRESET]);
   _listbox[LIST_PRESET]->set_w(MY_WIDTH);   
   fill_directory_listbox(_listbox[LIST_PRESET], _preset_filenames, Config::JOT_ROOT() + _directory, _extension, false, true, "-=NEW=-");

   glui->add_separator_to_panel(_panel[PANEL_PRESET]);

   //New preset name box
   _edittext[EDITTEXT_SAVE] = glui->add_edittext_to_panel(
      _panel[PANEL_PRESET], "", 
      GLUI_EDITTEXT_TEXT, NULL, 
      id+LIST_PRESET, edittext_cb);

   assert(_edittext[EDITTEXT_SAVE]);
   _edittext[EDITTEXT_SAVE]->disable();
   _edittext[EDITTEXT_SAVE]->set_w(MY_WIDTH);

   glui->add_separator_to_panel(_panel[PANEL_PRESET]);

   //Preset save button
   _button[BUT_SAVE] = glui->add_button_to_panel(
      _panel[PANEL_PRESET], "Save", 
      id+BUT_SAVE, button_cb);
   assert(_button[BUT_SAVE]);
   _button[BUT_SAVE]->set_w(MY_WIDTH-6);

  
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   
}

void
PresetsUI::update_non_lives()
{   
   if(_edittext[EDITTEXT_SAVE])
      _edittext[EDITTEXT_SAVE]->disable();
   
}

void
PresetsUI::button_cb(int id)
{
   switch(id&ID_MASK)
   {
       case BUT_SAVE:
         _ui[id >> ID_SHIFT]->preset_save_button();
       break;
   }
}



void  
PresetsUI::listbox_cb(int id)
{
 

   switch (id&ID_MASK)
   {       
       case LIST_PRESET:
         _ui[id >> ID_SHIFT]->preset_selected();
         break;
   }
}

void
PresetsUI::edittext_cb(int id)
{
   switch(id&ID_MASK)
   {
       case EDITTEXT_SAVE:
         _ui[id >> ID_SHIFT]->preset_save_text();
       break;
   }

}

void
PresetsUI::preset_selected()
{

   int val = _listbox[LIST_PRESET]->get_int_val();

   //if (val == 0)
   //{
   //   _button[BUT_SAVE]->disable();
   //}
   if(val != 0)
   {
      str_ptr filename = (char*)_listbox[LIST_PRESET]->curr_text;
      _filename = (Config::JOT_ROOT() + _directory + filename);
      _parent->child_callback(this, PRESET_SELECTED);
    //  if(!_parent->child_callback(this, PRESET_SELECTED))
    //     return;
   //   _button[BUT_SAVE]->enable();      
   }
        
}

void
PresetsUI::preset_save_button()
{

   int val = _listbox[LIST_PRESET]->get_int_val();
   if (val == 0)
   {
      //Enter save mode in which only the filename box is available.
      _glui->disable();
      _edittext[EDITTEXT_SAVE]->enable();
      WORLD::message("Enter preset name in box and hit <ENTER>");
   }
   else
   {
      str_ptr filename = (char*)_listbox[LIST_PRESET]->curr_text;
      _filename = (Config::JOT_ROOT() + _directory + filename);//_preset_filenames[val-1]);
      _parent->child_callback(this, PRESET_SAVE);
      _edittext[EDITTEXT_SAVE]->disable();
      //   return;
      //_button[BUT_SAVE]->disable();
      
   }
}

void
PresetsUI::preset_save_text()
{
   int i,j;
   char *goodchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_";

   char *origtext = _edittext[EDITTEXT_SAVE]->get_text();
   int origlen = strlen(origtext);

   char *newtext;

   bool fix;

   //Here we replace bad chars with _'s,
   //and truncate to fit in the listbox
   //If no adjustments occur, we go ahead
   //and save and add the new preset.

   if (origlen == 0)
      {
         //TELL
         return;
      }


   newtext = new char[origlen+1]; 
   assert(newtext);
   strcpy(newtext,origtext);

   fix = false;
   for (i=0; i<origlen; i++)
      {
         bool good = false;
         for (j=0; j<(int)strlen(goodchars); j++)
            {
               if (newtext[i]==goodchars[j])
                  {
                     good = true;
                  }
            }
         if (!good)
            {
               newtext[i] = '_';
               fix = true;
            }
      }

   if (fix) 
      {
         WORLD::message("Replaced bad characters with '_'. Continue editing.");
         _edittext[EDITTEXT_SAVE]->set_text(newtext);
         delete(newtext);
         return;
      }

   fix = false;
   while (!_listbox[LIST_PRESET]->check_item_fit(newtext) && strlen(newtext)>0)
      {
         fix = true;
         newtext[strlen(newtext)-1]=0;
      }

   if (fix) 
      {
         WORLD::message("Truncated name to fit listbox. Continue editing.");
         _edittext[EDITTEXT_SAVE]->set_text(newtext);
         delete(newtext);
         return;
      }

  
   _filename = (Config::JOT_ROOT() + _directory + newtext + _extension);
   if(_parent->child_callback(this, PRESET_SAVE_NEW)){
      _preset_filenames += (str_ptr(newtext) + _extension);
      
      _listbox[LIST_PRESET]->add_item(_preset_filenames.num(), **_preset_filenames[_preset_filenames.num()-1]);
      _listbox[LIST_PRESET]->set_int_val(_preset_filenames.num());
   }

   _edittext[EDITTEXT_SAVE]->set_text("");
   _glui->enable();
   _edittext[EDITTEXT_SAVE]->disable();
   //_button[BUT_SAVE]->disable();


}

// presets_ui.C
