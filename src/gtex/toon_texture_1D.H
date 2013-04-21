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
 * toon_texture_1D.H
 * Legacy toon shader, uses 1D textures, no pixel shaders
 * formerly the oryginal toon shader, reintroduced on Dec,28,2005
 **********************************************************************/
#ifndef TOON_TEXTURE_1D_H_IS_INCLUDED
#define TOON_TEXTURE_1D_H_IS_INCLUDED

#include "geom/texturegl.H"    // for ConventionalTexture
#include "gtex/basic_texture.H"

//name changed 
class ToonTexCB_1D : public GLStripCB {
 public:
   virtual void faceCB(CBvert* v, CBface* f);
};

class ToonTexture_1D : public BasicTexture {
 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist                *_ntt_tags;

 protected:
   static LIST<str_ptr>*      _toon_texture_names;
   static LIST<TEXTUREptr>*   _toon_texture_ptrs;
   static LIST<str_ptr>*      _toon_texture_remap_orig_names;
   static LIST<str_ptr>*      _toon_texture_remap_new_names;
    
    //******** Member Variables ********
   TEXTUREptr                    _tex;     
   str_ptr                       _tex_name;
   str_ptr                       _layer_name;
   int                           _use_paper;
   int                           _travel_paper;
   int                           _transparent;
   int                           _annotate;
   COLOR                         _color;
   double                        _alpha;
   int                           _light_index;
   int                           _light_dir;
   int                           _light_cam;
   Wvec                          _light_coords;

 public:
    // ******** CONSTRUCTOR ********
   ToonTexture_1D(Patch* patch = 0) :
      BasicTexture(patch, new ToonTexCB_1D),
      _tex_name(NULL_STR),
      _layer_name("Shader"),
      _use_paper(0),
      _travel_paper(0),
      _transparent(1),
      _annotate(1),
      _color(COLOR::white),
      _alpha(1.0),
      _light_index(0),
      _light_dir(1),
      _light_cam(1),
      _light_coords(Wvec(0,0,1)) {}

   //******** Member Methods ********
 public:
   //******** Accessors ********
   virtual str_ptr      get_tex_name() const       { return _tex_name;     }
   virtual void         set_tex_name(Cstr_ptr tn)  { _tex_name = tn; _tex=NULL;  }
   virtual str_ptr      get_layer_name() const     { return _layer_name;   }
   virtual void         set_layer_name(Cstr_ptr ln){ _layer_name = ln;     }
   virtual int          get_use_paper() const      { return _use_paper;    }
   virtual void         set_use_paper(int p)       { _use_paper = p;       }
   virtual int          get_travel_paper() const   { return _travel_paper; }
   virtual void         set_travel_paper(int t)    { _travel_paper = t;    }
   virtual int          get_transparent() const    { return _transparent;  }
   virtual int          get_annotate() const       { return _annotate;     }
   virtual double       get_alpha() const          { return _alpha;        }
   virtual void         set_alpha(double a)        { _alpha = a;           }
   virtual COLOR        get_color() const          { return _color;        }
   virtual int          set_color(CCOLOR &c)       { _color = c; return 1; }
   virtual int          get_light_index() const    { return _light_index;  }
   virtual void         set_light_index(int i)     { _light_index = i;     }
   virtual int          get_light_dir() const      { return _light_dir;    }
   virtual void         set_light_dir(int d)       { _light_dir = d;       }
   virtual int          get_light_cam() const      { return _light_cam;    }
   virtual void         set_light_cam(int c)       { _light_cam = c;       }
   virtual Wvec         get_light_coords() const   { return _light_coords; }
   virtual void         set_light_coords(CWvec &c) { _light_coords = c;    }

 protected:
   //******** Internal Methods ********
   void                 update_tex();
   void                 update_lights(CVIEWptr &v);

 public:
   //******** GTexture VIRTUAL METHODS ********
   virtual int          draw(CVIEWptr& v); 

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM*   dup() const                { return new ToonTexture_1D; }
   virtual CTAGlist&    tags() const;

   //******** IO Methods **********
   void                 put_layer_name(TAGformat &d) const;
   void                 get_layer_name(TAGformat &d);
   void                 put_tex_name(TAGformat &d) const;
   void                 get_tex_name(TAGformat &d);

   int&                 use_paper_()   { return _use_paper;    }  
   int&                 travel_paper_(){ return _travel_paper; }
   double&              alpha_()       { return _alpha;        }
   COLOR&               color_()       { return _color;        }
   int&                 light_index_() { return _light_index;  }
   int&                 light_dir_()   { return _light_dir;    }
   int&                 light_cam_()   { return _light_cam;    }
   Wvec&                light_coords_(){ return _light_coords; }

   // XXX - Deprecated
   void                 put_transparent(TAGformat &d) const;
   void                 get_transparent(TAGformat &d);
   void                 put_annotate(TAGformat &d) const;
   void                 get_annotate(TAGformat &d);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("NPRToonTexture_1D", ToonTexture_1D*,
                        OGLTexture, CDATA_ITEM *);
};

#endif // TOON_TEXTURE_1D_H_IS_INCLUDED

// end of file toon_texture.H
