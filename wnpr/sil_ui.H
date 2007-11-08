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
#ifndef _SIL_UI_H_IS_INCLUDED_
#define _SIL_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// SilUI
////////////////////////////////////////////

#include "disp/view.H"
#include "mesh/patch.H"
#include "npr/sil_and_crease_texture.H"

class GLUI;
class GLUI_Listbox;
class GLUI_EditText;
class GLUI_Button;
class GLUI_Slider;
class GLUI_StaticText;
class GLUI_Panel;
class GLUI_Rollout;
class GLUI_Rotation;
class GLUI_RadioButton;
class GLUI_RadioGroup;
class GLUI_Checkbox;
class GLUI_Graph;

class NPRTexture;

/*****************************************************************
 * SilDebugObs
 *****************************************************************/

class SilDebugObs {
   public:
      virtual ~SilDebugObs() {}
      virtual void notify_draw(SilAndCreaseTexture *t) = 0;
};

/*****************************************************************
 * SilUI
 *****************************************************************/

class SilUI : public FRAMEobs, SilDebugObs, public DATA_ITEM {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_REFRESH=0,
      BUT_BUFFER_GRAB,
      BUT_BUFFER_SAVE,
      BUT_PATCH_NEXT,
      BUT_PATH_NEXT,
      BUT_PATH_PREV,
      BUT_SEG_NEXT,
      BUT_SEG_PREV,
      BUT_VOTE_NEXT,
      BUT_VOTE_PREV,
      BUT_NUM
   };

   enum slider_id_t {
      SLIDE_RATE = 0,
      SLIDE_WEIGHT_FIT,
      SLIDE_WEIGHT_SCALE,
      SLIDE_WEIGHT_DISTORT,
      SLIDE_WEIGHT_HEAL,
      SLIDE_FIT_PIX,
      SLIDE_NUM
   };

   enum panel_id_t {
      PANEL_TOP=0,
      PANEL_MESH,
      PANEL_PATCH,
      PANEL_REFRESH,
      PANEL_PATH,
      PANEL_PATH_INFO,
      PANEL_SEG,
      PANEL_SEG_INFO,
      PANEL_VOTE,
      PANEL_VOTE_INFO,
      PANEL_VOTE_DATA,
      PANEL_NUM
   };

   enum rollout_id_t {
      ROLLOUT_PATH = 0,
      ROLLOUT_SEG,
      ROLLOUT_OPTIONS,
      ROLLOUT_BUFFER,
      ROLLOUT_VOTE,
      ROLLOUT_NUM
   };

   enum list_id_t {
      LIST_MESH = 0,
      LIST_BUFFER,
      LIST_NUM
   };

   enum graph_id_t {
      GRAPH_PATH = 0,
      GRAPH_SEG,
      GRAPH_NUM
   };

   enum text_id_t {
      TEXT_PATH = 0,
      TEXT_SEG,
      TEXT_PATCH,
      TEXT_VOTE,
      TEXT_VOTE_1,
      TEXT_VOTE_2,
      TEXT_VOTE_3,
      TEXT_VOTE_4,
      TEXT_NUM
   };

   enum edittext_id_t {
      EDITTEXT_BUFFER_NAME = 0,
      EDITTEXT_BUFFER_MISC1,
      EDITTEXT_BUFFER_MISC2,
      EDITTEXT_NUM
   };

   enum checkbox_id_t {
      CHECK_TRACK_STROKES = 0,
      CHECK_ZOOM_STROKES,
      CHECK_ALWAYS_UPDATE,
      CHECK_SIGMA_ONE,
      CHECK_NUM
   };

   enum radiogroup_id_t {
      RADGROUP_FIT = 0,
      RADGROUP_COVER,
      RADGROUP_NUM
   };

   enum radiobutton_id_t {
      RADBUT_FIT_SIGMA= 0,      
      RADBUT_FIT_PHASE,
      RADBUT_FIT_INTERPOLATE,
      RADBUT_FIT_OPTIMIZE,
      RADBUT_COVER_MAJORITY,
      RADBUT_COVER_ONE_TO_ONE,
      RADBUT_COVER_HYBRID,
      RADBUT_NUM
   };

   /******** STATIC MEMBERS VARS ********/
 private:
   static TAGlist*         _sui_tags;
 protected:
   static HASH             _hash; 
   static ARRAY<SilUI*>    _ui;

 public:

   static bool             is_vis(CVIEWptr& v);
   static bool             show(CVIEWptr& v);
   static bool             hide(CVIEWptr& v);
   static bool             update(CVIEWptr& v);

   static bool             zoom_strokes(CVIEWptr& v)     { return fetch(v)->zoom_strokes(); }

   static bool             always_update(CVIEWptr& v)    { return fetch(v)->always_update(); }
   static bool             sigma_one(CVIEWptr& v)        { return fetch(v)->sigma_one(); }
   static int              cover_type(CVIEWptr& v)       { return fetch(v)->cover_type(); }
   static int              fit_type(CVIEWptr& v)         { return fetch(v)->fit_type(); }
   static float            fit_pix(CVIEWptr& v)          { return fetch(v)->fit_pix(); }
   static float            weight_fit(CVIEWptr& v)       { return fetch(v)->weight_fit(); }
   static float            weight_scale(CVIEWptr& v)     { return fetch(v)->weight_scale(); }
   static float            weight_distort(CVIEWptr& v)   { return fetch(v)->weight_distort(); }
   static float            weight_heal(CVIEWptr& v)      { return fetch(v)->weight_heal(); }

 protected:

   static SilUI*           fetch(CVIEWptr& v);

 protected:
   /******** MEMBERS VARS ********/

   GELptr                        _bufferGEL;

   GEL*                          _selectedGEL;
   Patch*                        _selectedPatch;
   NPRTexture*                   _observedTexture;
   int                           _votePathIndex;
   unsigned int                  _votePathIndexStamp;
   int                           _strokePathId;
   int                           _strokePathIndex;   
   unsigned int                  _strokePathIndexStamp;
   int                           _voteIndex;   
   unsigned int                  _lastDrawStamp;

   int                           _id;
   bool                          _init;
   VIEWptr                       _view;

   str_list                      _mesh_names;
   str_list                      _buffer_filenames;

   int                           _zoom_strokes;

   int                           _sigma_one;
   int                           _always_update;
   int                           _fit_type;
   int                           _cover_type;
   float                         _fit_pix;
   float                         _weight_fit;
   float                         _weight_scale;
   float                         _weight_distort;
   float                         _weight_heal;

   GLUI*                         _glui;   
   ARRAY<GLUI_Button*>           _button;
   ARRAY<GLUI_Slider*>           _slider;
   ARRAY<GLUI_Panel*>            _panel;
   ARRAY<GLUI_Rollout*>          _rollout;
   ARRAY<GLUI_Listbox*>          _listbox;
   ARRAY<GLUI_Graph*>            _graph;
   ARRAY<GLUI_StaticText*>       _text;
   ARRAY<GLUI_EditText*>         _edittext;
   ARRAY<GLUI_Checkbox*>         _checkbox;
   ARRAY<GLUI_RadioGroup*>       _radgroup;
   ARRAY<GLUI_RadioButton*>      _radbutton;

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
 protected:     
   SilUI(VIEWptr v);
   virtual ~SilUI();

   /******** MEMBERS METHODS ********/
 protected:     
   bool     internal_is_vis();
   bool     internal_show();
   bool     internal_hide();
   bool     internal_update();
      
   void     init();

   void     build();
   void     destroy();

   void     adjust_control_sizes();

   void     update_non_lives();

   bool     update_mesh_list();
   void     update_patch();
   void     update_path();
   void     update_seg();
   void     update_vote();

   void     clear_path();
   void     clear_seg();
   void     clear_vote();


   void     fps_display();

   void     select_gel(GEL *g);
   void     deselect_gel();

   void     select_patch(Patch *p);
   void     deselect_patch();

   void     start_observing(NPRTexture *t);
   void     stop_observing();

   void     select_name();
   void     select_next_patch();

   void     init_votepath_index();
   void     clear_votepath_index();

   void     init_strokepath_index();
   void     clear_strokepath_index();

   void     init_vote_index();
   void     clear_vote_index();

   void     update_path_indices();

   void     select_votepath_next();
   void     select_votepath_prev();

   void     select_strokepath_next();
   void     select_strokepath_prev();

   void     select_vote_next();
   void     select_vote_prev();

   bool     zoom_strokes()	     { return (_zoom_strokes == 1); }

   bool     always_update()      { return (_always_update == 1); }
   bool     sigma_one()          { return (_sigma_one == 1); }
   float    fit_pix()            { return _fit_pix; }
   float    weight_fit()         { return _weight_fit; }
   float    weight_scale()       { return _weight_scale; }
   float    weight_distort()     { return _weight_distort; }
   float    weight_heal()        { return _weight_heal; }

   int      fit_type()           
   { 
      switch(_fit_type)
      {
         case SilUI::RADBUT_FIT_SIGMA:       return SilAndCreaseTexture::SIL_FIT_SIGMA;      break;
         case SilUI::RADBUT_FIT_PHASE:       return SilAndCreaseTexture::SIL_FIT_PHASE;      break;
         case SilUI::RADBUT_FIT_INTERPOLATE: return SilAndCreaseTexture::SIL_FIT_INTERPOLATE;break;
         case SilUI::RADBUT_FIT_OPTIMIZE:    return SilAndCreaseTexture::SIL_FIT_OPTIMIZE;   break;
         default: assert(0); return 0; break;
      }
   }

   
   int      cover_type()         
   { 
      switch(_cover_type + RADBUT_COVER_MAJORITY)
      {
         case SilUI::RADBUT_COVER_MAJORITY:  return SilAndCreaseTexture::SIL_COVER_MAJORITY;    break;
         case SilUI::RADBUT_COVER_ONE_TO_ONE:return SilAndCreaseTexture::SIL_COVER_ONE_TO_ONE;  break;
         case SilUI::RADBUT_COVER_HYBRID:    return SilAndCreaseTexture::SIL_COVER_TRIMMED;     break;
         default: assert(0); return 0; break;
      }      
   }

   void     buffer_grab();
   void     buffer_selected();
   void     buffer_save_button();
   void     buffer_name_text();

   bool     buffer_save(const char *f);
   bool     buffer_load(const char *f);

   /******** Convenience Methods ********/

   bool     valid_gel(CGELptr g);
   void     fill_buffer_listbox(GLUI_Listbox *, str_list &, Cstr_ptr &);

   /******** STATIC CALLBACK METHODS ********/
   static void button_cb(int id);
   static void slider_cb(int id);
   static void listbox_cb(int id);
   static void edittext_cb(int id);
   static void graph_cb(int id);
   static void checkbox_cb(int id);
   static void radiogroup_cb(int id);
   
   /******** FRAMEobs METHODS ********/
   virtual int  tick();

   /******** SilDebugObs METHODS ********/
   virtual void notify_draw(SilAndCreaseTexture *t);
   
   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS_BASE("SilUI", CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return new SilUI(VIEW::peek()); }
   virtual CTAGlist&    tags() const;

 protected:
   /******** I/O Methods ********/
   virtual void   get_paths (TAGformat &d);
   virtual void   put_paths (TAGformat &d) const;
   virtual void   get_selected_gel (TAGformat &d);
   virtual void   put_selected_gel (TAGformat &d) const;
   virtual void   get_selected_patch (TAGformat &d);        
   virtual void   put_selected_patch (TAGformat &d) const; 
   virtual void   get_vote_path_index(TAGformat &d);        //(_votePathIndexStamp, and in zx)
   virtual void   put_vote_path_index(TAGformat &d) const;
   virtual void   get_stroke_path_index(TAGformat &d);      //(_strokePathId, _strokePathIndexStamp, and in zx)
   virtual void   put_stroke_path_index(TAGformat &d) const;
   virtual void   get_vote_index(TAGformat &d);
   virtual void   put_vote_index(TAGformat &d) const;

   /******** I/O Access Methods ********/
   int&           always_update_()   { return _always_update; }
   int&           sigma_one_()       { return _sigma_one; }
   int&           fit_type_()        { return _fit_type; }
   int&           cover_type_()      { return _cover_type; }
   float&         fit_pix_()         { return _fit_pix; }
   float&         weight_fit_()      { return _weight_fit; }
   float&         weight_scale_()    { return _weight_scale; }
   float&         weight_distort_()  { return _weight_distort; }
   float&         weight_heal_()     { return _weight_heal; }


};

#endif // _SIL_UI_H_IS_INCLUDED_

/* end of file sil_ui.H */
