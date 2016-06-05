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
#include "buffer_ref_image.H"
#include "geom/world.H"
#include "geom/gl_view.H"

/**********************************************************************
 * BufferRefImage:
 **********************************************************************/

map<VIEWimpl*,BufferRefImage*> BufferRefImage::_hash;

BufferRefImage* 
BufferRefImage::lookup(CVIEWptr& v) 
{
   if (!v) {
      err_msg( "BufferRefImage::lookup: error -- view is nil");
      return nullptr;
   }
   if (!v->impl()) {
      err_msg( "BufferImage::lookup: error -- view->impl() is nil");
      return nullptr;
   }

   // hash on the view implementation rather than the view itself
   VIEWimpl *key = v->impl();
   map<VIEWimpl*,BufferRefImage*>::iterator it;
   it = _hash.find(key);
   BufferRefImage *ret = nullptr;
   if (it != _hash.end())
      ret = it->second;
   else if ((ret = new BufferRefImage(v)))
      _hash[key] = ret;

   return ret;
}

int  
BufferRefImage::need_update()  
{ 

   if (!is_observing())  {
      cerr << "BufferRefImage:need_update - Asked if update is needed --  "
           << "But we're not observing!\n";
      return false;
   }
   int w, h; 
   _view->get_size(w,h); 
   return (resize(w,h) || _dirty);
}

void
BufferRefImage::update() 
{
   if (!need_update())
      return;

   cerr << "BufferRefImage: Updated!\n";

   assert (_view != nullptr);
   VIEWimpl *impl = _view->impl();
   assert(impl != nullptr);

   _observing = false;
   impl->draw_setup();
   impl->draw_frame();
   _observing=true;
   
   glViewport(0,0,_width,_height);
   copy_to_ram();

   _dirty = 0;
  
}

BufferRefImage::BufferRefImage(CVIEWptr& v) : RefImage(v)  
{
   _f = nullptr;
   reset();
   _observing=false;

   // XXX - This should be in observe/unobserve, but it breaks it -- I
   // dunno why...

   // FIXME: this shared_ptr is hacky, but only the observer has it...
   WORLD::get_world()->schedule(shared_ptr<BufferRefImage>(this));
}


void 
BufferRefImage::observe()  
{
   _f = WORLD::lookup("fps");
   if (_f)
   {
      add(_f);
      WORLD::undisplay(_f, false);
   }

   reset();
   if (_view) {
      _observing = true;
      disp_obs();
      xform_obs();
      subscribe_all_mesh_notifications();
      _view->cam()->data()->add_cb(this);
      view_obs();
   }
}

void 
BufferRefImage::unobserve()  
{
   if (_f)
   {
      WORLD::display(_f, false);
      rem(_f);
      _f = nullptr;
   }

   if (_list.num() > 0)  {
      cerr << "BufferRefImage::unobserve - Cannot comply, "
           << "the high priority list isn't empty!\n";
      return;
   }

   if (_view) {
      unobs_display();
      unobs_xform();
      unsubscribe_all_mesh_notifications();
      _view->cam()->data()->rem_cb(this);
      unobs_view();
      //WORLD::get_world()->unschedule(shared_from_this());
   }
   _observing = false;
}

int
BufferRefImage::tick() 
{
   //XXX - This would be unnecessary if we unscheduled (but that breaks...)
   if (!is_observing()) return 1;

   if (_countup++ > COUNTUP && _dirty)
      update();
   return 1;
}

/* end of file buffer_ref_image.C */
