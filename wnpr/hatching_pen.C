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
// HatchingPen
////////////////////////////////////////////
//
// -Pen class which manages hatching groups
// -Talks to GLUI widget HatchingPenUI
// -Sets patches to HatchStokeTexture (soon to change name)
// -Talks to HatchingCollection in the texture
//
////////////////////////////////////////////



#include "npr/npr_texture.H"
#include "npr/hatching_group.H"

#include "hatching_pen.H"

#include "hatching_pen_ui.H"

using mlib::NDCpt;
using mlib::NDCpt_list;
using mlib::XYpt;

/////////////////////////////////////
// Constructor
/////////////////////////////////////

HatchingPen::HatchingPen(
   CGEST_INTptr &gest_int,
   CEvent& d, CEvent& m, CEvent& u,
   CEvent& shift_down, CEvent& shift_up,
   CEvent& ctrl_down,  CEvent& ctrl_up) :
   Pen(str_ptr("Hatching"), 
       gest_int, d, m, u,
       shift_down, shift_up, 
       ctrl_down, ctrl_up)
{
                
   // gestures we recognize:
   _draw_start += DrawArc(new TapGuard,      drawCB(&HatchingPen::tap_cb));
   _draw_start += DrawArc(new SlashGuard,    drawCB(&HatchingPen::slash_cb));
   _draw_start += DrawArc(new LineGuard,     drawCB(&HatchingPen::line_cb));
   _draw_start += DrawArc(new ScribbleGuard, drawCB(&HatchingPen::scribble_cb));
   _draw_start += DrawArc(new LassoGuard,    drawCB(&HatchingPen::lasso_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&HatchingPen::stroke_cb));

   // let's us override the drawing of gestures:

   _gesture_drawer = new GestureStrokeDrawer();
   HatchingStroke *h = new HatchingStroke;
   assert(h);
   h->set_vis(VIS_TYPE_SCREEN);

   _gesture_drawer->set_base_stroke_proto(h);
        
   _prev_gesture_drawer = 0;

   // ui vars:
   _curr_hatching_group = 0;
   _curr_create_type = HatchingGroup::FIXED_HATCHING_GROUP;
   _curr_curve_mode = HatchingGroup::CURVE_MODE_PLANE;

   _curr_npr_texture = 0;

   _ui = new HatchingPenUI(this);
   assert(_ui);


}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingPen::~HatchingPen()
{

   if(_ui)
      delete _ui;

   if(_gesture_drawer)
      delete _gesture_drawer;
}

/////////////////////////////////////
// activate()
/////////////////////////////////////
//
// -Init the GLUI box
// -Init gesture_drawer
// -Save some time (kill floor, set gtex)
//
/////////////////////////////////////

void 
HatchingPen::activate(State *s)
{

   if(_ui)
      _ui->show();

   Pen::activate(s);

   if(_gest_int && _gesture_drawer)
   {
      _prev_gesture_drawer = _gest_int->drawer();
      _gest_int->set_drawer(_gesture_drawer);   
   }

   //Change to patch's desired textures
   if (_view)
     _view->set_rendering(GTexture::static_name());       

   if(_ui)
      _ui->update();

}

/////////////////////////////////////
// deactivate()
/////////////////////////////////////
//
// -Hide the GLUI box
// -Clean up gesture_drawers
//
/////////////////////////////////////

bool
HatchingPen::deactivate(State* s)
{

   if (_curr_hatching_group)
   {
      if(!_curr_hatching_group->is_complete()) 
      {
         WORLD::message("Cannot change mode until group is complete.");
         return false;
      }
      else
      {
         deselect_current();
      }
   }
   else
      select_tex(0);

   if(_ui) _ui->hide();

   bool ret = Pen::deactivate(s);
   assert(ret);

   if(_gest_int && _prev_gesture_drawer)
   {
      _gest_int->set_drawer(_prev_gesture_drawer);   
      _prev_gesture_drawer = 0;
   }
   return true;

}


#include "mesh/uv_data.H"

/////////////////////////////////////
// key()
/////////////////////////////////////

void
HatchingPen::key(CEvent &e)
{
   Bface* f;

   switch(e._c)
   {
      case '\x1b':      //'ESC'
         cerr << "HatchingPen::key - Key press 'ESC'\n";
         undo_last();
      break;
      case '~': // XXX - Delete UV!!
         f = VisRefImage::get_face();

         if (f)
         {
            if (UVdata::has_uv(f))
            {
               cerr << "HatchingPen::key(~) - Clearing UV from.\n";
               f->rem_simplex_data(UVdata::lookup(f));
               f->mesh()->changed();

            }
            else
            {
               cerr << "HatchingPen::key(~) - No  UV to clear from face.\n";
            }
         }
         else
         {
            cerr << "HatchingPen::key(~) - No face to clear UV from.\n";
         }

      break;
      default:
         cerr << "HatchingPen::key - Unknown key '" << e._c << "'\n";
   }



}

/////////////////////////////////////
// tap()
/////////////////////////////////////
//
// Serves two purposes:
//
// -If current selected group is 'incomplete' then complete it
// -Else deselect (if possible) and select under the tap
//
/////////////////////////////////////


HatchingGroup*
dup_group(HatchingGroup* g)
{
   if (!g) {
      err_msg("dup_group: error: group is null");
      return 0;
   }

   // choose a name for a tmp file:
   str_ptr tmp("tmp.jot");

   {
      // using braces so the stream closes
      // when it goes out of scope...

      // open a stream to write to the tmp file:
      NetStream out(tmp, NetStream::ascii_w);
      if (out.fd() < 0) {
         err_ret("dup_group: error: can't open %s for writing", **tmp);
         return 0;
      }

      // write g to file:
      out << *g;
      *out.ostr() << endl << endl; // XXX - need this or it's effed
   }

   // open a stream to read from the tmp file:
   NetStream in(tmp, NetStream::ascii_r);
   if (in.fd() < 0) {
      err_ret("dup_group: error: can't open %s for reading", **tmp);
      return 0;
   }

   // create a new instance of the same type as g:
   HatchingGroup* ret = (HatchingGroup*)g->dup();
   if (!ret) {
      err_ret("dup_group: error: dup failed");
      return 0;
   }

   // XXX - need this or it crashes
   ret->set_patch(g->patch());

   // read the class name of g:
   str_ptr str; in >> str;     

   // read the rest of the file into the
   // new instance and return it:
   ret->decode(in);
   return ret;
}

int
HatchingPen::tap_cb(CGESTUREptr& gest, DrawState*&)
{
   //cerr << "HatchingPen::tap_cb" << endl;

   Bface *f;

   //Debugging stuff
   // If a hatchgroup is in progress, then a tap means to 
   // complete the LOD.  If the group is neat, or the
   // current LOD is empty, then the group is completed.
   // But in case the artist is spastic, we only accept
   // taps that aren't on the model (or any other model)

   //f = VisRefImage::Intersect(gest->center());
   
   f = 0; //quick hack to fix the case where the scene fills the entire screen
		  //a more sophisticated fix may be needed

   if ( (_curr_hatching_group) && (!_curr_hatching_group->is_complete()) )
   {
      if (!f) 
      {
         cerr << "HatchingPen::Ending current HatchGroup Level of Detail\n";
         bool ret = _curr_hatching_group->complete();

         if (_curr_hatching_group->is_complete()) 
            {
               WORLD::message("Hatching group completed.");

               // XXX - debugging for ssubrama...
               if (Config::get_var_bool("DEBUG_SSUBRAMA")) 
               {
                  // can we make a copy that works?
                  HatchingGroup* copy = dup_group(_curr_hatching_group);

                  // to test if it works, remove the old group,
                  // then add the copy and see if it gets drawn...
                  //
                  // to add/remove need to get hold of the hatching
                  // collection:
                  Patch* p = _curr_hatching_group->patch();
                  GTexture* cur_texture = p->cur_tex();
                  if (cur_texture->is_of_type(NPRTexture::static_name())) {
                     NPRTexture* npr_tex = (NPRTexture*)cur_texture;
                     // first just take out the old one:
                     if (Config::get_var_bool("REMOVE_ORIG_GROUP"))
                        npr_tex->hatching_collection()->remove_group(
                           _curr_hatching_group
                           );
                     // after verifying the old one is gone,
                     // run it again and add the new one:
                     if (Config::get_var_bool("ADD_DUP_GROUP"))
                        npr_tex->hatching_collection()->add_group(copy);
                  
                  }
               }
         }
         else 
         {
            if (ret)
               WORLD::message("Level of detail completed. Tap again to complete group.");
            else
               WORLD::message("Couldn't be completed.");
         }
         return 0;
      }
      else
      {
         WORLD::message("'Tap' on background to complete.");
         return 0;
      }
   }

   //Otherwise, we interpret a tap as an attempt to 
   //select a hatch group. We query the list of hatches
   //for intersected patch and grab the next one
   //in the series...

   //Deselect on a miss...
   Patch* p = get_ctrl_patch(f);
   if (!(f && p))
   {
      if (_curr_hatching_group) deselect_current();
      else select_tex(0);
      return 0;
   }

   GTexture* cur_texture = p->cur_tex();
   if(cur_texture->is_of_type(NPRTexture::static_name()))
   {
      NPRTexture* hatch_stroke_texture = (NPRTexture*)cur_texture;
             
      HatchingGroup *hg = hatch_stroke_texture->hatching_collection()->
         next_group(NDCpt(XYpt(gest->center())), _curr_hatching_group);

      if (hg)
      {
         if (_curr_hatching_group!=hg)
         {
            if (_curr_hatching_group) deselect_current();
            select(hg,hatch_stroke_texture);
            get_parameters();
         }
      }
      else
      {
         select_tex(hatch_stroke_texture);
      }
   }

   return 0;
}

/////////////////////////////////////
// *_cb
/////////////////////////////////////
//
// All these gesture cb's call to generate_stroke()
// to add a hatch to current/new hatch group
//
/////////////////////////////////////

int
HatchingPen::line_cb(CGESTUREptr& gest, DrawState*&)
{
   //cerr << "HatchingPen::line_cb()" << endl;
   generate_stroke(gest); 
   return 0;
}

int
HatchingPen::slash_cb(CGESTUREptr& gest, DrawState*&)
{
   //cerr << "HatchingPen::slash_cb()" << endl;
   generate_stroke(gest); 
   return 0;
}


int
HatchingPen::scribble_cb(CGESTUREptr& gest, DrawState*&)
{
   //err_msg("HatchingPen::scribble_cb()");
   generate_stroke(gest); 
   return 0;
}

int
HatchingPen::lasso_cb(CGESTUREptr& gest, DrawState*&)
{
   //err_msg("HatchingPen::lasso_cb()");
   generate_stroke(gest);
   return 0;
}

int
HatchingPen::stroke_cb(CGESTUREptr& gest, DrawState*&)
{
   //cerr << "HatchingPen::stroke_cb()" << endl;
   generate_stroke(gest);
   return 0;
}

/////////////////////////////////////
// generate_stroke
/////////////////////////////////////
//
// -Change gtex of patch under stroke to NPRTexture
// -If a hatch group is deselected and incomplete, end this stroke
// -Else create a new hatchgroup and add this
//
/////////////////////////////////////

int
HatchingPen::generate_stroke(CGESTUREptr& gest)
{

   cerr << "HatchingPen::generate_stroke()\n";

   Bface* f = VisRefImage::Intersect(gest->center());
   if (!f) 
      {
         cerr << "HatchingPen - generate_stroke: Center not on a mesh!\n";               
         WORLD::message("Center of stroke not on a mesh");
         return 0;
      }

   if (gest->pts().num() < 2) {
      cerr    << "HatchingPen: Can't make hatch stroke with only " << gest->pts().num() << " samples...\n";
      WORLD::message("Failed to generate hatch stroke (too few samples)...");
      return 0;
   }

   Patch* p = get_ctrl_patch(f);       assert(p);
   p->set_texture(NPRTexture::static_name());
   GTexture* cur_texture = p->cur_tex();
   if(!cur_texture->is_of_type(NPRTexture::static_name())) return 0;
   NPRTexture* npr_texture = (NPRTexture*)cur_texture;

   bool new_group = false;

   if ((_curr_hatching_group)&&(!_curr_hatching_group->is_complete()))
   {
      // Hatch group in progress. Check if we fall on right mesh (patch)...
      if (_curr_hatching_group->patch() != p)
         {
            cerr << "HatchingPen:generate_stroke - Stroke doesn't fall on right patch!\n";          
            WORLD::message("Stroke lies on wrong patch.");
            return 0;
         }
   } 
   else
   {
      if (_curr_hatching_group) deselect_current();

      new_group = true;

      cerr << "HatchingPen::Creating new Hatch Group\n";

      HatchingGroup *hg = 
            npr_texture->hatching_collection()->add_group(_curr_create_type);
      assert(hg);
      select(hg,npr_texture);
      apply_parameters();
   }

   NDCpt_list ndcpts = gest->pts();
   if (!_curr_hatching_group->add(ndcpts,gest->pressures(),_curr_curve_mode))
   {
      if (new_group)
      {
         WORLD::message("Failed to create new hatch group...");
         destroy_current();
      }
      else 
         WORLD::message("Failed to add hatch stroke...");
   }
   else if (new_group)
      WORLD::message("Created new hatch group.");

   return 0;
}

/////////////////////////////////////
// apply_parameters()
/////////////////////////////////////
//
// Apply the current settings to the 
// selected hatchgroup's prototype
//
/////////////////////////////////////

void    
HatchingPen::apply_parameters()
{
   cerr << "HatchingPen:apply_parameters - Stylizing hatch group.\n";

   if (_curr_hatching_group)
      _curr_hatching_group->set_params(&_params);

   //In case we applied params to a non-complete
   //group, we must do the the following to assure
   //the remaining gestures use the group's params
   update_gesture_proto();

}

/////////////////////////////////////
// get_parameters()
/////////////////////////////////////
//
// Set current settings to those of
// selected hatchgroup's prototype
// and update the GLUI box
//
/////////////////////////////////////

void    
HatchingPen::get_parameters()
{
   cerr << "HatchingPen:get_parameters - Fill out widets with selected group's data.\n";

   if (_curr_hatching_group)
   {
      _params.copy(_curr_hatching_group->get_params());
      _curr_create_type = _curr_hatching_group->type();
      _ui->update();

   }
}

/////////////////////////////////////
// select_tex()
/////////////////////////////////////

void    
HatchingPen::select_tex(NPRTexture *nt)
{

   if (_curr_npr_texture)
      _curr_npr_texture->set_selected(false);

   if ((_curr_npr_texture = nt))
      _curr_npr_texture->set_selected(true);

}


/////////////////////////////////////
// select()
/////////////////////////////////////

void    
HatchingPen::select(HatchingGroup *hg, NPRTexture *nt)
{
   cerr << "HatchingPen:select - Selecting a hatch group.\n";

   assert(!_curr_hatching_group);
   _curr_hatching_group = hg;
   _curr_hatching_group->select();

   select_tex(nt);

}

/////////////////////////////////////
// deselect_current()
/////////////////////////////////////

void    
HatchingPen::deselect_current()
{
   cerr << "HatchingPen:deselect - Deselecting current hatch group.\n";

   if (_curr_hatching_group) _curr_hatching_group->deselect();
   _curr_hatching_group = 0;

   select_tex(0);
}

/////////////////////////////////////
// destroy_current()
/////////////////////////////////////

void    
HatchingPen::destroy_current()
{
   cerr << "HatchingPen:destroy_current - Destroying current hatch group.\n";

   assert(_curr_hatching_group);
        
   _curr_hatching_group->deselect();
        
   GTexture* cur_texture = _curr_hatching_group->patch()->cur_tex();
   assert(cur_texture->is_of_type(NPRTexture::static_name()));
   NPRTexture* hatch_stroke_texture = (NPRTexture*)cur_texture;

   bool ret = hatch_stroke_texture->hatching_collection()->
                                 delete_group(_curr_hatching_group);

   assert(ret);

   _curr_hatching_group = 0;

}

/////////////////////////////////////
// next_hatch_mode()
/////////////////////////////////////
void  
HatchingPen::next_hatch_mode()
{
   if (_curr_hatching_group)
   {
      if (_curr_hatching_group->is_complete())
      {
         cerr << "HatchingPen::next_hatch_mode - A completed group cannot change type (fixed/free).\n" <<
            "   While a complete group is selected, new groups will be 'attached'\n" <<
            "   to create cross-hatching.  All attached groups must be of the same type.\n";
         WORLD::message("Cannot change type. Deselect current group first...");
      }
      else
      {
         cerr << "HatchingPen::next_hatch_mode - A non-completed group cannot change type (fixed/free).\n";
         WORLD::message("Can't change type.");
      }
   }
   else
   {
      (++_curr_create_type)%=HatchingGroup::NUM_HATCHING_GROUP_TYPES;
   }
}

/////////////////////////////////////
// next_curve_mode()
/////////////////////////////////////
void  
HatchingPen::next_curve_mode()
{
   (++_curr_curve_mode)%=HatchingGroup::NUM_CURVE_MODE;
}

/////////////////////////////////////
// next_style_mode()
/////////////////////////////////////
void  
HatchingPen::next_style_mode()
{
   if (_curr_hatching_group)
   {
      if (_curr_hatching_group->is_complete())
      {
         cerr << "HatchingPen::next_hatch_mode - A completed group cannot change style (neat/sloppy).\n" <<
            "   While a complete group is selected, new groups will be 'attached'\n" <<
            "   to create cross-hatching.  All attached groups must be of the same style.\n";
         WORLD::message("Cannot change type. Deselect current group first...");
      }
      else
      {
         cerr << "HatchingPen::next_hatch_mode - A non-completed group cannot change type (fixed/free).\n";
         WORLD::message("Can't change type.");
      }
   }
   else
   {
      _params.anim_style((_params.anim_style()+1)% HatchingGroup::NUM_STYLE_MODE);
   }

    
}

/////////////////////////////////////
// delete_current()
/////////////////////////////////////
//
// Invoked by GLUI to kill hatchgroups
//
/////////////////////////////////////
void    
HatchingPen::delete_current()
{

   if (_curr_hatching_group) 
   {
      WORLD::message("Destroying current hatch group.");
      destroy_current();
   }
   else
   {
      WORLD::message("Nothing selected to destroy.");
   }

}

/////////////////////////////////////
// undo_last()
/////////////////////////////////////
//
// Invoked by GLUI to kill hatchgroups
//
/////////////////////////////////////
void    
HatchingPen::undo_last()
{

   if (_curr_hatching_group) 
   {
      if(_curr_hatching_group->is_complete())
      {
         WORLD::message("Can't 'Undo' on complete groups. Use 'Delete All'...");
      }
      else
      {
         if (!_curr_hatching_group->undo_last())
         {
            delete_current();
         }
      }
   }
   else
   {
      WORLD::message("Nothing selected to Undo.");
   }

}

/////////////////////////////////////
// handle_event()
/////////////////////////////////////
int 
HatchingPen::handle_event(CEvent&, State* &)
{

	//cerr << "HatchingPen::handle_event() - ";

	//We always uses the params in the GUI except
	//when adding into a non-complete group. This
	//way, the gesture will reflect the params of the 
	//group being edited.

	//Note: In the case that apply is pressed, 
	//update_gesture_proto gets called...

	if ( (_curr_hatching_group) && (!_curr_hatching_group->is_complete()) )
	{
		cerr << "Editing new group, so don't update gesture stroke.\n";
	}
	else
	{
		cerr << "No/complete group selected, so use latest params for gesture stroke.\n";
		update_gesture_proto();
	}

   return 0;
}

#include "stroke/base_stroke.H"

/////////////////////////////////////
// update_gesture_proto()
/////////////////////////////////////
void
HatchingPen::update_gesture_proto()
{

   cerr << "HatchingPen::update_gesture_proto()\n";

   assert(_gesture_drawer);
   BaseStroke *s = _gesture_drawer->base_stroke_proto();
   assert(s);
	s->copy(_params.stroke());
	_gesture_drawer->set_base_stroke_proto(s);
	

}
/* end of file hatching_pen.C */
