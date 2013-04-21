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
/*!
 *  \file curvature_texture.C
 *  \brief Contains the implementation of the classes that implement the
 *  curvature visualization gTexture.
 *
 */

#include <cassert>

using namespace std;

#include "gtex/gl_extensions.H"
#include <GL/glu.h>

#include "gtex/rendering_mode.H"
#include "gtex/curvature_texture.H"

#include "mlib/points.H"

using namespace mlib;

/* Utility Function Prototypes */

// static void make_1D_texture(GLuint texture_name, int texture_size, int line_width,
//                          bool just_line);

static void make_2D_texture(GLuint texture_name, int texture_size, int line_width,
                            bool just_line);

static GLuint load_ARB_program(const char *header, GLenum target,
                               const char *program_string);

//----------------------------------------------------------------------------//

const char *CURVATURE_VERTEX_PROGRAM_STR =
"\
!!ARBvp1.0\n\
\n\
####################################################################\n\
### Real-time Silhouettes and Suggestive Contours\n\
### Vertex program for computing view dependent curvature values\n\
####################################################################\n\
\n\
## Inputs:   ipos : Vertex position\n\
##           ifpd : First principal direction and curvature (k1 in w component)\n\
##           ispd : Second principal direction and curvature (k2 in w component)\n\
##           ider : Derivative of curvature (unique dcurv tensor values)\n\
\n\
ATTRIB ipos = vertex.position;\n\
ATTRIB ifpd = vertex.attrib[6];\n\
ATTRIB ispd = vertex.attrib[7];\n\
ATTRIB ider = vertex.attrib[15];\n\
\n\
## Output:   opos = Output position\n\
##           tc0 = {radial curvature, derivative of radial curvature, n dot v}\n\
\n\
OUTPUT opos = result.position;\n\
OUTPUT ocol = result.color;      # Not used\n\
OUTPUT tc0 = result.texcoord[0]; # {rcurv, drcurv, ndotv}\n\
OUTPUT tc1 = result.texcoord[1];\n\
OUTPUT tc2 = result.texcoord[2];\n\
OUTPUT tc3 = result.texcoord[3];\n\
OUTPUT tc4 = result.texcoord[4];\n\
\n\
PARAM MVP[4] = { state.matrix.mvp };                  # Modelview*projection matrix\n\
PARAM MV[4] = { state.matrix.modelview };             # Modelview matrix\n\
PARAM MVIT[4] = { state.matrix.modelview.invtrans };  # Modelview^-1 matrix (used to get eye pos)\n\
\n\
PARAM PARAM_DRAW_ENABLE = program.env[0];\n\
PARAM PARAM_FEATURE = program.env[1];\n\
PARAM EYE_POS = program.env[2];\n\
\n\
# Vertex normal vector:\n\
TEMP normvec;\n\
\n\
# Eye vector:\n\
TEMP eyevec;\n\
\n\
# Computed pricipal direction values:\n\
#     *pdvals.x = cos(angle between principal direction and eye vector)\n\
#     *pdvals.y = (*pdvals.x)^2\n\
#     *pdvals.z = principal curvature times *pdvals.y\n\
#     *pdvals.w = not used\n\
TEMP fpdvals, spdvals;\n\
\n\
# Final scalar field values:\n\
#     finalsf.x = radial curvature\n\
#     finalsf.y = derivative of radial curvature\n\
#     finalsf.z = n dot v\n\
#     finalsf.w = not used\n\
TEMP finalsf;\n\
\n\
# Derivatives of principal curvatures:\n\
#     dks.x = derivative of k1\n\
" "\
#     dks.y = derivative of k2\n\
#     dks.z = not used\n\
#     dks.w = not used\n\
TEMP dks;\n\
\n\
# Temporary register (used during various computations)\n\
TEMP   temp, temp2;\n\
\n\
#############################################\n\
## Transform vertex position\n\
#############################################\n\
\n\
DP4 opos.x, MVP[0], ipos;\n\
DP4 opos.y, MVP[1], ipos;\n\
DP4 opos.z, MVP[2], ipos;\n\
DP4 opos.w, MVP[3], ipos;\n\
\n\
#############################################\n\
## Compute view dependent curvature values\n\
#############################################\n\
\n\
# Compute normal vector (XXX - Maybe replace with GL normal attribute)\n\
XPD normvec.xyz, ifpd, ispd;\n\
\n\
# Compute eye vector, normalize\n\
SUB eyevec, EYE_POS, ipos;\n\
DP3 eyevec.w, eyevec, eyevec;\n\
RSQ eyevec.w, eyevec.w;\n\
MUL eyevec.xyz, eyevec.w, eyevec;\n\
\n\
# Compute n dot v\n\
DP3 finalsf.z, normvec, eyevec;\n\
\n\
# Assuming that ifpd and eyevec are normalized so that dot product is equal to\n\
# the angle between them:\n\
DP3 fpdvals.x, ifpd, eyevec;           # fpdvals.x = u = cos\n\
MUL fpdvals.y, fpdvals.x, fpdvals.x;   # fpdvals.y = u^2 = cos^2\n\
MUL fpdvals.z, fpdvals.y, ifpd.w;      # fpdvals.z = k1 * u^2\n\
\n\
# Assuming that ispd and eyevec are normalized so that dot product is equal to\n\
# the angle between them:\n\
DP3 spdvals.x, ispd, eyevec;           # spdvals.x = v = cos\n\
MUL spdvals.y, spdvals.x, spdvals.x;   # spdvals.y = v^2 = cos^2\n\
MUL spdvals.z, spdvals.y, ispd.w;      # spdvals.z = k2 * v^2\n\
\n\
# Compute radial curvature:\n\
ADD finalsf.x, fpdvals.z, spdvals.z;   # k1 * u^2 + k2 * v^2\n\
\n\
# Compute derivative of radial curvature:\n\
\n\
# First, calculate derivative of normal curvature:\n\
PARAM three= {3.0, 0.0, 0.0, 0.0};\n\
MUL temp.x, fpdvals.x, ider.x;   # u*P\n\
MUL temp.y, spdvals.x, ider.y;   # v*Q\n\
MUL temp.y, three.x, temp.y;     # 3*v*Q\n\
ADD temp.x, temp.x, temp.y;      # u*P + 3*v*Q\n\
MUL temp.x, temp.x, fpdvals.y;   # u^3*P + 3*u^2*v*Q\n\
\n\
MUL temp.y, fpdvals.x, ider.z;   # u*S\n\
MUL temp.y, three.x, temp.y;     # 3*u*S\n\
MUL temp.z, spdvals.x, ider.w;   # v*T\n\
" "\
ADD temp.y, temp.y, temp.z;      # 3*u*S + v*T\n\
MUL temp.y, temp.y, spdvals.y;   # 3*u*v^2*S + v^3*T\n\
\n\
ADD temp.x, temp.x, temp.y;      # u^3*P + 3*u^2*v*Q + 3*u*v^2*S + v^3*T\n\
\n\
# Then add offset to deriv of normal curvature:\n\
#      float csc2theta = 1.0f / (u2 + v2);\n\
#      sctest_num[i] *= csc2theta;\n\
#      float tr = (themesh->curv2[i] - themesh->curv1[i]) *\n\
#            u * v * csc2theta;\n\
#      sctest_num[i] -= 2.0f * ndotv[i] * sqr(tr);\n\
\n\
ADD temp.y, fpdvals.y, spdvals.y;   # u^2 + v^2\n\
RCP temp.y, temp.y;                 # 1 / sin^2(theta)\n\
MUL temp.x, temp.x, temp.y;         # rescale deriv\n\
\n\
# Calculate torsion (tr):\n\
SUB temp.z, ispd.w, ifpd.w;      # (k2 - k1)\n\
MUL temp.z, temp.z, fpdvals.x;   # (k2 - k1)*u\n\
MUL temp.z, temp.z, spdvals.x;   # (k2 - k1)*u*v\n\
MUL temp.y, temp.y, temp.z;      # tr = (k2 - k1)*u*v / sin^2(theta)\n\
MUL temp.y, temp.y, temp.y;      # tr^2\n\
PARAM two = {2,0,0,0};\n\
MUL temp.y, two.x, temp.y;       # 2 * tr^2\n\
MUL temp.y, temp.y, finalsf.z;   # 2 * tr^2 * (n dot v)\n\
SUB temp.x, temp.x, temp.y;      # whew!\n\
\n\
# #extra stuff necessary for texture\n\
# #      if (extra_sin2theta)\n\
# #         sctest_num[i] *= u2 + v2;\n\
# ADD temp.y, fpdvals.y, spdvals.y; # u2 + v2\n\
# MUL temp.x, temp.x, temp.y;\n\
\n\
# #      sctest_den[i] = ndotv[i];\n\
# #      sctest_num[i] -= scthresh * sctest_den[i];\n\
# PARAM thresh = {3.5,0,0,0};\n\
MUL temp.y, PARAM_FEATURE.x, finalsf.z;\n\
RCP temp.z, PARAM_FEATURE.w;\n\
MUL temp.y, temp.y, temp.z;\n\
SUB temp.x, temp.x, temp.y;\n\
\n\
# #sctest_num[i] = sctest_num[i] * feature_size2 / sctest_den[i];\n\
RCP temp.y, finalsf.z;\n\
MUL temp.x, temp.x, temp.y;\n\
MUL temp.x, temp.x, PARAM_FEATURE.w;\n\
\n\
MOV finalsf.y, temp.x;\n\
\n\
# Scale scalar fields by feature size\n\
MUL finalsf.x, finalsf.x, PARAM_FEATURE.z;\n\
\n\
#TODO: this constant is different than RTSC- FIGURE OUT WHAT IT SHOULD BE\n\
PARAM fs={32,0,0,0};\n\
MUL finalsf.y, finalsf.y, fs.x;\n\
\n\
# If disabling contours or suggestive contours is toggled, set the\n\
# scalar field values to some appropriate constants\n\
" "\
MAD finalsf.z, finalsf.z, PARAM_DRAW_ENABLE.x, PARAM_DRAW_ENABLE.y;\n\
MAD finalsf.x, finalsf.x, PARAM_DRAW_ENABLE.z, PARAM_DRAW_ENABLE.w;\n\
\n\
# Project eyevec onto the tangent plane and normalize (w):\n\
MUL temp, normvec, finalsf.zzzz;\n\
SUB temp, eyevec, temp;\n\
DP3 temp.w, temp, temp;\n\
RSQ temp.w, temp.w;\n\
MUL temp.xyz, temp.w, temp;\n\
\n\
# Compute dot products with principal directions\n\
DP3 temp2.x, temp, ifpd; # w dot e1\n\
DP3 temp2.y, temp, ispd; # w dot e2\n\
\n\
# Compute derivatives of principal curvatures in w direction:\n\
MUL dks.x, temp2.x, ider.x;         # (w dot e1) * D_e1(k1)\n\
MAD dks.x, temp2.y, ider.y, dks.x;  # (w dot e1) * D_e1(k1) + (w dot e2) * D_e2(k1)\n\
MUL dks.y, temp2.x, ider.z;         # (w dot e1) * D_e1(k1)\n\
MAD dks.y, temp2.y, ider.w, dks.y;  # (w dot e1) * D_e1(k1) + (w dot e2) * D_e2(k1)\n\
\n\
MOV tc0, finalsf;\n\
MOV tc1, ifpd;\n\
MOV tc2, ispd;\n\
MOV tc3, ider;\n\
SWZ tc4, dks, x, y, 0, 0;\n\
\n\
END\n\
";

//----------------------------------------------------------------------------//

const char *CURVATURE_FRAGMENT_PROGRAM_STR =
"\
!!ARBfp1.0 \n\
\n\
ATTRIB tex0 = fragment.texcoord[0];\n\
ATTRIB tex1 = fragment.texcoord[1];\n\
ATTRIB tex2 = fragment.texcoord[2];\n\
ATTRIB tex3 = fragment.texcoord[3];\n\
ATTRIB tex4 = fragment.texcoord[4];\n\
ATTRIB col0 = fragment.color; \n\
ATTRIB ipos = fragment.position;\n\
\n\
OUTPUT out = result.color;\n\
\n\
PARAM PARAM_DRAW_CURVATURE = program.env[0];\n\
PARAM PARAM_RADIAL_FILTER = program.env[1];\n\
PARAM PARAM_MEAN_FILTER = program.env[2];\n\
PARAM PARAM_GAUSSIAN_FILTER = program.env[3];\n\
\n\
TEMP temp, temp2, temp3;   #temporary\n\
\n\
TEMP k_r, H, K; # Radial, Gaussian and mean curvature\n\
TEMP dH, dK; # Derivatives of Gaussian and mean curvatures\n\
\n\
TEMP k_r_color, H_color, K_color, final_color;\n\
\n\
PARAM one = {0.0,1.0,-1,1};\n\
PARAM half = {0.5, 1.0, 1.0, 1.0};\n\
PARAM big = {1.0, 0, 0, 0};\n\
\n\
PARAM red   = {0.0, 1.0, 1.0, 1.0};\n\
PARAM green = {1.0, 0.0, 1.0, 1.0};\n\
PARAM blue  = {1.0, 1.0, 0.0, 1.0};\n\
\n\
# Derivative of mean curvature:\n\
ADD dH.x, tex4.x, tex4.y;\n\
MUL dH.x, dH.x, 0.5.x;\n\
\n\
# Derivative of Gaussian curvature:\n\
MUL dK.x, tex1.w, tex4.y;       # k1 * D_w(k2)\n\
MAD dK.x, tex2.w, tex4.x, dK.x; # k1 * D_w(k2) + k2 * D_w(k1)\n\
\n\
# Radial curvature (k_r):\n\
SWZ k_r, tex0, x, 1, 0, 0;\n\
ADD_SAT k_r, k_r, {0.5, 0.5, 0.0, 0.0};\n\
TEX k_r_color, k_r, texture[2], 2D;\n\
#CMP temp.x, tex0.y, 0.25.x, 1.0.x;\n\
#MUL k_r_color, k_r_color, temp.xxxx;\n\
MUL k_r_color, k_r_color, red;\n\
CMP temp.x, PARAM_RADIAL_FILTER.x, tex0.y, 1.0.x;\n\
CMP temp.y, PARAM_RADIAL_FILTER.y, dH.x, 1.0.x;\n\
CMP temp.z, PARAM_RADIAL_FILTER.z, dK.x, 1.0.x;\n\
MUL temp.x, temp.x, temp.y;\n\
MUL temp.x, temp.x, temp.z;\n\
CMP temp.x, temp.x, PARAM_RADIAL_FILTER.w, 1.0.x;\n\
MUL k_r_color, k_r_color, temp.xxxx;\n\
MUL k_r_color, k_r_color, PARAM_DRAW_CURVATURE.xxxx;\n\
#SLT temp.x, tex0.z, 0.99.x;\n\
#MUL k_r_color, k_r_color, temp.xxxx;\n\
\n\
# Mean curvature (H):\n\
ADD H.x, tex1.w, tex2.w;\n\
MUL H.x, H.x, 0.5.x;\n\
ADD_SAT H.x, H.x, 0.5.x;\n\
MOV H.y, 1.0.x;\n\
TEX H_color, H, texture[0], 2D;\n\
MUL H_color, H_color, green;\n\
CMP temp.x, PARAM_MEAN_FILTER.x, tex0.y, 1.0.x;\n\
" "\
CMP temp.y, PARAM_MEAN_FILTER.y, dH.x, 1.0.x;\n\
CMP temp.z, PARAM_MEAN_FILTER.z, dK.x, 1.0.x;\n\
MUL temp.x, temp.x, temp.y;\n\
MUL temp.x, temp.x, temp.z;\n\
CMP temp.x, temp.x, PARAM_MEAN_FILTER.w, 1.0.x;\n\
MUL H_color, H_color, temp.xxxx;\n\
MUL H_color, H_color, PARAM_DRAW_CURVATURE.yyyy;\n\
\n\
# Gaussian curvature (K):\n\
MUL K.x, tex1.w, tex2.w;\n\
ADD_SAT K.x, K.x, 0.5.x;\n\
MOV K.y, 1.0.x;\n\
TEX K_color, K, texture[1], 2D;\n\
MUL K_color, K_color, blue;\n\
CMP temp.x, PARAM_GAUSSIAN_FILTER.x, tex0.y, 1.0.x;\n\
CMP temp.y, PARAM_GAUSSIAN_FILTER.y, dH.x, 1.0.x;\n\
CMP temp.z, PARAM_GAUSSIAN_FILTER.z, dK.x, 1.0.x;\n\
MUL temp.x, temp.x, temp.y;\n\
MUL temp.x, temp.x, temp.z;\n\
CMP temp.x, temp.x, PARAM_GAUSSIAN_FILTER.w, 1.0.x;\n\
MUL K_color, K_color, temp.xxxx;\n\
MUL K_color, K_color, PARAM_DRAW_CURVATURE.zzzz;\n\
\n\
MOV final_color, {0.0, 0.0, 0.0, 1.0};\n\
ADD_SAT final_color, final_color, k_r_color;\n\
ADD_SAT final_color, final_color, H_color;\n\
ADD_SAT final_color, final_color, K_color;\n\
SUB final_color, {1.0, 1.0, 1.0, 0.0}, final_color;\n\
\n\
MOV out, final_color;\n\
\n\
#~ #contours\n\
#~ MOV temp2.x, tex0.z;\n\
#~ ADD temp2.x, temp2.x, half.x;\n\
#~ MOV temp2.y, one.y;\n\
#~ TEX c_color, temp2, texture[0], 2D;\n\
\n\
#~ ADD c_color, c_color, green;\n\
#~ MIN c_color, one.yyyy, c_color;\n\
\n\
#~ #SC\n\
#~ MOV temp.x, tex0.x;\n\
#~ ADD temp.x, temp.x, half.x;\n\
#~ MOV temp.y, one.y;\n\
#~ TEX sc_color, temp, texture[1], 2D;\n\
\n\
#~ ADD sc_color, sc_color, blue;\n\
#~ MIN sc_color, one.yyyy, sc_color;\n\
\n\
#~ # RCP temp.w, tex0.y;\n\
#~ # POW temp.w, tex0.y, big.x;\n\
\n\
#~ # MUL temp.w, one.z, temp.w;\n\
#~ # CMP temp.y, tex0.y, temp.w, one.y;\n\
\n\
#~ SUB sc_color, one.yyyy, sc_color;\n\
#~ MUL sc_color, sc_color, tex0.yyyy;\n\
#~ SUB sc_color, one.yyyy, sc_color;\n\
\n\
#~ MUL final_color, sc_color, c_color;\n\
\n\
END\n\
";

//----------------------------------------------------------------------------//

/*!
 *  \brief Abstract base class for all curvature gTexture rendering modes.
 *
 */
class CurvatureRenderingMode : public RenderingMode {
   
   public:
   
      class CurvatureStripCB : public RenderingModeStripCB {
         
      };
   
      virtual ~CurvatureRenderingMode() { }
   
};

//----------------------------------------------------------------------------//

/*!
 *  \brief A curvature gTexture rendering mode that uses ARB vertex and
 *  fragment programs and multitexturing.
 *
 */
class CurvatureARBvpARBfpMultiTextureMode : public CurvatureRenderingMode {
   
   public:
   
      class StripCB : public CurvatureRenderingMode::CurvatureStripCB {
         
         public:
         
            //! \brief "face" callback.
            virtual void faceCB(CBvert* v, CBface* f);
         
      };
      
      CurvatureARBvpARBfpMultiTextureMode();
      
      ~CurvatureARBvpARBfpMultiTextureMode();
      
      virtual void setup_for_drawing_outside_dl(const Patch *patch);
      virtual void setup_for_drawing_inside_dl(const Patch *patch);
      
      virtual GLStripCB *get_new_strip_cb() const
         { return new StripCB; }
      
   private:
   
      GLuint vprog_name, fprog_name;
      GLuint line_texture_name, region_texture_name, rcurv_texture_name;
   
};

CurvatureARBvpARBfpMultiTextureMode::CurvatureARBvpARBfpMultiTextureMode()
   : vprog_name(0), fprog_name(0), line_texture_name(0), region_texture_name(0)
{
   
   if(GLExtensions::gl_arb_vertex_program_supported()){
      
#ifdef GL_ARB_vertex_program

      vprog_name = load_ARB_program("CurvatureTexture::init() - ",
                                    GL_VERTEX_PROGRAM_ARB,
                                    CURVATURE_VERTEX_PROGRAM_STR);

#endif // GL_ARB_vertex_program
      
   }
   
   if(GLExtensions::gl_arb_fragment_program_supported()){
      
#ifdef GL_ARB_fragment_program

      fprog_name = load_ARB_program("CurvatureTexture::init() - ",
                                    GL_FRAGMENT_PROGRAM_ARB,
                                    CURVATURE_FRAGMENT_PROGRAM_STR);

#endif // GL_ARB_fragment_program
      
   }
   
   if(GLExtensions::gl_arb_multitexture_supported()){
      
      glGenTextures(1, &line_texture_name);
      make_2D_texture(line_texture_name, 1024, 2, true);
      
      glGenTextures(1, &region_texture_name);
      make_2D_texture(region_texture_name, 1024, 2, false);
      
      glGenTextures(1, &rcurv_texture_name);
      make_2D_texture(rcurv_texture_name, 1024, 2, true);
      
   }
   
}

CurvatureARBvpARBfpMultiTextureMode::~CurvatureARBvpARBfpMultiTextureMode()
{
   
#if defined(GL_ARB_vertex_program) || defined(GL_ARB_fragment_program)

   if(vprog_name)
      glDeleteProgramsARB(1, &vprog_name);
   
   if(fprog_name)
      glDeleteProgramsARB(1, &fprog_name);
      
#endif

   if(line_texture_name)
      glDeleteTextures(1, &line_texture_name);
   
   if(region_texture_name)
      glDeleteTextures(1, &region_texture_name);
   
   if(rcurv_texture_name)
      glDeleteTextures(1, &rcurv_texture_name);
   
}

void
CurvatureARBvpARBfpMultiTextureMode::setup_for_drawing_outside_dl(const Patch *patch)
{
   
#ifdef GL_ARB_vertex_program
   
   glEnable(GL_VERTEX_PROGRAM_ARB);
   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vprog_name);
   
   GLfloat sc_thresh = CurvatureTexture::get_sugcontour_thresh();
   
   // Setting various parameters:
   glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1,
                              sc_thresh,          // Suggestive Contour Threshold
                              1.0f,               // Bounding sphere radius (not used here)
                              static_cast<GLfloat>(patch->mesh()->avg_len()),
                              static_cast<GLfloat>(sqr(patch->mesh()->avg_len())) );
   
   Wpt eye = VIEW::eye();
                              
   glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2,
                              static_cast<GLfloat>(eye[0]),
                              static_cast<GLfloat>(eye[1]),
                              static_cast<GLfloat>(eye[2]),
                              1.0f);
                              
#endif // GL_ARB_vertex_program

#ifdef GL_ARB_fragment_program
   
   glEnable(GL_FRAGMENT_PROGRAM_ARB);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fprog_name);

   bool draw_radial = CurvatureTexture::get_draw_radial_curv();
   bool draw_mean = CurvatureTexture::get_draw_mean_curv();
   bool draw_gaussian = CurvatureTexture::get_draw_gaussian_curv();
   
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
                              draw_radial   ? 1.0f : 0.0f,
                              draw_mean     ? 1.0f : 0.0f,
                              draw_gaussian ? 1.0f : 0.0f,
                              0.0f);
   
   int radial_filter
      = CurvatureTexture::get_radial_filter();
   
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1,
      radial_filter == CurvatureTexture::FILTER_RADIAL   ? -1.0f : 1.0f,
      radial_filter == CurvatureTexture::FILTER_MEAN     ? -1.0f : 1.0f,
      radial_filter == CurvatureTexture::FILTER_GAUSSIAN ? -1.0f : 1.0f,
      0.25f);
   
   int mean_filter
      = CurvatureTexture::get_mean_filter();
   
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2,
      mean_filter == CurvatureTexture::FILTER_RADIAL   ? -1.0f : 1.0f,
      mean_filter == CurvatureTexture::FILTER_MEAN     ? -1.0f : 1.0f,
      mean_filter == CurvatureTexture::FILTER_GAUSSIAN ? -1.0f : 1.0f,
      0.25f);
   
   int gaussian_filter
      = CurvatureTexture::get_gaussian_filter();
   
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 3,
      gaussian_filter == CurvatureTexture::FILTER_RADIAL   ? -1.0f : 1.0f,
      gaussian_filter == CurvatureTexture::FILTER_MEAN     ? -1.0f : 1.0f,
      gaussian_filter == CurvatureTexture::FILTER_GAUSSIAN ? -1.0f : 1.0f,
      0.25f);
   
#endif // GL_ARB_fragment_program
   
}

void
CurvatureARBvpARBfpMultiTextureMode::setup_for_drawing_inside_dl(const Patch *patch)
{
   
#ifdef GL_ARB_multitexture
   
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, line_texture_name);
   
   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, region_texture_name);
   
   glActiveTextureARB(GL_TEXTURE2_ARB);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, rcurv_texture_name);
   
   glActiveTextureARB(GL_TEXTURE0);
                 
#endif // GL_ARB_multitexture
      
#ifdef GL_ARB_vertex_program
      
   glEnable(GL_VERTEX_PROGRAM_ARB);
   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vprog_name);
   
   // Enabling various features:
   glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, 1.0, 0.0, 1.0, 0.0);
      
#endif // GL_ARB_vertex_program
      
#ifdef GL_ARB_fragment_program
      
   glEnable(GL_FRAGMENT_PROGRAM_ARB);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fprog_name);
      
#endif // GL_ARB_fragment_program
   
}

void
CurvatureARBvpARBfpMultiTextureMode::StripCB::faceCB(CBvert* v, CBface* f)
{
   
   using mlib::Wvec;
   
   // normal
   Wvec n;
   glNormal3dv(f->vert_normal(v,n).data());

   if (v->has_color())
      GL_COL(v->color(), alpha*v->alpha());
   
   Wvec pdir1 = v->pdir1();
   Wvec pdir2 = v->pdir2();

   // Send curvature data as vertex attributes:
   glVertexAttrib4dARB(6, pdir1[0], pdir1[1], pdir1[2], v->k1());
   glVertexAttrib4dARB(7, pdir2[0], pdir2[1], pdir2[2], v->k2());
   glVertexAttrib4dvARB(15, v->dcurv_tensor().dcurv);

   // vertex coords
   glVertex3dv(v->loc().data());
   
}

//----------------------------------------------------------------------------//

class CurvatureRenderingModeSelectionPolicy {
   
   public:
   
      inline static RenderingMode *SelectRenderingMode();
   
};

inline RenderingMode*
CurvatureRenderingModeSelectionPolicy::SelectRenderingMode()
{
   
   RenderingMode *mode = 0;
   
   if(GLExtensions::gl_arb_vertex_program_supported()){
      
      if(GLExtensions::gl_arb_fragment_program_supported()){
         
         if(GLExtensions::gl_arb_multitexture_supported()){
            
            mode = new CurvatureARBvpARBfpMultiTextureMode();
            
         }
         
      }
      
   }
   
   return mode;
   
}

typedef RenderingModeSingleton<CurvatureRenderingModeSelectionPolicy>
        CurvatureModeSingleton;

//----------------------------------------------------------------------------//

bool CurvatureTexture::draw_radial_curv = true;
bool CurvatureTexture::draw_mean_curv = true;
bool CurvatureTexture::draw_gaussian_curv = true;

CurvatureTexture::curvature_filter_t CurvatureTexture::radial_filter
   = CurvatureTexture::FILTER_NONE;
CurvatureTexture::curvature_filter_t CurvatureTexture::mean_filter
   = CurvatureTexture::FILTER_NONE;
CurvatureTexture::curvature_filter_t CurvatureTexture::gaussian_filter
   = CurvatureTexture::FILTER_NONE;

float CurvatureTexture::sugcontour_thresh = 0.0;

CurvatureTexture::CurvatureTexture(Patch* patch, StripCB* cb)
   : BasicTexture(patch, cb)
{
   
}

int
CurvatureTexture::draw(CVIEWptr& v)
{
   
   assert(cb() != 0);
   
   // XXX - This is a temporary hack to remove use of the CurvatureModeSingleton
   // from the CurvatureTexture constructor due to static initialization
   // ordering problems.
   if(!cb() || !dynamic_cast<CurvatureRenderingMode::CurvatureStripCB*>(cb()))
      set_cb(CurvatureModeSingleton::Instance().get_new_strip_cb());
   
   if (_ctrl)
      return _ctrl->draw(v);
   cb()->alpha = alpha();

   // XXX - dumb hack
   check_patch_texture_map();

   // set gl state (lighting, shade model)
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

   // this is a no-op unless needed:
   // (don't put it inside display list creation):
   _patch->apply_texture();     // GL_ENABLE_BIT

   // set color (affects GL_CURRENT_BIT):
   GL_COL(_patch->color(), alpha()); // GL_CURRENT_BIT
   
   CurvatureModeSingleton::Instance().setup_for_drawing_outside_dl(_patch);
      
   // Execute display list if it's valid:
   if (BasicTexture::dl_valid(v)) {
      
      BasicTexture::draw(v);
      
   } else { // Otherwise build a new display list:

      // Try to generate a display list:
      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);
      
      CurvatureModeSingleton::Instance().setup_for_drawing_inside_dl(_patch);
   
      // draw the triangle strips
      _patch->draw_tri_strips(_cb);
      
      CurvatureModeSingleton::Instance().after_drawing_inside_dl(_patch);
   
      // End the display list here:
      if(_dl.dl(v)){
         
         _dl.close_dl(v);
   
         // The display list is built; now execute it:
         BasicTexture::draw(v);
      }
   
   }
   
   CurvatureModeSingleton::Instance().after_drawing_outside_dl(_patch);

   // Restore gl state:
   glPopAttrib();

   return _patch->num_faces();

}

//----------------------------------------------------------------------------//

/* Utility Functions */

/*
static void
make_texture_row(GLfloat *row_data, int row_width, int line_width,
                 GLfloat left_val, GLfloat line_val, GLfloat right_val)
{
   
   int cur_row_pos = 0;
   
   if(row_width > line_width){
   
      for(; cur_row_pos < (row_width - line_width)/2; ++cur_row_pos){
         
         row_data[cur_row_pos] = left_val;
         
      }
      
      for(int i = 0; i < line_width; ++i, ++cur_row_pos){
         
         row_data[cur_row_pos] = line_val;
         
      }
      
   }
   
   for(; cur_row_pos < row_width; ++cur_row_pos){
      
      row_data[cur_row_pos] = right_val;
      
   }
   
}

static void
make_1D_texture(GLuint texture_name, int texture_size, int line_width,
                bool just_line)
{
   
   assert(texture_size > 0);
   assert(line_width > 0);
   
   GLfloat *pixels = new GLfloat[texture_size];
   
   GLfloat less_than_zero_value = just_line ? 0.0f : 0.75f;
   
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   
   glBindTexture(GL_TEXTURE_1D, texture_name);
   
   int miplevel = 0;
   
   while(texture_size){
      
      for(int i = 0; i < texture_size; ++i)
         pixels[i] = 0.0f;
   
      make_texture_row(pixels, texture_size, line_width, less_than_zero_value, 1.0, 0.0);
      
      glTexImage1D(GL_TEXTURE_1D, miplevel, GL_LUMINANCE, texture_size, 0,
                   GL_LUMINANCE, GL_FLOAT, pixels);
                   
      texture_size >>= 1;
      ++miplevel;
      
   }
   
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);   
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   
   delete [] pixels;
   
}
*/
static void
make_2D_texture(GLuint texture_name, int texture_size, int line_width,
                bool just_line)
{
   
   assert(texture_size > 0);
   assert(line_width > 0);
   
   GLfloat *pixels = new GLfloat[texture_size * texture_size];
   
//   GLfloat less_than_zero_value = just_line ? 0.0f : 0.75f;
   
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   
   glBindTexture(GL_TEXTURE_2D, texture_name);
   
//    int miplevel = 0;
//    
//    while(texture_size){
//       
//       int cur_pos  = 0;
//       
//       for(int row = 0; row < texture_size; ++row){
//          
//          GLfloat line_value = row > texture_size/2 ? 1.0f : 0.25f;
//    
//          make_texture_row(pixels + cur_pos, texture_size, line_width,
//                           less_than_zero_value, line_value, 0.0);
//          
//          cur_pos += texture_size;
//          
//       }
//       
//       glTexImage2D(GL_TEXTURE_2D, miplevel, GL_LUMINANCE, texture_size, texture_size,
//                    0, GL_LUMINANCE, GL_FLOAT, pixels);
//                    
//       texture_size >>= 1;
//       ++miplevel;
//       
//    }

   int miplevel = 0;
   GLfloat val;
   
   while (texture_size) {
      
      for (int i = 0; i < texture_size*texture_size; i++) {
      
         double x = (double) (i%texture_size) - 0.5 * texture_size + 0.5;
         double y = (double) (i/texture_size) - 0.5 * texture_size + 0.5;
         
         val = 0.0f;
         
         if (texture_size >= 4){
            
            if(!just_line && x < 0.0){
               
               val = 0.75f;
               
            }
         
            if (fabs(x) < line_width){
               
               if(y < 0.0){
            
                  val = 0.25f; //sqr(max(1.0 - y, 0.0));
               
               } else {
                  
                  val = 1.0f; // sqr(max(1.0f - y, 0.0));
            
               }
         
            }
            
         }
         
         pixels[i] = val;
      
      }
      
      glTexImage2D(GL_TEXTURE_2D, miplevel, GL_LUMINANCE, texture_size, texture_size,
                   0, GL_LUMINANCE, GL_FLOAT, pixels);
      
      texture_size >>= 1;
      ++miplevel;
      
   }
   
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   
   delete [] pixels;
   
}

GLuint
load_ARB_program(const char *header, GLenum target, const char *program_string)
{
   
   GLuint program_name = 0;
   
#if defined(GL_ARB_vertex_program) || defined(GL_ARB_fragment_program)

   bool success = false;
   bool native;
   
   glGenProgramsARB(1, &program_name);
   glBindProgramARB(target, program_name);
   glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB,
                      strlen(program_string), program_string);
   
   if(target == GL_VERTEX_PROGRAM_ARB){
   
      success = GLExtensions::gl_arb_vertex_program_loaded(header, native,
                          reinterpret_cast<const unsigned char *>(program_string));
   
   } else if(target == GL_FRAGMENT_PROGRAM_ARB) {
      
      success = GLExtensions::gl_arb_fragment_program_loaded(header, native,
                          reinterpret_cast<const unsigned char *>(program_string));
      
   }
   
   if(!success){
      
      glDeleteProgramsARB(1, &program_name);
      program_name = 0;
      
   }
   
#endif

   return program_name;
   
}
