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
#ifndef GLUI_DIALOGS_H_INCLUDED
#define GLUI_DIALOGS_H_INCLUDED

#include "widgets/file_select.H"
#include "widgets/alert_box.H" 


class GLUI;
class GLUI_Panel;
class GLUI_Slider;
class GLUI_Button;
class GLUI_Rollout;
class GLUI_Listbox;
class GLUI_EditText;
class GLUI_Checkbox;
class GLUI_BitmapBox;
class GLUI_StaticText;
class GLUI_ActiveText;
class GLUI_RadioGroup;
class GLUI_RadioButton;
class GLUT_WINSYS;

/*****************************************************************
 * GLUIPopUp
 *****************************************************************/

#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

class GLUIPopUp {
 protected:
   /******** MEMBER CLASSES ********/
   class IconBitmap {
      public:
         int      _width, _height;
         uchar*   _data;

         IconBitmap(int w, int h) : _width(0), _height(0), _data(NULL) { set_size(w,h); }
         IconBitmap() : _width(0), _height(0), _data(NULL) {}
         ~IconBitmap() { if (_data) delete[] _data; }

         void set(int *icon) 
         { 
            set_size(icon[0],icon[1]);
            // Input icon arrays have origin in upper left,
            // we prefer lower left...
            for (int y=0; y<_height; y++)
            {
               for (int x=0; x<_width*3; x++)
               {
                  _data[3*_width*(_height-1-y)+x] = icon[3*_width*y+x + 2];
               }
            }
         }
         void set_size(int w, int h)
         {
            if (_data) delete[] _data; _width = w; _height = h;
            _data = new uchar[_width * _height * 3]; assert(_data);  
         }
         void copy( int sx, int sy, IconBitmap *src, 
                    int dx, int dy, int w, int h)
         {
            IconBitmap *dst = this;

            assert(src); 
            //if ((w==-1) || (h==-1)) { w = src->_width; h = src->_height; }
            assert((sx >= 0) && ( sx    <  src->_width));
            assert((sy >= 0) && ( sy    <  src->_height));
            assert(( w >  0) && ((sx+w) <= src->_width));
            assert(( h >  0) && ((sy+h) <= src->_height));
            assert(dst);
            assert((dx >= 0) && ( dx    <  dst->_width));
            assert((dy >= 0) && ( dy    <  dst->_height));
            assert(             ((dx+w) <= dst->_width));
            assert(             ((dy+h) <= dst->_height));

            for (int x=0; x<w; x++)
            {
               for (int y=0; y<h; y++)
               {
                  dst->_data[3*((dy+y)*dst->_width + (dx+x)) + 0] = src->_data[3*((sy+y)*src->_width + (sx+x)) + 0];
                  dst->_data[3*((dy+y)*dst->_width + (dx+x)) + 1] = src->_data[3*((sy+y)*src->_width + (sx+x)) + 1];
                  dst->_data[3*((dy+y)*dst->_width + (dx+x)) + 2] = src->_data[3*((sy+y)*src->_width + (sx+x)) + 2];
               }
            }
         }
   };

 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static ARRAY<GLUIPopUp*>      _ui;

 public :    
   /******** STATIC MEMBER METHODS ********/
   static void                   slider_cbs(int id);
   static void                   button_cbs(int id);
   static void                   listbox_cbs(int id);
   static void                   edittext_cbs(int id);
   static void                   checkbox_cbs(int id);
   static void                   bitmapbox_cbs(int id);
   static void                   activetext_cbs(int id);
   static void                   radiogroup_cbs(int id);

 protected:
   /******** MEMBER VARIABLES ********/
   
   GLUT_WINSYS*                  _glut_winsys;
   GLUI*                         _glui;

   bool                          _blocking;

   int                           _id;

   ARRAY<GLUI_Panel*>            _panel;
   ARRAY<GLUI_Button*>           _button;
   ARRAY<GLUI_Slider*>           _slider;
   ARRAY<GLUI_Rollout*>          _rollout;
   ARRAY<GLUI_Listbox*>          _listbox;
   ARRAY<GLUI_EditText*>         _edittext;
   ARRAY<GLUI_Checkbox*>         _checkbox;
   ARRAY<GLUI_BitmapBox*>        _bitmapbox;
   ARRAY<GLUI_StaticText*>       _statictext;
   ARRAY<GLUI_ActiveText*>       _activetext;
   ARRAY<GLUI_RadioGroup*>       _radiogroup;
   ARRAY<GLUI_RadioButton*>      _radiobutton;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public  :
   GLUIPopUp(GLUT_WINSYS *w);
   virtual ~GLUIPopUp();

 protected:
   /******** MEMBER METHODS ********/
   virtual bool      is_showing() { return _glui != NULL; }

   virtual bool      show_glui(bool blocking);
   virtual bool      hide_glui();
   virtual void      build_glui();
   virtual void      unbuild_glui();

   virtual void      slider_cb(int id) {}
   virtual void      button_cb(int id) {}
   virtual void      listbox_cb(int id) {}
   virtual void      edittext_cb(int id) {}
   virtual void      checkbox_cb(int id) {}
   virtual void      bitmapbox_cb(int id) {}
   virtual void      activetext_cb(int id) {}
   virtual void      radiogroup_cb(int id) {}

};


/*****************************************************************
 * GLUIAlertBox
 *****************************************************************/

class GLUIAlertBox : public AlertBox, public GLUIPopUp  {
 protected:
   /******** WIDGET IDs ********/
   enum bitmapbox_id_t {
      BITMAPBOX_ICON = 0,
      BITMAPBOX_NUM
   };

   enum panel_id_t {
      PANEL_TEXT=0,
      PANEL_BUTTONS,
      PANEL_NUM
   };

 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static bool       _icon_init;
   static IconBitmap _icons[ICON_NUM];

 public :    
   /******** STATIC MEMBER METHODS ********/

 protected:
   /******** MEMBER VARIABLES ********/
   alert_cb_t        _cb;
   void*             _vp;
   void*             _vpd;
   int               _idx;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public  :
   GLUIAlertBox(GLUT_WINSYS *w);
   virtual ~GLUIAlertBox();

   /******** AlertBox METHODS ********/
 protected:   
   virtual bool      undisplay(int button);

 public:
   virtual bool      display(bool blocking, alert_cb_t cb, void *vp, void *vpd, int idx);

   virtual bool      is_displaying();

   /******** PopUp METHODS ********/
 protected:      
   virtual void      build_glui();
   virtual void      unbuild_glui();

   virtual void      button_cb(int id);
   virtual void      bitmapbox_cb(int id) {}

};

/*****************************************************************
 * GLUIFileSelect
 *****************************************************************/

MAKE_PTR_BASEC(DIR_ENTRY);

#define CDIR_ENTRYlist const DIR_ENTRYlist

class DIR_ENTRYlist : public LIST<DIR_ENTRYptr> 
{ 
 public :
   DIR_ENTRYlist(int num=8) : LIST<DIR_ENTRYptr>(num)  { }
};

#define CDIR_ENTRYptr const DIR_ENTRYptr

class DIR_ENTRY : public REFcounter {
 public:
   enum dir_entry_t {
      DIR_ENTRY_ROOT,
      DIR_ENTRY_DRIVE,
      DIR_ENTRY_DIRECTORY,
      DIR_ENTRY_FILE,
      DIR_ENTRY_UNKNOWN
   };

 public:

   DIR_ENTRYptr      _parent;
   DIR_ENTRYlist     _contents;

   str_ptr           _name;
   str_ptr           _full_path;

   double            _size;  
   time_t            _date;

   dir_entry_t       _type;
   
 public:

   DIR_ENTRY()       { clear(); }
   ~DIR_ENTRY()      { clear(); }

   void              clear()
   {
      _parent = NULL;
      _contents.clear();
      _name = NULL_STR;
      _full_path = NULL_STR;
      _size = -1.0;
      _date = 0;
      _type = DIR_ENTRY_UNKNOWN;

   }

};


class GLUIFileSelect : public FileSelect, public GLUIPopUp {
 protected:
   /******** ENUMS ********/
   enum bitmapbox_id_t {
      BITMAPBOX_ICON = 0,
      BITMAPBOX_R,
      BITMAPBOX_UP,
      BITMAPBOX_DOT,
      BITMAPBOX_PLUS,
      BITMAPBOX_X,
      BITMAPBOX_UP_FILE,
      BITMAPBOX_SCROLL_FILE,
      BITMAPBOX_DOWN_FILE,
      BITMAPBOX_NUM
   };

   enum statictext_id_t {
      STATICTEXT_SPACER_PATH=0,
      STATICTEXT_SPACER_R,
      STATICTEXT_SPACER_UP,
      STATICTEXT_SPACER_DOT,
      STATICTEXT_SPACER_X,
      STATICTEXT_SPACER_PLUS,
      //STATICTEXT_SPACER_PATH_MARGIN1,
      STATICTEXT_SPACER_PATH_MARGIN2,
      STATICTEXT_SPACER_FILES_TYPE,
      STATICTEXT_SPACER_FILES_NAME,
      STATICTEXT_SPACER_FILES_SIZE,
      STATICTEXT_SPACER_FILES_DATE,
      STATICTEXT_SPACER_FILES_SCROLL,
      //STATICTEXT_SPACER_ACTION_MARGIN1,
      //STATICTEXT_SPACER_ACTION_MARGIN2,
      STATICTEXT_LABEL_DOT,
      STATICTEXT_NUM
   };

   enum activetext_id_t {
      ACTIVETEXT_NUM=0
   };

   enum edittext_id_t {
      EDITTEXT_FILE = 0,
      EDITTEXT_NUM
   };

   enum checkbox_id_t {
      CHECKBOX_DOT = 0,
      CHECKBOX_NUM
   };

   enum listbox_id_t {
      LIST_PATH=0,
      LIST_FILTER,
      LIST_NUM
   };

   enum panel_id_t {
      PANEL_PATH=0,
      PANEL_FILES,
      PANEL_ACTION,
      PANEL_NUM
   };

   enum button_id_t {
      BUT_ACTION=0,
      BUT_CANCEL,
      BUT_HEADING_TYPE, 
      BUT_HEADING_NAME,
      BUT_HEADING_SIZE,
      BUT_HEADING_DATE,
      BUT_HEADING_SCROLL,
      BUT_NUM
   };

   enum bitmap_t {
      BITMAP_R = 0,
      BITMAP_UP,
      BITMAP_DOT,
      BITMAP_X,
      BITMAP_PLUS,
      BITMAP_UPARROW,
      BITMAP_DOWNARROW,
      BITMAP_SCROLL,
      BITMAP_FOLDER,
      BITMAP_DOC,
      BITMAP_DRIVE,
      BITMAP_BLANK,
      BITMAP_NUM
   };

   enum sort_t {
      SORT_NAME_UP = 0,
      SORT_NAME_DOWN,
      SORT_DATE_UP,
      SORT_DATE_DOWN,
      SORT_SIZE_UP,
      SORT_SIZE_DOWN
   };

   enum scrollbar_state_t {
      BAR_STATE_NONE = 0,
      BAR_STATE_UPPER_DOWN,
      BAR_STATE_LOWER_DOWN,
      BAR_STATE_SCROLL_DOWN
   };

   enum mode_t {
      MODE_NORMAL = 0,
      MODE_ADD,
      MODE_RENAME,
      MODE_DELETE
   };

 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static bool       _icon_init;
   static IconBitmap _icons[ICON_NUM];
   static bool       _bitmap_init;
   static IconBitmap _bitmaps[BITMAP_NUM];

 public :    
   /******** STATIC MEMBER METHODS ********/

 protected:
   /******** MEMBER VARIABLES ********/
   file_cb_t         _cb;
   void*             _vp;
   int               _idx;

   DIR_ENTRYptr      _current_path;
   mode_t            _current_mode;
   str_ptr           _current_mode_saved_file;
   str_list          _current_recent_paths;
   sort_t            _current_sort;
   int               _current_selection;
   double            _current_selection_time;
   int               _current_scroll;
   bool              _current_scrollbar_wheel;
   int               _current_scrollbar_wheel_position;
   int               _current_scrollbar_wheel_index;
   int               _current_scrollbar_state;
   bool              _current_scrollbar_state_inside;
   int               _current_scrollbar_state_pixel_position;
   int               _current_scrollbar_state_index_position;
   double            _current_scrollbar_state_above_ratio;
   double            _current_scrollbar_state_below_ratio;


   
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public:
   GLUIFileSelect(GLUT_WINSYS *w);
   virtual ~GLUIFileSelect();

   /******** MEMBER METHODS ********/
 protected:
   void              init();

   void              update();

   void              update_paths();
   void              update_files();
   void              update_actions();

   void              update_icons();
   void              update_pathlist();

   void              update_headings();
   void              update_listing();
   void              update_scroll();

   str_ptr           shorten_string(int, Cstr_ptr &);

   DIR_ENTRYptr      generate_dir_tree(Cstr_ptr &);
   DIR_ENTRYptr      generate_dir_entry(Cstr_ptr &full_path,Cstr_ptr &name);
   bool              generate_dir_contents(DIR_ENTRYptr &);

   void              sort_dir_contents(DIR_ENTRYptr &, sort_t);

   DIR_ENTRYptr      get_selected_entry();
   void              set_selected_entry(DIR_ENTRYptr);

   bool              do_directory_change(Cstr_ptr &);
   void              do_entry_select(int);
   void              do_sort_toggle(int);
   void              do_scrollbar(int,int,int,int,int,int);
   void              do_path_listbox();
   void              do_edittext_event();
   void              do_scroll_delta(int);
   void              do_scroll_set(int);
   void              do_up_directory();
   void              do_refresh();
   void              do_add_mode();
   void              do_rename_mode();
   void              do_delete_mode();
   void              do_add_action();
   void              do_rename_action();
   void              do_delete_action();
   void              do_cancel_action();

   void              compute_scroll_geometry(int, int &, int &, int &, int &, int &, int &);
/*
   str_ptr           getcwd_();
   bool              chdir_(Cstr_ptr &);
   bool              mkdir_(Cstr_ptr &);
   bool              rmdir_(Cstr_ptr &);
   bool              remove_(Cstr_ptr &);
   bool              rename_(Cstr_ptr &, Cstr_ptr &);
*/
   str_list          readdir_(Cstr_ptr &, Cstr_ptr &);
   bool              stat_(Cstr_ptr &, DIR_ENTRYptr &);


   /******** FileSelect METHODS ********/
 public:
   virtual bool      is_displaying();
   virtual bool      display(bool blocking, file_cb_t cb, void *vp, int idx);

 protected:   
   virtual bool      undisplay(int button, str_ptr path, str_ptr file);

   /******** PopUp METHODS ********/
 protected:      
   virtual void      build_glui();
   virtual void      unbuild_glui();

   virtual void      button_cb(int id);
   virtual void      checkbox_cb(int id);
   virtual void      bitmapbox_cb(int id);
   virtual void      listbox_cb(int id);
   virtual void      edittext_cb(int id);
   virtual void      activetext_cb(int id);

};


#endif

/* end of file glui_dialogs.H */
