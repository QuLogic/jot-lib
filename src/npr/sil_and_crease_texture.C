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
#include "base_jotapp/base_jotapp.H"
#include "disp/ref_img_client.H"
#include "mesh/lmesh.H"
#include "wnpr/line_pen.H"
#include "wnpr/sil_ui.H"
#include "stroke/edge_stroke.H"
#include "stroke/edge_stroke_pool.H"

//QQQ #include "svd_fit.H"
#include "npr_view.H"
#include "sil_and_crease_texture.H"

using namespace mlib;

extern "C" void HACK_mouse_right_button_up();

/*****************************************************************
 * SilAndCreaseTexture
 *****************************************************************/

/////////////////////////////////////////////////////////////////
// Sorting comparison functions
/////////////////////////////////////////////////////////////////

static int
arclen_compare_votes(const void* va, const void* vb)
{
   LuboVote* a = (LuboVote*) va;
   LuboVote* b = (LuboVote*) vb;
   double diff = a->_s - b->_s;
   return Sign2(diff);
}

static int
double_comp(const void* va, const void* vb)

{
   double* a = (double*) va;
   double* b = (double*) vb;
   double diff = *a - *b;
   return Sign2(diff);
}

static int
double_comp_rev(const void* va, const void* vb)

{
   double* a = (double*) va;
   double* b = (double*) vb;
   double diff = *b - *a;
   return Sign2(diff);
}

static int
coverage_comp(const void* va, const void* vb)
{
   CoverageBoundary* a = (CoverageBoundary*) va;
   CoverageBoundary* b = (CoverageBoundary*) vb;

   if ( a->_s == b->_s ) {
      if      ( a->_type == b->_type )
         return 0;
      else if ( a->_type == COVERAGE_START )
         return -1;
      else
         return 1;
   }

   double diff = a->_s - b->_s;
   return Sign2( diff );
}

static ARRAY<VoteGroup> *votegroups_comp_confidence_groups = NULL;

static int
votegroups_comp_confidence(const void* va, const void* vb)

{
   assert(votegroups_comp_confidence_groups);
   VoteGroup &a = (*votegroups_comp_confidence_groups)[*((int*)va)];
   VoteGroup &b = (*votegroups_comp_confidence_groups)[*((int*)vb)];
   double diff = a.confidence() - b.confidence();
   return Sign2((-diff));
}

static ARRAY<VoteGroup> *votegroups_comp_begin_groups = NULL;

static int
votegroups_comp_begin(const void* va, const void* vb)

{
   assert(votegroups_comp_begin_groups);
   VoteGroup &a = (*votegroups_comp_begin_groups)[*((int*)va)];
   VoteGroup &b = (*votegroups_comp_begin_groups)[*((int*)vb)];
   double diff = a.begin() - b.begin();
   return Sign2((diff));
}



/////////////////////////////////////////////////////////////////
// SilAndCreaseTexture
/////////////////////////////////////////////////////////////////

#define SIL_VIS_WIDTH            8.0f
#define SIL_VIS_ALPHA            1.0f
#define SIL_HIDDEN_WIDTH         8.0f
#define SIL_HIDDEN_ALPHA         0.4f
#define SIL_OCCLUDED_WIDTH       8.0f
#define SIL_OCCLUDED_ALPHA       0.2f

#define SILBACK_VIS_WIDTH        7.0f
#define SILBACK_VIS_ALPHA        1.0f
#define SILBACK_HIDDEN_WIDTH     7.0f
#define SILBACK_HIDDEN_ALPHA     0.4f
#define SILBACK_OCCLUDED_WIDTH   7.0f
#define SILBACK_OCCLUDED_ALPHA   0.2f

#define CREASE_VIS_WIDTH         6.0f
#define CREASE_VIS_ALPHA         1.0f
#define CREASE_HIDDEN_WIDTH      6.0f
#define CREASE_HIDDEN_ALPHA      0.4f
#define CREASE_OCCLUDED_WIDTH    6.0f
#define CREASE_OCCLUDED_ALPHA    0.2f

#define BORDER_VIS_WIDTH         5.0f
#define BORDER_VIS_ALPHA         1.0f
#define BORDER_HIDDEN_WIDTH      5.0f
#define BORDER_HIDDEN_ALPHA      0.4f
#define BORDER_OCCLUDED_WIDTH    5.0f
#define BORDER_OCCLUDED_ALPHA    0.2f

#define SUGLINE_VIS_WIDTH        4.0f
#define SUGLINE_VIS_ALPHA        1.0f
#define SUGLINE_HIDDEN_WIDTH     4.0f
#define SUGLINE_HIDDEN_ALPHA     0.4f
#define SUGLINE_OCCLUDED_WIDTH   4.0f
#define SUGLINE_OCCLUDED_ALPHA   0.2f

#define LINE_TEXTURE             "nprdata/stroke_textures/1D--dark-8.png"
#define LINE_TAPER               40.0f
#define LINE_FLARE               0.2f
#define LINE_FADE                3.0f
#define LINE_AFLARE              0.0f

const float line_widths[] =
   {
      SIL_VIS_WIDTH,       SIL_HIDDEN_WIDTH,       SIL_OCCLUDED_WIDTH,
      SILBACK_VIS_WIDTH,   SILBACK_HIDDEN_WIDTH,   SILBACK_OCCLUDED_WIDTH,
      CREASE_VIS_WIDTH,    CREASE_HIDDEN_WIDTH,    CREASE_OCCLUDED_WIDTH,
      BORDER_VIS_WIDTH,    BORDER_HIDDEN_WIDTH,    BORDER_OCCLUDED_WIDTH,
      SUGLINE_VIS_WIDTH,   SUGLINE_HIDDEN_WIDTH,   SUGLINE_OCCLUDED_WIDTH
   };

const float line_alphas[] =
   {
      SIL_VIS_ALPHA,       SIL_HIDDEN_ALPHA,       SIL_OCCLUDED_ALPHA,
      SILBACK_VIS_ALPHA,   SILBACK_HIDDEN_ALPHA,   SILBACK_OCCLUDED_ALPHA,
      CREASE_VIS_ALPHA,    CREASE_HIDDEN_ALPHA,    CREASE_OCCLUDED_ALPHA,
      BORDER_VIS_ALPHA,    BORDER_HIDDEN_ALPHA,    BORDER_OCCLUDED_ALPHA,
      SUGLINE_VIS_ALPHA,   SUGLINE_HIDDEN_ALPHA,   SUGLINE_OCCLUDED_ALPHA
   };

TAGlist*    SilAndCreaseTexture::_sct_tags   = NULL;

//////////////////////////////////////////////////////
// SilAndCreaseTexture Methods
//////////////////////////////////////////////////////

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
SilAndCreaseTexture::tags() const
{
   if (!_sct_tags) {
      _sct_tags = new TAGlist;
      *_sct_tags += new TAG_val<SilAndCreaseTexture,double>(
                       "crease_max_bend_angle",
                       &SilAndCreaseTexture::crease_max_bend_angle_);
      *_sct_tags += new TAG_val<SilAndCreaseTexture,double>(
                       "crease_thresh",
                       &SilAndCreaseTexture::crease_thresh_);
      *_sct_tags += new TAG_val<SilAndCreaseTexture,float>(
                       "noise_frequency",
                       &SilAndCreaseTexture::noise_frequency_);
      *_sct_tags += new TAG_val<SilAndCreaseTexture,float>(
                       "noise_order",
                       &SilAndCreaseTexture::noise_order_);
      *_sct_tags += new TAG_val<SilAndCreaseTexture,float>(
                       "noise_duration",
                       &SilAndCreaseTexture::noise_duration_);
      *_sct_tags += new TAG_val<SilAndCreaseTexture,bool>(
                       "noise_motion",
                       &SilAndCreaseTexture::noise_motion_);

      *_sct_tags += new TAG_meth<SilAndCreaseTexture>(
                       "sil_pool",
                       &SilAndCreaseTexture::put_sil_pools,
                       &SilAndCreaseTexture::get_sil_pool,  1);
      *_sct_tags += new TAG_meth<SilAndCreaseTexture>(
                       "crease_pool",
                       &SilAndCreaseTexture::put_crease_pools,
                       &SilAndCreaseTexture::get_crease_pool,  1);
   }

   return *_sct_tags;
}

/////////////////////////////////////
// get_sil_pool()
/////////////////////////////////////
void
SilAndCreaseTexture::get_sil_pool(TAGformat &d)
{
   //   cerr << "SilAndCreaseTexture::get_sil_pool()\n";

   str_ptr pool_name, class_name;
   int pool_index;

   *d >> pool_index;
   *d >> pool_name;
   *d >> class_name;

   if ((pool_index < 0) || (pool_index >= SIL_STROKE_POOL_NUM)) {
      cerr << "SilAndCreaseTexture::get_sil_pool() - Invalid sil pool index: "
      << pool_index << "!!!!\n";
      return;
   } else if (_sil_stroke_pools[pool_index]->class_name() != class_name) {
      cerr << "SilAndCreaseTexture::get_sil_pool() - WARNING!! Class name "
      << class_name << " not a " << _sil_stroke_pools[pool_index]->class_name() << "!!!!\n";
   }
   //
   else {
      //cerr << "SilAndCreaseTexture::get_sil_pool() - Loading " << pool_name << " sil pool.\n";
   }
   //
   _sil_stroke_pools[pool_index]->decode(*d);

   assert(_sil_stroke_pools[pool_index]->get_prototype()->get_propagate_offsets());
   assert(_sil_stroke_pools[pool_index]->get_prototype()->get_propagate_patch());
   assert(_sil_stroke_pools[pool_index]->get_prototype()->get_patch() == _patch);
}

/////////////////////////////////////
// put_sil_pools()
/////////////////////////////////////
void
SilAndCreaseTexture::put_sil_pools(TAGformat &d) const
{

   for(int i=0; i<_sil_stroke_pools.num(); i++ ) {
      d.id();
      (*d) << i << sil_stroke_pool((sil_stroke_pool_t)i);
      _sil_stroke_pools[i]->format(*d);
      d.end_id();
   }
}

/////////////////////////////////////
// get_crease_pool()
/////////////////////////////////////
void
SilAndCreaseTexture::get_crease_pool(TAGformat &d)
{
   //   cerr << "SilAndCreaseTexture::get_crease_pool()\n";

   str_ptr str;
   *d >> str;

   DATA_ITEM *data_item = DATA_ITEM::lookup(str);

   if (!data_item) {
      cerr << "SilAndCreaseTexture::get_crease_pool() - Class name "
      << str << " could not be found!!!!!!!" << endl;
      return;
   } else if (str != "EdgeStrokePool") {
      cerr << "SilAndCreaseTexture::get_crease_pool() - Class name "
      << str << " not an EdgeStrokePool!!!!!!" << endl;
      return;
   }
   //
   else {
      //      cerr << "SilAndCreaseTexture::get_crease_pool() - Loaded class name " << str << "\n";
   }
   //
   assert(EdgeStrokePool::mesh() == 0);
   EdgeStrokePool::set_mesh(_patch->mesh());

   EdgeStroke* es = new EdgeStroke();
   assert(es);
   es->set_alpha(1.0);
   es->set_width(2.0);
   es->set_color(COLOR(.3,.3,.3));
   es->set_vis_step_pix_size(_crease_vis_step_size);

   EdgeStrokePool *pool = new EdgeStrokePool(es);
   assert(pool);

   pool->decode(*d);

   EdgeStrokePool::set_mesh(0);

   _crease_stroke_pools += pool;

}

/////////////////////////////////////
// put_crease_pools()
/////////////////////////////////////
void
SilAndCreaseTexture::put_crease_pools(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM,
            "SilAndCreaseTexture::put_crease_pools() - Putting %d pools.",
            _crease_stroke_pools.num());

   for (int i = 0; i < _crease_stroke_pools.num(); i++) {
      d.id();
      _crease_stroke_pools[i]->format(*d);
      d.end_id();
   }
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////

SilAndCreaseTexture::SilAndCreaseTexture(Patch* patch)
      : OGLTexture(patch),
      _inited(false),
      _sil_paths_need_update(true),
      _sil_strokes_need_update(false),
      _crease_strokes_need_update(true),
      _crease_max_bend_angle(60.0),
      _crease_vis_step_size(15.0),
      _crease_thresh(0.5),
      _crease_hide(false),
      _noise_motion(0),
      _noise_frequency(0.0),
      _noise_order(0.0),
      _noise_duration(0.0),
      _noise_time_stamp(0.0)
{
   int i;

   for (i=0; i<SIL_STROKE_POOL_NUM; i++) {
      OutlineStroke *s = new OutlineStroke;
      assert(s);

      s->set_propagate_offsets(true);
      s->set_propagate_patch(true);
      s->set_patch(patch);

      s->set_width(line_widths[i]);
      s->set_alpha(line_alphas[i]);

      s->set_texture(Config::JOT_ROOT() + LINE_TEXTURE);
      s->set_taper(LINE_TAPER);
      s->set_flare(LINE_FLARE);
      s->set_fade(LINE_FADE);
      s->set_aflare(LINE_AFLARE);

      SilStrokePool *sp = new SilStrokePool(s);
      assert(sp);
      _sil_stroke_pools.add(sp);
   }

}
/////////////////////////////////////
// set_patch()
/////////////////////////////////////
void
SilAndCreaseTexture::set_patch(Patch* p)
{
   GTexture::set_patch(p);
   _zx_edge_tex.set_patch(p);

   for (int i=0; i<SIL_STROKE_POOL_NUM; i++) {
      assert(_sil_stroke_pools[i]->get_num_protos() == 1);
      _sil_stroke_pools[i]->get_prototype()->set_patch(p);
      _sil_stroke_pools[i]->set_prototype(_sil_stroke_pools[i]->get_prototype());
   }
}

void 
SilAndCreaseTexture::request_ref_imgs() 
{
   static bool HACK_ID_UPDATE = Config::get_var_bool("HACK_ID_UPDATE",false);
   if (HACK_ID_UPDATE && _sil_paths_need_update) {
      IDRefImage::schedule_update();
   } else {
      _zx_edge_tex.request_ref_imgs();
   }
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

SilAndCreaseTexture::~SilAndCreaseTexture()
{
   for (int i=0; i<_crease_stroke_pools.num(); i++) {
      delete _crease_stroke_pools[i];
   }

   while (_sil_stroke_pools.num() > 0) {
      delete _sil_stroke_pools.pop();
   }

   unobserve();
}

/////////////////////////////////////
// init()
/////////////////////////////////////

void SilAndCreaseTexture::init()
{

   if(_inited) {
      cerr << "SilAndCreaseTexture::init() called twice, returning" << endl;
      return;
   }

   observe();

   // Set up crease strokes.
   // (If crease stroke pools aren't empty, they
   // were probably read if from file, so we leave them.)

   //not just if it's empty

   //if (_crease_stroke_pools.empty()) {
   //   create_crease_strokes();
   //}

   _inited = true;
}

/////////////////////////////////////
// observe()
/////////////////////////////////////

void
SilAndCreaseTexture::observe()
{
   // CAMobs:
   VIEW::peek()->cam()->data()->add_cb(this);

   // BMESHobs:
   subscribe_all_mesh_notifications();

   // XFORMobs:
   every_xform_obs();
}

/////////////////////////////////////
// unobserve
/////////////////////////////////////

void
SilAndCreaseTexture::unobserve()
{
   // CAMobs:
   VIEW::peek()->cam()->data()->rem_cb(this);

   // BMESHobs:
   unsubscribe_all_mesh_notifications();

   // XFORMobs:
   unobs_every_xform();
}

/////////////////////////////////////
// create_crease_strokes
/////////////////////////////////////


//XXX - NEEDS IMPROVEMENT
void
SilAndCreaseTexture::create_crease_strokes()
{
   // We take advantage of the fact that crease strips are
   // already created by the mesh so that crease strip "end"
   // edges are given first, and so that the strips already tend
   // to be in long chains.
   //
   // This allows us to lay crease strokes along edges in the
   // order in which they are given by the crease strips,
   // breaking only where the angle between adjacent edges is too
   // small, with reasonably good results.

   assert(_patch);
   double min_crease_cos = cos(deg2rad(_crease_max_bend_angle));
   EdgeStrip* strip = _patch->cur_creases();

   //to make sure it does not try to draw something that already
   //has something else by checking if SimplexData already has a key.
   //Happens on patch bounderies.
   //(Simon)
   SimplexData* key_val_found;
   SimplexData* next_key_val_found;

   if (strip->empty())
      return;

   Bedge *cur_edge       = 0;
   Bvert *cur_vert_start = 0;
   Bvert *cur_vert_end   = 0;


   EdgeStrokePool* pool = 0;
   _crease_stroke_pools.clear();

   int stroke_started = 0;
   int end_stroke = 0;

   for (int i = 0; i < strip->num(); i++) {

      cur_edge       = strip->edge(i);
      cur_vert_start = strip->vert(i);
      cur_vert_end   = strip->next_vert(i);

      key_val_found = cur_edge->find_data(&EdgeStrokePool::foo);

      // start a new strip if we can
      if (!stroke_started) {
         if (!key_val_found) {
            EdgeStroke* es = new EdgeStroke();  // proto for new pool
            es->set_alpha(1.0);
            es->set_width(2.0);
            es->set_color(COLOR(.3,.3,.3));
            es->set_vis_step_pix_size(_crease_vis_step_size);
            pool = new EdgeStrokePool(es);
            _crease_stroke_pools += pool;
            pool->add_to_strip(cur_vert_start, cur_edge);

            stroke_started = 1;
         }
      }

      if (stroke_started) {

         //if there is next vert (e.g no break)
         //figure out if the angle is too small
         if (!strip->has_break(i) && cur_vert_end != strip->last()) {
            // add the vertex of next edge that is not also in cur
            next_key_val_found = strip->edge(i+1)->find_data(&EdgeStrokePool::foo);
            if (!next_key_val_found) {
               Bvert* next_vert_end = 0;
               next_vert_end = strip->next_vert(i+1);
               assert(next_vert_end);

               double dot = 0.0;

               dot = fabs((cur_vert_start->loc() - cur_vert_end->loc()).normalized() *
                          (next_vert_end->loc()  - cur_vert_end->loc()).normalized());

               if (dot > min_crease_cos) {
                  pool->add_to_strip(cur_vert_end, strip->edge(i+1));
                  cur_vert_start = cur_vert_end;
                  cur_vert_end   = next_vert_end;
               } else {
                  // angle between cur and next is too small, so break stroke
                  end_stroke = 1;
               }
            } else {
               end_stroke = 1;
            }
         } else {
            // there is no next, or cur is not adjacent to next, so break stroke
            end_stroke = 1;
         }

         // angle between cur and next is too small, so break stroke
         if (end_stroke) {
            // indicate break in edge strip
            cur_vert_start = 0;
            cur_vert_end = 0;
            stroke_started = 0;
            // To draw the strip, the pool must contain at least one
            // stroke.  The next call adds a stroke to the pool.
            if (pool)
               pool->get_stroke();
         }
      }
   }
}

/////////////////////////////////////
// read_stream()
/////////////////////////////////////

//XXX - OLD, remove this?
int
SilAndCreaseTexture::read_stream(istream& is, str_list &leftover)
{
   assert(_patch);
   assert(_sil_stroke_pools[SIL_VIS]->get_prototype()->get_patch() == _patch);

   cerr << "SilAndCreaseTexture::read_stream()\n";

   _sil_stroke_pools[SIL_VIS]->read_stream(is);

   assert(_sil_stroke_pools[SIL_VIS]->get_prototype()->get_propagate_offsets());
   assert(_sil_stroke_pools[SIL_VIS]->get_prototype()->get_propagate_patch());
   assert(_sil_stroke_pools[SIL_VIS]->get_prototype()->get_patch() == _patch);

   //XXX - Need?
   _zx_edge_tex.destroy_state();

   //Read the new file flag
   int ntmp,tmp;
   int num_noise_params;
   int num_sil_stroke_pools;
   int num_crease_stroke_pools;

   is >> tmp;

   //If the flag is -1 this is the new format
   if (tmp == -1) {
      is >> ntmp;

      //If the flag is -1 this is the *NEWER* new format
      if (ntmp == -1) {
         is >> num_noise_params;

         assert( (num_noise_params == 4) || (num_noise_params == 6));

         int foo;
         is >> foo;
         _noise_motion = (foo==1);
         is >> _noise_frequency;
         is >> _noise_order;
         is >> _noise_duration;
         if (num_noise_params == 6) {
            is >> _crease_max_bend_angle;
            is >> _crease_thresh;
         }
         is >> num_sil_stroke_pools;
      } else {
         num_sil_stroke_pools = ntmp;
      }

      assert(num_sil_stroke_pools == SIL_STROKE_POOL_NUM);

      //Read all the *remaining* sil stroke pools
      for(int i=1; i<SIL_STROKE_POOL_NUM; i++ ) {
         _sil_stroke_pools[i]->read_stream(is);

         assert(_sil_stroke_pools[i]->get_prototype()->get_propagate_offsets());
         assert(_sil_stroke_pools[i]->get_prototype()->get_propagate_patch());
         assert(_sil_stroke_pools[i]->get_prototype()->get_patch() == _patch);
      }

      //Now read the old crease pool count
      is >> num_crease_stroke_pools;
   }
   //Otherwise the flag is simply the crease pool count
   else {
      //Clear all the *remaining* sil stroke pools
      for(int i=1; i<SIL_STROKE_POOL_NUM; i++ ) {
         assert(_sil_stroke_pools[i]->get_prototype()->get_propagate_offsets());
         assert(_sil_stroke_pools[i]->get_prototype()->get_propagate_patch());
         assert(_sil_stroke_pools[i]->get_prototype()->get_patch() == _patch);
      }

      num_crease_stroke_pools = tmp;
   }

   // read all the crease stroke pools
   assert(EdgeStrokePool::mesh() == 0);
   EdgeStrokePool::set_mesh(_patch->mesh());
   for(int i=0; i<num_crease_stroke_pools; i++ ) {
      EdgeStrokePool *pool = new EdgeStrokePool();
      assert(pool);
      pool->read_stream(is);
      _crease_stroke_pools += pool;
   }
   EdgeStrokePool::set_mesh(0);

   leftover.clear();

   return 1;
}

/////////////////////////////////////
// write_stream()
/////////////////////////////////////

int
SilAndCreaseTexture::write_stream(ostream &os) const
{
   int i;

   cerr << "SilAndCreaseTexture::write_stream()\n";

   os << _begin_tag << class_name() << endl;

   // clear the strokes so we just write the proto
   //   SilAndCreaseTexture* me = (SilAndCreaseTexture*)this;
   _sil_stroke_pools[SIL_VIS]->write_stream(os);

   // write out -1 signifying the new file format
   // (used to be the number of crease stroke pools)
   // then write out the number of sil stroke pools
   os << -1;
   os << endl;

   // write out -1 signifying the *NEWER* new file format!!
   // (used to be the number of sil stroke pools)
   // then write out the number of noise params
   os << -1;
   os << endl;
   os << 6;
   os << endl;

   // write the noise params
   int foo = (_noise_motion)?(1):(0);
   os << foo;
   os << endl;
   os << _noise_frequency;
   os << endl;
   os << _noise_order;
   os << endl;
   os << _noise_duration;
   os << endl;
   os << _crease_max_bend_angle;
   os << endl;
   os << _crease_thresh;
   os << endl;

   os << _sil_stroke_pools.num();

   // write out all the *remaining* sil stroke pools
   for(i=1; i<_sil_stroke_pools.num(); i++ ) {
      _sil_stroke_pools[i]->write_stream(os);
   }


   // write out the number of crease stroke pools
   os << _crease_stroke_pools.num();

   // write out all the crease stroke pools
   for(i=0; i<_crease_stroke_pools.num(); i++ ) {
      _crease_stroke_pools[i]->write_stream(os);
   }

   os << _end_tag << endl;

   return 1;
}

/////////////////////////////////////
// recreate_creases()
/////////////////////////////////////

void
SilAndCreaseTexture::recreate_creases()
{
   if (!_patch)
      return;

   for (int i=0; i<_crease_stroke_pools.num(); i++) {
      delete _crease_stroke_pools[i];
   }

   _crease_stroke_pools.clear();

   BMESH* mesh = _patch->mesh();

   for (int k = mesh->nedges()-1; k>=0; k--)
      mesh->be(k)->compute_crease(_crease_thresh);

   mesh->changed();

   //create_crease_strokes();
}

/////////////////////////////////////
// set_crease_vis_step_size()
/////////////////////////////////////

void
SilAndCreaseTexture::set_crease_vis_step_size(double s)
{
   _crease_vis_step_size = s;

   for (int i=0; i<_crease_stroke_pools.num(); i++) {
      EdgeStrokePool* cur_pool = _crease_stroke_pools[i];

      OutlineStroke* cur_stroke = cur_pool->get_prototype();
      if (cur_stroke) {
         assert(cur_stroke->is_of_type(EdgeStroke::static_name()));
         ((EdgeStroke*)cur_stroke)->set_vis_step_pix_size(s);
      }

      for (int l=0; l<cur_pool->num_strokes(); l++) {
         OutlineStroke* cur_stroke = cur_pool->stroke_at(l);
         assert(cur_stroke->is_of_type(EdgeStroke::static_name()));
         ((EdgeStroke*)cur_stroke)->set_vis_step_pix_size(s);
      }
   }
}

/////////////////////////////////////
// draw()
/////////////////////////////////////

int
SilAndCreaseTexture::draw(CVIEWptr& v)
{
   if (!_inited)
      init();

   // XXX - is the following used?
   //
   // If needed, force crease strokes to be rebuilt before
   // drawing, by marking them 'dirty'.
   // (This is necessary if camera or scene has changed.)
   /*
   if (_crease_strokes_need_update) {
      for (int i=0; i<_crease_stroke_pools.num(); i++) {
         _crease_stroke_pools[i]->mark_dirty();
      }
      _crease_strokes_need_update = false;
   }
     
   if (!_crease_hide) {
      // draw the creases
      for (int j=0; j<_crease_stroke_pools.num(); j++) {
         _crease_stroke_pools[j]->draw_flat(v);
      }
   }
   */

   update_squiggle_vision();

   //Branch off to update the silhouette paths
   //if necessary. because of view change,
   // or the always update flag...
   update_sil_paths(v);

   //Create strokes from groups if necessary
   //(because paths changed in update_sil_paths)
   generate_strokes_from_groups();

   //for (int k=0; k<SIL_STROKE_POOL_NUM; k++) _sil_stroke_pools[k]->draw_flat(v);

   _sil_stroke_pools[SILBACK_OCCLUDED]->draw_flat(v);
   _sil_stroke_pools[SUGLINE_OCCLUDED]->draw_flat(v);
   _sil_stroke_pools[BORDER_OCCLUDED]->draw_flat(v);
   _sil_stroke_pools[CREASE_OCCLUDED]->draw_flat(v);
   _sil_stroke_pools[SIL_OCCLUDED]->draw_flat(v);
   _sil_stroke_pools[SILBACK_HIDDEN]->draw_flat(v);
   _sil_stroke_pools[SUGLINE_HIDDEN]->draw_flat(v);
   _sil_stroke_pools[BORDER_HIDDEN]->draw_flat(v);
   _sil_stroke_pools[CREASE_HIDDEN]->draw_flat(v);
   _sil_stroke_pools[SIL_HIDDEN]->draw_flat(v);
   _sil_stroke_pools[SILBACK_VIS]->draw_flat(v);
   _sil_stroke_pools[SUGLINE_VIS]->draw_flat(v);
   _sil_stroke_pools[BORDER_VIS]->draw_flat(v);
   _sil_stroke_pools[CREASE_VIS]->draw_flat(v);
   _sil_stroke_pools[SIL_VIS]->draw_flat(v);

   return 0;
}

/////////////////////////////////////
// update_squiggle_vision()
/////////////////////////////////////

void
SilAndCreaseTexture::update_squiggle_vision()
{

   static bool HACK_NOISE = Config::get_var_bool("HACK_NOISE",false,true);

   if (_noise_frequency > 0) {
      if (!_sil_paths_need_update && _noise_motion) {
         //Don't do squiggle vision!
      } else {

         double curr_time = VIEW::peek()->frame_time();

         if (curr_time > _noise_time_stamp) {
            double time_to_kill = curr_time - _noise_time_stamp;
            double period = 1.0/_noise_frequency;

            //If its been ages since we've been here, then time_to_kill
            //can be big.  If its alot, then there's no need to keep
            //track of missed changes.

            int max_num_protos = 0;
            int k=0;
            for (k=0; k<SIL_STROKE_POOL_NUM; k++)
               if (_sil_stroke_pools[k]->get_num_protos()>max_num_protos)
                  max_num_protos = _sil_stroke_pools[k]->get_num_protos();

            if (((int)ceil(time_to_kill/period)) > 2*max_num_protos)
               time_to_kill = 0.0;

            //Do the flips
            for (k=0; k<SIL_STROKE_POOL_NUM; k++) {
               int num_protos = _sil_stroke_pools[k]->get_num_protos();
               if (num_protos>1) {
                  int curr_proto = _sil_stroke_pools[k]->get_draw_proto();
                  int next_proto;

                  if (drand48() >= _noise_order)
                     next_proto = curr_proto + 1;
                  else
                     next_proto = (int)((double) num_protos * drand48());

                  next_proto = next_proto % num_protos;

                  _sil_stroke_pools[k]->set_draw_proto(next_proto);
                  if (HACK_NOISE)
                     _sil_stroke_pools[k]->get_active_prototype()->set_offset_phase((float)drand48());
               }
            }

            double time_till_flip = period * ( 1.0 + _noise_duration * ( drand48() - 0.5 ));
            while (time_till_flip < time_to_kill) {
               //Do the flips
               for (k=0; k<SIL_STROKE_POOL_NUM; k++) {
                  int num_protos = _sil_stroke_pools[k]->get_num_protos();
                  if (num_protos>1) {
                     int curr_proto = _sil_stroke_pools[k]->get_draw_proto();
                     int next_proto;

                     if (drand48() >= _noise_order)
                        next_proto = curr_proto + 1;
                     else
                        next_proto = (int)((double) num_protos * drand48());

                     next_proto = next_proto % num_protos;

                     _sil_stroke_pools[k]->set_draw_proto(next_proto);
                     if (HACK_NOISE)
                        _sil_stroke_pools[k]->get_active_prototype()->set_offset_phase((float)drand48());
                  }
               }

               time_to_kill -= time_till_flip;
               time_till_flip = period * ( 1.0 + _noise_duration * ( drand48() - 0.5 ));
            }
            _noise_time_stamp = curr_time + (time_till_flip - time_to_kill);
         } else {
            //Still waitin'!
         }
      }
   } else {
      for (int k=0; k<SIL_STROKE_POOL_NUM; k++)
         _sil_stroke_pools[k]->set_draw_proto(0);
   }

}

/////////////////////////////////////
// mark_all_dirty()
/////////////////////////////////////

void
SilAndCreaseTexture::mark_all_dirty()
{
   mark_sils_dirty();
   mark_creases_dirty();
}

/////////////////////////////////////
// mark_sils_dirty()
/////////////////////////////////////

void
SilAndCreaseTexture::mark_sils_dirty()
{
   _sil_paths_need_update = true;
}

/////////////////////////////////////
// mark_creases_dirty()
/////////////////////////////////////

void
SilAndCreaseTexture::mark_creases_dirty()
{
   _crease_strokes_need_update = true;
}


/////////////////////////////////////
// update_sil_paths()
/////////////////////////////////////

void
SilAndCreaseTexture::update_sil_paths(CVIEWptr &v)
{
   static bool DEBUG_LUBO = Config::get_var_bool("DEBUG_LUBO",false,true);

   bool ALWAYS_UPDATE = SilUI::always_update(v);

   if ( ! ( ALWAYS_UPDATE || _sil_paths_need_update || DEBUG_LUBO) )
      return;

   //Create paths with samples
   _zx_edge_tex.create_paths();

   //Cache some values into the paths
   //(Such as pix_to_ndc scale, etc.)
   cache_per_path_values();

   //Create fitted groups of samples
   generate_sil_groups();

   //Create new samples for next frame
   _zx_edge_tex.regen_group_samples();

   _sil_paths_need_update = false;

   _sil_strokes_need_update = true;
}


/////////////////////////////////////
// cache_per_path_values()
/////////////////////////////////////

void
SilAndCreaseTexture::cache_per_path_values()
{
   int k, num;

   LuboPathList& paths = _zx_edge_tex.paths();
   num = paths.num();

   ARRAY<double> offset_pix_lens(SIL_STROKE_POOL_NUM);
   ARRAY<double> stretch_factors(SIL_STROKE_POOL_NUM);

   double pix_to_ndc_scale = _zx_edge_tex.pix_to_ndc_scale();
   double cur_size = pix_to_ndc_scale * _patch->pix_size();

   for (k=0; k<SIL_STROKE_POOL_NUM; k++) {

      OutlineStroke *s = _sil_stroke_pools[k]->get_active_prototype();
      assert(s);

      // If no offsets, use the 'angle' as the pixel period of
      // the stylizations (in this case, a texture map.)
      // Always push params around, even if period=0 (by using 1)

      if (s->get_offsets())
         offset_pix_lens += s->get_offsets()->get_pix_len();
      else
         offset_pix_lens += max(1.0f,(s->get_angle()));

      // First time round, might need to set this...

      if (s->get_original_mesh_size() == 0.0) {
         assert(_sil_stroke_pools[k]->get_edit_proto() == 0);
         assert(_sil_stroke_pools[k]->get_num_protos() == 1);
         s->set_original_mesh_size(cur_size);
         _sil_stroke_pools[k]->set_prototype(s);
      }

      if ((_sil_stroke_pools[k]->get_coher_global())?
          (SilUI::sigma_one(VIEW::peek())):
          (_sil_stroke_pools[k]->get_coher_sigma_one()))
         stretch_factors += 1.0;
      else
         stretch_factors += (cur_size < epsAbsMath()) ? 1.0 : (s->get_original_mesh_size() / cur_size);

   }


   for (k=0; k<num; k++) {
      LuboPath *p = paths[k];

      p->set_pix_to_ndc_scale(pix_to_ndc_scale);

      int pool_index = type_and_vis_to_sil_stroke_pool(p->type(), p->vis());

      p->line_type() = pool_index;
      p->set_offset_pix_len(offset_pix_lens[pool_index]);
      p->set_stretch(stretch_factors[pool_index]);

   }

}

/////////////////////////////////////
// generate_sil_strokes()
/////////////////////////////////////

void
SilAndCreaseTexture::generate_sil_groups()
{
   static double MIN_PATH_PIX = Config::get_var_dbl("MIN_PATH_PIX",2.0,true);

   int i;

   const int   global_fit_type =   SilUI::fit_type(VIEW::peek());
   const int   global_cover_type = SilUI::cover_type(VIEW::peek());
   const bool  global_do_heal =   (global_cover_type == SIL_COVER_TRIMMED) &&
                                  (SilUI::weight_heal(VIEW::peek()) > 0.0);

   LuboPathList &paths = _zx_edge_tex.paths();

   double min_ndc = (paths.num())?(paths[0]->pix_to_ndc_scale() * MIN_PATH_PIX):(0);

   //Build and fit groups
   for (i=0; i < paths.num(); i++) {
      LuboPath* p = paths[i];

      if (p->length() < min_ndc )
         continue;

      bool  do_heal;
      int   fit_type, cover_type;

      SilStrokePool* pool = _sil_stroke_pools[p->line_type()];

      if (pool->get_coher_global()) {
         fit_type = global_fit_type;
         cover_type = global_cover_type;
         do_heal = global_do_heal;
      } else {
         fit_type = pool->get_coher_fit_type();
         cover_type = pool->get_coher_cover_type();
         do_heal = (cover_type == SIL_COVER_TRIMMED) && (pool->get_coher_wh() > 0.0);
      }

      fit_ptr     fit_func = fit_function(fit_type);
      cover_ptr   cover_func = cover_function(cover_type);

      build_groups(p);

      cull_small_groups(p);
      cull_short_groups(p);

      //split_looped_groups(p);

      split_gapped_groups(p);
      split_large_delta_groups(p);

      cull_backwards_groups(p);

      if (fit_type == SIL_FIT_INTERPOLATE)
         split_all_backtracking_groups(p);

      cull_small_groups(p);
      cull_short_groups(p);

      cull_sparse_groups(p);

      if (fit_type != SIL_FIT_INTERPOLATE) {
         fit_initial_groups(p, fit_func);

         if (fit_type == SIL_FIT_OPTIMIZE)
            cull_bad_fit_groups(p);

         coverage_manage_groups(p, cover_func);

         cull_outliers_in_groups(p);

         fit_final_groups(p, fit_func);

         if (do_heal)
            heal_groups(p, fit_func);

         refit_backward_fit_groups(p);

      } else {
         coverage_manage_groups(p, cover_func);

         fit_final_groups(p, fit_func);
      }

   }

   // Notify that groups have been built and fitted
   _zx_edge_tex.regen_group_notify();

}

/////////////////////////////////////
// build_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::build_groups ( LuboPath * p )
{
   int i,j,n;
   uint last_id=0, last_ind=0;

   ARRAY<LuboVote>& votes = p->votes();
   ARRAY<VoteGroup>& groups = p->groups();

   votes.sort(arclen_compare_votes);

   n = votes.num();

   groups.clear();
   //cerr << "Num Votes: " << n << "\n";
   for ( i=0 ; i < n ; i++ ) {
      bool added = false;
      //Consecutive votes often fall in the same group.
      //We accelerate this case explicitly...
      if ( last_id == votes[i]._stroke_id ) {
         groups[last_ind].add(votes[i]);
         added = true;
      } else {
         for (j=groups.num()-1; (j>=0) && (!added); j--) {
            if ( groups[j].id() == votes[i]._stroke_id ) {
               groups[j].add(votes[i]);
               last_id = groups[j].id();
               last_ind = j;
               added = true;
            }
         }
      }

      if ( !added ) {
         groups += VoteGroup ( votes[i]._stroke_id, p );
         last_id = votes[i]._stroke_id;
         last_ind = groups.num()-1;
         groups[last_ind].add( votes[i] );
      }

   }

   //Sort (also sets begin/end), and set unique id's
   for  (i = 0 ; i < groups.num(); i++ ) {
      groups[i].sort();
      groups[i].id() = p->gen_stroke_id();
   }

}

/////////////////////////////////////
// cull_small_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::cull_small_groups(LuboPath* p)
{
   static int GLOBAL_MIN_VOTES_PER_GROUP = Config::get_var_int("MIN_VOTES_PER_GROUP", 2,true);

   SilStrokePool* pool = _sil_stroke_pools[p->line_type()];
   int MIN_VOTES_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_VOTES_PER_GROUP):(pool->get_coher_mv()));

   ARRAY<VoteGroup>& groups = p->groups();
   int i, n = groups.num();

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;
      if (g.num() < MIN_VOTES_PER_GROUP)
         g.status() = VoteGroup::VOTE_GROUP_LOW_VOTES;
   }
}

/////////////////////////////////////
// cull_short_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::cull_short_groups(LuboPath* p)
{
   static double GLOBAL_MIN_PIX_PER_GROUP = Config::get_var_dbl("MIN_PIX_PER_GROUP", 5.0,true);
   static double GLOBAL_MIN_FRAC_PER_GROUP = Config::get_var_dbl("MIN_FRAC_PER_GROUP", 0.05,true);

   SilStrokePool* pool = _sil_stroke_pools[p->line_type()];
   double MIN_PIX_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_PIX_PER_GROUP):(pool->get_coher_mp()));
   double MIN_FRAC_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_FRAC_PER_GROUP):(pool->get_coher_m5()));

   double min_length = min( MIN_PIX_PER_GROUP * p->pix_to_ndc_scale(),
                            MIN_FRAC_PER_GROUP * p->length());

   ARRAY<VoteGroup>& groups = p->groups();
   int i, n = groups.num();

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;
      if ((g.end()- g.begin()) < min_length)
         g.status() = VoteGroup::VOTE_GROUP_LOW_LENGTH;
   }
}

/////////////////////////////////////
// cull_sparse_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::cull_sparse_groups(LuboPath* p)
{
   static double SPARSE_FACTOR = Config::get_var_dbl("SPARSE_FACTOR", 3.0,true);

   ARRAY<VoteGroup>& groups = p->groups();
   int i, n = groups.num();

   double sample_spacing = _zx_edge_tex.get_sampling_dist() * p->pix_to_ndc_scale();

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];
      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (g.num()  < 2) )
         continue;

      if ( ((g.end()- g.begin()) / (g.num()-1)) > SPARSE_FACTOR * sample_spacing)
         g.status() = VoteGroup::VOTE_GROUP_BAD_DENSITY;
   }
}

/////////////////////////////////////
// split_looped_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::split_looped_groups(LuboPath* p)
{

   if (!p->is_closed())
      return;

   const double GAP_FRACTION = 0.75;
   const double GAP_FACTOR = 3.0;

   const double DELTA_FRACTION = 0.75;
   const double DELTA_FACTOR = 4.0;


   ARRAY<VoteGroup>& groups = p->groups();
   ARRAY<double> gaps, sorted_gaps, deltas, sorted_deltas;

   int j, i, nv, n = groups.num(), j0, j1;
   double gap_thresh, delta_thresh, cnt;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      gaps.clear();
      sorted_gaps.clear();
      deltas.clear();
      sorted_deltas.clear();

      cnt = 0.0;
      for (j=0; j<nv-1; j++) {
         gaps += (g.vote(j+1)._s - g.vote(j)._s);
         double d = (g.vote(j+1)._t - g.vote(j)._t);
         deltas += d;
         cnt += ((d<0.0)?(-1.0):(+1.0));
      }

      // Bail out if < 75% of the
      // deltas have the same sign
      if (fabs(cnt)/(nv-1.0) < 0.5) {
         err_mesg(ERR_LEV_INFO, "SilAndCreaseTexture::split_looped_groups() - Noisy data...");
         continue;
      }

      sorted_gaps += gaps;
      sorted_gaps.sort(double_comp);

      sorted_deltas += deltas;
      if (cnt<0.0)
         sorted_deltas.sort(double_comp_rev);
      else
         sorted_deltas.sort(double_comp);

      // XXX - Hacky, but it might work:
      // Loop crossing found if there's
      // any reverse delta > delta_thresh
      // AND votes exists within gap_thresh
      // of boths ends of the path.
      // Toss votes between the first and
      // last large reverse delta and join
      // the two groups, shifting one of them.

      gap_thresh   = GAP_FACTOR *   sorted_gaps  [(int)(  GAP_FRACTION * (nv-2.0))];
      delta_thresh = -1.0 * DELTA_FACTOR * sorted_deltas[(int)(DELTA_FRACTION * (nv-2.0))];

      if ( sorted_deltas[0]/delta_thresh > 1.0) {
         //cerr << "SilAndCreaseTexture::split_looped_groups() - Splitting...\n";

         j = 0;
         j0 = nv;
         j1 = -1;
         while (sorted_deltas[j]/delta_thresh > 1.0) {
            int ind = deltas.get_index(sorted_deltas[j]);
            if (ind < j0)
               j0 = ind;
            if (ind > j1)
               j1 = ind;
            j++;
            assert(j < (nv-1));
         }


         if ( j0 != j1 ) {
            cerr << "SilAndCreaseTexture::split_looped_groups() - Hey! I found >1 large delta...\n";
         }


         if ( g.votes().first()._s > gap_thresh ) {
            cerr << "SilAndCreaseTexture::split_looped_groups() - No votes near start of path...\n";
            continue;
         }
         if ( g.votes().last()._s < (p->length()-gap_thresh) ) {
            cerr << "SilAndCreaseTexture::split_looped_groups() - No votes near end of path...\n";
            continue;
         }


         // If we make it past these checks, then
         // we break the group into [0,j0] [j1+1,num-1]
         // One day we'll try rejoining them
         // into a continguous group...

         groups += VoteGroup ( p->gen_stroke_id(), p );
         groups += VoteGroup ( p->gen_stroke_id(), p );

         VoteGroup& ng1 = groups[groups.num()-2];
         VoteGroup& ng2 = groups[groups.num()-1];

         VoteGroup& g = groups[i];

         g.status() = VoteGroup::VOTE_GROUP_SPLIT_LOOP;

         for (j=0; j<=j0; j++)
            ng1.add(g.vote(j));
         ng1.begin() = g.vote(0)._s;
         ng1.end() = g.vote(j0)._s;


         for (j=j1+1; j<nv; j++)
            ng2.add(g.vote(j));
         ng2.begin() = g.vote(j1+1)._s;
         ng2.end() = g.vote(nv-1)._s;
      }
   }
}

/////////////////////////////////////
// split_large_delta_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::split_large_delta_groups(LuboPath* p)
{
   const double DELTA_FRACTION = 0.75;
   const double DELTA_FORWARD_FACTOR = 6.0;
   const double DELTA_REVERSE_FACTOR = 3.0;

   ARRAY<VoteGroup>& groups = p->groups();
   ARRAY<double> deltas, sorted_deltas;

   int k, j, i, j0, nv, n = groups.num();
   double cnt, delta_forward_thresh, delta_reverse_thresh;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      deltas.clear();
      sorted_deltas.clear();

      cnt = 0.0;
      for (j=0; j<nv-1; j++) {
         double d = (g.vote(j+1)._t - g.vote(j)._t);
         deltas += d;
         cnt += ((d<0.0)?(-1.0):(+1.0));
      }

      // Bail out if < 75% of the
      // deltas have the same sign
      if (fabs(cnt)/(nv-1.0) < 0.5) {
         cerr << "SilAndCreaseTexture::split_large_delta_groups() - Noisy data...\n";
         continue;
      }

      sorted_deltas += deltas;
      if (cnt<0.0)
         sorted_deltas.sort(double_comp_rev);
      else
         sorted_deltas.sort(double_comp);

      delta_reverse_thresh = -1.0 * DELTA_REVERSE_FACTOR * sorted_deltas[(int)(DELTA_FRACTION * (nv-2.0))];
      delta_forward_thresh =  1.0 * DELTA_FORWARD_FACTOR * sorted_deltas[(int)(DELTA_FRACTION * (nv-2.0))];

      //if (sorted_deltas[0]/delta_thresh > 1.0)
      //   cerr << "SilAndCreaseTexture::split_large_delta_groups() - Splitting...\n";

      j0 = 0;
      for (j=0; j<nv-1; j++) {
         if ( (deltas[j]/delta_reverse_thresh > 1.0) || (deltas[j]/delta_forward_thresh > 1.0) ) {
            groups += VoteGroup ( p->gen_stroke_id(), p );
            VoteGroup& ng = groups.last();
            VoteGroup& g  = groups[i];

            for (k=j0; k<=j; k++)
               ng.add(g.vote(k));
            ng.begin() = g.vote(j0)._s;
            ng.end() = g.vote(j)._s;

            j0 = j+1;
         }
      }

      if (j0 != 0) {
         groups += VoteGroup ( p->gen_stroke_id(), p );
         VoteGroup& ng = groups.last();
         VoteGroup& g  = groups[i];
         g.status() = VoteGroup::VOTE_GROUP_SPLIT_LARGE_DELTA;

         for (k=j0; k<=j; k++)
            ng.add(g.vote(k));
         ng.begin() = g.vote(j0)._s;
         ng.end() = g.vote(j)._s;
      }
   }
}

/////////////////////////////////////
// split_gapped_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::split_gapped_groups(LuboPath* p)
{
   const double THRESH_FRACTION = 0.75;
   const double THRESH_FACTOR = 4.0;

   ARRAY<VoteGroup>& groups = p->groups();
   ARRAY<double> gaps, sorted_gaps;

   int k, j, i, j0, nv, n = groups.num();
   double thresh;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      gaps.clear();
      sorted_gaps.clear();

      for (j=0; j<nv-1; j++)
         gaps += (g.vote(j+1)._s - g.vote(j)._s);

      sorted_gaps += gaps;
      sorted_gaps.sort(double_comp);

      thresh = THRESH_FACTOR * sorted_gaps[(int)(THRESH_FRACTION * (nv-2.0))];

      j0 = 0;
      for (j=0; j<nv-1; j++) {

         if (gaps[j] > thresh) {

            groups += VoteGroup ( p->gen_stroke_id(), p );
            VoteGroup& ng = groups.last();
            VoteGroup& g  = groups[i];

            for (k=j0; k<=j; k++)
               ng.add(g.vote(k));
            ng.begin() = g.vote(j0)._s;
            ng.end() = g.vote(j)._s;

            j0 = j+1;
         }
      }

      if (j0 != 0) {
         groups += VoteGroup ( p->gen_stroke_id(), p );
         VoteGroup& ng = groups.last();
         VoteGroup& g  = groups[i];
         g.status() = VoteGroup::VOTE_GROUP_SPLIT_GAP;

         for (k=j0; k<=j; k++)
            ng.add(g.vote(k));
         ng.begin() = g.vote(j0)._s;
         ng.end() = g.vote(j)._s;
      }
   }
}

/////////////////////////////////////
// cull_backwards_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::cull_backwards_groups(LuboPath* p)
{

   const double BACK_THRESH = 0.0;

   ARRAY<VoteGroup>& groups = p->groups();

   int j, i, nv, n = groups.num();
   double cnt;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 2) )
         continue;

      cnt = 0.0;
      for (j=0; j<nv-1; j++) {
         cnt += ((g.vote(j+1)._t < g.vote(j)._t)?(-1.0):(+1.0));
      }

      if (cnt/(nv-1.0) <= BACK_THRESH) {
         g.status() = VoteGroup::VOTE_GROUP_CULL_BACKWARDS;
      }
   }

}

/////////////////////////////////////
// split_all_backtracking_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::split_all_backtracking_groups(LuboPath* p)
{
   ARRAY<VoteGroup>& groups = p->groups();
   ARRAY<double> deltas;

   int k, j, i, j0, nv, n = groups.num();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 2) )
         continue;

      deltas.clear();

      for (j=0; j<nv-1; j++)
         deltas += (g.vote(j+1)._t - g.vote(j)._t);

      j0 = 0;
      for (j=0; j<nv-1; j++) {
         if (deltas[j] < 0.0) {
            groups += VoteGroup ( p->gen_stroke_id(), p );
            VoteGroup& ng = groups.last();

            VoteGroup& g = groups[i];

            for (k=j0; k<=j; k++)
               ng.add(g.vote(k));
            ng.begin() = g.vote(j0)._s;
            ng.end() = g.vote(j)._s;

            j0 = j+1;
         }
      }

      if (j0 != 0) {
         groups += VoteGroup ( p->gen_stroke_id(), p );
         VoteGroup& ng = groups.last();

         VoteGroup& g = groups[i];

         g.status() = VoteGroup::VOTE_GROUP_SPLIT_ALL_BACKTRACK;

         for (k=j0; k<=j; k++)
            ng.add(g.vote(k));
         ng.begin() = g.vote(j0)._s;
         ng.end() = g.vote(j)._s;
      }
   }
}
/////////////////////////////////////
// fit_intial_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::fit_initial_groups(
   LuboPath* p,
   void (SilAndCreaseTexture::*fit_func)(VoteGroup&,double))
{
   // Note: p->stretch() is the inverse of the expected ratio

   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   ARRAY<VoteGroup>& groups = p->groups();

   int i, n = groups.num();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      assert(g.fstatus() == VoteGroup::FIT_NONE);

      if ( g.num() == 0 )
         arclength_fit(g,freq);
      else
         (*this.*fit_func)(g,freq);
   }

}
/////////////////////////////////////
// cull_bad_fit_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::cull_bad_fit_groups(LuboPath* p)
{
   ARRAY<VoteGroup>& groups = p->groups();

   int i, n = groups.num();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      assert(g.fstatus() != VoteGroup::FIT_NONE);

      if (g.fstatus() == VoteGroup::FIT_BACKWARDS)
         g.status() = VoteGroup::VOTE_GROUP_FIT_BACKWARDS;
      else
         //XXX - Not thinkning about any other badness... yet...
         assert(g.fstatus() == VoteGroup::FIT_GOOD);
   }

}
/////////////////////////////////////
// coverage_manage_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::coverage_manage_groups(LuboPath* p, void (SilAndCreaseTexture::*cover_func)(LuboPath*))
{

   //Do common stuff...

   (*this.*cover_func)(p);

}
/////////////////////////////////////
// cull_outliers_in_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::cull_outliers_in_groups(LuboPath* p)
{

   const double THRESH_FRACTION = 0.75;
   const double THRESH_FACTOR = 20.0;

   ARRAY<VoteGroup>& groups = p->groups();
   ARRAY<double> errs, sorted_errs;

   int j, i, cnt, nv, n = groups.num();
   double thresh;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      //XXX - Presently allow FIT_OLD since changing begin/end
      //invalidates the fit during coverage... however the
      //fits good enough for outlier analysis
      assert(g.fstatus() != VoteGroup::FIT_NONE);

      errs.clear();
      sorted_errs.clear();

      for (j=0; j<nv; j++) {
         double err = (g.vote(j)._t - g.get_t(g.vote(j)._s));
         errs += fabs(err);
      }

      sorted_errs += errs;
      sorted_errs.sort(double_comp);

      thresh = THRESH_FACTOR * sorted_errs[(int)(THRESH_FRACTION * (nv-2.0))];

      cnt = 0;
      for (j=0; j<nv; j++) {
         if (errs[j] > thresh) {
            g.vote(j)._status = LuboVote::VOTE_OUTLIER;
            cnt++;
         }
      }

      if (cnt) {
         g.fstatus() = VoteGroup::FIT_OLD;
      }
   }

}


/////////////////////////////////////
// fit_final_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::fit_final_groups(
   LuboPath* p,
   void (SilAndCreaseTexture::*fit_func)(VoteGroup&,double))
{
   // Note: p->stretch() is the inverse of the expected ratio

   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   ARRAY<VoteGroup>& groups = p->groups();

   int i, n = groups.num();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      if (g.fstatus() != VoteGroup::FIT_GOOD) {
         g.fstatus() = VoteGroup::FIT_NONE;
         g.fits().clear();

         if ( g.num() == 0 )
            arclength_fit(g,freq);
         else
            (*this.*fit_func)(g,freq);
      }
   }

}
/////////////////////////////////////
// heal_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::heal_groups(
   LuboPath* p,
   void (SilAndCreaseTexture::*fit_func)(VoteGroup&,double))
{

   static double GLOBAL_HEAL_JOIN_PIX_THRESH = Config::get_var_dbl("HEAL_JOIN_PIX_THRESH",3.0,true);
   static double GLOBAL_HEAL_DRAG_PIX_THRESH = Config::get_var_dbl("HEAL_DRAG_PIX_THRESH",15.0,true);

   SilStrokePool* pool = _sil_stroke_pools[p->line_type()];
   double HEAL_JOIN_PIX_THRESH = ((pool->get_coher_global())?(GLOBAL_HEAL_JOIN_PIX_THRESH):(pool->get_coher_hj()));
   double HEAL_DRAG_PIX_THRESH = ((pool->get_coher_global())?(GLOBAL_HEAL_DRAG_PIX_THRESH):(pool->get_coher_ht()));

   double pix_to_ndc = p->pix_to_ndc_scale();
   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   ARRAY<VoteGroup>& groups = p->groups();
   ARRAY<int> final_groups;

   int pi, pi_1, i, i0, j, n = groups.num();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      final_groups += i;
   }

   if (final_groups.num()<2)
      return;

   //We're assuming that the groups provide total coverage
   //of the path, and abutt one another perfectly...
   votegroups_comp_begin_groups = &groups;
   final_groups.sort(votegroups_comp_begin);

   i0 = -1;
   pi = 0;

   for (i=0; i < final_groups.num(); i++) {
      VoteGroup &gi   = groups[final_groups[i]];

      bool attach = false;

      if (i<(final_groups.num()-1)) {
         VoteGroup &gi_1 = groups[final_groups[i+1]];
         assert(gi.end() == gi_1.begin());

         double ti   =   gi.get_t(  gi.end()  );
         double ti_1 = gi_1.get_t(gi_1.begin());

         double floori   = floor(ti);
         double floori_1 = floor(ti_1);

         double d = (ti_1 - floori_1) - (ti - floori);

         if      (d >  0.5) { floori_1++; d -= 1.0; } else if (d < -0.5) { floori_1--; d += 1.0; }

         //Pixel difference in phase
         double dpix = fabs((d/freq)/pix_to_ndc);



         if (dpix < HEAL_JOIN_PIX_THRESH) {
            attach = true;
         } else if (dpix < HEAL_DRAG_PIX_THRESH) {
            //Add healing verts
            gi.votes() += LuboVote();
            gi_1.votes() += LuboVote();

            LuboVote  &vi   =   gi.votes().first();
            LuboVote &nvi   =   gi.votes().last();
            LuboVote  &vi_1 = gi_1.votes().first();
            LuboVote &nvi_1 = gi_1.votes().last();

            nvi._path_id   = vi._path_id;
            nvi._stroke_id = vi._stroke_id;
            nvi._s =         gi.end();
            nvi._t =         ti + d/2.0;
            nvi._status =    LuboVote::VOTE_HEALER;

            nvi_1._path_id   = vi_1._path_id;
            nvi_1._stroke_id = vi_1._stroke_id;
            nvi_1._s =         gi_1.begin();
            nvi_1._t =         ti_1 - d/2.0;
            nvi_1._status = LuboVote::VOTE_HEALER;

            //Invalidate fits
            gi.fstatus() = VoteGroup::FIT_OLD;
            gi.votes().sort(arclen_compare_votes);

            gi_1.fstatus() = VoteGroup::FIT_OLD;
            gi_1.votes().sort(arclen_compare_votes);

         } else {}

         pi_1 = pi + (int)(floori - floori_1);
      }

      //If we can attach with the next group
      if (attach) {
         //Record this as the starting group if
         //we're not already in a chain
         if (i0 == -1) {
            i0 = i;

            //Make a new VoteGroup
            groups += VoteGroup ( p->gen_stroke_id(), p );
            VoteGroup &ng   = groups.last();
            VoteGroup &gi   = groups[final_groups[i]];

            //Add the votes (p0 should be 0)
            assert (pi == 0);
            ng.votes() += gi.votes();
            ng.begin() = gi.begin();

         }
         //Otherwise, just add the votes to
         //the current chain
         else {
            //Add votes from gi (pi = ?)
            VoteGroup& ng = groups.last();
            n = gi.num();
            for (j=0; j<n; j++) { ng.add(gi.vote(j)); ng.votes().last()._t += pi; }
         }
         pi = pi_1;
      }
      //If we can't attach with next group...
      else {
         //Then add the votes if necessary
         if (i0 != -1) {
            //Add votes from gi (pi = ?)
            VoteGroup& ng = groups.last();
            n = gi.num();
            for (j=0; j<n; j++) { ng.add(gi.vote(j)); ng.votes().last()._t += pi; }
            ng.end() = gi.end();

            //Flag [i0,i] as healed, remove (i0,i]
            while (i > i0) {
               VoteGroup &gi = groups[final_groups[i]];
               gi.status() = VoteGroup::VOTE_GROUP_HEALED;
               final_groups.remove(i);
               i--;
            }
            VoteGroup &gi = groups[final_groups[i]];
            gi.status() = VoteGroup::VOTE_GROUP_HEALED;

            //Replace i0 with new group
            final_groups[i] = groups.num()-1;

            //Resort, since remove() mangles order
            final_groups.sort(votegroups_comp_begin);

            //Finally, fit the new group, but sort first!!
            ng.votes().sort(arclen_compare_votes);
            if ( ng.num() == 0 )
               arclength_fit(ng,freq);
            else
               (*this.*fit_func)(ng,freq);

            i0 = -1;
            pi = 0;
         }
      }

   }

   //Refit groups that got healer votes
   for (i=0; i < final_groups.num(); i++) {
      VoteGroup &gi   = groups[final_groups[i]];

      if (gi.fstatus() == VoteGroup::FIT_OLD) {
         gi.fits().clear();

         if ( gi.num() == 0 )
            arclength_fit(gi,freq);
         else
            (*this.*fit_func)(gi,freq);

      }
   }
}

/////////////////////////////////////
// refit_backward_fit_groups()
/////////////////////////////////////
void
SilAndCreaseTexture::refit_backward_fit_groups(
   LuboPath* p)
{
   // Note: p->stretch() is the inverse of the expected ratio

   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   ARRAY<VoteGroup>& groups = p->groups();

   int i, n = groups.num();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      if (g.fstatus() != VoteGroup::FIT_GOOD) {
         assert(g.fstatus() == VoteGroup::FIT_BACKWARDS);

         groups += VoteGroup ( p->gen_stroke_id(), p );
         VoteGroup& ng = groups.last();
         VoteGroup& gi = p->groups()[i];

         ng.votes() += gi.votes();
         ng.begin() = gi.begin();
         ng.end() = gi.end();

         gi.status() = VoteGroup::VOTE_GROUP_FINAL_FIT_BACKWARDS;

         if ( ng.num() == 0 )
            arclength_fit(ng,freq);
         else
            phasing_fit(ng,freq);

         assert(ng.fstatus() == VoteGroup::FIT_GOOD);

         cerr << "SilAndCreaseTexture::refit_backward_fit_groups - <<<BACKWARDS FIT>>>\n";
         //HACK_mouse_right_button_up();

      }
   }

}

/////////////////////////////////////
// random_fit()
/////////////////////////////////////
void
SilAndCreaseTexture::random_fit(VoteGroup& g, double freq)
{
   double phase = drand48();
   g.fits() += XYpt ( g.begin(), phase + 0.0 );
   g.fits() += XYpt ( g.end(),   phase + (g.end() - g.begin()) * freq );
   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// sigma_fit()
/////////////////////////////////////
void
SilAndCreaseTexture::sigma_fit(VoteGroup& g, double freq)
{
   g.fits() += XYpt ( g.begin(), 0.0 );
   g.fits() += XYpt ( g.end(),   (g.end() - g.begin())   * freq );
   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// arclength_fit()
/////////////////////////////////////
void
SilAndCreaseTexture::arclength_fit(VoteGroup& g, double freq)
{
   g.fits() += XYpt ( g.begin(), g.begin() * freq );
   g.fits() += XYpt ( g.end(),   g.end()   * freq );
   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// phasing_fit()
/////////////////////////////////////
void
SilAndCreaseTexture::phasing_fit(VoteGroup& g, double freq)
{
   //XXX - Fix this up to respect begin/end better

   int vote_count = 0;
   Wvec phase_vec;
   double phase, phase_i, phase_ave = 0.0;
   double t_begin, t_end, s_begin, s_end;

   for ( int i=0; i < g.num(); i++) {
      if (g.vote(i)._status == LuboVote::VOTE_OUTLIER)
         continue;

      phase_i = g.vote(i)._t - g.vote(i)._s*freq;

      phase_vec[1] += sin ( phase_i * TWO_PI );
      phase_vec[0] += cos ( phase_i * TWO_PI );

      phase_ave += phase_i;

      vote_count++;
   }

   phase_vec /= vote_count;
   phase_ave /= vote_count;

   phase = atan2 ( phase_vec[1] , phase_vec[0] );
   if ( phase < 0 )
      phase += TWO_PI;
   phase /= TWO_PI;

   //XXX - The phase is only unique to an integer,
   //but when plotting the vote vs. the fit,
   //we should try to get the right integer to
   //make the fit look like it actually 'fits'
   //the data...

   phase += floor(phase_ave);

   //Set the fit to cover all the votes
   //or more if begin/end demand it

   s_begin = min(g.begin(),g.first_vote()._s);
   s_end =   max(g.end(),  g.last_vote()._s);

   t_begin = s_begin * freq + phase;
   t_end   = s_end   * freq + phase;

   g.fits() += XYpt ( s_begin, t_begin );
   g.fits() += XYpt ( s_end  , t_end   );

   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// interpolating_fit()
/////////////////////////////////////
void
SilAndCreaseTexture::interpolating_fit(VoteGroup& g, double freq)
{
   bool bad = false;
   double t_begin, t_end, t_last, t;

   t_begin = g.first_vote()._t  + freq * ( g.begin() - g.first_vote()._s );
   t_end =    g.last_vote()._t  + freq * (   g.end() -  g.last_vote()._s );

   t_last = -DBL_MAX;
   if (g.begin() < g.first_vote()._s) {
      g.fits() += XYpt (g.begin(), t_begin);
   }
   for ( int i =0 ; i < g.num() ; i++ ) {
      assert(g.vote(i)._status == LuboVote::VOTE_GOOD);
      t = g.vote(i)._t;
      g.fits() += XYpt ( g.vote(i)._s, t);
      if (t<t_last)
         bad = true;
      t_last = t;
   }
   if (g.end() > g.vote(g.num()-1)._s) {
      g.fits() += XYpt ( g.end(), t_end );
   }

   if (bad)
      g.fstatus() = VoteGroup::FIT_BACKWARDS;
   else
      g.fstatus() = VoteGroup::FIT_GOOD;

}

#define REALLY_TINY 1.0e-20;

void lubksb(double *a, int n, int *indx, double b[])
{
   double sum;
   int i,ip,j;
   int ii=-1;
   for (i=0;i<n;i++) {
      ip=indx[i];
      sum=b[ip];
      b[ip]=b[i];
      if (ii>-1)
         for (j=ii;j<=i-1;j++)
            sum -= a[n*i+j]*b[j];
      else if (sum)
         ii=i;
      b[i]=sum;
   }
   for (i=n-1;i>=0;i--) {
      sum=b[i];
      for (j=i+1;j<n;j++)
         sum -= a[n*i+j]*b[j];
      b[i]=sum/a[n*i+i];
   }
}


bool ludcmp(double *a, int n, int *indx, double *d)
{
   int    i,imax,j,k;
   double big,dum,sum,temp;
   double *vv;

   vv = new double[n];
   assert(vv);

   *d=1.0;
   for (i=0;i<n;i++) {
      big=0.0;
      for (j=0;j<n;j++)
         if ((temp=fabs(a[n*i+j])) > big)
            big=temp;
   if (big == 0.0) { cerr << "ludcmp() - Singular!\n"; return false; }
      vv[i]=1.0/big;
   }
   for (j=0;j<n;j++) {
      for (i=0;i<j;i++) {
         sum=a[n*i+j];
         for (k=0;k<i;k++)
            sum -= a[n*i+k]*a[n*k+j];
         a[n*i+j]=sum;
      }
      big=0.0;
      for (i=j;i<n;i++) {
         sum=a[n*i+j];
         for (k=0;k<j;k++)
            sum -= a[n*i+k]*a[n*k+j];
         a[n*i+j]=sum;
         if ( (dum=vv[i]*fabs(sum)) >= big) {
            big=dum;
            imax=i;
         }
      }
      if (j != imax) {
         for (k=0;k<n;k++) {
            dum=a[n*imax+k];
            a[n*imax+k]=a[n*j+k];
            a[n*j+k]=dum;
         }
         *d = -(*d);
         vv[imax]=vv[j];
      }
      indx[j]=imax;
      if (a[n*j+j] == 0.0)
         a[n*j+j]=REALLY_TINY;
      if (j != n) {
         dum=1.0/(a[n*j+j]);
         for (i=j+1;i<n;i++)
            a[n*i+j] *= dum;
      }
   }
   delete[] vv;

   return true;
}
/////////////////////////////////////
// optimizing_fit()
/////////////////////////////////////
void
SilAndCreaseTexture::optimizing_fit(VoteGroup& g, double freq)
{

   double fit_pix_spacing, weight_fit, weight_scale, weight_distort, weight_heal;

   SilStrokePool* pool = _sil_stroke_pools[g.lubo_path()->line_type()];

   if (pool->get_coher_global()) {
      fit_pix_spacing = SilUI::fit_pix(VIEW::peek());
      weight_fit      = SilUI::weight_fit(VIEW::peek());
      weight_scale    = SilUI::weight_scale(VIEW::peek());
      weight_distort  = SilUI::weight_distort(VIEW::peek());
      weight_heal     = SilUI::weight_heal(VIEW::peek());
   } else {
      fit_pix_spacing = pool->get_coher_pix();
      weight_fit      = pool->get_coher_wf();
      weight_scale    = pool->get_coher_ws();
      weight_distort  = pool->get_coher_wb();
      weight_heal     = pool->get_coher_wh();
   }

   int i0, i, j, k, n;
   double yij,   wij,   tij,   xj,   xjd,   ndc_delta, factor;
   double yij_1, wij_1, tij_1, xj_1, xj_1d, foo;

   double begin = min(g.begin(),g.first_vote()._s);
   double end =   max(g.end(),  g.last_vote()._s);

   ndc_delta = (end - begin);

   n = max(2, (int)ceil( ndc_delta / g.lubo_path()->pix_to_ndc_scale() / (double)fit_pix_spacing ));

   ndc_delta /= (double)(n-1);

   // QQQ - Avoid using matrix class
   //   GXMatrixMNd A,d;
   //   A.SetDim(n,n,0.0);
   //   d.SetDim(n,1,0.0);

   double *A = new double[n*n];
   assert(A);
   double *d = new double[n];
   assert(d);

   for (i=0; i<n*n; i++)
      A[i]=0;
   for (i=0; i<n;   i++)
      d[i]=0;

   i = 0;

   assert(g.num()>0);

   // First, walk up to first vote within [begin,end]
   // (Should always stop immediately now...)
   while ((i < g.num()) && (g.vote(i)._s < begin))
      i++;

   for (j=0; j<n; j++) {
      //Fit - 1st term
      if (j>0) {
         while(i < g.num()) {
            if (g.vote(i)._s <= xj_1d) {
               assert(g.vote(i)._s >= xj_1);

               if (g.vote(i)._status != LuboVote::VOTE_OUTLIER) {
                  yij_1 = g.vote(i)._t - g.vote(i)._s * freq;

                  tij_1 = (g.vote(i)._s - xj_1)/ndc_delta;

                  if (g.vote(i)._status == LuboVote::VOTE_HEALER) {
                     wij_1 = 1.0;
                     factor = weight_heal * 2.0 * wij_1 * tij_1;
                  } else {
                     wij_1 = 1.0/g.num();
                     factor = weight_fit * 2.0 * wij_1 * tij_1;
                  }

                  A[n*j + j-1] += factor * (1.0 - tij_1);
                  A[n*j + j  ] += factor * (      tij_1);
                  d[j]         += factor * (      yij_1);
               }
               i++;
            } else {
               break;
            }
         }
      }

      //Fit - 2nd term

      if (j<(n-1)) {
         i0 = i;

         xj  = begin + j * ndc_delta;
         xjd = xj + ndc_delta;

         while(i < g.num()) {
            if (g.vote(i)._s <= xjd) {
               assert(g.vote(i)._s >= xj);

               if (g.vote(i)._status != LuboVote::VOTE_OUTLIER) {
                  yij = g.vote(i)._t  - g.vote(i)._s * freq;

                  tij = (g.vote(i)._s - xj)/ndc_delta;

                  if (g.vote(i)._status == LuboVote::VOTE_HEALER) {
                     wij = 1.0;
                     factor = weight_heal * 2.0 * wij * (1.0 - tij);
                  } else {
                     wij = 1.0/g.num();
                     factor = weight_fit * 2.0 * wij * (1.0 - tij);
                  }

                  A[n*j + j  ] += factor * (1.0 - tij);
                  A[n*j + j+1] += factor * (      tij);
                  d[j]         += factor * (      yij);
               }
               i++;
            } else {
               break;
            }
         }

         i = i0;
      }

      xj_1  = xj;
      xj_1d = xjd;

      factor = 2.0 * weight_scale / (double)(n*n);

      //Stretch
      for (k=0; k<n; k++) {
         A[n*j + k] -=  factor;
      }
      A[n*j + j] += n * factor;

      factor = 2.0 * weight_distort / (double) n;

      //Distort - 1st term
      if (j>1) {
         A[n*j + j-2] +=        factor;
         A[n*j + j-1] += -2.0 * factor;
         A[n*j + j  ] +=        factor;
      }
      //Distort - 2nd term
      if ((j>0)&&(j<(n-1))) {
         A[n*j + j-1] += -2.0 * factor;
         A[n*j + j  ] +=  4.0 * factor;
         A[n*j + j+1] += -2.0 * factor;
      }
      //Distort - 3rd term
      if (j<(n-2)) {
         A[n*j + j  ] +=        factor;
         A[n*j + j+1] += -2.0 * factor;
         A[n*j + j+2] +=        factor;
      }
   }

   int *IDX = new int[n];
   assert(IDX);

   if (ludcmp(A,n,IDX,&foo)) {
      lubksb(A,n,IDX,d);

      bool bad = false;

	  //Karol-> I found a bug here, fj_1 was used without being initialized
	  //only a vague idea what this loop actually does, but I've fixed it anyways :P

      
	  
	  for (j=0; j<n; j++) {
      double fj, fj_1 = 1.0; // <- seems to be working fine with this value 

         xj = begin + (double)j*ndc_delta;

         fj = d[j] + xj * freq;

         if ((j>0) && (fj < fj_1))
            bad = true;

         g.fits() += XYpt (xj, fj);

         fj_1 = fj;
      }

      if (bad)
         g.fstatus() = VoteGroup::FIT_BACKWARDS;
      else
         g.fstatus() = VoteGroup::FIT_GOOD;
   } else {
      arclength_fit(g,freq);
   }

   delete[] IDX;
   delete[] A;
   delete[] d;
}



/////////////////////////////////////
// majority_cover()
/////////////////////////////////////
void
SilAndCreaseTexture::majority_cover(LuboPath *p)
{

   ARRAY<VoteGroup>& groups = p->groups();
   int i, max_ind=-1, max_votes=0, n = groups.num();

   for ( i=0; i<n ; i++ ) {
      VoteGroup& g = groups[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      g.status() = VoteGroup::VOTE_GROUP_NOT_MAJORITY;

      if (g.num() > max_votes) {
         max_ind = i;
         max_votes = g.num();
      }
   }

   if (max_ind != -1) {
      VoteGroup& g = groups[max_ind];
      g.status() = VoteGroup::VOTE_GROUP_GOOD;

      if ( (0.0 < g.begin()) && (0.0 < g.first_vote()._s)
           && (g.fstatus() != VoteGroup::FIT_NONE))
         g.fstatus() = VoteGroup::FIT_OLD;
      g.begin() = 0.0;

      if ( (p->length() > g.end())  && (p->length() > g.last_vote()._s)
           && (g.fstatus() != VoteGroup::FIT_NONE))
         g.fstatus() = VoteGroup::FIT_OLD;
      g.end() = p->length();
   } else {
      //Deal with no groups
      groups += VoteGroup ( p->gen_stroke_id(), p );
      VoteGroup& ng = groups.last();
      ng.begin() = 0;
      ng.end()   = p->length();
   }


}

/////////////////////////////////////
// one_to_one_cover()
/////////////////////////////////////
void
SilAndCreaseTexture::one_to_one_cover(LuboPath *p)
{
   static double GLOBAL_MIN_PIX_PER_GROUP = Config::get_var_dbl("MIN_PIX_PER_GROUP", 5.0,true);
   static double GLOBAL_MIN_FRAC_PER_GROUP = Config::get_var_dbl("MIN_FRAC_PER_GROUP", 0.05,true);

   SilStrokePool* pool = _sil_stroke_pools[p->line_type()];
   double MIN_PIX_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_PIX_PER_GROUP):(pool->get_coher_mp()));
   double MIN_FRAC_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_FRAC_PER_GROUP):(pool->get_coher_m5()));

   double min_length = min( MIN_PIX_PER_GROUP * p->pix_to_ndc_scale(),
                            MIN_FRAC_PER_GROUP * p->length());

   ARRAY<VoteGroup>& groups = p->groups();
   int i, n = groups.num();
   double i_1_num, i_num, del, cnt;

   ARRAY<CoverageBoundary> final_boundary(2*n);

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      if ((g.end() - g.begin()) < min_length) {
         g.status() = VoteGroup::VOTE_GROUP_NOT_ONE_TO_ONE;
      } else {
         final_boundary += CoverageBoundary(i, g.begin(), COVERAGE_START);
         final_boundary += CoverageBoundary(i,   g.end(),   COVERAGE_END);
      }
   }

   n = final_boundary.num();

   if (n>0) {
      final_boundary.sort(coverage_comp);
      assert(final_boundary[0]._type == COVERAGE_START);
      //Now fall through
   }

   //Now fill the holes in the actual VoteGroups

   cnt = 1;
   for (i=1; i<n; i++) {
      if (cnt == 0) {
         assert(final_boundary[i-1]._type == COVERAGE_END);
         assert(  final_boundary[i]._type == COVERAGE_START);

         del = final_boundary[i]._s - final_boundary[i-1]._s;
         if (del>0.0) {
            VoteGroup &gi_1 = groups[final_boundary[i-1]._vg];
            VoteGroup &gi   = groups[final_boundary[i  ]._vg];

            i_1_num = gi_1.num();
            i_num =   gi.num();

            double s = gi_1.end() + del*((double)i_1_num)/((double)(i_1_num + i_num));

            if ((s < gi.begin()) && (s < gi.first_vote()._s) &&
                (gi.fstatus() != VoteGroup::FIT_NONE) )
               gi.fstatus()   = VoteGroup::FIT_OLD;
            gi.begin() = s;

            if ((gi_1.end() < s) && (gi_1.last_vote()._s < s) &&
                (gi_1.fstatus() != VoteGroup::FIT_NONE) )
               gi_1.fstatus() = VoteGroup::FIT_OLD;
            gi_1.end() = s;

         }
         cnt++;
      } else {
         if (final_boundary[i]._type == COVERAGE_START) {
            cnt++;
         } else {
            assert(final_boundary[i]._type == COVERAGE_END);
            cnt--;
         }

      }
   }

   //We found groups and filled the holes, now just tidy up the ends...
   if (cnt == 0) {
      assert(final_boundary.first()._type == COVERAGE_START);
      assert( final_boundary.last()._type == COVERAGE_END);

      VoteGroup &gf = groups[final_boundary.first()._vg];
      VoteGroup &gl = groups[ final_boundary.last()._vg];

      if ((0.0 < gf.begin()) && (0.0 < gf.first_vote()._s) &&
          (gf.fstatus() != VoteGroup::FIT_NONE) )
         gf.fstatus() = VoteGroup::FIT_OLD;
      gf.begin() = 0.0;

      if ((gl.end() < p->length()) && (gl.last_vote()._s < p->length()) &&
          (gl.fstatus() != VoteGroup::FIT_NONE) )
         gl.fstatus() = VoteGroup::FIT_OLD;
      gl.end() = p->length();

   }
   //Otherwise, just put a fresh-voteless group in...
   else {
      assert(cnt == 1);
      groups += VoteGroup ( p->gen_stroke_id(), p);
      VoteGroup &ng = groups.last();
      ng.begin() = 0;
      ng.end()   = p->length();
   }


}

/////////////////////////////////////
// hybrid_cover()
/////////////////////////////////////
void
SilAndCreaseTexture::hybrid_cover(LuboPath *p)
{
   static double GLOBAL_MIN_PIX_PER_GROUP = Config::get_var_dbl("MIN_PIX_PER_GROUP", 5.0, true);
   static double GLOBAL_MIN_FRAC_PER_GROUP = Config::get_var_dbl("MIN_FRAC_PER_GROUP", 0.05,true);

   SilStrokePool* pool = _sil_stroke_pools[p->line_type()];
   double MIN_PIX_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_PIX_PER_GROUP):(pool->get_coher_mp()));
   double MIN_FRAC_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_FRAC_PER_GROUP):(pool->get_coher_m5()));

   double min_length = min( MIN_PIX_PER_GROUP * p->pix_to_ndc_scale(),
                            MIN_FRAC_PER_GROUP * p->length());


   ARRAY<VoteGroup>& groups = p->groups();
   int i0, i, n = groups.num();
   double i0_len, i_len, del;

   ARRAY<CoverageBoundary> boundary(2*n);
   ARRAY<CoverageBoundary> final_boundary(2*n);
   ARRAY<int>              confidence_groups;


   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      boundary += CoverageBoundary(i, g.begin(), COVERAGE_START);
      boundary += CoverageBoundary(i,   g.end(),   COVERAGE_END);

      g.status() = VoteGroup::VOTE_GROUP_NOT_HYBRID;
   }


   n = boundary.num()/2;

   if (n>0) {

      //Assign confidences
      for (i=0; i<n; i++) {
         VoteGroup &g = groups[boundary[2*i]._vg];

         //XXX - Just look at length right now...
         g.confidence() = (g.end() - g.begin())/p->length();
      }

      boundary.sort(coverage_comp);

      assert(boundary[0]._type == COVERAGE_START);

      final_boundary += boundary[0];
      confidence_groups += boundary[0]._vg;

      n = n * 2;
      for (i=1; i<n; i++) {
         CoverageBoundary &cb = boundary[i];

         //Between covereage regions
         if (final_boundary.last()._type == COVERAGE_END) {
            assert(confidence_groups.num() == 0);
            assert(cb._type == COVERAGE_START);

            final_boundary += cb;
            confidence_groups += cb._vg;
         }
         //In the midst of a coverage region
         else {
            //Found the end of a group
            if (cb._type == COVERAGE_END) {
               //If its not the current covering region...
               if (cb._vg != confidence_groups[0]) {
                  //Just drop it from the array of current groups
                  confidence_groups.rem(cb._vg);
               }
               //Otherwise, complete this group...
               else {
                  final_boundary += cb;
                  confidence_groups.rem(cb._vg);

                  //And begin the next group
                  if (confidence_groups.num()>0) {
                     votegroups_comp_confidence_groups = &groups;
                     confidence_groups.sort(votegroups_comp_confidence);
                     final_boundary += CoverageBoundary(confidence_groups[0], cb._s, COVERAGE_START);
                  }
               }
            }
            //Otherwise, found the start of a group
            else {
               //If it's less confident...
               if (groups[cb._vg].confidence() <= groups[confidence_groups[0]].confidence()) {
                  //Just tuck it into the list of current groups
                  confidence_groups += cb._vg;
               }
               //If it's more confident
               else {
                  //Complete the current group
                  final_boundary += CoverageBoundary(confidence_groups[0], cb._s, COVERAGE_END);

                  //Replace the maximum with the new groups
                  confidence_groups[0] = cb._vg;

                  //And begin the next group
                  final_boundary += cb;
               }
            }
         }
      }

      //Now kill off any coverages that are too narrow
      n = final_boundary.num()/2;
      for (i=0; i<n; i++) {
         if ((final_boundary[2*i+1]._s - final_boundary[2*i]._s) < min_length) {
            final_boundary[2*i]._type = final_boundary[2*i+1]._type = COVERAGE_BAD;
         }
      }

      //Now fall through
   }

   //Now fill the holes...

   //XXX - Maybe fitting should use all the votes, and we just use a window of that,
   //      so window (begin/end) modifications don't invalidate the fits...

   i0 = -1;
   n = final_boundary.num()/2;
   for (i=0; i<n; i++) {
      if (final_boundary[2*i]._type != COVERAGE_BAD) {
         //First good coverage should cover start of path
         if (i0 == -1) {
            i_len = final_boundary[2*i+1]._s - final_boundary[2*i]._s;
            final_boundary[2*i]._s = 0.0;
         } else {
            i_len = final_boundary[2*i+1]._s - final_boundary[2*i]._s;
            del = final_boundary[2*i]._s - final_boundary[2*i0+1]._s;
            if (del>0.0)
               final_boundary[2*i]._s = (final_boundary[2*i0+1]._s += del*i0_len/(i0_len + i_len));
         }
         i0 = i;
         i0_len = i_len;
      }
   }

   //If any coverage was found, set up the groups...
   if (i0 != -1) {
      //First stretch last coverage to end of path
      final_boundary[2*i0+1]._s = p->length();

      //Now setup the corresponding groups...
      for (i=0; i<n; i++) {
         if (final_boundary[2*i]._type != COVERAGE_BAD) {
            VoteGroup &vg = groups[final_boundary[2*i]._vg];

            //If the group's not yet used just adjust it's begin/end
            if (vg.status() == VoteGroup::VOTE_GROUP_NOT_HYBRID) {
               double s;

               s = final_boundary[2*i]._s;
               if ((s < vg.begin()) && (s < vg.first_vote()._s) &&
                   (vg.fstatus() != VoteGroup::FIT_NONE) )
                  vg.fstatus()   = VoteGroup::FIT_OLD;
               vg.begin() = s;

               s = final_boundary[2*i+1]._s;
               if ((vg.end() < s) && (vg.last_vote()._s < s) &&
                   (vg.fstatus() != VoteGroup::FIT_NONE) )
                  vg.fstatus() = VoteGroup::FIT_OLD;
               vg.end() = s;

               vg.status() = VoteGroup::VOTE_GROUP_GOOD;
            }

            //Otherwise, its been segmented, so introduce a new group for this segment
            else {
               groups += VoteGroup ( p->gen_stroke_id(), p );
               VoteGroup& ng = groups.last();

               VoteGroup &vg = groups[final_boundary[2*i]._vg];

               ng.votes() += vg.votes();
               ng.begin() = final_boundary[2*i]._s;
               ng.end()   = final_boundary[2*i+1]._s;
            }
         }
      }


   }
   //Otherwise, just put a fresh-voteless group in...
   else {
      groups += VoteGroup ( p->gen_stroke_id(), p);
      VoteGroup &ng = groups.last();
      ng.begin() = 0;
      ng.end()   = p->length();
   }


}

/////////////////////////////////////
// generate_strokes_from_groups()
/////////////////////////////////////

void
SilAndCreaseTexture::generate_strokes_from_groups()
{
   static double SIL_TO_STROKE_PIX_SAMPLING = Config::get_var_dbl("SIL_TO_STROKE_PIX_SAMPLING",6.0,true);

   int n, i, j, k, num;
   double sbegin, send, sdelta, ubegin, uend, udelta, length;
   OutlineStroke* stroke;

   if (!_sil_strokes_need_update)
      return;

   LuboPathList& paths = _zx_edge_tex.paths();

   double step_size = (paths.num())?(paths[0]->pix_to_ndc_scale() * SIL_TO_STROKE_PIX_SAMPLING):(0);

   
   for (k=0; k<SIL_STROKE_POOL_NUM; k++) {
      _sil_stroke_pools[k]->blank();
      _sil_stroke_pools[k]->set_path_index_stamp(_zx_edge_tex.path_stamp());
      _sil_stroke_pools[k]->set_group_index_stamp(_zx_edge_tex.group_stamp());
   }

   for (k=0; k<paths.num(); k++) {
      LuboPath *p = paths[k];

      n = p->groups().num(); 
     
      length = p->length();
      for ( i = 0 ; i < n ; i++ ) {
         VoteGroup &g = p->groups()[i];

         if (g.status() != VoteGroup::VOTE_GROUP_GOOD)
            continue;

         assert(g.fstatus() == VoteGroup::FIT_GOOD) ;

         stroke = _sil_stroke_pools[p->line_type()]->get_stroke();
         assert(stroke);

         stroke->set_path_index(k);
         stroke->set_group_index(g.id());

         sbegin = g.begin();
         ubegin = sbegin / length;

         send = g.end();
         uend = send / length;

         sdelta = send - sbegin;
         udelta = uend - ubegin;

         assert(sdelta>=0);

         if (sdelta > 0) {
            num =(int)max(2.0, ceil(sdelta/step_size));

            sdelta /= (double)(num-1);
            udelta /= (double)(num-1);

            if ( stroke->get_offsets())
               stroke->get_offsets()->set_manual_t(true);
            stroke->set_offset_stretch((float)(1.0/p->stretch()));
            stroke->set_gen_t(false);

            stroke->set_offset_lower_t(g.get_t(sbegin));
            stroke->set_min_t         (g.get_t(sbegin));

            stroke->set_offset_upper_t(g.get_t(send));
            stroke->set_max_t         (g.get_t(send));

            NDCZpt beg = p->pt(ubegin);
            NDCZpt end = p->pt(uend);

            if ((num == 2) && (end-beg).planar_length() < gEpsZeroMath ) {
               //cerr << "SilAndCreaseTexture::generate_strokes_from_groups() - Ah-ha!! Culled a 2 vertex, gEpsZeroMath length group!\n";
               _sil_stroke_pools[p->line_type()]->remove_stroke(stroke);
            } else {
               stroke->add
               (g.get_t(sbegin), beg);
               for (j=1; j < num-1; j++)
                  stroke->add
                  ( g.get_t(sbegin + j * sdelta), p->pt(ubegin + j * udelta) );
               stroke->add
               (g.get_t(send), end);
            }
         } else {
            cerr << "SilAndCreaseTexture::generate_strokes_from_groups() - ERROR!! Culled a ZERO length group!\n";
            _sil_stroke_pools[p->line_type()]->remove_stroke(stroke);
         }

      }
   }
   bool selection_changed = false;
   for (k=0; k<SIL_STROKE_POOL_NUM; k++) {
      selection_changed |= _sil_stroke_pools[k]->update_selection(paths);
   }
   if (selection_changed) {
      LinePen *line_pen = 0;
      if (BaseJOTapp::instance() &&
          (line_pen = dynamic_cast<LinePen*>(BaseJOTapp::instance()->cur_pen()))){
         line_pen->selection_changed(LinePen::LINE_PEN_SELECTION_CHANGED__SIL_TRACKING);
      }
   }
   _sil_strokes_need_update = false;
}

