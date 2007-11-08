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


#include "gtex/gl_extensions.H"
#include "paper_effect.H"
#include "geom/texturegl.H"
#include "geom/gl_util.H"
#include "geom/gl_view.H"

/*****************************************************************
 * Paper Texture Remapping
 *****************************************************************/

char *paper_remap_base = "nprdata/paper_textures/";
char *paper_remap_fnames[][2] = 
{
   {"pube.png",                  "p-combed-1.png"},
   {"basic_paper.png",           "p-noisy-1.png"},
   {"big_canvas.png",            "p-rough-2.png"},
   {"big_rough.png",             "p-cement-1.png"},
   {"blacktop.png",              "p-cement-2.png"},
   {"blobby2.png",               "p-blobby-1.png"},
   {"brushed_and_teased.png",    "p-brushed-1.png"},
   {"brushed_paper.png",         "p-brushed-2.png"},
   {"burlap.png",                "p-weave-1.png"},
   {"cane.png",                   "p-fabric-1.png"},
//   {"canvas.png",                x
//   {"canvas_inv.png",            x
   {"cold_press_water_color.png","p-noisy-2.png"},
   {"combs.png",                 "p-combed-1.png"},
   {"craqulure.png",             "p-blobby-2.png"},
   {"cross_fibre.png",           "p-fiber-4.png"},
   {"crusty_src.png",            "p-rough-1.png"},
   {"eroded_pavement.png",       "p-cement-3.png"},
   {"fine_grain.png",            "p-fine-1.png"},
   {"fine_paper.png",            "p-noisy-3.png"},
   {"finger_print.png",          "p-finger-1.png"},
   {"furry_bark.png",            "p-brushed-3.png"},
   {"gnarly12.png",              "p-ridged-1.png"},
//   {"half_tone_small.png",       x
//   {"halftone.png",              x
   {"handmade_paper_stock.png",  "p-fabric-2.png"},
   {"harsh.png",                 "p-cement-4.png"},
   {"hot_press_water_color.png", "p-fine-2.png"},
//   {"laid_pastel_paper.png",     x
   {"loom_woven.png",            "p-weave-2.png"},
//   {"marble.png",                x
   {"mini_glyphs.png",           "p-weird-1.png"},
   {"old_angora.png",            "p-rough-3.png"},
   {"paper.png",                 "p-noisy-4.png"},
   {"paper1.png",                "p-noisy-4.png"},
//   {"paper2.png",                x
   {"paper3.png",                "p-ridged-1.png"},
//   {"paper4.png",                x
   {"paper5.png",                "p-rough-4.png"},
   {"plaid.png",                 "p-weave-3.png"},
   {"pulpy_handmade.png",        "p-cement-5.png"},
   {"raw_silk.png",              "p-fabric-3.png"},
   {"ribbed_dreckle.png",        "p-ridged-2.png"},
   {"ribbed_pastel.png",         "p-ridged-3.png"},
   {"rice_contrast.png",         "p-fiber-1.png"},
   {"rice_src.png",              "p-fiber-2.png"},
   {"rough_src.png",             "p-rough-5.png"},
   {"rough_tooth.png",           "p-brushed-4.png"},
   {"sanded_pastel_board.png",   "p-fine-3.png"},
   {"sandy_watercolor.png",      "p-cement-6.png"},
   {"sandy_watercolor2.png",     "p-cement-6a.png"},
   {"scratchy_textile.png",      "p-fabric-4.png"},
   {"sidewalk.png",              "p-cement-7.png"},
   {"silk_fine.png",             "p-fine-4.png"},
   {"silk_fine2.png",            "p-fine-4a.png"},
   {"silk_high.png",             "p-fabric-5.png"},
   {"small_canvas.png",          "p-fine-5.png"},
   {"smooth_water_color.png",    "p-fabric-3.png"},
   {"string_paper.png",          "p-fiber-3.png"},
   {"super_fine.png",            "p-fine-6.png"},
   {"tissue.png",                "p-cement-8.png"},
   {"turbulent.png",             "p-weird-2.png"},
   {"vertical_stringy.png",      "p-weird-3.png"},
   {"water_color_pad.png",       "p-ridged-4.png"},
   {"watercolor2.png",           "p-cement-9.png"},
   {"watercolor_big_bumpy.png",  "p-rough-6.png"},
   {"watercolor_bumpy.png",      "p-rough-6a.png"},
   {"wool.png",                  "p-weird-4.png"},
   { NULL,                        NULL}
};


/*****************************************************************
 * PaperEffect
 *****************************************************************/

int               PaperEffect::_implementation = PaperEffect::IMPLEMENTATION__NONE;

TEXTUREptr        PaperEffect::_paper_texture;

LIST<str_ptr>*    PaperEffect::_paper_texture_names = 0;
LIST<TEXTUREptr>* PaperEffect::_paper_texture_ptrs = 0;
LIST<str_ptr>*    PaperEffect::_paper_texture_remap_orig_names = 0;
LIST<str_ptr>*    PaperEffect::_paper_texture_remap_new_names = 0;

GLuint            PaperEffect::_disabled_no_frag_prog_arb;
GLuint            PaperEffect::_disabled_1d_frag_prog_arb;
GLuint            PaperEffect::_disabled_2d_frag_prog_arb;
GLuint            PaperEffect::_paper_with_no_frag_prog_arb;
GLuint            PaperEffect::_paper_with_1d_frag_prog_arb;
GLuint            PaperEffect::_paper_with_2d_frag_prog_arb;

GLuint            PaperEffect::_paper_frag_shader_ati;

const char *      PaperEffect::_DisabledNoFragProgARB = 
{
"!!ARBfp1.0\n\
\
ATTRIB   iCol  = fragment.color;\
\
OUTPUT   oCol  = result.color;\
\
TEMP     tResult;\
\
\
\
MOV      tResult, iCol;\
\
MUL      tResult.rgb, tResult, tResult.a;\
\
MOV      oCol, tResult;\
\
END"
};

const char *      PaperEffect::_Disabled1DFragProgARB = 
{
"!!ARBfp1.0\n\
\
ATTRIB   iCol  = fragment.color;\
ATTRIB   iTex0 = fragment.texcoord[0];\
\
OUTPUT   oCol  = result.color;\
\
TEMP     tResult;\
\
\
\
TXP      tResult, iTex0, texture[0], 1D;\
\
MUL      tResult, iCol, tResult;\
\
MUL      tResult.rgb, tResult, tResult.a;\
\
MOV      oCol, tResult;\
\
END"
};

const char *      PaperEffect::_Disabled2DFragProgARB = 
{
"!!ARBfp1.0\n\
\
ATTRIB   iCol  = fragment.color;\
ATTRIB   iTex0 = fragment.texcoord[0];\
\
OUTPUT   oCol  = result.color;\
\
TEMP     tResult;\
\
\
\
TXP      tResult, iTex0, texture[0], 2D;\
\
MUL      tResult, iCol, tResult;\
\
MUL      tResult.rgb, tResult, tResult.a;\
\
MOV      oCol, tResult;\
\
END"
};

const char *      PaperEffect::_PaperWithNoFragProgARB = 
{
"!!ARBfp1.0\n\
\
ATTRIB   iCol  = fragment.color;\
ATTRIB   iTex0 = fragment.texcoord[0];\
ATTRIB   iTex1 = fragment.texcoord[1];\
\
OUTPUT   oCol  = result.color;\
\
PARAM    cHalf = { 0.5, 0.5, 0.5, 0.5 };\
PARAM    cOne  = { 1.0, 1.0, 1.0, 1.0 };\
PARAM    cTwo  = { 2.0, 2.0, 2.0, 2.0 };\
\
PARAM    c2Cont = program.env[1];\
PARAM    c2Brig = program.env[2];\
\
TEMP     tResult;\
TEMP     tPeak;\
TEMP     tValley;\
\
\
\
MOV      tResult, iCol;\
\
\
\
MUL_SAT  tPeak.a,   cTwo,  tResult;\
\
MAD_SAT  tValley.a, cTwo,  tResult, -cOne;\
\
\
\
TXP      tResult.a, iTex1, texture[1], 2D;\
\
MUL_SAT  tResult.a, c2Brig, tResult;\
ADD      tResult.a, tResult, -cHalf;\
MAD_SAT  tResult.a, c2Cont, tResult,  cHalf;\
\
LRP      tResult.a, tResult, tPeak, tValley;\
\
\
\
MUL      tResult.rgb, tResult, tResult.a;\
\
MOV      oCol, tResult;\
\
END"
};

const char *      PaperEffect::_PaperWith1DFragProgARB = 
{
"!!ARBfp1.0\n\
\
ATTRIB   iCol  = fragment.color;\
ATTRIB   iTex0 = fragment.texcoord[0];\
ATTRIB   iTex1 = fragment.texcoord[1];\
\
OUTPUT   oCol  = result.color;\
\
PARAM    cHalf = { 0.5, 0.5, 0.5, 0.5 };\
PARAM    cOne  = { 1.0, 1.0, 1.0, 1.0 };\
PARAM    cTwo  = { 2.0, 2.0, 2.0, 2.0 };\
\
PARAM    c2Cont = program.env[1];\
PARAM    c2Brig = program.env[2];\
\
TEMP     tResult;\
TEMP     tPeak;\
TEMP     tValley;\
\
\
\
TXP      tResult, iTex0, texture[0], 1D;\
\
MUL      tResult, iCol,  tResult;\
\
\
\
MUL_SAT  tPeak.a,   cTwo,  tResult;\
\
MAD_SAT  tValley.a, cTwo,  tResult, -cOne;\
\
\
\
TXP      tResult.a, iTex1, texture[1], 2D;\
\
MUL_SAT  tResult.a, c2Brig, tResult;\
ADD      tResult.a, tResult, -cHalf;\
MAD_SAT  tResult.a, c2Cont, tResult,  cHalf;\
\
LRP      tResult.a, tResult, tPeak, tValley;\
\
\
\
MUL      tResult.rgb, tResult, tResult.a;\
\
MOV      oCol, tResult;\
\
END"
};

const char *      PaperEffect::_PaperWith2DFragProgARB = 
{
"!!ARBfp1.0\n\
\
ATTRIB   iCol  = fragment.color;\
ATTRIB   iTex0 = fragment.texcoord[0];\
ATTRIB   iTex1 = fragment.texcoord[1];\
\
OUTPUT   oCol  = result.color;\
\
PARAM    cHalf = { 0.5, 0.5, 0.5, 0.5 };\
PARAM    cOne  = { 1.0, 1.0, 1.0, 1.0 };\
PARAM    cTwo  = { 2.0, 2.0, 2.0, 2.0 };\
\
PARAM    c2Cont = program.env[1];\
PARAM    c2Brig = program.env[2];\
\
TEMP     tResult;\
TEMP     tPeak;\
TEMP     tValley;\
\
\
\
TXP      tResult, iTex0, texture[0], 2D;\
\
MUL      tResult, iCol,  tResult;\
\
\
\
MUL_SAT  tPeak.a,   cTwo,  tResult;\
\
MAD_SAT  tValley.a, cTwo,  tResult, -cOne;\
\
\
\
TXP      tResult.a, iTex1, texture[1], 2D;\
\
MUL_SAT  tResult.a, c2Brig, tResult;\
ADD      tResult.a, tResult, -cHalf;\
MAD_SAT  tResult.a, c2Cont, tResult,  cHalf;\
\
LRP      tResult.a, tResult, tPeak, tValley;\
\
\
\
MUL      tResult.rgb, tResult, tResult.a;\
\
MOV      oCol, tResult;\
\
END"
};

/*****************************************************************
 * gl_multi_tex_coord_2dv function pointer
 *****************************************************************/

// Calling to paper_coord just calls glMultiTexCood2dv through
// this pointer.  However, if postprocess=true, we'd like to
// quietly, and quickly ignore the calls, so we change to a no_op
// on this pointer when postprocess=true (which is default)

#ifndef APICALLCONVENTION
#ifdef WIN32
#define APICALLCONVENTION __stdcall
#else
#define APICALLCONVENTION
#endif //WIN32
#endif //APICALLCONVENTION

void  APICALLCONVENTION   no_op(GLenum, const GLdouble *) {}
void (APICALLCONVENTION * gl_multi_tex_coord_2dv) (GLenum, const GLdouble *) = &no_op;

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////

static str_ptr paper_filename_default = "/nprdata/paper_textures/paper.png";

////////////////////////////////////////////////////////////////////////////////
// PaperEffect Methods
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// The no-op texture coord to use in post process mode
/////////////////////////////////////

/////////////////////////////////////
// Constructor
/////////////////////////////////////

PaperEffect::PaperEffect()
{
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

PaperEffect::~PaperEffect()
{
}

/////////////////////////////////////
// paper_coord()
/////////////////////////////////////
// Calls through the function pointer.
// This is either a no-op or the real gl call
// This pointer is set when the postprocess flag is toggled
void    
PaperEffect::paper_coord(const double *v)
{
#ifdef GL_ARB_multitexture
   gl_multi_tex_coord_2dv(GL_TEXTURE1_ARB, v); 
#endif
}


/////////////////////////////////////
// toggle_active
/////////////////////////////////////

void
PaperEffect::toggle_active()
{
   if (!_is_inited) init();

   if (!_is_supported)
   {
      assert(!_is_active);
      err_mesg(ERR_LEV_INFO, "PaperEffect::toggle_active() - **SORRY!** Can't activate -- no hardware support.");
      return;
   }

   if(_is_active)
   {
      _is_active = false;
      gl_multi_tex_coord_2dv = no_op;
      err_mesg(ERR_LEV_INFO, "PaperEffect::toggle_active() - Now inactive.");
   }
   else
   {
      _is_active = true;
#ifdef GL_ARB_multitexture
      gl_multi_tex_coord_2dv = glMultiTexCoord2dvARB;
#endif
      err_mesg(ERR_LEV_INFO, "PaperEffect::toggle_active() - Now active.");
   }
   notify_usage_toggled();
}

/////////////////////////////////////
//  begin_paper_effect()
/////////////////////////////////////
void    
PaperEffect::begin_paper_effect(bool apply, double x, double y)
{
   check_new_paper(); 
   begin_paper_effect(((apply)?(_paper_texture):(NULL)), _cont, _brig, x, y);
}

/////////////////////////////////////
//  end_paper_effect()
/////////////////////////////////////
   
void
PaperEffect::end_paper_effect(bool apply)
{ 
   //XXX - Check again?
   check_new_paper(); 
   end_paper_effect(((apply)?(_paper_texture):(NULL)));
}

/////////////////////////////////////
//  begin_paper_effect()
/////////////////////////////////////
void
PaperEffect::begin_paper_effect(TEXTUREptr t, float cont, float brig, double orig_x, double orig_y)
{

   GL_VIEW::print_gl_errors("PaperEffect::begin_paper_effect() [Start] - ");

   if (!_is_inited) init();

   if (!_is_supported) return;

   if ((_implementation & IMPLEMENTATION__GL_ARB) && !GLExtensions::get_debug())
   {
      begin_paper_effect_arb(t, cont, brig, orig_x, orig_y);
   }
   else if (_implementation & IMPLEMENTATION__GL_NV)
   {
      begin_paper_effect_nv(t, cont, brig, orig_x, orig_y);
   }
   else if (_implementation & IMPLEMENTATION__GL_ATI)
   {
      begin_paper_effect_ati(t, cont, brig, orig_x, orig_y);
   }
   else
   {
      assert(_implementation == IMPLEMENTATION__NONE);
   }

   GL_VIEW::print_gl_errors("PaperEffect::begin_paper_effect() [End] - ");
}

/////////////////////////////////////
//  end_paper_effect()
/////////////////////////////////////
void
PaperEffect::end_paper_effect(TEXTUREptr t)
{
   GL_VIEW::print_gl_errors("PaperEffect::end_paper_effect() [Start] - ");

   assert(_is_inited);

   if (!_is_supported) return;

   if ((_implementation & IMPLEMENTATION__GL_ARB) && !GLExtensions::get_debug())
   {
      end_paper_effect_arb(t);
   }
   else if (_implementation & IMPLEMENTATION__GL_NV)
   {
      end_paper_effect_nv(t);
   }
   else if (_implementation & IMPLEMENTATION__GL_ATI)
   {
      end_paper_effect_ati(t);
   }
   else
   {
      assert(_implementation == IMPLEMENTATION__NONE);
   }

   GL_VIEW::print_gl_errors("PaperEffect::end_paper_effect() [End] - ");
}

/////////////////////////////////////
//  begin_paper_effect_nv()
/////////////////////////////////////
void
PaperEffect::begin_paper_effect_nv(TEXTUREptr t, float cont, float brig, double orig_x, double orig_y)
{
   assert(_is_inited);
   assert(_is_supported);
   assert(_implementation & IMPLEMENTATION__GL_NV);

   //If not active, or no texture, then just premultiply alpha...
   if (!_is_active || t==NULL) 
   {
#ifdef GL_NV_register_combiners

   GLint query;

   assert(!glIsEnabled(GL_REGISTER_COMBINERS_NV));

   //Check if were actually applying the texture
   //We need this to combine the diffuse color and texture properly
   GLboolean using_texture0 = glIsEnabled(GL_TEXTURE_2D) || glIsEnabled(GL_TEXTURE_1D);

   //XXX - Assume that if it's enabled, we're using MODULATE
   //If this not good enough, we must modify the register combiner
   //state below to implement the desired texture environment given by query
   if (using_texture0)
   {
      glGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &query);
      assert(query == GL_MODULATE);
   }

   //XXX - Now setup the reg. comb. state, stomping the old state
   // We use 1 stage of general combiners.
   glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

   // A,B(RGB) <-- Primary(RGB), Tex0(RGB) [or (1,1,1)  if (using_texture0 == false).]
   glCombinerInputNV(GL_COMBINER0_NV,
                     GL_RGB,              GL_VARIABLE_A_NV,
                     GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
   if (using_texture0)
   {
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_RGB,           GL_VARIABLE_B_NV,
                        GL_TEXTURE0_ARB,  GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
   }
   else
   {
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_RGB,           GL_VARIABLE_B_NV,
                        GL_ZERO,          GL_UNSIGNED_INVERT_NV,     GL_RGB);
   }
   // AB(RGB) --> SPARE0(RGB)  ['pigment color'] (via GL_MODULATE!!)
   glCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,             
                      GL_SPARE0_NV,    GL_DISCARD_NV,   GL_DISCARD_NV,
                      GL_NONE,         GL_NONE,
                      GL_FALSE,        GL_FALSE,        GL_FALSE);


   // A,B(ALPHA) <-- Primary(ALPHA), Tex0(ALPHA) [or (1) if (using_texture0 == false).]
   glCombinerInputNV(GL_COMBINER0_NV,
                     GL_ALPHA,            GL_VARIABLE_A_NV,          
                     GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
   if (using_texture0)
   {
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_ALPHA,         GL_VARIABLE_B_NV,          
                        GL_TEXTURE0_ARB,  GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
   }
   else
   {
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_ALPHA,         GL_VARIABLE_B_NV,          
                        GL_ZERO,          GL_UNSIGNED_INVERT_NV,     GL_ALPHA);
   }

   // (AB)(ALPHA) --> SPARE0(ALPHA) 
   glCombinerOutputNV(GL_COMBINER0_NV,    GL_ALPHA,             
                      GL_SPARE0_NV,       GL_DISCARD_NV,   GL_DISCARD_NV,
                      GL_NONE,   GL_NONE,
                      GL_FALSE,           GL_FALSE,        GL_FALSE);

   //Final stage:
   //Outgoing RGB,ALPHA <-- SPARE0(RGB),SPARE0(ALPHA)
   glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                          GL_ZERO,      GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
   glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                          GL_E_TIMES_F_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
   glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                          GL_ZERO,      GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
   glFinalCombinerInputNV(GL_VARIABLE_E_NV,
                          GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
   glFinalCombinerInputNV(GL_VARIABLE_F_NV,
                          GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
   glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                          GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);


   glEnable(GL_REGISTER_COMBINERS_NV);

#endif
   }
   else
   {
#if defined(GL_NV_register_combiners) && defined(GL_ARB_multitexture)

      GLint query;
      double uscale, vscale, uoff, voff, aspect;
      int w, h;
      VIEW_SIZE(w,h);

      if (w >= h)
      {
         aspect = (double)w/(double)h;
         uscale = (double)w/(2.0*aspect*(double)t->image().width());
         vscale = (double)h/(2.0*(double)t->image().height());
         uoff = aspect;
         voff = 1.0;
      }
      else
      {
         aspect = (double)h/(double)w;
         uscale = (double)w/(2.0*(double)t->image().width());
         vscale = (double)h/(2.0*aspect*(double)t->image().height());
         uoff = 1.0;
         voff = aspect;
      }

      //We should push texture state to save the old reg. combiner
      //state and texture state for both units, but we're going to assume that only
      //PaperEffect plays with the 2nd texture unit and reg. combiners,
      //so we just go ahead and stomp all over this state. 
      //The texture state is local to the current active unit, so
      //we just change to the 2nd unit, and go crazy:

      //First, since we assume nobody else is using reg. combiners
      //let's be sure that this is true.
      assert(!glIsEnabled(GL_REGISTER_COMBINERS_NV));

      //Now, let's just be sure that TEXTURE0 was the
      //active unit to start with since this should
      //be true is we're the only guys using multitex.
   
      glGetIntegerv ( GL_ACTIVE_TEXTURE_ARB, &query); 
      assert(query == GL_TEXTURE0_ARB);

      //Check if were actually applying the texture
      //We need this to combine the diffuse color and texture properly
      GLboolean using_texture0 = glIsEnabled(GL_TEXTURE_2D) || glIsEnabled(GL_TEXTURE_1D);

      //XXX - Assume that if it's enabled, we're using MODULATE
      //If this not good enough, we must modify the register combiner
      //state below to implement the desired texture environment given by query
      if (using_texture0)
      {
         glGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &query);
         assert(query == GL_MODULATE);
      }

      //XXX - Now setup the reg. comb. state, stomping the old state
      // We use 2 stages of general combiners.
      glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 2);

      //1st stage:
      // Computes brightness/contrast adjustment to height field.
      // Evaluates. 'peak' and 'trough' basis functions at the 'pressure'(ALPHA).

      // Contrast ranges 0-1 with 0.5 meaning no effect
      // Brightness ranges 0-1 with 0.5 meaning no effect
      // Due to the limits of using a single combiner for 
      // both effects, brightening becomes less dramatic
      // as contrast is increased

      GLfloat constant0[4] = {0.0, 0.0, 0.0, 0.0};
      GLfloat constant1[4] = {0.0, 0.0, 0.0, 0.0};

      assert(cont>=0.0); assert(cont<=1.0f);
      assert(brig>=0.0); assert(brig<=1.0f);
      constant0[3] = cont;
      constant1[3] = (brig<=0.5f)?(cont*brig/0.5f):(cont+(1.0f-cont)*(brig-0.5f)/0.5f);

      glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, constant0);
      glCombinerParameterfvNV(GL_CONSTANT_COLOR1_NV, constant1);

      // A,B(RGB) <-- 0.5-CONST0(ALPHA), 0.5 [ (0.5-contrast)*0.5 ]
      // C,D(RGB) <-- CONST1(ALPHA), (TEX1)(ALPHA) [brightness adjusted contrast*height]
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_RGB,                 GL_VARIABLE_A_NV,          
                        GL_CONSTANT_COLOR0_NV,  GL_HALF_BIAS_NEGATE_NV,    GL_ALPHA);
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_RGB,                 GL_VARIABLE_B_NV,          
                        GL_ZERO,                GL_HALF_BIAS_NEGATE_NV,    GL_ALPHA);
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_RGB,                 GL_VARIABLE_C_NV,          
                        GL_CONSTANT_COLOR1_NV,  GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_RGB,                 GL_VARIABLE_D_NV,          
                        GL_TEXTURE1_ARB,        GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);

      //(AB+CD)(RGB) --> SPARE1(RGB)  [brightness and contrast adjusted height]
      glCombinerOutputNV(GL_COMBINER0_NV,    GL_RGB,             
                         GL_DISCARD_NV,      GL_DISCARD_NV,   GL_SPARE1_NV,
                         GL_SCALE_BY_TWO_NV, GL_NONE,
                         GL_FALSE,           GL_FALSE,        GL_FALSE);

      // A,B(ALPHA) <-- Primary(ALPHA), Tex0(ALPHA) [or (1) if (using_texture0 == false).]
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_ALPHA,            GL_VARIABLE_A_NV,          
                        GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
      if (using_texture0)
      {
         glCombinerInputNV(GL_COMBINER0_NV,
                           GL_ALPHA,         GL_VARIABLE_B_NV,          
                           GL_TEXTURE0_ARB,  GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
      }
      else
      {
         glCombinerInputNV(GL_COMBINER0_NV,
                           GL_ALPHA,         GL_VARIABLE_B_NV,          
                           GL_ZERO,          GL_UNSIGNED_INVERT_NV,     GL_ALPHA);
      }

      // C,D(ALPHA) <-- (+1),(-0.5) [for use as a bias, CD=-0.5, in trough transfer function]
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_ALPHA,            GL_VARIABLE_C_NV,          
                        GL_ZERO,             GL_UNSIGNED_INVERT_NV,   GL_ALPHA);
      glCombinerInputNV(GL_COMBINER0_NV,
                        GL_ALPHA,            GL_VARIABLE_D_NV,          
                        GL_ZERO,             GL_HALF_BIAS_NORMAL_NV,  GL_ALPHA);

      //   2x(AB)(ALPHA) --> SPARE0(ALPHA)  [peak transfer function at incoming alpha (as GL_MODULATE)]
      //2x(AB+CD)(ALPHA) --> SPARE1(ALPHA)  [trough tranfer function at incoming alpha (as GL_MODULATE)]
      glCombinerOutputNV(GL_COMBINER0_NV,    GL_ALPHA,             
                         GL_SPARE0_NV,       GL_DISCARD_NV,   GL_SPARE1_NV,
                         GL_SCALE_BY_TWO_NV, GL_NONE,
                         GL_FALSE,           GL_FALSE,        GL_FALSE);


      // 2nd stage:
      // Computes the 'pigment color'(RGB) and 'pressure'(ALPHA) via a GL_MODULATE style multiply
      // Computes the 'deposited pigment fraction'(ALPHA)
      //   as weighted sum of 'peak' and 'trough' using 'height' (TEX1ALPHA)

      // A,B(RGB) <-- Primary(RGB), Tex0(RGB) [or (1,1,1)  if (using_texture0 == false).]
      glCombinerInputNV(GL_COMBINER1_NV,
                        GL_RGB,              GL_VARIABLE_A_NV,
                        GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      if (using_texture0)
      {
         glCombinerInputNV(GL_COMBINER1_NV,
                           GL_RGB,           GL_VARIABLE_B_NV,
                           GL_TEXTURE0_ARB,  GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      }
      else
      {
         glCombinerInputNV(GL_COMBINER1_NV,
                           GL_RGB,           GL_VARIABLE_B_NV,
                           GL_ZERO,          GL_UNSIGNED_INVERT_NV,     GL_RGB);
      }
      // AB(RGB) --> SPARE0(RGB)  ['pigment color'] (via GL_MODULATE!!)
      glCombinerOutputNV(GL_COMBINER1_NV, GL_RGB,             
                         GL_SPARE0_NV,    GL_DISCARD_NV,   GL_DISCARD_NV,
                         GL_NONE,         GL_NONE,
                         GL_FALSE,        GL_FALSE,        GL_FALSE);

      // A,C(ALPHA) <-- SPARE0(ALPHA), SPARE1(ALPHA) ['peak' and 'trough']
      // B,D(ALPHA) <-- TEX1(ALPHA), (1-TEX1)(ALPHA) ['peak', 'trough' weights in linear combo]
      // B,D(ALPHA) <-- SPARE1(BLUE), (1-SPARE1)(BLUE) ['peak', 'trough' weights in linear combo]
      glCombinerInputNV(GL_COMBINER1_NV,
                        GL_ALPHA,            GL_VARIABLE_A_NV,          
                        GL_SPARE0_NV,        GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
      glCombinerInputNV(GL_COMBINER1_NV,
                        GL_ALPHA,            GL_VARIABLE_C_NV,          
                        GL_SPARE1_NV,        GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
      glCombinerInputNV(GL_COMBINER1_NV,
                        GL_ALPHA,            GL_VARIABLE_B_NV,          
                        GL_SPARE1_NV,        GL_UNSIGNED_IDENTITY_NV,   GL_BLUE);
      glCombinerInputNV(GL_COMBINER1_NV,
                        GL_ALPHA,            GL_VARIABLE_D_NV,          
                        GL_SPARE1_NV,        GL_UNSIGNED_INVERT_NV,     GL_BLUE);

      //(AB+CD)(ALPHA) --> SPARE0(ALPHA)  ['deposited pigment fraction' as (h)'peak'+(1-h)'trough']
      glCombinerOutputNV(GL_COMBINER1_NV,    GL_ALPHA,             
                         GL_DISCARD_NV,      GL_DISCARD_NV,   GL_SPARE0_NV,
                         GL_NONE,            GL_NONE,
                         GL_FALSE,           GL_FALSE,        GL_FALSE);

      // XXX - Newer version premultiplies by alpha!!!!!!!

   /*
      //Final stage:
      //Outgoing RGB,ALPHA <-- SPARE0(RGB),SPARE0(ALPHA)
      glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                             GL_ZERO,        GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                             GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                             GL_ZERO,        GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                             GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
   */

      //Final stage:
      //Outgoing RGB,ALPHA <-- SPARE0(RGB),SPARE0(ALPHA)
      glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                             GL_ZERO,      GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                             GL_E_TIMES_F_NV,  GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                             GL_ZERO,      GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_E_NV,
                             GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_RGB);
      glFinalCombinerInputNV(GL_VARIABLE_F_NV,
                             GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);
      glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                             GL_SPARE0_NV,   GL_UNSIGNED_IDENTITY_NV,   GL_ALPHA);


      //Now switch to TEXTURE1 and stomp all over the old state
      glActiveTextureARB(GL_TEXTURE1_ARB);

      t->apply_texture();              //calls glBindTexture and sets state
   
      //Setup the scale matrix to convert from NDC -> UV
      glGetIntegerv ( GL_MATRIX_MODE, &query); 
      glMatrixMode( GL_TEXTURE );
      glLoadIdentity();
      glScaled( uscale, vscale, 1.0 );
      glTranslated( uoff - orig_x, voff - orig_y, 0.0 );
      glMatrixMode( query );

      glEnable(GL_TEXTURE_2D);

      //Turn on the combiners
      glEnable(GL_REGISTER_COMBINERS_NV);

      //And return to the 1st texturing unit
      glActiveTextureARB(GL_TEXTURE0_ARB);

#endif
   
   }
}

/////////////////////////////////////
//  begin_paper_effect_ati()
/////////////////////////////////////
void
PaperEffect::begin_paper_effect_ati(TEXTUREptr t, float cont, float brig, double orig_x, double orig_y)
{
   assert(_is_inited);
   assert(_is_supported);
   assert(_implementation & IMPLEMENTATION__GL_ATI);


#if defined(GL_ATI_fragment_shader) && defined(GL_ARB_multitexture) && !defined(NON_ATI_GFX)

   GLint query;

   //We should push texture state to save the state for both units, 
   //but we're going to assume that only PaperEffect plays with the 
   //2nd texture unit, so we just go ahead and stomp all over this state. 

   //First, since we assume nobody else is using fragment programs...
   //Let's be sure that this is true.
   assert(!glIsEnabled(GL_FRAGMENT_SHADER_ATI));

   //Now, let's just be sure that TEXTURE0 was the
   //active unit to start with since this should
   //be true if we're the only guys using multitex.
   glGetIntegerv ( GL_ACTIVE_TEXTURE_ARB, &query); 
   assert(query == GL_TEXTURE0_ARB);

   //Check if were actually applying the texture.
   //We need this to combine the diffuse color and texture properly
   GLboolean using_texture0 = glIsEnabled(GL_TEXTURE_2D) || glIsEnabled(GL_TEXTURE_1D);

   //XXX - Assume that if it's enabled, we're using MODULATE
   //If this not good enough, we must modify the programs to
   //implement the desired texturing scheme given by the query...
   if (using_texture0)
   {
      glGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &query);
      assert(query == GL_MODULATE);
   }

   // See if we're actually doing ye olde paper effect...
   bool using_texture1 = _is_active && (t != NULL);

   if (using_texture1)
   {
      //Compute stuff for the ndc->uv conversion matrix
      double uscale, vscale, uoff, voff, aspect;
      int w, h;   VIEW_SIZE(w,h);
      if (w >= h)
      {
         aspect = (double)w/(double)h;
         uscale = (double)w/(2.0*aspect*(double)t->image().width());
         vscale = (double)h/(2.0*(double)t->image().height());
         uoff = aspect;
         voff = 1.0;
      }
      else
      {
         aspect = (double)h/(double)w;
         uscale = (double)w/(2.0*(double)t->image().width());
         vscale = (double)h/(2.0*aspect*(double)t->image().height());
         uoff = 1.0;
         voff = aspect;
      }

      //Now switch to TEXTURE1 and stomp all over the old state
      glActiveTextureARB(GL_TEXTURE1_ARB);

      t->apply_texture(); //calls glBindTexture and sets state

      //Setup the scale matrix to convert from NDC -> UV
      glGetIntegerv ( GL_MATRIX_MODE, &query); 
      glMatrixMode( GL_TEXTURE );
      glLoadIdentity();
      glScaled( uscale, vscale, 1.0 );
      glTranslated( uoff - orig_x, voff - orig_y, 0.0 );
      glMatrixMode( query );

      glEnable(GL_TEXTURE_2D);

      //And return to the 1st texturing unit
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }


   //Now setup the fragment program...

   GLfloat c0[4], c1[4], c2[4];

   c0[0]=c0[1]=c0[2]=c0[3]=((using_texture0)?(1.0f):(0.0f));

   assert(cont>=0.0); assert(cont<=1.0f);
   assert(brig>=0.0); assert(brig<=1.0f);

   if (using_texture1)
   {
      c1[0]=c1[1]=c1[2]=c1[3] = 0.5f - cont;
      c2[0]=c2[1]=c2[2]=c2[3] = (brig<=0.5f)?(2.0f*cont*brig):(cont+(1.0f-cont)*(brig-0.5f)/0.5f);
   }
   else
   {
      c1[0]=c1[1]=c1[2]=c1[3] = 0.5f;
      c2[0]=c2[1]=c2[2]=c2[3] = 0.0f;
   }
   glBindFragmentShaderATI(_paper_frag_shader_ati); 

   glSetFragmentShaderConstantATI (GL_CON_0_ATI, c0);
   glSetFragmentShaderConstantATI (GL_CON_1_ATI, c1);
   glSetFragmentShaderConstantATI (GL_CON_2_ATI, c2);
   
   glEnable(GL_FRAGMENT_SHADER_ATI);  //GL_ENABLE_BIT

#endif
   
}

/////////////////////////////////////
//  begin_paper_effect_arb()
/////////////////////////////////////
void
PaperEffect::begin_paper_effect_arb(TEXTUREptr t, float cont, float brig, double orig_x, double orig_y)
{
   assert(_is_inited);
   assert(_is_supported);
   assert(_implementation & IMPLEMENTATION__GL_ARB);

#if defined(GL_ARB_fragment_program) && defined(GL_ARB_multitexture)

   GLint query;

   //We should push texture state to save the state for both units, 
   //but we're going to assume that only PaperEffect plays with the 
   //2nd texture unit, so we just go ahead and stomp all over this state. 

   //First, since we assume nobody else is using fragment programs...
   //Let's be sure that this is true.
   assert(!glIsEnabled(GL_FRAGMENT_PROGRAM_ARB));

   //Now, let's just be sure that TEXTURE0 was the
   //active unit to start with since this should
   //be true if we're the only guys using multitex.
   glGetIntegerv ( GL_ACTIVE_TEXTURE_ARB, &query); 
   assert(query == GL_TEXTURE0_ARB);

   //Check if were actually applying the texture.
   //We need this to combine the diffuse color and texture properly
   GLboolean using_texture0 = glIsEnabled(GL_TEXTURE_2D) || glIsEnabled(GL_TEXTURE_1D);

   //XXX - Assume that if it's enabled, we're using MODULATE
   //If this not good enough, we must modify the programs to
   //implement the desired texturing scheme given by the query...
   if (using_texture0)
   {
      glGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &query);
      assert(query == GL_MODULATE);
   }

   // See if we're actually doing ye olde paper effect...
   bool using_texture1 = _is_active && (t != NULL);

   if (using_texture1)
   {
      //Compute stuff for the ndc->uv conversion matrix
      double uscale, vscale, uoff, voff, aspect;
      int w, h;   VIEW_SIZE(w,h);
      if (w >= h)
      {
         aspect = (double)w/(double)h;
         uscale = (double)w/(2.0*aspect*(double)t->image().width());
         vscale = (double)h/(2.0*(double)t->image().height());
         uoff = aspect;
         voff = 1.0;
      }
      else
      {
         aspect = (double)h/(double)w;
         uscale = (double)w/(2.0*(double)t->image().width());
         vscale = (double)h/(2.0*aspect*(double)t->image().height());
         uoff = 1.0;
         voff = aspect;
      }

      //Now switch to TEXTURE1 and stomp all over the old state
      glActiveTextureARB(GL_TEXTURE1_ARB);

      t->apply_texture(); //calls glBindTexture and sets state

      //Setup the scale matrix to convert from NDC -> UV
      glGetIntegerv ( GL_MATRIX_MODE, &query); 
      glMatrixMode( GL_TEXTURE );
      glLoadIdentity();
      glScaled( uscale, vscale, 1.0 );
      glTranslated( uoff - orig_x, voff - orig_y, 0.0 );
      glMatrixMode( query );

      glEnable(GL_TEXTURE_2D);

      //And return to the 1st texturing unit
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }


   //Now setup the fragment program...
   GLuint prog;
   if (using_texture1)
   {
      if (glIsEnabled(GL_TEXTURE_1D)) 
      {
         prog = _paper_with_1d_frag_prog_arb;
      }
      else if (glIsEnabled(GL_TEXTURE_2D)) 
      {
         prog = _paper_with_2d_frag_prog_arb;
      }
      else 
      {
         prog = _paper_with_no_frag_prog_arb;
      }
   }
   else
   {
      if (glIsEnabled(GL_TEXTURE_1D)) 
      {
         prog = _disabled_1d_frag_prog_arb;
      }
      else if (glIsEnabled(GL_TEXTURE_2D)) 
      {
         prog = _disabled_2d_frag_prog_arb;
      }
      else 
      {
         prog = _disabled_no_frag_prog_arb;
      }
   }

   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, prog); 

   float c = 2.0f * cont;
   float b = 2.0f * ((brig<=0.5f)?(brig):(1.0f-brig+(brig-0.5f)/cont));

   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, c, c, c, c);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, b, b, b, b);

   glEnable(GL_FRAGMENT_PROGRAM_ARB);  //GL_ENABLE_BIT

#endif

}


/////////////////////////////////////
//  end_paper_effect_nv()
/////////////////////////////////////
void
PaperEffect::end_paper_effect_nv(TEXTUREptr t)
{
   assert(_is_inited);
   assert(_is_supported);
   assert(_implementation & IMPLEMENTATION__GL_NV);

   //If not active, or no texture, then just premultiply alpha...
   if (!_is_active || t==NULL) 
   {
#ifdef GL_NV_register_combiners

   // Make sure these are still turned on
   assert(glIsEnabled(GL_REGISTER_COMBINERS_NV));
  
   // Kill the combiners
   glDisable(GL_REGISTER_COMBINERS_NV);

#endif
   }
   else
   {
#if defined(GL_NV_register_combiners) && defined(GL_ARB_multitexture)

   GLint query;

   // Make sure these are still turned on
   assert(glIsEnabled(GL_REGISTER_COMBINERS_NV));
  
   // Since nobody else should be playing with
   // multitexturing, we should be using the 1st unit
   glGetIntegerv ( GL_ACTIVE_TEXTURE_ARB, &query); 
   assert(query == GL_TEXTURE0_ARB);

   // Now change to 2nd unit
   glActiveTextureARB(GL_TEXTURE1_ARB);
   // Kill the combiners
   glDisable(GL_REGISTER_COMBINERS_NV);
   // Kill the 2nd texture
   glDisable(GL_TEXTURE_2D);
   // Return to the 1st unit
   glActiveTextureARB(GL_TEXTURE0_ARB);

#endif
   }   
}

/////////////////////////////////////
//  end_paper_effect_ati()
/////////////////////////////////////
void 
PaperEffect::end_paper_effect_ati(TEXTUREptr t)
{
   assert(_is_inited);
   assert(_is_supported);
   assert(_implementation & IMPLEMENTATION__GL_ATI);

#if defined(GL_ATI_fragment_shader) && defined(GL_ARB_multitexture) && !defined(NON_ATI_GFX)

   GLint query;

   // Make sure that fragment shader is enabled
   // XXX - Could even check that the shader has
   // an identifier that belongs to the set of
   // used by this effect, but that's just totally anal...
   assert(glIsEnabled(GL_FRAGMENT_SHADER_ATI));
   glDisable(GL_FRAGMENT_SHADER_ATI);

   // Since nobody else should be playing with
   // multitexturing, we should still be using the 1st unit...
   glGetIntegerv ( GL_ACTIVE_TEXTURE_ARB, &query); 
   assert(query == GL_TEXTURE0_ARB);

   // Turn off paper texture...
   bool using_texture1 = _is_active && (t != NULL);
   if (using_texture1)
   {
      // Now change to 2nd unit
      glActiveTextureARB(GL_TEXTURE1_ARB);

      // Sanity check
      assert(glIsEnabled(GL_TEXTURE_2D));

      // Kill the 2nd texture
      glDisable(GL_TEXTURE_2D);

      // Return to the 1st unit
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }

#endif
 
}

/////////////////////////////////////
//  end_paper_effect_arb()
/////////////////////////////////////
void
PaperEffect::end_paper_effect_arb(TEXTUREptr t)
{
   assert(_is_inited);
   assert(_is_supported);
   assert(_implementation & IMPLEMENTATION__GL_ARB);

   GL_VIEW::print_gl_errors("PaperEffect::end_paper_effect_arb() [Start] - ");

#if defined(GL_ARB_fragment_program) && defined(GL_ARB_multitexture)

   GLint query;

   // Make sure that fragment program is enabled
   // XXX - Could even check that the program has
   // an identifier that belongs to the set of
   // prgrams used by this effect, but that's
   // just totally anal...
   assert(glIsEnabled(GL_FRAGMENT_PROGRAM_ARB));
   glDisable(GL_FRAGMENT_PROGRAM_ARB);

   // Since nobody else should be playing with
   // multitexturing, we should still be using the 1st unit...
   glGetIntegerv ( GL_ACTIVE_TEXTURE_ARB, &query); 
   assert(query == GL_TEXTURE0_ARB);

   // Turn off paper texture...
   bool using_texture1 = _is_active && (t != NULL);
   if (using_texture1)
   {
      // Now change to 2nd unit
      glActiveTextureARB(GL_TEXTURE1_ARB);

      // Sanity check
      assert(glIsEnabled(GL_TEXTURE_2D));

      // Kill the 2nd texture
      glDisable(GL_TEXTURE_2D);

      // Return to the 1st unit
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }

#endif
   GL_VIEW::print_gl_errors("PaperEffect::end_paper_effect_arb() [End] - ");
}

/////////////////////////////////////
//  is_alpha_premult()
/////////////////////////////////////
bool
PaperEffect::is_alpha_premult()
{
   if (!_is_inited) init();

   return _is_supported;
}


/////////////////////////////////////
// get_texture() 
/////////////////////////////////////
TEXTUREptr
PaperEffect::get_texture(Cstr_ptr &in_tf, str_ptr &tf)
{
   int ind;

   tf = in_tf;

   //Do lazy initialization...
   if (!_paper_texture_names)
   {
      _paper_texture_names = new LIST<str_ptr>; assert(_paper_texture_names);
      _paper_texture_ptrs = new LIST<TEXTUREptr>; assert(_paper_texture_ptrs);

      _paper_texture_remap_orig_names = new LIST<str_ptr>; assert(_paper_texture_remap_orig_names);
      _paper_texture_remap_new_names = new LIST<str_ptr>; assert(_paper_texture_remap_new_names);

      int i = 0;
      while (paper_remap_fnames[i][0] != NULL)
      {
         _paper_texture_remap_orig_names->add(Config::JOT_ROOT() + paper_remap_base + paper_remap_fnames[i][0]);
         _paper_texture_remap_new_names->add(Config::JOT_ROOT() + paper_remap_base + paper_remap_fnames[i][1]);
         i++;
      }
   }

   //No paper
   if (tf == NULL_STR)
   {
      tf = NULL_STR;
   return NULL;
   }
   else if ((ind = _paper_texture_names->get_index(tf)) != BAD_IND)
   {
      //Finding original name in cache...

      //If it's a failed texture...
      if ((*_paper_texture_ptrs)[ind] == NULL)
      {
         //...see if it was remapped...
         int ii = _paper_texture_remap_orig_names->get_index(tf);
         //...and change to looking up the remapped name            
         if (ii != BAD_IND)
         {
            str_ptr old_tf = tf;
            tf = (*_paper_texture_remap_new_names)[ii];

            ind = _paper_texture_names->get_index(tf);

            err_mesg(ERR_LEV_SPAM, 
               "PaperEffect::get_texture() - Previously remapped -=<[{ (%s) ---> (%s) }]>=-", 
                  **old_tf, **tf );
         }
      }

      //Now see if the final name yields a good texture...
      if ((*_paper_texture_ptrs)[ind] != NULL)
      {
         err_mesg(ERR_LEV_SPAM, "PaperEffect::get_texture() - Using cached copy of texture.");
         tf = tf;
         return (*_paper_texture_ptrs)[ind];
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::get_texture() - **ERROR** Previous caching failure: '%s'...", **tf);
         tf = NULL_STR;
         return NULL;
      }
   }
   //Haven't seen this name before...
   else
   {
      err_mesg(ERR_LEV_SPAM, "PaperEffect::get_texture() - Not in cache: '%s'", **tf);

      Image i(**tf);

      //Can't load the texture?
      if (i.empty())
      {
         //...check for a remapped file...
         int ii = _paper_texture_remap_orig_names->get_index(tf);

         //...and use that name instead....
         if (ii != BAD_IND)
         {
            //...but also indicate that the original name is bad...

            _paper_texture_names->add(tf);
            _paper_texture_ptrs->add(NULL);

            str_ptr old_tf = tf;
            tf = (*_paper_texture_remap_new_names)[ii];

            err_mesg(ERR_LEV_ERROR, 
               "PaperEffect::get_texture() - Remapping --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                  **old_tf, **tf );

            i.load_file(**tf);
         }
      }

      //If the final name loads, store the cached texture...
      if (!i.empty())
      {
      TEXTUREglptr t = new TEXTUREgl();
         
         t->set_save_img(true);
         t->set_image(i.copy(),i.width(),i.height(),i.bpp());

         _paper_texture_names->add(tf);
         _paper_texture_ptrs->add(t);

         err_mesg(ERR_LEV_INFO, 
            "PaperEffect::get_texture() - Cached: (w=%d h=%d bpp=%u) %s", 
               i.width(), i.height(), i.bpp(), **tf);

         tf = tf;
      return t;
      }
      //Otherwise insert a failed NULL...
      else
      {
         err_mesg(ERR_LEV_ERROR, "PaperEffect::get_texture() - *****ERROR***** Failed loading to cache: '%s'", **tf);

         _paper_texture_names->add(tf);
         _paper_texture_ptrs->add(NULL);

         tf = NULL_STR;
         return NULL;
      }
   }
   // g++ 4.0 on macosx needs the following:
   return NULL;
}

/*
TEXTUREptr
PaperEffect::get_texture(Cstr_ptr &tf)
{
   int index;
   
   if (tf == NULL_STR)
   {
   return 0;
   }
   else if ((index = _paper_names.get_index(tf)) != BAD_IND)
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::get_texture() - Using cached copy of texture.");
      return _paper_textures[index];
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::get_texture() - Not in cache: %s", **tf);

      Image image(**tf);

      if (!image.empty())
      {
      TEXTUREglptr t = new TEXTUREgl("");
         t->set_save_img(true);
      t->set_image(image.copy(),image.width(),image.height(),image.bpp());

         err_mesg(ERR_LEV_INFO, "PaperEffect::get_texture() - Cached: w=%d h=%d bpp=%d", image.width(), image.height(), image.bpp());

         _paper_names.add(tf);
         _paper_textures.add(t);

      return t;
      }
      else
      {
         err_mesg(ERR_LEV_ERROR, "PaperEffect::get_texture() - Error loading to cache: %s", **tf);
         return 0;
      }
   }   
}
*/

/////////////////////////////////////
// check_new_paper() 
/////////////////////////////////////
bool
PaperEffect::check_new_paper()
{
   //XXX - Since mipmapping is NOT on, these textures must have 2^n dimensions? (REALLY?)

   str_ptr new_paper_filename = (_paper_tex)?(Config::JOT_ROOT() + _paper_tex):(NULL_STR);

   if (new_paper_filename != _paper_filename)        
   {
      str_ptr ret_filename;
      
      _paper_texture = get_texture(new_paper_filename, ret_filename);
      _paper_filename = ret_filename;

      if (ret_filename == NULL_STR)
      {
         _paper_tex = NULL_STR;         
      }
      else
      {
         //JOT_ROOT should be at start of this filename...
         assert(strstr(**_paper_filename,**Config::JOT_ROOT()) == **_paper_filename );

         //Now strip it off...
         _paper_tex = &((**_paper_filename)[Config::JOT_ROOT().len()]);
      }


      notify_paper_changed();

      return true;
   }
   return false;
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
PaperEffect::init()
{
   GL_VIEW::print_gl_errors("PaperEffect::init() [Start] - ");

   assert(!_is_inited);

   _is_inited = true;

   GLExtensions::init();

   _is_supported = false;

   _implementation = IMPLEMENTATION__NONE;

   err_mesg(ERR_LEV_INFO, "\nPaperEffect::init() - Querying hardware support...");      

   // Check for GL_ARB_multitexture...
   err_mesg(ERR_LEV_INFO, "PaperEffect::init() - Trying for GL_ARB_multitexture...");      
   
   if (init_tex())
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_ARB_multitexture is available.");      

      // Check for GL_ARB_fragement_program...
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - Trying for GL_ARB_fragement_program...");      
      if (init_arb())
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_ARB_fragement_program is available.");      
         
         _is_supported = true;
         _implementation |= IMPLEMENTATION__GL_ARB;
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_ARB_fragement_program is *NOT* available.");      
      }   

      // Check for GL_NV_register_combiners...
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - Trying for GL_NV_register_combiners...");      
      
      if (init_nv())
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_NV_register_combiners is available.");      
         
         _is_supported = true;
         _implementation |= IMPLEMENTATION__GL_NV;
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_NV_register_combiners is *NOT* available."); 
      }

      // Check for GL_ATI_fragment_shader...
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - Trying for GL_ATI_fragment_shader...");      
      
      if (init_ati())
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_ATI_fragment_shader is available.");      
         
         _is_supported = true;
         _implementation |= IMPLEMENTATION__GL_ATI;
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_ATI_fragment_shader is *NOT* available."); 
      }
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...GL_ARB_multitexture is *NOT* available.");      
   }

   if (_is_supported)
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...hardware support available!");
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init() - ...hardware support *NOT* available! **SORRY**");
   }

   GL_VIEW::print_gl_errors("PaperEffect::init() [End] - ");

}

/////////////////////////////////////
// init_tex()
/////////////////////////////////////
bool
PaperEffect::init_tex()
{
   bool ret = false;

   if (GLExtensions::gl_arb_multitexture_supported()) 
   {
#ifdef GL_ARB_multitexture
      GLint num_texture = 0;
      glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &num_texture);
      if (num_texture >= 2)
      {
         ret = true;
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_tex() - Need 2 TEX units, found %d.", num_texture);
      }
      else 
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_tex() - Need 2 TEX units, *ONLY* found %d!", num_texture);
#endif
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init_nv() - GL_ARB_multitexture is NOT supported by hardware!");
   }

   return ret;
}

/////////////////////////////////////
// init_nv()
/////////////////////////////////////
bool
PaperEffect::init_nv()
{
   bool ret = false;

   if (GLExtensions::gl_nv_register_combiners_supported()) 
   {
#ifdef GL_NV_register_combiners
      GLint num_stage = 0;
      glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV,&num_stage);
      if (num_stage >= 2) 
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_nv() - Need 2 general combiner stages, found %d.", num_stage);
         ret = true;
      }
      else 
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_nv() - Need 2 general combiner stages, *ONLY* found %d!", num_stage);
#endif
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init_nv() - GL_NV_register_combiners is NOT supported by hardware!");
   }

   return ret;
}

/////////////////////////////////////
// init_ati()
/////////////////////////////////////
bool
PaperEffect::init_ati()
{
   bool ret = false;

   if (GLExtensions::gl_ati_fragment_shader_supported()) 
   {
   
#if !defined(NON_ATI_GFX)
#ifdef GL_ATI_fragment_shader

      GL_VIEW::print_gl_errors("PaperEffect::init_ati() [Start] - ");

      ret = true;

      err_mesg(ERR_LEV_INFO, "PaperEffect::init_ati() - Compiling fragment shader...");

      _paper_frag_shader_ati = glGenFragmentShadersATI(1); 
      glBindFragmentShaderATI(_paper_frag_shader_ati); 

      glBeginFragmentShaderATI();

      glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0_ARB, GL_SWIZZLE_STQ_DQ_ATI); 
      glSampleMapATI(GL_REG_1_ATI, GL_TEXTURE1_ARB, GL_SWIZZLE_STQ_DQ_ATI); 

      //Pass 1 - Instruction Pair 1
      //R0[rgb] <- R0[rgb] * PRIM[rgb] {modulate by TEX0}
      //R0[a] <-   R0[a]   * PRIM[a]
      glColorFragmentOp2ATI( GL_MUL_ATI,                         
                                 GL_REG_0_ATI,           GL_NONE,    GL_NONE,  
                                 GL_REG_0_ATI,           GL_NONE,    GL_NONE,  
                                 GL_PRIMARY_COLOR_ARB,   GL_NONE,    GL_NONE); 

      glAlphaFragmentOp2ATI( GL_MUL_ATI, 
                                 GL_REG_0_ATI,                       GL_NONE,
                                 GL_REG_0_ATI,           GL_NONE,    GL_NONE,
                                 GL_PRIMARY_COLOR_ARB,   GL_NONE,    GL_NONE);


      //Pass 1 - Instruction Pair 2
      //R4[rgb] <- (C0[rgb]>0.5) ? R0[rgb] : PRIM[rgb] {load PRIM or PRIM*TEX0 into R0 depending on C0}
      //R4[a]   <- (C0[a]>0.5)   ? R0[a]   : PRIM[a]
      glColorFragmentOp3ATI( GL_CND_ATI,                         
                                 GL_REG_0_ATI,           GL_NONE,    GL_NONE,  
                                 GL_REG_0_ATI,           GL_NONE,    GL_NONE,  
                                 GL_PRIMARY_COLOR_ARB,   GL_NONE,    GL_NONE,                                   
                                 GL_CON_0_ATI,           GL_NONE,    GL_NONE); 
      glAlphaFragmentOp3ATI( GL_CND_ATI, 
                                 GL_REG_0_ATI,                       GL_NONE,  
                                 GL_REG_0_ATI,           GL_NONE,    GL_NONE,  
                                 GL_PRIMARY_COLOR_ARB,   GL_NONE,    GL_NONE,                                   
                                 GL_CON_0_ATI,           GL_NONE,    GL_NONE); 

      //Pass 1 - Instruction Pair 4
      //R2[a] <- clamp(2*R0[a])
      glAlphaFragmentOp1ATI( GL_MOV_ATI, 
                                 GL_REG_2_ATI,                       GL_SATURATE_BIT_ATI,
                                 GL_REG_0_ATI,           GL_NONE,    GL_2X_BIT_ATI);


      //Pass 1 - Instruction Pair 5
      //R3[a] <- clamp(2*(R0[a]-.5))
      glAlphaFragmentOp1ATI( GL_MOV_ATI, 
                                 GL_REG_3_ATI,                       GL_SATURATE_BIT_ATI,
                                 GL_REG_0_ATI,           GL_NONE,    GL_BIAS_BIT_ATI | GL_2X_BIT_ATI);
      
      
      //Pass 1 - Instruction Pair 6
      //R1[a] <- C1[a] + 2*C2[a]*R1[a] {(0.5-cont)+2(2*brig'*cont)*h}
      glAlphaFragmentOp3ATI( GL_MAD_ATI, 
                                 GL_REG_1_ATI,                       GL_SATURATE_BIT_ATI,
                                 GL_CON_2_ATI,           GL_NONE,    GL_2X_BIT_ATI,
                                 GL_REG_1_ATI,           GL_NONE,    GL_NONE,
                                 GL_CON_1_ATI,           GL_NONE,    GL_NONE);

      //Pass 1 - Instruction Pair 7
      //R0[a] <- R1[a]*R2[a] + (1-R1[a])*R3[a]
      glAlphaFragmentOp3ATI( GL_LERP_ATI, 
                                 GL_REG_0_ATI,                       GL_NONE,
                                 GL_REG_1_ATI,           GL_NONE,    GL_NONE,
                                 GL_REG_2_ATI,           GL_NONE,    GL_NONE,
                                 GL_REG_3_ATI,           GL_NONE,    GL_NONE);

      //Pass 1 - Instruction Pair 8
      //R0[rgb] <- R0[rgb] * R0[aaa]

      glColorFragmentOp2ATI( GL_MUL_ATI,                           //ADD,SUB,MUL,MAD,LERP,MOV,CND,CND0,DOTn
                                 GL_REG_0_ATI, GL_NONE,  GL_NONE,  //NONE = RED | GREEN | BLUE
                                                                   //EIGHTH, QUARTH, HALF, 2X, 4X, 8X | SATURATE
                                 GL_REG_0_ATI, GL_NONE,  GL_NONE,  //RED , GREEN , BLUE, ALPHA, NONE
                                                                   //NEGATE | COMP |BIAS | 2X [-(2*((1.0-s)-0.5))]
                                 GL_REG_0_ATI, GL_ALPHA, GL_NONE); 


      glEndFragmentShaderATI();

      ret = !GL_VIEW::print_gl_errors("PaperEffect::init_ati() [End] - ") && ret;

      if (ret)
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_ati() - ...done.");
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_ati() - ...failed!!");
      }
#endif
#endif
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init_ati() - GL_ATI_fragment_shader is NOT supported by hardware!");
   }

   return ret;
}

/////////////////////////////////////
// init_arb()
/////////////////////////////////////
bool
PaperEffect::init_arb()
{
   bool ret = false;

   //XXX - Make sure we're not getting here with pending errors...
   GL_VIEW::print_gl_errors("PaperEffect::init_arb() [Start] - ");

   if (GLExtensions::gl_arb_fragment_program_supported())
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init_arb() - GL_ARB_fragment_program supported. Trying programs...");
#ifdef GL_ARB_fragment_program

      bool n, native = true;

      ret = true; 

      glGenProgramsARB(1, &_disabled_no_frag_prog_arb); 
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _disabled_no_frag_prog_arb); 
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(_DisabledNoFragProgARB), _DisabledNoFragProgARB);
      ret = ret && GLExtensions::gl_arb_fragment_program_loaded("PaperEffect::init_arb() (Off - No Tex) - ", n, (const unsigned char *)_DisabledNoFragProgARB);
      native = native && n;
      
      glGenProgramsARB(1, &_disabled_1d_frag_prog_arb); 
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _disabled_1d_frag_prog_arb); 
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(_Disabled1DFragProgARB), _Disabled1DFragProgARB);
      ret = ret && GLExtensions::gl_arb_fragment_program_loaded("PaperEffect::init_arb() (Off - 1D Tex) - ", n, (const unsigned char *)_Disabled1DFragProgARB);
      native = native && n;

      glGenProgramsARB(1, &_disabled_2d_frag_prog_arb); 
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _disabled_2d_frag_prog_arb); 
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(_Disabled2DFragProgARB), _Disabled2DFragProgARB);
      ret = ret && GLExtensions::gl_arb_fragment_program_loaded("PaperEffect::init_arb() (Off - 2D Tex) - ", n, (const unsigned char *)_Disabled2DFragProgARB);
      native = native && n;

      glGenProgramsARB(1, &_paper_with_no_frag_prog_arb); 
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _paper_with_no_frag_prog_arb); 
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(_PaperWithNoFragProgARB), _PaperWithNoFragProgARB);
      ret = ret && GLExtensions::gl_arb_fragment_program_loaded("PaperEffect::init_arb() (On - No Tex) - ", n, (const unsigned char *)_PaperWithNoFragProgARB);
      native = native && n;
      
      glGenProgramsARB(1, &_paper_with_1d_frag_prog_arb); 
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _paper_with_1d_frag_prog_arb); 
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(_PaperWith1DFragProgARB), _PaperWith1DFragProgARB);
      ret = ret && GLExtensions::gl_arb_fragment_program_loaded("PaperEffect::init_arb() (On - 1D Tex) - ", n, (const unsigned char *)_PaperWith1DFragProgARB);
      native = native && n;

      glGenProgramsARB(1, &_paper_with_2d_frag_prog_arb); 
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _paper_with_2d_frag_prog_arb); 
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(_PaperWith2DFragProgARB), _PaperWith2DFragProgARB);
      ret = ret && GLExtensions::gl_arb_fragment_program_loaded("PaperEffect::init_arb() (On - 2D Tex) - ", n, (const unsigned char *)_PaperWith2DFragProgARB);
      native = native && n;

      if (ret)
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_arb() - ...all programs loaded successfully, ");
         if (native)
         {
            err_mesg(ERR_LEV_INFO, "PaperEffect::init_arb() - ...and will run native in hardware!");
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "PaperEffect::init_arb() - ...but all will *NOT* run native in hardware!");
         }
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "PaperEffect::init_arb() - ...all programs *NOT* loaded successfully.");
      }
      
#endif
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "PaperEffect::init_arb() - GL_ARB_fragment_program is NOT supported by hardware!");
   }

   GL_VIEW::print_gl_errors("PaperEffect::init_arb() [End] - ");

   return ret;
}
