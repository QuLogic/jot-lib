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
/*****************************************************************
 * buffer_ref_image.H
 *****************************************************************/
#ifndef BUFFER_REF_IMAGE_HEADER
#define BUFFER_REF_IMAGE_HEADER

#include "std/support.H"
#include "gtex/ref_image.H"

/**********************************************************************
 * BufferRefImage:
 *
 * Used to speed interactive drawing applications.  Interactive
 * stuff (gesture polylines) are added to a list of objects and
 * drawn to the the view after first copying the image.  The
 * image has a similar update policy to VisRefImage.  When the
 * camera, etc. are unchanging, this image obviates the need to
 * draw the scene, thereby accelerating drawing responsiveness.
 *
 **********************************************************************/


#define COUNTUP 4
#define CBufferRefImage const BufferRefImage

MAKE_PTR_SUBC(BufferRefImage, FRAMEobs);

class BufferRefImage : public RefImage,
                       protected DISPobs,
                       protected XFORMobs,
                       protected CAMobs,
                       protected BMESHobs,
                       public FRAMEobs,
                       protected VIEWobs
{
 public:
   virtual ~BufferRefImage() { unobserve(); }

   //******** MANAGERS ********
   // no public constructor -- get your hands on one this way:
   static BufferRefImage* lookup(CVIEWptr& v);

   // make sure the reference image is current:
   virtual void update();
   int need_update();

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const { return str_ptr("BufferRefImage"); }

   void force_dirty() { _dirty = 1; }

   // to turn observing on/off:
   void observe  ();
   void unobserve();
   bool is_observing() {return _observing;}

   void add (CGELptr &g) { _list.add(g);}
   bool rem (CGELptr &g) { return _list.rem(g);} 
   CGELlist & list () {return _list;}

   //void draw_list() {draw_objects(_list);}

   virtual int  tick();

   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
   BufferRefImage(CVIEWptr& v);

   static  HASH  _hash; // associates a BufferRefImage with a VIEW

   //******** DATA ********
   int          _dirty;             // flag set when change occurred
   int          _countup;
   bool         _observing;
   GELlist      _list;

   GELptr      _f; //fps

   //******** UPDATING ********
   void reset() { _dirty = 1; _countup = 0; }
   
   virtual bool resize(uint new_w, uint new_h) {
      if (!RefImage::resize(new_w, new_h))
         return false;
      reset();
      return true;
   }

   //******** OBSERVER METHODS ********:

   // Convenience, used below:
   void changed() { reset(); _countup = COUNTUP; }

   // EXISTobs:
   // XXX - BufferRefImage does not derive from EXISTobs so the
   //       following has no effect:
   virtual void notify_exist (CGELptr&, int) { changed(); }

   // DISPobs:
   virtual void notify       (CGELptr&, int) { changed(); }

   // CAMobs:
   virtual void notify(CCAMdataptr&) { reset(); }       // It's different

   // XFORMobs
   virtual void notify_xform (CGEOMptr&, STATE) { changed(); }

   // BMESHobs:
   virtual void notify_xform (BMESH*, mlib::CWtransf&, CMOD&){ changed(); } 
   virtual void notify_change(BMESH*, BMESH::change_t) { changed(); }

   virtual void notify_view   (CVIEWptr &v, int) { if (v == _view) changed(); }
};

#endif // BUFFER_REF_IMAGE_HEADER

/* end of file buffer_ref_image.H */
