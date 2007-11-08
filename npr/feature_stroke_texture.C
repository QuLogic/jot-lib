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
#include "feature_stroke_texture.H"
#include "gtex/color_id_texture.H"
#include "gtex/ref_image.H"
#include "stroke/decal_line_stroke.H"
#include "mesh/ioblock.H"
#include "geom/gl_view.H"

using namespace mlib;

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       FeatureStrokeTexture::_fst_tags = 0;


/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
FeatureStrokeTexture::tags() const
{
   if (!_fst_tags) {
      _fst_tags = new TAGlist;
      *_fst_tags += OGLTexture::tags();

      // XXX - Wrap the nasty old read/write_stream
      // stuff into a single tag -- so we can achieve
      // some other things to do with storing annotations
      // into separate files.  However, we'll keep
      // read_stream for older files. See read_steam
      // in NPRTexture to see how we detect this.
      //
      // In future, there should probably be two new tags
      // which format and decode the decal pool and
      // the sil_and_crease texture. Those two objects
      // should also change over to using tags,
      // but maintain their read/write streams
      // for backward compatibility.
      //
      // Once we convert all our stuff over to tags,
      // we can read old files, write the new format,
      // and trash all the old read/write streaming...
      *_fst_tags += new TAG_meth<FeatureStrokeTexture>(
                       "legacy_stream",
                       &FeatureStrokeTexture::put_legacy_stream,
                       &FeatureStrokeTexture::get_legacy_stream,
                       1);

      //New tags!
      *_fst_tags += new TAG_meth<FeatureStrokeTexture>(
                       "decal_pool",
                       &FeatureStrokeTexture::put_decal_pool,
                       &FeatureStrokeTexture::get_decal_pool,
                       1);
      *_fst_tags += new TAG_meth<FeatureStrokeTexture>(
                       "sil_and_crease_texture",
                       &FeatureStrokeTexture::put_sil_and_crease_texture,
                       &FeatureStrokeTexture::get_sil_and_crease_texture,
                       1);



   }
   return *_fst_tags;
}

/////////////////////////////////////
// decode()
/////////////////////////////////////
STDdstream&
FeatureStrokeTexture::decode(STDdstream &d)
{
   STDdstream &ret = OGLTexture::decode(d);
   changed();
   return ret;
}

/////////////////////////////////////
// put_decal_pool()
/////////////////////////////////////
void
FeatureStrokeTexture::put_decal_pool(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "FeatureStrokeTexture::put_decal_pool()");

   d.id();
   _decal_strokes.format(*d);
   d.end_id();

}
/////////////////////////////////////
// get_decal_pool()
/////////////////////////////////////
void
FeatureStrokeTexture::get_decal_pool(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "FeatureStrokeTexture::get_decal_pool()");

   str_ptr str;
   *d >> str;

   if (_decal_strokes.class_name() != str) {
      err_mesg(ERR_LEV_WARN, "FeatureStrokeTexture::get_decal_pool() - WARNING! Class name '%s' not a '%s'!!",
               **str, **_decal_strokes.class_name() );
   }

   // XXX - Hackery!
   assert(DecalLineStroke::mesh() == 0);
   assert(_patch);
   DecalLineStroke::set_mesh(_patch->mesh());

   // Load the decal pool
   _decal_strokes.decode(*d);

   // XXX - More hackery!
   BaseStroke* s = _decal_strokes.get_prototype();
   assert(s && s->is_of_type(DecalLineStroke::static_name()));

   ((DecalLineStroke*)s)->set_patch(_patch);

   for (int j=0; j<_decal_strokes.num_strokes(); j++) {
      s = _decal_strokes.stroke_at(j);
      assert(s && s->is_of_type(DecalLineStroke::static_name()));
      ((DecalLineStroke*)s)->set_patch(_patch);
   }

   DecalLineStroke::set_mesh(0);

}

/////////////////////////////////////
// put_sil_and_crease_texture()
/////////////////////////////////////
void
FeatureStrokeTexture::put_sil_and_crease_texture(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "FeatureStrokeTexture::put_sil_and_crease_texture()");

   d.id();
   _sil_and_crease_tex->format(*d);
   d.end_id();

}
/////////////////////////////////////
// get_sil_and_crease_texture()
/////////////////////////////////////
void
FeatureStrokeTexture::get_sil_and_crease_texture(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "FeatureStrokeTexture::get_sil_and_crease_texture()");

   str_ptr str;
   *d >> str;

   if (_sil_and_crease_tex->class_name() != str) {
      err_mesg(ERR_LEV_ERROR, "FeatureStrokeTexture::get_sil_and_crease_texture() - Class name '%s' not a '%s'!!!!",
               **str, **_sil_and_crease_tex->class_name());
      return;
   }

   _sil_and_crease_tex->decode(*d);

   //cleanup

}
/////////////////////////////////////
// put_legacy_stream()
/////////////////////////////////////
void
FeatureStrokeTexture::put_legacy_stream(TAGformat &d) const
{
   /* XXX - Deprecated
      d.id();
      write_stream(*(*d).stream());
      d.end_id();
   */
}

/////////////////////////////////////
// get_legacy_stream()
/////////////////////////////////////
void
FeatureStrokeTexture::get_legacy_stream(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "FeatureStrokeTexture::get_legacy_stream() - **Old School**");

   str_list leftover;

   read_stream(*(*d).istr(),leftover);

   // XXX - Not sure what order to put these back
   // but this is temporary and seems to work...
   while (leftover.num()>0)
      ((*d).istr())->putback(***(leftover.pop()));

}



FeatureStrokeTexture::FeatureStrokeTexture(Patch *patch)
      : OGLTexture(patch),
      _sil_and_crease_tex(new SilAndCreaseTexture(patch)),
      _inited(false),
      _decal_strokes_need_update(true)
{

   assert(_sil_and_crease_tex);
   _decal_strokes.set_prototype(new DecalLineStroke(_patch));
}


void
FeatureStrokeTexture::init()
{
   if (_inited)
      return;

   observe();

   if(_sil_and_crease_tex)
      _sil_and_crease_tex->init();

   _inited = true;
}

void
FeatureStrokeTexture::observe()
{
   // CAMobs:
   VIEW::peek()->cam()->data()->add_cb(this);

   // BMESHobs:
   //XXX - (rdk) Only need to observe your own mesh's
   //transformations... I hope?!?!
   //subscribe_all_mesh_notifications();
   assert(_patch->mesh());
   subscribe_mesh_notifications(_patch->mesh());

   //XXX - (rdk) ditto here, but I'm leaving it for now...
   // XFORMobs:
   every_xform_obs();
}


void
FeatureStrokeTexture::unobserve()
{
   // CAMobs:
   VIEW::peek()->cam()->data()->rem_cb(this);

   // BMESHobs:
   //XXX - (rdk) Only need to observe your own mesh's
   //transformations...
   //unsubscribe_all_mesh_notifications();
   assert(_patch->mesh());
   this->unsubscribe_mesh_notifications(_patch->mesh());

   // XFORMobs:
   unobs_every_xform();
}


int
FeatureStrokeTexture::draw(CVIEWptr& v)
{
   //(simon)
   //err_mesg(ERR_LEV_ERROR, "FeatureStrokeTexture::draw - got here");

   if (!_inited)
      init();

   // If needed, force decal strokes to be rebuilt before
   // drawing, by marking them 'dirty'.
   // (This is necessary if camera or scene has changed.)
   if (_decal_strokes_need_update) {
      _decal_strokes.mark_dirty();
      _decal_strokes_need_update = false;
   }

   _decal_strokes.draw_flat(v);
   _sil_and_crease_tex->draw(v);

   return _patch->faces().num();
}


int
FeatureStrokeTexture::draw_id_ref()
{
   if (_sil_and_crease_tex)
      return _sil_and_crease_tex->draw_id_ref();

   return _patch->num_faces();
}

void
FeatureStrokeTexture::set_seethru(int s)
{
   if (_sil_and_crease_tex)
      _sil_and_crease_tex->set_seethru(s);
}

void
FeatureStrokeTexture::set_new_branch(int b)
{
   if (_sil_and_crease_tex)
      _sil_and_crease_tex->set_new_branch(b);
}

int
FeatureStrokeTexture::draw_id_ref_pre1()
{
   if (_sil_and_crease_tex)
      return _sil_and_crease_tex->draw_id_ref_pre1();

   return _patch->num_faces();
}

int
FeatureStrokeTexture::draw_id_ref_pre2()
{
   if (_sil_and_crease_tex)
      return _sil_and_crease_tex->draw_id_ref_pre2();

   return _patch->num_faces();
}

int
FeatureStrokeTexture::draw_id_ref_pre3()
{
   if (_sil_and_crease_tex)
      return _sil_and_crease_tex->draw_id_ref_pre3();

   return _patch->num_faces();
}

int
FeatureStrokeTexture::draw_id_ref_pre4()
{
   if (_sil_and_crease_tex)
      return _sil_and_crease_tex->draw_id_ref_pre4();

   return _patch->num_faces();
}


int
FeatureStrokeTexture::draw_vis_ref()
{
   // Draw ID-colored silhouette edges,
   // omitting concave ones, 1 pixel wide:
   draw_id_sils(true, 3.0);

   // draw triangles with polygon offset but not with ID colors:
   draw_id_triangles(true, true);

   draw_id_creases(3.0);

   return _patch->num_faces();
}


/***********************************************************************
 * Method : FeatureStrokeTexture::write_stream
 * Params : ostream& os
 * Returns: int
 * Effects: 
 ***********************************************************************/
int
FeatureStrokeTexture::write_stream(ostream& os)  const
{

   _decal_strokes.write_stream(os);

   if (_sil_and_crease_tex)
      _sil_and_crease_tex->write_stream(os);

   return 1;
}


/***********************************************************************
 * Method : FeatureStrokeTexture::read_stream
 * Params : istream&, str_list& leftover
 * Returns: int
 * Effects: 
 ***********************************************************************/
int
FeatureStrokeTexture::read_stream(istream &is, str_list& leftover)
{
   assert(DecalLineStroke::mesh() == 0);

   assert(_patch);
   DecalLineStroke::set_mesh(_patch->mesh());

   // read in the decal strokes
   _decal_strokes.read_stream(is);

   BaseStroke* s = _decal_strokes.get_prototype();
   assert(s);
   assert(s->is_of_type(DecalLineStroke::static_name()));

   ((DecalLineStroke*)s)->set_patch(_patch);

   for (int j=0; j<_decal_strokes.num_strokes(); j++) {
      s = _decal_strokes.stroke_at(j);
      assert(s);
      assert(s->is_of_type(DecalLineStroke::static_name()));

      ((DecalLineStroke*)s)->set_patch(_patch);
   }

   DecalLineStroke::set_mesh(0);

   int i;

   static IOBlockList blocklist;

   if (blocklist.num() == 0) {
      blocklist += new IOBlockMeth<FeatureStrokeTexture>("GTEXTURE",
                   &FeatureStrokeTexture::read_gtexture,  this);
   } else {
      for (i = 0; i < blocklist.num(); i++) {
         ((IOBlockMeth<FeatureStrokeTexture> *) blocklist[i])->set_obj(this);
      }
   }

   const int retval = IOBlock::consume(is, blocklist, leftover);
   changed();
   return retval;
}

int
FeatureStrokeTexture::read_gtexture(
   istream & is,
   str_list & leftover )
{
   // XXX much from Patch::read_gtexture()
   leftover.clear();
   const int namelen = 256;
   char name1[namelen];
   char name[namelen];
   char *wherecr;
   is.getline(name1, namelen); // Finish "#BEGIN GTEXTURE" line
   is.getline(name, namelen); // Get name of texture

   // Strip out CR
   // XXX - also in ViewStroke::read_stroke()
   while ((wherecr = strchr(name, '\015'))) {
      *wherecr = 0;
   }

   ARRAY<GTexture *> all_tex;
   all_tex += _sil_and_crease_tex;

   GTexture *tex = 0;
   Cstr_ptr &name_str = name;
   for (int i = 0; !tex && i< all_tex.num(); i++) {
      if (all_tex[i]->class_name() == name_str) {
         tex = all_tex[i];
      }
   }

   if (!tex) {
      err_mesg(ERR_LEV_WARN, "FeatureStrokeTexture::read_gtexture() - Warning! Texture subclasses not supported '%s'", name);

      tex = (GTexture*) DATA_ITEM::lookup(name);

      if (!tex) {
         err_mesg(ERR_LEV_ERROR, "FeatureStrokeTexture::read_gtexture() - Could not find GTexture '%s' (skipping)...", name);
         return 1;
      }
      if (!(tex = (GTexture*)tex->dup())) {
         err_mesg(ERR_LEV_ERROR, "FeatureStrokeTexture::find_gtexture() - tex->dup() returned nil");
         return -1;
      }
   }

   return tex->read_stream(is, leftover);
}


DecalLineStroke*
FeatureStrokeTexture::get_decal_stroke()
{
   // patch must be set for decal strokes to work
   assert(_patch);

   if(!_decal_strokes.get_prototype() ||
      !_decal_strokes.get_prototype()
      ->is_of_type(DecalLineStroke::static_name()))
      _decal_strokes.set_prototype(new DecalLineStroke(_patch));

   BaseStroke* s = _decal_strokes.get_stroke();
   assert(s->is_of_type(DecalLineStroke::static_name()));
   return  (DecalLineStroke*)s;
}


DecalLineStroke*
FeatureStrokeTexture::get_decal_stroke_proto()
{
   // patch must be set for decal strokes to work
   assert(_patch);

   if(!_decal_strokes.get_prototype() ||
      !_decal_strokes.get_prototype()
      ->is_of_type(DecalLineStroke::static_name())) {
      _decal_strokes.set_prototype(new DecalLineStroke(_patch));
   }
   return (DecalLineStroke*)_decal_strokes.get_prototype();
}


BStrokePool*
FeatureStrokeTexture::get_decal_stroke_pool()
{
   // patch must be set for decal strokes to work
   assert(_patch);

   // make sure stroke pool is inited correctly
   if(!_decal_strokes.get_prototype() ||
      !_decal_strokes.get_prototype()
      ->is_of_type(DecalLineStroke::static_name())) {
      _decal_strokes.set_prototype(new DecalLineStroke(_patch));
   }

   return &_decal_strokes;
}

void
FeatureStrokeTexture::notify_xform(BMESH* m, CWtransf& t, CMOD&)
{
   err_mesg(ERR_LEV_SPAM, "FeatureStrokeTexture::notify_xform()");

   assert(_patch->mesh() == m);

   _decal_strokes_need_update = true;

   for ( int i=0; i<_decal_strokes.num_strokes(); i++ ) {
      OutlineStroke* s = _decal_strokes.stroke_at(i);
      assert(s);
      ((DecalLineStroke*)s)->xform_locations(t);
   }

   //     if(_sil_and_crease_tex)
   //        _sil_and_crease_tex->set_orig_mesh_size();

}
