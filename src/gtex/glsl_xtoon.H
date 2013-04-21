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
 * glsl_xtoon.H:
 **********************************************************************/
#ifndef GLSL_XTOON_H_IS_INCLUDED
#define GLSL_XTOON_H_IS_INCLUDED

#include "glsl_shader.H"

/**********************************************************************
 * GLSLToonXShader:
 *
 *  GLSL version of the extended toon shader
 *
 **********************************************************************/
class GLSLXToonShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   GLSLXToonShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL XToon",
                        GLSLXToonShader*, BasicTexture, CDATA_ITEM*);

   // Set the name of the texture to use (full path):
   void set_tex(Cstr_ptr& full_path_name);

   //******** GLSLShader VIRTUAL METHODS ********

   // Called in init(); query and store the "locations" of
   // uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   // Init the 1D toon texture by loading from file:
   virtual void init_textures();

   // Activate the 1D toon texture for drawing:
   virtual void activate_textures();

   virtual void set_gl_state(GLbitfield mask=0) const;

   //******** GTexture VIRTUAL METHODS ********

   // Draw the patch:
  // virtual int draw(CVIEWptr& v); 
   //virtual int load_tex(); //loads texture


   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLXToonShader; }




   //******** Accessors ********

   virtual Cstr_ptr& get_tex_name() const       { return _tex_name;     }
   virtual void set_tex_name(Cstr_ptr& tn)  {
     // cerr << "GLSLXToonShader::set_tex_name: deconstructing... ";
     // _tex = NULL;
     // cerr << "done" << endl;
      _tex_name = tn; 
	  set_tex(_tex_name);
   }

   virtual str_ptr      get_layer_name() const     { return _layer_name;   }
   virtual void         set_layer_name(Cstr_ptr ln){ _layer_name = ln;   _update_uniforms=true;  }
   virtual int          get_use_paper() const      { return false;}//_use_paper;    }
   virtual void         set_use_paper(int p)       { _use_paper = p;    _update_uniforms=true;   }
   virtual int          get_travel_paper() const   { return _travel_paper; }
   virtual void         set_travel_paper(int t)    { _travel_paper = t; _update_uniforms=true; }
   virtual int          get_transparent() const    { return _transparent;  }
   virtual int          get_annotate() const       { return _annotate;     }
   virtual double       get_alpha() const          { return _alpha;        }
   virtual void         set_alpha(double a)        { _alpha = a;     _update_uniforms=true;       }
   virtual COLOR        get_color() const          { return _color;        }
   virtual int          set_color(CCOLOR &c)       { _color = c; _update_uniforms=true; return 0; }
   virtual int          get_light_index() const    { return _light_index;  }
   virtual void         set_light_index(int i)     { _light_index = i;     _update_uniforms=true; }
   virtual int          get_light_dir() const      { return _light_dir;    }
   virtual void         set_light_dir(int d)       { _update_uniforms=true; _light_dir = d;       }
   virtual int          get_light_cam() const      { return _light_cam;    }
   virtual void         set_light_cam(int c)       { _light_cam = c;     _update_uniforms=true;   }
   virtual mlib::Wvec   get_light_coords() const   { return _light_coords; }
   virtual void         set_light_coords(mlib::CWvec &c) { _light_coords = c;   _update_uniforms=true;  }
   virtual double       get_target_length() const  { return _target_length;}
   virtual void         set_target_length(double t){ _target_length = t;   _update_uniforms=true; }
   virtual double       get_max_factor() const     { return _max_factor;   }
   virtual void         set_max_factor(double f)   { _max_factor = f;     _update_uniforms=true;  }
   virtual double       get_smooth_factor() const  { return _smooth_factor;}
   virtual void         set_smooth_factor(double f){ _smooth_factor = f;   _update_uniforms=true; }
   virtual double       get_smooth_detail() const  { return _smoothDetail;}
   virtual void         set_smooth_detail(int d)   { _smoothDetail = d;   _update_uniforms=true; }

   //******** IO Methods **********

   void                 put_layer_name(TAGformat &d) const;
   void                 get_layer_name(TAGformat &d);
   void                 put_tex_name(TAGformat &d) const;
   void                 get_tex_name(TAGformat &d);

   int&                 use_paper_()   { return _use_paper; }//_use_paper;    }  
   int&                 travel_paper_(){ return _travel_paper; }
   double&              alpha_()       { return _alpha;        }
   COLOR&               color_()       { return _color;        }
   int&                 light_index_() { return _light_index;  }
   int&                 light_dir_()   { return _light_dir;    }
   int&                 light_cam_()   { return _light_cam;    }
   mlib::Wvec&          light_coords_(){ return _light_coords; }
   void                 set_detail_map(int dmap) { _detail_map = dmap;  _update_uniforms=true;}
   bool                 normals_smoothed(){ return _normals_smoothed; }
   bool                 normals_elliptic(){ return _normals_elliptic; }
   bool                 normals_spheric(){ return _normals_spheric; }
   bool                 normals_cylindric(){ return _normals_cylindric; }
   void                 update_curvatures(bool b);
   void                 set_inv_detail(bool b) {_invert_detail = b;   _update_uniforms=true;}
   void                                 set_normals(int i);


 protected:
   //******** Internal Methods ********
   //bool load_texture();
  // bool activate_texture();
   //void load_uniforms();

   //******** Member Variables ********

   TEXTUREglptr _tex;   
   Image        _image;
   str_ptr      _tex_name;
   str_ptr      _layer_name;
   int          _use_paper;
   int          _travel_paper;
   int          _transparent;
   int          _annotate;
   COLOR        _color;
   double       _alpha;
   int          _light_index;
   int          _light_dir;
   int          _light_cam;
   mlib::Wvec   _light_coords;
   int          _detail_map;
   double       _target_length;
   double       _max_factor;
   bool         _normals_smoothed;
   bool         _normals_elliptic;
   bool         _normals_spheric;
   bool         _normals_cylindric;
   double       _smooth_factor;
   bool         _update_curvatures;
   double       _frame_rate;
   int          _nb_stat_frames;
   bool         _invert_detail;
   bool         _update_uniforms;
   bool         _tex_is_good;
   int          _smoothNormal;
   int          _smoothDetail;



   GLint _tex_loc;
   GLint _Dmap_loc;
   GLint _Sdtl_loc;
   GLint _Sfct_loc;
   GLint _trgt_loc;
   GLint _Mfct_loc;
   GLint _Fpnt_loc;
   GLint _Lidx_loc;
   GLint _Ldir_loc;
   GLint _Lcam_loc;
   GLint _Lcoord_loc;



   //******** VIRTUAL METHODS ********

   // Return the names of the xtoon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("xtoon"); }
   virtual str_ptr fp_filename() { return fp_name("xtoon"); }
};

#endif // GLSL_XTOON_H_IS_INCLUDED

// end of file glsl_xtoon.H
