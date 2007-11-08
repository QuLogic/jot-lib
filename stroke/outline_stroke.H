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

#ifndef OUTLINE_STROKE_HEADER
#define OUTLINE_STROKE_HEADER

#include "stroke/base_stroke.H"
#include "mesh/patch.H"

// This class exists to allow customized offset scaling for mak's
// outline strokes.  Eventually, these policies will probably be
// specified in BaseStroke in some way and this class will no longer
// be necessary.

#define COutlineStroke const OutlineStroke

class OutlineStroke : public BaseStroke {
 
   /******** STATIC MEMBER VARIABLES ********/   
 private:
   static TAGlist*              _os_tags;

   /******** MEMBER VARIABLES ********/
 protected:
   // May or may not be duped, never in IO tags
   Patch*   _patch;

   // May or may not be dupped, always in the IO tags
   double   _original_mesh_size; 
   //double   _offset_scale;

   // Not dupped or in IO!
   bool     _propagate_patch;
   bool     _propagate_mesh_size;

   uint     _path_index;      //For selection tracking
   uint     _group_index;     //by the pools

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
 public:
   OutlineStroke(Patch* p = 0) :
      _patch(p),
      _original_mesh_size(0.0),
      _propagate_patch(false),
      _propagate_mesh_size(true),
      _path_index(0),
      _group_index(0)
      //_offset_scale(1.0) 
      {}

   virtual ~OutlineStroke() {}  

   /******** Member Methods *******/

   virtual  BaseStroke* copy() const 
      { OutlineStroke *s = new OutlineStroke; assert(s); s->copy(*this); return s; }

   virtual void         copy(COutlineStroke& v) 
   { 
      if (v._propagate_mesh_size)   _original_mesh_size  = v._original_mesh_size; 
      if (v._propagate_patch)       _patch               = v._patch; 
      BaseStroke::copy(v); 
   }

   virtual void         copy(CBaseStroke& v) { BaseStroke::copy(v);  }

   virtual void draw_start();
   virtual void draw_end();

   /******** ACCESSOR MEMBER METHODS ********/

   void     set_original_mesh_size(double s) { _original_mesh_size = s;    }
   double   get_original_mesh_size() const   { return _original_mesh_size; }

   void     set_patch(Patch* p)              { _patch = p;                 }
   Patch*   get_patch() const                { return _patch;              }

   void     set_propagate_patch(bool b)      { _propagate_patch = b;       }
   bool     get_propagate_patch() const      { return _propagate_patch;    }

   void     set_propagate_mesh_size(bool b)  { _propagate_mesh_size = b;   }
   bool     get_propagate_mesh_size() const  { return _propagate_mesh_size;}

   void     set_path_index(uint i)           { _path_index = i;            }
   uint     get_path_index() const           { return _path_index;         }

   void     set_group_index(uint i)          { _group_index = i;           }
   uint     get_group_index() const          { return _group_index;        }

   /******** BaseStroke METHODS ********/

   // To prevent warnings:
   virtual void   interpolate_vert(
      BaseStrokeVertex *v, BaseStrokeVertex *vleft, double u) 
         { BaseStroke::interpolate_vert(v, vleft, u); }

   virtual void   apply_offset(BaseStrokeVertex* v, BaseStrokeOffset* o);
   
   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS2("OutlineStroke", BaseStroke, CDATA_ITEM *);
   virtual DATA_ITEM*  dup() const        { return copy(); }
   virtual CTAGlist&   tags() const;

   /******** I/O Access Methods ********/
 protected:
   double&        original_mesh_size_()   { return _original_mesh_size;  }

   
};

#endif // OUTLINE_STROKE_HEADER

/* end of file outline_stroke.H */


