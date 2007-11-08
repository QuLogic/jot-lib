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


#include "geom/gl_view.H"
#include "aux_ref_image.H"


/**********************************************************************
 * AuxRefImage:
 **********************************************************************/

HASH    AuxRefImage::_hash(16);

AuxRefImage* 
AuxRefImage::lookup(CVIEWptr& v)
{
   if (!v) {
      err_msg( "AuxRefImage::lookup: error -- view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg( "AuxRefImage::lookup: error -- view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself
   long key = (long)v->impl();
   AuxRefImage* ret = (AuxRefImage*) _hash.find(key);
   if (!ret && (ret = new AuxRefImage(v)))
      _hash.add(key, (void *)ret);
   return ret;
}

bool
AuxRefImage::needs_update() 
{ 
   for (int i=0; i < _list.num(); i++) 
      if (_list[i]->needs_update()) 
         return true;

   return false;
}


void
AuxRefImage::update()
{
   if (!needs_update()) return;

   int w,h;
   _view->get_size(w,h);

   // Before resizing the view, gently set its camera aside,
   // careful not to trigger callbacks:
   CAMptr view_cam = _view->cam();

   // Copy cam params to our cam
   assert(_cam && _view->cam());
   *_cam = *view_cam;

   // Now tell the view to use our cam, which has no
   // callback observers:
   _view->use_cam(_cam);

   // XXX - not pushing attribs... F'ed up policy

   for (int i=0; i < _list.num(); i++) {
      if (_list[i]->needs_update()) {
         //cerr << "AuxRefImage: Updating job " << i << "\n";
         AUX_JOBptr j = _list[i];

         resize(j->width(),j->height());

         // Temporarily resize the view, not triggering
         // callbacks to camera observers of the real cam:
         _view->set_size(_width,_height,0,0);

         // setup GL state
         glDrawBuffer(GL_BACK);
         glReadBuffer(GL_BACK);
         _view->setup_lights();
         glLineWidth(1.0);
         glDepthMask(GL_TRUE);
         glDepthFunc(GL_LESS);
         glEnable(GL_DEPTH_TEST);
         glDisable(GL_NORMALIZE);
         glDisable(GL_BLEND);
         glEnable(GL_COLOR_MATERIAL);
         glEnable(GL_CULL_FACE);
         glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
         glShadeModel(GL_SMOOTH);
  
         CCOLOR &c = _view->color();

         glClearColor((float)c[0],(float)c[1],(float)c[2],1);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

         // Update the AUX_JOB
         draw_objects(j->list());

         copy_to_ram();
         copy_rgb(j->image());
         j->updated();
      }
   }

   // Restore the view to its regular size
   _view->set_size(w,h,0,0);

   // And last, gently put the camera back in, careful not
   // to bump it and trigger camera callbacks:
   _view->use_cam(view_cam);
}

void
AuxRefImage::draw_objects(CGELlist& drawn) const
{
   VIEWptr view = VIEW::peek();

   glMatrixMode(GL_PROJECTION);
   glLoadMatrixd(view->ndc_proj().transpose().matrix());

   for (int i=0; i < drawn.num(); i++) {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      drawn[i]->draw(view);
   }
}

AuxRefImage::AuxRefImage(CVIEWptr& v) :
   RefImage(v),
   _cam(new CAM("AuxRefImage cam"))
{
}

/* end of file aux_ref_image.C */
