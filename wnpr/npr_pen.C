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
// NPRPen
////////////////////////////////////////////



#include "npr/npr_texture.H"

// #include "draw.H"
#include "npr_pen.H"

#include "npr_pen_ui.H"

class FooGestureDrawer : public GestureDrawer{
 public:
   virtual GestureDrawer* dup() const { return new FooGestureDrawer; }
   virtual int draw(const GESTURE*, CVIEWptr&) { return 0; }
};

/////////////////////////////////////
// Constructor
/////////////////////////////////////

NPRPen::NPRPen(
   CGEST_INTptr &gest_int,
   CEvent& d, CEvent& m, CEvent& u,
   CEvent& shift_down, CEvent& shift_up,
   CEvent& ctrl_down,  CEvent& ctrl_up) :
   Pen(str_ptr("Basecoat"), 
       gest_int, d, m, u,
       shift_down, shift_up, 
       ctrl_down, ctrl_up)
{
                
   // gestures we recognize:
   _draw_start += DrawArc(new TapGuard,      drawCB(&NPRPen::tap_cb));
   _draw_start += DrawArc(new SlashGuard,    drawCB(&NPRPen::slash_cb));
   _draw_start += DrawArc(new LineGuard,     drawCB(&NPRPen::line_cb));
   _draw_start += DrawArc(new ScribbleGuard, drawCB(&NPRPen::scribble_cb));
   _draw_start += DrawArc(new LassoGuard,    drawCB(&NPRPen::lasso_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&NPRPen::stroke_cb));

   // let's us override the drawing of gestures:

   _gesture_drawer = new FooGestureDrawer();
        
   _prev_gesture_drawer = 0;

   // ui vars:
   _curr_npr_tex = 0;

   _ui = new NPRPenUI(this);
   assert(_ui);


}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

NPRPen::~NPRPen()
{

   if(_ui)
      delete _ui;

   if(_gesture_drawer)
      delete _gesture_drawer;
}

/////////////////////////////////////
// activate()
/////////////////////////////////////

void 
NPRPen::activate(State *s)
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
}

/////////////////////////////////////
// deactivate()
/////////////////////////////////////

bool  
NPRPen::deactivate(State* s)
{
   deselect_current_texture();
   
   if(_ui)
      _ui->hide();

   bool ret = Pen::deactivate(s);
   assert(ret);


   if(_gest_int && _prev_gesture_drawer)
   {
      _gest_int->set_drawer(_prev_gesture_drawer);   
      _prev_gesture_drawer = 0;
   }
   return true;
}

/////////////////////////////////////
// tap()
/////////////////////////////////////


int
NPRPen::tap_cb(CGESTUREptr& gest, DrawState*&)
{
   cerr << "NPRPen::tap_cb" << endl;

   Bface *f = VisRefImage::Intersect(gest->center());
   Patch* p = get_ctrl_patch(f);
   if (!(f && p)) {
      if (_curr_npr_tex)
         deselect_current_texture();
      return 0;
   }

   // Set the selected face's patch to using NPRTexture
   // It might already be doing this, but who cares!
   p->set_texture(NPRTexture::static_name());

   GTexture* cur_texture = p->cur_tex();
   if(!cur_texture->is_of_type(NPRTexture::static_name()))
      return 0; //Shouldn't happen

   NPRTexture* nt = (NPRTexture*)cur_texture;

   if (_curr_npr_tex)
      {
         if (_curr_npr_tex != nt)
            {
               deselect_current_texture();
               select_current_texture(nt);
            }
      }
   else
      select_current_texture(nt);

   return 0;
}

/////////////////////////////////////
// *_cb
/////////////////////////////////////

int
NPRPen::line_cb(CGESTUREptr&, DrawState*&)
{
   cerr << "NPRPen::line_cb()" << endl;
   return 0;
}

int
NPRPen::slash_cb(CGESTUREptr&, DrawState*&)
{
   cerr << "NPRPen::slash_cb()" << endl;
   return 0;
}


int
NPRPen::scribble_cb(CGESTUREptr&, DrawState*&)
{
   err_msg("NPRPen::scribble_cb()");
   return 0;
}

int
NPRPen::lasso_cb(CGESTUREptr&, DrawState*&)
{
   err_msg("NPRPen::lasso_cb()");
   return 0;
}

int
NPRPen::stroke_cb(CGESTUREptr&, DrawState*&)
{
   cerr << "NPRPen::stroke_cb()" << endl;
   return 0;
}

/////////////////////////////////////
// select_current_texture()
/////////////////////////////////////

void    
NPRPen::select_current_texture(NPRTexture *nt)
{
   cerr << "NPRPen:select_current_texture - Selecting a texture.\n";

   assert(!_curr_npr_tex);
   _curr_npr_tex = nt;

   _curr_npr_tex->set_selected(true);

   _ui->update();

   _ui->select();

   _ui->update();
}

/////////////////////////////////////
// deselect_current_texture()
/////////////////////////////////////

void    
NPRPen::deselect_current_texture()
{
   cerr << "NPRPen:deselect - Deselecting current texture.\n";

   _ui->deselect();

   if (_curr_npr_tex)
     _curr_npr_tex->set_selected(false);
   _curr_npr_tex= 0;

   _ui->update();

}

/* end of file npr_pen.C */
