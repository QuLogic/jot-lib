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

#include <cctype>      // isspace()

#include "geom/gl_view.H"
#include "gtex/paper_effect.H"
#include "mesh/ioblock.H"
#include "net/io_manager.H"
#include "std/config.H"

#include "npr_texture.H"

using namespace mlib;

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       NPRTexture::_nt_tags = 0;

bool NPRTexture::_show_strokes = true;
bool NPRTexture::_show_coats = true;

static bool ZX_NEW_BRANCH = Config::get_var_bool("ZX_NEW_BRANCH",true,true);

/////////////////////////////////////
// Constructor
/////////////////////////////////////
NPRTexture::NPRTexture(Patch *patch) : 
   OGLTexture(patch),
   _stroke_tex(new FeatureStrokeTexture(patch)),
   _hatching_collection(new HatchingCollection(patch)),
   _basecoat_id(-1),
   _bkg_tex(new NPRBkgTexture(patch)),
   _ctrl_frm(new NPRControlFrameTexture(patch)),

   _transparent(1),
   _annotate(1),

   _polygon_offset_factor(
      (float)Config::get_var_dbl("BMESH_OFFSET_FACTOR", 1.0, true)
      ),
   _polygon_offset_units(
      (float)Config::get_var_dbl("BMESH_OFFSET_UNITS", 1.0, true)
      ),

   _see_thru(false),

   _selected(false),

   _data_file(NULL_STR),
   _in_data_file(false)

{
   assert(_stroke_tex);
   assert(_hatching_collection);

   assert(_bkg_tex);
   assert(_ctrl_frm);

   for (int i=0; i<ZXFLAG_NUM; i++)
      _see_thru_flags += false;

   _see_thru_flags[ZXFLAG_SIL_VISIBLE] = true;
   _see_thru_flags[ZXFLAG_CREASE_VISIBLE] = true;
   _see_thru_flags[ZXFLAG_BORDER_VISIBLE] = true;
}

/////////////////////////////////////
// insert_basecoat()
/////////////////////////////////////
void
NPRTexture::insert_basecoat(int i, GTexture* t)
{
   assert(i<=_basecoats.num());
   assert(t);

   _basecoats.insert(i,1);
   _basecoats[i] = t;
}

/////////////////////////////////////
// remove_basecoat()
/////////////////////////////////////
void
NPRTexture::remove_basecoat(int i)
{
   assert(i<_basecoats.num());

   int k;
   ARRAY<GTexture*> temp;

   for (k=0;   k<i; k++)
      temp += _basecoats[k];
   for (k=i+1; k<_basecoats.num(); k++)
      temp += _basecoats[k];

   delete _basecoats[i];

   _basecoats.clear();

   _basecoats += temp;

}

/////////////////////////////////////
// get_color()
/////////////////////////////////////
COLOR
NPRTexture::get_color() const
{
	
   if (_basecoats.num() == 0) {
      return _bkg_tex->get_color();
   } else {
      if (NPRSolidTexture::isa(_basecoats[0])) {
         return (((NPRSolidTexture*)_basecoats[0])->get_color());
      } else if (XToonTexture::isa(_basecoats[0])) {
         return (((XToonTexture*)_basecoats[0])->get_color());
      } else if (GLSLXToonShader::isa(_basecoats[0])) {
         return (((GLSLXToonShader*)_basecoats[0])->get_color());
      } else
         return COLOR(1,0,0);
   }
}

/////////////////////////////////////
// get_alpha()
/////////////////////////////////////
double
NPRTexture::get_alpha() const
{
   if (_basecoats.num() == 0) {
      return _bkg_tex->get_alpha();
   } else {
      if (NPRSolidTexture::isa(_basecoats[0])) {
         return (((NPRSolidTexture*)_basecoats[0])->get_alpha());
      } else if (XToonTexture::isa(_basecoats[0])) {
         return (((XToonTexture*)_basecoats[0])->get_alpha());
      } else if (GLSLXToonShader::isa(_basecoats[0])) {
         return (((GLSLXToonShader*)_basecoats[0])->get_alpha());
      } else
         return 1.0;
   }
}

void 
NPRTexture::request_ref_imgs()
{
   static bool suppress_id =
      Config::get_var_bool("SUPPRESS_NPR_TEX_ID_IMAGE",false);
   if (suppress_id) {
      return;
   }
   //XXX - Oughta just do this stuff right...
   static bool HACK_ID_UPDATE = Config::get_var_bool("HACK_ID_UPDATE",false);
   if (HACK_ID_UPDATE) {
      _stroke_tex->request_ref_imgs();
   } else {
      IDRefImage::schedule_update();
   }
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
NPRTexture::draw(CVIEWptr& v)
{

   static double a = Config::get_var_dbl("NPR_SELECT_ALPHA", 0.3, true);
   int i, n = 0;

   if (_ctrl)
      return _ctrl->draw(v);

   if (_show_coats ) {
      //GLSLXToon Shader doesn't need to use this background texture
      //layer...infact in messes it up, might be a easier way to set
      //this up though with the get_transparent function.
      if (get_transparent() && _basecoats.num() != 0 &&
          !(GLSLXToonShader::isa(_basecoats[0])))
         n += _bkg_tex->draw(v);

      //for all basecoats activated draw
      for (i=0; i<_basecoats.num(); i++)
         n += _basecoats[i]->draw(v);
   }

   //if a model is selected, draw the selection border around it(i think)
   if (_selected) {

      HSVCOLOR hsv(get_color());

      hsv[1] = 0.0;

      if (hsv[2] < 0.5)
         hsv[2] = 1.0;
      else //(hsv[2] < 0.5)
         hsv[2] = 0.0;

      _ctrl_frm->set_alpha(a);
      _ctrl_frm->set_color(hsv);

      n += _ctrl_frm->draw(v);
   }
   return n;
}

/////////////////////////////////////
// draw_final()
/////////////////////////////////////
int
NPRTexture::draw_final(CVIEWptr& v)
{
   int n = 0;

   if (_show_strokes) {
      n += _stroke_tex->draw(v);
      notify_sdo_draw(_stroke_tex->sil_and_crease_tex());
      n += _hatching_collection->draw(v);
   }

   if (_selected)
      n += _ctrl_frm->draw_final(v);

   return n;
}


// *********************************************************
//
// There are now 5 passes to the ID ref image.
//
// Each pass is performed for the whole scene before
// moving to the next pass. The order is as follows...
//
// draw_id_ref_pre1()  -- NEW
// draw_id_ref_pre2()  -- NEW
// draw_id_ref_pre3()  -- NEW
// draw_id_ref_pre4()  -- NEW
// draw_id_ref()       -- OLD
//
// Maybe we'd prefer to have post calls (rather that pre)??
//
// Anyhoo, to keep things safe, have some flag that branches around
// any new code.  When set, the 3 new calls do something, and the
// old call is modified. When unset, the 3 new calls do nothing
// and the old call executes as is presently...

/////////////////////////////////////
// draw_id_ref_pre1()
/////////////////////////////////////

int
NPRTexture::draw_id_ref_pre1()
{
   //***Phil, leave this stuff here...

   //Stick this here so that transparency is set before
   //actually drawing, and we get in and out of the
   //sorted list of blended objects appropriately...
   if (_patch) {
      int t = get_transparent();

      if ((t==0) != _patch->mesh()->geom()->has_transp()) {
         if (t==0)
            _patch->mesh()->geom()->set_transp(0.9999f);
         else
            _patch->mesh()->geom()->unset_transp();

         TRANSPobs::notify_transp_obs(_patch->mesh()->geom());
      }
   }

   //Stick this here to avoid drawing creases in the
   //'old' way when they're been flagged for use
   //in the sil pipeline
   if (_stroke_tex) {
      _stroke_tex->sil_and_crease_tex()->set_hide_crease(
         (_see_thru_flags[ZXFLAG_CREASE_VISIBLE] ||
          _see_thru_flags[ZXFLAG_CREASE_HIDDEN] ||
          _see_thru_flags[ZXFLAG_CREASE_OCCLUDED]) );
   }

   //***Phil, put your goods here...
   //CHECK FOR CHANGES TO DRAW CONTROLS

   //sync the zx_edge_stroke_texture to the current state;
   _stroke_tex->set_new_branch(ZX_NEW_BRANCH);

   _stroke_tex->set_seethru(_see_thru);
   for ( int i = 0; i < ZXFLAG_NUM; i++ )
      _stroke_tex->sil_and_crease_tex()->set_zxsil_render_flag(i, _see_thru_flags[i]);

   //Siggraph03 hack (1/19/03)
   _stroke_tex->sil_and_crease_tex()->zx_edge_tex()->set_crease_max_bend_angle(
      _stroke_tex->sil_and_crease_tex()->get_crease_max_bend_angle());

   //seethru silhouette texture:: draw object triangles (color and z)
   if ( ZX_NEW_BRANCH ) {
      int n=0;
      if (get_annotate() && _see_thru ) {
         n += _stroke_tex->draw_id_ref_pre1();
         n += draw_id_triangles(true, true,
                                _polygon_offset_factor,
                                _polygon_offset_units);
      }
      return n;

   }

   return 0;
}

/////////////////////////////////////
// draw_id_ref_pre2()
/////////////////////////////////////
int
NPRTexture::draw_id_ref_pre2()
{
   //seethru silhouette texture:: hidden silhouette pass (WITH depth check - on cleared Z buffer)
   if ( ZX_NEW_BRANCH ) {
      int n=0;
      if ( get_annotate() && _see_thru ) {
         n += _stroke_tex->draw_id_ref_pre2();
      }
      return n;
   } else {
      return 0;
   }
}

/////////////////////////////////////
// draw_id_ref_pre3()
/////////////////////////////////////
int
NPRTexture::draw_id_ref_pre3()
{
   //seethru silhouette texture:: regenerate id triangle z values (no color) - on cleared z buffer
   if ( ZX_NEW_BRANCH ) {
      int n=0;
      if( get_annotate() && _see_thru ) {

         n += _stroke_tex->draw_id_ref_pre3();

         //Just into z cause color buffer is masked out
         n += draw_id_triangles(true, true,
                                _polygon_offset_factor,
                                _polygon_offset_units);
      }
      return n;
   } else {
      return 0;
   }
}
/////////////////////////////////////
// draw_id_ref_pre4()
/////////////////////////////////////
int
NPRTexture::draw_id_ref_pre4()
{
   //seethru silhouette texture:: visible silhouette pass (enable depth check )
   if ( ZX_NEW_BRANCH ) {
      int n=0;
      if( get_annotate() && _see_thru ) {
         n += _stroke_tex->draw_id_ref_pre4();

         if  ( !_see_thru_flags[ZXFLAG_CREASE_VISIBLE] &&
               !_see_thru_flags[ZXFLAG_CREASE_HIDDEN] &&
               !_see_thru_flags[ZXFLAG_CREASE_OCCLUDED])
            n += draw_id_creases(2.0);    //draw creases to idref if they
         //aren't being drawn by ZXedgestroke..
      }
      return n;
   } else {
      return 0;
   }
}

/////////////////////////////////////
// draw_id_ref()
/////////////////////////////////////
int
NPRTexture::draw_id_ref()
{

   //***Phil, prevent this from happening when using the new method!

   // Start w/ ID triangles, needed by hatching groups:
   int n=0;

   //Hack... If the annotatable box is unchecked, we just don't
   //draw the id image! (Useful for some transparecy situations.)

   if ( ZX_NEW_BRANCH ) {
      if (get_annotate() && !_see_thru ) {

         //still need to do the standard routine for opaque objects

         n += _stroke_tex->draw_id_ref();

         if  ( !_see_thru_flags[ZXFLAG_CREASE_VISIBLE] &&
               !_see_thru_flags[ZXFLAG_CREASE_HIDDEN] &&
               !_see_thru_flags[ZXFLAG_CREASE_OCCLUDED])
            n += draw_id_creases(2.0);

         n += draw_id_triangles(true, true,
                                _polygon_offset_factor,
                                _polygon_offset_units);
      }
   } else if (get_annotate()) {
      // Current status (1/3/02):
      //   FeatureStrokeTexture::draw_id_ref() just calls
      //   SilAndCreaseTexture::draw_id_ref(), which just calls
      //   ZXedgeStrokeTexture::draw_id_ref(), which just draws
      //   'loop IDs' into the reference image.


      n += _stroke_tex->draw_id_ref();

      // XXX - Why draw creases?
      //       If the SilAndCreaseTexture needs them,
      //       shouldn't it draw them?
      if  ( !_see_thru_flags[ZXFLAG_CREASE_VISIBLE] &&
            !_see_thru_flags[ZXFLAG_CREASE_HIDDEN] &&
            !_see_thru_flags[ZXFLAG_CREASE_OCCLUDED])
         n += draw_id_creases(2.0);

      n += draw_id_triangles(true, true,
                             _polygon_offset_factor,
                             _polygon_offset_units);
   } else {
      //n += draw_id_triangles(true, true);
   }

   return n;
}

/////////////////////////////////////
// draw_vis_ref()
/////////////////////////////////////
int
NPRTexture::draw_vis_ref()
{
   int n = draw_id_triangles(true, true);

   if (get_annotate()) {
      n += draw_id_creases(3.0);
      n += draw_id_sils(true, 3.0);
   }

   return n;
}


// XXX - Deprecated
/////////////////////////////////////
// write_stream()
/////////////////////////////////////
int
NPRTexture::write_stream(ostream &os) const
{
   err_mesg(ERR_LEV_WARN,
            "NPRTexture::write_stream() - Piping to newer tag method...");

   os << _begin_tag;

   STDdstream stream(&os);
   old_format(stream);

   os << _end_tag << endl;
   return 1;
}

// XXX - Deprecated
/////////////////////////////////////
// read_stream()
/////////////////////////////////////
int
NPRTexture::read_stream(istream &is, str_list & leftover)
{
   err_mesg(ERR_LEV_WARN,
            "NPRTexture::read_stream() - Piping to newer tag method...\n");

   int retval = 1;

   STDdstream str(&is);
   decode(str);

   // XXX - REALLY Deprecated IO technique. We're already done
   // unless we discover a legacy IOBlock called GTEXTURE
   //
   char pound;

   // Eat whitespace:
   // XXX - Is there a portable way to do this?
   //       iostream::eatwhite() is Microsoft-only.
   while (isspace(is.peek()))
      is.get();

   if (is.peek() == '#') {
      is >> pound;
      // See if we're at a #BEGIN GTEXTURE
      if ((pound == '#') && (is.peek() == 'B')) {
         is.putback(pound);
         err_mesg(ERR_LEV_WARN, "NPRTexture::read_stream() - ******* <BEGIN> LOADING 'OLD' FORMAT FILE *******");

         static IOBlockList blocklist;

         if (blocklist.num() == 0) {
            blocklist +=
               new IOBlockMeth<NPRTexture>("GTEXTURE",
                                           &NPRTexture::read_gtexture,  this);
         } else {
            for (int i = 0; i < blocklist.num(); i++)
               ((IOBlockMeth<NPRTexture> *) blocklist[i])->set_obj(this);
         }
         retval = IOBlock::consume(is, blocklist, leftover);
         err_mesg(ERR_LEV_WARN, "NPRTexture::read_stream() - ******* <END> LOADING 'OLD' FORMAT FILE *******");
      } else {
         is.putback(pound);
      }
   }
   changed();

   return retval;
}

/////////////////////////////////////
// read_gtexture()
/////////////////////////////////////

//XXX - This is deprecated. It'll
//go away when we convert over
//our old files...

int
NPRTexture::read_gtexture(
   istream & is,
   str_list & leftover )
{
   assert(_stroke_tex);

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

   Cstr_ptr &name_str = name;

   // we only support reading the _stroke_texture sub-texture
   assert(_stroke_tex->class_name() == name_str);

   return _stroke_tex->read_stream(is, leftover);
}



/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
NPRTexture::tags() const
{
   if (!_nt_tags) {
      _nt_tags = new TAGlist;
      *_nt_tags += OGLTexture::tags();

      *_nt_tags += new TAG_meth<NPRTexture>(
         "npr_data_file",
         &NPRTexture::put_npr_data_file,
         &NPRTexture::get_npr_data_file,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "collection",
         &NPRTexture::put_collection,
         &NPRTexture::get_collection,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "line_stroke_texture",
         &NPRTexture::put_line_stroke_texture,
         &NPRTexture::get_line_stroke_texture,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "feature_stroke_texture",
         &NPRTexture::put_feature_stroke_texture,
         &NPRTexture::get_feature_stroke_texture,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "transparent",
         &NPRTexture::put_transparent,
         &NPRTexture::get_transparent,
         0);
      *_nt_tags += new TAG_meth<NPRTexture>(
         "annotate",
         &NPRTexture::put_annotate,
         &NPRTexture::get_annotate,
         0);
      *_nt_tags += new TAG_meth<NPRTexture>(
         "polygon_offset_factor",
         &NPRTexture::put_polygon_offset_factor,
         &NPRTexture::get_polygon_offset_factor,
         0);
      *_nt_tags += new TAG_meth<NPRTexture>(
         "polygon_offset_units",
         &NPRTexture::put_polygon_offset_units,
         &NPRTexture::get_polygon_offset_units,
         0);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "see_thru",
         &NPRTexture::put_see_thru,
         &NPRTexture::get_see_thru,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "see_thru_flags",
         &NPRTexture::put_see_thru_flags,
         &NPRTexture::get_see_thru_flags,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "basecoat",
         &NPRTexture::put_basecoats,
         &NPRTexture::get_basecoat,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "texture_id",
         &NPRTexture::put_basecoat_id,
         &NPRTexture::get_basecoat_id,
         0);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "bkg_texture",
         &NPRTexture::put_bkg_texture,
         &NPRTexture::get_bkg_texture,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "solid_texture",
         &NPRTexture::put_solid_texture,
         &NPRTexture::get_solid_texture,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "toon_texture",
         &NPRTexture::put_toon_texture,
         &NPRTexture::get_toon_texture,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "xtoon_texture",
         &NPRTexture::put_xtoon_texture,
         &NPRTexture::get_xtoon_texture,
         1);

      //View stuff

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_color",
         &NPRTexture::put_view_color,
         &NPRTexture::get_view_color,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_alpha",
         &NPRTexture::put_view_alpha,
         &NPRTexture::get_view_alpha,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_paper_use",
         &NPRTexture::put_view_paper_use,
         &NPRTexture::get_view_paper_use,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_paper_name",
         &NPRTexture::put_view_paper_name,
         &NPRTexture::get_view_paper_name,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_paper_active",
         &NPRTexture::put_view_paper_active,
         &NPRTexture::get_view_paper_active,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_texture",
         &NPRTexture::put_view_texture,
         &NPRTexture::get_view_texture,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_coords",
         &NPRTexture::put_view_light_coords,
         &NPRTexture::get_view_light_coords,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_positional",
         &NPRTexture::put_view_light_positional,
         &NPRTexture::get_view_light_positional,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_cam_space",
         &NPRTexture::put_view_light_cam_space,
         &NPRTexture::get_view_light_cam_space,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_color_diff",
         &NPRTexture::put_view_light_color_diff,
         &NPRTexture::get_view_light_color_diff,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_color_amb",
         &NPRTexture::put_view_light_color_amb,
         &NPRTexture::get_view_light_color_amb,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_global",
         &NPRTexture::put_view_light_color_global,
         &NPRTexture::get_view_light_color_global,
         1);

      *_nt_tags += new TAG_meth<NPRTexture>(
         "view_light_enable",
         &NPRTexture::put_view_light_enable,
         &NPRTexture::get_view_light_enable,
         1);


   }
   return *_nt_tags;
}

/////////////////////////////////////
// old_format()
/////////////////////////////////////
// XXX - We need to override format() so
// we can make the name of this on a line by itself
// XXX - But this was needed only for the old
// way, so new we only do this when writing legacy streams...
/////////////////////////////////////
STDdstream  &
NPRTexture::old_format(STDdstream &ds) const
{
   TAGformat d(&ds, class_name(), 1);

   err_mesg(ERR_LEV_WARN, "NPRTexture::old_format()");

   (*d) << class_name();
   (*d).ws("\n");
   (*d).write_open_delim();

   // the rest is the same
   for (int i=0; i<tags().num(); i++)
      tags()[i]->format(this, *d);  // write name value pair
   ds.write_newline();         // add carriage return
   d.end_id();            // write end_delimiter

   return *d;
}

/////////////////////////////////////
// get_npr_data_file()
/////////////////////////////////////
void
NPRTexture::get_npr_data_file (TAGformat &d)
{
   //Sanity check...
   assert(!_in_data_file);

   str_ptr str;
   *d >> str;

   if (str == "NULL_STR") {
      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_npr_data_file() - Loaded NULL string.");
      _data_file = NULL_STR;
   } else {
      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_npr_data_file() - Loaded string: '%s'.", **str);

      _data_file = str;

      str_ptr fname = IOManager::load_prefix() + str + ".npr";

      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_npr_data_file() - Opening: '%s'...", **fname);

      fstream fin;
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/

      fin.open(**(fname),ios::in | ios::nocreate);
#else

      fin.open(**(fname),ios::in);
#endif


      if (!fin) {
         err_mesg(ERR_LEV_ERROR, "NPRTexture::get_npr_data_file() - Could not open: '%s'...", **fname);
         //but leave the _data_file as is so future
         //serialization could create it
      } else {
         STDdstream s(&fin);
         s >> str;

         if (str != NPRTexture::static_name()) {
            err_mesg(ERR_LEV_ERROR, "NPRTexture::get_npr_data_file() - Not NPRTexture: '%s'...", **str);
         } else {
            _in_data_file = true;
            decode(s);
            _in_data_file = false;
         }
      }
   }
}

/////////////////////////////////////
// put_npr_data_file()
/////////////////////////////////////
void
NPRTexture::put_npr_data_file (TAGformat &d) const
{
   //Ignore this tag if we're presently writing to
   //an external npr file...
   if (_in_data_file)
      return;

   //If there's no npr file, dump the null string
   if (_data_file == NULL_STR) {
      d.id();
      *d << str_ptr("NULL_STR");
      d.end_id();
   }
   //Otherwise, try to open the file, then write the
   //tags to the external file, and dump the filename
   //to the given tag stream
   else {
      str_ptr fname = IOManager::save_prefix() + _data_file + ".npr";
      fstream fout;
      fout.open(**fname,ios::out);
      //If this fails, then dump the null string
      if (!fout) {
         err_mesg(ERR_LEV_ERROR, "NPRTexture::put_npr_data_file -  Could not open: '%s', so changing to using no external npr file...!", **fname);

         //and actually change tex's policy so the
         //tags serialize into the main stream
         ((str_ptr)_data_file) = NULL_STR;

         d.id();
         *d << str_ptr("NULL_STR");
         d.end_id();
      }
      //Otherwise, do the right thing
      else {
         d.id();
         *d << _data_file;
         d.end_id();

         //Set the flag so tags will know we're in a data file

         // the ((NPRTexture*)this)-> stuff is to "cast away" the
         // const of this function so that the _in_data_file
         // member can be modified
         ((NPRTexture*)this)->_in_data_file = true;
         STDdstream stream(&fout);
         format(stream);
         ((NPRTexture*)this)->_in_data_file = false;

      }
   }
}

/////////////////////////////////////
// put_collection()
/////////////////////////////////////
void
NPRTexture::put_collection(TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   err_mesg(ERR_LEV_SPAM, "NPRTexture::put_collection()");

   assert(_hatching_collection);
   d.id();
   _hatching_collection->format(*d);
   d.end_id();

}


/////////////////////////////////////
// get_collection()
/////////////////////////////////////
void
NPRTexture::get_collection(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_SPAM, "NPRTexture::get_collection()");

   assert(_hatching_collection);

   //Grab the class name... should be HatchingCollection
   str_ptr str;
   *d >> str;

   if ((str != HatchingCollection::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_collection() - Not 'HatchingCollection': '%s'!!", **str);

      return;
   }

   _hatching_collection->decode(*d);
}

/////////////////////////////////////
// put_line_stroke_texture()
/////////////////////////////////////
void
NPRTexture::put_line_stroke_texture(TAGformat &d) const
{
   // XXX - Deprecated
}


/////////////////////////////////////
// get_line_stroke_texture()
/////////////////////////////////////
void
NPRTexture::get_line_stroke_texture(TAGformat &d)
{

   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_WARN, "NPRTexture::get_line_stroke_texture() -- Eating old file format nonesense...");

   //Deprecated - Suck it up

   bool start = false;
   int count = 0;
   char buf;

   while(!start || count > 0) {
      *d >> buf;
      if      (buf == '{') { count++; start = true; } else if (buf == '}') { count--; }
   }
}

/////////////////////////////////////
// put_feature_stroke_texture()
/////////////////////////////////////
void
NPRTexture::put_feature_stroke_texture(TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   err_mesg(ERR_LEV_SPAM, "NPRTexture::put_feature_stroke_texture()");

   assert(_stroke_tex);
   d.id();
   _stroke_tex->format(*d);
   d.end_id();

}


/////////////////////////////////////
// get_feature_stroke_texture()
/////////////////////////////////////
void
NPRTexture::get_feature_stroke_texture(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_SPAM, "NPRTexture::get_feature_stroke_texture()");

   assert(_stroke_tex);

   //Grab the class name... should be FeatureStrokeTexture
   str_ptr str;
   *d >> str;

   if ((str != FeatureStrokeTexture::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_feature_stroke_texture() - Not FeatureStrokeTexture: '%s'!!", **str);
      return;
   }

   _stroke_tex->decode(*d);
}

/////////////////////////////////////
// put_basecoat_id()
/////////////////////////////////////
void
NPRTexture::put_basecoat_id(TAGformat &d) const
{
   // XXX - Deprecated
}


/////////////////////////////////////
// get_basecoat_id()
/////////////////////////////////////
void
NPRTexture::get_basecoat_id(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_WARN, "NPRTexture::get_basecoat_id() - ***NOTE: Loading OLD format file!***");

   *d >> _basecoat_id;
}

/////////////////////////////////////
// put_bkg_texture()
/////////////////////////////////////
void
NPRTexture::put_bkg_texture(TAGformat &d) const
{

   // XXX - Deprecated

}


/////////////////////////////////////
// get_bkg_texture()
/////////////////////////////////////
void
NPRTexture::get_bkg_texture(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_WARN, "NPRTexture::get_bkg_texture() - ***NOTE: Loading OLD format file!***");

   // Grab this thing, and strip out the info
   // if its the 'current' basecoat

   str_ptr str;
   *d >> str;

   if ((str != NPRBkgTexture::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_bkg_texture() - Not NPRBkgTexture: '%s'!!",**str);
      return;
   }

   NPRBkgTexture t;
   t.decode(*d);
   if (_basecoat_id == 0) {
      set_transparent(t.get_transparent());
      set_annotate(t.get_annotate());
   }
}

/////////////////////////////////////
// put_solid_texture()
/////////////////////////////////////
void
NPRTexture::put_solid_texture(TAGformat &d) const
{
   // XXX - Deprecated
}


/////////////////////////////////////
// get_solid_texture()
/////////////////////////////////////
void
NPRTexture::get_solid_texture(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_WARN, "NPRTexture::get_solid_texture() - ***NOTE: Loading OLD format file!***");


   str_ptr str;
   *d >> str;

   if ((str != NPRSolidTexture::static_name())) {
      // XXX - should throw away stuff from unknown obj

      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_solid_texture() - Not NPRSolidTexture: '%s'!!",**str);
      return;
   }

   if (_basecoat_id == 1) {
      NPRSolidTexture *t = new NPRSolidTexture(_patch);
      t->decode(*d);
      set_transparent(t->get_transparent());
      set_annotate(t->get_annotate());
      insert_basecoat(get_basecoat_num(),t);
   } else {
      NPRSolidTexture t;
      t.decode(*d);
   }
}


/////////////////////////////////////
// put_toon_texture()
/////////////////////////////////////
void
NPRTexture::put_toon_texture(TAGformat &d) const
{
   // XXX - Deprecated
}


/////////////////////////////////////
// get_toon_texture()
/////////////////////////////////////
void
NPRTexture::get_toon_texture(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_WARN, "NPRTexture::get_toon_texture() - ***NOTE: Loading OLD format file!***");

   str_ptr str;
   *d >> str;

   if ((str != XToonTexture::static_name())) {
      // XXX - should throw away stuff from unknown obj

      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_toon_texture() - Not XToonTexture: '%s'!!",**str);
      return;
   }

   if (_basecoat_id == 2) {
	   cout << "d" << endl;
      XToonTexture *t = new XToonTexture(_patch);
      t->decode(*d);
      set_transparent(t->get_transparent());
      set_annotate(t->get_annotate());
      insert_basecoat(get_basecoat_num(),t);
   } else {
 cout << "ds" << endl;
      XToonTexture t;
      t.decode(*d);
   }

}


/////////////////////////////////////
// put_toon_texture()
/////////////////////////////////////
void
NPRTexture::put_xtoon_texture(TAGformat &d) const
{
   // XXX - Deprecated
}


/////////////////////////////////////
// get_toon_texture()
/////////////////////////////////////
void
NPRTexture::get_xtoon_texture(TAGformat &d)
{
	cout << "D" << endl;
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_WARN, "NPRTexture::get_xtoon_texture() - ***NOTE: Loading OLD format file!***");

   str_ptr str;
   *d >> str;

   if ((str != GLSLXToonShader::static_name())) {
      // XXX - should throw away stuff from unknown obj

      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_xtoon_texture() - Not GLSLXToonTexture: '%s'!!",**str);
      return;
   }

   if (_basecoat_id == 3) {
      GLSLXToonShader *t = new GLSLXToonShader(_patch);
      t->decode(*d);
      set_transparent(t->get_transparent());
      set_annotate(t->get_annotate());
      insert_basecoat(get_basecoat_num(),t);
   } else {
	  GLSLXToonShader t;
      t.decode(*d);
   }

}

////////////////////////////////////
// put_transparent()
/////////////////////////////////////
void
NPRTexture::put_transparent (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << _transparent;
   d.end_id();

}

/////////////////////////////////////
// get_transparent()
/////////////////////////////////////
void
NPRTexture::get_transparent (TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   if (!( ( _in_data_file && (_data_file != NULL_STR)) ||
          (!_in_data_file && (_data_file == NULL_STR))    )) {
      err_mesg(ERR_LEV_WARN, "NPRTexture::get_transparent - WARNING!! This tag should not be here. Resave this file!");
   }

   *d >> _transparent;

}

/////////////////////////////////////
// put_annotate()
/////////////////////////////////////
void
NPRTexture::put_annotate (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << _annotate;
   d.end_id();

}

/////////////////////////////////////
// get_annotate()
/////////////////////////////////////
void
NPRTexture::get_annotate (TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   if (!( ( _in_data_file && (_data_file != NULL_STR)) ||
          (!_in_data_file && (_data_file == NULL_STR))    )) {
      err_mesg(ERR_LEV_WARN, "NPRTexture::get_annotate - WARNING!! This tag should be here. Resave this file!");
   }

   *d >> _annotate;

}

/////////////////////////////////////
// put_polygon_offset_factor()
/////////////////////////////////////
void
NPRTexture::put_polygon_offset_factor (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << _polygon_offset_factor;
   d.end_id();

}

/////////////////////////////////////
// get_polygon_offset_factor()
/////////////////////////////////////
void
NPRTexture::get_polygon_offset_factor (TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   if (!( ( _in_data_file && (_data_file != NULL_STR)) ||
          (!_in_data_file && (_data_file == NULL_STR))    )) {
      err_mesg(ERR_LEV_WARN, "NPRTexture::get_polygon_offset_factor - WARNING!! This tag should be here. Resave this file!");
   }

   *d >> _polygon_offset_factor;

}

/////////////////////////////////////
// put_polygon_offset_units()
/////////////////////////////////////
void
NPRTexture::put_polygon_offset_units (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << _polygon_offset_units;
   d.end_id();

}

/////////////////////////////////////
// get_polygon_offset_units()
/////////////////////////////////////
void
NPRTexture::get_polygon_offset_units (TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   //Sanity check - might change policy on 2nd part
   if (!( ( _in_data_file && (_data_file != NULL_STR)) ||
          (!_in_data_file && (_data_file == NULL_STR))    )) {
      err_mesg(ERR_LEV_WARN, "NPRTexture::get_polygon_offset_units - WARNING!! This tag should be here. Resave this file!");
   }

   *d >> _polygon_offset_units;

}


/////////////////////////////////////
// put_see_thru()
/////////////////////////////////////
void
NPRTexture::put_see_thru (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << ((_see_thru)?(1):(0));
   d.end_id();

}

/////////////////////////////////////
// get_see_thru()
/////////////////////////////////////
void
NPRTexture::get_see_thru (TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   int s;
   *d >> s;
   set_see_thru(s==1);
}

/////////////////////////////////////
// put_see_thru_flags()
/////////////////////////////////////
void
NPRTexture::put_see_thru_flags (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   assert(_see_thru_flags.num() == ZXFLAG_NUM);
   *d << ZXFLAG_NUM;
   for (int i=0; i<ZXFLAG_NUM; i++) {
      *d << ((_see_thru_flags[i])?(1):(0));
   }
   d.end_id();

}

/////////////////////////////////////
// get_see_thru_flags()
/////////////////////////////////////
void
NPRTexture::get_see_thru_flags (TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   int n;

   *d >> n;

   assert(n == ZXFLAG_NUM);

   for (int i=0; i<ZXFLAG_NUM; i++) {
      int f;
      *d >> f;
      _see_thru_flags[i] = (f==1);
   }

}
/////////////////////////////////////
// put_basecoats()
/////////////////////////////////////
void
NPRTexture::put_basecoats(TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   err_mesg(ERR_LEV_SPAM, "NPRTexture::put_basecoats()");

   for (int i=0; i< _basecoats.num(); i++) {
      d.id();
      _basecoats[i]->format(*d);
      d.end_id();
   }
}


/////////////////////////////////////
// get_basecoat()
/////////////////////////////////////
void
NPRTexture::get_basecoat(TAGformat &d)
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   err_mesg(ERR_LEV_SPAM, "NPRTexture::get_basecoat()");

   str_ptr str;
   *d >> str;

   if ((str == NPRSolidTexture::static_name())) {
      NPRSolidTexture *t = new NPRSolidTexture(_patch);
      t->decode(*d);
      insert_basecoat(get_basecoat_num(),t);
   } else if ((str == XToonTexture::static_name())) {
      XToonTexture *t = new XToonTexture(_patch);
      t->decode(*d);
      insert_basecoat(get_basecoat_num(),t);
   } else if ((str == GLSLXToonShader::static_name())) {
      GLSLXToonShader *t = new GLSLXToonShader(_patch);
      t->decode(*d);
      insert_basecoat(get_basecoat_num(),t);
   } else {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "NPRTexture::get_basecoat() - Not NPR[Toon|Solid]Texture: '%s'!!", **str);
      return;
   }

}
//**** VIEW STUFF ***//

// XXX - Deprecated the serialization of the view into .sm (use .jot!)

static bool put_view_stuff = Config::get_var_bool("PUT_VIEW_STUFF_IN_SM",false,true);

/////////////////////////////////////
// get_view_color()
/////////////////////////////////////
void
NPRTexture::get_view_color (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   COLOR c;

   *d >> c;

   v->set_color(c);
}

/////////////////////////////////////
// put_view_color()
/////////////////////////////////////
void
NPRTexture::put_view_color (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      d.id();
      *d << v->color();
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_alpha()
/////////////////////////////////////
void
NPRTexture::get_view_alpha (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   double a;

   *d >> a;

   v->set_alpha(a);
}

/////////////////////////////////////
// put_view_alpha()
/////////////////////////////////////
void
NPRTexture::put_view_alpha (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      d.id();
      *d << v->get_alpha();
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_paper_use()
/////////////////////////////////////
void
NPRTexture::get_view_paper_use (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   int p;

   *d >> p;

   v->set_use_paper((p==1)?true:false);
}

/////////////////////////////////////
// put_view_paper_use()
/////////////////////////////////////
void
NPRTexture::put_view_paper_use (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      d.id();
      *d << ((v->get_use_paper())?(1):(0));
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_paper_name()
/////////////////////////////////////
void
NPRTexture::get_view_paper_name (TAGformat &d)
{
   //XXX - May need something to handle filenames with spaces

   str_ptr str, tex;
   *d >> str;

   if (str == "NULL_STR") {
      tex = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_view_paper_name() - Loaded NULL string.");
   } else {
      tex = str;
      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_view_paper_name() - Loaded string: '%s'", **tex);
   }
   PaperEffect::set_paper_tex(tex);

}

/////////////////////////////////////
// put_view_paper_name()
/////////////////////////////////////
void
NPRTexture::put_view_paper_name (TAGformat &d) const
{
   if (put_view_stuff) {
      //XXX - May need something to handle filenames with spaces
      d.id();
      if (PaperEffect::get_paper_tex() == NULL_STR) {
         err_mesg(ERR_LEV_SPAM, "NPRTexture::put_view_paper_name() - Wrote NULL string.");
         *d << "NULL_STR";
         *d << " ";
      } else {
         //Here we strip off JOT_ROOT
         *d << PaperEffect::get_paper_tex();
         *d << " ";
         err_mesg(ERR_LEV_SPAM, "NPRTexture::put_view_paper_name() - Wrote string: '%s'", **PaperEffect::get_paper_tex());
      }
      d.end_id();
   }
}
/////////////////////////////////////
// get_view_paper_active()
/////////////////////////////////////
void
NPRTexture::get_view_paper_active (TAGformat &d)
{

   int a;

   *d >> a;

   PaperEffect::set_delayed_activate(a==1);


}

/////////////////////////////////////
// put_view_paper_active()
/////////////////////////////////////
void
NPRTexture::put_view_paper_active (TAGformat &d) const
{
   if (put_view_stuff) {
      d.id();
      *d << ((PaperEffect::is_active())?(1):(0));
      d.end_id();
   }
}
/////////////////////////////////////
// get_view_texture()
/////////////////////////////////////
void
NPRTexture::get_view_texture (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   str_ptr str;
   *d >> str;

   if (str == "NULL_STR") {
      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_view_texture() - Loaded NULL string.");
      v->set_bkg_file(NULL_STR);
   } else {
      err_mesg(ERR_LEV_SPAM, "NPRTexture::get_view_texture() - Loaded string: '%s'", **str);
      v->set_bkg_file(Config::JOT_ROOT()+str);
   }

}

/////////////////////////////////////
// put_view_texture()
/////////////////////////////////////
void
NPRTexture::put_view_texture (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      //XXX - May need something to handle filenames with spaces

      d.id();
      if (v->get_bkg_file() == NULL_STR) {
         err_mesg(ERR_LEV_SPAM, "NPRTexture::put_view_texture() - Wrote NULL string.");
         *d << "NULL_STR";
         *d << " ";
      } else {
         //Here we strip off JOT_ROOT

         str_ptr tex = v->get_bkg_file();
         str_ptr str;
         int i;
         for (i=Config::JOT_ROOT().len(); i<(int)tex.len(); i++)
            str = str + str_ptr(tex[i]);
         *d << **str;
         *d << " ";
         err_mesg(ERR_LEV_SPAM, "NPRTexture::put_view_texture() - Wrote string: '%s'", **str);
      }
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_coords()
/////////////////////////////////////
void
NPRTexture::get_view_light_coords (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   ARRAY<Wvec> c;

   *d >> c;

   assert(c.num()==4);

   for (int i=0; i<4; i++)
      v->light_set_coordinates_v(i,c[i]);

}

/////////////////////////////////////
// put_view_light_coords()
/////////////////////////////////////
void
NPRTexture::put_view_light_coords (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      ARRAY<Wvec> c;

      for (int i=0; i<4; i++)
         c.add(v->light_get_coordinates_v(i));

      d.id();
      *d << c;
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_positional()
/////////////////////////////////////
void
NPRTexture::get_view_light_positional (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   ARRAY<int> p;

   *d >> p;

   assert(p.num()==4);

   for (int i=0; i<4; i++)
      v->light_set_positional(i,(p[i]==1)?(true):(false));

}

/////////////////////////////////////
// put_view_light_positional()
/////////////////////////////////////
void
NPRTexture::put_view_light_positional (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      ARRAY<int> p;

      for (int i=0; i<4; i++)
         p.add((v->light_get_positional(i))?(1):(0));

      d.id();
      *d << p;
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_cam_space()
/////////////////////////////////////
void
NPRTexture::get_view_light_cam_space (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   ARRAY<int> c;

   *d >> c;

   assert(c.num()==4);

   for (int i=0; i<4; i++)
      v->light_set_in_cam_space(i,((c[i]==1)?(true):(false)));

}

/////////////////////////////////////
// put_view_light_cam_space()
/////////////////////////////////////
void
NPRTexture::put_view_light_cam_space (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      ARRAY<int> c;

      for (int i=0; i<4; i++)
         c.add((v->light_get_in_cam_space(i))?(1):(0));

      d.id();
      *d << c;
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_color_diff()
/////////////////////////////////////
void
NPRTexture::get_view_light_color_diff (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   ARRAY<COLOR> c;

   *d >> c;

   assert(c.num()==4);

   for (int i=0; i<4; i++)
      v->light_set_diffuse(i,c[i]);

}

/////////////////////////////////////
// put_view_light_color_diff()
/////////////////////////////////////
void
NPRTexture::put_view_light_color_diff (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      ARRAY<COLOR> c;

      for (int i=0; i<4; i++)
         c.add(v->light_get_diffuse(i));

      d.id();
      *d << c;
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_color_amb()
/////////////////////////////////////
void
NPRTexture::get_view_light_color_amb (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   ARRAY<COLOR> a;

   *d >> a;

   assert(a.num()==4);

   for (int i=0; i<4; i++)
      v->light_set_ambient(i,a[i]);

}

/////////////////////////////////////
// put_view_light_color_amb()
/////////////////////////////////////
void
NPRTexture::put_view_light_color_amb (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      ARRAY<COLOR> a;

      for (int i=0; i<4; i++)
         a.add(v->light_get_ambient(i));

      d.id();
      *d << a;
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_color_global()
/////////////////////////////////////
void
NPRTexture::get_view_light_color_global (TAGformat &d)
{
   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   COLOR g;

   *d >> g;

   v->light_set_global_ambient(g);

}

/////////////////////////////////////
// put_view_light_color_global()
/////////////////////////////////////
void
NPRTexture::put_view_light_color_global (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      d.id();
      *d << v->light_get_global_ambient();
      d.end_id();
   }
}

/////////////////////////////////////
// get_view_light_enable()
/////////////////////////////////////
void
NPRTexture::get_view_light_enable (TAGformat &d)
{

   VIEWptr v = VIEW::peek();
   assert(v != NULL);

   ARRAY<int> e;

   *d >> e;

   assert(e.num()==4);

   for (int i=0; i<4; i++)
      v->light_set_enable(i,(e[i]==1)?(true):(false));

}

/////////////////////////////////////
// put_view_light_enable()
/////////////////////////////////////
void
NPRTexture::put_view_light_enable (TAGformat &d) const
{
   if (put_view_stuff) {
      VIEWptr v = VIEW::peek();
      assert(v != NULL);

      ARRAY<int> e;

      for (int i=0; i<4; i++)
         e.add((v->light_get_enable(i))?(1):(0));

      d.id();
      *d << e;
      d.end_id();
   }
}

/////////////////////////////////////
// notify_sdo_draw()
/////////////////////////////////////
void
NPRTexture::notify_sdo_draw (SilAndCreaseTexture *t)
{
   for (int i=0; i<_sdolist.num(); i++)
      _sdolist[i]->notify_draw(t);


}
