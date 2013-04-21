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
#ifndef AUX_REF_IMAGE_HEADER
#define AUX_REF_IMAGE_HEADER

#include "std/support.H"
#include "geom/image.H"
#include "gtex/ref_image.H"

/**********************************************************************
 * AUX_JOB:
 *
 * I'll fill this in when I decide how this works...
 *
 **********************************************************************/

MAKE_PTR_BASEC(AUX_JOB);
#define CAUX_JOBptr const AUX_JOBptr

class  AUX_JOB : public REFcounter {

 protected:

   int          _w;
   int          _h;
   bool         _dirty;  
   GELlist      _list;
   Image        _img;

 public:

   AUX_JOB() : _w(0), _h(0), _dirty(false) {}
   virtual ~AUX_JOB()                      {}

   void              add (CGELptr &g)     { _list.add(g); }
   bool              rem (CGELptr &g)     { return _list.rem(g);  } 
   CGELlist&         list ()              { return _list; }

   void              set_dirty()          { _dirty = true;                        }
   void              clear_dirty()        { _dirty = false;                       }
   bool              is_dirty()           { return _dirty;                        }

   void              set_size(int w, int h)  { if((w>0)&&(h>0)){_w=w;_h=h;_img.resize(w,h,3);}}
   int               width()              { return _w;                            }
   int               height()             { return _h;                            }

   Image&            image()              { return _img;                          }

   virtual bool      needs_update()       { return ((_w>0)&&(_h>0)&&is_dirty()&&(_list.num()>0)); }

   virtual void      updated()            { _dirty = false;                       }
};

#define CAUX_JOBlist const AUX_JOBlist
class AUX_JOBlist : public LIST<AUX_JOBptr> { 
 public :
   AUX_JOBlist(int num=16) : LIST<AUX_JOBptr>(num)  { }
   AUX_JOBlist(CAUX_JOBptr &g)             { add(g);       }
};

/**********************************************************************
 * AuxRefImage:
 *
 * I'll fill this in when I decide how this works...
 *
 **********************************************************************/

#define CAuxRefImage const AuxRefImage

class AuxRefImage : public RefImage {

 protected:

   static HASH          _hash; 
        
   AUX_JOBlist          _list;
   CAMptr               _cam;

 public:

   virtual              ~AuxRefImage()          {}

   static AuxRefImage*  lookup(CVIEWptr& v);

   virtual void         update();

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const { return str_ptr("AuxRefImage"); }

   virtual void         draw_objects(CGELlist&) const;

   void                 add(CAUX_JOBptr &j)     { _list.add(j);         }
   bool                 rem(CAUX_JOBptr &j)     { return _list.rem(j);  } 

 protected:

   AuxRefImage(CVIEWptr& v);

   bool                 needs_update();

};

#endif // AUX_REF_IMAGE_HEADER

/* end of file aux_ref_image.H */
