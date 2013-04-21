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
#ifndef PAPER_EFFECT_HEADER
#define PAPER_EFFECT_HEADER

#include "disp/paper_effect_base.H"
#include "geom/texturegl.H"

/*****************************************************************
 * PaperEffect
 *****************************************************************/
class PaperEffect : public PaperEffectBase {
 protected:
   /******** PROTECTED MEMBERS TYPES ********/   
   enum pe_implementation_t {
      IMPLEMENTATION__NONE    = 0,
      IMPLEMENTATION__GL_ARB  = 1,
      IMPLEMENTATION__GL_NV   = 2,
      IMPLEMENTATION__GL_ATI  = 4
   };

   /******** STATIC MEMBER VARIABLES ********/   

   //Indicates which hardware implemenation...
   static int                       _implementation;

   //IMPLEMENTION__GL_ARB stuff...
   static GLuint                    _disabled_no_frag_prog_arb;
   static GLuint                    _disabled_1d_frag_prog_arb;
   static GLuint                    _disabled_2d_frag_prog_arb;
   static GLuint                    _paper_with_no_frag_prog_arb;
   static GLuint                    _paper_with_1d_frag_prog_arb;
   static GLuint                    _paper_with_2d_frag_prog_arb;
   
   static GLuint                    _paper_frag_shader_ati;

   static const char *              _DisabledNoFragProgARB;
   static const char *              _Disabled1DFragProgARB;
   static const char *              _Disabled2DFragProgARB;
   static const char *              _PaperWithNoFragProgARB;
   static const char *              _PaperWith1DFragProgARB;
   static const char *              _PaperWith2DFragProgARB;

   //General stuff...
   static TEXTUREptr                _paper_texture;

   //Since many strokes will use the paper
   //textures, we don't want to waste tonnes
   //of memory with redundant copies, or waste
   //time switching between different texture
   //objects that are identical.  Instead, we
   //cache all of the textures, and their
   //filenames, and serve up textures from the cache.
//   static LIST<str_ptr>             _paper_names;
//   static LIST<TEXTUREptr>          _paper_textures;
   static LIST<str_ptr>*      _paper_texture_names;
   static LIST<TEXTUREptr>*   _paper_texture_ptrs;
   static LIST<str_ptr>*      _paper_texture_remap_orig_names;
   static LIST<str_ptr>*      _paper_texture_remap_new_names;
 
 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   PaperEffect();
   virtual ~PaperEffect();
   
   /******** STATIC MEMBER METHODS ********/

   /******** INTERFACE METHODS ********/

   // Use this to get a texture for use with the
   // next two functions
   static TEXTUREptr    get_texture(Cstr_ptr &tf, str_ptr &ret);

   // Encloses objects being drawn in non-postprocess mode
   // If t==NULL then the paper effect is not invoked
   static void    begin_paper_effect(TEXTUREptr t, float cont, float bri, double x=0.0, double y=0.0);
   static void    end_paper_effect(TEXTUREptr t);

   // This version uses the global paper texture
   static void    begin_paper_effect(bool apply, double x=0.0, double y=0.0); 
   static void    end_paper_effect(bool apply); 

   static bool    is_alpha_premult(); 

   // For specifying paper coords (in ndc) for object verts in non-post mode
   static void    paper_coord(const double *v);

   // Toggles on/off (above 4 calls do nothing if not active)
   static void    toggle_active();  

   static void    delayed_activate()  { if (_delayed_activate && (_delayed_activate_state != _is_active)) toggle_active(); _delayed_activate = false;}

 protected:
   /******** INTERNAL METHODS ********/

   static void    begin_paper_effect_arb(TEXTUREptr t, float cont, float bri, double x, double y);
   static void    begin_paper_effect_nv(TEXTUREptr t, float cont, float bri, double x, double y);
   static void    begin_paper_effect_ati(TEXTUREptr t, float cont, float bri, double x, double y);

   static void    end_paper_effect_arb(TEXTUREptr t);
   static void    end_paper_effect_nv(TEXTUREptr t);
   static void    end_paper_effect_ati(TEXTUREptr t);

   static bool    check_new_paper();

   static void    init();

   static bool    init_tex();
   static bool    init_arb();
   static bool    init_nv();
   static bool    init_ati();

};

#endif /* PAPER_EFFECT_HEADER */

/* end of file paper_effect.H */
