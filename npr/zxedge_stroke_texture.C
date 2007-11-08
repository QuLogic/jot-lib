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
/*
* class to handle zero-crossing silhouettes
* and LuboSampling
*/
#include <cmath>
#include "mesh/lmesh.H"
#include "npr/npr_view.H"
#include "zxedge_stroke_texture.H"
#include "sil_and_crease_texture.H"
#include "std/config.H"
#include "gtex/curvature_ui.H"
// Must have std/support.H (actually windows.h) before gl.h so
// Windows is happy:
#include <GL/glu.h>

using namespace mlib;

//YYY Newer g++ warns about ambiguous template choice
//so we tell it explicitly...
#define TFMULTFIX (Point3<Wpt, Wvec>) // Not needed anymore (it seems)
#define VECMULTFIX (Vec3<Wvec>)       // Not used in this file

extern "C" void HACK_mouse_right_button_up();

// Experimental matrix operators for NDC points and vecs:
// XXX - to be moved somewhere more useful.
/*
inline NDCZpt 
operator*(CWtransf& xf, CNDCZpt& p)
{
   Wpt tmp(p[0], p[1], p[2]);
 
   tmp = xf * TFMULTFIX tmp; // f'ing solaris, man
 
   return NDCZpt(tmp[0], tmp[1], tmp[2]);
}
*/
/*
inline NDCZvec
operator*(CWtransf& xf, CNDCZvec& p)
{
   Wvec tmp(p[0], p[1], p[2]);
   tmp = xf * tmp;
   return NDCZvec(tmp[0], tmp[1], tmp[2]);
}
*/
inline
Wtransf get_obj_to_ndc(Patch* patch_, LMESHptr mesh_)
{
  return (patch_)
         ? patch_->mesh()->obj_to_ndc()
         : (mesh_)
         ? mesh_->obj_to_ndc()
         : VIEW::peek_cam()->ndc_projection() * Identity;
}

static bool DEBUG_STROKES       = Config::get_var_bool("DEBUG_STROKES",false,true);
static bool DEBUG_ZX            = Config::get_var_bool("DEBUG_ZX_STROKES",false,true);
static bool debug_lubo = Config::get_var_bool("DEBUG_LUBO",false,true);
uint LuboPath::_lubosample_id = 0;
uint LuboPath::_lubostroke_id = 0;
uint ZXedgeStrokeTexture::_next_id       = 0;
uint ZXedgeStrokeTexture::_next_id_stamp = 0;

bool ZXedgeStrokeTexture::_use_new_idref_method = ( Config::get_var_bool("NEW_IDREF_METHOD",true,true) || Config::get_var_bool("ZX_NEW_BRANCH",true,true) );

ZXedgeStrokeTexture::ZXedgeStrokeTexture(Patch* patch) :
      OGLTexture(patch),
	  _stroke_3d(NULL), // added by alexni, bug fix
      _polyline(NULL),
      _pix_to_ndc_scale(0),
      _vis_sampling(2),
      _stroke_sampling(6),
      _screen_to_ref(0),
      _draw_creases(0),
      _seethru(0),
      _new_branch(0),
      _cam_change_stamp(0),
      _last_xform_id(0),
      _dirty(true),
      _need_update(false),
      _stamp(0),
      _strokes_drawn_stamp(0),
      _paths_created_stamp(0),
      _groups_created_stamp(0),
      _crease_max_bend_angle(180),
      _mesh(NULL)
{
   _prototype.set_flare((float)1.0);
   _prototype.set_taper(100);
   _prototype.set_alpha(1);
   _prototype.set_width(5.5);


   //by default, in seethru mode, only draw the frontfacing visible sils
   _render_flags[0] = true;
   for ( int i=1 ; i < ZXFLAG_NUM; i++ )
      _render_flags[i]=false;
   _offsets = NULL;

   _vis_sampling = Config::get_var_dbl("SIL_VIS_SAMPLE_SPACING",2.0,true);
}

ZXedgeStrokeTexture::ZXedgeStrokeTexture(CLMESHptr& mesh) :
      OGLTexture(NULL),
	  _stroke_3d(NULL), // added by alexni, bug fix
      _polyline(NULL),
      _pix_to_ndc_scale(0),
      _vis_sampling(2),
      _stroke_sampling(6),
      _screen_to_ref(0),
      _draw_creases(0),
      _seethru(0),
      _new_branch(0),
      _cam_change_stamp(0),
      _last_xform_id(0),
      _dirty(true),
      _need_update(false),
      _stamp(0),
      _strokes_drawn_stamp(0),
      _paths_created_stamp(0),
      _groups_created_stamp(0),
      _crease_max_bend_angle(180),
      _mesh(mesh)
{
   _prototype.set_flare((float)1.0);
   _prototype.set_taper(100);
   _prototype.set_alpha(1);
   _prototype.set_width(5.5);


   //by default, in seethru mode, only draw the frontfacing visible sils
   _render_flags[0] = true;
   for ( int i=1 ; i < ZXFLAG_NUM; i++ )
      _render_flags[i]=false;
   _offsets = NULL;

   _vis_sampling = Config::get_var_dbl("SIL_VIS_SAMPLE_SPACING",2.0,true);
}

ZXedgeStrokeTexture::~ZXedgeStrokeTexture()
{
   // We cached 'em. Now we trash 'em.

   _bstrokes.delete_all();
}

bool
ZXedgeStrokeTexture::strokes_need_update()
{
   return true;

   // XXX - needs work

   /*
   assert(_patch);

    if (_stamp == VIEW::stamp())  // we already checked in this frame
    return _need_update;

     _need_update = false;

      if (_dirty ||
      _cam_change_stamp == 0 ||
      _last_xform_id == 0 ||
      (VIEW::peek()->cam()->data()->stamp() > _cam_change_stamp) ||
      (_patch->mesh()->geom() &&
      _patch->mesh()->geom()->data_xform().id().val() > _last_xform_id)){
      _need_update = true;
      _cam_change_stamp = VIEW::peek()->cam()->data()->stamp();
      if (_patch->mesh()->geom())
      _last_xform_id = _patch->mesh()->geom()->data_xform().id().val();
      _stamp = VIEW::stamp();
      }
      return _need_update;
   */
}

void
ZXedgeStrokeTexture::cache_per_frame_vals(CVIEWptr &v)
{
   // Cache per-frame computations: 
   IDRefImage::set_instance(IDRefImage::lookup(v));
   _id_ref = IDRefImage::instance();
   _screen_to_ref = _id_ref->width()/double(v->width());
   _pix_to_ndc_scale = VIEW::pix_to_ndc_scale();
}

void
ZXedgeStrokeTexture::rebuild_if_needed(CVIEWptr &v)
{

   if (debug_lubo ||
      (_strokes_drawn_stamp != VIEW::stamp() && strokes_need_update())) {
      // Create the paths with votes
      create_paths();
      // Build strokes from the processed ndc array:
      ndcz_to_strokes();
      // We did it this frame:
      _strokes_drawn_stamp = VIEW::stamp();
      // May be needed if strokes_need_update() ever gets fixed:
      _dirty = false;
   }
}

void
ZXedgeStrokeTexture::create_paths()
{
   // This is called by SilAndCreaseTexture,
   // which skips ndcz_to_strokes().
   // That is why it's separated out from
   // rebuild_if_needed()

   // Cache per-frame computations:
  
   cache_per_frame_vals(VIEW::peek());      
   // Process ZCrossStrip into ndc array
   // (does visibility and sampling):
   sils_to_ndcz();

   // Build Parameterized Paths ( LuboPaths ) from the big ndc array
   _paths.delete_all();      

   if ( get_new_branch()) {
      add_paths_seethru();
   } else {
      add_paths();
   }
   // Compute new parameterizations from the old:
   propagate_sil_parameterization();

   regen_paths_notify();

}

void
ZXedgeStrokeTexture::init_next_id()
{
   if (_next_id_stamp == VIEW::stamp())
      return;
   _next_id_stamp = VIEW::stamp();

   if (debug_lubo) {
      // to see the paths in blue in the id ref
      // (hopefully doing this will not itself cause the bug)
      _next_id = 0x660000;
   } else {
      _next_id = 0x0000a000;
   }
}

void
ZXedgeStrokeTexture::setIDcolor(int path_id)
{
   // Convert path_id to the value written to the framebuffer:
   glColor4ubv((GLubyte*)&path_id);
}

void
ZXedgeStrokeTexture::setIDcolor_param(uint path_id)
{
   // Convert path_id to the value written to the framebuffer:
   glColor4ubv((GLubyte*)&path_id);
}

double
intersect_with_frustum ( NDCZpt& inner, NDCZpt& outer, NDCZpt& ret )
{
   NDCZpt frustum = NDCZpt(XYpt(1,1));
   //we know we have an intersection
   double dx = outer[0] - inner[0];
   double dy = outer[1] - inner[1];

   double len = (outer-inner).planar_length();
   if ( len == 0 ) { ret = inner; return 0 ;}

   //upper bound
   if ( dy > 0 )  {  //try upper bound
      double upper_xsect = inner[0]+dx*( frustum[1]-inner[1])/dy;
      if ( upper_xsect < frustum[0] && upper_xsect > -frustum[0] ) {
         ret = NDCZpt( upper_xsect,  frustum[1], 0);
         return (ret-inner).planar_length() / len;
      }
   }                 //try lower bound
   if ( dy < 0 )  {
      double lower_xsect = inner[0]+dx*(-frustum[1]-inner[1])/dy;
      if ( lower_xsect < frustum[0] && lower_xsect > -frustum[0] ) {
         ret = NDCZpt( lower_xsect, -frustum[1],0);
         return (ret-inner).planar_length() / len;
      }
   }
   if ( dx > 0 )  {
      double left_ysect  = inner[1]+dy*( frustum[0]-inner[0])/dx;
      if ( left_ysect < frustum[1] && left_ysect > -frustum[1] ) {
         ret = NDCZpt( frustum[0], left_ysect,  0);
         return (ret-inner).planar_length() / len;
      }
   }
   if ( dx < 0 ) {
      double right_ysect = inner[1]+dy*(-frustum[0]-inner[0])/dx;
      if ( right_ysect < frustum[1] && right_ysect > -frustum[1] ) {
         ret = NDCZpt(-frustum[0], right_ysect, 0);
         return (ret-inner).planar_length() / len;
      }
   }

   ret = inner;

   return 0;
}


void
ZXedgeStrokeTexture::sil_path_preprocess_seethru()
{
//    static bool DOUG = Config::get_var_bool("DOUG",false,false);

   //process all paths to be in frustum, get partial lengths
   //before we go to draw the ID ref image
   //this allows us to do a more accurate lookup for our

   //XXX this happens ( conditionally ) in sils_split_on_gradient



   _pre_zx_segs.clear(); //instead of modifying the sils from the patch
   //we build our own list that we add to

   if ( type_is_enabled(STYPE_SIL) ||
        type_is_enabled(STYPE_BF_SIL) )
      sils_split_on_gradient();

   if (type_is_enabled(STYPE_CREASE) )
      add_creases_to_sils();
   if ( type_is_enabled(STYPE_BORDER) )
      add_borders_to_sils();
   if (type_is_enabled(STYPE_WPATH) )
      add_wpath_to_sils();
   if (type_is_enabled(STYPE_POLYLINE) )
      add_polyline_to_sils();  
   //pre zx are finished.
   err_adv(false, "sil_path_preprocess_seethru: %d _pre_zx_segs", _pre_zx_segs.num());   
      
   //process them into ref segs.
   _ref_segs.clear();           

   int i;
   int loop_begin=0;
   int loop_end=0;
   int ref_start=0;
   int ref_end=0;
   int sil_num = _pre_zx_segs.num();   
  
   //const Wtransf& ndc_xform =      (_patch) ? _patch->mesh()->obj_to_ndc() : (_mesh) ? _mesh->obj_to_ndc() : Identity;
   const Wtransf& ndc_xform =  get_obj_to_ndc(_patch, _mesh);     
   const Wtransf& w_to_obj_xform = (_patch) ? _patch->mesh()->inv_xform()  : (_mesh) ? _mesh->inv_xform() : Identity;
   int cur_type;

   
   while ( loop_begin < sil_num ) {
          
      // Find the end of the loop
      loop_end = loop_begin;
      cur_type = _pre_zx_segs[loop_begin].type();

      while ( loop_end+1 < sil_num && !_pre_zx_segs[++loop_end].end()) {          
           assert( cur_type == _pre_zx_segs[loop_end].type() ); //loop ends when .end == true
      }

      
      assert( cur_type == _pre_zx_segs[loop_end].type() );

      bool in_frustum, last_in_frustum;
      double partial_length   = 0;
      NDCZpt last_npt         = NDCZpt(_pre_zx_segs[loop_begin].p(), ndc_xform);
      last_in_frustum         = last_npt.in_frustum();

      ref_start = _ref_segs.num();  
      if ( last_in_frustum ) { //first point
         //don't add path_id's yet      
         
         assert ( cur_type == _pre_zx_segs[loop_begin].type() );
         _ref_segs += SilSeg (
                         last_npt, true, SIL_VISIBLE, 0,
                         _pre_zx_segs[loop_begin].v(),
                         _pre_zx_segs[loop_begin].s(),
                         _pre_zx_segs[loop_begin].p(),
                         partial_length
                      );
         _ref_segs.last().type() = _pre_zx_segs[loop_begin].type();
         assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );
      }
      
      for  ( i = loop_begin+1; i <= loop_end; i++ ) {
         //main loop.  either add the next point, or handle frustum intersection  
        
         NDCZpt npt        =  NDCZpt(_pre_zx_segs[i].p(), ndc_xform);
         in_frustum        =  npt.in_frustum();

         Bsimplex* bary_face =  ( i != loop_end ) ? _pre_zx_segs[i].s() : _pre_zx_segs[i-1].s();         
         bool make_edge      =  ( i != loop_end ) ? true  : false;

         if ( in_frustum && last_in_frustum ) {
            //both in frustum, so we're happy    
           
            partial_length   += ( npt-last_npt ).planar_length();

            assert ( cur_type == _pre_zx_segs[i].type() );
            _ref_segs += SilSeg (
                            npt, make_edge, SIL_VISIBLE, 0,
                            _pre_zx_segs[i].v(),
                            bary_face,
                            _pre_zx_segs[i].p(),
                            partial_length
                         );
            _ref_segs.last().type() = _pre_zx_segs[i].type();
            assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );

         } else if (in_frustum != last_in_frustum) {

            double w;
            NDCZpt frust_npt;
            if ( !last_in_frustum ) {
               //we are entering the frustum

               if ( ( w = intersect_with_frustum(npt, last_npt, frust_npt ) ) > 0 ) {
                  frust_npt[2]  = interp ( npt[2], last_npt[2], w ); //proper z val;
                  Wpt frust_wpt = w_to_obj_xform * /*TFMULTFIX*/ Wpt(frust_npt);

                  partial_length = 0;        //reset partial length here

                  assert ( cur_type == _pre_zx_segs[i-1].type() );
                  //add frustum cut point
                  _ref_segs += SilSeg (
                                  frust_npt, true, SIL_VISIBLE, 0,
                                  _pre_zx_segs[i-1].v(),
                                  _pre_zx_segs[i-1].s(),
                                  frust_wpt,
                                  partial_length
                               );
                  _ref_segs.last().type() = _pre_zx_segs[i-1].type();
                  assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );
                  partial_length += ( npt-frust_npt).planar_length();
               } else
                  partial_length=0;       //or here

               assert( cur_type == _pre_zx_segs[i].type() );

               //add this point
               _ref_segs += SilSeg (
                               npt, make_edge, SIL_VISIBLE, 0,
                               _pre_zx_segs[i].v(),
                               bary_face,
                               _pre_zx_segs[i].p(),
                               partial_length
                            );
               _ref_segs.last().type() = _pre_zx_segs[i].type();
               assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );
            } else {
               //we are exiting the frustum
               if ( ( w = intersect_with_frustum(last_npt, npt, frust_npt) ) > 0 ) {

                  frust_npt[2]  = interp (last_npt[2], npt[2], w );
                  Wpt frust_wpt  = w_to_obj_xform * /* TFMULTFIX */ Wpt(frust_npt);

                  assert ( cur_type == _pre_zx_segs[i-1].type() );

                  partial_length += (frust_npt-last_npt).planar_length();
                  _ref_segs += SilSeg (
                                  frust_npt, false, SIL_VISIBLE, 0,
                                  _pre_zx_segs[i].v(),
                                  _pre_zx_segs[i-1].s(),
                                  frust_wpt,
                                  partial_length
                               );
                  _ref_segs.last().type() = _pre_zx_segs[i-1].type();
                  assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );

               } else {                  
                  _ref_segs.last().e() = false;  //stop the loop here
               }
            }
         }

         last_in_frustum  = in_frustum;
         last_npt         = npt;
      }

      ref_end = _ref_segs.num();

      err_adv(false, "sil_path_preprocess_seethru: %d _ref_segs", _ref_segs.num());   
      

      // assert ( ( ref_end == ref_start) || (ref_end - ref_start > 1 ) );

      static bool hack_fix_loop_pos = Config::get_var_bool("HACK_FIX_LOOP_POS",false,true);
      static bool use_mintest       = Config::get_var_bool("HACK_USES_MINTEST",false,true);

      /*HACK*/
      if ( hack_fix_loop_pos ) {

         bool mintest=false;

         if ( (cur_type == STYPE_SIL) && ( ref_end > ref_start ) ) {

            assert ( ref_end - ref_start > 1 );

            if (  _ref_segs[ref_start].p() == _ref_segs[ref_end-1].p()  &&
                  _ref_segs[ref_start].v() == SIL_VISIBLE               &&
                  _ref_segs[ref_start].v() == _ref_segs[ref_end-1].v()  ) {

               //cerr << "passed test" << endl;
               int j = ref_end-1; //start at last seg ( null e );
               int k;
               while ( j>ref_start && _ref_segs[j-1].e() )
                  j--; //find a broken connection before this end

               if ( j == ref_start && use_mintest) { //if no break or frustum or other shite, break at the bottom ( ndcz) of the loop.
                  double tmp_min_y = DBL_MAX;
                  for ( k=ref_start; k < ref_end; k++ ) {
                     /*
                                          if ( _ref_segs[k].p()[1] < tmp_min_y ) 
                                          { 
                                             j = k;
                                             tmp_min_y = _ref_segs[k].p()[1];
                                             mintest=true;
                                          }
                     */
                     if ( _ref_segs[k].p()[0] < tmp_min_y ) {
                        j = k;
                        tmp_min_y = _ref_segs[k].p()[0];
                        mintest=true;
                     }

                  }
               }

               if ( j > ref_start && j != ref_end-1) {

                  _ref_segs.pop(); //the duplicate point at end isn't needed

                  if ( !(use_mintest&&mintest) )
                     _ref_segs[j-1].e() = 0; //terminate the end (if we stopped because of vis );

                  int block_size = _ref_segs.num() - j;
                  _ref_segs.insert(ref_start, block_size);
                  int seam = ref_start+block_size;
                  if ( block_size > 0 ) {

                     for ( k = seam-1; k >= ref_start ; k-- ) {
                        //insert these segments at the beginning of the section we just added
                        _ref_segs[k] = _ref_segs.pop();
                     }
                     ref_end = _ref_segs.num();
                     //correct the partial length array;

                     //the segment moved from the end is now the beginning
                     //so the item at ref_start should have length 0.  relative lengths are
                     //still correct, so decrement this segment accordingly
                     double length_dec = _ref_segs[ref_start].l();   //needs to start at zero now
                     for ( k = ref_start; k < seam ; k++ ) {
                        _ref_segs[k].l() -= length_dec;
                     }
                     //this seg starts at 0, so increment everything by length of inserted seg
                     //plus the planar length between 'em.
                     ref_end = _ref_segs.num();
                     double length_inc = _ref_segs[seam-1].l() + (_ref_segs[seam].p()-_ref_segs[seam-1].p()).planar_length();

                     for ( k = seam; k < ref_end && _ref_segs[k].e() ; k++ ) {
                        _ref_segs[k].l() += length_inc;
                        if ( k < ref_end-1 && !_ref_segs[k+1].e() )
                           _ref_segs[k+1].l() += length_inc;
                     }


                     if ( use_mintest && mintest ) {
                        //if you've done something because of a mintest
                        _ref_segs += _ref_segs[ref_start];
                        _ref_segs.last().e() = false; //pop on the first point and null the connector;
                        _ref_segs.last().l() = _ref_segs[_ref_segs.num()-2].l() + ( _ref_segs.last().p() - _ref_segs[_ref_segs.num()-2].p() ).planar_length();
                     }

                  }
               }
            }
         }
      }
      /*HACK*/


      loop_begin = loop_end+1;

   }

}

void
ZXedgeStrokeTexture::sil_path_preprocess()
{
//    static bool DOUG = Config::get_var_bool("DOUG",false,false);
   //process all paths to be in frustum, get partial lengths
   //before we go to draw the ID ref image
   //this allows us to do a more accurate lookup for our
   //ray casting step

   if ( get_new_branch() ) {
      sil_path_preprocess_seethru() ;
      return;
   }

   _path_ids.clear();

   //instead of modifying the sils from the patch
   //we build our own list that we add to

   //if there is no patch - things SHOULD alredy be loaded in _pre_zx_segs at this point
   if(_patch)
      _pre_zx_segs = _patch->cur_zx_sils().segs();

      
   if ( type_is_enabled(STYPE_CREASE) )
      add_creases_to_sils();
   if ( type_is_enabled(STYPE_BORDER) )
      add_borders_to_sils();


   const Wtransf& ndc_xform =     get_obj_to_ndc(_patch, _mesh);
   const Wtransf& w_to_obj_xform = (_patch) ? _patch->mesh()->inv_xform()  : (_mesh) ? _mesh->inv_xform() : Identity;

   //_pre_zx_segs should now be loaded with the proper data   

   int i;
   int loop_begin=0;
   int loop_end=0;
   int ref_start=0;
   int ref_end=0;
   int sil_num = _pre_zx_segs.num();
   int vis;
   int cur_type;

   _ref_segs.clear();


   while ( loop_begin < sil_num ) {
        
      loop_end = loop_begin;

      cur_type = _pre_zx_segs[loop_begin].type();

      while ( loop_end+1 < sil_num && !_pre_zx_segs[++loop_end].end()) {
            assert( cur_type == _pre_zx_segs[loop_end].type() ); //loop ends when .end == 1
      }     
      assert( cur_type == _pre_zx_segs[loop_end].type() );

      bool in_frustum, last_in_frustum;
      double partial_length   = 0;
      NDCZpt last_npt         = NDCZpt(_pre_zx_segs[loop_begin].p(), ndc_xform);
      last_in_frustum         = last_npt.in_frustum();


      vis = ( _pre_zx_segs[loop_begin].g() ) ? SIL_VISIBLE : SIL_BACKFACING;

      ref_start = _ref_segs.num();

      if ( last_in_frustum ) {
         //don't add path_id's yet
         _ref_segs += SilSeg (
                         last_npt, true, vis, 0,
                         _pre_zx_segs[loop_begin].v(),
                         _pre_zx_segs[loop_begin].s(),
                         _pre_zx_segs[loop_begin].p(),
                         partial_length
                      );
         _ref_segs.last().type() = _pre_zx_segs[loop_begin].type();
         assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );
      }

      for  ( i = loop_begin+1; i <= loop_end; i++ ) {
         //main loop.  either add the next point, or handle frustum intersection
         NDCZpt npt        =  NDCZpt(_pre_zx_segs[i].p(), ndc_xform);
         in_frustum        =  npt.in_frustum();

         vis = ( _pre_zx_segs[i].g() )? SIL_VISIBLE : SIL_BACKFACING;


         Bsimplex* bary_face =  ( i != loop_end   ) ? _pre_zx_segs[i].s() : _pre_zx_segs[i-1].s();         
         bool   make_edge =  ( i != loop_end   ) ? true            : false;

         if ( in_frustum && last_in_frustum ) {
            //both in frustum, so we're happy
            partial_length   += ( npt-last_npt ).planar_length();
            _ref_segs += SilSeg (
                            npt, make_edge, vis, 0,
                            _pre_zx_segs[i].v(),
                            bary_face,
                            _pre_zx_segs[i].p(),
                            partial_length
                         );
            _ref_segs.last().type() = _pre_zx_segs[i].type();
            assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );

         } else if (in_frustum != last_in_frustum) {
            double w;
            NDCZpt frust_npt;
            if ( !last_in_frustum ) {

               //we are entering the frustum
               if ( ( w = intersect_with_frustum(npt, last_npt, frust_npt ) ) > 0 ) {
                  frust_npt[2]  = interp ( npt[2], last_npt[2], w ); //proper z val;
                  Wpt frust_wpt = w_to_obj_xform * /* TFMULTFIX */ Wpt(frust_npt);

                  vis = ( _pre_zx_segs[i-1].g()  )? SIL_VISIBLE : SIL_BACKFACING;

                  partial_length = 0;        //reset partial length here

                  _ref_segs += SilSeg (
                                  frust_npt, true, vis, 0,
                                  _pre_zx_segs[i-1].v(),
                                  _pre_zx_segs[i-1].s(),
                                  frust_wpt,
                                  partial_length
                               );
                  _ref_segs.last().type() = _pre_zx_segs[i-1].type();
                  assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );

                  partial_length += ( npt-frust_npt).planar_length();
               } else
                  partial_length=0;       //or here


               vis = (  _pre_zx_segs[i].g() )? SIL_VISIBLE : SIL_BACKFACING;

               //add this point
               _ref_segs += SilSeg (
                               npt, make_edge, vis, 0,
                               _pre_zx_segs[i].v(),
                               bary_face,
                               _pre_zx_segs[i].p(),
                               partial_length
                            );
               _ref_segs.last().type() = _pre_zx_segs[i].type();
               assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );

            } else {
               //we are exiting the frustum
               if ( ( w = intersect_with_frustum(last_npt, npt, frust_npt) ) > 0 ) {

                  frust_npt[2]  = interp (last_npt[2], npt[2], w );
                  Wpt frust_wpt  = w_to_obj_xform * /* TFMULTFIX */ Wpt(frust_npt);

                  vis = (  _pre_zx_segs[i].g() && _ref_segs.last().v()==SIL_VISIBLE  )? SIL_VISIBLE : SIL_BACKFACING;

                  partial_length += (frust_npt-last_npt).planar_length();
                  _ref_segs += SilSeg (
                                  frust_npt, false, vis, 0,
                                  _pre_zx_segs[i].v(),
                                  _pre_zx_segs[i-1].s(),
                                  frust_wpt,
                                  partial_length
                               );
                  _ref_segs.last().type() = _pre_zx_segs[i-1].type();
                  assert ( _ref_segs.num() < 2 || !( _ref_segs[ _ref_segs.num()-2].e() && (_ref_segs[_ref_segs.num()-2].type() != _ref_segs.last().type() ) ) );
                  
               } else {
                  _ref_segs.last().e() = false;  //stop the loop here
               }
            }
         }

         last_in_frustum  = in_frustum;
         last_npt         = npt;
      }

      ref_end = _ref_segs.num();

      //we have finished one loop
      //did the end of the loop remain visible?

      // if these two points are equivalent ,we have a loop.  can we shift these segments
      // so that the two connect properly?  either shift this where the loop goes
      // out of frustum or where it becomes a backfacing seg


      if ( (cur_type == STYPE_SIL) && ( ref_end > ref_start ) ) {

         //printf("diff is %d \n", ref_end - ref_start);
         assert ( ref_end - ref_start > 1 );

         if (  _ref_segs[ref_start].p() == _ref_segs[ref_end-1].p()  &&
               _ref_segs[ref_start].v() == SIL_VISIBLE               &&
               _ref_segs[ref_start].v() == _ref_segs[ref_end-1].v()     ) {
            //cerr << "passed test" << endl;
            int j = ref_end-1; //start at last seg ( null e );
            int k;
            while ( j>ref_start && _ref_segs[j-1].e() && _ref_segs[j-1].v()==SIL_VISIBLE )
               j--; //find a broken connection before this end

            if ( j > ref_start ) {

               _ref_segs[j-1].e() = 0; //terminate the end (if we stopped because of vis );
               //fprintf( stderr, "shift start %d end %d -- found non-vis at %d\n", ref_start, ref_end, j );

               _ref_segs.pop(); //the duplicate point at end isn't needed
               int block_size = _ref_segs.num() - j;
               _ref_segs.insert(ref_start, block_size);
               int seam = ref_start+block_size;
               if ( block_size > 0 ) {
                  for ( k = seam-1; k >= ref_start ; k-- ) {
                     //insert these segments at the beginning of the section we just added
                     _ref_segs[k] = _ref_segs.pop();
                  }

                  //correct the partial length array;

                  //the segment moved from the end is now the beginning
                  //so the item at ref_start should have length 0.  relative lengths are
                  //still correct, so decrement this segment accordingly
                  double length_dec = _ref_segs[ref_start].l();   //needs to start at zero now
                  for ( k = ref_start; k < seam ; k++ ) {
                     _ref_segs[k].l() -= length_dec;
                  }
                  //this seg starts at 0, so increment everything by length of inserted seg
                  //plus the planar length between 'em.
                  ref_end = _ref_segs.num();
                  double length_inc = _ref_segs[seam-1].l() + (_ref_segs[seam].p()-_ref_segs[seam-1].p()).planar_length();
                  for ( k = seam; _ref_segs[k].e() ; k++ ) {
                     _ref_segs[k].l() += length_inc;
                     if ( k < ref_end -1 && !_ref_segs[k+1].e() )
                        _ref_segs[k+1].l() += length_inc;
                  }
               }
            }
         }
      }


      loop_begin = loop_end+1;

   }


}

int
ZXedgeStrokeTexture::draw_id_ref_pre1()
{  
   if ( get_new_branch() ) {
      if (get_seethru() ) {
         _path_ids.clear();
         _ffseg_lengths.clear();
         init_next_id();
         sil_path_preprocess();
      }
   }
   return 0;
}

int
ZXedgeStrokeTexture::draw_id_ref_pre2()
{

   if ( get_new_branch() ) {
      if ( get_seethru() ) {
         return draw_id_ref_param_invis_pass();
      }
   }
   return 0;
}

int
ZXedgeStrokeTexture::draw_id_ref_pre3()
{
   if ( get_new_branch() ) {
      if ( get_seethru() ) {
         return 0;
      }
   }
   return 0;
}

int
ZXedgeStrokeTexture::draw_id_ref_pre4()
{
   if ( get_new_branch() ) {
      if ( get_seethru() ) {
         return draw_id_ref_param_vis_pass();
      }
   }
   return 0;
}


int
ZXedgeStrokeTexture::draw_id_ref()
{
   if ( get_new_branch() ) {
      if ( !get_seethru() ) {        
         _path_ids.clear();
         _ffseg_lengths.clear();
         init_next_id();
         sil_path_preprocess();
         return draw_id_ref_param_vis_pass();
      }
      return 0;
   }

   int i;
   static float line_width = (float) Config::get_var_int("SIL_VIS_PATH_WIDTH", 3, true);

   int numpaths=0;
   //   const Wtransf& ndc_xform = _patch->mesh()->obj_to_ndc();

   // make sure next_id is current for this frame
   init_next_id();
   //branch off to new stuff!
   if ( _use_new_idref_method )  {
      sil_path_preprocess();
      return draw_id_ref_parameterized();
   }


   _path_ids.clear();

   if(_patch)
       _pre_zx_segs = _patch->cur_zx_sils().segs();


   // XXX - should be done in Patch

   if ( type_is_enabled(STYPE_CREASE))
      add_creases_to_sils();
   if ( type_is_enabled(STYPE_BORDER))
      add_borders_to_sils();


   if (_pre_zx_segs.empty())
      return 0;

   // XXX - hack to avoid assert below
   //if ( _pre_zx_segs.num() > 0 && _pre_zx_segs.last().f() != NULL)
   //   return 0;

   _path_ids.realloc     (_pre_zx_segs.num());


   glPushAttrib(
      GL_LINE_BIT       |       // line width
      GL_LIGHTING_BIT   |       // shade model
      GL_CURRENT_BIT    |       // current color
      GL_ENABLE_BIT             // lighting, culling, blending
   );

   glLineWidth(line_width);              // GL_LINE_BIT
   glShadeModel(GL_FLAT);       // GL_LIGHTING_BIT
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   glDisable(GL_BLEND);         // GL_ENABLE_BIT

   bool started = false;

   bool lvis = false;
   bool vis = false;

   int path_id = gen_id();


   setIDcolor(path_id);

   //if ( _pre_zx_segs.num() > 0 )
   //   assert(_pre_zx_segs[_pre_zx_segs.num()-1].f() == NULL);


   for ( i=0; i < _pre_zx_segs.num(); i++) {

      // record current path_id:

      _path_ids += path_id;

      vis = _pre_zx_segs[i].g();


      if ( vis ) {
         // We are in a visible ("front-facing") section of the curve.
         // Start new line strip if needed:

         if ( !started ) {
            glBegin(GL_LINE_STRIP);
            started = true;
            numpaths++; //number of paths started ( one vis segment ) ;
         }

         // Do the leading point of the segment:
         glVertex3dv(_pre_zx_segs[i].p().data()); //always plot a visible point
      }

      if ( started && _pre_zx_segs[i].end()) {
         // The path has ended - stop GL.
         glEnd();
         started=false;
      } else if ( started && i < _pre_zx_segs.num()-1 && !_pre_zx_segs[i+1].g()) {
         // The path has gone backfacing - stop GL line
         glEnd();
         started=false;
      }


      // Increment path_id when a last seg is found ( end of path )
      // setting color that NEXT point will draw in
      if ( _pre_zx_segs[i].end()) {
         // Next point is on a new path
         //if ( i > 0 )
         //   assert (_pre_zx_segs[i-1].f()); //no consecutive baddies
         path_id = gen_id();

         setIDcolor(path_id);
      } else if ( vis && i < _pre_zx_segs.num()-1 && !_pre_zx_segs[i+1].g()) {
         // Will become backfacing (gradient ) on next point.
         path_id = gen_id();

         setIDcolor(path_id);
      } else if ( !vis && i < _pre_zx_segs.num()-1 && _pre_zx_segs[i+1].g()) {
         // Will become front-facing ( gradient ) on next point.
         path_id = gen_id();

         setIDcolor(path_id);
      }

      lvis = vis;
   }
   //   cerr << "\n" << endl;
   glPopAttrib();
   //cerr << "idref: " << numpaths << endl;
   return (_patch) ? _patch->num_faces() : 0;
}

int
ZXedgeStrokeTexture::draw_id_ref_param_object_pass()
{
   //ONLY draw the meshes and other objects in the scene
   return 0;
}

int
ZXedgeStrokeTexture::draw_id_ref_param_invis_pass()
{


   //INVISIBLE PASS (pass2)
   //
   //Using a color that we've marked as invisible ( using the second bit of the alpha channel )
   //we're going to draw all silhouettes into the idref image
   //without checking or writing into the depth buffer
   //
   int i,j;
   static float line_width = (float) Config::get_var_int("SIL_VIS_PATH_WIDTH", 3, true);

   //now we're checking _ref_segs

   if (_ref_segs.empty())
      return 0;


   glPushAttrib(
      GL_LINE_BIT       |       // line width
      GL_POINT_BIT      |       // point size
      GL_LIGHTING_BIT   |       // shade model
      GL_CURRENT_BIT    |       // current color
      GL_ENABLE_BIT            // lighting, culling, blending
   );

   glLineWidth (line_width);     // GL_LINE_BIT
   glShadeModel(GL_SMOOTH);       // GL_LIGHTING_BIT  //we want smoove shading, to interpolate them distinces.
   glDisable   (GL_LIGHTING);      // GL_ENABLE_BIT
   glDisable   (GL_BLEND);         // GL_ENABLE_BIT


   glEnable(GL_DEPTH_TEST);  // GL_ENABLE_BIT
   //DISABLE DEPTH TESTING
   //glDisable(GL_DEPTH_TEST);  // GL_ENABLE_BIT

   bool started = false;

   int  index_start = 0;
   int  index_end   = 0;

   int  cur_vis = SIL_VISIBLE;
   int  vis_mode = SIL_VISIBLE;

   int  cur_type = STYPE_SIL;
   int  type_mode = STYPE_SIL;


   for ( i=0; i < _ref_segs.num(); i++) {

      cur_vis = _ref_segs[i].v();
      cur_type = _ref_segs[i].type();

      assert ( cur_vis==SIL_VISIBLE );

      // if any visibility mode is on ,we need to draw into the idref
      // in both passes to properly show the hiddenline subset
      // unless it's only visible

      if ( !started && type_draws_to_idref_invis(cur_type) ) {
         assert ( _ref_segs[i].e() );
         started = true;
         index_start = i;
         vis_mode = cur_vis;
         type_mode = cur_type;
      }


      if ( started ) {
         assert(( cur_vis == SIL_VISIBLE) && (cur_type == type_mode) );
      }

      if ( started && !_ref_segs[i].e() ) {
         index_end = i;

         //starts at zero for a continuous
         //in-frustum segment
         //we've now split on backfacing segments
         //so we have to normalize for each segment

         //our draw mode affects the boundaries we're drawing.
         //our visibility is checked per face

         int id = gen_id_invis();

         double ffseg_start   = _ref_segs[index_start].l();   //partial length at beginning of seg
         double ffseg_length  = _ref_segs[index_end].l() - ffseg_start;//planar length of segment

         glBegin(GL_LINE_STRIP);

         for ( j=index_start; j <= index_end; j++ ) {
            _ref_segs[j].pl() = (_ref_segs[j].l() - ffseg_start); //ndc length relative to this ffsegment

            uint len = (uint) ( 255.0 * _ref_segs[j].pl() / ffseg_length );
            assert ( len < 256 );

            uint col = id | len ;

            _ref_segs[j].id_invis() = col; // store an invisible id for this segment
            setIDcolor_param(col);
            glVertex3dv (_ref_segs[j].w().data() );

         }
         glEnd();

         _path_ids         += id;
         _ffseg_lengths    += ffseg_length;
         started = false;
      }

   }

   glPopAttrib();

   return (_patch) ? _patch->num_faces() : 0;   
}

int
ZXedgeStrokeTexture::draw_id_ref_param_vis_pass()
{

   //VISIBLE PASS (pass3)

   //enable depth checking, and draw the silhouettes into the IDREF image
   //with a flag set to be the VISIBLE id's.  where visible they will (partially)
   //overwrite the silhouettes drawn by the previous pass.

   // this is almost exactly like the original idref image pass, except that we
   // don't clear out our bookkeeping (cuz inivisible pass does it now )

   int i,j;
   static float line_width = (float) Config::get_var_int("SIL_VIS_PATH_WIDTH", 3, true);

   //now we're checking _ref_segs

   if (_ref_segs.empty())
      return 0;

   glPushAttrib(
      GL_LINE_BIT       |       // line width
      GL_POINT_BIT      |       // point size
      GL_LIGHTING_BIT   |       // shade model
      GL_CURRENT_BIT    |       // current color
      GL_ENABLE_BIT     |       // lighting, culling, blending
      GL_DEPTH_BUFFER_BIT    // depth testingn
   );

   glLineWidth(line_width);  // GL_LINE_BIT
   glShadeModel(GL_SMOOTH);      // GL_LIGHTING_BIT  //we want smoove shading, to interpolate them distinces.
   glDisable(GL_LIGHTING);   // GL_ENABLE_BIT
   glDisable(GL_BLEND);    // GL_ENABLE_BIT
   glEnable(GL_DEPTH_TEST);   // GL_ENABLE_BIT

   bool started = false;

   int  index_start = 0;
   int  index_end   = 0;

   int  cur_vis = SIL_VISIBLE;
   int  vis_mode = SIL_VISIBLE;

   int  cur_type = STYPE_SIL;
   int  type_mode = STYPE_SIL;


   for ( i=0; i < _ref_segs.num(); i++) {

      cur_vis = _ref_segs[i].v();
      cur_type = _ref_segs[i].type();

      assert ( cur_vis == SIL_VISIBLE );

      // We are in a visible ("front-facing") section of the curve.
      // start tracing out its extent
      // if any visibility mode is on ,we need to draw into the idref
      // in both passes to properly show the hiddenline subset

      // XXX we should never draw a "BACKFACING" sil during the visible pass
      // because it will interfere with the proper silhouette line
      // due to polygon offsets..


      if ( !started && type_draws_to_idref_vis(cur_type) ) {
         assert ( _ref_segs[i].e() );
         started = true;
         index_start = i;
         vis_mode = cur_vis;
         type_mode = cur_type;
      }

      if ( started ) {
         assert(( cur_vis == SIL_VISIBLE) && (cur_type == type_mode) );
      }

      if ( started && !_ref_segs[i].e() ) {
         index_end = i;

         //we've traversed a visible front facing segment
         //now draw it with correctly parameterized length

         //starts at zero for a continuous
         //in-frustum segment
         //we've now split on backfacing segments
         //so we have to normalize for each segment

         //our draw mode affects the boundaries we're drawing.
         //our visibility is checked per face

         int id = gen_id_vis();

         double ffseg_start   = _ref_segs[index_start].l();   //partial length at beginning of seg
         double ffseg_length  = _ref_segs[index_end].l() - ffseg_start;//planar length of segment

         glBegin(GL_LINE_STRIP);

         for ( j=index_start; j <= index_end; j++ ) {
            _ref_segs[j].pl() = (_ref_segs[j].l() - ffseg_start); //ndc length relative to this ffsegment

            uint len = (uint) ( 255.0 * _ref_segs[j].pl() / ffseg_length );
            assert ( len < 256 );

            uint col = id | len ;

            _ref_segs[j].id() = col; // store an invisible id for this segment
            setIDcolor_param(col);
            glVertex3dv (_ref_segs[j].w().data() );
         }
         glEnd();

         _path_ids         += id;
         _ffseg_lengths    += ffseg_length;
         started=false;
      }

   }
   glPopAttrib();


  return (_patch) ? _patch->num_faces() : 0;
  
}


int
ZXedgeStrokeTexture::draw_id_ref_parameterized()
{

   assert ( !get_new_branch() );
   int i,j;
   static float line_width = (float) Config::get_var_int("SIL_VIS_PATH_WIDTH", 3, true);

   //now we're checking _ref_segs

   if (_ref_segs.empty())
      return 0;


   glPushAttrib(
      GL_LINE_BIT       |       // line width
      GL_POINT_BIT      |       // point size
      GL_LIGHTING_BIT   |       // shade model
      GL_CURRENT_BIT    |       // current color
      GL_ENABLE_BIT     |        // lighting, culling, blending
      GL_DEPTH_BUFFER_BIT    // depth testing
   );

   glLineWidth(line_width);     // GL_LINE_BIT
   glShadeModel(GL_SMOOTH);       // GL_LIGHTING_BIT  //we want smoove shading, to interpolate them distinces.
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   glDisable(GL_BLEND);         // GL_ENABLE_BIT
   glEnable(GL_DEPTH_TEST);
   bool started = false;
   bool need_draw = false;
   int  index_start = 0;
   int  index_end   = 0;
   //   bool lvis = false;
   bool vis = false;

   //these stay parallel, as a means to remember the length of each path
   //drawn with the id's listed in _path_ids...
   _path_ids.clear();
   _ffseg_lengths.clear();


   for ( i=0; i < _ref_segs.num(); i++) {

      vis = (_ref_segs[i].v() == SIL_VISIBLE );
      //cerr << "vis at " << i << "\tis " << vis << "\tplength " << _ref_segs[i].l();
      //cerr << endl;

      if ( vis ) {
         // We are in a visible ("front-facing") section of the curve.
         // start tracing out its extent
         if ( !started ) {
            started = true;
            need_draw = false;
            index_start = i;
            //cerr << "start at: " << i << endl;
         }

         index_end = i;
      } else { assert (_ref_segs[i].v() == SIL_BACKFACING); }

      if ( started && !_ref_segs[i].e() ) {
         //path ends ( either actual path finishes, or goes out of frustum
         need_draw= true;
         started=false;
      } else if ( started && i+1 < _ref_segs.num() && _ref_segs[i+1].v() != SIL_VISIBLE) {
         assert (_ref_segs[i+1].v() == SIL_BACKFACING);
         // front/backfacing test happens for each segment with reference to a face
         // so it applies to this point and the one that follows
         // we should draw this next point
         //path turns backfacing after next point, so stop here
         index_end = i+1;
         need_draw = true;
         started=false;
      }

      if ( need_draw ) {

         //we've traversed a visible front facing segment
         //now draw it with correctly parameterized length

         //starts at zero for a continuous
         //in-frustum segment
         //we've now split on backfacing segments
         //so we have to normalize for each segment

         int id = gen_id();

         double ffseg_start   = _ref_segs[index_start].l();   //partial length at beginning of seg
         double ffseg_length  = _ref_segs[index_end].l() - ffseg_start;//planar length of segment
         //glPointSize(8);
         //glColor3f ( 0, 1 ,0 );
         //glBegin(GL_POINTS);
         //glVertex3dv( _ref_segs[index_start].w().data() );
         //glEnd();

         glBegin(GL_LINE_STRIP);
         //cerr << "start strip" << endl;
         for ( j=index_start; j <= index_end; j++ ) {

            _ref_segs[j].pl() = ( (_ref_segs[j].l() - ffseg_start) ); //ndc length relative to this ffsegment
            uint len = (uint) ( 255.0 * _ref_segs[j].pl() / ffseg_length );
            uint col = id | len ;
            _ref_segs[j].id() = col;
            //fprintf ( stderr, "col %x\tlen: %f\ttotal_len: %f\tnum: %d\n", col, _ref_segs[j].l(), ffseg_length, i-index_start);
            setIDcolor_param(col);
            glVertex3dv (_ref_segs[j].w().data() );
            //cerr << "len " << _ref_segs[j].l() << endl;

         }
         glEnd();
         //cerr << "end strip length " << ffseg_length << endl;

         //record the length of each segment drawn, indexed by its id
         _path_ids         += id;
         _ffseg_lengths    += ffseg_length;
         //fprintf ( stderr , "path %x\tffseglen %f\n", _path_ids.last(), _ffseg_lengths.last() );
         need_draw = false;

      }

   }

   for ( int k = 0; k < _path_ids.num(); k++ ) {
      //cerr << "path id " << _path_ids[k] << "\tlength" << _ffseg_lengths[k] << endl;
   }
   //   cerr << "\n" << endl;

   glPopAttrib();
   //cerr << "idref: " << numpaths << endl;
   return _patch ? _patch->num_faces() : 0;
}


//MODIFICATIONS TO THE LIST OF POLYLINES THAT ARE CONSIDERED AS "SILHOUETTES"

void
ZXedgeStrokeTexture::sils_split_on_gradient()
{

   //explicitly separate front-facing and backfacing portions of silhouettes;

   //iterators
   int i,j;
   int loop_start=0;
   int loop_end=0;
   int loop_break=0; //mark an inconsistency where the loop should break;
   int section_mark=0;
   int pre_segs_start, pre_segs_end;

   bool pass_front = type_is_enabled(STYPE_SIL);
   bool pass_back  = type_is_enabled(STYPE_BF_SIL);
   bool pass;

   pre_segs_start = _pre_zx_segs.num();

   ARRAY<ZXseg>& zx_segs = (_patch) ? _patch->cur_zx_sils().segs() : _pre_zx_segs;
   //ARRAY<ZXseg>& zx_segs = _patch->cur_zx_sils().segs();
        
   int sil_num = zx_segs.num();

   if ( sil_num == 0 )
      return ;

   while ( loop_start < sil_num ) {    

      // loop end is the index in the zx strip of the end of a
      // 'valid' (front-facing) piece of silhouette.

     
      loop_end = loop_start; 

      while ( loop_end+1 < sil_num && !zx_segs[++loop_end].end())
         ; // loop ends when _pre_zx_segs[bla].end is true
            
      bool start_grad = zx_segs[loop_start].g();
      for (i = loop_start ; i < loop_end && zx_segs[i].g() == start_grad; i++ )
         ;

      loop_break = i;
      if ( loop_break == loop_end ) {
         //  loop (or loop segment) has consistent gradient - send them on in..
         int type = (start_grad) ? STYPE_SIL : STYPE_BF_SIL;
         for ( i = loop_start; i < loop_end; i++ ) {
            _pre_zx_segs += zx_segs[i];
            _pre_zx_segs.last().settype(type);
            assert ( _pre_zx_segs.num() < 2 || !( _pre_zx_segs[ _pre_zx_segs.num()-2].s() && (_pre_zx_segs[_pre_zx_segs.num()-2].g() != _pre_zx_segs.last().g() ) ) );
         }
         _pre_zx_segs += zx_segs[i];
         _pre_zx_segs.last().settype(type);
         _pre_zx_segs.last().setg(start_grad);

         assert ( _pre_zx_segs.num() < 2 || !( _pre_zx_segs[ _pre_zx_segs.num()-2].s() && (_pre_zx_segs[_pre_zx_segs.num()-2].g() != _pre_zx_segs.last().g() ) ) );

      } else {
         i = loop_break;
         while ( i < loop_end ) {
            section_mark = i;
            bool mark_grad = zx_segs[section_mark].g();
            int mark_type = (mark_grad) ? STYPE_SIL : STYPE_BF_SIL;
            for ( i = section_mark; i < loop_end && zx_segs[i].g() == mark_grad; i++ ) {
               _pre_zx_segs += zx_segs[i];
               _pre_zx_segs.last().settype(mark_type);
               assert ( _pre_zx_segs.num() < 2 || !( _pre_zx_segs[ _pre_zx_segs.num()-2].s() && (_pre_zx_segs[_pre_zx_segs.num()-2].g() != _pre_zx_segs.last().g() ) ) );
            }

            _pre_zx_segs += zx_segs[i];
            _pre_zx_segs.last().settype(mark_type);
            _pre_zx_segs.last().setg(mark_grad);
            _pre_zx_segs.last().set_bary(zx_segs[i-1].s());
            _pre_zx_segs.last().setf(NULL);
            _pre_zx_segs.last().set_end();
            
            assert ( _pre_zx_segs.num() < 2 || !( _pre_zx_segs[ _pre_zx_segs.num()-2].s() && (_pre_zx_segs[_pre_zx_segs.num()-2].g() != _pre_zx_segs.last().g() ) ) );

         }

         //add that nasty first bit

         if (  zx_segs[loop_start].p() == zx_segs[loop_end].p()  &&
               zx_segs[loop_start].g() == _pre_zx_segs.last().g() ) {
            _pre_zx_segs.pop();
         } //pop the end if it's a loop to reconnect

         int mark_type = (zx_segs[loop_start].g()) ? STYPE_SIL : STYPE_BF_SIL;
         for ( i = loop_start; i < loop_break; i++ ) {
            _pre_zx_segs += zx_segs[i];
            _pre_zx_segs.last().settype(mark_type);
            assert ( _pre_zx_segs.num() < 2 || !( _pre_zx_segs[ _pre_zx_segs.num()-2].s() && (_pre_zx_segs[_pre_zx_segs.num()-2].g() != _pre_zx_segs.last().g() ) ) );

         }
         _pre_zx_segs += zx_segs[i];
         _pre_zx_segs.last().setg(zx_segs[loop_start].g());
         _pre_zx_segs.last().set_bary(zx_segs[i-1].s());
         _pre_zx_segs.last().settype(mark_type);
         _pre_zx_segs.last().setf(NULL);
         _pre_zx_segs.last().set_end();
                  
         assert ( _pre_zx_segs.num() < 2 || !( _pre_zx_segs[ _pre_zx_segs.num()-2].s() && (_pre_zx_segs[_pre_zx_segs.num()-2].g() != _pre_zx_segs.last().g() ) ) );

      }

      loop_start = loop_end+1;

   }

   pre_segs_end = _pre_zx_segs.num();


   //POSTFILTER  -don't consider points that we won't use;
   // this should have already happened in the above code, but

   // between pre_segs_end and pre_segs_begin we have only added the sils
   // don't alter any earlier portion of the array
   if ( pass_front && pass_back )
      return;  // we want both, don't filter.

   j = pre_segs_start;
   for ( i=pre_segs_start; i < pre_segs_end; i++ ) {
      pass = ( _pre_zx_segs[i].type() == STYPE_SIL ) ? pass_front : pass_back ;
      if ( pass ) {
         _pre_zx_segs[j] = _pre_zx_segs[i];
         j++;
      }
   }
   if ( j != pre_segs_end )
      _pre_zx_segs.truncate(j);

}

//Convenience!!
inline double
my_angle(CWpt& a, CWpt& b, CWpt& c)
{
   // treating a, b, c as an ordered sequence of points,
   // return the exterior angle formed at b.
   // the answer is between 0 and pi radians.
   // (i.e. if they are colinear the angle is 0 radians):

   // XXX - should be in mlib

   return (c-b).angle(b-a);
}

inline double
my_angle(CBvert* a, CBvert* b, CBvert* c)
{
   // convenience: get the exterior angle formed by the
   // 3 vertices in the given order

   // XXX - should be in mesh/bvert.H (eventually)
   if (!(a && b && c))
      return 0;
   return my_angle(a->loc(), b->loc(), c->loc());
}


void
ZXedgeStrokeTexture::add_to_sils(CEdgeStrip& strip, int type, double angle_thresh)
{
   //**NOTE**: If angle_thresh (in radians) is > 0, then it's used
   //used as a max thresh for breaking these strips. The angle
   //lies [0,pi]

   // Add given edge strip to the list of valid silhouettes
   // ( pre-visibility )

   // XXX -
   //   Assumes this method is called just once for each
   //   time the zx sil strips are built on the patch.
   //   Otherwise the same edges can be added repeatedly.

   if (strip.empty())
      return;

   //assert (_patch);

   for (int i=0; i<strip.num(); i++) {

      // Get current edge, a face adjacent to it(unless there is no face), and current vert:
      Bedge* e = strip.edge(i);
      Bsimplex* s = (e->get_face()) ? (Bsimplex*)e->get_face() : (Bsimplex*)e;      
      Bvert* v = strip.vert(i);

      _pre_zx_segs += ZXseg( s, v->loc(), v, true, s , type, false);

      // Check if the line strip breaks at the next vert
      // Either due to end of strip, or bending angle

      if ( strip.has_break(i+1) || ((angle_thresh>=0) &&
            my_angle(v, strip.vert(i+1), strip.next_vert(i+1)) > angle_thresh)) {
         v = strip.next_vert(i);
             _pre_zx_segs += ZXseg( NULL, v->loc(), v, true, s , type, true);
      }    
   }

}

void
ZXedgeStrokeTexture::add_creases_to_sils()
{
   // Add crease edges to the list of valid silhouettes

   // Work with current strip (in case of subdivision):
   CEdgeStrip* creases = _patch->cur_creases();

   if (!creases)
      return;

   // Don't accept any creases with less than 2 adjacent faces:
   // XXX - really should check individually that each
   //       crease edge has at least one adjacent face.
   double thresh = (_crease_max_bend_angle<180)?(deg2rad(_crease_max_bend_angle)):(-1.0);
   if (creases->edges().nfaces_is_equal(2))
      add_to_sils(*creases, STYPE_CREASE, thresh);
}

void
ZXedgeStrokeTexture::add_borders_to_sils()
{
   // Add border edges to the list of valid silhouettes.
   // Border edges have just 1 adjacent face.

   // Work with current strip (in case of subdivision):
   // XXX - ctrl_patch issues?!
   CEdgeStrip* borders = _patch->cur_borders();
   if (borders)
      add_to_sils(*borders, STYPE_BORDER);
}

void
ZXedgeStrokeTexture::add_wpath_to_sils()
{ 
   if(!_stroke_3d)
      return;
   err_adv(false, "set_stroke_3d: adding %f edges ", _stroke_3d->num());
   add_to_sils((CEdgeStrip&)*_stroke_3d, 0, -1.0);   //STYPE_SIL

}

void
ZXedgeStrokeTexture::add_polyline_to_sils()
{   
   if(!_polyline)
      return;
      
   //err_adv(false, "ZXEDGE::add_polyline_to_sils: %d polilines ", _polyline->num());
 
   for (int i=0; i < _polyline->num(); i++){  
    if(i < _polyline->num()-1)
       _pre_zx_segs += ZXseg(NULL, (*_polyline)[i], NULL, true, NULL , 0, false);
    else
       _pre_zx_segs += ZXseg(NULL, (*_polyline)[i], NULL, true, NULL , 0, true);
       //end of the strip
   }

 

}
         
void
ZXedgeStrokeTexture::sils_to_ndcz()
{
   // translate between silhouette points and screen points
   // silhouette paths are complete, include invisible &
   // off-screen points --philipd

   // I.e., process the ZCrossPath into appropriately-sampled
   // screen-space points with known visibility, right? --lem

   // Can't proceed if _path_ids array is empty.
   // This can happen if draw_id_ref() was never called,
   // but now some wiseguy is calling sils_to_ndcz().

   // note: this now checks for the first p_id != -1,
   // since id's are set to -1 by default

   
   if ( _use_new_idref_method ) { resample_ndcz(); return;}

   if (_path_ids.empty()) {
      static bool warned = false;
      if (!warned) {
         cerr << "ZXedgeStrokeTexture::sils_to_ndcz: "
         << "can't proceed... try unsetting SHOW_LINE_STROKES"
         << endl;
         warned = true;
      }
      return;
   }

   // Start fresh:
   _sil_segs.clear();


   int zn = _pre_zx_segs.num();
   if (zn == 0)
      return;

   int lb = 0;  // index of loop begin
   int le = 0;  // index of loop end

   // Indices into _npoints array
   // for start and end of loop:
   int pstart, pend;

   // current and last in-frustum flags
   bool in_frustum, lin_frustum;
   int vis, lvis;

   int path_id=0;

   const Wtransf& ndc_xform = get_obj_to_ndc(_patch, _mesh);

   bool close_loop = false;    
   double sample_scale = _vis_sampling * _pix_to_ndc_scale;
   double dist_from_last_sample =0;
   while ( lb < zn ) {

      //
      // Find beginning and end of loop
      //

      // lb is the index in the zx strip of the beginning of a
      // 'valid' (front-facing) piece of silhouette.

      for ( ; lb < zn && _pre_zx_segs[lb].end(); lb++)
         ; // lb stops on a first valid seg
      //for ( ; lb < zn && !_pre_zx_segs[lb].f(); lb++)
      //   ; // lb stops on a non-NULL face

      // le is the index in the zx strip of the end of a
      // 'valid' (front-facing) piece of silhouette.
      le = lb;
      while ( le+1 < zn && !_pre_zx_segs[++le].end())
         ; // le stops on the last seg
      //while ( le+1 < zn && _pre_zx_segs[++le].f())
      //   ; // le stops on a NULL face

      //
      // Process loop into finely-sampled visible parts
      //

      // Get path id for this point on the loop:      
      path_id = _path_ids[lb];

      // record "last" point, i.e. last visited
      Wpt       wpt_last    = _pre_zx_segs[lb].p();
      NDCZpt    npt_last    = NDCZpt(wpt_last, ndc_xform);
      Bsimplex* bf_last     = _pre_zx_segs[lb].s();
      Wvec      bc_last     = _pre_zx_segs[lb].bc();
      int       last_i      = lb;
      // record whether point now being visited is in frustum
      lin_frustum = npt_last.in_frustum();
      lvis = check_vis( lb, npt_last, path_id ) ;

      // see if it wraps back unto itself, and that point is visible
      close_loop = ( _pre_zx_segs[lb].p() == _pre_zx_segs[le].p() && lvis==SIL_VISIBLE );

      // index in _npoints of the start of the path we'll build:
      pstart = _sil_segs.num();

      dist_from_last_sample = 0;
      // Iterate over zx strip from lb to le:
      for (int i = lb; i <= le; i++ ) {

         // Lookup the path id for this part of the loop:
         path_id = _path_ids[i];

         // record point currently being visited
         Wpt       wpt1 = _pre_zx_segs[i].p();
         NDCZpt    npt1 = NDCZpt( wpt1, ndc_xform );
         Bsimplex* bf1  = _pre_zx_segs[i].s();
         bool      last = _pre_zx_segs[i].end();
         Wvec      bc_1 = _pre_zx_segs[i].bc();
         // Have we gone far enough?


         double seg_len = (npt_last - npt1).planar_length();

         // dist_from_last_sample + seg_len > sample_scale
         //
         //
         double dl = seg_len / sample_scale;
         if (dl < 1 && i != le  && i != lb) {
            continue;
         }

         // Check visibility of the point
         in_frustum = npt1.in_frustum();
         vis = check_vis( i , npt1, path_id );
         bool tmp_in_frustum = in_frustum;
         int  tmp_vis = vis;

         // For long segments we have to sample them internally.
         // But don't do it when both points are offscreen:
         bool need_sub = (  vis==SIL_VISIBLE || lvis==SIL_VISIBLE );

         // Number of internal samples to generate:
         int segs = (int) floor (dl);

         if ( need_sub && segs > 1 ) {
            Wpt     sub_wlast;
            NDCZpt  sub_nlast;
            Wvec     bc_wpt1last;

            //  need to convert bc of edge point in cur face
            // to proper bc for last face
            //  XXX if we are subsampling, bflast is adjacent to bf_1

            // if bf_1 is last is 1 ( end of segments ) then bc is defined for bf_last
            if ( last )
               bc_wpt1last = bc_1;

            else {
               Bedge* e = (get_bface(bf1)) ? (get_bface(bf1))->shared_edge((Bface*)bf_last) : (Bedge*)bf1;
               if (e) {

                  Bvert* v1 = e->v1();
                  Bvert* v2 = e->v2();
                  
                  //bc_wpt1last[bf_last->vindex(v1)-1] = bc_1[bf1->vindex(v1)-1];
                  //bc_wpt1last[bf_last->vindex(v2)-1] = bc_1[bf1->vindex(v2)-1];
                  
                  int temp_index = (get_bface(bf_last)) ? (get_bface(bf_last))->vindex(v1) : ((Bedge*)bf_last)->vindex(v1);
                  int temp_index2 = (get_bface(bf1)) ? (get_bface(bf1))->vindex(v1) : ((Bedge*)bf1)->vindex(v1);
                  bc_wpt1last[temp_index-1] = bc_1[temp_index2-1];

                  temp_index = (get_bface(bf_last)) ? (get_bface(bf_last))->vindex(v2) : ((Bedge*)bf_last)->vindex(v2);
                  temp_index2 = (get_bface(bf1)) ? (get_bface(bf1))->vindex(v2) : ((Bedge*)bf1)->vindex(v2);
                  bc_wpt1last[temp_index-1] = bc_1[temp_index2-1];
               }
            }
            
            if ( last_i != i-1 ) {
               last_i      = i-1;
               wpt_last    = _pre_zx_segs[last_i].p();
               npt_last    = NDCZpt(wpt_last, ndc_xform);
               bf_last     = _pre_zx_segs[last_i].s();
               bc_last     = _pre_zx_segs[last_i].bc();
               // record whether point now being visited is in frustum
               lin_frustum = npt_last.in_frustum();
               lvis = check_vis( last_i, npt_last, path_id ) ;

            }

            for ( int j=1 ; j <segs ; j++ ) {
               double ax = j/(double)segs;


               // XXX -
               //  probably should take equal steps in NDC, not world
               //  does a linear interp in NDC correspond to a linear interp
               //  when projected back into worldspace?


               Wpt wpx = interp ( wpt_last, wpt1, ax);
               //        Wvec bc = interp ( bc_last, bc_wpt1last , ax);

               NDCZpt npx = NDCZpt ( wpx, ndc_xform );
               in_frustum = npx.in_frustum();
               vis = check_vis( i , npx , path_id );

               if (in_frustum) {

                  _sil_segs += SilSeg (npx, true, vis, path_id , _pre_zx_segs[i].v(), bf_last, wpx, 0.0);
                  _sil_segs.last().type() = _pre_zx_segs[i].type();

               } else if ( lin_frustum ) {

                  if (!_sil_segs.empty()) {
                     _sil_segs.last().e() = false;
                  }
               }

               sub_wlast = wpx;
               sub_nlast = npx;
               lin_frustum = in_frustum;
               lvis = vis;
            }
            wpt_last = sub_wlast;
            npt_last = sub_nlast;
         }

         //rescue visibility of point i;

         in_frustum = tmp_in_frustum;
         vis = tmp_vis;

         if ( in_frustum ) {
            //add point, edge
            _sil_segs += SilSeg (npt1, true, vis, path_id , _pre_zx_segs[i].v(), bf1, _pre_zx_segs[i].bc());
            _sil_segs.last().type() = _pre_zx_segs[i].type();

         } else if ( lin_frustum ) {  //not visible.  always close path
            if (!_sil_segs.empty()) {
               _sil_segs.last().e() = false;
            }
         }

         wpt_last = wpt1;
         npt_last = npt1;
         bf_last = bf1;
         bc_last = bc_1;
         lin_frustum = in_frustum;
         lvis = vis;
         last_i = i;
      }

      // The loop concluded, so we break the path here.
      if (!_sil_segs.empty()) {
         _sil_segs.last().e() = false;
      }



      pend = _sil_segs.num();

      // fix breaks, etc.


      /* stupid cleaner broke again! */
      if (_sil_segs.valid_index(pstart) &&
          _sil_segs.valid_index(pend-1))
         loop_clean ( pstart, pend, close_loop );
      // start the next one right after
      // the end of the current one:
      lb = le+1;
   }
}



void
ZXedgeStrokeTexture::loop_clean(int pstart, int pend, bool close_loop )
{  
   int i; 
   /*
   * SWINE ALERT - CODE ORANGE
   *   this appears to be fixed
   */ 
   if ( close_loop ) {
      // cerr << "close ? " ;
      int j,k;
      // close_loop flag means pstart is visible.
      // now find the first point where loop is invisible, or breaks

      if ( (_sil_segs[pstart].p() - _sil_segs[pend-1].p()).planar_length() > 2 )
         cerr << "bad loop close!" << endl;

      for ( j = pstart; j < _sil_segs.num() && _sil_segs[j].v()==SIL_VISIBLE; j++ )
         ; // j is first invisible

      for ( k = pstart; k < _sil_segs.num() && _sil_segs[k].e() ; k++ )
         ; // k is first line break

      if ( j != _sil_segs.num() && j != 0 && k != 0) { //if loop is fully visible, there are no edges

         //if loop is already invisible before break,
         //set _is_edge to FALSE at k
         //breaking the loop so that we can join it later.
         if ( j-1 < k ) {
            k=j-1;
            _sil_segs[k].e() = false;
         }

         //  cerr << "bad loop close!" << endl;
         //cerr << "closing loop\t" << pstart << "\t" << pend << endl;

         NDCSilPath     tmp_silpath;

         //read first path from pstart to k into temp array
         for ( i = pstart; i <= k; i++ ) {
            tmp_silpath += _sil_segs[i];
         }

         assert ( !tmp_silpath.last().e() ); //last edge should be false
         //truncate array
         chop_n ( pstart, 1+(k-pstart));

         //pop the last point of the loop
         //it is a copy of the first point in the array

         assert ( !_sil_segs.last().e() );



         _sil_segs.pop();

         //add first path seg to end

         _sil_segs += tmp_silpath;

         assert ( !_sil_segs.last().e() );

         if ( pstart != 0 ) {

            assert ( !_sil_segs[pstart-1].e() );
         }
         //prior to beginning of loop should be NULL

      }
   }


   for ( i=pstart; i < _sil_segs.num()-1; i++ ) {
      if ( !_sil_segs[i].e() ) {
         //connect successive loop segments if screen distance < 4 pix;

         if ((_sil_segs[i].p()-_sil_segs[i+1].p()).planar_length() < 4*_pix_to_ndc_scale)
            _sil_segs[i].e() = true;


      }
   }
}

void
ZXedgeStrokeTexture::chop_n(int start, int num)
{
   // remove num elements starting at index start

   assert ( start+num <= _sil_segs.num() );
   for ( int i = start; i+num < _sil_segs.num() ; i++) {

      _sil_segs[i] = _sil_segs[i+num];
   }

   _sil_segs.truncate ( _sil_segs.num()-num);
}

int
ZXedgeStrokeTexture::check_vis(int i, CNDCZpt& npt, int path_id)
{
   //    for interpolated points, that of endpoint
   // npt is screen space of that point
   // path_id is current loop

   // Reject points from outside the view frustum:
   if (!npt.in_frustum())
      return SIL_OUT_OF_FRUSTUM;

   // Reject "backfacing" sil segments:
   if (!_pre_zx_segs[i].g())
      return SIL_BACKFACING;

   // Return true if the value is found in a 3x3 search area:
   // ('1' is the "radius" of the 3x3 region)
   bool r = _id_ref->find_val_in_box(path_id, npt, 1);
   //if ( r ) _id_ref->val(npt) = path_id;   //debugging test
   return (r) ? SIL_VISIBLE : SIL_OCCLUDED;
}

void
ZXedgeStrokeTexture::resample_ndcz_seethru()
{

   //Traverse the list of NDC points generated by
   //the processing we did before drawing the IDref
   //resample as needed, perform visibility checks

   assert(_path_ids.num() == _ffseg_lengths.num());     
   
   double sample_scale = _vis_sampling * _pix_to_ndc_scale;
   int i, j, last;
   int loop_begin=0;
   int loop_end=0;
   int seg_start;
   int seg_end;


   int sections=0;

   int ref_num = _ref_segs.num();

  double dist_from_last_sample=0;

   _sil_segs.clear(); //clear out the new array

   Wtransf ndc_matrix = VIEW::peek_cam()->ndc_projection();

   const Wtransf& obj_to_w_xform =  (_patch) ? _patch->mesh()->xform()    : (_mesh) ? _mesh->xform() : Identity;
   const Wtransf& w_to_obj_xform =  (_patch) ? _patch->mesh()->inv_xform(): (_mesh) ? _mesh->inv_xform() : Identity;

   //cerr << "ref_num " << ref_num << endl;
   while ( loop_begin < ref_num ) {
      //reset loop boundaries at a valid stretch

      int cur_type = _ref_segs[loop_begin].type();
      assert ( _ref_segs[loop_begin].e() ) ;
      loop_end = loop_begin;
      while ( loop_end+1 < ref_num && _ref_segs[++loop_end].e() ) {
         // loop_end stops on on first break
         assert(cur_type == _ref_segs[loop_end].type());
      }        
      assert(cur_type == _ref_segs[loop_end].type());

      seg_start = _sil_segs.num();
      last= loop_begin;



      for ( i=loop_begin; i <= loop_end; i++ ) {

         dist_from_last_sample = (_ref_segs[i].p() - _ref_segs[last].p()).planar_length() ;
         sections = (int) floor (dist_from_last_sample/sample_scale );

         if ( sections > 0 || i==loop_begin || i==loop_end) {
            //always add the last point and first point!

            //check vis on this point
            check_vis_mask_seethru(_ref_segs[i]);

            if ( i != loop_begin && last != i-1 )
               check_vis_mask_seethru(_ref_segs[i-1]);

            //always subsample this segment, but only if they
            //share the same id.  otherwise just add the endpoints.

            if (  i != loop_begin ) {

               assert  ( (_ref_segs[i-1].id()       & 0xffffff00) == (_ref_segs[i].id()         & 0xffffff00 ) );
               assert  ( (_ref_segs[i-1].id_invis() & 0xffffff00) == (_ref_segs[i].id_invis()   & 0xffffff00 ) );

               Wpt   wi_1 = obj_to_w_xform * /* TFMULTFIX */ _ref_segs[i-1].w();
               Wpt   wi   = obj_to_w_xform * /* TFMULTFIX */ _ref_segs[i  ].w();

               double hi_1= ndc_matrix(3,0)*wi_1[0] + ndc_matrix(3,1)*wi_1[1] +
                            ndc_matrix(3,2)*wi_1[2] + ndc_matrix(3,3);

               double hi = ndc_matrix(3,0)*wi[0] + ndc_matrix(3,1)*wi[1] +
                           ndc_matrix(3,2)*wi[2] + ndc_matrix(3,3);

               for ( j=1; j < sections; j++ ) {

                  //if needed interpolate new points between point
                  //and the point immediately previous ( i-1, NOT last);

                  static bool old_crap = Config::get_var_bool("USE_OLD_LENGTH_ENCODING",false,true);

                  //////////////////////////////
                  double   w;
                  NDCZpt   npt;
                  Wpt      wpt;
                  double   len;
                  double   plen;
                  uint     new_id;
                  uint     new_id_i;
                  int      vis;
                  //////////////////////////////
                  if (old_crap) {
                     w     =  (double)j/(double)sections;
                     npt   = interp ( _ref_segs[i-1].p(),    _ref_segs[i].p(),    w );
                     wpt   = w_to_obj_xform * /* TFMULTFIX */ Wpt(npt);
                     len   = interp ( _ref_segs[i-1].l(),    _ref_segs[i].l(),    w );
                     plen  = interp ( _ref_segs[i-1].pl(),   _ref_segs[i].pl(),   w );

                     //                int id_diff   = _ref_segs[i].id() - _ref_segs[i-1].id();
                     //                uint    id    = _ref_segs[i-1].id() + (uint)(id_diff*w);
                     //      int id_invis_diff   = _ref_segs[i].id_invis() - _ref_segs[i-1].id_invis();
                     //                uint    id_invis    = _ref_segs[i-1].id_invis() + (uint)(id_invis_diff*w);

                     //XXX - use _ffseglens, not _ref_segs[loop_end].pl()!!!!!!!
                     new_id    = ( _ref_segs[i].id() & 0xffffff00 ) |
                                 ((uint) ( 255.0 * plen / _ref_segs[loop_end].pl() ));
                     //XXX - use _ffseglens, not _ref_segs[loop_end].pl()!!!!!!!
                     new_id_i  = ( _ref_segs[i].id_invis() & 0xffffff00 ) |
                                 ((uint) ( 255.0 * plen / _ref_segs[loop_end].pl() ));

                     vis   = SIL_VISIBLE;
                  }
                  //////////////////////////////
                  else {
                     w        = (double)j/(double)sections;
                     //XXX - Interpolating in world space! Not right...
                     wpt      = interp ( _ref_segs[i-1].w(),    _ref_segs[i].w(),       w );
                     len      = interp ( _ref_segs[i-1].l(),    _ref_segs[i].l(),       w );

                     Wpt wwpt = obj_to_w_xform * /* TFMULTFIX */ wpt;

                     npt      = NDCZpt(wwpt);

                     double   p_gap    = _ref_segs[i].pl() - _ref_segs[i-1].pl();
                     double   p_diff   = clamp( (npt-_ref_segs[i-1].p()).planar_length() , 0.0, p_gap );
                     double   p_frac   = p_diff/p_gap;

                     plen     = _ref_segs[i-1].pl() + p_diff;

                     //Account for perspective correction...
                     double   h        = ndc_matrix(3,0)*wwpt[0] + ndc_matrix(3,1)*wwpt[1] +
                                         ndc_matrix(3,2)*wwpt[2] + ndc_matrix(3,3);

                     new_id   = (uint)( h * ((1.0 - p_frac) / hi_1 * _ref_segs[i-1].id() +
                                             (p_frac) / hi   * _ref_segs[i  ].id())  );
                     new_id_i = (uint)( h * ((1.0 - p_frac) / hi_1 * _ref_segs[i-1].id_invis() +
                                             (p_frac) / hi   * _ref_segs[i  ].id_invis())  );

                     new_id            = clamp(new_id,   _ref_segs[i-1].id(),       _ref_segs[i].id());
                     new_id_i          = clamp(new_id_i, _ref_segs[i-1].id_invis(), _ref_segs[i].id_invis());

                     vis       = SIL_VISIBLE;
                  }
                  //////////////////////////////

                  SilSeg s= SilSeg(
                               npt, true, vis, new_id,
                               _ref_segs[i-1].bv(),
                               _ref_segs[i-1].s(),
                               wpt, len, plen
                            );
                  s.id_invis() = new_id_i;
                  s.type()   = _ref_segs[i].type();

                  check_vis_mask_seethru(s);
                  _sil_segs += s;
               }
            }

            //loop end can get in here without passing the distance test
            //but we need to know that it's not identical to the last point( in NDC );

            if ( i == loop_end && dist_from_last_sample <= gEpsZeroMath ) {
               _sil_segs.pop();           //pop from sil segs and add loop end instead
               //because its flags are proper

               if ( last != loop_begin )  //but only add loop end again if we have added more than one point
               {
                  _sil_segs += _ref_segs[loop_end];  //OUTER LOOP ENDS
               }
            } else  // just toss it to the segment list
            {
               _sil_segs += _ref_segs[i];
               last = i;
            }

         }
      }

      seg_end = _sil_segs.num();

      loop_begin = loop_end + 1;
   }
}


void
ZXedgeStrokeTexture::resample_ndcz()
{

   //Traverse the list of NDC points generated by
   //the processing we did before drawing the IDref
   //resample as needed, perform visibility checks

   /** dRAW them all**/


   /***  ***/
   // branch out for new stuff
   if ( get_new_branch() ) { resample_ndcz_seethru(); return; }
   /*** ***/

   double sample_scale = _vis_sampling * _pix_to_ndc_scale;
   int i, j, last;
   int loop_begin=0;
   int loop_end=0;
   int seg_start;
   int seg_end;

   int sections=0;

   int ref_num = _ref_segs.num();

   double dist_from_last_sample=0;


   _sil_segs.clear();

   while ( loop_begin < ref_num ) {
       //XXX- A hack April 11, 2006, getting this assertion, want to see if we can make things work without it :D
      //assert ( _ref_segs[loop_begin].e() );
      //for ( ; loop_begin < ref_num && !_ref_segs[loop_begin].e(); loop_begin++)
      //   ; // loop_begin stops on first connection
      loop_end = loop_begin;
      while ( loop_end+1 < ref_num && _ref_segs[++loop_end].e() )
         ; // loop_end stops on on first break

      seg_start = _sil_segs.num();

      last= loop_begin;

      for ( i=loop_begin; i <= loop_end; i++ ) {

         dist_from_last_sample = (_ref_segs[i].p() - _ref_segs[last].p()).planar_length() ;
         sections = (int) floor (dist_from_last_sample/sample_scale );


         if ( sections > 0 || i==loop_begin || i==loop_end) { //always add the last point and first point!

            //check vis on this point
            check_vis_mask(_ref_segs[i]);
            if ( i != loop_begin && last != i-1 )
               check_vis_mask(_ref_segs[i-1]);

            //subsample this segment, but only if at least one is visible and they
            //share the same id.  otherwise just add the endpoints
            if (  i != loop_begin &&
                  (_ref_segs[i-1].v() == SIL_VISIBLE || _ref_segs[i].v() == SIL_VISIBLE  ) &&
                  (_ref_segs[i-1].id() & 0xffffff00) == (_ref_segs[i].id() & 0xffffff00  )     ) {
               //_sil_segs += _ref_segs[i-1];
               for ( j=1; j < sections; j++ ) {
                  //if needed interpolate new points between point
                  //and the point immediately previous ( i-1, NOT last);
                  double  w     =  (double)j/(double)sections;
                  NDCZpt  npt   = interp ( _ref_segs[i-1].p(),    _ref_segs[i].p(),       w );
                  double  len   = interp ( _ref_segs[i-1].l(),    _ref_segs[i].l(),       w );
                  double  plen  = interp ( _ref_segs[i-1].pl(),   _ref_segs[i].pl(),      w );

                  int id_diff   = _ref_segs[i].id() - _ref_segs[i-1].id();
                  uint    id    = _ref_segs[i-1].id() + (uint)(id_diff*w);

                  int     vis   = SIL_VISIBLE;

                  SilSeg s= SilSeg(
                               npt, true, vis, id,
                               _ref_segs[i-1].bv(),
                               _ref_segs[i-1].s(),
                               Wpt(npt), len, plen
                            );
                  s.type() = _ref_segs[i-1].type(); //XXX new stuff for rob's mods
                  check_vis_mask(s);
                  _sil_segs += s;
               }
            }


            //loop end can get in here without passing the distance test
            //but we need to know that it's not identical to the last point( in NDC );

            if ( i == loop_end && dist_from_last_sample <= gEpsZeroMath ) {
               _sil_segs.pop();           //pop from sil segs and add loop end instead
               //because its flags are proper

               if ( last != loop_begin )  //but only add loop end again if we have added more than one point
               {
                  _sil_segs += _ref_segs[loop_end];  //OUTER LOOP ENDS
               }
            } else  // just toss it to the segment list
            {
               _sil_segs += _ref_segs[i];
               last = i;
            }

         }
      }

      seg_end = _sil_segs.num();


      //clean up loops after this pass as well
      /*
      if ( 2==3 && seg_end-seg_start > 0 && _sil_segs[seg_start].p() == _sil_segs[seg_end].p() && _sil_segs[seg_start].v() == SIL_VISIBLE) { 
         //we's in a LOOP!

         int j = seg_end-1; //start at last seg ( null e );
         int k;

         //find a point out of visibility;
         while ( j>seg_start && _sil_segs[j-1].v() == SIL_VISIBLE ) j--; 

         if ( j > seg_start ) { // if the loop isn't completely visible ( any segment occluded, backfacing, out of frustum )
            
            _sil_segs[j].e() = 0; //break connectivity at this point

      //            j; //start of this segment
            
            _sil_segs.pop(); //pop off identical point that closes the loop ( with its segment terminator)
            int block_size = _sil_segs.num() - j;
            _sil_segs.insert(seg_start, block_size);

            for ( k = seg_start+block_size-1; k >= seg_start ; k-- ) { 
               //insert these segments at the beginning of the section we just added
               _sil_segs[k] = _sil_segs.pop();         
            }
         }
      }
      */

      loop_begin = loop_end + 1;


   }



   //cerr << "resampled segments: " << _sil_segs.num() << endl;


}

int
ZXedgeStrokeTexture::check_vis_mask(SilSeg &s)
{

   //this is already in frustum

   //and we've done the backfacing test
   //if it's gotten this far , all we need is the
   //occlusion test
   static int VIS_ID_RAD = Config::get_var_int("VIS_ID_RAD",1, true);
   //fprintf(stderr, "OLDVIS %d ID %x\n" ,s.v(), s.id() );
   double pix_seg_len = _ffseg_lengths[_path_ids.get_index(s.id() & 0xffffff00)] * VIEW::peek()->ndc2pix_scale();
   int range = (int) ceil (2.0 * max ( 2.0, 256.0/pix_seg_len ) );
   if ( s.v() == SIL_VISIBLE ) {
      if ( _id_ref->find_val_in_box(s.id(), 0xffffff00, s.p(), VIS_ID_RAD, range) )
         s.v() = SIL_VISIBLE;
      else
         s.v() = SIL_OCCLUDED;
   }

   //fprintf(stderr, "NEWVIS %d\n" ,s.v());


   return s.v();

}

int
ZXedgeStrokeTexture::check_vis_mask_seethru(SilSeg &s)
{

   static uint mask = 0xffffff00;

   static int VIS_ID_RAD = Config::get_var_int("VIS_ID_RAD",2, true);
   static int INVIS_ID_RAD = Config::get_var_int("INVIS_ID_RAD",2, true);

   //this is already in frustum
   //okay, this is where we need to find the visibility on this segment

   //this depends on the seethru flags

   int nbr, idx=0;
   double fseg_len;
   double ndc2pix = VIEW::peek()->ndc2pix_scale();

   //default case;
   s.v() = SIL_OCCLUDED;

   //check first for visibility
   if ( is_vis_path_id(s.id()) ) {
      idx = _path_ids.get_index(s.id() & mask);
      assert(idx != BAD_IND);

      fseg_len = _ffseg_lengths[idx] * ndc2pix;

      nbr = (int) ceil (2.0 * max ( 2.0, 256.0/fseg_len ) );

      if  ( _id_ref->find_val_in_box (s.id(), mask, s.p(), VIS_ID_RAD, nbr) )
         s.v() = SIL_VISIBLE;
   }

   if ( s.v() != SIL_VISIBLE   &&  is_invis_path_id(s.id_invis()) ) {
      idx = _path_ids.get_index(s.id_invis() & mask);
      assert(idx != BAD_IND);

      fseg_len = _ffseg_lengths[idx] * ndc2pix;

      nbr = (int) ceil (2.0 * max ( 2.0, 256.0/fseg_len ) );

      if ( _id_ref->find_val_in_box  (s.id_invis(), mask, s.p(), INVIS_ID_RAD, nbr) )
         s.v() = SIL_HIDDEN;
   }

   return s.v();

}

void
ZXedgeStrokeTexture::set_stroke_3d(CEdgeStrip* stroke_3d)
{
   if (!stroke_3d)
      return;   
   _stroke_3d = stroke_3d;
 
}

// Makes Edgestrip out of wpts
void
ZXedgeStrokeTexture::set_polyline(CWpt_list& polyline)
{ 
   _polyline = &polyline;
   //cerr << "b/c " << _polyline << endl; 
}


void
ZXedgeStrokeTexture::set_prototype(CBaseStroke &s)
{
   _prototype.copy(s);

   // Must update the parameters of the strokes
   // using the fresh prototype
   _bstrokes.copy(s);
}

void
ZXedgeStrokeTexture::set_offsets(BaseStrokeOffsetLISTptr ol)
{
   _offsets = ol;

   // Must update the parameters of the strokes
   // using the fresh offsets
   _bstrokes.set_offsets(ol);
}


void
ZXedgeStrokeTexture::ndcz_to_strokes()
{
   // for the ndc vector, place contiguous visible points into
   // strokes

   // We re-use old strokes rather than delete and allocate every frame.

   // Reset strokes before re-building them:
   _bstrokes.clear_strokes();

   // Number of points visited for each one added to a stroke:
   int samp = sample_step();

   int bnum = 0;  // index to current base stroke
   NDCZpt last;   // last point added
   int cnt=0;     // count of points added

   int n = _sil_segs.num();



   bool started=false;
   for (int i=0 ; i<n ; i++) {

      if (_sil_segs[i].v()==SIL_VISIBLE ) {

         if (!started ) {
            if ( _sil_segs[i].e() ) {

               if ( bnum == _bstrokes.num() ) {
                  // No more old strokes to re-use;
                  // have to allocate a new one:
                  _bstrokes.add( _prototype.copy() );
                  _bstrokes[bnum]->set_offsets(_offsets);
               }
               assert ( bnum < _bstrokes.num() );
               // bstroke = _bstrokes.next();
               _bstrokes[bnum]->add
               ( _sil_segs[i].p() );
               cnt = 0;

               last = _sil_segs[i].p();

               started = true;
            }
         } else {
            // Add every samp points, or if it's the last point
            if ( cnt%samp == 0 || !_sil_segs[i].e() ) {
               if (  ( last - _sil_segs[i].p() ).planar_length() > 0 ) {
                  _bstrokes[bnum]->add
                  ( _sil_segs[i].p() );
                  last = _sil_segs[i].p();
               }
            }
         }
      }
      if ( started && !(_sil_segs[i].e() && _sil_segs[i].v()==SIL_VISIBLE) ) {
         // XXX - maybe add the previous point if it was visible
         //       but not added?
         started = false;
         bnum++;
      }
      cnt++;
   }
}

int
ZXedgeStrokeTexture::draw(CVIEWptr& v)
{    
   rebuild_if_needed(v);
   return _bstrokes.draw(v);
}




void
ZXedgeStrokeTexture::add_paths_seethru()
{
   /*
      for (int i=0; i<_sil_segs.num() && add_path_seethru(i); )
         ;
   */
   //cerr << "hello " << _sil_segs.num() << endl;

      
   for (int i=0; i<_sil_segs.num(); i++) {

      // find start of next path:
      while ( _sil_segs.valid_index(i) && !check_render_flags(_sil_segs[i].type(), _sil_segs[i].v() ) )
         i++;

      // if there is no next path we're done
      if (!_sil_segs.valid_index(i))
         break;

      LuboPath* p = new LuboPath;
      p->type() = _sil_segs[i].type();
      p->vis()  = _sil_segs[i].v();
      _paths += p;


      //the branch here should be noted - in the silsegs array, we kept track of both a visible and invisible id from the reference image
      //before we could read back from the ref-image to know visibility. now that the check has been done, we store
      //whichever id is appropriate, according to the visibility flags.
      //p.s.  we should be looking at the vis flags in the lubopath, but if all else fails, is_invis_path_id(x) and is_vis_path_id(x) can help you

      if ( p->vis() == SIL_VISIBLE ) {
         //if we're visible, we add the regular id ( as in the standard case )
         for ( ; _sil_segs.valid_index(i) && _sil_segs[i].e() && _sil_segs[i].v()==p->vis(); i++) {
            assert( _sil_segs[i].type()==p->type());
            p->add
            (_sil_segs[i].p(), true, _sil_segs[i].id() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
         }
         // add the last point
         if ( _sil_segs.valid_index(i) && _sil_segs[i].v()==p->vis() ) // we stopped because of the edge going bad
         {
            assert(_sil_segs[i].type()==p->type());
            p->add
            (_sil_segs[i].p(), true, _sil_segs[i].id() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
         }
         
      } else {
         //if we're invisible (therefore a hiddenline) we need to add the id from when it was drawn in hiddenline
         for ( ; _sil_segs.valid_index(i) && _sil_segs[i].e() && _sil_segs[i].v()==p->vis(); i++) {
            assert(_sil_segs[i].type()==p->type());
            p->add
            (_sil_segs[i].p(), true, _sil_segs[i].id_invis() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
         }
         // add the last point
         if (_sil_segs.valid_index(i) && _sil_segs[i].v()==p->vis() ) {
            assert(_sil_segs[i].type()==p->type());
            p->add
            (_sil_segs[i].p(), true, _sil_segs[i].id_invis() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
         }
      }

      // computes lengths:
      p->complete();

      if (p->num() <= 1) {
         _paths.pop();
      }

      assert(( _use_new_idref_method ) );
      if ( _use_new_idref_method ) {

         //retrieve the correct length
         //for the front-facing segment
         //drawn into ref image

         //id_set().num() == 1 all the time!
         for ( int j=0; j < p->id_set().num()  ; j++ ) {
            int ind = _path_ids.get_index(p->id_set(j) );
            assert ( _path_ids.valid_index( ind ) ) ;
            p->ffseg_lengths() += _ffseg_lengths[ind];

         }

         int n = p->num();

         if ( p->id_set().num() == 1 && p->ff_len(0) > p->ff_len(n-1)  ) {
            assert(0); // XXX - Not needed these days... For now...
            //cerr << "\tsingle wrap" << endl;
            //segment is located where loop closes
            int offset=n-1;
            while ( offset > 0 && p->ff_len(offset) > p->ff_len(offset-1) )
               offset--;

            p->id_offsets().add     (offset);
            p->id_set().add         (p->id_set(0));
            p->ffseg_lengths().add  (p->ffseg_length(0));

         }  //else , multiple ids found, but we end where we began
         else if ( p->id_set().num() > 1 && p->id_set().first() == ( p->id(n-1) & 0xffffff00 ) ) {
            assert(0); // XXX - Not needed these days... For now...
            //loop is multisegment
            //cerr << "\tmultisegment with wrap" << endl;
            uint first_id = p->id_set().first();
            int offset=n-1;
            while ( offset > 0 && ( p->id(offset-1) & 0xffffff00 ) == first_id )
               offset--;

            p->id_offsets().add        (offset);
            p->id_set().add            (p->id_set(0));
            p->ffseg_lengths().add     (p->ffseg_length(0));

         }
         //else if ( p->id_set().num() > 1 ) cerr << "\tnon-wrapping multi" << endl;
         //else cerr << "\tnon-wrapping single" << endl;
         //okay, so now if we have any sort of
         //loop trickery, we have a duplicate entry in the
         //id_set to indicate that.

         //there's an extra entry in the offsets array for the end of the stroke
         //so that now the path in
         //id_set[i] is of ndc length ffseg_lengths[i]
         //and extends from id_offsets[i] to id_offsets[i+1]-1;
         p->id_offsets().add(n);

      }

   }
}


bool
ZXedgeStrokeTexture::add_path_seethru(int &i)
{


   // find start of next path:
   while ( _sil_segs.valid_index(i) && !check_render_flags(_sil_segs[i].type(), _sil_segs[i].v() ) )
      i++;

   // if there is no next path we're done
   if (!_sil_segs.valid_index(i))
      return 0;

   LuboPath* p = new LuboPath;
   p->type() = _sil_segs[i].type();
   p->vis()  = _sil_segs[i].v();
   _paths += p;


   //the branch here should be noted - in the silsegs array, we kept track of both a visible and invisible id from the reference image
   //before we could read back from the ref-image to know visibility. now that the check has been done, we store
   //whichever id is appropriate, according to the visibility flags.
   //p.s.  we should be looking at the vis flags in the lubopath, but if all else fails, is_invis_path_id(x) and is_vis_path_id(x) can help you

   if ( p->vis() == SIL_VISIBLE ) {
      //if we're visible, we add the regular id ( as in the standard case )
      for ( ; _sil_segs.valid_index(i) && _sil_segs[i].e() && _sil_segs[i].v()==p->vis(); i++) {
         assert( _sil_segs[i].type()==p->type());
         p->add
         (_sil_segs[i].p(), true, _sil_segs[i].id() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
      }
      // add the last point
      if (_sil_segs.valid_index(i) && _sil_segs[i].v()==p->vis() ) {
         assert(_sil_segs[i].type()==p->type());

         p->add
         (_sil_segs[i].p(), true, _sil_segs[i].id() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
      }
   } else {
      //if we're invisible (therefore a hiddenline) we need to add the id from when it was drawn in hiddenline
      for ( ; _sil_segs.valid_index(i) && _sil_segs[i].e() && _sil_segs[i].v()==p->vis(); i++) {
         assert(_sil_segs[i].type()==p->type());
         p->add
         (_sil_segs[i].p(), true, _sil_segs[i].id_invis() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
      }
      // add the last point
      if (_sil_segs.valid_index(i) && _sil_segs[i].v()==p->vis() && _sil_segs[i].type()==p->type() ) {
         assert(_sil_segs[i].type()==p->type());
         p->add
         (_sil_segs[i].p(), true, _sil_segs[i].id_invis() , _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
      }
   }

   // computes lengths:
   p->complete();


   assert(( _use_new_idref_method ) );
   if ( _use_new_idref_method ) {

      //retrieve the correct length
      //for the front-facing segment
      //drawn into ref image

      //id_set().num() == 1 all the time!
      for ( int j=0; j < p->id_set().num()  ; j++ ) {
         int ind = _path_ids.get_index(p->id_set(j) );
         assert ( _path_ids.valid_index( ind ) ) ;
         p->ffseg_lengths() += _ffseg_lengths[ind];

      }

      int n = p->num();

      if ( p->id_set().num() == 1 && p->ff_len(0) > p->ff_len(n-1)  ) {
         assert(0); // XXX - Not needed these days... For now...
         //cerr << "\tsingle wrap" << endl;
         //segment is located where loop closes
         int offset=n-1;
         while ( offset > 0 && p->ff_len(offset) > p->ff_len(offset-1) )
            offset--;

         p->id_offsets().add     (offset);
         p->id_set().add         (p->id_set(0));
         p->ffseg_lengths().add  (p->ffseg_length(0));

      }  //else , multiple ids found, but we end where we began
      else if ( p->id_set().num() > 1 && p->id_set().first() == ( p->id(n-1) & 0xffffff00 ) ) {
         assert(0); // XXX - Not needed these days... For now...
         //loop is multisegment
         //cerr << "\tmultisegment with wrap" << endl;
         uint first_id = p->id_set().first();
         int offset=n-1;
         while ( offset > 0 && ( p->id(offset-1) & 0xffffff00 ) == first_id )
            offset--;

         p->id_offsets().add        (offset);
         p->id_set().add            (p->id_set(0));
         p->ffseg_lengths().add     (p->ffseg_length(0));

      }
      //else if ( p->id_set().num() > 1 ) cerr << "\tnon-wrapping multi" << endl;
      //else cerr << "\tnon-wrapping single" << endl;
      //okay, so now if we have any sort of
      //loop trickery, we have a duplicate entry in the
      //id_set to indicate that.

      //there's an extra entry in the offsets array for the end of the stroke
      //so that now the path in
      //id_set[i] is of ndc length ffseg_lengths[i]
      //and extends from id_offsets[i] to id_offsets[i+1]-1;
      p->id_offsets().add(n);

   }

   i++;
   return 1;
}

void
ZXedgeStrokeTexture::add_paths()
{  
   
   for (int i=0; i<_sil_segs.num() && add_path(i); i++) 
      ;

}

bool
ZXedgeStrokeTexture::add_path(int& i)
{

   if ( get_new_branch() )
      return add_path_seethru(i);



   static bool long_paths = Config::get_var_bool("LONG_LUBO",false,true);

   // if long_paths is set, go for long paths, including occluded parts.
   // otherwise force breaks in path at non-visible parts.

   // find start of next path:
   if (long_paths) {
      while (_sil_segs.valid_index(i) && !_sil_segs[i].e())
         i++;
   } else {
      while (_sil_segs.valid_index(i) && !(_sil_segs[i].e() && _sil_segs[i].v()==SIL_VISIBLE))
         i++;
   }

   // if there is no next path we're done
   if (!_sil_segs.valid_index(i))
      return 0;

   LuboPath* p = new LuboPath;
   _paths += p;

   //to interface with rob's new drawing code
   p->type() = _sil_segs[i].type();
   p->vis()  = SIL_VISIBLE;

   if (long_paths) {

      for ( ; _sil_segs.valid_index(i) && _sil_segs[i].e(); i++)
         p->add
         (_sil_segs[i].p(), _sil_segs[i].v()==SIL_VISIBLE, _sil_segs[i].id(), _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());

      // add the last one
      if (_sil_segs.valid_index(i))
         p->add
         (_sil_segs[i].p(), _sil_segs[i].v()==SIL_VISIBLE, _sil_segs[i].id(), _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());

   } else {

      for ( ; _sil_segs.valid_index(i) && _sil_segs[i].e() && _sil_segs[i].v()==SIL_VISIBLE; i++)
      {        
         p->add
         (_sil_segs[i].p(), _sil_segs[i].v()==SIL_VISIBLE, _sil_segs[i].id(), _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());

      }
      // add the last one
      if (_sil_segs.valid_index(i) && _sil_segs[i].v()==SIL_VISIBLE){
         p->add
         (_sil_segs[i].p(), _sil_segs[i].v()==SIL_VISIBLE, _sil_segs[i].id(), _sil_segs[i].s(), _sil_segs[i].bc(), _sil_segs[i].pl());
        
      }
   }

   // computes lengths:
   p->complete();

   //fprintf(stderr, "lubopath has %d ids\n", p->id_set().num() );
   if ( _use_new_idref_method ) {

      //retrieve the correct length
      //for the front-facing segment
      //drawn into ref image
      //int idn = p->id_set().num();
      for ( int j=0; j < p->id_set().num()  ; j++ ) {
         int ind = _path_ids.get_index(p->id_set(j) );
         if ( _path_ids.valid_index( ind ) )
            p->ffseg_lengths() += _ffseg_lengths[ind];
         else {
            //we have a bogus id here ( not drawn in ref image )
            cerr << "whoa, bogus id!\t" << "%x" << p->id_set(j) << endl;
            p->id_set()    .pull_index(j);
            p->id_offsets().pull_index(j);
            j--; //check at this index again
         }
         //fprintf (stderr , "\tpath %d has length %f\n", j, p->ffseg_length(j) );
      }
      //fprintf(stderr, "\t it has %d ids after cleaning\n", p->id_set().num() );

      //buy american
      int n = p->num();
      if ( p->id_set().num() == 1 && p->ff_len(0) > p->ff_len(n-1)  ) {
         //cerr << "\tsingle wrap" << endl;
         //segment is located where loop closes
         int offset=n-1;
         while ( offset > 0 && p->ff_len(offset) > p->ff_len(offset-1) )
            offset--;

         p->id_offsets().add     (offset);
         p->id_set().add         (p->id_set(0));
         p->ffseg_lengths().add  (p->ffseg_length(0));

      }  //else , multiple ids found, but we end where we began
      else if ( p->id_set().num() > 1 && p->id_set().first() == ( p->id(n-1) & 0xffffff00 ) )   {
         //loop is multisegment
         //cerr << "\tmultisegment with wrap" << endl;
         uint first_id = p->id_set().first();
         int offset=n-1;
         while ( offset > 0 && ( p->id(offset-1) & 0xffffff00 ) == first_id )
            offset--;

         p->id_offsets().add        (offset);
         p->id_set().add            (p->id_set(0));
         p->ffseg_lengths().add     (p->ffseg_length(0));

      }
      //else if ( p->id_set().num() > 1 ) cerr << "\tnon-wrapping multi" << endl;
      //else cerr << "\tnon-wrapping single" << endl;
      //okay, so now if we have any sort of
      //loop trickery, we have a duplicate entry in the
      //id_set to indicate that.

      //there's an extra entry in the offsets array for the end of the stroke
      //so that now the path in
      //id_set[i] is of ndc length ffseg_lengths[i]
      //and extends from id_offsets[i] to id_offsets[i+1]-1;
      p->id_offsets().add(n);

   }


   return 1;
}

void
ZXedgeStrokeTexture::propagate_sil_parameterization_seethru()
{

   static bool draw_props =  Config::get_var_bool("DRAW_PROPAGATION",false,true) ;
   static bool no_box_check = Config::get_var_bool("NO_BOX_CHECK",false,true);


   // if we have changed tracking styles, throw out the previous parametrization
   // this should be semi-temporary.
   
   if (!_lubo_samples.empty()) {

      // Get mapping from old NDC to new:
      //   xfp is valid for points
      //   xfn is valid for "normal" vectors
      //
      //assert(_patch && _patch->mesh());
      //      Wtransf xfp = _patch->mesh()->obj_to_ndc() * _old_ndc.inverse();
      //      Wtransf xfn = xfp.inverse().transpose();

      Wtransf xfp = (_patch) ? _patch->xform() : (_mesh) ? _mesh->xform() : Identity;
      if (_patch)
          (void)_patch->inv_xform().transpose();
      else if (_mesh)
          (void)_mesh->inv_xform().transpose();
      Wtransf obj = get_obj_to_ndc(_patch, _mesh);

      Wpt wp;
      Wvec wn;
      // step size of 1 pixel of the reference image
      double step = 1.0 * _pix_to_ndc_scale * _screen_to_ref;

      // Max number of steps to take (1 pixel each):
      static const int MAX_STEPS = Config::get_var_int("MAX_LUBO_STEPS", 6, true);

      // start fresh:
      _paths.reset_votes();

      int num_missed = 0;

      ARRAY<Vec2i> offsets;

      offsets += Vec2i( 0, 0);
      offsets += Vec2i(-1,-1);
      offsets += Vec2i( 0,-1);
      offsets += Vec2i( 1,-1);
      offsets += Vec2i(-1, 0);
      offsets += Vec2i( 1, 0);
      offsets += Vec2i(-1, 1);
      offsets += Vec2i( 0, 1);
      offsets += Vec2i( 1, 1);



      ARRAY<uint>       ids;               // loop ids found whilst sampling the idref
      ARRAY<LuboPath*>  matching_paths;    // paths which match the above ids
      ARRAY<uint>       matching_ids;
      // Propagate parameter choices from old samples to new paths:
      //cerr << "Propagation::attempting to propagate %d votes " << _lubo_samples.num() << endl;

      //cerr << "=====================\nNum Lubos: " << _lubo_samples.num() << "\n";

      for (int i=0; i<_lubo_samples.num(); i++) {
         //cerr << "\t\t" << i ;

         ids.clear();
         matching_paths.clear();
         matching_ids.clear();
         // Map old points/normals to new screen locations:

         LuboSample& lbsample = _lubo_samples[i];



         lbsample.get_wpt( wp );

         //wp = obj * wp;

         //NDCZpt p( wp[0],wp[1],wp[2]

         NDCZpt  p( xfp * /* TFMULTFIX */ wp ) ;

         if ( !p.in_frustum() )
            continue; // cull samples out of frustum

         if(!get_bface(lbsample._s)) continue;

         (get_bface(lbsample._s))->bc2norm_blend(lbsample._bc, wn);

         NDCZvec n( /*xfn * */wn , obj.derivative(wp) );

         // The plan is to search in the ID reference image from
         // p along the n direction a short distance, looking for
         // stroke IDs
         // Decide delt vector for stepping:

         NDCvec delt = NDCvec(n).normalized()*step;

         if ( lbsample._type == STYPE_BF_SIL )
            delt *= -1.0;

         //       NDCvec perp = delt.perpend() ;
         LuboPath* path = 0;    // path to be found
         uint id = 0;
         NDCpt cur;             // location where found
         //         int x = 0;
         int j;

         //cerr << "Sample #" << i << ":\n";

         for (j = 0; j<MAX_STEPS && matching_paths.num() == 0 ; j++) {
            // Check the ref image for a stroke at each point
            // along the search direction:
            cur = p + delt*j;
            Point2i cent = _id_ref->ndc_to_pix(cur);

            if ( j == 0 ) {
               // XXX - test a neighborhood of pixels
               // when we are at the "original position"
               if ( no_box_check ) {
                  id = _id_ref->val(cent);
                  if ( id_fits_sample(id, lbsample) )
                     ids.add_uniquely(id);

                  if (debug_lubo || draw_props) {
                     // draws a white dot at the hit point
                     _id_ref->val(cent) = 0x009f9f9f;
                     if ( id == 0 )
                        _id_ref->val(cent) = 0x00ffffff;
                  }
               } else {
                  for ( int off = 0 ; off < offsets.num(); off++ ) {
                     id = _id_ref->val(cent+offsets[off]);
                     if  ( id_fits_sample(id, lbsample) )
                        ids.add_uniquely(id);

                     if (debug_lubo || draw_props) {
                        // draws a white dot at the hit point,greem if right vis
                        if  ( id_fits_sample(id, lbsample) )
                           _id_ref->val(cent+offsets[off] ) = 0x0000ff00;
                        else
                           _id_ref->val(cent+offsets[off] ) = 0x00ffffff;
                     }
                  }
               }
            } else {
               id = _id_ref->val(cent);
               if ( id_fits_sample(id, lbsample) )
                  ids.add_uniquely(id);

               if (debug_lubo || draw_props) {
                  if ( id_fits_sample(id, lbsample) )
                     _id_ref->val(cent) = 0x0000ff00;
                  else
                     _id_ref->val(cent ) = 0x00ffffff;
               }

            }
            //cerr << "  " << ids.num() << " IDs --> ";
            // find a path that owns that id:
            assert ( _use_new_idref_method );
            if ( _use_new_idref_method ) {
               for  ( int k = 0 ; k < ids.num() ; k++ ) {
                  int x =0;
                  while ( (path= _paths.lookup(ids[k] & 0xffffff00, x)) ) {
                     //cerr << "stype:" << lbsample._type << " svis: " << lbsample._vis;
                     //cerr << " ptype:" << path->type() << " pvis:" << path->vis() << endl;
                     if ( sample_matches_path ( lbsample, path ) && path->in_range(ids[k]) ) {
                        //cerr << "\tsample matches path" << endl;
                        //if ( matching_paths.add_uniquely(path) ) matching_ids.add(ids[k]);
                        matching_paths.add(path);
                        matching_ids.add  (ids[k]);
                     }
                     x++;
                  }
               }
               //if we are unsuccessful, clear these ids..
               if ( matching_paths.num() == 0 )
                  ids.clear();
            }
            //cerr << matching_ids.num() << " matches. ";
            // If it hits "air" (background) there's no point
            // continuing at all. We're supposed to crawl from
            // inside the mesh toward the silhouette.
            // but don't do this until we're away from our base point...
            if (j > 2 && id == 0) {
               //cerr << " Air Ball!\n";
               break;
            } else {
               //cerr << "\n";
            }

         }


         if (j == MAX_STEPS) {
            num_missed++;
         }



         // if matching_paths isn't empty, search for the closest one to this point
         if (matching_paths.num() > 0 ) {
            LuboPath*   closest_path = NULL;
            double      min_dist = DBL_MAX;
            double      tmp_dist = 0;
            NDCpt       intersection_point;
            NDCpt       tmp_point;
            int         segment_index=0;
            int         tmp_index=0;
            //            int         path_index=0;

            // find the path that comes closest to tha
            for ( int k = 0 ; k < matching_paths.num() ; k++ ) {
               assert( _use_new_idref_method ) ;
               if ( _use_new_idref_method ) {
                  tmp_dist = matching_paths[k]->get_closest_point_at(matching_ids[k], cur, delt, tmp_point, tmp_index );
                  //tmp_dist = matching_paths[k]->get_closest_point( cur, delt, tmp_point, tmp_index );
               }
               /*
                              else tmp_dist = matching_paths[k]->get_closest_point( cur, delt, tmp_point, tmp_index );
               */
               if ( tmp_dist < min_dist ) {
                  min_dist             = tmp_dist;
                  closest_path         = matching_paths[k];
                  intersection_point   = tmp_point;
                  segment_index        = tmp_index;
               }
            }

            if ( closest_path ) {
               //cerr << "SCORED!!!\n";
               closest_path->register_vote( lbsample, lbsample._path_id , intersection_point, segment_index );
            } else {
               //XXX Right?!
               assert( closest_path );
               num_missed++;
            }

         } else {
            //cerr << "FAILED!!!\n";
            num_missed++;
         }


      }

      //if ( num_missed > 2 ) { err_mesg(ERR_LEV_ERROR, "missed %d of %d", num_missed, _lubo_samples.num() ); }
      if (debug_lubo && num_missed > 3) {
         err_mesg(ERR_LEV_ERROR, "num missed: %d", num_missed);
      }

      //cerr << "Missed: " << num_missed << "\n";
   }

   // save current projection matrix into old one  
   _old_ndc = get_obj_to_ndc(_patch, _mesh);

}

void
ZXedgeStrokeTexture::propagate_sil_parameterization()
{
   // Apply the "Lubo" algorithm


   // branch for new stuff
   if ( get_new_branch() ) { propagate_sil_parameterization_seethru(); return; }
   // end

   static bool draw_props =  Config::get_var_bool("DRAW_PROPAGATION",false,true) ;
   static bool no_box_check = Config::get_var_bool("NO_BOX_CHECK",false,true);


   // if we have changed tracking styles, throw out the previous parametrization
   // this should be semi-temporary.

   
   if (!_lubo_samples.empty()) {

      // Get mapping from old NDC to new:
      //   xfp is valid for points
      //   xfn is valid for "normal" vectors
      //
      //assert(_patch && _patch->mesh());
      //      Wtransf xfp = _patch->mesh()->obj_to_ndc() * _old_ndc.inverse();
      //      Wtransf xfn = xfp.inverse().transpose();

      
      Wtransf xfp = (_patch) ? _patch->xform() : (_mesh) ? _mesh->xform() : Identity;
      if(_patch)
          (void)_patch->inv_xform().transpose();
      else if (_mesh)
          (void)_mesh->inv_xform().transpose();
      Wtransf obj = get_obj_to_ndc(_patch, _mesh);

      Wpt wp;
      Wvec wn;
      // step size of 1 pixel of the reference image
      double step = 1.0 * _pix_to_ndc_scale * _screen_to_ref;

      // Max number of steps to take (1 pixel each):
      static const int MAX_STEPS = Config::get_var_int("MAX_LUBO_STEPS", 6,true);

      // start fresh:
      _paths.reset_votes();

      int num_missed = 0;

      ARRAY<Vec2i> offsets;

      offsets += Vec2i( 0, 0);
      offsets += Vec2i(-1,-1);
      offsets += Vec2i( 0,-1);
      offsets += Vec2i( 1,-1);
      offsets += Vec2i(-1, 0);
      offsets += Vec2i( 1, 0);
      offsets += Vec2i(-1, 1);
      offsets += Vec2i( 0, 1);
      offsets += Vec2i( 1, 1);

      ARRAY<uint>       ids;               // loop ids found whilst sampling the idref
      ARRAY<LuboPath*>  matching_paths;    // paths which match the above ids
      ARRAY<uint>       matching_ids;
      // Propagate parameter choices from old samples to new paths:
      //cerr << "Propagation::attempting to propagate %d votes " << _lubo_samples.num() << endl;
      //cerr << "Num Lubos = " << _lubo_samples.num() << "\n";

      for (int i=0; i<_lubo_samples.num(); i++) {
         //cerr << "\t\t" << i ;
         ids.clear();
         matching_paths.clear();
         matching_ids.clear();
         // Map old points/normals to new screen locations:

         _lubo_samples[i].get_wpt ( wp );


         NDCZpt p(xfp * /* TFMULTFIX */ wp);

         if ( !p.in_frustum() )
            continue; // cull samples out of frustum

         if(!get_bface(_lubo_samples[i]._s)) continue; 
       
         (get_bface(_lubo_samples[i]._s))->bc2norm_blend(_lubo_samples[i]._bc, wn);

         NDCZvec n( /*xfn **/ wn , obj.derivative(wp) );

         // The plan is to search in the ID reference image from
         // p along the n direction a short distance, looking for
         // stroke IDs
         // Decide delt vector for stepping:

         NDCvec delt = NDCvec(n).normalized()*step;
         //         NDCvec perp = delt.perpend() ;
         LuboPath* path = 0;    // path to be found
         uint id = 0;
         uint tmp_id =0;               // matching path ID found in id ref
         NDCpt cur;             // location where found
         //         int x = 0;
         int j;


         for (j = 0; j<MAX_STEPS && matching_paths.num() == 0 ; j++) {
            // Check the ref image for a stroke at each point
            // along the search direction:
            cur = p + delt*j;
            Point2i cent = _id_ref->ndc_to_pix(cur);

            if ( j == 0 ) {
               // XXX - test a neighborhood of pixels
               // when we are at the "original position"
               if ( no_box_check ) {
                  id = _id_ref->val(cent);
                  if ( is_path_id(id) )
                     ids.add_uniquely(id);
               } else {
                  for ( int off = 0 ; off < offsets.num(); off++ ) {
                     tmp_id = _id_ref->val(cent+offsets[off]);
                     if  ( is_path_id ( tmp_id ) ) {
                        if ( debug_lubo && is_path_id ( id ) && tmp_id != id )
                           cerr << "XXXmultiple ids found!" << endl;
                        id = tmp_id;
                        if ( is_path_id(id) )
                           ids.add_uniquely(id);

                     }

                     if (debug_lubo || draw_props) {
                        // draws a white dot at the hit point
                        _id_ref->val(cent+offsets[off] ) = 0x009f9f9f;
                        if ( id == 0 )
                           _id_ref->val(cent+offsets[off] ) = 0x00ffffff;
                     }
                  }
               }
            } else {
               //otherwise just sample at the pixel you land in
               id = _id_ref->val(cent);
               if ( is_path_id(id) )
                  ids.add_uniquely(id);


               if (debug_lubo || draw_props) {
                  // draws a white dot at the hit point
                  _id_ref->val(cent) = 0x009f9f9f;
                  if ( id == 0 )
                     _id_ref->val(cent ) = 0x00ffffff;
               }

            }



            // find a path that owns that id:

            if ( _use_new_idref_method ) {
               for  ( int k = 0 ; k < ids.num() ; k++ ) {
                  int x =0;
                  while ( (path= _paths.lookup(ids[k] & 0xffffff00, x)) ) {
                     if ( path->in_range(ids[k]) ) {

                        //if ( matching_paths.add_uniquely(path) ) matching_ids.add(ids[k]);
                        matching_paths.add(path);
                        matching_ids.add  (ids[k]);

                     }
                     x++;
                  }
               }
            } else {
               for  ( int k = 0 ; k < ids.num() ; k++ ) {
                  int x =0;
                  while ( (path= _paths.lookup(ids[k], x)) ) {
                     matching_paths.add_uniquely(path);
                     x++;
                  }
               }
            }


            // If it hits "air" (background) there's no point
            // continuing at all. We're supposed to crawl from
            // inside the mesh toward the silhouette.
            // but don't do this until we're away from our base point...
            if (j > 2 && id == 0)
               break;

         }




         if (j == MAX_STEPS)
            num_missed++;



         // if matching_paths isn't empty, search for the closest one to this point
         if (matching_paths.num() > 0 ) {
            LuboPath*   closest_path = NULL;
            double      min_dist = DBL_MAX;
            double      tmp_dist = 0;
            NDCpt       intersection_point;
            NDCpt       tmp_point;
            int         segment_index=0;
            int         tmp_index=0;
            //            int         path_index=0;

            // find the path that comes closest to tha
            for ( int k = 0 ; k < matching_paths.num() ; k++ ) {
               //XXX - rob , get_closest_point is the function that tests path-point distance

               if ( _use_new_idref_method ) {
                  tmp_dist = matching_paths[k]->get_closest_point_at(matching_ids[k], cur, delt, tmp_point, tmp_index );
                  //tmp_dist = matching_paths[k]->get_closest_point( cur, delt, tmp_point, tmp_index );
               } else
                  tmp_dist = matching_paths[k]->get_closest_point( cur, delt, tmp_point, tmp_index );

               if ( tmp_dist < min_dist ) {
                  min_dist             = tmp_dist;
                  closest_path         = matching_paths[k];
                  intersection_point   = tmp_point;
                  segment_index        = tmp_index;
               }
            }

            if ( closest_path ) {

               closest_path->register_vote( _lubo_samples[i], _lubo_samples[i]._path_id , intersection_point, segment_index );

            } else
               num_missed++;

         } else
            num_missed++;


      }

      //if ( num_missed > 2 ) { err_mesg(ERR_LEV_ERROR, "missed %d of %d", num_missed, _lubo_samples.num() ); }
      if (debug_lubo && num_missed > 3) {
         err_mesg(ERR_LEV_ERROR, "num missed: %d", num_missed);
      }

      // Do "voting" on new strokes
   }

   // save current projection matrix into old one
   _old_ndc = get_obj_to_ndc(_patch, _mesh);
}


void
ZXedgeStrokeTexture::regen_group_samples()
{

   static int SAMPLE_STEP = Config::get_var_int("LUBO_SAMPLE_STEP", 4,true);
   double sample_dist = _vis_sampling * _pix_to_ndc_scale * SAMPLE_STEP;
   _paths.gen_group_samples(sample_dist, _lubo_samples);

}


/*****************************************************************
* LuboVote
*****************************************************************/

/////////////////////////////////////
// get_status()
/////////////////////////////////////
/*
void
LuboVote::get_status(TAGformat &d)
{
   //cerr << "LuboVote::get_status()\n";
 
   int status;
 
   *d >> status;
 
   _status = (lv_status_t)status;
 
 
}
  */
/////////////////////////////////////
// put_status()
/////////////////////////////////////
/*void
LuboVote::put_status(TAGformat &d) const
{
   cerr << "LuboVote::put_status()\n";
 
   d.id();
   *d << (int)_status;
   d.end_id();
}
   */
/*****************************************************************
* LuboPathList
*****************************************************************/

TAGlist* LuboPathList::_lpl_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
LuboPathList::tags() const
{
   if (!_lpl_tags) {
      _lpl_tags = new TAGlist;

      *_lpl_tags += new TAG_meth<LuboPathList>(
                       "path",
                       &LuboPathList::put_paths,
                       &LuboPathList::get_path,
                       1);

   }
   return *_lpl_tags;
}



/////////////////////////////////////
// get_path()
/////////////////////////////////////
void
LuboPathList::get_path(TAGformat &d)
{
   cerr << "LuboPathList::get_path()\n";

   //Grab the class name... should be BaseStrokeOffset
   str_ptr str;
   *d >> str;

   if ((str != LuboPath::static_name())) {
      // XXX - should throw away stuff from unknown obj
      cerr << "LuboPathList::get_path() - Not LuboPath: '" << str << "'" << endl;
      return;

   }


   LuboPath *p = new LuboPath;
   assert(p);

   p->decode(*d);
   p->complete();

   add
      (p);
}


/////////////////////////////////////
// put_paths()
/////////////////////////////////////
void

LuboPathList::put_paths(TAGformat &d) const
{
   cerr << "LuboPathList::put_paths()\n";

   int i;

   for (i=0; i<num(); i++) {
      d.id();
      ((*this)[i])->format(*d);
      d.end_id();

   }


}

/////////////////////////////////////
// votepath_id_to_index()
/////////////////////////////////////
int
LuboPathList::votepath_id_to_index(uint id) const
{

   //Given a votepath id from a previous frame, return the index of the
   //path in this frame with the most votes that have that ._path_id
   //if a tie, returns first
   //if not found , returns -1

   int max_ind = -1;
   int max_count = 0;

   for ( int i=0; i < num(); i++ ) {

      LuboPath * lp = _array[i];
      int count = 0;
      for ( int j =0 ; j < lp->votes().num() ; j++ ) {
         if ( lp->votes()[j]._path_id == id )
            count++;
      }
   if ( count > max_count ) { max_count = count; max_ind = i;}
   }

   return max_ind;
}
/////////////////////////////////////
// strokepath_id_to_index()
/////////////////////////////////////
int
LuboPathList::strokepath_id_to_index(uint id, int path_index) const
{

   //Given a stroke id from a previous frame, return the index of the
   //stroke in path possessing the most votes from that id

   //initially id's are unique to groups,
   //but not after merge or split operations )

   //returns -1 if no strokes match

   int stroke_ind = -1;
   int max_count = 0;
   LuboPath * lp = _array[path_index];

   for ( int i=0; i < lp->groups().num(); i++ ) {
      VoteGroup& g = lp->groups()[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD)
         continue;
      int count = 0;
      for ( int j=0; j < g.num() ; j++ )
         if ( g.vote(j)._stroke_id == id )
            count++;
   if ( count > max_count ) { max_count = count; stroke_ind = i; }
   }

   return stroke_ind;
}

/////////////////////////////////////
// strokepath_id_to_indices()
/////////////////////////////////////

bool
LuboPathList::strokepath_id_to_indices(uint id, int* path_index, int* stroke_index ) const
{

   //Given a stroke id from a previous frame, return the index of the
   //path in this frame with the most votes that have that ._stroke_id
   //if a tie, returns first
   //if not found , returns -1

   *path_index       = -1;
   *stroke_index     = -1;
   int max_count     = 0;
   int i,j,k;
   for (  i=0; i < num(); i++ ) {
      LuboPath * lp = _array[i];
      for ( j =0 ; j < lp->groups().num(); j++ ) {
         VoteGroup& g   = lp->groups()[j];
         if (g.status() != VoteGroup::VOTE_GROUP_GOOD)
            continue;
         int count      = 0;
         for ( k = 0 ; k < g.num() ; k++ )
            if ( g.vote(k)._stroke_id == id )
               count++;
      if  ( count > max_count ) { max_count = count; *path_index = i; *stroke_index = j;}
      }
   }

   return (*path_index < 0) ? false : true ;

}


/*****************************************************************
* LuboPath
*****************************************************************/

TAGlist* LuboPath::_lp_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
LuboPath::tags() const
{
   if (!_lp_tags) {
      _lp_tags = new TAGlist;

      *_lp_tags += new TAG_val<LuboPath,double>(
                      "stretch",
                      &LuboPath::stretch_);
      *_lp_tags += new TAG_val<LuboPath,double>(
                      "pix_to_ndc_scale_",
                      &LuboPath::pix_to_ndc_scale_);
      *_lp_tags += new TAG_val<LuboPath,double>(
                      "offset_pix_len",
                      &LuboPath::offset_pix_len_);

      *_lp_tags += new TAG_meth<LuboPath>(
                      "pts",
                      &LuboPath::put_pts,
                      &LuboPath::get_pts,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "path_id",
                      &LuboPath::put_path_id,
                      &LuboPath::get_path_id,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "bcs",
                      &LuboPath::put_bcs,
                      &LuboPath::get_bcs,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "faces",
                      &LuboPath::put_faces,
                      &LuboPath::get_faces,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "len",
                      &LuboPath::put_len,
                      &LuboPath::get_len,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "id_set",
                      &LuboPath::put_id_set,
                      &LuboPath::get_id_set,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "id_offsets",
                      &LuboPath::put_id_offsets,
                      &LuboPath::get_id_offsets,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "ffseg_lengths",
                      &LuboPath::put_ffseg_lengths,
                      &LuboPath::get_ffseg_lengths,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "vote",
                      &LuboPath::put_votes,
                      &LuboPath::get_vote,
                      1);
      *_lp_tags += new TAG_meth<LuboPath>(
                      "group",
                      &LuboPath::put_groups,
                      &LuboPath::get_group,
                      1);
   }
   return *_lp_tags;
}

/////////////////////////////////////
// get_pts()
/////////////////////////////////////
void
LuboPath::get_pts(TAGformat &d)
{
   cerr << "LuboPath::get_pts()\n";

   *d >> _pts;

}

/////////////////////////////////////
// put_pts()
/////////////////////////////////////
void
LuboPath::put_pts(TAGformat &d) const
{
   cerr << "LuboPath::put_pts()\n";

   d.id();
   *d << _pts;
   d.end_id();
}

/////////////////////////////////////
// get_path_id()
/////////////////////////////////////
void
LuboPath::get_path_id(TAGformat &d)
{
   cerr << "LuboPath::get_path_id()\n";

   *d >> _path_id;

}

/////////////////////////////////////
// put_path_id()
/////////////////////////////////////
void
LuboPath::put_path_id(TAGformat &d) const
{
   cerr << "LuboPath::put_path_id()\n";

   d.id();
   *d << _path_id;
   d.end_id();
}

/////////////////////////////////////
// get_bcs()
/////////////////////////////////////
void
LuboPath::get_bcs(TAGformat &d)
{
   cerr << "LuboPath::get_bcs()\n";

   *d >> _bcs;

}

/////////////////////////////////////
// put_bcs()
/////////////////////////////////////
void
LuboPath::put_bcs(TAGformat &d) const
{
   cerr << "LuboPath::put_bcs()\n";

   d.id();
   *d << _bcs;
   d.end_id();

}

/////////////////////////////////////
// get_faces()
/////////////////////////////////////
void
LuboPath::get_faces(TAGformat &d)
{
   cerr << "LuboPath::get_faces()\n";

   int num;

   *d >> num;

   for (int i=0; i<num; i++) {
      int face_pointer;
      *d >> face_pointer;
      _simplexes.add((Bsimplex*)face_pointer);
   }

}

/////////////////////////////////////
// put_faces()
/////////////////////////////////////
void
LuboPath::put_faces(TAGformat &d) const
{
   cerr << "LuboPath::put_faces()\n";

   d.id();
   *d << _simplexes.num();
   for (int i=0; i<_simplexes.num(); i++) {
      *d << (int)_simplexes[i];
   }
   d.end_id();

}

/////////////////////////////////////
// get_len()
/////////////////////////////////////
void
LuboPath::get_len(TAGformat &d)
{
   cerr << "LuboPath::get_len()\n";

   *d >> _len;

}

/////////////////////////////////////
// put_len()
/////////////////////////////////////
void
LuboPath::put_len(TAGformat &d) const
{
   cerr << "LuboPath::put_len()\n";

   d.id();
   *d << _len;
   d.end_id();
}

/////////////////////////////////////
// get_id_set()
/////////////////////////////////////
void
LuboPath::get_id_set(TAGformat &d)
{
   cerr << "LuboPath::get_id_set()\n";

   *d >> _id_set;

}

/////////////////////////////////////
// put_id_set()
/////////////////////////////////////
void
LuboPath::put_id_set(TAGformat &d) const
{
   cerr << "LuboPath::put_id_set()\n";

   d.id();
   *d << _id_set;
   d.end_id();
}

/////////////////////////////////////
// get_id_offsets()
/////////////////////////////////////
void
LuboPath::get_id_offsets(TAGformat &d)
{
   cerr << "LuboPath::get_id_offsets()\n";

   *d >> _id_offsets;

}

/////////////////////////////////////
// put_id_offsets()
/////////////////////////////////////
void
LuboPath::put_id_offsets(TAGformat &d) const
{
   cerr << "LuboPath::put_id_offsets()\n";

   d.id();
   *d << _id_offsets;
   d.end_id();
}

/////////////////////////////////////
// get_ffseg_lengths()
/////////////////////////////////////
void
LuboPath::get_ffseg_lengths(TAGformat &d)
{
   cerr << "LuboPath::get_ffseg_lengths()\n";

   *d >> _ffseg_lengths;

}

/////////////////////////////////////
// put_ffseg_lengths()
/////////////////////////////////////
void
LuboPath::put_ffseg_lengths(TAGformat &d) const
{
   cerr << "LuboPath::put_ffseg_lengths()\n";

   d.id();
   *d << _ffseg_lengths;
   d.end_id();
}

/////////////////////////////////////
// get_vote()
/////////////////////////////////////
void
LuboPath::get_vote(TAGformat &d)
{
   //   cerr << "LuboPath::get_vote()\n";

   //Grab the class name... should be LuboVote
   str_ptr str;
   *d >> str;

   if ((str != LuboVote::static_name())) {
      // XXX - should throw away stuff from unknown obj
      cerr << "LuboPath::get_vote() - Not LuboVote: '" << str << "'" << endl;
      return;
   }

   _votes.add(LuboVote());
   _votes.last().decode(*d);

}

/////////////////////////////////////
// put_votes()
/////////////////////////////////////
void
LuboPath::put_votes(TAGformat &d) const
{
   cerr << "LuboPath::put_votes()\n";

   int i;

   for (i=0; i<_votes.num(); i++) {
      d.id();
      _votes[i].format(*d);
      d.end_id();
   }

}

/////////////////////////////////////
// get_group()
/////////////////////////////////////
void
LuboPath::get_group(TAGformat &d)
{
   cerr << "LuboPath::get_group()\n";

   //Grab the class name... should be VoteGroup
   str_ptr str;
   *d >> str;

   if ((str != VoteGroup::static_name())) {
      // XXX - should throw away stuff from unknown obj
      cerr << "LuboPath::get_group() - Not VoteGroup: '" << str << "'" << endl;
      return;
   }

   _groups.add(VoteGroup(this));
   _groups.last().decode(*d);

}

/////////////////////////////////////
// put_groups()
/////////////////////////////////////
void
LuboPath::put_groups(TAGformat &d) const
{
   cerr << "LuboPath::put_groups()\n";

   int i;

   for (i=0; i<_groups.num(); i++) {
      d.id();
      _groups[i].format(*d);
      d.end_id();
   }

}


void
LuboPath::clear()
{
   _pts.clear();
   _path_id.clear();
   _id_set.clear();
   _ffseg_lengths.clear();
   _id_offsets.clear();
   _len.clear();
   _stretch = 1;
   _pix_to_ndc_scale = 1;

}

void
LuboPath::add
   (CNDCZpt& p, bool vis, uint id)
{
   _pts.add(p);
   _path_id.add(id);
   _id_set.add_uniquely(id);    // add each id just once

}

void
LuboPath::add
   (CNDCZpt& p, bool vis, uint id, Bsimplex * s, CWvec& bc, double len)
{
   _pts.add(p);
   _path_id.add(id);       //full id ( plus length )
   _simplexes.add (s);
   _bcs.add (bc);
   if (_id_set.add_uniquely(id & 0xffffff00) )
      _id_offsets.add(_pts.num()-1);    // add each id just once
   _len.add(len);

}

void
LuboPath::gen_group_samples( double spacing , int path_index,  ARRAY<LuboSample>& samples) const
{

   double t;
   Bsimplex* s;
   Wvec bc;      
   
   if (!( num() > 1 ) ) {
      //cerr << "LuboPath::gen_group_samples:::: PATH HAS " << num() << " POINT\n";
      return;
   }
   
   Wtransf inv_xform = (_simplexes.num() && _simplexes[0] && _simplexes[0]->mesh()) ? _simplexes[0]->mesh()->inv_xform() : Identity;

   int b = 0 ;          // buf;
   int n = num()-1 ;    // num()-buf;
   for ( int i=0 ; i < _groups.num() ; i++ ) {
      //create a set of samples for each stroke that we drew along this path
      //each remaining votegroup in the groups array represents a drawn stroke

      VoteGroup& g = _groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD)
         continue;


   if ( g.end() - g.begin() < spacing * 0.1 ) { continue ; }
      int      nsegs       = (int) ceil ( ( g.end() - g.begin() ) / spacing );
      double   nspacing    = ( g.end()-g.begin() ) / nsegs;


      int l, m ,r ;
      int last_added=0;
      l=b;
      r=n;

     
      for  ( int k = 0 ; k <= nsegs && l != n ; k++ ) {


         r=n;

         double target_s = g.begin() + nspacing * k ;

         assert ( l != r );
         while ( ( m=(l+r)/2 ) != l ) {
            if ( target_s > get_s(m) )
               l = m;
            else
               r = m;
         }
         

         //add interpolation point
       
         if ( n-b != 1 && _simplexes[l] == _simplexes[r]) {
            t  = g.get_t(target_s);
            s  = _simplexes[l];

            assert (this == g.lubo_path() );

            if ( get_s(r) == get_s(l) ) {
                cerr << "addr (this) " << this << "\t g.pointer "
                     << g.lubo_path() << endl;

               assert(0);
               //__asm int 3;
            }
            double weight = ( target_s - get_s(l) ) / ( get_s(r) - get_s(l) );

            NDCZpt pt1  = interp ( _pts[l], _pts[r], weight ) ;
            Wpt tmp(pt1);
            tmp = inv_xform * /* TFMULTFIX */ tmp;

            if(!s) continue ;
            s->project_barycentric( tmp, bc );


            samples     += LuboSample ( g.id(), tmp, tan(l).perpend(), t, s, bc );
            samples.last()._path_id    = path_index;
            samples.last()._stroke_id  = g.id();
            samples.last()._type= g.lubo_path()->type(); //assign the same type and
            samples.last()._vis = g.lubo_path()->vis();  //visibility as the parent
            last_added = l;
         } else {
            int add;
            
            if ( last_added != l ) {
               add = l;
            } else {
               add = r;
            }
            
            t  = g.get_t(get_s(add));

            // XXX- should we do something else if it's the last null face?
            s  = (add !=n ) ? _simplexes[add] : _simplexes[add-1]; //if it's the last point you have a null face.  bummer!

            NDCZpt pt1  = _pts[add];
            Wpt tmp(pt1);
            tmp = inv_xform * /* TFMULTFIX */ tmp;

            if(!s) continue ;
            s->project_barycentric( tmp, bc );

            samples     += LuboSample ( g.id(), tmp, tan(add).perpend(), t, s, bc );
            samples.last()._path_id    = path_index;
            samples.last()._stroke_id  = g.id();
            samples.last()._type   = g.lubo_path()->type(); //assign the same type and
            samples.last()._vis   = g.lubo_path()->vis();  //visibility as the parent
            last_added = add;
            l = r;
         }
        

         /***
                     t  = g.get_t(target_s);
                     f  = _faces[l];
                     double weight = 0;
                     if ( get_s(r) != get_s(l) ) { 
                        weight = ( target_s - get_s(l) ) / ( get_s(r) - get_s(l) );
                     }
                     NDCZpt pt1  = interp ( _pts[l], _pts[r], weight ) ;
                     Wpt tmp(pt1);
                     f->project_barycentric( tmp, bc );
                     
                     samples     += LuboSample ( g.id(), tmp, tan(l).perpend(), t, f, bc ); 
                     samples.last()._path_id    = path_index;
                     samples.last()._stroke_id  = g.id();
                     samples.last()._type   = g.lubo_path()->type(); //assign the same type and 
                     samples.last()._vis   = g.lubo_path()->vis();  //visibility as the parent
                     //to avoid cross-pollination
         ****/
      }
   }
}


double
LuboPath::get_closest_point(CNDCpt &p, CNDCvec &v, NDCpt &ret_pt, int& ret_index )
{

   if (_pts.empty())
      return DBL_MAX;

   double   min_dist = DBL_MAX;   // distance to nearest point
   int      min_index = -1;
   double   s = -1;               // arc-len param of nearest point

   NDCpt    min_pt;   // for debugging
   NDCpt    q;


   // XXX - step back from original point so that ray-intersect doesn't twitch
   // when we search along the ray ( vs closest point) we should move back one step
   // to avoid any roundoff/sampling error

   // cur -= delt;

   for (int i=0; i<_pts.num()-1; i++) {
      // XXX
      // get nearest point on seg
      // change this to a ray test
      // using p and v

      q = seg(i).project_to_seg(p);
      double d = q.dist(p);


      if (d < min_dist) {
         min_index = i;
         min_dist = d;
         s = get_s(i) + q.dist(pt(i)); // arc-len parameter of q
         min_pt = q;
      }

   }
   if (s < 0)
      return DBL_MAX;

   ret_index   = min_index;
   ret_pt      = min_pt;
   return min_dist;

}

double
LuboPath::get_closest_point_at(uint ref_val, CNDCpt &p, CNDCvec &v, NDCpt &ret_pt, int& ret_index )
{


   static bool verify_range = Config::get_var_bool("VERIFY_IDREF_RANGE",false,true);
   if (_pts.empty())
      return DBL_MAX;

   int      i , j;

   uint id = ref_val & 0xffffff00;                 //id portion of ref_val
   double lval = (double)(ref_val & 0x000000ff);   //parameter portion of ref_val

   int      n         = _pts.num();
   double   min_dist  = DBL_MAX;   // distance to nearest point
   int      min_index = -1;
   double   s         = -1;               // arc-len param of nearest point
   double   ffs       = -1;
   NDCpt    min_pt;   // for debugging
   NDCpt    q;
   double   d;



   // XXX - step back from original point so that ray-intersect doesn't twitch
   // when we search along the ray ( vs closest point) we should move back one step
   // to avoid any roundoff/sampling error


   // we are given len as a point hit by the algorithm
   // let's search in an area around len;

   // this lubopath may enclose several path id's
   // we have drawn the segment of lengths _ffseg_length into the idref buffer
   // with a single channel varying over the length of the stroke ( 256 segments )
   // we now need to find an appropriate range to search, given a particular segment index

   // if the length <  256 pixels, each segment is pixel
   //        length >  256 pixels, each division is several pixels

   int   pathn    =  _id_set.num();
   bool  wrap     = ( pathn > 1 && id == _id_set[0] && id == _id_set[pathn-1] ); //parameter wrap
   int   ind      = ( wrap ) ? 0 : _id_set.get_index(id); // get index matches from the end of the array


   double pix_dist      =  VIEW::peek()->pix_to_ndc_scale();

   double len           =  ffseg_length(ind) * lval / 255.0;
   double error_margin  =  max ( ffseg_length(ind) / 255.0 , pix_dist*2.0 );


   //len                  += error_margin * 0.5;
   double len_delta     =  3* error_margin;



   double upper_len     = len + len_delta;
   double lower_len     = len - len_delta;

   //fprintf ( stderr , "len %f : len_delta %f low_len %f up_len %f\n", len, len_delta, lower_len, upper_len);

   int l, m, r, lower_ind, upper_ind;


   l = (wrap) ? id_offset(pathn-1)     : id_offset(ind);
   r = (wrap) ? n + id_offset(1)-1     : id_offset(ind+1)-1;
   //cerr << "ind range " << l << " to " << r << endl;
   //binary search for lower index
   while ( ( m=(l+r)/2 ) != l ) {
      if ( lower_len >  _len[m%n] )
         l = m;
      else
         r = m;
   }
   lower_ind = l;

   //left bound is okay - now set right bound back out to start pos;
   r = (wrap) ? n + id_offset(1)-1     : id_offset(ind+1)-1;

   //and search for the upper index
   while ( ( m=(l+r)/2 ) != l ) {
      if ( upper_len > _len[m%n] )
         l = m;
      else
         r = m;
   }
   upper_ind = r;

   //*** special case if we are near the beginning or end ****/
   if ( pathn == 1 ) {
      if ( upper_len > _len[n-1] ) {
         //search around to the first part of path
         double w_upper_len = upper_len - _len[n-1];
         l = 0;
         r = n-1;
         while ( ( m =(l+r)/2 ) != l  ) {
            if ( w_upper_len > _len[m] )
               l=m;
            else
               r=m;
         }
         upper_ind = r+n;           //search a little farther
         //fprintf (stderr, "case 1: lower %d upper %d \n", lower_ind, upper_ind );

      }
      if ( lower_len < 0.0       ) {
         //search around the loop
         double w_lower_len = lower_len + _len[n-1];
         l = 0;
         r = n-1;
         while ( ( m =(l+r)/2 ) != l  ) {
            if ( w_lower_len > _len[m] )
               l=m;
            else
               r=m;
         }
         lower_ind = l;           //need to use the mod factor in the loop to go from the end
         upper_ind = upper_ind+n;   //around to beginning, 'stead of the other way round
         //fprintf (stderr, "case 2: lower %d upper %d \n", lower_ind, upper_ind ) ;
      }
   }
   //traverse these indices
   for (j=lower_ind; j < upper_ind; j++) {
      // XXX
      // get nearest point on seg
      // change this to a ray test
      // using p and v
      i = j%(n);
      if ( i==n-1)
         continue;  //we don't check the fake segment, because if it's a loop
      //first and last points are identical
      q = seg(i).project_to_seg(p);
      d = q.dist(p);
      //cerr << "dist " << d << endl;
      if (d < min_dist) {
         min_index = i;
         min_dist = d;
         s = get_s(i) + q.dist(pt(i)); // arc-len parameter of q
         ffs = ff_len(i) + q.dist(pt(i));
         min_pt = q;
      }

   }

   /// DEBUGGING ///
   //
   //  verify this answer by comparing it to the brute force result
   //  we would have received by searching the entire path
   //
   /////////////////
   if ( verify_range ) {

      double   brute_min_dist  = DBL_MAX;   // distance to nearest point
      int      brute_min_index = -1;
      double   brute_s         = -1;               // arc-len param of nearest point
      double   brute_ffs       = -1;
      NDCpt    brute_min_pt;   // for debugging

      for (j=0; j < n-1; j++) {
         // XXX
         // get nearest point on seg
         // change this to a ray test
         // using p and v


         q = seg(j).project_to_seg(p);
         d = q.dist(p);
         //cerr << "dist " << d << endl;
         if (d < brute_min_dist) {
            brute_min_index = j;
            brute_min_dist = d;
            brute_s = get_s(j) + q.dist(pt(j)); // arc-len parameter of q
            brute_ffs= ff_len(j) + q.dist(pt(j));
            brute_min_pt = q;
         }

      }
      if ( brute_min_dist != min_dist && 1==2) {
         fprintf ( stderr, "********* STAMP: %d ********\n", VIEW::peek()->stamp() );

         fprintf ( stderr, "measure for %f : using id %x : (%d of %d ) -  wrap=%d\nlength %f and %f of total %f delta(pix) = %f\nindex %d to %d of %d total offset %d\n",
                   len, id , ind, _id_set.num(), wrap ,
                   _len[lower_ind%n],   _len[upper_ind%n], _ffseg_lengths[ind], len_delta/pix_dist,
                   lower_ind%n,         upper_ind%n , n  , _id_offsets[ind] );
         fprintf ( stderr,   "min %f ne brute min %f\nindices %d vs %d\tseglengths %f vs %f \n",
                   min_dist,            brute_min_dist,
                   min_index,           brute_min_index,
                   ff_len(min_index),   ff_len(brute_min_index)  );
         cerr << "minpt : " << min_pt << endl << "brutemint : " << brute_min_pt << endl;

         HACK_mouse_right_button_up();

      }
      fprintf( stderr ,"start\t%f\trange\t%f\tfound_s\t%f\tbrute_s\t%f\n", len, len_delta, ffs, brute_ffs );
      //if ( verify_range && pathn > 1) {
      //   cerr << "test range wrap is " << wrap << endl;
      //   fprintf (stderr, "l %d\tr %d\tn %d\tnpaths %d\n", l, r, n, pathn );
      //   for ( int k = 0; k < pathn; k++ ) {
      //      fprintf (stderr, "\tsection %d id %x offset %d length %f\n", k, id_set(k), id_offset(k), ffseg_length(k) );
      //   }                                   cd jot
      
      //   fprintf (stderr, "final offset %d\n", id_offset(pathn) );
      // }
   }

   if (s < 0)
      return DBL_MAX;

   ret_index   = min_index;
   ret_pt      = min_pt;
   return min_dist;

}

bool
LuboPath::in_range( uint ref_val)
{
   if (num() < 2)
      return false;

   int n = _id_set.num();

   uint id = ref_val & 0xffffff00;                 //id portion of ref_val

   double lval = (double)(ref_val & 0x000000ff);   //parameter portion of ref_val

   double len;

   int ind;

   if ( id == _id_set[0] ) {  //if matches the first id, it may also be the end ( loop case )
      //cerr << "in range::matches first id" << endl;
      len = lval * _ffseg_lengths[0] / 256.0;
      if ( ff_len(_id_offsets[0]) < len && len < ff_len(_id_offsets[1]-1) )
         return true;  //easy case

      else if ( n > 1 && _id_set[n-1] == id ) { //wrap case
         if      ( is_closed() && ff_len(_id_offsets[n-1]) < len && len < ff_len(0)   )
            return true; //a closed  loop
         else if ( ff_len(_id_offsets[n-1]) < len && len < ff_len(_id_offsets[n]-1) )
            return true; //clipped portion of a loop
      }
   } else if ( _id_set.valid_index ( ind = _id_set.get_index(id) ) ) {             //id doesn't match the first, so don't worry about the loop case
      //cerr << "multisegment path" << endl;
      //get index of this id
      //(XXX the valid_index is a bravery issue. in range shouldn't be called on this id if the path didn't match)
      len = lval * _ffseg_lengths[ind] / 256.0;
      if ( ff_len(_id_offsets[ind]) < len &&  len < ff_len(_id_offsets[ind+1]-1) )
         return true;      //check in range
   }
   //cerr << " ack!::didn't match an index!" << endl;
   return false;
}

bool
LuboPath::register_vote ( LuboSample& sample, int path_id, CNDCpt& pt, int index)
{
   static bool draw_props = Config::get_var_bool("DRAW_PROPAGATION",false,true);
   static const int MAX_STEPS = Config::get_var_int("MAX_LUBO_STEPS", 6,true);

   CWtransf &xf = (sample._s) ? sample._s->mesh()->xform() : Identity;

   Wpt world_vote_pt;
   sample.get_wpt(world_vote_pt);
   world_vote_pt = xf * /* TFMULTFIX */ world_vote_pt;

   double ndc_dist = (  pt - NDCpt(world_vote_pt) ).length() * VIEW::peek()->ndc2pix_scale();

   //XXX - Was 3, but that's conservative is MAX_STEPS is 6!! (1/17/03)
   //if (ndc_dist > 3)
   if (ndc_dist >= (MAX_STEPS+1)) {
      //   NDCpt nwpt(world_vote_pt);
      if (draw_props)
         IDRefImage::instance()->val(pt) = 0x00ffff00;
      //err_mesg(ERR_LEV_ERROR, "hit_pt in frustum\t%f\t%f\t sample_pt_in_frustum\t%f\t%f", pt[0], pt[1], nwpt[0], nwpt[1] );
      // err_mesg(ERR_LEV_ERROR, "LuboPath::register_vote - Dropping vote with %f pixel propagation distance. (PHIL... WHY?!?!)", ndc_dist);
      return false;
   } else if (draw_props)
      IDRefImage::instance()->val(pt) = 0x00ff0000;

   double s = get_s(index) + (pt-_pts[index]).length();  // arclen along the path
   double conf =1;

   _votes += LuboVote(s , sample._t, conf );

   //necessary info for the votes to carry in order to track properly
   // should be in the constructor once this is finalized
   _votes.last()._path_id     = sample._path_id ;
   _votes.last()._stroke_id   = sample._stroke_id;

   _votes.last()._ndc_dist    = ndc_dist; //(pix, actually... no?)

   double alph = ( s - get_s(index) ) / ( get_s(index+1) - get_s(index) );

   Wpt temp1, temp2;
   get_wpt(index, temp1);
   get_wpt( index+1, temp2);
   Wpt world_path_pt = xf * /* TFMULTFIX */ interp ( temp1, temp2, alph );

   _votes.last()._world_dist = ( world_path_pt - world_vote_pt ).length() / sample._s->mesh()->pix_size();

   return true;

}


/*****************************************************************
* Handy Stuff
*****************************************************************/

static int
arclen_compare_votes(const void* va, const void* vb)
{
   LuboVote* a = (LuboVote*) va;
   LuboVote* b = (LuboVote*) vb;
   return Sign2((a->_s - b->_s));
}


static int
x_compare_samples(const void* va, const void* vb)
{

   XYpt* a = (XYpt*) va;
   XYpt* b = (XYpt*) vb;
   return Sign2(((*a)[0] - (*b)[0]));

}

/*****************************************************************
* VoteGroup
*****************************************************************/

TAGlist* VoteGroup::_vg_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
VoteGroup::tags() const
{
   if (!_vg_tags) {
      _vg_tags = new TAGlist;

      *_vg_tags += new TAG_val<VoteGroup,unsigned int>(
                      "base_id",
                      &VoteGroup::base_id_);
      *_vg_tags += new TAG_val<VoteGroup,double>(
                      "confidence",
                      &VoteGroup::confidence_);
      *_vg_tags += new TAG_val<VoteGroup,double>(
                      "begin",
                      &VoteGroup::begin_);
      *_vg_tags += new TAG_val<VoteGroup,double>(
                      "end",
                      &VoteGroup::end_);

      *_vg_tags += new TAG_meth<VoteGroup>(
                      "status",
                      &VoteGroup::put_status,
                      &VoteGroup::get_status,
                      1);
      *_vg_tags += new TAG_meth<VoteGroup>(
                      "fstatus",
                      &VoteGroup::put_fstatus,
                      &VoteGroup::get_fstatus,
                      1);
      *_vg_tags += new TAG_meth<VoteGroup>(
                      "fit",
                      &VoteGroup::put_fits,
                      &VoteGroup::get_fits,
                      1);
      *_vg_tags += new TAG_meth<VoteGroup>(
                      "vote",
                      &VoteGroup::put_votes,
                      &VoteGroup::get_vote,
                      1);

   }
   return *_vg_tags;

}

/////////////////////////////////////
// get_status()
/////////////////////////////////////
void
VoteGroup::get_status(TAGformat &d)
{
   cerr << "VoteGroup::get_status()\n";

   int status;

   *d >> status;

   _status = (vg_status_t)status;

}

/////////////////////////////////////
// put_status()
/////////////////////////////////////
void
VoteGroup::put_status(TAGformat &d) const
{
   //cerr << "VoteGroup::put_status()\n";

   d.id();
   *d << (int)_status;
   d.end_id();
}


/////////////////////////////////////
// get_fstatus()
/////////////////////////////////////
void
VoteGroup::get_fstatus(TAGformat &d)
{
   cerr << "VoteGroup::get_fstatus()\n";

   int fstatus;

   *d >> fstatus;

   _fstatus = (fit_status_t)fstatus;

}

/////////////////////////////////////
// put_fstatus()
/////////////////////////////////////
void
VoteGroup::put_fstatus(TAGformat &d) const
{
   cerr << "VoteGroup::put_fstatus()\n";

   d.id();
   *d << (int)_fstatus;
   d.end_id();
}



/////////////////////////////////////
// get_vote()
/////////////////////////////////////
void
VoteGroup::get_vote(TAGformat &d)
{
   //cerr << "VoteGroup::get_vote()\n";



   //Grab the class name... should be LuboVote
   str_ptr str;
   *d >> str;

   if ((str != LuboVote::static_name())) {
      // XXX - should throw away stuff from unknown obj
      cerr << "VoteGroup::get_vote() - Not LuboVote: '" << str << "'" << endl;
      return;
   }

   _votes.add(LuboVote());
   _votes.last().decode(*d);


}

/////////////////////////////////////
// put_votes()
/////////////////////////////////////
void
VoteGroup::put_votes(TAGformat &d) const
{
   cerr << "VoteGroup::put_votes()\n";

   int i;

   for (i=0; i<_votes.num(); i++) {
      d.id();
      _votes[i].format(*d);
      d.end_id();
   }

}

/////////////////////////////////////
// get_fits()
/////////////////////////////////////
void
VoteGroup::get_fits(TAGformat &d)
{
   cerr << "VoteGroup::get_fits()\n";

   *d >> _fits;

}

/////////////////////////////////////
// put_fits()
/////////////////////////////////////
void
VoteGroup::put_fits(TAGformat &d) const
{
   cerr << "VoteGroup::put_fits()\n";

   d.id();
   *d << _fits;
   d.end_id();
}


double                        // a more efficient and appropriate version of this function
VoteGroup::get_t( double s )  // ought to replace this basic linear interpolation scheme

{

   int n = _fits.num();
   //   int i = 0;
   int l, m, r;
   // base cases

   if    ( n == 0 )
      return 0;                //if no samples, return zero
   if    ( n == 1 )
      return _fits[0][1];  //if one sample, return one val

   if    ( s < _fits[0][0]    )
      return _fits[0][1];
   if    ( s > _fits[n-1][0]  )
      return _fits[n-1][1];

   //   while (  i < n && s > _fits[i][0] ) i++;
   l = 0;
   r = n-1;
   while ( ( m=(l+r)/2 ) != l )
   {
      if ( s > _fits[m][0] )
         l = m;
      else
         r = m;
   }

   // extrapolation cases    XXX these shouldn't happen if the samples properly bound the group

   // meaty bits
   double w = ( s - _fits[l][0] ) /  ( _fits[r][0] - _fits[l][0] );
   double t = interp ( _fits[l][1], _fits[r][1], w );

   return t;
}





void
VoteGroup::fitsort()
{
   //sort fit samples by arclength position
   _fits.sort(x_compare_samples);
}


void
VoteGroup::sort()
{
   //resort the group by arclength
   _votes.sort(arclen_compare_votes);
   if ( _votes.num() > 0 ) {
      _begin = _votes[0]._s;
      _end   = _votes.last()._s;
   } else {
      _begin=0;
      _end=0;
   }
}


/* end of file zxedge_stroke_texture.C */
