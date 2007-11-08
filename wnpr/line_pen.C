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
// line_pen.C
////////////////////////////////////////////

#include "base_jotapp/base_jotapp.H"
#include "gest/vieweasel.H"
#include "npr/npr_texture.H"

#include "line_pen.H"

#include "line_pen_ui.H"

#include "stroke/edge_stroke.H"
#include "stroke/decal_line_stroke.H"

using namespace mlib;

#define BASELINE_STROKE_TEXTURE  "nprdata/system_textures/baseline.png"
#define BASELINE_STROKE_WIDTH    7.0f
#define BASELINE_STROKE_ALPHA    1.0f
#define BASELINE_STROKE_FLARE    0.0f
#define BASELINE_STROKE_TAPER    0.0f
#define BASELINE_STROKE_FADE     1.0f
#define BASELINE_STROKE_AFLARE   0.0f
#define BASELINE_STROKE_HALO     0.0f
#define BASELINE_STROKE_COLOR    COLOR(0.1f,0.1f,1.0f)
#define BASELINE_STROKE_ANGLE    30.0f

/*****************************************************************
 * BlankGestureDrawer
 *****************************************************************/

class BlankGestureDrawer : public GestureDrawer{
 public:
   virtual GestureDrawer* dup() const { return new BlankGestureDrawer; }
   virtual int draw(const GESTURE*, CVIEWptr&) { return 0; }
};

/*****************************************************************
 * LinePen
 *****************************************************************/

/////////////////////////////////////
// Constructor
/////////////////////////////////////

LinePen::LinePen(
   CGEST_INTptr &gest_int,
   CEvent& d, CEvent& m, CEvent& u,
   CEvent& shift_down, CEvent& shift_up,
   CEvent& ctrl_down,  CEvent& ctrl_up) :
   Pen(str_ptr("Line"), 
       gest_int, d, m, u,
       shift_down, shift_up, 
       ctrl_down, ctrl_up)
{
               
   // gestures we recognize:
   _draw_start += DrawArc(new TapGuard,      drawCB(&LinePen::tap_cb));
   _draw_start += DrawArc(new SlashGuard,    drawCB(&LinePen::slash_cb));
   _draw_start += DrawArc(new LineGuard,     drawCB(&LinePen::line_cb));
   _draw_start += DrawArc(new ScribbleGuard, drawCB(&LinePen::scribble_cb));
   _draw_start += DrawArc(new LassoGuard,    drawCB(&LinePen::lasso_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&LinePen::stroke_cb));

   assert(_gest_int);

   // Place to remember old settings
   _prev_gesture_drawer = 0;

   // Drawer for when we dont wanna draw strokes
   _blank_gesture_drawer = new BlankGestureDrawer(); assert(_blank_gesture_drawer);
   // Drawer for when we do
   _gesture_drawer = new GestureStrokeDrawer();   assert(_gesture_drawer);
   _gesture_drawer->set_base_stroke_proto(new OutlineStroke);

   // UI vars:
   _ui = new LinePenUI(this);   assert(_ui);

   _curr_tex = NULL;
   _curr_pool = NULL;
   _curr_stroke = NULL;
   _curr_mode = EDIT_MODE_NONE;

   _undo_prototype = new OutlineStroke(); assert(_undo_prototype);
   _undo_prototype->set_propagate_mesh_size(false);

   _virtual_baseline = true;

   _baseline_gel = new BASELINE_GEL;  assert(_baseline_gel != NULL);

   BaseStroke *s = _baseline_gel->stroke();

   s->set_texture(Config::JOT_ROOT() + BASELINE_STROKE_TEXTURE);
   s->set_width(  BASELINE_STROKE_WIDTH);
   s->set_alpha(  BASELINE_STROKE_ALPHA);
   s->set_flare(  BASELINE_STROKE_FLARE);
   s->set_taper(  BASELINE_STROKE_TAPER);
   s->set_fade(   BASELINE_STROKE_FADE);
   s->set_aflare( BASELINE_STROKE_AFLARE);
   s->set_halo(   BASELINE_STROKE_HALO);
   s->set_color(  BASELINE_STROKE_COLOR); 
   s->set_angle(  BASELINE_STROKE_ANGLE);

   observe();
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

LinePen::~LinePen()
{
   unobserve();

   assert(_ui);
   delete _ui;

   assert(_gesture_drawer);
   delete _gesture_drawer;

   assert(_blank_gesture_drawer);
   delete _blank_gesture_drawer;

   assert(_baseline_gel != NULL);
   _baseline_gel = NULL;

   delete _undo_prototype;
}

/////////////////////////////////////
// observe()
/////////////////////////////////////
void
LinePen::observe() 
{
   // CAMobs:
   VIEW::peek()->cam()->data()->add_cb(this);

   // BMESHobs:
   //subscribe_all_mesh_notifications();

   // XFORMobs:
   //every_xform_obs();
}

/////////////////////////////////////
// unobserve()
/////////////////////////////////////
void 
LinePen::unobserve() 
{
   // CAMobs:
   VIEW::peek()->cam()->data()->rem_cb(this);

   // BMESHobs:
   //unsubscribe_all_mesh_notifications();

   // XFORMobs:
   //unobs_every_xform();
}

/////////////////////////////////////
// activate()
/////////////////////////////////////

void 
LinePen::activate(State *s)
{

   if(_ui) _ui->show();

   Pen::activate(s);

   _ui->update();

   _prev_gesture_drawer = _gest_int->drawer();
   _gest_int->set_drawer(_gesture_drawer);   

   //Change to patch's desired textures
   if (_view) _view->set_rendering(GTexture::static_name());       
}

/////////////////////////////////////
// deactivate()
/////////////////////////////////////

bool  
LinePen::deactivate(State* s)
{
   easel_clear();

   perform_selection(NULL, NULL, NULL, EDIT_MODE_NONE);
   
   if(_ui) _ui->hide();

   bool ret = Pen::deactivate(s);   assert(ret);

   _gest_int->set_drawer(_prev_gesture_drawer);   
   _prev_gesture_drawer = NULL;

   return true;
}

/////////////////////////////////////
// pick_stroke()
/////////////////////////////////////
bool
LinePen::pick_stroke(
   NPRTexture*       tex,
   CNDCpt&           p,
   double            radius,
   BStrokePool*&     ret_pool,
   OutlineStroke*&   ret_stroke,
   edit_mode_t&      ret_mode)
{
   assert(tex);
   
   int i;

   double         min_dist    = DBL_MAX;
   double         tmp_dist;
   BStrokePool*   tmp_pool;
   OutlineStroke* tmp_stroke;

   //Decals
   tmp_pool    = tex->stroke_tex()->get_decal_stroke_pool();   assert(tmp_pool);
   tmp_stroke  = tmp_pool->pick_stroke(p, tmp_dist, radius);
   
   if (tmp_stroke && (tmp_dist < min_dist))  
   {
      min_dist = tmp_dist;  ret_pool = tmp_pool; ret_stroke  = tmp_stroke;
   }
   

   for ( i=0; i<SilAndCreaseTexture::SIL_STROKE_POOL_NUM; i++) 
   {
      tmp_pool    = tex->stroke_tex()->sil_and_crease_tex()->get_sil_stroke_pool(
                                       (SilAndCreaseTexture::sil_stroke_pool_t)i);
      tmp_stroke  = tmp_pool->pick_stroke(p, tmp_dist, radius);

      if ((tmp_stroke) && (tmp_dist < min_dist))
      {
         min_dist = tmp_dist;  ret_pool = tmp_pool; ret_stroke  = tmp_stroke;
      }
   }

   if (!tex->stroke_tex()->sil_and_crease_tex()->get_hide_crease())
   {
      ARRAY<EdgeStrokePool*>* crease_stroke_pools = tex->stroke_tex()->sil_and_crease_tex()->get_crease_stroke_pools();  assert(crease_stroke_pools);
      for (i=0; i<crease_stroke_pools->num(); i++) 
      {
         tmp_pool    = (*crease_stroke_pools)[i];
         tmp_stroke  = tmp_pool->pick_stroke(p, tmp_dist, radius);

         if ((tmp_stroke) && (tmp_dist < min_dist))
         {
            min_dist = tmp_dist;  ret_pool = tmp_pool; ret_stroke  = tmp_stroke;
         }
      }
   }


   if (min_dist == DBL_MAX)
   {
      ret_stroke = NULL;      ret_pool   = NULL;      ret_mode   = EDIT_MODE_NONE;
      return false;
   }
   else
   {
      assert(ret_pool && ret_stroke);
      assert(min_dist <= radius);
      if (     ret_pool->class_name() == SilStrokePool::static_name())   ret_mode   = EDIT_MODE_SIL;
      else if (ret_pool->class_name() == EdgeStrokePool::static_name())  ret_mode   = EDIT_MODE_CREASE;
      else if (ret_pool->class_name() == DecalStrokePool::static_name()) ret_mode   = EDIT_MODE_DECAL;
      else assert(0);
      return true;
   }
}

/////////////////////////////////////
// handle_event()
/////////////////////////////////////
int 
LinePen::handle_event(CEvent& e, State* &s)
{
   // this is called by the GEST_INT on a down event in case the Pen
   // wants to take over handling the event... we don't... but
   // we'll take the opportunity to monkey with the gesture drawer...

   update_gesture_drawer();

   return 0;
}

/////////////////////////////////////
// update_gesture_drawer()
/////////////////////////////////////
void
LinePen::update_gesture_drawer()
{
   bool drawable;
   CBaseStroke *s;

   //Sanity checks, mostly...
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      assert(_curr_pool->get_prototype());

      s = _curr_pool->get_prototype();
      drawable = true;
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      
      s = _curr_stroke;
      drawable = (s != NULL);
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      
      s = NULL;
      drawable = true;
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);

      s = NULL;
      drawable = false;
   }   

   if (drawable)
   {
      _gest_int->set_drawer(_gesture_drawer);         
   }
   else
   {
      _gest_int->set_drawer(_blank_gesture_drawer);   
   }

   if (!s)
   {
      s = _ui->get_stroke(); assert(s);
   }

   BaseStroke *gd = _gesture_drawer->base_stroke_proto(); assert(gd);

   OutlineStroke o;
   o.set_propagate_mesh_size(false);
   o.set_propagate_patch(false);
   o.set_propagate_offsets(false);
   o.copy(*s);

   gd->copy(o);

   _gesture_drawer->set_base_stroke_proto(gd);

}

/////////////////////////////////////
// tap()
/////////////////////////////////////
int
LinePen::tap_cb(CGESTUREptr& gest, DrawState*&)
{
   double PICK_PATCH_RADIUS   = 20;
   double PICK_STROKE_RADIUS  = 8;
   double PICK_CREASE_RADIUS  = 2;
   double PICK_FACE_RADIUS    = 8;

   cerr << "LinePen::tap_cb" << endl;

   NPRTexture*    new_tex     = NULL;		
   BStrokePool*   new_pool    = NULL;
   OutlineStroke* new_stroke  = NULL;
   edit_mode_t    new_mode    = EDIT_MODE_NONE;

   Patch* p;
   Bedge *e;
   Bface *f;

   if ((p = VisRefImage::get_ctrl_patch(gest->center(),PICK_PATCH_RADIUS)))
   {
      GTexture* t = p->cur_tex();   
      
      if (!t || !(t->class_name() == NPRTexture::static_name()))
      {
         //Set the selected face's patch to using NPRTexture
         p->set_texture(NPRTexture::static_name());

         t = p->cur_tex();  assert(t && (t->class_name() == NPRTexture::static_name()));
      }         
         
      new_tex = (NPRTexture*)t;

      if (pick_stroke(new_tex, gest->center(), 
                        PICK_STROKE_RADIUS, new_pool, new_stroke, new_mode))
      {
         assert(new_pool   != NULL);
         assert(new_stroke != NULL);
         assert(new_mode   != EDIT_MODE_NONE);
      }
      // We failed to pick a stroke directy, so try picking from the mesh
      // first creases, and then faces...
      else if ((e = VisRefImage::get_edge(gest->center(),PICK_CREASE_RADIUS)))
      { 
         assert (p == e->patch()->ctrl_patch());
         if (e->is_crease()) 
         {
            if (!new_tex->stroke_tex()->sil_and_crease_tex()->get_hide_crease())
            {
               // try to find a crease stroke pool associated with the edge
               SimplexData * d = e->find_data(&EdgeStrokePool::foo);
               if(d) 
               { 
                  new_pool = ((EdgeStrokePool::EdgeData*)d)->pool(); assert(new_pool);

                  if (new_pool->num_strokes()==0) 
                  {
                     new_stroke = NULL;
                  }
                  else 
                  {
                     assert(!new_pool->get_selected_stroke());
                     new_pool->set_selected_stroke_index(0);
                     OutlineStroke* s = new_pool->get_selected_stroke(); 
                     assert(s && s->is_of_type(EdgeStroke::static_name()));
                     new_stroke = s;        
                  }
                  new_mode = EDIT_MODE_CREASE;
               }
               else
               {
                  new_tex = NULL;
               }
            }
            else
            {
               new_tex = NULL;
            }
         }
         else
         {
            new_tex = NULL;
         }
      } 
      else if ((f = VisRefImage::get_face(gest->center(),PICK_FACE_RADIUS)))
      { 
         assert (p == f->patch()->ctrl_patch());

         new_pool = new_tex->stroke_tex()->get_decal_stroke_pool();  assert(new_pool);
   
         if (new_pool->num_strokes()==0) 
         {
            new_stroke = NULL;
         }
         else 
         {
            assert(!new_pool->get_selected_stroke());
            new_pool->set_selected_stroke_index(0); 
            OutlineStroke* s = new_pool->get_selected_stroke(); 
            assert(s && s->is_of_type(DecalLineStroke::static_name()));
            new_stroke = s;        
         }
         new_mode = EDIT_MODE_DECAL;
      }
      //Failed to pick anything, despite finding a patch
      //in the initial serach, so bail out.
      else
      {
         new_tex = NULL;
      }
   }

   easel_clear();

   apply_undo_prototype();

   perform_selection(new_tex, new_pool, new_stroke, new_mode);

   store_undo_prototype();

   display_mode();

   _ui->update();

   return 1;
}

/////////////////////////////////////
// perform_selection()
/////////////////////////////////////
void
LinePen::perform_selection(
   NPRTexture*    new_tex,
   BStrokePool*   new_pool,
   OutlineStroke* new_stroke,
   edit_mode_t    new_mode)
{
   //NPRTexture*    old_tex     = _curr_tex;
   //BStrokePool*   old_pool    = _curr_pool;
   //OutlineStroke* old_stroke  = _curr_stroke;
   //edit_mode_t    old_mode    - _curr_mode;

   if (_curr_tex != new_tex)
   {
      if (_curr_tex) _curr_tex->set_selected(false);
      _curr_tex = new_tex;
      if (_curr_tex) 
      {
         _curr_tex->set_selected(true);
      }
      assert(_curr_pool != new_pool);
      assert((_curr_stroke != new_stroke) || (!_curr_stroke));
   }

   if (_curr_mode != new_mode)   
   {
      assert(_curr_pool != new_pool);
      assert((_curr_stroke != new_stroke) || (!_curr_stroke));
   }

   if (_curr_pool != new_pool)
   {
      if (_curr_pool)
      {
         _curr_pool->set_selected_stroke(NULL);
      }
      _curr_pool = new_pool;
      _curr_mode = new_mode;
      assert((_curr_stroke != new_stroke) || (!_curr_stroke));
   }

   if (_curr_stroke != new_stroke)
   {
      _curr_stroke = new_stroke;
      if (_curr_pool)
      {
         _curr_pool->set_selected_stroke(_curr_stroke);
      }
      else
      {
         assert(!_curr_stroke);
      }
   }
   
   update_gesture_drawer();
}

/////////////////////////////////////
// display_mode()
/////////////////////////////////////
void
LinePen::display_mode() 
{
   
   str_ptr mode_text;
   int i;
 
   switch(_curr_mode) 
   {
      case EDIT_MODE_DECAL:
         mode_text = str_ptr("Editing Decals");
      break;
      case EDIT_MODE_CREASE:
         mode_text = str_ptr("Editing Creases");
      break;
      case EDIT_MODE_SIL:
         assert(_curr_tex);
         assert(_curr_pool);

         SilAndCreaseTexture::sil_stroke_pool_t sil_type;

         for (i=0; i<SilAndCreaseTexture::SIL_STROKE_POOL_NUM; i++) 
         {
             if (_curr_pool == _curr_tex->stroke_tex()->sil_and_crease_tex()->
                                 get_sil_stroke_pool((SilAndCreaseTexture::sil_stroke_pool_t)i))
             {
               sil_type = (SilAndCreaseTexture::sil_stroke_pool_t)i;
             }
         }

         mode_text = str_ptr("Editing Lines (") + SilAndCreaseTexture::sil_stroke_pool(sil_type) + ")";
      break;
      case EDIT_MODE_NONE:
         mode_text = str_ptr("Editing NOTHING");
      break;
      default:
         assert(0);
      break;
   }

   WORLD::message(mode_text);
   cerr << "LinePen::display_mode - " << mode_text << "\n";

}

/////////////////////////////////////
// apply_undo_prototype()
/////////////////////////////////////
void
LinePen::apply_undo_prototype() 
{

   modify_active_prototype(_undo_prototype);

}

/////////////////////////////////////
// store_undo_prototype()
/////////////////////////////////////
void
LinePen::store_undo_prototype() 
{
   COutlineStroke *s;
   
   s = retrieve_active_prototype();

   if (s)
   {
      _undo_prototype->copy(*s);
   }

}

/////////////////////////////////////
// retrieve_active_prototype()
/////////////////////////////////////
COutlineStroke*
LinePen::retrieve_active_prototype() 
{
   OutlineStroke *s;

   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
   
      s = _curr_pool->get_prototype();
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

      s = _curr_stroke;
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

      s = _curr_stroke;
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);

      s = NULL;
   }

   return s;
}

/////////////////////////////////////
// modify_active_prototype()
/////////////////////////////////////
void
LinePen::modify_active_prototype(COutlineStroke *s) 
{
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
   
      _curr_pool->get_prototype()->copy(*s);
      int flag = _curr_pool->set_prototype(_curr_pool->get_prototype());

      if (flag & BSTROKEPOOL_SET_PROTOTYPE__NEEDS_FLUSH)
      {
         //Period changed, so need to reset paramaterization
         _curr_tex->stroke_tex()->sil_and_crease_tex()->zx_edge_tex()->destroy_state();
         _curr_tex->stroke_tex()->sil_and_crease_tex()->mark_sils_dirty();
         cerr << "LinePen::apply_undo_prototype() - Resetting parameterization.\n";
      }
      else if (flag & BSTROKEPOOL_SET_PROTOTYPE__NEEDED_ADJUSTMENT)
      {
         WORLD::message("Enforcing period and base mesh size of base prototype.");
         cerr << "LinePen::apply_undo_prototype() - Period of base prototype enforced!!!\n";
      }

   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

      if (_curr_stroke) _curr_stroke->copy(*s);
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

      if (_curr_stroke) _curr_stroke->copy(*s);
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);
   }

}

/////////////////////////////////////
// button_mesh_recrease()
/////////////////////////////////////
void
LinePen::button_mesh_recrease() 
{
   assert(_curr_tex);

   _curr_tex->stroke_tex()->sil_and_crease_tex()->recreate_creases();

   WORLD::message("Creases Recomputed");

   //XXX - Prolly not needed due to mesh observers hearing aobut new creases
   _curr_tex->stroke_tex()->sil_and_crease_tex()->mark_all_dirty();

}

/////////////////////////////////////
// button_noise_prototype_next()
/////////////////////////////////////
void
LinePen::button_noise_prototype_next() 
{
   assert(_curr_tex);
   assert(_curr_pool);
   assert(_curr_mode == EDIT_MODE_SIL);
   assert(_curr_pool->class_name() == SilStrokePool::static_name());

   SilStrokePool* sil_pool = (SilStrokePool*)_curr_pool;

   assert(sil_pool->get_num_protos()>1);

   apply_undo_prototype();

   sil_pool->set_edit_proto( ((sil_pool->get_edit_proto()+1)%(sil_pool->get_num_protos())) );

   store_undo_prototype();

   _ui->update();

   WORLD::message(str_ptr("Selected Prototype ") + 
                     str_ptr(sil_pool->get_edit_proto()+1) + " of " + str_ptr(sil_pool->get_num_protos()));

   //XXX - Pools are smart enough to get dirty
   //_curr_tex->stroke_tex()->sil_and_crease_tex()->mark_all_dirty();
}

/////////////////////////////////////
// button_noise_prototype_del()
/////////////////////////////////////
void
LinePen::button_noise_prototype_del() 
{
   assert(_curr_tex);
   assert(_curr_pool);
   assert(_curr_mode == EDIT_MODE_SIL);
   assert(_curr_pool->class_name() == SilStrokePool::static_name());

   SilStrokePool* sil_pool = (SilStrokePool*)_curr_pool;

   assert(sil_pool->get_num_protos()>1);

   sil_pool->del_prototype();

   store_undo_prototype();

   _ui->update();

   WORLD::message("Deleted Prototype");

   //XXX - Pools are smart enough to get dirty
   //_curr_tex->stroke_tex()->sil_and_crease_tex()->mark_all_dirty();
}

/////////////////////////////////////
// button_noise_prototype_add()
/////////////////////////////////////
void
LinePen::button_noise_prototype_add() 
{
   assert(_curr_tex);
   assert(_curr_pool);
   assert(_curr_mode == EDIT_MODE_SIL);
   assert(_curr_pool->class_name() == SilStrokePool::static_name());

   SilStrokePool* sil_pool = (SilStrokePool*)_curr_pool;

   sil_pool->add_prototype();

   apply_undo_prototype();

   sil_pool->set_edit_proto(sil_pool->get_num_protos()-1);

   store_undo_prototype();

   _ui->update();

   WORLD::message("Added Prototype");

   //XXX - Pools are smart enough to get dirty
   //_curr_tex->stroke_tex()->sil_and_crease_tex()->mark_all_dirty();
}

/////////////////////////////////////
// button_edit_cycle_line_types()
/////////////////////////////////////
void
LinePen::button_edit_cycle_line_types()
{
   BStrokePool*   new_pool;
   OutlineStroke* new_stroke;

   assert(_curr_tex);

   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));

      SilAndCreaseTexture::sil_stroke_pool_t sil_type;
      for (int i=0; i<SilAndCreaseTexture::SIL_STROKE_POOL_NUM; i++) 
      {
          if (_curr_pool == _curr_tex->stroke_tex()->sil_and_crease_tex()->
                              get_sil_stroke_pool((SilAndCreaseTexture::sil_stroke_pool_t)i))
          {
            sil_type = (SilAndCreaseTexture::sil_stroke_pool_t)i;
          }
      }

      new_pool = _curr_tex->stroke_tex()->sil_and_crease_tex()->
                              get_sil_stroke_pool((SilAndCreaseTexture::sil_stroke_pool_t)
                                    (((int)sil_type+1)%SilAndCreaseTexture::SIL_STROKE_POOL_NUM)   );

   }
   else
   {
      new_pool = _curr_tex->stroke_tex()->sil_and_crease_tex()->
                              get_sil_stroke_pool(SilAndCreaseTexture::SIL_VIS);
   }

   new_stroke = (new_pool->num_strokes())?(new_pool->stroke_at(0)):(NULL);

   easel_clear();

   apply_undo_prototype();

   perform_selection(_curr_tex,new_pool,new_stroke,EDIT_MODE_SIL);

   store_undo_prototype();

   display_mode();

   _ui->update();

}

/////////////////////////////////////
// button_edit_cycle_decal_groups()
/////////////////////////////////////
void
LinePen::button_edit_cycle_decal_groups()
{
   OutlineStroke* new_stroke;
   BStrokePool*   new_pool;

   assert(_curr_tex);

   if(_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));

      new_pool = _curr_pool;

      if(_curr_stroke)
      {
         assert(_curr_pool->get_selected_stroke() == _curr_stroke);

         int i = _curr_pool->get_selected_stroke_index();
         new_stroke = _curr_pool->stroke_at((i+1)%_curr_pool->num_strokes());
      }
      else
      {
         if (_curr_pool->num_strokes()>0)
         {
            new_stroke = _curr_pool->stroke_at(0);
         }
         else
         {
            new_stroke = NULL;
         }
      }
   }
   else
   {
      new_pool = _curr_tex->stroke_tex()->get_decal_stroke_pool();

      if (new_pool->num_strokes()>0)
      {
         new_stroke = new_pool->stroke_at(0);
      }
      else
      {
         new_stroke = NULL;
      }
   }

   easel_clear();

   apply_undo_prototype();

   perform_selection(_curr_tex,new_pool,new_stroke,EDIT_MODE_DECAL);

   store_undo_prototype();

   display_mode();

   _ui->update();
}

/////////////////////////////////////
// button_edit_cycle_crease_paths()
/////////////////////////////////////
void
LinePen::button_edit_cycle_crease_paths()
{
   BStrokePool*   new_pool;
   OutlineStroke* new_stroke;

   assert(_curr_tex);

   ARRAY<EdgeStrokePool*>* pools = _curr_tex->stroke_tex()->sil_and_crease_tex()->get_crease_stroke_pools();  assert(pools);

   assert(pools->num()>0);

   if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      EdgeStrokePool* edge_pool = (EdgeStrokePool*)_curr_pool;
      
      int i = pools->get_index(edge_pool); assert(i!=BAD_IND);
      new_pool = (*pools)[(i+1)%pools->num()];

   }
   else
   {
      new_pool = (*pools)[0];
   }

   new_stroke = (new_pool->num_strokes())?(new_pool->stroke_at(0)):(NULL);

   easel_clear();

   apply_undo_prototype();

   perform_selection(_curr_tex,new_pool,new_stroke,EDIT_MODE_CREASE);

   store_undo_prototype();

   display_mode();

   _ui->update();
}

/////////////////////////////////////
// button_edit_cycle_crease_strokes()
/////////////////////////////////////
void
LinePen::button_edit_cycle_crease_strokes()
{
   OutlineStroke* new_stroke;

   assert(_curr_tex);

   assert(_curr_mode == EDIT_MODE_CREASE);

   assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));

   assert(_curr_pool->num_strokes() > 0);

   if(_curr_stroke)
   {
      assert(_curr_pool->get_selected_stroke() == _curr_stroke);

      int i = _curr_pool->get_selected_stroke_index();
      new_stroke = _curr_pool->stroke_at((i+1)%_curr_pool->num_strokes());
   }
   else
   {
      new_stroke = _curr_pool->stroke_at(0);
   }

   easel_clear();

   apply_undo_prototype();

   perform_selection(_curr_tex,_curr_pool,new_stroke,EDIT_MODE_CREASE);

   store_undo_prototype();

   display_mode();

   _ui->update();
}

/////////////////////////////////////
// button_edit_offset_edit()
/////////////////////////////////////
void
LinePen::button_edit_offset_edit()
{
   assert((_curr_mode == EDIT_MODE_SIL) || (_curr_mode == EDIT_MODE_CREASE));
   COutlineStroke *s = retrieve_active_prototype();
   assert(s && (s->get_offsets() != NULL));

   easel_clear();   

   easel_populate();

   if(!easel_is_empty())
      easel_update_baseline();
   
   _ui->update();
}

/////////////////////////////////////
// button_edit_offset_clear()
/////////////////////////////////////
void
LinePen::button_edit_offset_clear()
{
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      assert(_curr_pool->get_prototype());

      OutlineStroke o;

      o.set_propagate_offsets(true);
      o.set_propagate_mesh_size(true);
      o.set_propagate_patch(false);

      COutlineStroke *op;

      op = retrieve_active_prototype();   assert(op);
      assert(op->class_name() == OutlineStroke::static_name());
      //assert(op->get_offsets() != NULL);
      assert(op->get_patch() == _curr_tex->patch());

      o.copy(*op);
      o.set_offsets(NULL);
      //XXX - Do this here?
      o.set_original_mesh_size(_view->pix_to_ndc_scale() * _curr_tex->patch()->pix_size());

      modify_active_prototype(&o);

      WORLD::message("Offsets cleared from Lines.");
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);

      OutlineStroke o;

      o.set_propagate_offsets(true);
      o.set_propagate_mesh_size(false);

      COutlineStroke *op;

      op = retrieve_active_prototype();   assert(op);
      assert(op->class_name() == EdgeStroke::static_name());
      //assert(op->get_offsets() != NULL);

      o.copy(*op);
      o.set_offsets(NULL);

      modify_active_prototype(&o);

      WORLD::message("Offsets cleared from Creases.");
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

      //Trash selected decal
      
      OutlineStroke *s = _curr_stroke;   assert(s);
      assert(s->is_of_type(DecalLineStroke::static_name()));

      apply_undo_prototype();

      perform_selection(_curr_tex, _curr_pool, NULL, _curr_mode);

      store_undo_prototype();

      _curr_pool->remove_stroke(s);      

      WORLD::message("Decal cleared from surface.");
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);

      //Shouldn't be able to press this button right now!
      assert(0);
   }   

   _ui->update();
}

/////////////////////////////////////
// button_edit_offset_undo()
/////////////////////////////////////
void
LinePen::button_edit_offset_undo()
{
   assert(!easel_is_empty());

   easel_pop();

   if (!easel_is_empty())
      easel_update_baseline();
   else
    _ui->update();
}

/////////////////////////////////////
// compute_baseline()
/////////////////////////////////////
void
compute_baseline(CLIST<GESTUREptr> & gest_list,
                  PIXEL & p,
                  VEXEL& v)
{
   if(gest_list.empty())
      return;

   double x_min = DBL_MAX;
   double y_min = DBL_MAX;
   double x_max = -DBL_MAX;
   double y_max = -DBL_MAX;

   for (int i=0; i<gest_list.num(); i++) {

      PIXEL_list pix_pts = gest_list[i]->pts();  

      for (int j = 0; j < pix_pts.num(); j++) {
         if ((pix_pts[j])[0] < x_min)
            x_min = (pix_pts[j])[0];
         if ((pix_pts[j])[0] > x_max) 
            x_max = (pix_pts[j])[0];

         if ((pix_pts[j])[1] < y_min)
            y_min = (pix_pts[j])[1];
         if ((pix_pts[j])[1] > y_max)
            y_max = (pix_pts[j])[1];
      }
   }

   double y_mid = y_min + ((y_max - y_min)/2.0);

   PIXEL baseline_begin = PIXEL(x_min, y_mid);
   PIXEL baseline_end   = PIXEL(x_max, y_mid);
   VEXEL baseline_vec   = baseline_end - baseline_begin;

   // return the start point and direction of baseline

   p = baseline_begin;
   v = baseline_vec;
}

/////////////////////////////////////
// generate_offsets()
/////////////////////////////////////
BaseStrokeOffsetLISTptr
get_offsets(
   CLIST<GESTUREptr> &gest_list,
   OutlineStroke* s) 
{
   static float MIN_OFFSET_DELTA_T = 0.001f;
   assert(gest_list.num());

   BaseStrokeOffsetLISTptr ret = new BaseStrokeOffsetLIST;   assert (ret);

   //Use selected baseline
   if (s)
   {
      assert(s->get_verts().num() >= 2);

      for (int j=0; j<gest_list.num(); j++) 
      {
         CPIXEL_list   &pix_pts = gest_list[j]->pts();         assert(pix_pts.num() >= 2);
         CARRAY<double> presses = gest_list[j]->pressures();   assert(pix_pts.num() == presses.num());

         NDCZpt_list pts;
         for (int i=0; i<pix_pts.num(); i++) pts += NDCZpt(pix_pts[i]); 
         

         BaseStrokeOffsetLISTptr os = s->generate_offsets(pts, presses); 

         if (os == NULL)
         {
            cerr << "LinePen::get_offsets - WARNING! Stroke returned no offsets...\n";
            continue;
         }

         for (int k=0; k<os->num(); k++) (*ret) += (*os)[k];

         if (!ret->get_pix_len()) ret->set_pix_len(os->get_pix_len());
      }
   }
   //Otherwise, use the virutal baseline
   else
   {
      PIXEL start_p;
      VEXEL baseline_vec;

      compute_baseline(gest_list, start_p, baseline_vec);
   
      // Sanity check: vector should be horizontal
      assert (baseline_vec[1] == 0);

      double mid_y        = start_p[1];
      double baseline_len = baseline_vec.length();

      if (!baseline_len) 
      {
         cerr << "LinePen::get_offsets - WARNING! Zero length baseline...\n";
      }
      else
      {
         for (int j=0; j<gest_list.num(); j++) 
         {

            CPIXEL_list   &pix_pts = gest_list[j]->pts();         assert(pix_pts.num() >= 2);
            CARRAY<double> presses = gest_list[j]->pressures();   assert(pix_pts.num() == presses.num());

            BaseStrokeOffset prev_o;  // for comparing distances
                                      // between successive offsets

            bool first_offset_set = false;
      
            for (int k = 0; k < pix_pts.num(); k++) 
            {
               double t = ((pix_pts[k])[0] - start_p[0])/baseline_len;
               double l = (pix_pts[k])[1] - mid_y;

               BaseStrokeOffset cur_o;
               cur_o._len = l;
               cur_o._pos = t;
               cur_o._press = presses[k];

               // Must enforce minimum t between successive offsets...
               if (ret->empty() || 
                   fabs(cur_o._pos - prev_o._pos) >= MIN_OFFSET_DELTA_T) 
               {
                  if (!first_offset_set) 
                  {
                     first_offset_set = true;
                     cur_o._type = BaseStrokeOffset::OFFSET_TYPE_BEGIN;
                  }
                  else 
                  {
                     cur_o._type = BaseStrokeOffset::OFFSET_TYPE_MIDDLE;
                  }

                  (*ret) += cur_o;         
               }
               prev_o = cur_o;
            }

            if (ret->last()._type == BaseStrokeOffset::OFFSET_TYPE_MIDDLE)
            {
               ret->last()._type = BaseStrokeOffset::OFFSET_TYPE_END;
            }
            //We skipped all the offets except the first one...
            //Add the last one back -- unless it's identical to the start...
            else
            {
               assert(ret->last()._type == BaseStrokeOffset::OFFSET_TYPE_BEGIN);
               if (prev_o._pos - ret->last()._pos >= gEpsZeroMath)
               {
                  prev_o._type = BaseStrokeOffset::OFFSET_TYPE_END;
                  (*ret) += prev_o;         
               }
               else
               {
                  cerr << "LinePen::get_offsets - WARNING! NEGLIGABLE length baseline...\n";
                  ret->pop();
               }
            }
         }
         ret->set_pix_len(baseline_len);
      }
   }

   if (ret->num() == 0)
   {
      cerr << "LinePen::get_offsets - WARNING!! No offets generated.\n";
      ret = NULL;
   }

   return ret;
}


/////////////////////////////////////
// button_edit_offset_apply()
/////////////////////////////////////
void
LinePen::button_edit_offset_apply()
{
   assert(!easel_is_empty());

   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      assert(_curr_pool->get_prototype());

      BaseStrokeOffsetLISTptr os = get_offsets(_gestures, ((_virtual_baseline)?(NULL):(_curr_stroke)));
      if (os == NULL)
      {
         WORLD::message("Failed to produce offsets from sketch");
         return;
      }
      else
      {
         easel_clear();
      }
      assert(os->num() >= 2);

      os->set_replicate(true);
      os->set_hangover(false);
      os->set_manual_t(true);

      OutlineStroke o;

      o.set_propagate_offsets(true);
      o.set_propagate_mesh_size(true);
      o.set_propagate_patch(false);

      COutlineStroke *op;

      op = retrieve_active_prototype();   assert(op);
      assert(op->class_name() == OutlineStroke::static_name());
      assert(op->get_patch() == _curr_tex->patch());

      o.copy(*op);
      o.set_offsets(os);
      o.set_original_mesh_size(_view->pix_to_ndc_scale() * _curr_tex->patch()->pix_size());

      modify_active_prototype(&o);

      WORLD::message("Offsets applied to Lines.");
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);

      BaseStrokeOffsetLISTptr os = get_offsets(_gestures, ((_virtual_baseline)?(NULL):(_curr_stroke)));
      if (os == NULL)
      {
         WORLD::message("Failed to produce offsets from sketch");
         return;
      }
      else
      {
         easel_clear();
      }
      assert(os->num() >= 2);

      os->set_replicate(false);
      os->set_hangover(true);

      OutlineStroke o;

      o.set_propagate_offsets(true);
      o.set_propagate_mesh_size(false);

      COutlineStroke *op;

      op = retrieve_active_prototype();   assert(op);
      assert(op->class_name() == EdgeStroke::static_name());

      o.copy(*op);
      o.set_offsets(os);

      modify_active_prototype(&o);

      WORLD::message("Offsets applied to Creases.");
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(!_curr_stroke);

////

{
   for (int i=0; i<_gestures.num(); i++) 
   {
      const  PIXEL_list&   pix   = _gestures[i]->pts();
      const ARRAY<double>& press = _gestures[i]->pressures();
  
      PIXEL_list   filtered_pix;
      ARRAY<double> filtered_press;

      assert(pix.num() == press.num());

      // No duplicate pixels at all, damn it!
      for (int l=0; l<pix.num(); l++) 
         if (filtered_pix.add_uniquely(pix[l])) filtered_press.add(press[l]);

      // must have at least 2 gesture vertices
      if(filtered_pix.num() < 2) continue;

      OutlineStroke *s = _curr_pool->get_stroke(); 
      assert(s && s->is_of_type(DecalLineStroke::static_name()));
      DecalLineStroke *ds = (DecalLineStroke*)s;
      assert(ds->get_offsets() == NULL);

      ds->clear();

      set_decal_stroke_verts(filtered_pix, filtered_press, ds);
   
      if (ds->num_vert_locs() < 2) 
      {
         cerr << "LinePen::draw_decal() - Decal with < 2 verts... Discarding...\n";
         _curr_pool->remove_stroke(ds);
      }
      else
      {
         apply_undo_prototype();

         perform_selection(_curr_tex, _curr_pool, ds, _curr_mode);

         _ui->apply_stroke();
   
         store_undo_prototype();

      }
   }
}

////

      easel_clear();

      WORLD::message("Decals applied to surface.");
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);

      //Shouldn't be able to press this button right now!
      assert(0);
   }   

   _ui->update();
  

}

/////////////////////////////////////
// button_edit_style_apply()
/////////////////////////////////////
void
LinePen::button_edit_style_apply()
{
   //XXX - Just sanity checks
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      assert(_curr_pool->get_prototype());
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);

      //Shouldn't be able to press this button right now!
      assert(0);
   }   

   _ui->apply_stroke();
   store_undo_prototype();

   WORLD::message("Stroke style applied.");
}

/////////////////////////////////////
// button_edit_style_get()
/////////////////////////////////////
void
LinePen::button_edit_style_get()
{
   //XXX - Just sanity checks
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      assert(_curr_pool->get_prototype());
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);

      //Shouldn't be able to press this button right now!
      assert(0);
   }   
   

   _ui->update_stroke();
}

/////////////////////////////////////
// button_edit_stroke_add()
/////////////////////////////////////
void
LinePen::button_edit_stroke_add()
{

   assert(_curr_mode == EDIT_MODE_CREASE);
   assert(_curr_tex);
   assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
   assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

   OutlineStroke *s = _curr_pool->get_stroke();   assert(s);

   s->clear();
   assert(s->is_of_type(EdgeStroke::static_name()));
   s->set_offsets(NULL);

   easel_clear();

   apply_undo_prototype();

   perform_selection(_curr_tex, _curr_pool, s, _curr_mode);

   _ui->apply_stroke();
   
   store_undo_prototype();

   _ui->update();

   WORLD::message("Crease stroke added.");
}

/////////////////////////////////////
// button_edit_stroke_del()
/////////////////////////////////////
void
LinePen::button_edit_stroke_del()
{
   assert(_curr_mode == EDIT_MODE_CREASE);
   assert(_curr_tex);
   assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
   assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));

   OutlineStroke *s = _curr_stroke;   assert(s);
   assert(s->is_of_type(EdgeStroke::static_name()));

   easel_clear();

   apply_undo_prototype();

   perform_selection(_curr_tex, _curr_pool, NULL, _curr_mode);

   store_undo_prototype();

   _curr_pool->remove_stroke(s);

   _ui->update();

   WORLD::message("Crease stroke removed.");
}

/////////////////////////////////////
// button_edit_synth_rubber()
/////////////////////////////////////
void
LinePen::button_edit_synth_rubber()
{

}

/////////////////////////////////////
// button_edit_synth_synthesize()
/////////////////////////////////////
void
LinePen::button_edit_synth_synthesize()
{

}

/////////////////////////////////////
// button_edit_synth_ex_add()
/////////////////////////////////////
void
LinePen::button_edit_synth_ex_add()
{

}

/////////////////////////////////////
// button_edit_synth_ex_del()
/////////////////////////////////////
void
LinePen::button_edit_synth_ex_del()
{

}

/////////////////////////////////////
// button_edit_synth_ex_clear()
/////////////////////////////////////
void
LinePen::button_edit_synth_ex_clear()
{

}

/////////////////////////////////////
// button_edit_synth_all_clear()
/////////////////////////////////////
void
LinePen::button_edit_synth_all_clear()
{

}

/////////////////////////////////////
// *_cb
/////////////////////////////////////

int
LinePen::line_cb(CGESTUREptr &g, DrawState*&)
{
   cerr << "LinePen::line_cb()" << endl;
   return create_stroke(g);
}

int
LinePen::slash_cb(CGESTUREptr &g, DrawState*&)
{
   cerr << "LinePen::slash_cb()" << endl;
   return create_stroke(g);
}

int
LinePen::scribble_cb(CGESTUREptr &g, DrawState*&)
{
   err_msg("LinePen::scribble_cb()");
   return create_stroke(g);
}

int
LinePen::lasso_cb(CGESTUREptr &g, DrawState*&)
{
   err_msg("LinePen::lasso_cb()");
   return create_stroke(g);
}

int
LinePen::stroke_cb(CGESTUREptr &g, DrawState*&)
{
   cerr << "LinePen::stroke_cb()" << endl;
   return create_stroke(g);
}


/////////////////////////////////////
// create_stroke()
/////////////////////////////////////
int
LinePen::create_stroke(CGESTUREptr &g)
{
   bool accept_gesture = false;

   //Sanity checks, mostly...
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      assert(_curr_pool->get_prototype());
      
      accept_gesture = true;
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      
      if (_curr_stroke) accept_gesture = true;
   }
   else if (_curr_mode == EDIT_MODE_DECAL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      
      //If a decal's selected -- unselect it...
      if (_curr_stroke)
      {
         assert(easel_is_empty());
         apply_undo_prototype();
         perform_selection(_curr_tex, _curr_pool, NULL, _curr_mode);
         //store_undo_prototype();
      }
      accept_gesture = true;
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_NONE);

      assert(!_curr_tex);
      assert(!_curr_pool);
      assert(!_curr_stroke);
   }   

   if (accept_gesture)
   {

      //See if we like the gesture
   
      bool foo = easel_is_empty();

      easel_add(g);

      easel_update_baseline();

      if (foo) _ui->update();

      return 1;
   }
   else
   {
      //Nothing...

      assert(easel_is_empty());

      return 0;
   }

}

/////////////////////////////////////
// notify() [camera obs callback]
/////////////////////////////////////
void 
LinePen::notify(CCAMdataptr &)
{
   VIEW_EASELptr e = BaseJOTapp::instance()->easels().cur_easel();
   
   if (_gestures.num() == 0)
   {
      assert((e == NULL) || (e->lines().num() == 0));
   }
   else
   {
      if (e != NULL)
      {
         assert(e->lines().num() == (_gestures.num()+1));

         while(!_gestures.empty()) 
         {
            e->rem_line(_gestures.pop());
         }   
         e->rem_line(_baseline_gel);

         assert(e->lines().num() == 0);
      }
      else
      {
         _gestures.clear();
      }
      _ui->update();
   }
}

/////////////////////////////////////
// easel_add()
/////////////////////////////////////
void
LinePen::easel_add(CGESTUREptr &g)
{
   VIEW_EASELptr e = BaseJOTapp::instance()->easels().cur_easel();

   if (_gestures.num() == 0)
   {
      if (e == NULL)
      {
         BaseJOTapp::instance()->easels().make_new_easel(_view);
         e = BaseJOTapp::instance()->easels().cur_easel();  assert(e != NULL);
      }
      else
      {
         assert(e->lines().num() == 0);
      }
      e->add_line(_baseline_gel);
      if (_curr_mode == EDIT_MODE_SIL)
         WORLD::message("Sketching Lines.");
      else if (_curr_mode == EDIT_MODE_CREASE)
         WORLD::message("Sketching Creases.");
      else if (_curr_mode == EDIT_MODE_DECAL)
         WORLD::message("Sketching Decals.");
      else assert(0);
   }
   else
   {
      assert(e != NULL);
      assert(e->lines().num() == (_gestures.num() + 1));
   }

   _gestures += g;
   e->add_line(g);

}

/////////////////////////////////////
// easel_is_empty()
/////////////////////////////////////
bool
LinePen::easel_is_empty()
{
   VIEW_EASELptr e = BaseJOTapp::instance()->easels().cur_easel();
   
   if (_gestures.num() == 0)
   {
      assert((e == NULL) || (e->lines().num() == 0));
      return true;
   }
   else
   {
      assert(e != NULL);
      assert(e->lines().num() == (_gestures.num() + 1));
      return false;
   }

}

/////////////////////////////////////
// easel_clear()
/////////////////////////////////////
void
LinePen::easel_clear()
{
   VIEW_EASELptr e = BaseJOTapp::instance()->easels().cur_easel();
   
   if (_gestures.num() == 0)
   {
      assert((e == NULL) || (e->lines().num() == 0));
   }
   else
   {
      assert(e != NULL);
      assert(e->lines().num() == (_gestures.num()+1));

      while(!_gestures.empty()) 
      {
         e->rem_line(_gestures.pop());
      }   
      e->rem_line(_baseline_gel);

      assert(e->lines().num() == 0);
   }
}

/////////////////////////////////////
// easel_pop()
/////////////////////////////////////
void
LinePen::easel_pop()
{
   VIEW_EASELptr e = BaseJOTapp::instance()->easels().cur_easel();
   
  (_gestures.num() > 0);

   assert(e != NULL);
   assert(e->lines().num() == (_gestures.num()+1));

   e->rem_line(_gestures.pop());

   if (_gestures.num() == 0)
   {
      assert(e->lines().num() == 1);
      e->rem_line(_baseline_gel);
      assert(e->lines().num() == 0);
   }
   else
   {
      assert(e->lines().num() == (_gestures.num()+1));
   }
}

/////////////////////////////////////
// easel_update_baseline()
/////////////////////////////////////
void
LinePen::easel_update_baseline()
{
   LIST<GESTUREptr> tmp_gestures;

   VIEW_EASELptr e = BaseJOTapp::instance()->easels().cur_easel();
   
   assert(_gestures.num() != 0);

   assert(e != NULL);
   assert(e->lines().num() == (_gestures.num()+1));


   OutlineStroke *s;

   //XXX - Lottsa sanity checks
   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      s = _curr_stroke;
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(_curr_stroke);
      s = _curr_stroke;
   }
   else 
   {
      assert(_curr_mode == EDIT_MODE_DECAL);
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      assert(!_curr_stroke);
      s = NULL;
   }

   if (_curr_mode == EDIT_MODE_DECAL)
   {
      _baseline_gel->pts().clear();
   }
   else
   {
      if (_virtual_baseline || !s)
      {
         _baseline_gel->pts().clear();
         
         PIXEL start_p;
         VEXEL baseline_vec;

         compute_baseline(_gestures, start_p, baseline_vec);
         assert (baseline_vec[1] == 0);

         if (baseline_vec.length() > 1)
         {
            _baseline_gel->pts().add(start_p - 0.1 * baseline_vec);
            _baseline_gel->pts().add(start_p + 0.5 * baseline_vec);
            _baseline_gel->pts().add(start_p + 1.1 * baseline_vec);
         }
      }
      else
      {
         assert(s);
         const StrokeVertexArray&  vs = s->get_verts();

         _baseline_gel->pts().clear();

         for (int i=0; i < vs.num(); i++)
            _baseline_gel->pts().add(vs[i]._base_loc);
         
      }
   }


}

/////////////////////////////////////
// easel_populate()
/////////////////////////////////////
void
LinePen::easel_populate()
{
   
   OutlineStroke *s;

   if (!_virtual_baseline)
   {
      WORLD::message("Must use Virtual Baseline setting.");
      return;
   }

   if (_curr_mode == EDIT_MODE_SIL)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));
      s = _curr_pool->get_prototype();
   }
   else if (_curr_mode == EDIT_MODE_CREASE)
   {
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == EdgeStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      s = _curr_stroke;
   }
   else
   {
      assert(_curr_mode == EDIT_MODE_DECAL);
      assert(_curr_tex);
      assert(_curr_pool && (_curr_pool->class_name() == DecalStrokePool::static_name()));
      assert(!_curr_stroke || (_curr_pool->get_selected_stroke() == _curr_stroke));
      //Shouldn't happen at all, really...
      assert(0);
   }

   assert(s && s->get_offsets() != NULL);

   CBaseStrokeOffsetLISTptr ol = s->get_offsets();

   double pix_len = ol->get_pix_len();

   PIXEL center(XYpt(0,0));

   double x_vec = center[0] - pix_len/2.0;
   double y_vec = center[1];

   GESTUREptr g = NULL;

   for (int i=0; i<ol->num(); i++)
   {
      
      CBaseStrokeOffset &o = (*ol)[i];

      if (o._type == BaseStrokeOffset::OFFSET_TYPE_BEGIN)
      {
         assert(g == NULL);
         g = new GESTURE(_gest_int,_gestures.num(),PIXEL(x_vec+pix_len*o._pos,y_vec+o._len),o._press,_gesture_drawer);
      }
      else if (o._type == BaseStrokeOffset::OFFSET_TYPE_MIDDLE)
      {
         assert(g != NULL);
         g->add(PIXEL(x_vec+pix_len*o._pos,y_vec+o._len), 0, o._press);
      }
      else
      {
         assert(g != NULL);
         assert(o._type == BaseStrokeOffset::OFFSET_TYPE_END);
         g->add(PIXEL(x_vec+pix_len*o._pos,y_vec+o._len), 0, o._press);
         //g->complete(PIXEL(x_vec+pix_len*o._pos,y_vec+o._len));
         easel_add(g);
         g = NULL;
      }
   }
   assert(g == NULL);

   assert(!easel_is_empty());

}


/////////////////////////////////////
// key()
/////////////////////////////////////
void
LinePen::key(CEvent &e)
{
   switch(e._c)
   {
      case '\x1b':      //'ESC'
         cerr << "LinePen::key - Key press 'ESC'\n";
         if(!easel_is_empty())
         {
            easel_pop();
            
            if (!easel_is_empty()) 
               easel_update_baseline();
            else 
               _ui->update();
         }
      break;
      case 'y':      //'y'
         cerr << "LinePen::key - Key press 'y'\n";
         if(!easel_is_empty())
         {
            button_edit_offset_apply();
         }
      break;
      case 'u':      //'u'
         cerr << "LinePen::key - Key press 'u'\n";
         if ((_curr_mode == EDIT_MODE_SIL) || 
               (((_curr_mode == EDIT_MODE_CREASE) || (_curr_mode == EDIT_MODE_DECAL)) && (_curr_stroke)) )
         {
            button_edit_style_apply();
         }
      break;
      case 'h':      //'h'
         cerr << "LinePen::key - Key press 'h'\n";
         if((_curr_mode == EDIT_MODE_SIL) || (_curr_mode == EDIT_MODE_CREASE))
         {
            COutlineStroke *s = retrieve_active_prototype();
            if (s && (s->get_offsets() != NULL))
            {
               button_edit_offset_edit();
            }
         }
      break;
      case 'j':      //'j'
         cerr << "LinePen::key - Key press 'j'\n";
         if ((_curr_mode == EDIT_MODE_SIL) || 
               (((_curr_mode == EDIT_MODE_CREASE) || (_curr_mode == EDIT_MODE_DECAL))&& (_curr_stroke)) )
         {
            button_edit_style_get();
         }
      break;
      default:
         cerr << "LinePen::key - Unknown key '" << e._c << "'\n";
      break;
   }
 

}



//////////////////////////////////////////////
// decal building helpers
/////////////////////////////////////////////


//////////////////////////////////////////////
// intersect_edge()
/////////////////////////////////////////////

static bool 
intersect_edge(
   Bedge* e, 
   CNDCpt &p1, 
   CNDCpt &p2,
   Wpt& surf)
{
   assert(e);
   CWtransf& xf = e->mesh()->xform();

   // push p1 and p2 out into world space; form a plane containing them and the eye,
   // and get its normal vec, n. 
 
   Wpt w1 = xf * e->v1()->loc();
   Wpt w2 = xf * e->v2()->loc();
 
   Wpt wp1(p1, w1);
   Wpt wp2(p2, w1);

   NDCpt inter;
   bool success = e->ndc_intersect(p1, p2, inter);       

   Wpt eye = VIEW::peek()->cam()->data()->from();
 
   Wvec n = cross(wp1 - eye, wp1 - wp2);
   n = n / n.length();
 
   // OK, we're looking for the point of the form w1 + a (w2 - w1) that's
   // on this plane. We know that p1 is on it, so the plane eqn is n . X = n . p1
   // Hence we solve n . (w1 + a(w2 - w1)) = n . p1
   // a (w2 - w1) . n = n(p1 - w1), ie. 
 
   double d = n * (w2 - w1);
   if (fabs(d) < .0001) {
      return false;
   }
 
   double a = (n * (wp1 - w1)) / d;
   if ((a < 0.0) || (a > 1.0)) {
      return false;
   } 

   surf = w1 + a* (w2 - w1);

   CWtransf& inv_xf = e->mesh()->inv_xform();

   // transform intersection point to object space
   surf = inv_xf * surf;

   return success;
}

//////////////////////////////////////////////
// interpolate_pressure()
/////////////////////////////////////////////

double
interpolate_pressure(
   CNDCpt p1, CNDCpt p2, 
   double press1, double press2,
   CNDCpt inter)
{
   double delt = NDCvec(p2-p1).length();

   double dist = NDCvec(inter-p1).length();

   double t = dist/delt;
   double result = press1 + (t * (press2 - press1));
   return result;
}

//////////////////////////////////////////////
// set_decal_stroke_verts()
/////////////////////////////////////////////

void
LinePen::set_decal_stroke_verts(
   CPIXEL_list& pix, 
   const ARRAY<double>& press,
   DecalLineStroke* stroke)
{
   assert(stroke);
   assert (pix.num() == press.num());

   BaseVisRefImage *vis_ref = BaseVisRefImage::lookup(VIEW::peek()); assert(vis_ref);

   // Set decal-stroke vertices on the surface of the mesh by
   // projecting the pixel locations of the gesture vertices onto the
   // mesh triangles; if adjacent gesture pixel points p1, p2 do not
   // project onto the same face, we set additional decal-stroke
   // vertices at the intersection points of the screen-space edge
   // given by p1 and p2 with any mesh edges of the visible portions of 
   // the mesh surface.
    
   for(int i = 0; i < pix.num()-1; i++) 
   {    
      if (pix[i] == pix[i+1]) 
      {
         cerr << "LinePen::set_decal_stroke_verts() - Duplicate gesture pixels... skipping...\n";
         continue;
      }

      NDCpt p1 = pix[i];
      NDCpt p2 = pix[i+1];

      double press1 = press[i];
      double press2 = press[i+1];
      double inter_press = 0.0;
      
      Wpt sp1, sp2; // projections of p1 and p2, respectively, onto mesh surface

      Bface *f1 = NULL;
      Bface *f2 = NULL;

      f1 = vis_ref->vis_intersect(p1, sp1);
      
      if(!f1) return;

      f2 = vis_ref->vis_intersect(p2, sp2);
      
      if(!f2) return;

      // sanity check
      assert(f1 && f2);

      // drop a point for the first pixel
      if (!stroke->add_vert_loc(sp1,f1, press1)) return;

      if(f1 == f2) continue;

      int j = 0;  // loop index

      Wpt surf;

      // see if faces share an edge
      Bedge * e = f1->shared_edge(f2);
      if (e) 
      {
         // find crossing point on shared edge
         if (intersect_edge(e, p1, p2, surf)) 
         {
            inter_press = interpolate_pressure(p1, p2, press1, press2, surf);
            
            if(!stroke->add_vert_loc(surf,e, inter_press)) return;
         }
      }
      else 
      {  // faces are not adjacent, so try to connect by walking
         Bedge* cur_edge = 0;
         Bface* cur_face = 0;

         // find first edge crossed
         for(j = 1; j < 4; j++) 
         {
            assert(f1->e(j));
            if(intersect_edge(f1->e(j), p1, p2, surf))
            {
               cur_edge = f1->e(j);

               inter_press = interpolate_pressure(p1, p2, press1, press2, surf);

               if(!stroke->add_vert_loc(surf, cur_edge, inter_press)) return;
               break;   
            }
         }
         if (!cur_edge) 
         {
            cerr << "LinePen::set_decal_stroke_verts() - Walking, no first edge found, aborting...\n";
            return;
            //break;
         }

         cur_face = cur_edge->other_face(f1);
         assert(cur_face && cur_face != f2);
        
         while(cur_edge && cur_face != f2) 
         {
            Bedge* next_edge = 0;
          
            for(j = 1; j < 4; j++) 
            {
               assert(cur_face->e(j));
               if((cur_face->e(j) != cur_edge) && intersect_edge(cur_face->e(j), p1, p2, surf))
               {
                  next_edge = cur_face->e(j);
                  inter_press = interpolate_pressure(p1, p2, press1, press2, surf);
                  
                  if(!stroke->add_vert_loc(surf, next_edge, inter_press))return;      
                  break;   
               }
            } 
            if(next_edge == 0) break;

            cur_edge = next_edge;
            cur_face = cur_edge->other_face(cur_face);
            assert(cur_face);
         }
        
         if (cur_face != f2) 
         {
            // This would happen, for example, if gesture crossed a silhoutte 
            cerr << "LinePen::set_decal_stroke_verts() -  Walk failed! Could not reach face 2. Breaking stroke...\n";
            return;
         }
      }

   }

   // XXX -- TO DO:  Add last decal stroke vertex by intersecting last gesture pixel point
   // XXX ?!?!?!?!?!?!?!? (rdk)
}

//////////////////////////////////////////////
// selection_changed()
/////////////////////////////////////////////
void           
LinePen::selection_changed(line_pen_selection_t t)
{
//   NPRTexture*    old_tex     = _curr_tex;
//   BStrokePool*   old_pool    = _curr_pool;
   OutlineStroke* old_stroke  = _curr_stroke;
//   edit_mode_t    old_mode    = _curr_mode;

   switch(t)
   {
      case LINE_PEN_SELECTION_CHANGED__SIL_TRACKING:
         assert(_curr_tex);
         assert(_curr_pool && (_curr_pool->class_name() == SilStrokePool::static_name()));

         _curr_stroke = _curr_pool->get_selected_stroke();

         if ((old_stroke) && (!_curr_stroke))
         {
            if(!easel_is_empty())
               easel_update_baseline();
            _ui->update();
            WORLD::message("Failed to track stroke.");
         }
         else if (_curr_stroke)
         {
            if(!easel_is_empty())
               easel_update_baseline();
         }
      break;
      default:

      break;
   }  
}

/* end of file line_pen.C */
