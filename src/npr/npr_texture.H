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
#ifndef HATCHING_STROKE_TEXTURE_HEADER
#define HATCHING_STROKE_TEXTURE_HEADER

////////////////////////////////////////////
// NPRTexture
////////////////////////////////////////////
//
// -This is the texture used to hold and
//  render hatching groups
// -Features stroke texture is also held
//  and rendered here
// -The hatchgroups live in a 'hatching collection'
//
////////////////////////////////////////////

#include "wnpr/sil_ui.H"
#include "gtex/ref_image.H"
#include "mesh/patch.H"
#include "gtex/sils_texture.H"     
#include "gtex/solid_color.H" 
#include "npr/feature_stroke_texture.H"
#include "npr/hatching_collection.H"
#include "npr/npr_bkg_texture.H"
#include "npr/npr_solid_texture.H"
#include "gtex/xtoon_texture.H"
#include "gtex/glsl_xtoon.H"
#include "npr/npr_control_frame.H"
#include "std/config.H"

class DecalLineStroke;
class BaseStroke;

/*****************************************************************
 * NPRTexture
 *****************************************************************/

class NPRTexture : public OGLTexture {

 private:        
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist *_nt_tags;
   static bool    _show_strokes;
   static bool    _show_coats;  
 public:        
   /******** STATIC MEMBER METHODS ********/
   static void    set_show_strokes(bool b) { _show_strokes = b; }
   static bool    toggle_strokes() { return _show_strokes = ! _show_strokes; }
   static bool    toggle_coats() { return _show_coats = ! _show_coats; } 

 protected:

   /******** MEMBER VARIABLES ********/         

   FeatureStrokeTexture*         _stroke_tex;
   HatchingCollection*           _hatching_collection;
       
   int                           _basecoat_id;
   NPRBkgTexture*                _bkg_tex;
   NPRControlFrameTexture*       _ctrl_frm;
   GTexture_list                 _basecoats;

   int                           _transparent;
   int                           _annotate;

   float                         _polygon_offset_factor;
   float                         _polygon_offset_units;

   bool                          _see_thru;
   ARRAY<bool>                   _see_thru_flags;

   bool                          _selected;

   str_ptr                       _data_file;
   bool                          _in_data_file;

   ARRAY<SilDebugObs *>          _sdolist;
 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/
   NPRTexture(Patch* patch = 0);
   virtual ~NPRTexture() { 
      delete   _hatching_collection;
      gtextures().delete_all();
   } 
 
   /******** MEMBER METHODS ********/

   void                 set_selected(bool s)                { _selected = s;        }
   bool                 get_selected() const                { return _selected;     }
   void                 set_data_file(str_ptr f)            { _data_file = f; }
   Cstr_ptr&            get_data_file() const               { return _data_file; }
   virtual int          get_transparent() const             { return _transparent;  }
   virtual void         set_transparent(int t)              { _transparent = t;     }
   virtual int          get_annotate() const                { return _annotate;     }
   virtual void         set_annotate(int a)                 { _annotate = a; /*_patch->mesh()->changed();*/ }
   void                 set_polygon_offset_factor(float f)  { _polygon_offset_factor = f;    }
   float                get_polygon_offset_factor()         { return _polygon_offset_factor; }
   void                 set_polygon_offset_units(float u)   { _polygon_offset_units = u;    }
   float                get_polygon_offset_units()          { return _polygon_offset_units; }
   void                 set_see_thru(bool s)                { _see_thru = s;        }
   bool                 get_see_thru() const                { return _see_thru;     }
   void                 set_see_thru_flag(int i, bool s)    { assert(i<ZXFLAG_NUM); _see_thru_flags[i] = s;    }
   bool                 get_see_thru_flag(int i) const      { assert(i<ZXFLAG_NUM); return _see_thru_flags[i]; }



   // Result depends upon basecoats
   COLOR                get_color() const;
   double               get_alpha() const;

   /***Interfaces to backcoats***/
   int                  get_basecoat_num() const            { return _basecoats.num(); }
   GTexture*            get_basecoat(int i) const           { return (_basecoats.valid_index(i))?(_basecoats[i]):(NULL); }
   void                 insert_basecoat(int i, GTexture* t);
   void                 remove_basecoat(int i);


   void                 add_obs(SilDebugObs *o)             { bool u = _sdolist.add_uniquely(o); assert(u); }
   void                 rem_obs(SilDebugObs *o)             { _sdolist.rem(o);}
   void                 notify_sdo_draw(SilAndCreaseTexture *t);

   /******** GTEXTURE METHODS ********/
   virtual GTexture_list gtextures() const {
      GTexture_list ret = _basecoats;
      ret += _stroke_tex;
      ret += _bkg_tex;
      ret += _ctrl_frm;
      return ret;
   }

   virtual void set_patch(Patch* p) {
      GTexture::set_patch(p);
      _hatching_collection->set_patch(p);
   }

   virtual int          draw(CVIEWptr& v);
   virtual int          draw_final(CVIEWptr& v);
   virtual int          draw_id_ref();
   virtual int          draw_id_ref_pre1();
   virtual int          draw_id_ref_pre2();
   virtual int          draw_id_ref_pre3();
   virtual int          draw_id_ref_pre4();
   virtual int          draw_vis_ref();

   // We pipe these into the better IO system...
   virtual int          write_stream(ostream &os) const;
   virtual int          read_stream (istream &, str_list &leftover);

   virtual int          read_gtexture(istream &is, str_list &);


   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("NPRTexture", OGLTexture, CDATA_ITEM *);
   virtual DATA_ITEM*   dup()  const      { return new NPRTexture; }
   virtual CTAGlist&    tags() const;
   STDdstream&          old_format(STDdstream &ds) const;

   /******** I/O functions ********/

   void                 get_transparent (TAGformat &d);
   void                 put_transparent (TAGformat &d) const;
   void                 get_annotate (TAGformat &d);
   void                 put_annotate (TAGformat &d) const;
   void                 get_polygon_offset_factor (TAGformat &d);
   void                 put_polygon_offset_factor (TAGformat &d) const;
   void                 get_polygon_offset_units (TAGformat &d);
   void                 put_polygon_offset_units (TAGformat &d) const;
   
   void                 get_see_thru (TAGformat &d);
   void                 put_see_thru (TAGformat &d) const;
   void                 get_see_thru_flags (TAGformat &d);
   void                 put_see_thru_flags (TAGformat &d) const;

   void                 get_npr_data_file (TAGformat &d);
   void                 put_npr_data_file (TAGformat &d) const;

   void                 get_collection (TAGformat &d);
   void                 put_collection (TAGformat &d) const;
   void                 get_line_stroke_texture (TAGformat &d);
   void                 put_line_stroke_texture (TAGformat &d) const;
   void                 get_feature_stroke_texture (TAGformat &d);
   void                 put_feature_stroke_texture (TAGformat &d) const;

   void                 get_basecoat (TAGformat &d);
   void                 put_basecoats (TAGformat &d) const;

   void                 get_basecoat_id (TAGformat &d);
   void                 put_basecoat_id (TAGformat &d) const;
   void                 get_bkg_texture (TAGformat &d);
   void                 put_bkg_texture (TAGformat &d) const;
   void                 get_solid_texture (TAGformat &d);
   void                 put_solid_texture (TAGformat &d) const;
   void                 get_toon_texture (TAGformat &d);
   void                 put_toon_texture (TAGformat &d) const;
   void                 get_xtoon_texture (TAGformat &d);
   void                 put_xtoon_texture (TAGformat &d) const;


   // XXX - Note, these are deprecated. 
   // The new .jot files hold this info...
   /******** VIEW I/O functions ********/
   void                 get_view_color (TAGformat &d);
   void                 put_view_color (TAGformat &d) const;
   void                 get_view_alpha (TAGformat &d);
   void                 put_view_alpha (TAGformat &d) const;
   void                 get_view_paper_use (TAGformat &d);
   void                 put_view_paper_use (TAGformat &d) const;
   void                 get_view_paper_name (TAGformat &d);
   void                 put_view_paper_name (TAGformat &d) const;
   void                 get_view_paper_active (TAGformat &d);
   void                 put_view_paper_active (TAGformat &d) const;
   void                 get_view_texture (TAGformat &d);
   void                 put_view_texture (TAGformat &d) const;
   void                 get_view_light_coords (TAGformat &d);
   void                 put_view_light_coords (TAGformat &d) const;
   void                 get_view_light_positional (TAGformat &d);
   void                 put_view_light_positional (TAGformat &d) const;
   void                 get_view_light_cam_space (TAGformat &d);
   void                 put_view_light_cam_space (TAGformat &d) const;
   void                 get_view_light_color_diff (TAGformat &d);
   void                 put_view_light_color_diff (TAGformat &d) const;
   void                 get_view_light_color_amb (TAGformat &d);
   void                 put_view_light_color_amb (TAGformat &d) const;
   void                 get_view_light_color_global (TAGformat &d);
   void                 put_view_light_color_global (TAGformat &d) const;
   void                 get_view_light_enable (TAGformat &d);
   void                 put_view_light_enable (TAGformat &d) const;

   /******** Ref_Img_Client METHODS ********/
   virtual void request_ref_imgs();
      
   /******** MORE MEMBER METHODS ********/

   //***Interfaces to strokes***

   //Interface to hatching
   HatchingCollection * hatching_collection() { return _hatching_collection;}

   //Interface to feature strokes
   FeatureStrokeTexture* stroke_tex() { return _stroke_tex; }

};

#endif
