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
#include <GL/glew.h>

#include "glut_winsys.H" 
#include "glui_dialogs.H" 
#include "tty_glut.H"
#include "glui/glui_jot.H"
#include "std/file.H"
#include "widgets/alert_box_icon_exclaim.H"
#include "widgets/alert_box_icon_question.H"
#include "widgets/alert_box_icon_warn.H"
#include "widgets/alert_box_icon_info.H"
#include "widgets/alert_box_icon_jot.H"
#include "widgets/file_select_icon_arrow_down.H"
#include "widgets/file_select_icon_arrow_up.H"
#include "widgets/file_select_icon_scroll.H"
#include "widgets/file_select_icon_doc.H"
#include "widgets/file_select_icon_doc_x.H"
#include "widgets/file_select_icon_doc_r.H"
#include "widgets/file_select_icon_blank.H"
#include "widgets/file_select_icon_drive.H"
#include "widgets/file_select_icon_folder.H"
#include "widgets/file_select_icon_folder_r.H"
#include "widgets/file_select_icon_folder_x.H"
#include "widgets/file_select_icon_folder_up.H"
#include "widgets/file_select_icon_folder_dot.H"
#include "widgets/file_select_icon_folder_plus.H"
#include "widgets/file_select_icon_jot.H"
#include "widgets/file_select_icon_disc.H"
#include "widgets/file_select_icon_disc_save.H"
#include "widgets/file_select_icon_disc_load.H"

/*****************************************************************
 * GLUIPopUp
 *****************************************************************/

//////////////////////////////////////////////////////
// Static Variables Initialization
//////////////////////////////////////////////////////
vector<GLUIPopUp*>    GLUIPopUp::_ui;

//////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////
GLUIPopUp::GLUIPopUp(GLUT_WINSYS *w) :
   _glut_winsys(w),
   _glui(nullptr),
   _blocking(true)
{
   _ui.push_back(this);
   _id = ((int)_ui.size()-1);
}

//////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////
GLUIPopUp::~GLUIPopUp()
{
   cerr << "GLUIPopUp::~GLUIPopUp - Error!!! Destructor not implemented.\n";
}

//////////////////////////////////////////////////////
// show_glui()
//////////////////////////////////////////////////////
bool     
GLUIPopUp::show_glui(bool blocking)
{
   assert(!_glui);

   _blocking = blocking;

   if (_blocking)
   {
      GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);

      if (mgr->get_blocker())
      {
         cerr << "GLUIPopUp::show_glui() - *ERROR* Another PopUp is already blocking!\n";
         return false;
      }

      mgr->set_blocker(_glut_winsys);
   }

   build_glui(); 
   
   assert(_glui);

   _glui->show();

   return true;
}


//////////////////////////////////////////////////////
// build_glui()
//////////////////////////////////////////////////////
void     
GLUIPopUp::build_glui()
{
   int root_x, root_y, root_w, root_h;

   _glut_winsys->size(root_w,root_h);
   _glut_winsys->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("PopUp", 0, root_x + root_w/2, root_y + root_h/2);

   assert(_glui);

   _glui->set_main_gfx_window(_glut_winsys->id());

   //Sub-classes do stuff here...

}

//////////////////////////////////////////////////////
// unbuild_glui()
//////////////////////////////////////////////////////
void     
GLUIPopUp::unbuild_glui()
{
   //Sub-classes do stuff here...

   assert(_glui);

   _panel.clear(); 
   _button.clear();
   _slider.clear();
   _listbox.clear();
   _rollout.clear();
   _edittext.clear();
   _bitmapbox.clear();
   _checkbox.clear();
   _statictext.clear();
   _activetext.clear();
   _radiogroup.clear();
   _radiobutton.clear();

   _glui->set_cursor(GLUT_CURSOR_WAIT);

   _glui->close();

   _glui = nullptr;

}

//////////////////////////////////////////////////////
// hide_glui()
//////////////////////////////////////////////////////
bool
GLUIPopUp::hide_glui()
{
   assert(_glui);

   unbuild_glui();

   if (_blocking)
   {
      GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);

      assert(mgr->get_blocker());

      mgr->clear_blocker(_glut_winsys);
   }

   return true;
}

//////////////////////////////////////////////////////
// slider_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::slider_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->slider_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// button_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::button_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->button_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// activetext_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::activetext_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->activetext_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// listbox_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::listbox_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->listbox_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// edittext_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::edittext_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->edittext_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// checkbox_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::checkbox_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->checkbox_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// bitmapbox_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::bitmapbox_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->bitmapbox_cb(id&ID_MASK);
}

//////////////////////////////////////////////////////
// radiogroup_cbs()
//////////////////////////////////////////////////////
void
GLUIPopUp::radiogroup_cbs(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());
   _ui[id >> ID_SHIFT]->radiogroup_cb(id&ID_MASK);
}

/*****************************************************************
 * GLUIAlertBox
 *****************************************************************/

//////////////////////////////////////////////////////
// Static Variables Initialization
//////////////////////////////////////////////////////

bool                       GLUIAlertBox::_icon_init = false;
GLUIAlertBox::IconBitmap   GLUIAlertBox::_icons[ICON_NUM];

//////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////
GLUIAlertBox::GLUIAlertBox(GLUT_WINSYS *w) :
   GLUIPopUp(w),
      _cb(nullptr),
      _vp(nullptr),
      _vpd(nullptr),
      _idx(-1)
{
   if (!_icon_init)
   {
      _icons[JOT_ICON].set(alert_box_jot_icon);
      _icons[INFO_ICON].set(alert_box_info_icon);
      _icons[QUESTION_ICON].set(alert_box_question_icon);
      _icons[EXCLAMATION_ICON].set(alert_box_exclaim_icon);
      _icons[WARNING_ICON].set(alert_box_warn_icon);
      _icon_init = true;
   }
}

//////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////
GLUIAlertBox::~GLUIAlertBox()
{

}

//////////////////////////////////////////////////////
// is_displaying()
//////////////////////////////////////////////////////
bool 
GLUIAlertBox::is_displaying()  
{ 
   return is_showing();
}

//////////////////////////////////////////////////////
// display()
//////////////////////////////////////////////////////
bool 
GLUIAlertBox::display(bool blocking, alert_cb_t cb, void *vp, void *vpd, int idx)  
{ 

   if (is_displaying())
   {
      cerr << "GLUIAlertBox::display - *ERROR* It's already displaying!\n";
      return false;
   }

   if (_buttons.empty())
   {
      cerr << "GLUIAlertBox::display - *ERROR* No buttons defined!\n";
      return false;
   }

   assert(_cb == nullptr);
   assert(_vp == nullptr);
   assert(_vpd == nullptr);
   assert(_idx == -1);

   if (!show_glui(blocking))
   {
      cerr << "GLUIAlertBox::display - *ERROR* Couldn't show GLUI window!\n";
      return false;
   }

   assert(is_displaying());

   _cb = cb; 
   _vp = vp; 
   _vpd = vpd; 
   _idx = idx;

   return true; 
}

//////////////////////////////////////////////////////
// undisplay()
//////////////////////////////////////////////////////
bool
GLUIAlertBox::undisplay(int button) 
{ 
   assert(is_displaying());
   
   if (!hide_glui())
   {
      cerr << "GLUIAlertBox::undisplay - *ERROR* Couldn't hide GLUI window!\n";
      return false;
   }
  
   assert(!is_displaying());

   clear_title();
   clear_icon();
   clear_default();
   clear_buttons();
   clear_text();

   alert_cb_t   cb  = _cb;
   void        *vp  = _vp;
   void        *vpd = _vpd;
   int         idx  = _idx;

   _cb  = nullptr;
   _vp  = nullptr;
   _vpd = nullptr;
   _idx = -1;

   if (cb)
   {
      cb(vp, vpd, idx, button);
   }

   return true;
}

//////////////////////////////////////////////////////
// build_glui()
//////////////////////////////////////////////////////
void     
GLUIAlertBox::build_glui()
{
   GLUIPopUp::build_glui();

   vector<string>::size_type j;

   int id = _id << ID_SHIFT;

   _glui->rename(_title);

   assert(_panel.empty());        _panel.resize(PANEL_NUM, nullptr);
   assert(_bitmapbox.empty());    _bitmapbox.resize(BITMAPBOX_NUM, nullptr);
   assert(_button.empty());       _button.resize(_buttons.size(), nullptr);
   assert(_statictext.empty());   _statictext.resize(_text.size() ? _text.size() : 1, nullptr);
   
   if ((_icon != NO_ICON) || !_text.empty())
   {
      _panel[PANEL_TEXT] = new GLUI_Panel(_glui, "", GLUI_PANEL_RAISED);
      assert(_panel[PANEL_TEXT]);           

      if (_icon != NO_ICON)
      {
         _bitmapbox[BITMAPBOX_ICON] = new GLUI_BitmapBox(
            _panel[PANEL_TEXT], "",
            id+BITMAPBOX_ICON, bitmapbox_cbs, false);
         assert(_bitmapbox[BITMAPBOX_ICON]);
      
         _bitmapbox[BITMAPBOX_ICON]->set_border(0);
         _bitmapbox[BITMAPBOX_ICON]->set_margin(4);
         _bitmapbox[BITMAPBOX_ICON]->set_img_size(_icons[_icon]._width,_icons[_icon]._height);
         _bitmapbox[BITMAPBOX_ICON]->copy_img(_icons[_icon]._data,_icons[_icon]._width,_icons[_icon]._height,3);
      }

      new GLUI_Column(_panel[PANEL_TEXT], false);

      if (_text.empty()) {
         _statictext[0] = new GLUI_StaticText(_panel[PANEL_TEXT], " ");
         assert(_statictext[0]);
         _statictext[0]->set_alignment(GLUI_ALIGN_LEFT);
         _statictext[0]->set_h(16);

      } else {
         for (j=0; j<_text.size(); j++) {
            _statictext[j] = new GLUI_StaticText(_panel[PANEL_TEXT], _text[j].c_str());
            assert(_statictext[j]);
            _statictext[j]->set_alignment(GLUI_ALIGN_LEFT);
            _statictext[j]->set_h(16);
         }
      }

      new GLUI_Column(_panel[PANEL_TEXT], false);
   }

   _panel[PANEL_BUTTONS] = new GLUI_Panel(_glui, "", GLUI_PANEL_NONE);
   assert(_panel[PANEL_BUTTONS]);           

   for (j=0; j<_buttons.size(); j++)
   {
      _button[j] = new GLUI_Button(
         _panel[PANEL_BUTTONS], _buttons[j].c_str(),
         id+j, button_cbs);
      assert(_button[j]);
      
      _button[j]->set_w(75);

      if (j != _buttons.size()-1)
         new GLUI_Column(_panel[PANEL_BUTTONS], false);
   }
   
   _panel[PANEL_BUTTONS]->set_alignment(GLUI_ALIGN_RIGHT);

   //Fix up sizes...
   int delta; 
   
   if (_panel[PANEL_TEXT] && ((delta = _panel[PANEL_BUTTONS]->get_w() - _panel[PANEL_TEXT]->get_w()) > 0))
   {
      _statictext[0]->set_w(_statictext[0]->get_w() + delta);
   }

   //Center the window

   int root_x, root_y, root_w, root_h, x, y;

   _glut_winsys->size(root_w,root_h);
   _glut_winsys->position(root_x,root_y);

   x = root_x + (root_w - _glui->get_w())/2;
   y = root_y + (root_h - _glui->get_h())/2;

   _glui->reposition(max(x,root_x),max(y,root_y));

   //Set default button
   if (_default >= 0)
   {
      assert(_default < (int)_button.size());
      _glui->activate_control(_button[_default],GLUI_ACTIVATE_TAB);
   }
}

//////////////////////////////////////////////////////
// unbuild_glui()
//////////////////////////////////////////////////////
void     
GLUIAlertBox::unbuild_glui()
{
   //Clean up...
   
   GLUIPopUp::unbuild_glui();
}

//////////////////////////////////////////////////////
// button_cb()
//////////////////////////////////////////////////////
void
GLUIAlertBox::button_cb(int id)
{
   undisplay(id);
}

   
/*****************************************************************
 * GLUIFileSelect
 *****************************************************************/

#define GLUI_FILE_SELECT_NUM_FILES        10
#define GLUI_FILE_SELECT_NUM_RECENT       5
#define GLUI_FILE_SELECT_PATH_WIDTH       330
#define GLUI_FILE_SELECT_NAME_WIDTH       260
#define GLUI_FILE_SELECT_SIZE_WIDTH       85
#define GLUI_FILE_SELECT_DATE_WIDTH       135
//#define GLUI_FILE_SELECT_FILTER_WIDTH     455
#define GLUI_FILE_SELECT_FILTER_WIDTH     425
#define GLUI_FILE_SELECT_ACTION_WIDTH     85
#define GLUI_FILE_SELECT_HEADING_GAP      4
#define GLUI_FILE_SELECT_SCROLL_MIN       20
#define GLUI_FILE_SELECT_DOUBLECLICK_TIME 0.65

//////////////////////////////////////////////////////
// Static Variables Initialization
//////////////////////////////////////////////////////

bool                       GLUIFileSelect::_icon_init = false;
GLUIFileSelect::IconBitmap GLUIFileSelect::_icons[ICON_NUM];

bool                       GLUIFileSelect::_bitmap_init = false;
GLUIFileSelect::IconBitmap GLUIFileSelect::_bitmaps[BITMAP_NUM];

//////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////
GLUIFileSelect::GLUIFileSelect(GLUT_WINSYS *w) :
   GLUIPopUp(w),
      _cb(nullptr),
      _vp(nullptr),
      _idx(-1),
      _current_path(nullptr),
      _current_mode(MODE_NORMAL),
      _current_sort(SORT_NAME_UP),
      _current_selection(-1),
      _current_selection_time(0.0),
      _current_scroll(0),
      _current_scrollbar_wheel(false),
      _current_scrollbar_wheel_position(-1),
      _current_scrollbar_wheel_index(-1),
      _current_scrollbar_state(BAR_STATE_NONE),
      _current_scrollbar_state_inside(false),
      _current_scrollbar_state_pixel_position(0),
      _current_scrollbar_state_index_position(0),
      _current_scrollbar_state_above_ratio(0.0),
      _current_scrollbar_state_below_ratio(0.0)
{
   if (!_icon_init)
   {
      _icons[LOAD_ICON].set(file_select_disc_load_icon);
      _icons[SAVE_ICON].set(file_select_disc_save_icon);
      _icons[DISC_ICON].set(file_select_disc_icon);
      _icons[JOT_ICON].set(file_select_jot_icon);
      _icon_init = true;
   }
   if (!_bitmap_init)
   {
      _bitmaps[BITMAP_X].set(file_select_doc_x_icon);
      _bitmaps[BITMAP_R].set(file_select_doc_r_icon);
      _bitmaps[BITMAP_UP].set(file_select_folder_up_icon);
      _bitmaps[BITMAP_DOT].set(file_select_folder_dot_icon);
      _bitmaps[BITMAP_PLUS].set(file_select_folder_plus_icon);
      _bitmaps[BITMAP_UPARROW].set(file_select_arrow_up_icon);
      _bitmaps[BITMAP_DOWNARROW].set(file_select_arrow_down_icon);
      _bitmaps[BITMAP_SCROLL].set(file_select_scroll_icon);
      _bitmaps[BITMAP_DRIVE].set(file_select_drive_icon);
      _bitmaps[BITMAP_FOLDER].set(file_select_folder_icon);
      _bitmaps[BITMAP_DOC].set(file_select_doc_icon);
      _bitmaps[BITMAP_BLANK].set(file_select_blank_icon);
      _bitmap_init = true;
   }
}

//////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////
GLUIFileSelect::~GLUIFileSelect()
{

}

//////////////////////////////////////////////////////
// is_displaying()
//////////////////////////////////////////////////////
bool 
GLUIFileSelect::is_displaying()  
{ 
   return is_showing(); 
}

//////////////////////////////////////////////////////
// display()
//////////////////////////////////////////////////////
bool 
GLUIFileSelect::display(bool blocking, file_cb_t cb, void *vp, int idx) 
{ 
   if (is_displaying())
   {
      cerr << "GLUIFileSelect::display - *ERROR* It's already displaying!\n";
      return false;
   }

   assert(_cb == nullptr);
   assert(_vp == nullptr);
   assert(_idx == -1);

   if (!show_glui(blocking))
   {
      cerr << "GLUIFileSelect::display - *ERROR* Couldn't show GLUI window!\n";
      return false;
   }

   assert(is_displaying());

   _cb = cb; 
   _vp = vp; 
   _idx = idx;

   return true; 
}

//////////////////////////////////////////////////////
// undisplay()
//////////////////////////////////////////////////////
bool
GLUIFileSelect::undisplay(int button, string path, string file)
{ 
   assert(is_displaying());
   
   if (!hide_glui())
   {
      cerr << "GLUIFileSelect::undisplay - *ERROR* Couldn't hide GLUI window!\n";
      return false;
   }
  
   assert(!is_displaying());

   _path = path;
   _file = file;

   if (button == OK_ACTION)
   {
      pair<set<string>::iterator,bool> result = _current_recent_path_set.insert(_path);
      if (result.second) {
         _current_recent_paths.push_back(_path);
         while (_current_recent_paths.size() > GLUI_FILE_SELECT_NUM_RECENT) {
            _current_recent_path_set.erase(_current_recent_paths[0]);
            _current_recent_paths.erase(_current_recent_paths.begin());
         }
      }
   }

   clear_title();
   clear_action();
   clear_icon();
   //clear_path();
   clear_file();
   clear_filter();
   //clear_filters();

   file_cb_t    cb = _cb;
   void        *vp = _vp;
   int         idx = _idx;

   _cb = nullptr;
   _vp = nullptr;
   _idx = -1;

   if (cb)
   {
      cb(vp, idx, button, path, file);
   }

   return true;
}

//////////////////////////////////////////////////////
// build_glui()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::build_glui()
{
   GLUIPopUp::build_glui();

   int i,spacer;

   int id = _id << ID_SHIFT;

   _glui->rename(_title);

   assert(_panel.empty());        _panel.resize(PANEL_NUM, nullptr);
   assert(_button.empty());       _button.resize(BUT_NUM, nullptr);
   assert(_edittext.empty());     _edittext.resize(EDITTEXT_NUM, nullptr);
   assert(_checkbox.empty());     _checkbox.resize(CHECKBOX_NUM, nullptr);
   assert(_listbox.empty());      _listbox.resize(LIST_NUM, nullptr);
   assert(_bitmapbox.empty());    _bitmapbox.resize(BITMAPBOX_NUM   +   GLUI_FILE_SELECT_NUM_FILES, nullptr);
   assert(_statictext.empty());   _statictext.resize(STATICTEXT_NUM + 2*GLUI_FILE_SELECT_NUM_FILES, nullptr);
   assert(_activetext.empty());   _activetext.resize(ACTIVETEXT_NUM +   GLUI_FILE_SELECT_NUM_FILES, nullptr);

   //Controls

   _panel[PANEL_PATH] = new GLUI_Panel(_glui, "", GLUI_PANEL_RAISED /*,GLUI_PANEL_NONE*/);
   assert(_panel[PANEL_PATH]);           

   if (_icon != NO_ICON)
   {
      _bitmapbox[BITMAPBOX_ICON] = new GLUI_BitmapBox(
         _panel[PANEL_PATH], "",
         id+BITMAPBOX_ICON, bitmapbox_cbs, false);
      assert(_bitmapbox[BITMAPBOX_ICON]);
   
      _bitmapbox[BITMAPBOX_ICON]->set_border(0);
      _bitmapbox[BITMAPBOX_ICON]->set_margin(0);
      _bitmapbox[BITMAPBOX_ICON]->set_img_size(_icons[_icon]._width,_icons[_icon]._height);
      _bitmapbox[BITMAPBOX_ICON]->copy_img(_icons[_icon]._data,_icons[_icon]._width,_icons[_icon]._height,3);
   }

   new GLUI_Column(_panel[PANEL_PATH], false);

   //Spacer
   _statictext[STATICTEXT_SPACER_PATH] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_PATH]);

   //Path
   _listbox[LIST_PATH] = new GLUI_Listbox(
      _panel[PANEL_PATH], 
      "Path ", nullptr,
      id+LIST_PATH, listbox_cbs);
   assert(_listbox[LIST_PATH]);
   _listbox[LIST_PATH]->set_w(GLUI_FILE_SELECT_PATH_WIDTH);
   _listbox[LIST_PATH]->set_alignment(GLUI_ALIGN_RIGHT);

   new GLUI_Column(_panel[PANEL_PATH], false);

/*
   //Margin Spacer
   _statictext[STATICTEXT_SPACER_PATH_MARGIN1] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_PATH_MARGIN1]);
   _statictext[STATICTEXT_SPACER_PATH_MARGIN1]->set_w(0); 
   _statictext[STATICTEXT_SPACER_PATH_MARGIN1]->set_h(0); 

   new GLUI_Column(_panel[PANEL_PATH], false);
*/
   //Spacer
   _statictext[STATICTEXT_SPACER_UP] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_UP]);

   //Up
   _bitmapbox[BITMAPBOX_UP] = new GLUI_BitmapBox(
      _panel[PANEL_PATH], "",
      id+BITMAPBOX_UP, bitmapbox_cbs);
   assert(_bitmapbox[BITMAPBOX_UP]);

   _bitmapbox[BITMAPBOX_UP]->set_border(0);
   _bitmapbox[BITMAPBOX_UP]->set_margin(2);
   _bitmapbox[BITMAPBOX_UP]->set_depressable(true);
   _bitmapbox[BITMAPBOX_UP]->set_img_size(_bitmaps[BITMAP_UP]._width,_bitmaps[BITMAP_UP]._height);
   _bitmapbox[BITMAPBOX_UP]->copy_img(_bitmaps[BITMAP_UP]._data,_bitmaps[BITMAP_UP]._width,_bitmaps[BITMAP_UP]._height,3);

   new GLUI_Column(_panel[PANEL_PATH], false);

   //Spacer
   _statictext[STATICTEXT_SPACER_DOT] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_DOT]);

   //Dot

   _bitmapbox[BITMAPBOX_DOT] = new GLUI_BitmapBox(
      _panel[PANEL_PATH], "",
      id+BITMAPBOX_DOT, bitmapbox_cbs);
   assert(_bitmapbox[BITMAPBOX_DOT]);

   _bitmapbox[BITMAPBOX_DOT]->set_border(0);
   _bitmapbox[BITMAPBOX_DOT]->set_margin(2);
   _bitmapbox[BITMAPBOX_DOT]->set_depressable(true);
   _bitmapbox[BITMAPBOX_DOT]->set_img_size(_bitmaps[BITMAP_DOT]._width,_bitmaps[BITMAP_DOT]._height);
   _bitmapbox[BITMAPBOX_DOT]->copy_img(_bitmaps[BITMAP_DOT]._data,_bitmaps[BITMAP_DOT]._width,_bitmaps[BITMAP_DOT]._height,3);

   new GLUI_Column(_panel[PANEL_PATH], false);

   //Spacer
   _statictext[STATICTEXT_SPACER_PLUS] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_PLUS]);

   //Plus

   _bitmapbox[BITMAPBOX_PLUS] = new GLUI_BitmapBox(
      _panel[PANEL_PATH], "",
      id+BITMAPBOX_PLUS, bitmapbox_cbs);
   assert(_bitmapbox[BITMAPBOX_PLUS]);

   _bitmapbox[BITMAPBOX_PLUS]->set_border(0);
   _bitmapbox[BITMAPBOX_PLUS]->set_margin(2);
   _bitmapbox[BITMAPBOX_PLUS]->set_depressable(true);
   _bitmapbox[BITMAPBOX_PLUS]->set_img_size(_bitmaps[BITMAP_PLUS]._width,_bitmaps[BITMAP_PLUS]._height);
   _bitmapbox[BITMAPBOX_PLUS]->copy_img(_bitmaps[BITMAP_PLUS]._data,_bitmaps[BITMAP_PLUS]._width,_bitmaps[BITMAP_PLUS]._height,3);

   new GLUI_Column(_panel[PANEL_PATH], false);

   //Spacer
   _statictext[STATICTEXT_SPACER_R] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_R]);

   //R

   _bitmapbox[BITMAPBOX_R] = new GLUI_BitmapBox(
      _panel[PANEL_PATH], "",
      id+BITMAPBOX_R, bitmapbox_cbs);
   assert(_bitmapbox[BITMAPBOX_R]);

   _bitmapbox[BITMAPBOX_R]->set_border(0);
   _bitmapbox[BITMAPBOX_R]->set_margin(2);
   _bitmapbox[BITMAPBOX_R]->set_depressable(true);
   _bitmapbox[BITMAPBOX_R]->set_img_size(_bitmaps[BITMAP_R]._width,_bitmaps[BITMAP_R]._height);
   _bitmapbox[BITMAPBOX_R]->copy_img(_bitmaps[BITMAP_R]._data,_bitmaps[BITMAP_R]._width,_bitmaps[BITMAP_R]._height,3);

   new GLUI_Column(_panel[PANEL_PATH], false);

   //Spacer
   _statictext[STATICTEXT_SPACER_X] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_X]);

   //X

   _bitmapbox[BITMAPBOX_X] = new GLUI_BitmapBox(
      _panel[PANEL_PATH], "",
      id+BITMAPBOX_X, bitmapbox_cbs);
   assert(_bitmapbox[BITMAPBOX_X]);

   _bitmapbox[BITMAPBOX_X]->set_border(0);
   _bitmapbox[BITMAPBOX_X]->set_margin(2);
   _bitmapbox[BITMAPBOX_X]->set_depressable(true);
   _bitmapbox[BITMAPBOX_X]->set_img_size(_bitmaps[BITMAP_X]._width,_bitmaps[BITMAP_X]._height);
   _bitmapbox[BITMAPBOX_X]->copy_img(_bitmaps[BITMAP_X]._data,_bitmaps[BITMAP_X]._width,_bitmaps[BITMAP_X]._height,3);

   new GLUI_Column(_panel[PANEL_PATH], false);

   //Margin Spacer
   _statictext[STATICTEXT_SPACER_PATH_MARGIN2] = new GLUI_StaticText(
      _panel[PANEL_PATH],
      "");
   assert(_statictext[STATICTEXT_SPACER_PATH_MARGIN2]);
   _statictext[STATICTEXT_SPACER_PATH_MARGIN2]->set_w(0); 
   _statictext[STATICTEXT_SPACER_PATH_MARGIN2]->set_h(0); 


   //Files

   _panel[PANEL_FILES] = new GLUI_Panel(_glui, "", GLUI_PANEL_NONE /*,GLUI_PANEL_RAISED*/);
   assert(_panel[PANEL_FILES]);           

   //Types

   //Spacer

   _button[BUT_HEADING_TYPE] = new GLUI_Button(
      _panel[PANEL_FILES],
      "",
      id+BUT_HEADING_TYPE, button_cbs);
   assert(_button[BUT_HEADING_TYPE]);
   _button[BUT_HEADING_TYPE]->set_alignment(GLUI_ALIGN_CENTER);
   _button[BUT_HEADING_TYPE]->disable();
   //_button[BUT_HEADING_TYPE]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);

   //Spacer
   _statictext[STATICTEXT_SPACER_FILES_TYPE] = new GLUI_StaticText(
      _panel[PANEL_FILES],
      "");
   assert(_statictext[STATICTEXT_SPACER_FILES_TYPE]);
   _statictext[STATICTEXT_SPACER_FILES_TYPE]->set_w(0);
   _statictext[STATICTEXT_SPACER_FILES_TYPE]->set_alignment(GLUI_ALIGN_CENTER);
   
   for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++)
   {
      _bitmapbox[BITMAPBOX_NUM + i] = new GLUI_BitmapBox(
         _panel[PANEL_FILES], "",
         id+BITMAPBOX_NUM + i, bitmapbox_cbs, false);
      assert(_bitmapbox[BITMAPBOX_NUM + i]);
   
      _bitmapbox[BITMAPBOX_NUM + i]->set_border(0);
      _bitmapbox[BITMAPBOX_NUM + i]->set_margin(0);
      _bitmapbox[BITMAPBOX_NUM + i]->set_alignment(GLUI_ALIGN_RIGHT);
      _bitmapbox[BITMAPBOX_NUM + i]->set_img_size(_bitmaps[BITMAP_FOLDER]._width,_bitmaps[BITMAP_FOLDER]._height);
      _bitmapbox[BITMAPBOX_NUM + i]->copy_img(_bitmaps[BITMAP_FOLDER]._data,_bitmaps[BITMAP_FOLDER]._width,_bitmaps[BITMAP_FOLDER]._height,3);
   }

   //Names

   new GLUI_Column(_panel[PANEL_FILES], false);

   _button[BUT_HEADING_NAME] = new GLUI_Button(
      _panel[PANEL_FILES],
      "Name",
      id+BUT_HEADING_NAME, button_cbs);
   assert(_button[BUT_HEADING_NAME]);
   _button[BUT_HEADING_NAME]->set_alignment(GLUI_ALIGN_LEFT);
   _button[BUT_HEADING_NAME]->set_w(GLUI_FILE_SELECT_NAME_WIDTH-1);

   //Spacer
   _statictext[STATICTEXT_SPACER_FILES_NAME] = new GLUI_StaticText(
      _panel[PANEL_FILES],
      "");
   assert(_statictext[STATICTEXT_SPACER_FILES_NAME]);
   _statictext[STATICTEXT_SPACER_FILES_NAME]->set_w(0); 

   for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++) {
      char tmp[64];
      sprintf(tmp, "Filename #%d", i);
      _activetext[ACTIVETEXT_NUM + i] = new GLUI_ActiveText(
         _panel[PANEL_FILES],
         tmp,
         id+ACTIVETEXT_NUM + i, activetext_cbs);
      assert(_activetext[ACTIVETEXT_NUM + i]);
      _activetext[ACTIVETEXT_NUM + i]->set_alignment(GLUI_ALIGN_LEFT);
      _activetext[ACTIVETEXT_NUM + i]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);
   }

   //Sizes

   new GLUI_Column(_panel[PANEL_FILES], false);

   _button[BUT_HEADING_SIZE] = new GLUI_Button(
      _panel[PANEL_FILES],
      "Size",
      id+BUT_HEADING_SIZE, button_cbs);
   assert(_button[BUT_HEADING_SIZE]);
   _button[BUT_HEADING_SIZE]->set_alignment(GLUI_ALIGN_LEFT);
   _button[BUT_HEADING_SIZE]->set_w(GLUI_FILE_SELECT_SIZE_WIDTH);

   //Spacer
   _statictext[STATICTEXT_SPACER_FILES_SIZE] = new GLUI_StaticText(
      _panel[PANEL_FILES],
      "");
   assert(_statictext[STATICTEXT_SPACER_FILES_SIZE]);
   _statictext[STATICTEXT_SPACER_FILES_SIZE]->set_w(0); 

   for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++)
   {
      _statictext[STATICTEXT_NUM + 2*i] = new GLUI_StaticText(
         _panel[PANEL_FILES],
         " X.XXX KB");
      assert(_statictext[STATICTEXT_NUM + 2*i]);
      _statictext[STATICTEXT_NUM + 2*i]->set_alignment(GLUI_ALIGN_RIGHT);
      _statictext[STATICTEXT_NUM + 2*i]->set_right_justify(true);
      _statictext[STATICTEXT_NUM + 2*i]->set_w(GLUI_FILE_SELECT_SIZE_WIDTH);
      //_statictext[STATICTEXT_NUM + 2*i]->set_w(0);
   }

   //Dates
   
   new GLUI_Column(_panel[PANEL_FILES], false);
   
   _button[BUT_HEADING_DATE] = new GLUI_Button(
      _panel[PANEL_FILES],
      "Date",
      id+BUT_HEADING_DATE, button_cbs);
   assert(_button[BUT_HEADING_DATE]);
   _button[BUT_HEADING_DATE]->set_alignment(GLUI_ALIGN_LEFT);
   _button[BUT_HEADING_DATE]->set_w(GLUI_FILE_SELECT_DATE_WIDTH);

   //Spacer
   _statictext[STATICTEXT_SPACER_FILES_DATE] = new GLUI_StaticText(
      _panel[PANEL_FILES],
      "");
   assert(_statictext[STATICTEXT_SPACER_FILES_DATE]);
   _statictext[STATICTEXT_SPACER_FILES_DATE]->set_w(0); 

   for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++)
   {
      _statictext[STATICTEXT_NUM + 2*i + 1] = new GLUI_StaticText(
         _panel[PANEL_FILES],
         " DD/MM/YY HH:MM");
      assert(_statictext[STATICTEXT_NUM + 2*i + 1]);
      _statictext[STATICTEXT_NUM + 2*i + 1]->set_alignment(GLUI_ALIGN_RIGHT);
      _statictext[STATICTEXT_NUM + 2*i + 1]->set_right_justify(true);
      _statictext[STATICTEXT_NUM + 2*i + 1]->set_w(GLUI_FILE_SELECT_DATE_WIDTH);
      //_statictext[STATICTEXT_NUM + 2*i + 1]->set_w(0);
   }

   //Scrolling

   new GLUI_Column(_panel[PANEL_FILES], false);

   //Spacer
   _button[BUT_HEADING_SCROLL] = new GLUI_Button(
      _panel[PANEL_FILES],
      "",
      id+BUT_HEADING_SCROLL, button_cbs);
   assert(_button[BUT_HEADING_SCROLL]);
   _button[BUT_HEADING_SCROLL]->set_alignment(GLUI_ALIGN_LEFT);
   _button[BUT_HEADING_SCROLL]->disable();
   //_button[BUT_HEADING_TYPE]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);

   //Spacer
   _statictext[STATICTEXT_SPACER_FILES_SCROLL] = new GLUI_StaticText(
      _panel[PANEL_FILES],
      "");
   assert(_statictext[STATICTEXT_SPACER_FILES_SCROLL]);
   _statictext[STATICTEXT_SPACER_FILES_SCROLL]->set_w(0); 

   _bitmapbox[BITMAPBOX_UP_FILE] = new GLUI_BitmapBox(
      _panel[PANEL_FILES], "",
      id+BITMAPBOX_UP_FILE, bitmapbox_cbs,true);
   assert(_bitmapbox[BITMAPBOX_UP_FILE]);

   _bitmapbox[BITMAPBOX_UP_FILE]->set_border(0);
   _bitmapbox[BITMAPBOX_UP_FILE]->set_margin(2);
   _bitmapbox[BITMAPBOX_UP_FILE]->set_depressable(true);
   _bitmapbox[BITMAPBOX_UP_FILE]->set_alignment(GLUI_ALIGN_LEFT);
   _bitmapbox[BITMAPBOX_UP_FILE]->set_img_size(_bitmaps[BITMAP_UPARROW]._width,_bitmaps[BITMAP_UPARROW]._height);
   _bitmapbox[BITMAPBOX_UP_FILE]->copy_img(_bitmaps[BITMAP_UPARROW]._data,_bitmaps[BITMAP_UPARROW]._width,_bitmaps[BITMAP_UPARROW]._height,3);


   _bitmapbox[BITMAPBOX_SCROLL_FILE] = new GLUI_BitmapBox(
      _panel[PANEL_FILES], "",
      id+BITMAPBOX_SCROLL_FILE, bitmapbox_cbs, true);
   assert(_bitmapbox[BITMAPBOX_SCROLL_FILE]);

   _bitmapbox[BITMAPBOX_SCROLL_FILE]->set_border(0);
   _bitmapbox[BITMAPBOX_SCROLL_FILE]->set_margin(2);
   _bitmapbox[BITMAPBOX_SCROLL_FILE]->set_depressable(true);
   _bitmapbox[BITMAPBOX_SCROLL_FILE]->set_alignment(GLUI_ALIGN_LEFT);
   _bitmapbox[BITMAPBOX_SCROLL_FILE]->set_img_size(_bitmaps[BITMAP_SCROLL]._width,_bitmaps[BITMAP_SCROLL]._height);
   _bitmapbox[BITMAPBOX_SCROLL_FILE]->copy_img(_bitmaps[BITMAP_SCROLL]._data,_bitmaps[BITMAP_SCROLL]._width,_bitmaps[BITMAP_SCROLL]._height,3);

   _bitmapbox[BITMAPBOX_DOWN_FILE] = new GLUI_BitmapBox(
      _panel[PANEL_FILES], "",
      id+BITMAPBOX_DOWN_FILE, bitmapbox_cbs, true);
   assert(_bitmapbox[BITMAPBOX_DOWN_FILE]);

   _bitmapbox[BITMAPBOX_DOWN_FILE]->set_border(0);
   _bitmapbox[BITMAPBOX_DOWN_FILE]->set_margin(2);
   _bitmapbox[BITMAPBOX_DOWN_FILE]->set_depressable(true);
   _bitmapbox[BITMAPBOX_DOWN_FILE]->set_alignment(GLUI_ALIGN_LEFT);
   _bitmapbox[BITMAPBOX_DOWN_FILE]->set_img_size(_bitmaps[BITMAP_DOWNARROW]._width,_bitmaps[BITMAP_DOWNARROW]._height);
   _bitmapbox[BITMAPBOX_DOWN_FILE]->copy_img(_bitmaps[BITMAP_DOWNARROW]._data,_bitmaps[BITMAP_DOWNARROW]._width,_bitmaps[BITMAP_DOWNARROW]._height,3);


   //Adjust scroller

   _bitmapbox[BITMAPBOX_SCROLL_FILE]->set_img_size(_bitmapbox[BITMAPBOX_SCROLL_FILE]->get_image_w(), 
                                                    (GLUI_FILE_SELECT_NUM_FILES) * (_bitmapbox[BITMAPBOX_NUM]->get_h()) 
                                                      + (GLUI_FILE_SELECT_NUM_FILES-1) * 3 - 3
                                                         - _bitmapbox[BITMAPBOX_UP_FILE]->get_h() 
                                                         - _bitmapbox[BITMAPBOX_DOWN_FILE]->get_h() 
                                                         - 3);

   //Actions
   
   _panel[PANEL_ACTION] = new GLUI_Panel(_glui, "", GLUI_PANEL_RAISED);
   assert(_panel[PANEL_ACTION]);           

   new GLUI_Column(_panel[PANEL_ACTION], false);

   _edittext[EDITTEXT_FILE] = new GLUI_EditText(
      _panel[PANEL_ACTION], "Filename ", 
      GLUI_EDITTEXT_TEXT, nullptr,
      id+EDITTEXT_FILE, edittext_cbs);
   assert(_edittext[EDITTEXT_FILE]);
   _edittext[EDITTEXT_FILE]->set_alignment(GLUI_ALIGN_RIGHT);
   //_edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

   _listbox[LIST_FILTER] = new GLUI_Listbox(
      _panel[PANEL_ACTION], 
      "Filter Mask ", nullptr,
      id+LIST_FILTER, listbox_cbs);
   assert(_listbox[LIST_FILTER]);
   _listbox[LIST_FILTER]->set_alignment(GLUI_ALIGN_RIGHT);
   _listbox[LIST_FILTER]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

   new GLUI_Column(_panel[PANEL_ACTION], false);

   _statictext[STATICTEXT_LABEL_DOT] = new GLUI_StaticText(
      _panel[PANEL_ACTION],
      "Dot");
   assert(_statictext[STATICTEXT_LABEL_DOT]);
   _statictext[STATICTEXT_LABEL_DOT]->set_w(0); 
   _statictext[STATICTEXT_LABEL_DOT]->set_w(_statictext[STATICTEXT_LABEL_DOT]->get_w()+3); 
   _statictext[STATICTEXT_LABEL_DOT]->set_alignment(GLUI_ALIGN_LEFT);

   _checkbox[CHECKBOX_DOT] = new GLUI_Checkbox(
      _panel[PANEL_ACTION], "", nullptr,
      id+CHECKBOX_DOT, checkbox_cbs);
   assert(_checkbox[CHECKBOX_DOT]);
   _checkbox[CHECKBOX_DOT]->set_show_name(false);  
   _checkbox[CHECKBOX_DOT]->set_w(0);
   _checkbox[CHECKBOX_DOT]->set_alignment(GLUI_ALIGN_CENTER);

   //Fix size (should always have shorter name...)
   _edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH - 
                                 (_edittext[EDITTEXT_FILE]->string_width(_listbox[LIST_FILTER]->get_name()) - 
                                          _edittext[EDITTEXT_FILE]->string_width(_edittext[EDITTEXT_FILE]->get_name())));
/*   
   new GLUI_Column(_panel[PANEL_ACTION], false);
   
   //Margin spacer
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN1] = new GLUI_StaticText(
      _panel[PANEL_ACTION],
      "");
   assert(_statictext[STATICTEXT_SPACER_ACTION_MARGIN1]);
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN1]->set_w(0); 
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN1]->set_h(0); 
*/
   new GLUI_Column(_panel[PANEL_ACTION], false);
   
   _button[BUT_ACTION] = new GLUI_Button(
      _panel[PANEL_ACTION], _action.c_str(),
      id+BUT_ACTION, button_cbs);
   assert(_button[BUT_ACTION]);
   _button[BUT_ACTION]->set_w(GLUI_FILE_SELECT_ACTION_WIDTH);

   _button[BUT_CANCEL] = new GLUI_Button(
      _panel[PANEL_ACTION], "Cancel", 
      id+BUT_CANCEL, button_cbs);
   assert(_button[BUT_CANCEL]);
   _button[BUT_CANCEL]->set_w(GLUI_FILE_SELECT_ACTION_WIDTH);
/*
   new GLUI_Column(_panel[PANEL_ACTION], false);

   //Margin spacer
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN2] = new GLUI_StaticText(
      _panel[PANEL_ACTION],
      "");
   assert(_statictext[STATICTEXT_SPACER_ACTION_MARGIN2]);
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN2]->set_w(0); 
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN2]->set_h(0); 
*/
   //Adjust file spacers
   _statictext[STATICTEXT_SPACER_FILES_TYPE]->set_h(GLUI_FILE_SELECT_HEADING_GAP);
   _statictext[STATICTEXT_SPACER_FILES_NAME]->set_h(GLUI_FILE_SELECT_HEADING_GAP);
   _statictext[STATICTEXT_SPACER_FILES_SIZE]->set_h(GLUI_FILE_SELECT_HEADING_GAP);
   _statictext[STATICTEXT_SPACER_FILES_DATE]->set_h(GLUI_FILE_SELECT_HEADING_GAP);
   _statictext[STATICTEXT_SPACER_FILES_SCROLL]->set_h(GLUI_FILE_SELECT_HEADING_GAP);

   _button[BUT_HEADING_TYPE]->set_w(_bitmapbox[BITMAPBOX_NUM]->get_w()+2);
   _button[BUT_HEADING_SCROLL]->set_w(_bitmapbox[BITMAPBOX_SCROLL_FILE]->get_w()-1);

   
   //Adjust path spacers
   spacer = ((_icon != NO_ICON) ? 
                     (_bitmapbox[BITMAPBOX_ICON]->get_h()) : 
                     (_bitmapbox[BITMAPBOX_UP]->get_h())) 
                                    - _listbox[LIST_PATH]->get_h() - 6;
   spacer = max(0,spacer);
   _statictext[STATICTEXT_SPACER_PATH]->set_h(spacer);
   _statictext[STATICTEXT_SPACER_PATH]->set_w(0);

   spacer = ((_icon != NO_ICON) ? 
                     (_bitmapbox[BITMAPBOX_ICON]->get_h()) : 
                     (_listbox[LIST_PATH]->get_h())) 
                                    - _bitmapbox[BITMAPBOX_UP]->get_h() - 4;
   spacer = max(0,spacer);

   _statictext[STATICTEXT_SPACER_UP]->set_h(spacer);
   _statictext[STATICTEXT_SPACER_UP]->set_w(0);
   _statictext[STATICTEXT_SPACER_DOT]->set_h(spacer);
   _statictext[STATICTEXT_SPACER_DOT]->set_w(0);
   _statictext[STATICTEXT_SPACER_PLUS]->set_h(spacer);
   _statictext[STATICTEXT_SPACER_PLUS]->set_w(0);
   _statictext[STATICTEXT_SPACER_R]->set_h(spacer);
   _statictext[STATICTEXT_SPACER_R]->set_w(0);
   _statictext[STATICTEXT_SPACER_X]->set_h(spacer);
   _statictext[STATICTEXT_SPACER_X]->set_w(0);

   spacer = max(0, _panel[PANEL_FILES]->get_w() - _panel[PANEL_PATH]->get_w());
   //_statictext[STATICTEXT_SPACER_PATH_MARGIN1]->set_w(spacer/2);
   //_statictext[STATICTEXT_SPACER_PATH_MARGIN2]->set_w(spacer/2);
   _statictext[STATICTEXT_SPACER_PATH_MARGIN2]->set_w(spacer);

   //Adjust action spacers
/*
   spacer = _panel[PANEL_FILES]->get_w() - _panel[PANEL_ACTION]->get_w();
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN1]->set_w(spacer/2);
   _statictext[STATICTEXT_SPACER_ACTION_MARGIN2]->set_w(spacer/2);
*/
   spacer = _panel[PANEL_FILES]->get_w() - _panel[PANEL_ACTION]->get_w();
   _button[BUT_ACTION]->set_w(_button[BUT_ACTION]->get_w() + spacer);
   _button[BUT_CANCEL]->set_w(_button[BUT_CANCEL]->get_w() + spacer);

   //Center the window
   int root_x, root_y, root_w, root_h, x, y;

   _glut_winsys->size(root_w,root_h);
   _glut_winsys->position(root_x,root_y);

   x = root_x + (root_w - _glui->get_w())/2;
   y = root_y + (root_h - _glui->get_h())/2;

   _glui->reposition(max(x,root_x),max(y,root_y));

   init();

   // Send unhandled middle button events to the scroller
   // WIN32 GLUT's been hacked to turn mouse wheel events into
   // small middle button drag events...
   _glui->set_default_middle_handler(_bitmapbox[BITMAPBOX_SCROLL_FILE]);

   _glui->activate_control(_edittext[EDITTEXT_FILE],GLUI_ACTIVATE_TAB);

}

//////////////////////////////////////////////////////
// unbuild_glui()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::unbuild_glui()
{
   //Clean up...
   GLUIPopUp::unbuild_glui();
}

//////////////////////////////////////////////////////
// readdir_()
//////////////////////////////////////////////////////
vector<string>
GLUIFileSelect::readdir_(const string &path, const string &filter)
{
   vector<string> list;
   const string dot("."), dotdot("..");
   bool hide_leading_dot = _checkbox[CHECKBOX_DOT]->get_int_val() == 0;
#ifdef WIN32
   WIN32_FIND_DATA file;
   HANDLE hFile;
   
   assert(filter != "");
   
   //General filter only applies to files, and not directories... so use '*' filter
   if ((hFile = FindFirstFile((path + "/" + "*").c_str(), &file)) != INVALID_HANDLE_VALUE) {
      while (true) {
         string fname(file.cFileName); assert(fname.length()>0);
	      
         if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            //However the leading '.' filter checkbox still applies!
            if ( (( hide_leading_dot) && (fname[0] != '.'))  ||
                 ((!hide_leading_dot) && (fname!=dot)&&(fname!=dotdot))  )
            {
               list.push_back(fname);
            }

         }
         if (!FindNextFile(hFile, &file)) break;
      }
      FindClose(hFile);
   }

   //General filter applies to files
   if ((hFile = FindFirstFile((path + "/" + filter).c_str(), &file)) != INVALID_HANDLE_VALUE) {
      while (true) {
         string fname(file.cFileName); assert(fname.length()>0);

         if (!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            //However the leading '.' filter checkbox also applies!
            if (!(hide_leading_dot && fname[0] == '.')) {
               list.push_back(fname);
            }
         }
         if (!FindNextFile(hFile, &file)) break;
      }
      FindClose(hFile);
   }
#else
   DIR *dir = nullptr;
   struct dirent *direntry;

   if ((dir = opendir(path.c_str())) != nullptr) {
      struct stat statbuf;
      while ((direntry = readdir(dir)) != nullptr) {
         string file(direntry->d_name), full_path = path + "/" + file;
         
         if (!stat(full_path.c_str(), &statbuf)) {
            //General filter only applies to files, not directories...
            if ((statbuf.st_mode&S_IFMT) == S_IFDIR) {
               //But the '.' filter checkbox still applies
               if ( ( hide_leading_dot && file[0]!='.')             ||
                    (!hide_leading_dot && file!=dot && file!=dotdot)  )
               {
                  list.push_back(file);
               }
            } 
            else if (((statbuf.st_mode&S_IFMT) != S_IFDIR) && !fnmatch(filter.c_str(), file.c_str(), FNM_PATHNAME & FNM_PERIOD)) {
               //However the leading '.' filter checkbox also applies!
               if (!(hide_leading_dot && file[0]=='.')) {
                  list.push_back(file);
               }
            }
         }
      }
      closedir(dir);
   }
#endif
   return list;
}

//////////////////////////////////////////////////////
// stat_()
//////////////////////////////////////////////////////
bool
GLUIFileSelect::stat_(const string &cpath, DIR_ENTRYptr &ret)
{
   assert(cpath != "");

   string path = cpath;

   ret->clear();

#ifdef WIN32
   
   struct _stat buf;

   char   buf_drv[_MAX_DRIVE], buf_dir[_MAX_DIR];
   char buf_fname[_MAX_FNAME], buf_ext[_MAX_EXT];

   _splitpath(path.c_str(), buf_drv, buf_dir, buf_fname, buf_ext);

   string s_drv(buf_drv);
   string s_dir(buf_dir);
   string s_fname(buf_fname);

   //Roll fname and extension together and make sure
   //it's non-null only for real file/folder names (not dot)
   s_fname = s_fname + buf_ext;

   //If fname points to a directory via a '.', then drop the
   if (s_fname == ".")
   {
      s_fname = "";
   }

   //Full paths must start with a drive
   if (s_drv == "")
   {
      cerr << "GLUIFileSelect::stat_() - Not a full path with drive prefix: '" << path << "'\n";
      return false;
   }
   //Path must point to a literal entity
   else if (s_fname == "..")
   {
      cerr << "GLUIFileSelect::stat_() - Relative paths not allowed: '" << path << "'\n";
      return false;
   }
   //No file or directory indicates a drive's root dir
   else if ( ((s_dir == "/")  ||  (s_dir == "\\") || (s_dir == "")) && (s_fname == "") )
   {
      //Clean up path...
      path = s_drv + "\\";

      string drive_type_name;
      ULARGE_INTEGER total_space, free_space;
      uint drive_type = GetDriveType(path.c_str());

      switch (drive_type)
      {
         case DRIVE_REMOVABLE:   drive_type_name = "[Floppy Drive]"; break;
         case DRIVE_FIXED:       drive_type_name = "[Hard Drive]"; break;
         case DRIVE_REMOTE:      drive_type_name = "[Network Drive]"; break;
         case DRIVE_CDROM:       drive_type_name = "[Disc Drive]"; break;
         case DRIVE_RAMDISK:     drive_type_name = "[RAM Drive]"; break;
         case DRIVE_UNKNOWN:     drive_type_name = "[*UNKNOWN TYPE*]"; break;
         case DRIVE_NO_ROOT_DIR: drive_type_name = "[*NO ROOT DIRECTORY*]"; break;
         default: assert(0); break;
      }

      ret->_type = DIR_ENTRY::DIR_ENTRY_DRIVE;
      ret->_full_path = path;
      ret->_name = drive_type_name;

      //Ignores error if media not present to avoid the pop-up prompt to insert disc...
      unsigned int old_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS); 
      //XXX - Floppy seek is so slow, it really chokes... Forget it.
      if ((drive_type != DRIVE_REMOVABLE) && (GetDiskFreeSpaceEx(path.c_str(), &free_space, &total_space, nullptr)))
//      if (GetDiskFreeSpaceEx(path.c_str(), &free_space, &total_space, nullptr))
      {
         ret->_size = (LONGLONG)(total_space.QuadPart-free_space.QuadPart);
      }
      SetErrorMode(old_mode);
   }
   else
   {
      //Fix up paths with trailing slashes because
      //they don't end with a file/folder name, 
      //otherwise _stat will fail out...

      //s_drv looks like "X:"
      //s_dir looks like "/some/folders/with/trailing/slash/"
      //s_fname looks like "name.extension" (could be a folder or file)
      if (s_fname == "")
      {
         //We're not just a drive, so s_dir must
         //contain folder(s) and we'll just lop off the trailing slash
         //so path points to the foldername...
         
         //XXX - use strcpy instead...
         if ( (s_dir[s_dir.length()-1] != '/') && (s_dir[s_dir.length()-1] != '\\') )
            
         {
            cerr << "GLUIFileSelect::stat_() - Error. Something is malformed about path. Trailing slash expected: '" << path << "'\n";
            return false;
         }

         path = s_drv + s_dir.substr(0, s_dir.length()-1);
      }
      else
      {
         path = s_drv + s_dir + s_fname;
      }


      if (_stat(path, &buf) != 0)
      {
         cerr << "GLUIFileSelect::stat_() - Failed to get information on target: '" << path << "'\n";
         return false;
      }

      switch (buf.st_mode & _S_IFMT)
      {
         case _S_IFREG:
            ret->_type = DIR_ENTRY::DIR_ENTRY_FILE;
            ret->_full_path = path;
            ret->_size = buf.st_size;  
            ret->_date = buf.st_mtime;
         break;
         case _S_IFDIR:
            ret->_type = DIR_ENTRY::DIR_ENTRY_DIRECTORY;
            ret->_full_path = path;
            //ret->_size = buf.st_size;  
            ret->_date = buf.st_mtime;
         break;
         default:
            ret->_type = DIR_ENTRY::DIR_ENTRY_UNKNOWN;
         break;
      }
   }
#else
   struct stat buf;
 
   //Path must start with a '/'
   if (path[0] != '/')
   {
      cerr << "GLUIFileSelect::stat_() - Not a full path beginning with a '/': '" << path << "'\n";
      return false;
   }
   //Path must point to a literal entity
   if (path[path.length()-1] == '.')
   {
      cerr << "GLUIFileSelect::stat_() - Relative paths with .'s not allowed: '" << path << "'\n";
      return false;
   }
   else
   {
      //Treat root directory like a win32 'drive'
      if (path.length() == 1)
      {
         assert(path == "/");

         if (stat("/.", &buf) != 0)
         {
            cerr << "GLUIFileSelect::stat_() - Failed to get information on target: '/.'\n";
            return false;
         }

         switch (buf.st_mode & S_IFMT)
         {
            case S_IFDIR:
               ret->_full_path = "/";
               ret->_type =      DIR_ENTRY::DIR_ENTRY_DRIVE;
               ret->_name =      "[Root]";  
               //ret->_size =    buf.st_size;  
               ret->_date =      buf.st_mtime;
            break;
            default:
               cerr << "GLUIFileSelect::stat_() - Target: '/.' claims not to be a directory!!\n";
               return false;
            break;
         }  
      }
      else
      {
         //Fix up paths with trailing slashes because
         //they don't end with a file/folder name, 
         //otherwise _stat will fail out...
         if (path[path.length()-1] == '/')
         {
            //We'll just lop off the trailing slash
            //so path points to the foldername...
            path = path.substr(0, path.length()-1);
         }

         if (stat(path.c_str(), &buf) != 0)
         {
            cerr << "GLUIFileSelect::stat_() - Failed to get information on target: '" << path << "'\n";
            return false;
         }

         switch (buf.st_mode & S_IFMT)
         {
            case S_IFDIR:
               ret->_type =      DIR_ENTRY::DIR_ENTRY_DIRECTORY;
               ret->_full_path = path;
               //ret->_size =    buf.st_size;  
               ret->_date =      buf.st_mtime;
            break;
            case S_IFREG:
               ret->_type =      DIR_ENTRY::DIR_ENTRY_FILE;
               ret->_full_path = path;
               ret->_size =      buf.st_size;  
               ret->_date =      buf.st_mtime;
            break;
            default:
               ret->_type = DIR_ENTRY::DIR_ENTRY_UNKNOWN;
            break;
         }  
      }
   }
#endif
   
   return true;

}



/////////////////////////////////////////////////////
// generate_dir_contents()
//////////////////////////////////////////////////////
bool
GLUIFileSelect::generate_dir_contents(DIR_ENTRYptr &dir)
{
   vector<string>::size_type i;

   assert((dir->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY) ||
          (dir->_type == DIR_ENTRY::DIR_ENTRY_DRIVE) ||
          (dir->_type == DIR_ENTRY::DIR_ENTRY_ROOT) );

   dir->_contents.clear();

   if (dir->_type != DIR_ENTRY::DIR_ENTRY_ROOT) {
      assert(_filter < _filters.size());
      //readdir wants a path pointing to a dir...
      //_full_path already has trailing slashes removed
      //for directories, but not for 'drives' i.e. root directories...
      //so add the . back in that case....
      vector<string> entries = readdir_(dir->_full_path +
                                           (dir->_type == DIR_ENTRY::DIR_ENTRY_DRIVE?".":""),
                                        get_filter());

      for (i=0; i<entries.size(); i++) {
         // Careful about assembling paths, /'s and filenames
         // Don't need trailing slashes for root direntories a.k.a. 'drives'...
         DIR_ENTRYptr e = generate_dir_entry(
            dir->_full_path + 
               (dir->_type == DIR_ENTRY::DIR_ENTRY_DRIVE?"":"/") + entries[i],
            entries[i]);

         if (  (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY) ||
               (e->_type == DIR_ENTRY::DIR_ENTRY_FILE) )
         {
            dir->_contents.push_back(e);
         }
         else if (e->_type == DIR_ENTRY::DIR_ENTRY_UNKNOWN)
         {
            cerr << "GLUIFileSelect::generate_dir_contents() - WARNING!! Unknown directory entry: '" << e->_full_path << "'\n";
         }
         else
         {
            assert(e->_type != DIR_ENTRY::DIR_ENTRY_DRIVE);
            assert(e->_type != DIR_ENTRY::DIR_ENTRY_ROOT);
         }
      }
   }
   else
   {
      DIR_ENTRYptr e;
#ifdef WIN32
      // XXX - Should this be more forgiving under failure? Nah
      DWORD drives = GetLogicalDrives();

      assert(drives != 0);

      for (int i=0; i < 26; i++) {
         if (drives & 1<<i) {
            string drv((char)('A'+i));
            e = generate_dir_entry(drv + ":\\", drv + ": ");
            assert((e != nullptr) && (e->_type == DIR_ENTRY::DIR_ENTRY_DRIVE));
            dir->_contents.push_back(e);
         }
      }
#else
      // XXX - Should this be more forgiving under failure? Nah
      e = generate_dir_entry("/", "/ ");
      assert( (e != nullptr) && (e->_type == DIR_ENTRY::DIR_ENTRY_DRIVE));
      dir->_contents.push_back(e);
#endif

      //Verify this path... 
      //Save the CWD before mucking about...
      string old_cwd = getcwd_(), jotroot_cwd;

      if (chdir_(Config::JOT_ROOT())) {
         if ((jotroot_cwd = getcwd_()) != "") {
            e = generate_dir_entry(jotroot_cwd, "");
            if ((e != nullptr) && (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY)) {
               dir->_contents.push_back(e);
            }
         }
      }
      if (old_cwd != "") chdir_(old_cwd);

      //These paths have been 'tested' by chdiring to them,
      //and reading back the getcwd... They're 'safe'
      for (i=0; i<_current_recent_paths.size(); i++) {
         e = generate_dir_entry(_current_recent_paths[i], "");
         if ((e != nullptr) && (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY)) {
            dir->_contents.push_back(e);
         }
      }
   }

   return true;
}


//////////////////////////////////////////////////////
// sort_dir_contents()
//////////////////////////////////////////////////////

static int sort_name(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
#ifdef WIN32
//Case-sensitive string comparison...
//{ return strcmp(      va->_name.c_str(), vb->_name.c_str());   }
//Case-INsensitive version...
{ return _stricmp(      va->_name.c_str(), vb->_name.c_str());   }
#else
//Case-sensitive string comparison...
//{ return strcmp(      va->_name.c_str(), vb->_name.c_str());   }
//Case-INsensitive version...
{ return strcasecmp(    va->_name.c_str(), vb->_name.c_str());   }
#endif

static int sort_type(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
{ return                va->_type -  vb->_type;    }
static int sort_size(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
{ return Sign2(         va->_size -  vb->_size);   }
static int sort_date(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
{ return Sign2(difftime(va->_date,   vb->_date));  }


static bool sort_by_name_up(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
{ int ret; if (!(ret = sort_type(va,vb))) if (!(ret = sort_name(va,vb))) ret = sort_date(va,vb); return ret < 0; }
static bool sort_by_date_up(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
{ int ret; if (!(ret = sort_type(va,vb))) if (!(ret = sort_date(va,vb))) ret = sort_name(va,vb); return ret < 0; }
static bool sort_by_size_up(CDIR_ENTRYptr va, CDIR_ENTRYptr vb)
{ int ret; if (!(ret = sort_type(va,vb))) if (!(ret = sort_size(va,vb))) ret = sort_name(va,vb); return ret < 0; }

static bool sort_by_name_down(CDIR_ENTRYptr va, CDIR_ENTRYptr vb) { return sort_by_name_up(vb,va); }
static bool sort_by_date_down(CDIR_ENTRYptr va, CDIR_ENTRYptr vb) { return sort_by_date_up(vb,va); }
static bool sort_by_size_down(CDIR_ENTRYptr va, CDIR_ENTRYptr vb) { return sort_by_size_up(vb,va); }

void
GLUIFileSelect::sort_dir_contents(DIR_ENTRYptr &dir, sort_t sort)
{
   assert(dir != nullptr);

   bool (*func)(CDIR_ENTRYptr, CDIR_ENTRYptr);

   switch(sort)
   {
      case SORT_NAME_UP:   func = sort_by_name_up;    break;
      case SORT_NAME_DOWN: func = sort_by_name_down;  break;
      case SORT_DATE_UP:   func = sort_by_date_up;    break;
      case SORT_DATE_DOWN: func = sort_by_date_down;  break;
      case SORT_SIZE_UP:   func = sort_by_size_up;    break;
      case SORT_SIZE_DOWN: func = sort_by_size_down;  break;
      default: assert(0); break;
   }

   if (dir->_type != DIR_ENTRY::DIR_ENTRY_ROOT) {
      std::sort(dir->_contents.begin(), dir->_contents.end(), func);
   }
}

//////////////////////////////////////////////////////
// get_selected_entry()
//////////////////////////////////////////////////////
DIR_ENTRYptr
GLUIFileSelect::get_selected_entry()
{
   if (_current_selection == -1) return nullptr;

   assert(_current_path != nullptr);
   assert( (_current_scroll + _current_selection) < (int)_current_path->_contents.size());

   return _current_path->_contents[_current_scroll + _current_selection];
}

//////////////////////////////////////////////////////
// set_selected_entry()
//////////////////////////////////////////////////////
void
GLUIFileSelect::set_selected_entry(DIR_ENTRYptr e)
{
   if (e == nullptr)
   {
      _current_selection = -1;
   }
   else
   {
      assert(_current_path != nullptr);

      DIR_ENTRYlist::iterator it = std::find(_current_path->_contents.begin(), _current_path->_contents.end(), e);

      assert(it != _current_path->_contents.end());

      int ind = it - _current_path->_contents.begin();

      ind -= _current_scroll;

      if ((ind >= 0) && (ind < GLUI_FILE_SELECT_NUM_FILES))
      {
         _current_selection = ind;
      }
      else
      {
         _current_selection = -1;
      }
   }
}

//////////////////////////////////////////////////////
// do_scroll_delta()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_scroll_delta(int delta)
{
   do_scroll_set(_current_scroll + delta);
}

//////////////////////////////////////////////////////
// do_scroll_set()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_scroll_set(int scroll)
{
   assert(_current_path != nullptr);
   assert((_current_scroll >= 0) &&
          (_current_scroll <= max(0,(int)_current_path->_contents.size() - GLUI_FILE_SELECT_NUM_FILES + 1)));

   DIR_ENTRYptr selected_entry = get_selected_entry();

   _current_scroll = scroll;

   _current_scroll = max(_current_scroll,0);

   _current_scroll = min(_current_scroll,max(0,(int)_current_path->_contents.size() - GLUI_FILE_SELECT_NUM_FILES + 1));

   set_selected_entry(selected_entry);
   
   update();   
}


//////////////////////////////////////////////////////
// do_scrollbar()
//////////////////////////////////////////////////////

void
GLUIFileSelect::do_scrollbar(int e, int x, int y, int i, int k, int m)
{
   //Middle and Right mouse button callbacks go through disabled controls...
   if (!_bitmapbox[BITMAPBOX_SCROLL_FILE]->enabled) return;

   //int w = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_image_w();
   int h = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_image_h();

   int n_delta, p_delta;

   int pix_below, pix_showing, pix_above;
   int num_below, num_showing, num_above;

   compute_scroll_geometry(h, pix_below, pix_showing, pix_above, num_below, num_showing, num_above);

   switch (e)
   {

      case GLUI_BITMAPBOX_EVENT_MOUSE_DOWN:
         // Cancel any pending wheel stuff...
         _current_scrollbar_wheel = false;

         // Clicked above scroller -- page up
         if ( y < pix_below )
         {
            _current_scrollbar_state = BAR_STATE_LOWER_DOWN;
            _current_scrollbar_state_inside = i!=0;
            update_scroll();
         }
         // Clicked above scroller -- page down
         else if ( y >= h - pix_above )
         {
            _current_scrollbar_state = BAR_STATE_UPPER_DOWN;
            _current_scrollbar_state_inside = i!=0;
            update_scroll();
         }
         // Start dragging scroller...
         else
         {
            _current_scrollbar_state = BAR_STATE_SCROLL_DOWN;
            _current_scrollbar_state_inside = i!=0;
            _current_scrollbar_state_pixel_position = y;
            _current_scrollbar_state_index_position = _current_scroll;
            _current_scrollbar_state_above_ratio = (double)num_above/(double)pix_above;
            _current_scrollbar_state_below_ratio = (double)num_below/(double)pix_below;
            update_scroll();
         }
      break;
      case GLUI_BITMAPBOX_EVENT_MOUSE_MOVE:

         // If we're dragging the scroller...
         if (_current_scrollbar_state == BAR_STATE_SCROLL_DOWN)
         {
            p_delta = _current_scrollbar_state_pixel_position - y;

            if (p_delta > 0)
            {
               n_delta = int((double)(p_delta) * _current_scrollbar_state_below_ratio + 0.5);
            }
            else if (p_delta < 0)
            {
               n_delta = int((double)(p_delta) * _current_scrollbar_state_above_ratio + 0.5);
            }
            else
            {
               n_delta = 0;
            }
            _current_scrollbar_state_inside = i!=0;

            do_scroll_set(_current_scrollbar_state_index_position + n_delta);
         }
         else if (_current_scrollbar_state == BAR_STATE_LOWER_DOWN)
         {
            bool old_i = _current_scrollbar_state_inside;
            _current_scrollbar_state_inside = i!=0;
            if ((i!=0)!=old_i) update_scroll();
         }
         else if (_current_scrollbar_state == BAR_STATE_UPPER_DOWN)
         {
            bool old_i = _current_scrollbar_state_inside;
            _current_scrollbar_state_inside = i!=0;
            if ((i!=0)!=old_i) update_scroll();
         }
      break;
      case GLUI_BITMAPBOX_EVENT_MOUSE_UP:

         // If we're dragging the scroller...
         if (_current_scrollbar_state == BAR_STATE_SCROLL_DOWN)
         {
            p_delta = _current_scrollbar_state_pixel_position - y;

            if (p_delta > 0)
            {
               n_delta = int((double)(p_delta) * _current_scrollbar_state_below_ratio + 0.5);
            }
            else if (p_delta < 0)
            {
               n_delta = int((double)(p_delta) * _current_scrollbar_state_above_ratio + 0.5);
            }
            else
            {
               n_delta = 0;
            }

            _current_scrollbar_state_inside = i!=0;
            _current_scrollbar_state = BAR_STATE_NONE;
            do_scroll_set(_current_scrollbar_state_index_position + n_delta);
         }
         else
         {
            if (i && _current_scrollbar_state == BAR_STATE_LOWER_DOWN)
            {
               n_delta = num_showing;
            }
            else if (i && _current_scrollbar_state == BAR_STATE_UPPER_DOWN)
            {
               n_delta = -num_showing;
            }
            else
            {
               n_delta = 0;            
            }

            _current_scrollbar_state_inside = i!=0;
            _current_scrollbar_state = BAR_STATE_NONE;
            do_scroll_delta(n_delta);
         }
         
      break;
      case GLUI_BITMAPBOX_EVENT_KEY:
         switch(k)
         {
            case 103: //down arrow
               do_scroll_delta(+1);
            break;
            case 101: //up arrow
               do_scroll_delta(-1);
            break;
            case 105: //page down
               do_scroll_delta(+num_showing);
            break;
            case 104: //page up
               do_scroll_delta(-num_showing);
            break;
            case 107: //end
               do_scroll_set(_current_scroll + num_showing + num_below);
            break;
            case 106: //home
               do_scroll_set(0);
            break;
            default:

            break;
         }
      break;
      case GLUI_BITMAPBOX_EVENT_MIDDLE_DOWN:
         if (_current_scrollbar_state == BAR_STATE_NONE)
         {
            _current_scrollbar_wheel = true;
            _current_scrollbar_wheel_index = _current_scroll;
            _current_scrollbar_wheel_position = y;   

            for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++)
            {
               //If a file was the active control, deactivate it.
               //Looks wired to see the files scrolling by, 
               //with a fixed file slot active...
               if (_activetext[ACTIVETEXT_NUM + i]->active)
               {
                  _glui->deactivate_current_control();
                  _glui->activate_control(_bitmapbox[BITMAPBOX_SCROLL_FILE], GLUI_ACTIVATE_MOUSE);
               }
            }
         }
      break;
      case GLUI_BITMAPBOX_EVENT_MIDDLE_MOVE:
         if (_current_scrollbar_wheel == true)
         {
            p_delta = _current_scrollbar_wheel_position - y;

            if (p_delta > 0)
            {
               n_delta = int((double)(p_delta) / 10.0);
            }
            else if (p_delta < 0)
            {
               n_delta = int((double)(p_delta) / 10.0);
            }
            else
            {
               n_delta = 0;
            }
            do_scroll_set(_current_scrollbar_wheel_index + n_delta);
         }
      break;
      case GLUI_BITMAPBOX_EVENT_MIDDLE_UP:
         if (_current_scrollbar_wheel == true)
         {
/*
            p_delta = _current_scrollbar_wheel_position - y;

            if (p_delta > 0)
            {
               n_delta = int((double)(p_delta) / 10.0);
            }
            else if (p_delta < 0)
            {
               n_delta = int((double)(p_delta) / 10.0);
            }
            else
            {
               n_delta = 0;
            }
            do_scroll_set(_current_scrollbar_wheel_index + n_delta);
*/
            _current_scrollbar_wheel = false;
         }
      break;
      case GLUI_BITMAPBOX_EVENT_RIGHT_DOWN:
      case GLUI_BITMAPBOX_EVENT_RIGHT_MOVE:
      case GLUI_BITMAPBOX_EVENT_RIGHT_UP:
      case GLUI_BITMAPBOX_EVENT_NONE:
      break;
      default:
         assert(0);
      break;
   }

}

//////////////////////////////////////////////////////
// do_up_directory()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_up_directory()
{
   bool ret = do_directory_change(_current_path->_parent->_full_path); assert(ret);
}

//////////////////////////////////////////////////////
// do_refresh()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_refresh()
{
   bool ret = do_directory_change(_current_path->_full_path); assert(ret);
}

//////////////////////////////////////////////////////
// do_delete_mode()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_delete_mode()
{
   DIR_ENTRYptr e = get_selected_entry(); 
   
   assert(e != nullptr);
   assert((e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY) ||
          (e->_type == DIR_ENTRY::DIR_ENTRY_FILE) );

   _current_mode_saved_file = _edittext[EDITTEXT_FILE]->get_text();

   if (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY)
   {
      _edittext[EDITTEXT_FILE]->set_text(("Delete directory " + e->_full_path + " ?").c_str());
   }
   else
   {
      _edittext[EDITTEXT_FILE]->set_text(("Delete file " + e->_full_path + " ?").c_str());
   }
   

   _current_mode = MODE_DELETE;

   update();
}

//////////////////////////////////////////////////////
// do_rename_mode()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_rename_mode()
{
   DIR_ENTRYptr e = get_selected_entry(); 

   assert(e != nullptr);
   assert((e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY) ||
          (e->_type == DIR_ENTRY::DIR_ENTRY_FILE) );

   _current_mode_saved_file = _edittext[EDITTEXT_FILE]->get_text();

   _edittext[EDITTEXT_FILE]->set_text(e->_name.c_str());

   _current_mode = MODE_RENAME;

   update();

}

//////////////////////////////////////////////////////
// do_add_mode()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_add_mode()
{
   assert(_current_path != nullptr);
   assert(_current_path->_type != DIR_ENTRY::DIR_ENTRY_ROOT);

   _current_mode_saved_file = _edittext[EDITTEXT_FILE]->get_text();

   _edittext[EDITTEXT_FILE]->set_text("NewFolder");

   _current_mode = MODE_ADD;

   update();

}

//////////////////////////////////////////////////////
// do_delete_action()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_delete_action()
{
   DIR_ENTRYptr e = get_selected_entry();   assert(e != nullptr);

   if (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY)
   {
      if (!rmdir_(e->_full_path))
      {
         cerr << "GLUIFileSelect::do_delete_action() - **FAILED**\n";
      }
   }
   else if (e->_type == DIR_ENTRY::DIR_ENTRY_FILE)
   {
      if (!remove_(e->_full_path))
      {
         cerr << "GLUIFileSelect::do_delete_action() - **FAILED**\n";
      }
   }
   else
   {
      assert(0); 
   }

   //_edittext[EDITTEXT_FILE]->set_text(_current_mode_saved_file.c_str());
   _edittext[EDITTEXT_FILE]->set_text("");
   _current_mode = MODE_NORMAL;

   do_refresh();
}

//////////////////////////////////////////////////////
// do_rename_action()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_rename_action()
{
   assert(_current_path != nullptr);

   DIR_ENTRYptr e = get_selected_entry();   assert(e != nullptr);

   string new_name = _current_path->_full_path +
                          (_current_path->_type == DIR_ENTRY::DIR_ENTRY_DRIVE?"":"/") +
                             _edittext[EDITTEXT_FILE]->get_text();


   if (!rename_(e->_full_path, new_name))
   {
      cerr << "GLUIFileSelect::do_rename_action() - **FAILED**\n";
   }

   //_edittext[EDITTEXT_FILE]->set_text(_current_mode_saved_file.c_str());
   _edittext[EDITTEXT_FILE]->set_text("");
   _current_mode = MODE_NORMAL;

   do_refresh();
}


//////////////////////////////////////////////////////
// do_add_action()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_add_action()
{
   assert(_current_path != nullptr);

   string new_folder = _current_path->_full_path +
                           (_current_path->_type == DIR_ENTRY::DIR_ENTRY_DRIVE?"":"/") +
                              _edittext[EDITTEXT_FILE]->get_text();

   if (!mkdir_(new_folder))
   {
      cerr << "GLUIFileSelect::do_add_action() - Failed to create directory: '" << new_folder << "'\n";
   }

   _edittext[EDITTEXT_FILE]->set_text(_current_mode_saved_file.c_str());
   _current_mode = MODE_NORMAL;

   do_refresh();
}

//////////////////////////////////////////////////////
// do_cancel_action()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_cancel_action()
{
   _edittext[EDITTEXT_FILE]->set_text(_current_mode_saved_file.c_str());
   _current_mode = MODE_NORMAL;
   update();
}


//////////////////////////////////////////////////////
// do_path_listbox()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_path_listbox()
{
   int chosen_ind = _listbox[LIST_PATH]->get_int_val();

   if (chosen_ind == -1) 
   {
      _listbox[LIST_PATH]->set_int_val(0);
   }
   else
   {
      DIR_ENTRYptr  dir = _current_path;
      DIR_ENTRYlist dirs;

      //To avoid flicking, don't actually let the 
      //listbox change to the selected item...
      //Let that happen when the directory change
      //re-fills the listbox and selects the right item...
      _listbox[LIST_PATH]->set_int_val(0);

      //Compile recursion of current path
      while (dir->_type != DIR_ENTRY::DIR_ENTRY_ROOT)
      {
         dirs.push_back(dir);
         dir = dir->_parent;
      }

      DIR_ENTRYptr chosen_dir;

      if (chosen_ind < (int)dirs.size())
      {
         chosen_dir = dirs[chosen_ind];
      }
      else if (chosen_ind == (int)dirs.size())
      {
         chosen_dir = dir;
      }
      else
      {
         chosen_dir = dir->_contents[chosen_ind - 1 - dirs.size()];
      }
      do_directory_change(chosen_dir->_full_path);
   }
}


//////////////////////////////////////////////////////
// do_edittext_event()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_edittext_event() 
{
   int reason = _edittext[EDITTEXT_FILE]->get_event_key();

   if ( (_current_mode == MODE_RENAME) ||
        (_current_mode == MODE_DELETE) ||
        (_current_mode == MODE_ADD) ) 
   {
      switch(reason)
      {
         case 13: //enter
            button_cb(BUT_ACTION);
         break;
         case 27: //esc
            button_cb(BUT_CANCEL);
         break;
         default: //tabbed or moused to another control
            //chill out
         break;
      }
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      switch(reason)
      {
         case 13: //enter
            button_cb(BUT_ACTION);
         break;
         case 27: //esc
            button_cb(BUT_CANCEL);
         break;
         default: //tabbed or moused to another control
            //chill out
         break;
      }
   }

}

//////////////////////////////////////////////////////
// do_entry_select()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_entry_select(int ind)
{
   DIR_ENTRYptr e;

   assert(ind >= -1);
   assert(ind < GLUI_FILE_SELECT_NUM_FILES);

   
   if (_current_selection == -1)
   {

      _current_selection = ind;      
      e = get_selected_entry();

      if (e == nullptr)
      {
         _current_selection_time = 0.0;
      }
      else if ((e->_type == DIR_ENTRY::DIR_ENTRY_DRIVE) ||
               (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY))

      {
         _current_selection_time = the_time();         
         update();
      }
      else if (e->_type == DIR_ENTRY::DIR_ENTRY_FILE)
      {
         _current_selection_time = the_time();         
         _edittext[EDITTEXT_FILE]->set_text(e->_name.c_str());
         update();
      }
      else
      {
         assert(0);
      }
   }
   else if (_current_selection != ind)
   {
      _current_selection = ind;      
      e = get_selected_entry();

      if (e == nullptr)
      {
         _current_selection_time = 0.0;
         update();
      }
      else if ((e->_type == DIR_ENTRY::DIR_ENTRY_DRIVE) ||
               (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY))

      {
         _current_selection_time = the_time();         
         update();
      }
      else if (e->_type == DIR_ENTRY::DIR_ENTRY_FILE)
      {
         _current_selection_time = the_time();         
         _edittext[EDITTEXT_FILE]->set_text(e->_name.c_str());
         update();
      }
      else
      {
         assert(0);
      }
   }
   else
   {
      _current_selection = ind;      
      e = get_selected_entry();

      if (e == nullptr)
      {
         _current_selection_time = 0.0;
      }
      else if ((e->_type == DIR_ENTRY::DIR_ENTRY_DRIVE) ||
               (e->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY))

      {
         double new_time = the_time();
         double delta_time = new_time - _current_selection_time;

         if (delta_time < GLUI_FILE_SELECT_DOUBLECLICK_TIME)
         {
            do_directory_change(e->_full_path);
         }
         else
         {
            _current_selection_time = new_time;
            //update();
         }
      }
      else if (e->_type == DIR_ENTRY::DIR_ENTRY_FILE)
      {
         double new_time = the_time();
         double delta_time = new_time - _current_selection_time;

         if (delta_time < GLUI_FILE_SELECT_DOUBLECLICK_TIME)
         {
            //double clicked a file -- see if the file name matches
            //the current name in the edittext box first...
            if (e->_name == _edittext[EDITTEXT_FILE]->get_text())
            {
               undisplay(OK_ACTION, 
                  _current_path->_full_path +
                     (_current_path->_type == DIR_ENTRY::DIR_ENTRY_DRIVE?"":"/"),
                  _edittext[EDITTEXT_FILE]->get_text());
            }
            //if not, just set the edittext to the clicked file's name
            else
            {
               _current_selection_time = new_time;
               _edittext[EDITTEXT_FILE]->set_text(e->_name.c_str());
               update();
            }
         }
         else
         {
            _current_selection_time = new_time;
            _edittext[EDITTEXT_FILE]->set_text(e->_name.c_str());
            update();
         }
      }
      else
      {
         assert(0);
      }
   }
   
}

//////////////////////////////////////////////////////
// do_sort_toggle()
//////////////////////////////////////////////////////
void
GLUIFileSelect::do_sort_toggle(int button)
{
   DIR_ENTRYptr selected_entry = get_selected_entry();

   switch(button)
   {
      case BUT_HEADING_NAME:
         if (_current_sort == SORT_NAME_UP) 
            _current_sort = SORT_NAME_DOWN;
         else 
            _current_sort = SORT_NAME_UP;
      break;
      case BUT_HEADING_SIZE:
         if (_current_sort == SORT_SIZE_UP) 
            _current_sort = SORT_SIZE_DOWN;
         else 
            _current_sort = SORT_SIZE_UP;
      break;
      case BUT_HEADING_DATE:
         if (_current_sort == SORT_DATE_UP) 
            _current_sort = SORT_DATE_DOWN;
         else 
            _current_sort = SORT_DATE_UP;
      break;
      default:
         assert(0);
      break;
   }

   assert(_current_path != nullptr);

   sort_dir_contents(_current_path,_current_sort);
   
   set_selected_entry(selected_entry);

   assert(_current_scroll < (int)_current_path->_contents.size());

   update();
}

//////////////////////////////////////////////////////
// generate_dir_entry()
//////////////////////////////////////////////////////
DIR_ENTRYptr
GLUIFileSelect::generate_dir_entry(const string &full_path, const string &name)
{
   DIR_ENTRYptr ret = make_shared<DIR_ENTRY>();

   if (full_path == "")
   {
      ret->_type = DIR_ENTRY::DIR_ENTRY_ROOT;
      ret->_full_path = "";
   }
   else
   {
      if (!stat_(full_path,ret))
      {
         ret = nullptr;
      }
   }

   if (ret != nullptr)
   {
      //Assign the given name and append with
      //any existing name...
      if (name != "")
      {
         ret->_name = name + ret->_name;
      }
      //Assign path name to name by default...
      else
      {
         ret->_name = ret->_full_path;
      }
   }

   return ret;
}

//////////////////////////////////////////////////////
// init()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::init()
{
   bool ret;

   ret = do_directory_change(_path);  assert(ret);

   _edittext[EDITTEXT_FILE]->set_text(_file.c_str());

   //Maintain persistent setting
   //_current_sort = SORT_NAME_UP;

   //Update path?
   //_path = _current_path->_full_path;

}

//////////////////////////////////////////////////////
// do_directory_change()
//////////////////////////////////////////////////////
bool
GLUIFileSelect::do_directory_change(const string &dir)
{
   bool ret;
 
   int old_cursor = _glui->get_cursor();
   _glui->set_cursor(GLUT_CURSOR_WAIT);

   DIR_ENTRYptr new_path = generate_dir_tree(dir);

   if (new_path == nullptr)
   {
      new_path = generate_dir_tree("");
      assert(new_path != nullptr);
      cerr << "GLUIFileSelect::do_directory_change() - Failed to open: '" << dir << 
                  "'. Falling back to '" << new_path->_name << "'\n";
      ret = false;
   }
   else
   {
      ret = true;
   }

   _current_selection = -1;

   _current_scroll = 0;

   _current_path = new_path;

   sort_dir_contents(_current_path,_current_sort);

   update();

   _glui->set_cursor(old_cursor);

   return ret;
}


//////////////////////////////////////////////////////
// generate_dir_tree()
//////////////////////////////////////////////////////
DIR_ENTRYptr
GLUIFileSelect::generate_dir_tree(const string &new_path)
{
   bool foo;

   //Return value
   DIR_ENTRYptr cur_entry, ret_entry;

   //Save the CWD before mucking about...
   string old_cwd = getcwd_();

   if (old_cwd == "")
   {
      cerr << "GLUIFileSelect::generate_dir_tree() - ERROR!! Couldn't retreive old CWD!\n";
   }

   if (new_path == "")
   {
      ret_entry = generate_dir_entry("", "Entire File System");
      foo = generate_dir_contents(ret_entry); assert(foo);
   }
   else if (chdir_(new_path))
   {
      string tmp_cwd, cur_cwd;

      cur_cwd   = getcwd_();                          assert(cur_cwd != "");
      ret_entry = generate_dir_entry(cur_cwd, "");    assert(ret_entry != nullptr);
      cur_entry = ret_entry;

      while (1)
      {
         foo = chdir_("..");   assert(foo);
         tmp_cwd = getcwd_();  assert(tmp_cwd != "");

         if (tmp_cwd == cur_cwd) break;

         cur_entry->_parent = generate_dir_entry(tmp_cwd, ""); assert(cur_entry->_parent != nullptr);

         cur_entry = cur_entry->_parent;
         cur_cwd = tmp_cwd;
      }

      //Create ROOT entry
      cur_entry->_parent = generate_dir_entry("", "Entire File System");
      cur_entry = cur_entry->_parent;

      //Get listing
      foo = generate_dir_contents(ret_entry); assert(foo);

      //Add favorites and drives (root's listing)
      foo = generate_dir_contents(cur_entry); assert(foo);

   }
   else
   {
      ret_entry = nullptr;
   }


   if (old_cwd != "")
   {
      if (!chdir_(old_cwd))
      {
         cerr << "GLUIFileSelect::generate_dir_tree() - Failed restoring old CWD...\n";
      }
   }

   return ret_entry;
}



//////////////////////////////////////////////////////
// update()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update()
{
   update_paths();
   update_files();
   update_actions();
   
}

//////////////////////////////////////////////////////
// update_paths()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_paths()
{
   update_pathlist();
   update_icons();
}

//////////////////////////////////////////////////////
// update_pathlist()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_pathlist()
{
   string foo,bar;

   int i;
   string::size_type j;

   if ( (_current_mode == MODE_RENAME) ||
        (_current_mode == MODE_DELETE) ||
        (_current_mode == MODE_ADD) ) 
   {
      _listbox[LIST_PATH]->disable();
      _listbox[LIST_PATH]->set_w(GLUI_FILE_SELECT_PATH_WIDTH);
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      // Fill up the listbox
      _listbox[LIST_PATH]->enable();
      _listbox[LIST_PATH]->set_w(GLUI_FILE_SELECT_PATH_WIDTH);

      DIR_ENTRYlist dirs;
      DIR_ENTRYptr  dir = _current_path;

      //Compile recursion of current path
      while (dir->_type != DIR_ENTRY::DIR_ENTRY_ROOT)
      {
         dirs.push_back(dir);
         dir = dir->_parent;
      }

      //Clear it out
      while (_listbox[LIST_PATH]->delete_item(-1)) {}
      for (i=0; _listbox[LIST_PATH]->delete_item(i); i++) {}

      //Add this placeholder for final selection
      //to avoid flickering...
      foo = _current_path->_name; 
      bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_PATH], bar.c_str())) bar=shorten_string(--j,foo);
      _listbox[LIST_PATH]->add_item(-2, bar.c_str());

      //Master root 
      _listbox[LIST_PATH]->add_item(-1,         "--File Systems----------------");
      foo = dir->_name; 
      bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_PATH], bar.c_str())) bar=shorten_string(--j,foo);
      _listbox[LIST_PATH]->add_item(dirs.size(), bar.c_str());

      assert(dir->_contents.size() > 0); //At least holds JOT_ROOT and drives (WIN32) or root directoy (Unix)

      i = 0;

      //Drives
      if (dir->_contents[i]->_type == DIR_ENTRY::DIR_ENTRY_DRIVE)
      {
         _listbox[LIST_PATH]->add_item(-1,         "--Drives-----------------------");
         while ( (i < (int)dir->_contents.size()) && (dir->_contents[i]->_type == DIR_ENTRY::DIR_ENTRY_DRIVE))
         {
            foo = dir->_contents[i]->_name; 
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_PATH], bar.c_str())) bar=shorten_string(--j,foo);
            _listbox[LIST_PATH]->add_item(dirs.size() + 1 + i, bar.c_str());
            i++;
         }
      }

      //Recent
      if (dir->_contents[i]->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY)
      {
         _listbox[LIST_PATH]->add_item(-1,         "--Recent-----------------------");
         while (i < (int)dir->_contents.size()) {
            assert(dir->_contents[i]->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY);
            foo = dir->_contents[i]->_name; 
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_PATH], bar.c_str())) bar=shorten_string(--j,foo);
            _listbox[LIST_PATH]->add_item(dirs.size() + 1 + i, bar.c_str());
            i++;
         }
      }

      assert(i == (int)dir->_contents.size());

      //Current
      if (dirs.size() > 0)
      {
         _listbox[LIST_PATH]->add_item(-1,         "--Current---------------------");

         for (i=dirs.size()-1; i>=0 ; i--)
         {
            foo = dirs[i]->_name; 
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_PATH], bar.c_str())) bar=shorten_string(--j,foo);
            _listbox[LIST_PATH]->add_item(i, bar.c_str());
         }
      }
      
      _listbox[LIST_PATH]->set_int_val(0);

      //Remove placeholder
      _listbox[LIST_PATH]->delete_item(-2);
   }
}

//////////////////////////////////////////////////////
// update_icons()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_icons()
{
   if ( (_current_mode == MODE_RENAME) ||
        (_current_mode == MODE_DELETE) ||
        (_current_mode == MODE_ADD) ) 
   {
      _bitmapbox[BITMAPBOX_DOT]->disable();
      _bitmapbox[BITMAPBOX_UP]->disable();
      _bitmapbox[BITMAPBOX_PLUS]->disable();
      _bitmapbox[BITMAPBOX_R]->disable();
      _bitmapbox[BITMAPBOX_X]->disable();
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      // -refresh
      _bitmapbox[BITMAPBOX_DOT]->enable();
      
      assert(_current_path != nullptr);
      // -up dir
      if (_current_path->_parent != nullptr)
      {
         assert(_current_path->_type != DIR_ENTRY::DIR_ENTRY_ROOT);
         _bitmapbox[BITMAPBOX_UP]->enable();
      }
      else
      {
         _bitmapbox[BITMAPBOX_UP]->disable();
      }
   
      // -new dir
      if ((_current_path->_type == DIR_ENTRY::DIR_ENTRY_DRIVE) ||
          (_current_path->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY))
      {
         _bitmapbox[BITMAPBOX_PLUS]->enable();
      }
      else
      {
         _bitmapbox[BITMAPBOX_PLUS]->disable();
      }

      // -rename/-delete
      DIR_ENTRYptr selected_entry = get_selected_entry();
      if ((selected_entry != nullptr) &&
          (_current_path->_type != DIR_ENTRY::DIR_ENTRY_ROOT) &&
            ( (selected_entry->_type == DIR_ENTRY::DIR_ENTRY_DIRECTORY) ||
              (selected_entry->_type == DIR_ENTRY::DIR_ENTRY_FILE)))
      {
         _bitmapbox[BITMAPBOX_R]->enable();
         _bitmapbox[BITMAPBOX_X]->enable();
      }
      else
      {
         _bitmapbox[BITMAPBOX_R]->disable();
         _bitmapbox[BITMAPBOX_X]->disable();
      }
   }
}


//////////////////////////////////////////////////////
// update_files()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_files()
{
   update_headings();
   update_listing();
   update_scroll();
}


//////////////////////////////////////////////////////
// update_headings()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_headings()
{
   if ( (_current_mode == MODE_RENAME) ||
        (_current_mode == MODE_DELETE) ||
        (_current_mode == MODE_ADD) ) 
   {
      _button[BUT_HEADING_NAME]->disable();
      _button[BUT_HEADING_SIZE]->disable();
      _button[BUT_HEADING_DATE]->disable();
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      //Re-set button labels
      assert(_current_path != nullptr);

      _button[BUT_HEADING_NAME]->set_name("Name");
      _button[BUT_HEADING_SIZE]->set_name("Size");
      _button[BUT_HEADING_DATE]->set_name("Date");

      if (_current_path->_type == DIR_ENTRY::DIR_ENTRY_ROOT)
      {
         _button[BUT_HEADING_NAME]->disable();
         _button[BUT_HEADING_SIZE]->disable();
         _button[BUT_HEADING_DATE]->disable();
      }
      else
      {
         _button[BUT_HEADING_NAME]->enable();
         _button[BUT_HEADING_SIZE]->enable();
         _button[BUT_HEADING_DATE]->enable();

         switch(_current_sort)
         {
            case SORT_NAME_UP:
               _button[BUT_HEADING_NAME]->set_name("Name [^]");
            break;
            case SORT_NAME_DOWN:
               _button[BUT_HEADING_NAME]->set_name("Name [v]");
            break;
            case SORT_DATE_UP:
               _button[BUT_HEADING_DATE]->set_name("Date [^]");
            break;
            case SORT_DATE_DOWN:
               _button[BUT_HEADING_DATE]->set_name("Date [v]");
            break;
            case SORT_SIZE_UP:
               _button[BUT_HEADING_SIZE]->set_name("Size [^]");
            break;
            case SORT_SIZE_DOWN:
               _button[BUT_HEADING_SIZE]->set_name("Size [v]");
            break;
            default:
               assert(0);
         }
      }
   }
}

//////////////////////////////////////////////////////
// shorten_string()
//////////////////////////////////////////////////////
string
GLUIFileSelect::shorten_string(string::size_type new_len, const string &str)
{
   string ret;

   string::size_type len = str.length();

   assert( new_len < len );

   //Return only new_len characters from string, chop
   //extra from the middle, and replace with ellipses...

   string::size_type starting_len   = new_len/2;
   string::size_type finishing_len  = new_len - starting_len;

   ret = str.substr(0, starting_len) + "..." + str.substr(len-finishing_len);

   return ret;
}

//////////////////////////////////////////////////////
// update_listing()
//////////////////////////////////////////////////////
#define CHR_BUF_SIZE 1024
void     
GLUIFileSelect::update_listing()
{
   int i;
   string::size_type j;

   if ( (_current_mode == MODE_RENAME) ||
        (_current_mode == MODE_DELETE) ||
        (_current_mode == MODE_ADD) ) 
   {
      for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++)
      {
         _bitmapbox[BITMAPBOX_NUM + i]->disable();
         _activetext[ACTIVETEXT_NUM + i]->disable();
         _activetext[ACTIVETEXT_NUM + i]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);
         _statictext[STATICTEXT_NUM + 2*i]->disable();
         _statictext[STATICTEXT_NUM + 2*i]->set_w(GLUI_FILE_SELECT_SIZE_WIDTH);
         _statictext[STATICTEXT_NUM + 2*i + 1]->disable();
         _statictext[STATICTEXT_NUM + 2*i + 1]->set_w(GLUI_FILE_SELECT_DATE_WIDTH);      
      }
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      for (i=0;i<GLUI_FILE_SELECT_NUM_FILES;i++)
      {
         if ( (_current_path != nullptr) &&
              (_current_scroll+i < (int)_current_path->_contents.size()) )
         {
            int         ret;
            char        chr_buf[CHR_BUF_SIZE];
            string      foo, bar;
            IconBitmap  *bm;
            DIR_ENTRYptr e = _current_path->_contents[_current_scroll+i]; assert(e != nullptr);

            switch(e->_type)
            {
               case DIR_ENTRY::DIR_ENTRY_DRIVE:       bm = &_bitmaps[BITMAP_DRIVE];   break;
               case DIR_ENTRY::DIR_ENTRY_DIRECTORY:   bm = &_bitmaps[BITMAP_FOLDER];  break;
               case DIR_ENTRY::DIR_ENTRY_FILE:        bm = &_bitmaps[BITMAP_DOC];     break;
               default: assert(0); break;
            }
            _bitmapbox[BITMAPBOX_NUM + i]->enable();
            _bitmapbox[BITMAPBOX_NUM + i]->set_img_size(bm->_width, bm->_height);
            _bitmapbox[BITMAPBOX_NUM + i]->copy_img(bm->_data, bm->_width, bm->_height, 3);

            _activetext[ACTIVETEXT_NUM + i]->enable();
            _activetext[ACTIVETEXT_NUM + i]->set_name(" ");
            _activetext[ACTIVETEXT_NUM + i]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);
            foo = e->_name; 
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_activetext[ACTIVETEXT_NUM + i], bar.c_str())) bar=shorten_string(--j,foo);
            _activetext[ACTIVETEXT_NUM + i]->set_text(bar.c_str());
            _activetext[ACTIVETEXT_NUM + i]->set_highlighted(i == _current_selection);

            //XXX - Truncate...
            if      (e->_size >= 1e13) sprintf(chr_buf,"%3.f TB", e->_size/1e12);
            else if (e->_size >= 1e10) sprintf(chr_buf,"%3.f GB", e->_size/1e9);
            else if (e->_size >= 1e7)  sprintf(chr_buf,"%3.f MB", e->_size/1e6);
            else if (e->_size >= 1e3)  sprintf(chr_buf,"%3.f KB", e->_size/1e3);
            else if (e->_size >= 0)    sprintf(chr_buf,"%3.f B",  e->_size);
            else                       chr_buf[0]=0;

            _statictext[STATICTEXT_NUM + 2*i]->enable();
            _statictext[STATICTEXT_NUM + 2*i]->set_name(" ");
            _statictext[STATICTEXT_NUM + 2*i]->set_w(GLUI_FILE_SELECT_SIZE_WIDTH);
            foo = chr_buf; 
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_statictext[STATICTEXT_NUM + 2*i], bar.c_str())) bar=shorten_string(--j,foo);
            _statictext[STATICTEXT_NUM + 2*i]->set_text(bar.c_str());

            //XXX - Truncate...
            if (e->_date > 0)
            {
               ret = strftime(chr_buf, CHR_BUF_SIZE, "%m/%d/%Y %I:%M %p", localtime(&e->_date)); assert(ret != 0);
            }
            else
            {
               chr_buf[0] = 0;
            }
            _statictext[STATICTEXT_NUM + 2*i + 1]->enable();
            _statictext[STATICTEXT_NUM + 2*i + 1]->set_name(" ");
            _statictext[STATICTEXT_NUM + 2*i + 1]->set_w(GLUI_FILE_SELECT_DATE_WIDTH);      
            foo = chr_buf; 
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_statictext[STATICTEXT_NUM + 2*i + 1], bar.c_str())) bar=shorten_string(--j,foo);
            _statictext[STATICTEXT_NUM + 2*i + 1]->set_text(bar.c_str());
         }
         else
         {
            IconBitmap *bm = &_bitmaps[BITMAP_BLANK];
            
            if ( (_current_path->_contents.size() == 0) && (i==0) )
            {
               _bitmapbox[BITMAPBOX_NUM + i]->enable();
               _bitmapbox[BITMAPBOX_NUM + i]->set_img_size(bm->_width, bm->_height);
               _bitmapbox[BITMAPBOX_NUM + i]->copy_img(bm->_data, bm->_width, bm->_height, 3);
         
               _activetext[ACTIVETEXT_NUM + i]->disable();
               _activetext[ACTIVETEXT_NUM + i]->set_text("Empty");
               _activetext[ACTIVETEXT_NUM + i]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);
               _activetext[ACTIVETEXT_NUM + i]->set_highlighted(false);
               assert(i != _current_selection);

               _statictext[STATICTEXT_NUM + 2*i]->disable();
               _statictext[STATICTEXT_NUM + 2*i]->set_text("");
               _statictext[STATICTEXT_NUM + 2*i]->set_w(GLUI_FILE_SELECT_SIZE_WIDTH);
         
               _statictext[STATICTEXT_NUM + 2*i + 1]->disable();
               _statictext[STATICTEXT_NUM + 2*i + 1]->set_text("");
               _statictext[STATICTEXT_NUM + 2*i + 1]->set_w(GLUI_FILE_SELECT_DATE_WIDTH);   
            }
            else
            {
               _bitmapbox[BITMAPBOX_NUM + i]->enable();
               _bitmapbox[BITMAPBOX_NUM + i]->set_img_size(bm->_width, bm->_height);
               _bitmapbox[BITMAPBOX_NUM + i]->copy_img(bm->_data, bm->_width, bm->_height, 3);
         
               _activetext[ACTIVETEXT_NUM + i]->disable();
               _activetext[ACTIVETEXT_NUM + i]->set_text("");
               _activetext[ACTIVETEXT_NUM + i]->set_w(GLUI_FILE_SELECT_NAME_WIDTH);
               _activetext[ACTIVETEXT_NUM + i]->set_highlighted(false);
               assert(i != _current_selection);

               _statictext[STATICTEXT_NUM + 2*i]->disable();
               _statictext[STATICTEXT_NUM + 2*i]->set_text("");
               _statictext[STATICTEXT_NUM + 2*i]->set_w(GLUI_FILE_SELECT_SIZE_WIDTH);
         
               _statictext[STATICTEXT_NUM + 2*i + 1]->disable();
               _statictext[STATICTEXT_NUM + 2*i + 1]->set_text("");
               _statictext[STATICTEXT_NUM + 2*i + 1]->set_w(GLUI_FILE_SELECT_DATE_WIDTH);   
            }
         }
      }
   }
}

//////////////////////////////////////////////////////
// update_scroll()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_scroll()
{
   int i,j;

   if ( (_current_mode == MODE_RENAME) ||
        (_current_mode == MODE_DELETE) ||
        (_current_mode == MODE_ADD) ) 
   {
      _bitmapbox[BITMAPBOX_DOWN_FILE]->disable();
      _bitmapbox[BITMAPBOX_UP_FILE]->disable();
      _bitmapbox[BITMAPBOX_SCROLL_FILE]->disable();
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      //Otherwise...

      int w = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_image_w();
      int h = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_image_h();

      int pix_below, pix_showing, pix_above;
      int num_below, num_showing, num_above;

      IconBitmap  slider(w,h);
      IconBitmap *proto = &_bitmaps[BITMAP_SCROLL];  
      assert(proto && (proto->_width == w));

      compute_scroll_geometry(h, pix_below, pix_showing, pix_above, num_below, num_showing, num_above);

      i=0;

      if (pix_below)
      {
         slider.copy(0, 5, proto, 0, i, w, 1); i++;
         if ((_current_scrollbar_state == BAR_STATE_LOWER_DOWN) && _current_scrollbar_state_inside)
         {
            for (j=1; j<pix_below; j++)      { slider.copy(0, 5, proto, 0, i, w, 1); i++; }
         }
         else
         {
            for (j=1; j<pix_below; j++)      { slider.copy(0, 6, proto, 0, i, w, 1); i++; }
         }
      }
   
      slider.copy(0, 0, proto, 0, i, w, 2); i+=2;
      for (j=2; j<(pix_showing-2); j++)   { slider.copy(0, 2, proto, 0, i, w, 1); i++; }
      slider.copy(0, 3, proto, 0, i, w, 2); i+=2;

      if (pix_above)
      {
         if ((_current_scrollbar_state == BAR_STATE_UPPER_DOWN) && _current_scrollbar_state_inside)
         {
            for (j=0; j<(pix_above-1); j++)  { slider.copy(0, 7, proto, 0, i, w, 1); i++; }
         }
         else
         {
            for (j=0; j<(pix_above-1); j++)  { slider.copy(0, 6, proto, 0, i, w, 1); i++; }
         }
         slider.copy(0, 7, proto, 0, i, w, 1); i++;
      }

      assert(i==h);

      _bitmapbox[BITMAPBOX_SCROLL_FILE]->copy_img(slider._data, slider._width, slider._height ,3);


      if (pix_below)
      {
         _bitmapbox[BITMAPBOX_DOWN_FILE]->enable();
      }
      else
      {
         _bitmapbox[BITMAPBOX_DOWN_FILE]->disable();
      }

      if (pix_above)
      {
         _bitmapbox[BITMAPBOX_UP_FILE]->enable();
      }
      else
      {
         _bitmapbox[BITMAPBOX_UP_FILE]->disable();
      }

      if (pix_above || pix_below)
      {
         _bitmapbox[BITMAPBOX_SCROLL_FILE]->enable();
      }
      else
      {
         _bitmapbox[BITMAPBOX_SCROLL_FILE]->disable();
      }
   }
}

//////////////////////////////////////////////////////
// compute_scroll_geometry()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::compute_scroll_geometry(int h, int &pix_below, int &pix_showing, int &pix_above,
                                               int &num_below, int &num_showing, int &num_above)
{
   assert(_current_path != nullptr);
   assert((_current_scroll >= 0) &&
          (_current_scroll <= max(0,(int)_current_path->_contents.size() - GLUI_FILE_SELECT_NUM_FILES + 1)));

   double n = _current_path->_contents.size();

   double n_above    = _current_scroll;
   double n_showing  = min(n - n_above , GLUI_FILE_SELECT_NUM_FILES - 1.0);
   double n_below    = n - n_above - n_showing;

   double n_ab       = n_above + n_below;

   double p_showing = max((double)GLUI_FILE_SELECT_SCROLL_MIN, h * ((n>0.0)?(1.0 - n_above/n - n_below/n):(0.0)));
   double p_above = (h-p_showing) * ((n_ab>0.0)?(n_above/n_ab):(0.0));
   double p_below = (h-p_showing) * ((n_ab>0.0)?(n_below/n_ab):(0.0));

   num_above   = int(n_above);
   num_showing = int(n_showing);
   num_below   = int(n_below);

   pix_showing = int(p_showing);
   pix_above   = int(p_above);
   pix_below   = int(p_below);

   int leftover = h - (pix_below + pix_above + pix_showing);

   if (leftover)
   {
      if ((pix_below < pix_above) && (num_below))
      {
         pix_below += leftover;
      }
      else if (num_above)
      {
         pix_above += leftover;
      }
      else
      {
         pix_showing += leftover;
      }
   }

}

//////////////////////////////////////////////////////
// update_actions()
//////////////////////////////////////////////////////
void     
GLUIFileSelect::update_actions()
{
   int i;
   string::size_type j;
   vector<string>::size_type k;

   if (_current_mode == MODE_RENAME)
   {
      _listbox[LIST_FILTER]->disable();
      _listbox[LIST_FILTER]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

      _statictext[STATICTEXT_LABEL_DOT]->disable();
      _checkbox[CHECKBOX_DOT]->disable();

      _edittext[EDITTEXT_FILE]->set_name("Rename ");
      _edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH - 
                                    (_edittext[EDITTEXT_FILE]->string_width(_listbox[LIST_FILTER]->get_name()) - 
                                             _edittext[EDITTEXT_FILE]->string_width(_edittext[EDITTEXT_FILE]->get_name())));
      _edittext[EDITTEXT_FILE]->enable();
      _glui->activate_control(_edittext[EDITTEXT_FILE],GLUI_ACTIVATE_TAB);

      _button[BUT_ACTION]->set_name("RENAME");
      _button[BUT_ACTION]->enable();

      _button[BUT_CANCEL]->set_name("Cancel");
      _button[BUT_CANCEL]->enable();
   }
   else if (_current_mode == MODE_DELETE)
   {
      _listbox[LIST_FILTER]->disable();
      _listbox[LIST_FILTER]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

      _statictext[STATICTEXT_LABEL_DOT]->disable();
      _checkbox[CHECKBOX_DOT]->disable();

      _edittext[EDITTEXT_FILE]->set_name(" ");
      _edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH - 
                                    (_edittext[EDITTEXT_FILE]->string_width(_listbox[LIST_FILTER]->get_name()) - 
                                             _edittext[EDITTEXT_FILE]->string_width(_edittext[EDITTEXT_FILE]->get_name())));
      _edittext[EDITTEXT_FILE]->disable();

      _button[BUT_ACTION]->set_name("DELETE");
      _button[BUT_ACTION]->enable();

      _button[BUT_CANCEL]->set_name("Cancel");
      _button[BUT_CANCEL]->enable();
      _glui->activate_control(_button[BUT_CANCEL],GLUI_ACTIVATE_TAB);
   }
   else if (_current_mode == MODE_ADD) 
   {
      _listbox[LIST_FILTER]->disable();
      _listbox[LIST_FILTER]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

      _statictext[STATICTEXT_LABEL_DOT]->disable();
      _checkbox[CHECKBOX_DOT]->disable();

      _edittext[EDITTEXT_FILE]->set_name("Create ");
      _edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH - 
                                    (_edittext[EDITTEXT_FILE]->string_width(_listbox[LIST_FILTER]->get_name()) - 
                                             _edittext[EDITTEXT_FILE]->string_width(_edittext[EDITTEXT_FILE]->get_name())));
      _edittext[EDITTEXT_FILE]->enable();
      _glui->activate_control(_edittext[EDITTEXT_FILE],GLUI_ACTIVATE_TAB);

      _button[BUT_ACTION]->set_name("CREATE");
      _button[BUT_ACTION]->enable();

      _button[BUT_CANCEL]->set_name("Cancel");
      _button[BUT_CANCEL]->enable();
   }
   else
   {
      assert(_current_mode == MODE_NORMAL);

      assert(_current_path != nullptr);

      if (_current_path->_type == DIR_ENTRY::DIR_ENTRY_ROOT)
      {
         _listbox[LIST_FILTER]->disable();
         _listbox[LIST_FILTER]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

         _statictext[STATICTEXT_LABEL_DOT]->disable();
         _checkbox[CHECKBOX_DOT]->disable();

         _button[BUT_ACTION]->disable();
         _button[BUT_ACTION]->set_name(_action.c_str());

         _button[BUT_CANCEL]->enable();
         _button[BUT_CANCEL]->set_name("Cancel");

         _edittext[EDITTEXT_FILE]->disable();
         _edittext[EDITTEXT_FILE]->set_name("Filename ");
         _edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH - 
                                       (_edittext[EDITTEXT_FILE]->string_width(_listbox[LIST_FILTER]->get_name()) - 
                                                _edittext[EDITTEXT_FILE]->string_width(_edittext[EDITTEXT_FILE]->get_name())));
      }
      else
      {
         string foo, bar;

         assert(_filter < _filters.size());

         _listbox[LIST_FILTER]->enable();
         _listbox[LIST_FILTER]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH);

         _statictext[STATICTEXT_LABEL_DOT]->enable();
         _checkbox[CHECKBOX_DOT]->enable();

         //Clear it out
         for (i=0; _listbox[LIST_FILTER]->delete_item(i); i++) {}

         //Add this placeholder for final selection to avoid flickering...
         foo = get_filter();
         bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_FILTER], bar.c_str())) bar=shorten_string(--j,foo);
         _listbox[LIST_FILTER]->add_item(-1, bar.c_str());

         for (k=0; k < _filters.size(); k++) {
            foo = _filters[k];
            bar=foo;j=foo.length();while(!jot_check_glui_fit(_listbox[LIST_FILTER], bar.c_str())) bar=shorten_string(--j,foo);
            _listbox[LIST_FILTER]->add_item(k, bar.c_str());
         }

         _listbox[LIST_FILTER]->set_int_val(_filter);

         //Remove placeholder
         _listbox[LIST_FILTER]->delete_item(-1);

         _button[BUT_ACTION]->enable();
         _button[BUT_ACTION]->set_name(_action.c_str());

         _button[BUT_CANCEL]->enable();
         _button[BUT_CANCEL]->set_name("Cancel");

         _edittext[EDITTEXT_FILE]->enable();
         _edittext[EDITTEXT_FILE]->set_name("Filename ");
         _edittext[EDITTEXT_FILE]->set_w(GLUI_FILE_SELECT_FILTER_WIDTH - 
                                       (_edittext[EDITTEXT_FILE]->string_width(_listbox[LIST_FILTER]->get_name()) - 
                                                _edittext[EDITTEXT_FILE]->string_width(_edittext[EDITTEXT_FILE]->get_name())));

      }
   }
}

//////////////////////////////////////////////////////
// checkbox_cb()
//////////////////////////////////////////////////////
void
GLUIFileSelect::checkbox_cb(int id)
{
   switch(id)
   {
      case CHECKBOX_DOT:
         do_refresh();
      break;
      default:
         assert(0);
   }
}

//////////////////////////////////////////////////////
// listbox_cb()
//////////////////////////////////////////////////////
void
GLUIFileSelect::listbox_cb(int id)
{
   switch(id)
   {
      case LIST_FILTER:
         if ((int)_filter != _listbox[LIST_FILTER]->get_int_val()) {
            _filter = _listbox[LIST_FILTER]->get_int_val();
            do_refresh();
         }
      break;
      case LIST_PATH:
         do_path_listbox();
      break;
      default:
         assert(0);
   }
}

//////////////////////////////////////////////////////
// edittext_cb()
//////////////////////////////////////////////////////
void
GLUIFileSelect::edittext_cb(int id)
{
   switch(id)
   {
      case EDITTEXT_FILE:
         do_edittext_event();
      break;
      default:
         assert(0);
   }
}

//////////////////////////////////////////////////////
// button_cb()
//////////////////////////////////////////////////////
void
GLUIFileSelect::button_cb(int id)
{
   if (_current_mode != MODE_NORMAL)
   {
      switch(id)
      {
         case BUT_ACTION:
            if (_current_mode == MODE_RENAME)
            {
               do_rename_action();
            }
            else if (_current_mode == MODE_DELETE)
            {
               do_delete_action();
            }
            else if (_current_mode == MODE_ADD) 
            {
               do_add_action();
            }  
            else
            {
               assert(0);
            }
         break;
         case BUT_CANCEL:
            do_cancel_action();
         break;
         default:
            assert(0);
         break;
      }
   }
   else
   {
      assert(_current_path != nullptr);

      string text;
      string filter_chars, bad_chars;
      
      filter_chars += "*";
      filter_chars += "?";

      bad_chars += "/";
      bad_chars += "\\";
      
#ifdef WIN32
      bad_chars += ";";
      bad_chars += "\"";
      bad_chars += "<";
      bad_chars += ">";
      bad_chars += "|";
#endif

      switch(id)
      {
         case BUT_ACTION:
            //If the filename contains a filter character, we must be trying
            //to change filters... go for it.
            text = _edittext[EDITTEXT_FILE]->get_text();
            if (text.find_first_of(filter_chars) != string::npos)
            {
               set_filter(text);
               _edittext[EDITTEXT_FILE]->set_text("");
               do_refresh();
            }
            else if (text.find_first_of(bad_chars) != string::npos)
            {
               _edittext[EDITTEXT_FILE]->set_text("BadFilename");
               _glui->activate_control(_edittext[EDITTEXT_FILE],GLUI_ACTIVATE_TAB);
               do_refresh();
            }
            else
            {
               //Only exit successfully with a valid directory... 
               //The file root doesn't count!
               if (_current_path->_type != DIR_ENTRY::DIR_ENTRY_ROOT)
               {
                  undisplay(OK_ACTION,
                        _current_path->_full_path +
                           (_current_path->_type == DIR_ENTRY::DIR_ENTRY_DRIVE?"":"/"),
                        _edittext[EDITTEXT_FILE]->get_text());
               }
               else
               {
                  //Shouldn't be able to get here...
                  assert(0);
               }
            }
         break;
         case BUT_CANCEL:
            undisplay(CANCEL_ACTION, _path, _file);
         break;
         case BUT_HEADING_NAME:
         case BUT_HEADING_SIZE:
         case BUT_HEADING_DATE:
            do_sort_toggle(id);
         break;
         default:
            assert(0);
      }
   }
}

//////////////////////////////////////////////////////
// bitmapbox_cb()
//////////////////////////////////////////////////////
void
GLUIFileSelect::bitmapbox_cb(int id)
{
   if (id == BITMAPBOX_SCROLL_FILE)
   {
      int e = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_event();
      int x = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_event_x();
      int y = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_event_y();
      int i = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_event_in();
      int k = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_event_key();
      int m = _bitmapbox[BITMAPBOX_SCROLL_FILE]->get_event_mod();
      do_scrollbar(e,x,y,i,k,m);
   }
   else
   {
      switch (_bitmapbox[id]->get_event())
      {
         case GLUI_BITMAPBOX_EVENT_NONE:
         case GLUI_BITMAPBOX_EVENT_MOUSE_DOWN:
         case GLUI_BITMAPBOX_EVENT_MOUSE_MOVE:
         case GLUI_BITMAPBOX_EVENT_KEY:
         case GLUI_BITMAPBOX_EVENT_MIDDLE_DOWN:
         case GLUI_BITMAPBOX_EVENT_MIDDLE_MOVE:
         case GLUI_BITMAPBOX_EVENT_MIDDLE_UP:
         case GLUI_BITMAPBOX_EVENT_RIGHT_DOWN:
         case GLUI_BITMAPBOX_EVENT_RIGHT_MOVE:
         case GLUI_BITMAPBOX_EVENT_RIGHT_UP:
            //Do nothing for these events...
         break;
         case GLUI_BITMAPBOX_EVENT_MOUSE_UP:
            //Do respeective event on mouse up
            //if the pointer's still inside the
            //bitmap...
            if (_bitmapbox[id]->get_event_in())
            {
               switch(id)
               {
                  case BITMAPBOX_UP_FILE:
                     do_scroll_delta(-1);
                  break;
                  case BITMAPBOX_DOWN_FILE:
                     do_scroll_delta(+1);
                  break;
                  case BITMAPBOX_UP:
                     do_up_directory();
                  break;
                  case BITMAPBOX_DOT:
                     do_refresh();
                  break;
                  case BITMAPBOX_R:
                     do_rename_mode();
                  break;
                  case BITMAPBOX_X:
                     do_delete_mode();
                  break;
                  case BITMAPBOX_PLUS:
                     do_add_mode();
                  break;
                  default:
                     assert(0);
                  break;
               }
            }
         break;
         default:
            assert(0);
         break;
      }
   }
}

//////////////////////////////////////////////////////
// activetext_cb()
//////////////////////////////////////////////////////
void
GLUIFileSelect::activetext_cb(int id)
{

   if (id < ACTIVETEXT_NUM)
   {
      //Switch on possible 'named' active text controls...
/*      
      switch(id)
      {
         case ACTIVETEXT_XXX:

         break;
         default:
            assert(0);
         break;
      }
*/
   }
   else
   {
      do_entry_select(id-ACTIVETEXT_NUM);
   }

}
