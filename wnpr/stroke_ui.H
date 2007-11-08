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
#ifndef _STROKE_UI_H_IS_INCLUDED_
#define _STROKE_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingPenUI
////////////////////////////////////////////
//
// -Manages a GLUI window that handles the interface
// -Actual 'live' variables reside within HatchingPen
// -Selection updates these vars, and update() refreshes the widgets
// -Apply button tells HatchingPen to apply the current settings
//
////////////////////////////////////////////

#include "gtex/aux_ref_image.H"
#include "gtex/paper_effect.H"
#include "stroke/base_stroke.H"

#define	GESTURE_SMOOTHING_PASSES	3
#define	GESTURE_PIXEL_RESOLUTION	15.0

/**********************************************************************
 * STROKE_UI_JOB:
 **********************************************************************/
class StrokeUI;
MAKE_PTR_SUBC(STROKE_UI_JOB,AUX_JOB);
#define CSTROKE_UI_JOBptr const STROKE_UI_JOBptr

class  STROKE_UI_JOB : public AUX_JOB {

 protected:

   StrokeUI*               _ui;

 public:

   STROKE_UI_JOB() : _ui(0)               {}
   virtual ~STROKE_UI_JOB()               {}

   void              set_ui(StrokeUI* ui) { _ui = ui; }
        
   //******** AUX_JOB METHODS ********

   virtual bool      needs_update();
   virtual void      updated();      

};


/*****************************************************************
 * STROKE_GEL
 *****************************************************************/

#define CSTROKE_GELptr const STROKE_GELptr
MAKE_PTR_SUBC(STROKE_GEL,GEL);

class STROKE_GEL : public GEL {
 protected:

	BaseStroke           _stroke;

	mlib::NDCZpt_list				_pts;

 public:
   STROKE_GEL()            
   { 
      _stroke.set_vis(VIS_TYPE_SCREEN); 
      _stroke.set_gen_t(true);
      _pts.clear(); 
   }
   ~STROKE_GEL()           { }

	BaseStroke*       stroke()	{ return &_stroke;	}
	mlib::NDCZpt_list&		pts()	   { return _pts;			}

 protected:
   mlib::NDCZpt_list       make_smooth(mlib::CNDCZpt_list &l) {
      mlib::NDCZpt_list new_l;
      if (l.num()>0)   new_l.add(l[0]);
      for (int i=1;i<(l.num()-1);i++)  new_l.add(( l[i-1]*1.0 + l[i]*4.0 + l[i+1]*1.0 ) / (6.0));
      if (l.num()>1)   new_l.add(l[l.num()-1]);
      return new_l;
   }

   //******** GEL METHODS ********
 public:

   virtual int				draw(CVIEWptr &v) 
	{ 
		int i;
		mlib::NDCZpt_list smooth_pts;
		if (_pts.num()<2) smooth_pts = _pts;
		else
		{

			int n;
			_pts.update_length();
			n = (int)floor(_pts.length()*VIEW::peek()->ndc2pix_scale()/GESTURE_PIXEL_RESOLUTION);
			smooth_pts.add(_pts[0]);
			for (i=1;i<n;i++) smooth_pts.add(_pts.interpolate((float)i/(float)n));
			smooth_pts.add(_pts[_pts.num()-1]);
		}

		for (i=0;i<GESTURE_SMOOTHING_PASSES;i++) smooth_pts = make_smooth(smooth_pts);

      smooth_pts.update_length();
      
      _stroke.clear();
      _stroke.set_min_t(0.0);
      _stroke.set_max_t(smooth_pts.length()*VIEW::peek()->ndc2pix_scale()/
                                                (max(_stroke.get_angle(),1.0f)));


      for (i=0;i<smooth_pts.num();i++) _stroke.add(smooth_pts[i]);

      _stroke.draw_start(); 
		i = _stroke.draw(v); 
		_stroke.draw_end(); 
		return i;
	}
	virtual DATA_ITEM*	dup()	const			{ return 0; }

};

class GLUI;
class GLUI_Listbox;
class GLUI_EditText;
class GLUI_Button;
class GLUI_Slider;
class GLUI_StaticText;
class GLUI_Panel;
class GLUI_BitmapBox;
class GLUI_Rollout;


/*****************************************************************
 * StrokeUIClient
 *****************************************************************/
class StrokeUIClient {
        
 public:
   virtual ~StrokeUIClient() {}
   virtual void         build(GLUI*, GLUI_Panel*, int)   {}
   virtual void         destroy(GLUI*, GLUI_Panel*)      {}
   virtual void         changed()                        {}
   virtual const char * plugin_name()                    { return "";}  

};

/*****************************************************************
 * StrokeUI
 *****************************************************************/
class StrokeUI : public PaperEffectObs {

   friend class STROKE_UI_JOB;

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_SAVE,
      BUT_DEBUG,
      BUT_NEXT_PEN,
      BUT_PREV_PEN,
      BUT_NUM
   };

   enum listbox_id_t {
      LIST_PRESET = 0,
      LIST_TEXTURE,
      LIST_PAPER,
      LIST_NUM
   };

   enum slider_id_t {
      SLIDE_H = 0,
      SLIDE_S,
      SLIDE_V,
      SLIDE_A,
      SLIDE_WIDTH,
      SLIDE_ALPHA_FADE,
      SLIDE_HALO,
      SLIDE_TAPER,
      SLIDE_FLARE,
      SLIDE_AFLARE,
      SLIDE_BRIGHTNESS,
      SLIDE_CONTRAST,
      SLIDE_ANGLE,
      SLIDE_PHASE,
      SLIDE_FOO,
      SLIDE_FOO2,
      SLIDE_NUM
   };

   enum edittext_id_t {
      EDITTEXT_SAVE = 0,
      EDITTEXT_NUM
   };

   enum bitmapbox_id_t {
      BITMAPBOX_PREVIEW = 0,
      BITMAPBOX_NUM
   };

   enum panel_id_t {
      PANEL_STROKE=0,
      PANEL_PRESET,
      PANEL_TEXTURE,
      PANEL_PAPER,
      PANEL_PEN,
      PANEL_NUM
   };

   enum rollout_id_t {
      ROLLOUT_COLORTEXTURE = 0,
      ROLLOUT_SHAPE,
      ROLLOUT_PLUGIN,
      ROLLOUT_NUM
   };

 protected:
   // Since we must use static members for callbacks,
   // and we *might* (?) have more that one of these
   // instantiated (due to multiple views), we maintain 
   // a list of all these ui's hashed by view (for lookup)
   // AND in an indexed array (for callbacks)
   // and use the callback id (shifted by ID_SHIFT)
   // to identify the appropriate object...
         
   /******** STATIC MEMBERS VARS ********/

   static HASH             _hash; 
   static ARRAY<StrokeUI*> _ui;

 public:

   static bool             capture(CVIEWptr& v, StrokeUIClient *suic);
   static bool             release(CVIEWptr& v, StrokeUIClient *suic);
   static bool             show(CVIEWptr& v, StrokeUIClient *suic);
   static bool             hide(CVIEWptr& v, StrokeUIClient *suic);
   static bool             update(CVIEWptr& v, StrokeUIClient *suic);
   static bool             set_params(CVIEWptr& v, StrokeUIClient *suic, CBaseStroke* sp);
   static CBaseStroke*     get_params(CVIEWptr& v, StrokeUIClient *suic);

 protected:

   static StrokeUI*             fetch(CVIEWptr& v);

 protected:
   /******** MEMBERS VARS ********/

   int                           _id;
   bool                          _init;
   VIEWptr                       _view;
   StrokeUIClient*               _client;

   //The parameters of the stroke stored
   //in a BaseStroke
   BaseStroke                    _params;

   int                           _curr_stroke;
   STROKE_UI_JOBptr              _job;
   LIST<STROKE_GELptr>           _strokes;

   GLUI*                         _glui;   
   float                         _column_width;
   str_list                      _texture_filenames;
   str_list                      _paper_filenames;
   str_list                      _preset_filenames;
   ARRAY<GLUI_Listbox*>          _listbox;
   ARRAY<GLUI_Button*>           _button;
   ARRAY<GLUI_Slider*>           _slider;
   ARRAY<GLUI_EditText*>         _edittext;
   ARRAY<GLUI_Panel*>            _panel;
   ARRAY<GLUI_Rollout*>          _rollout;
   ARRAY<GLUI_BitmapBox*>        _bitmapbox;


   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
 protected:     
   StrokeUI(VIEWptr v);
   virtual ~StrokeUI();

   /******** MEMBERS METHODS ********/

   bool                    internal_verify(StrokeUIClient *suic);
   bool                    internal_capture(StrokeUIClient* suic); 
   bool                    internal_release();
   bool                    internal_show();
   bool                    internal_hide();
   bool                    internal_update();
   
   CBaseStroke*            internal_get_params() { return &_params; }
   bool                    internal_set_params(CBaseStroke* sp) 
										{ if (sp) { _params.copy(*sp);   return true; } return false; }

   void                    init();
   void                    set_preview_size();

   void                    build();
   void                    destroy();

   void                    stroke_img_updated();
   void                    update_stroke_img();
   void                    update_non_lives();
   void                    params_changed();
   void                    preset_selected();
   void                    preset_save_button();
   void                    preset_save_text();

   bool                    save_preset(const char *f);
   bool                    load_preset(const char *f);

   /******** Convenience Methods ********/

   mlib::NDCZpt                  xy_to_ndcz(int x, int y);
   void                    fill_texture_listbox(GLUI_Listbox *lb, str_list &save_files, Cstr_ptr &full_path);
   void                    fill_paper_listbox(GLUI_Listbox *lb, str_list &save_files, Cstr_ptr &full_path);
   void                    fill_preset_listbox(GLUI_Listbox *lb, str_list &save_files, Cstr_ptr &full_path);

   /******** EVENT HANDLERS ********/

   void                    handle_preview_bitmapbox();

   /******** STATIC CALLBACK METHODS ********/
   static void button_cb(int id);
   static void listbox_cb(int id);
   static void slider_cb(int id);
   static void edittext_cb(int id);
   static void bitmapbox_cb(int id);

   // ******** PaperEffectObs Method ********
   virtual void            usage_changed() { update_stroke_img(); }
   virtual void            paper_changed() { update_stroke_img(); }


};


#endif // _STROKE_UI_H_IS_INCLUDED_

/* end of file stroke_ui.H */
