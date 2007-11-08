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
/**********************************************************************
 * ref_img_drawer.H
 **********************************************************************/
#ifndef REF_IMG_DRAWER_IS_INCLUDED
#define REF_IMG_DRAWER_IS_INCLUDED

#include "disp/ref_img_client.H"
#include "disp/view.H"
#include "disp/colors.H"

/*****************************************************************
 * RefImgDrawer and subclasses:
 *****************************************************************/
class RefImgDrawer {
 public:
   virtual ~RefImgDrawer() {}
   virtual int draw(RefImageClient*) const = 0;
};

class RegularDrawer : public RefImgDrawer {
 public:
   RegularDrawer(CVIEWptr& v) : _v(v) {}
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw(_v) : 0;
   }
 protected:
   VIEWptr _v; // which view to use
};

class VisRefDrawer : public RefImgDrawer {
 public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_vis_ref() : 0;
   }
};

class IDRefDrawer : public RefImgDrawer {
 public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_id_ref() : 0;
   }
};

class IDPre1Drawer : public RefImgDrawer {
 public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_id_ref_pre1() : 0;
   }
};

class IDPre2Drawer : public RefImgDrawer {
 public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_id_ref_pre2() : 0;
   }
};

class IDPre3Drawer : public RefImgDrawer {
 public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_id_ref_pre3() : 0;
   }
};

class IDPre4Drawer : public RefImgDrawer {
 public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_id_ref_pre4() : 0;
   }
};

class ColorRefDrawer : public RefImgDrawer {
 public:
   ColorRefDrawer(int i) : _i(i) {}
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_color_ref(_i) : 0;
   }
 protected:
   int  _i; // which color reference image to draw?
};

class HaloRefDrawer : public RefImgDrawer{
public:
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_halo_ref() : 0;
   }
};

class FinalDrawer : public RefImgDrawer {
 public:
   FinalDrawer(CVIEWptr& v) : _v(v) {}
   virtual int draw(RefImageClient* r) const {
      return r ? r->draw_final(_v) : 0;
   }
 protected:
   VIEWptr _v; // which view to use
};

#endif // REF_IMG_DRAWER_IS_INCLUDED

// ref_img_drawer.H
