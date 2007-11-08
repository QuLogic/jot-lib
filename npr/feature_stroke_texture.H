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
#ifndef FEATURE_STROKE_TEXTURE_HEADER
#define FEATURE_STROKE_TEXTURE_HEADER

#include "mesh/patch.H"
#include "npr/sil_and_crease_texture.H"
#include "stroke/decal_stroke_pool.H"

class DecalLineStroke;

////////////////////////////////////////////
// FeatureStrokeTexture
////////////////////////////////////////////

class FeatureStrokeTexture : public OGLTexture,
                             protected CAMobs,
                             protected BMESHobs,
                             protected XFORMobs
{
 private:        
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist       *_fst_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   DecalStrokePool      _decal_strokes;    
   SilAndCreaseTexture* _sil_and_crease_tex;
   bool                 _inited;
   bool                 _decal_strokes_need_update;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/
   FeatureStrokeTexture(Patch* patch = 0);
   virtual ~FeatureStrokeTexture() {  unobserve(); }

   /******** MEMBER METHODS ********/
   void init();

   virtual void set_patch(Patch* p) {
      GTexture::set_patch(p); _sil_and_crease_tex->set_patch(p);
      if(_decal_strokes.get_prototype()) _decal_strokes.get_prototype()->set_patch(_patch);
   }

   void mark_all_dirty() { 
      _decal_strokes_need_update = true;
      _sil_and_crease_tex->mark_all_dirty(); 
   }

   //******** DRAWING METHODS ********

   virtual int          draw(CVIEWptr& v);
   virtual int          draw_vis_ref();
   virtual int          draw_id_ref();
   virtual int          draw_id_ref_pre1();
   virtual int          draw_id_ref_pre2();
   virtual int          draw_id_ref_pre3();
   virtual int          draw_id_ref_pre4();

   //******** ACCESSOR METHODS ********

   DecalLineStroke*     get_decal_stroke();
   DecalLineStroke*     get_decal_stroke_proto();
   BStrokePool*         get_decal_stroke_pool();

   SilAndCreaseTexture* sil_and_crease_tex()    { return _sil_and_crease_tex; }

   virtual void         set_new_branch(int b);
   virtual void         set_seethru(int s);

   // XXX - Deprecated I/O
   virtual int          write_stream(ostream& os) const;
   virtual int          read_stream (istream&, str_list &leftover);
   virtual int          read_gtexture(istream &is, str_list &);


  //******** OBSERVER METHODS ********
   void observe  ();
   void unobserve();

   // CAMobs:
   virtual void notify(CCAMdataptr&) { _decal_strokes_need_update = true;  }

   // XFORMobs
   virtual void notify_xform(CGEOMptr&, STATE) {
      _decal_strokes_need_update = true;
   }

   // BMESHobs:
   virtual void notify_xform(BMESH*, mlib::CWtransf&, CMOD&);

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("FeatureStrokeTexture", OGLTexture, CDATA_ITEM *);
   virtual DATA_ITEM    *dup() const { return new FeatureStrokeTexture; }
   virtual CTAGlist&    tags() const;
   virtual STDdstream   &decode(STDdstream &d);

   /******** I/O functions ********/
   void                 get_legacy_stream (TAGformat &d);
   void                 put_legacy_stream (TAGformat &d) const;

   void                 get_decal_pool (TAGformat &d);
   void                 put_decal_pool (TAGformat &d) const;

   void                 get_sil_and_crease_texture (TAGformat &d);
   void                 put_sil_and_crease_texture (TAGformat &d) const;

   //******** RefImageClient METHODS ********

   virtual void request_ref_imgs() { _sil_and_crease_tex->request_ref_imgs(); }
};

#endif
