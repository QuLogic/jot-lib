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
 * npr_solid_texture.H
 **********************************************************************/
#ifndef NPR_SOLID_TEXTURE_H_IS_INCLUDED
#define NPR_SOLID_TEXTURE_H_IS_INCLUDED

#include "geom/texturegl.H"    // for ConventionalTexture
#include "gtex/basic_texture.H"

class NPRSolidTexCB : public GLStripCB {
 public:
   virtual void faceCB(CBvert* v, CBface* f);
};

class NPRSolidTexture : public BasicTexture {
 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist                *_nst_tags;
 protected:
   static LIST<str_ptr>*      _solid_texture_names;
   static LIST<TEXTUREptr>*   _solid_texture_ptrs;
   static LIST<str_ptr>*      _solid_texture_remap_orig_names;
   static LIST<str_ptr>*      _solid_texture_remap_new_names;

 protected:
   //******** Member Variables ********
   TEXTUREptr                    _tex;     
   str_ptr                       _tex_name;
   str_ptr                       _layer_name;
   int                           _use_paper;
   int                           _travel_paper;
   int                           _use_lighting;
   int                           _light_specular;
   int                           _transparent;
   int                           _annotate;
   bool                          _uv_in_dl;
   COLOR                         _color;
   double                        _alpha;

 public:
    // ******** CONSTRUCTOR ********
   NPRSolidTexture(Patch* patch = 0) :
      BasicTexture(patch, new NPRSolidTexCB),
      _tex_name(NULL_STR),
      _layer_name("Shader"),
      _use_paper(0),
      _travel_paper(0),
      _use_lighting(0),
      _light_specular(0),
      _transparent(1),
      _annotate(1),
      _uv_in_dl(false),
      _color(COLOR::white),
      _alpha(1.0) {}

   //******** Member Methods ********
 public:
   //******** Accessors ********
   virtual str_ptr      get_tex_name() const       { return _tex_name;        }
   virtual void         set_tex_name(Cstr_ptr tn)  { _tex_name = tn; _tex=NULL;  }
   virtual str_ptr      get_layer_name() const     { return _layer_name;      }
   virtual void         set_layer_name(Cstr_ptr ln){ _layer_name = ln;        }
   virtual int          get_use_paper() const      { return _use_paper;       }
   virtual void         set_use_paper(int p)       { _use_paper = p;          }
   virtual int          get_travel_paper() const   { return _travel_paper;    }
   virtual void         set_travel_paper(int t)    { _travel_paper = t;       }
   virtual int          get_use_lighting() const   { return _use_lighting;    }
   virtual void         set_use_lighting(int l)    { _use_lighting = l;       }
   virtual int          get_light_specular() const { return _light_specular;  }
   virtual void         set_light_specular(int s)  { _light_specular = s;     }
   virtual int          get_transparent() const    { return _transparent;     }
   virtual int          get_annotate() const       { return _annotate;        }
   virtual double       get_alpha() const          { return _alpha;           }
   virtual void         set_alpha(double a)        { _alpha = a;              }
   virtual COLOR        get_color() const          { return _color;           }
   virtual int          set_color(CCOLOR &c)       { _color = c; return 1;    }

 protected:
   //******** Internal Methods ********
   void                 update_tex();

 public:
   //******** GTexture VIRTUAL METHODS ********
   virtual int          draw(CVIEWptr& v); 

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM*   dup() const                { return new NPRSolidTexture; }
   virtual CTAGlist&    tags() const;

   //******** IO Methods **********
   void                 put_tex_name(TAGformat &d) const;
   void                 get_tex_name(TAGformat &d);
   void                 put_layer_name(TAGformat &d) const;
   void                 get_layer_name(TAGformat &d);

   int&                 use_paper_()      { return _use_paper;       }
   int&                 travel_paper_()   { return _travel_paper;    }
   int&                 use_lighting_()   { return _use_lighting;    }
   int&                 light_specular_() { return _light_specular;  }
   double&              alpha_()          { return _alpha;           }
   COLOR&               color_()          { return _color;           }

   // XXX - Deprecated
   void                 put_transparent(TAGformat &d) const;
   void                 get_transparent(TAGformat &d);
   void                 put_annotate(TAGformat &d) const;
   void                 get_annotate(TAGformat &d);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("NPRSolidTexture", OGLTexture, CDATA_ITEM *);

};

#endif // TOON_TEXTURE_H_IS_INCLUDED

/* end of file npr_solid_texture.H */
