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
 *  \file line_drawing.C
 *  \brief Contains the implementation of the classes that implement the
 *  "Line Drawing" rendering style gTexture.
 *
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <cstring>
#include <cassert>

using namespace std;

#include "gtex/gl_extensions.H"
#include "gtex/rendering_mode.H"
#include "gtex/basic_texture.H"
#include "gtex/solid_color.H"
#include "gtex/line_drawing.H"

#include "mlib/points.H"

using namespace mlib;

/* Vertex and Fragment Shaders */

const char *LINE_DRAWING_VERTEX_PROGRAM_STR =
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
# Temporary register (used during various computations)\n\
TEMP   temp;\n\
\n\
#############################################\n\
## Transform vertex position\n\
#############################################\n\
\n\
" "\
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
ADD temp.y, temp.y, temp.z;      # 3*u*S + v*T\n\
MUL temp.y, temp.y, spdvals.y;   # 3*u*v^2*S + v^3*T\n\
\n\
ADD temp.x, temp.x, temp.y;      # u^3*P + 3*u^2*v*Q + 3*u*v^2*S + v^3*T\n\
\n\
# Then add offset to deriv of normal curvature:\n\
#      float csc2theta = 1.0f / (u2 + v2);\n\
#      sctest_num[i] *= csc2theta;\n\
" "\
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
MAD finalsf.z, finalsf.z, PARAM_DRAW_ENABLE.x, PARAM_DRAW_ENABLE.y;\n\
MAD finalsf.x, finalsf.x, PARAM_DRAW_ENABLE.z, PARAM_DRAW_ENABLE.w;\n\
\n\
MOV tc0, finalsf;\n\
\n\
END\n\
";

const char *LINE_DRAWING_FRAGMENT_PROGRAM_NO_SSD_STR =
"\
!!ARBfp1.0 \n\
\n\
ATTRIB tex0 = fragment.texcoord[0];\n\
ATTRIB col0 = fragment.color; \n\
ATTRIB ipos = fragment.position;\n\
\n\
OUTPUT out = result.color;\n\
\n\
PARAM PARAM_DRAW_COLOR = program.env[0];\n\
\n\
TEMP temp, temp2, temp3;   #temporary\n\
\n\
TEMP c_color, sc_color, final_color;\n\
\n\
\n\
PARAM one = {0.0,1.0,-1,1};\n\
PARAM half = {0.5, 1.0, 1.0, 1.0};\n\
PARAM big = {1.0, 0, 0, 0};\n\
PARAM blue = {0.0, 0.0, 1.0, 0.0};\n\
PARAM green = {0.0, 1.0, 0.0, 0.0};\n\
\n\
#contours\n\
MOV temp2.x, tex0.z;\n\
ADD temp2.x, temp2.x, half.x;\n\
MOV temp2.y, one.y;\n\
TEX c_color, temp2, texture[0], 2D;\n\
\n\
CMP temp, PARAM_DRAW_COLOR.xxxx, green, {0.0, 0.0, 0.0, 0.0};\n\
ADD_SAT c_color, c_color, temp;\n\
\n\
#SC\n\
MOV temp.x, tex0.x;\n\
ADD temp.x, temp.x, half.x;\n\
MOV temp.y, one.y;\n\
TEX sc_color, temp, texture[1], 2D;\n\
\n\
CMP temp, PARAM_DRAW_COLOR.xxxx, blue, {0.0, 0.0, 0.0, 0.0};\n\
ADD_SAT sc_color, sc_color, temp;\n\
\n\
# RCP temp.w, tex0.y;\n\
# POW temp.w, tex0.y, big.x;\n\
\n\
# MUL temp.w, one.z, temp.w;\n\
# CMP temp.y, tex0.y, temp.w, one.y;\n\
\n\
SUB sc_color, one.yyyy, sc_color;\n\
MUL sc_color, sc_color, tex0.yyyy;\n\
SUB sc_color, one.yyyy, sc_color;\n\
\n\
MUL final_color, sc_color, c_color;\n\
\n\
# fix alpha (may not be necessary)\n\
#PARAM testcnst = {-0.1, 0.1, 0, 0};\n\
#RCP temp.x, tex0.x;\n\
#MUL temp.y, temp.x, testcnst.x;\n\
#MOV temp.zw, testcnst.w;\n\
\n\
#test toon shader\n\
#MOV temp2, tex1;\n\
#MOV temp2.y, ipos.z;\n\
#TEX temp2, temp2, texture[2], 2D;\n\
#ADD temp2, temp2, program.env[1];\n\
#MIN temp2, temp2, {1,1,1,1};\n\
\n\
#MUL temp, temp2, temp;\n\
\n\
MOV out, final_color;\n\
#MOV out.xyz, tex0.z;\n\
\n\
END\n\
";

//----------------------------------------------------------------------------//

const char *LINE_DRAWING_VERTEX_SHADER_STR =
"\
/*\n\
 *  Line Drawing Vertex Shader\n\
 *\n\
 */\n\
\n\
#version 110\n\
\n\
uniform bool contours_on;    // Enable contour drawing\n\
uniform bool sugcontours_on; // Enable suggestive contour drawing\n\
uniform float scthresh;      // Suggestive contour threshold\n\
uniform float feature_size;  // Mesh feature size\n\
\n\
uniform vec3 eye_pos;        // Eye position in world coordinates\n\
\n\
attribute vec3 pdir1, pdir2; // Principal directions (normalized)\n\
attribute float k1, k2;      // Principal curvatures\n\
attribute vec4 dcurv_tensor; // Derivative of curvature tensor\n\
\n\
varying float rcurv;         // Radial curvature\n\
varying float drcurv;        // Derivative of radial curvature\n\
varying float ndotv;         // Normal vector dotted with view vector\n\
\n\
const float FS = 32.0;         // Feature size constant from RTSC (may need tweaking)\n\
\n\
void\n\
main()\n\
{\n\
   \n\
   gl_Position = ftransform();\n\
   \n\
   // Compute view vector:\n\
   vec3 view_vec = normalize(eye_pos - vec3(gl_Vertex));\n\
   \n\
   // Compute N dot V:\n\
   ndotv = dot(gl_Normal, view_vec);\n\
   \n\
   float u = dot(pdir1, view_vec); // cos(angle between pdir1 and view_vec)\n\
   float u2 = u*u;\n\
   float v = dot(pdir2, view_vec); // cos(angle between pdir2 and view_vec)\n\
   float v2 = v*v;\n\
   \n\
   // Compute radial curvature:\n\
   rcurv = k1*u2 + k2*v2;\n\
   \n\
   // Compute derivative of radial curvature:\n\
   float csc2theta = 1.0/(u2 + v2);\n\
   \n\
   drcurv = (u*u2*dcurv_tensor.x\n\
          + 3.0*u2*v*dcurv_tensor.y\n\
          + 3.0*v2*u*dcurv_tensor.z\n\
          + v*v2*dcurv_tensor.w)\n\
          * csc2theta;\n\
   \n\
   float tr = (k2 - k1)*u*v*csc2theta;\n\
   \n\
   drcurv -= 2.0*ndotv*tr*tr;\n\
   \n\
   float feature_size2 = feature_size*feature_size;\n\
   \n\
   drcurv -= ndotv*scthresh/feature_size2;\n\
   \n\
   drcurv *= feature_size2/ndotv;\n\
   \n\
   drcurv *= FS;\n\
   \n\
   // Enable/disable features based on uniform values:\n\
   ndotv = contours_on ? ndotv : 1.0;\n\
   rcurv = sugcontours_on ? rcurv : -1.0;\n\
   \n\
}\n\
";

const char *LINE_DRAWING_FRAGMENT_SHADER_STR =
"\
/*\n\
 *  Line Drawing Fragment Shader\n\
 *\n\
 */\n\
\n\
#version 110\n\
\n\
uniform float width;         // Line width\n\
\n\
varying float rcurv;         // Radial curvature\n\
varying float drcurv;        // Derivative of radial curvature\n\
varying float ndotv;         // Normal vector dotted with view vector\n\
\n\
void\n\
main()\n\
{\n\
   \n\
   vec2 values = vec2(rcurv, ndotv);\n\
   \n\
   vec2 ddx = dFdx(values);\n\
   vec2 ddy = dFdy(values);\n\
   vec2 grad_len = sqrt(ddx*ddx + ddy*ddy);\n\
   grad_len = grad_len*width + 1.0e-6;\n\
   vec2 dst_to_zero = abs(values)/grad_len;\n\
   \n\
   vec2 gaussian = exp(-(dst_to_zero*dst_to_zero));\n\
   \n\
   \n\
   float fade_out = max(((1.0/(0.008*abs(drcurv))) - 0.4), 0.0);\n\
   \n\
   gl_FragColor.r = 1.0;\n\
   gl_FragColor.b = 1.0 - gaussian[0] * max(fade_out, drcurv >= 0.0 ? 1.0 : 0.0);\n\
   gl_FragColor.g = 1.0 - gaussian[1];\n\
   gl_FragColor.a = 1.0;\n\
   \n\
}\n\
";

//----------------------------------------------------------------------------//

/* Utility Function Prototypes */

static GLuint load_ARB_program(const char *header, GLenum target,
                               const char *program_string);

#ifdef GL_ARB_shader_objects

GLhandleARB load_ARB_shader(const char *header, GLenum shader_type,
                            const char *shader_string);

bool link_ARB_shader(const char *header, GLhandleARB prog_handle);

void print_ARB_infolog(GLhandleARB object);

#endif // GL_ARB_shader_objects

void maketexture(double width, double fadeout=0.0);

//----------------------------------------------------------------------------//

/* Line Drawing Rendering Style Implementation */

/*!
 *  \brief The abstract interface for a "Line Drawing" sytle rendering mode.
 *
 */
class LineDrawingRenderingMode : public RenderingMode {
   
   public:
   
      class LineDrawingStripCB : public RenderingModeStripCB {
         
      };
      
      virtual ~LineDrawingRenderingMode() { }
   
};

//----------------------------------------------------------------------------//

/*!
 *  \brief A rendering mode for the "Line Drawing" rendering style that uses
 *  OpenGL ARB vertex and fragment programs without screen space derivatives
 *  (multitexturing is used instead).
 *
 */
class LineDrawingVprogFprogNoSSDMode : public LineDrawingRenderingMode {
   
   public:
   
      /*!
       *  \brief Handles callbacks for drawing triangle strips for the "Line Drawing"
       *  rendering style.
       *
       */
      class StripCB : public LineDrawingRenderingMode::LineDrawingStripCB {
         
         public:
         
            //! \brief "face" callback.
            virtual void faceCB(CBvert* v, CBface* f);
      
      };
   
      LineDrawingVprogFprogNoSSDMode();
      
      ~LineDrawingVprogFprogNoSSDMode();
   
      virtual void setup_for_drawing_outside_dl(const Patch *patch);
      virtual void setup_for_drawing_inside_dl(const Patch *patch);
      
      virtual GLStripCB *get_new_strip_cb() const
         { return new StripCB(); }
   
   private:
   
      LineDrawingVprogFprogNoSSDMode(const LineDrawingVprogFprogNoSSDMode &other);
      LineDrawingVprogFprogNoSSDMode &operator=(const LineDrawingVprogFprogNoSSDMode &rhs);
   
      GLuint vprog_name, fprog_name, c_texture_name, sc_texture_name;
   
};

LineDrawingVprogFprogNoSSDMode::LineDrawingVprogFprogNoSSDMode()
   : vprog_name(0), fprog_name(0), c_texture_name(0), sc_texture_name(0)
{
   
   // Vertex Program Setup:
   
   if(GLExtensions::gl_arb_vertex_program_supported()){
      
      err_mesg(ERR_LEV_INFO, "LineDrawingTexture::init() - Can use ARB vertex programs!");
      
#ifdef GL_ARB_vertex_program

      vprog_name = load_ARB_program("LineDrawingTexture::init() - ",
                                    GL_VERTEX_PROGRAM_ARB,
                                    LINE_DRAWING_VERTEX_PROGRAM_STR);
      
#endif // GL_ARB_vertex_program
      
   }
   
   // Fragment Program Setup:
   
   if(GLExtensions::gl_arb_fragment_program_supported()){
      
      err_mesg(ERR_LEV_INFO, "LineDrawingTexture::init() - Can use ARB fragment programs!");
      
#ifdef GL_ARB_fragment_program

      fprog_name = load_ARB_program("LineDrawingTexture::init() - ",
                                    GL_FRAGMENT_PROGRAM_ARB,
                                    LINE_DRAWING_FRAGMENT_PROGRAM_NO_SSD_STR);
      
#endif // GL_ARB_fragment_program
      
   }
   
   if(GLExtensions::gl_arb_multitexture_supported()){
   
      glGenTextures(1, &c_texture_name);
      glBindTexture(GL_TEXTURE_2D, c_texture_name);
      maketexture(2.0);
      
      glGenTextures(1, &sc_texture_name);
      glBindTexture(GL_TEXTURE_2D, sc_texture_name);
      maketexture(1.0);
      
   }
   
}

LineDrawingVprogFprogNoSSDMode::~LineDrawingVprogFprogNoSSDMode()
{
   
#if defined(GL_ARB_vertex_program) || defined(GL_ARB_fragment_program)

   if(vprog_name)
      glDeleteProgramsARB(1, &vprog_name);
   
   if(fprog_name)
      glDeleteProgramsARB(1, &fprog_name);
      
#endif

   if(c_texture_name)
      glDeleteTextures(1, &c_texture_name);
   
   if(sc_texture_name)
      glDeleteTextures(1, &sc_texture_name);
   
}

void
LineDrawingVprogFprogNoSSDMode::setup_for_drawing_outside_dl(const Patch *patch)
{
   
#ifdef GL_ARB_vertex_program
   
   glEnable(GL_VERTEX_PROGRAM_ARB);
   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vprog_name);
   
   bool draw_contours =
      LineDrawingTexture::get_draw_contours();
   bool draw_sugcontours =
      LineDrawingTexture::get_draw_sugcontours();
   
   // Enabling various features:
   glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0,
                              draw_contours    ? 1.0f : 0.0f,
                              draw_contours    ? 0.0f : 1.0f,
                              draw_sugcontours ? 1.0f : 0.0f,
                              draw_sugcontours ? 0.0f : 1.0f);
   
   GLfloat sc_thresh = LineDrawingTexture::get_sugcontour_thresh();
   
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
   
   bool draw_color = LineDrawingTexture::get_draw_in_color();
   
   // Enabling drawing in color of black and white:
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
                              draw_color ? -1.0f : 1.0f,
                              draw_color ? -1.0f : 1.0f,
                              draw_color ? -1.0f : 1.0f,
                              draw_color ? -1.0f : 1.0f);
   
#endif // GL_ARB_fragment_program
   
}

void
LineDrawingVprogFprogNoSSDMode::setup_for_drawing_inside_dl(const Patch *patch)
{
   
#ifdef GL_ARB_multitexture
   
   glActiveTextureARB(GL_TEXTURE0);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, c_texture_name);
   
   glActiveTextureARB(GL_TEXTURE1);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, sc_texture_name);
   
   glActiveTextureARB(GL_TEXTURE0);
                 
#endif // GL_ARB_multitexture
      
// #ifdef GL_ARB_vertex_program
//       
//    glEnable(GL_VERTEX_PROGRAM_ARB);
//    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vprog_name);
//       
// #endif // GL_ARB_vertex_program
//       
// #ifdef GL_ARB_fragment_program
//       
//    glEnable(GL_FRAGMENT_PROGRAM_ARB);
//    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fprog_name);
//       
// #endif // GL_ARB_fragment_program
   
}

void
LineDrawingVprogFprogNoSSDMode::StripCB::faceCB(CBvert* v, CBface* f)
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

/*!
 *  \brief A rendering mode for the "Line Drawing" rendering style that uses the
 *  OpenGL shading language.
 *
 */
class LineDrawingGLSLMode : public LineDrawingRenderingMode {
   
   public:
   
      /*!
       *  \brief Handles callbacks for drawing triangle strips for the "Line Drawing"
       *  rendering style.
       *
       */
      class StripCB : public LineDrawingRenderingMode::LineDrawingStripCB {
         
         public:
         
            StripCB(const LineDrawingGLSLMode *mode_in)
               : LineDrawingRenderingMode::LineDrawingStripCB(), mode(mode_in) { }
         
            //! \brief "face" callback.
            virtual void faceCB(CBvert* v, CBface* f);
            
         private:
         
            const LineDrawingGLSLMode *mode;
      
      };
   
      LineDrawingGLSLMode();
      
      ~LineDrawingGLSLMode();
   
      virtual void setup_for_drawing_outside_dl(const Patch *patch);
      virtual void setup_for_drawing_inside_dl(const Patch *patch);
      
      virtual void after_drawing_outside_dl(const Patch *patch);
      
      virtual GLStripCB *get_new_strip_cb() const
         { return new StripCB(this); }
   
   private:
   
      LineDrawingGLSLMode(const LineDrawingGLSLMode &other);
      LineDrawingGLSLMode &operator=(const LineDrawingGLSLMode &rhs);
      
      GLint contours_on_uniform_loc;
      GLint sugcontours_on_uniform_loc;
      GLint scthresh_uniform_loc;
      GLint feature_size_uniform_loc;
      GLint eye_pos_uniform_loc;
      GLint width_uniform_loc;
      
      GLint pdir1_attrib_loc, pdir2_attrib_loc;
      GLint k1_attrib_loc, k2_attrib_loc;
      GLint dcurv_tensor_attrib_loc;
      
#ifdef GL_ARB_shader_objects
   
      GLhandleARB vshader_handle, fshader_handle, prog_handle;
      
#endif // GL_ARB_shader_objects
   
};

LineDrawingGLSLMode::LineDrawingGLSLMode()
   : contours_on_uniform_loc(-1), sugcontours_on_uniform_loc(-1),
     scthresh_uniform_loc(-1), feature_size_uniform_loc(-1),
     eye_pos_uniform_loc(-1), width_uniform_loc(-1),
   
#ifdef GL_ARB_shader_objects

     vshader_handle(0), fshader_handle(0), prog_handle(0)
   
#endif // GL_ARB_shader_objects

{
   
   // Vertex Shader Setup:
   
   if(GLExtensions::gl_arb_vertex_shader_supported()){
      
      err_mesg(ERR_LEV_INFO, "LineDrawingTexture::init() - Can use ARB vertex shaders!");
      
#if defined(GL_ARB_shader_objects) && defined(GL_ARB_vertex_shader) 

      vshader_handle = load_ARB_shader("LineDrawingTexture::init() - ",
                                       GL_VERTEX_SHADER_ARB,
                                       LINE_DRAWING_VERTEX_SHADER_STR);
      
#endif // defined(GL_ARB_shader_objects) && defined(GL_ARB_vertex_shader) 
      
   }
   
   // Fragment Shader Setup:
   
   if(GLExtensions::gl_arb_fragment_shader_supported()){
      
      err_mesg(ERR_LEV_INFO, "LineDrawingTexture::init() - Can use ARB fragment shaders!");
      
#if defined(GL_ARB_shader_objects) && defined(GL_ARB_fragment_shader) 

      fshader_handle = load_ARB_shader("LineDrawingTexture::init() - ",
                                       GL_FRAGMENT_SHADER_ARB,
                                       LINE_DRAWING_FRAGMENT_SHADER_STR);
      
#endif // defined(GL_ARB_shader_objects) && defined(GL_ARB_fragment_shader) 
      
   }
   
   // Linking Shaders:
   
   if(GLExtensions::gl_arb_shader_objects_supported()){
      
#ifdef GL_ARB_shader_objects

      prog_handle = glCreateProgramObjectARB();
      
      glAttachObjectARB(prog_handle, vshader_handle);
      glAttachObjectARB(prog_handle, fshader_handle);
      
      if(!link_ARB_shader("LineDrawingTexture::init() - ", prog_handle)){
         
         glDeleteObjectARB(prog_handle);
         glDeleteObjectARB(vshader_handle);
         glDeleteObjectARB(fshader_handle);
         
         prog_handle = vshader_handle = fshader_handle = 0;
         
      } else {
         
         glUseProgramObjectARB(prog_handle);
         
         // Retrieving uniform variable locations:
         
         contours_on_uniform_loc = glGetUniformLocationARB(prog_handle, "contours_on");
         sugcontours_on_uniform_loc = glGetUniformLocationARB(prog_handle, "sugcontours_on");
         scthresh_uniform_loc = glGetUniformLocationARB(prog_handle, "scthresh");
         feature_size_uniform_loc = glGetUniformLocationARB(prog_handle, "feature_size");
         eye_pos_uniform_loc = glGetUniformLocationARB(prog_handle, "eye_pos");
         width_uniform_loc = glGetUniformLocationARB(prog_handle, "width");
         
         // Retrieving attribute variable locations:
         
         pdir1_attrib_loc = glGetAttribLocationARB(prog_handle, "pdir1");
         pdir2_attrib_loc = glGetAttribLocationARB(prog_handle, "pdir2");
         k1_attrib_loc = glGetAttribLocationARB(prog_handle, "k1");
         k2_attrib_loc = glGetAttribLocationARB(prog_handle, "k2");
         dcurv_tensor_attrib_loc = glGetAttribLocationARB(prog_handle, "dcurv_tensor");
         
         glUseProgramObjectARB(0);
         
      }

#endif // GL_ARB_shader_objects
      
   }
   
}

LineDrawingGLSLMode::~LineDrawingGLSLMode()
{
   
#ifdef GL_ARB_shader_objects

   glDeleteObjectARB(prog_handle);
   glDeleteObjectARB(vshader_handle);
   glDeleteObjectARB(fshader_handle);
      
#endif // GL_ARB_shader_objects
   
}

void
LineDrawingGLSLMode::setup_for_drawing_outside_dl(const Patch *path)
{
   
#ifdef GL_ARB_shader_objects
   
   glUseProgramObjectARB(prog_handle);
   
   Wpt eye = VIEW::eye();
                              
   glUniform3fARB(eye_pos_uniform_loc,
                  static_cast<GLfloat>(eye[0]),
                  static_cast<GLfloat>(eye[1]),
                  static_cast<GLfloat>(eye[2]));
   
   // Setting various parameters:
   glUniform1fARB(scthresh_uniform_loc,
                  LineDrawingTexture::get_sugcontour_thresh());
   
   // Enabling various features:
   glUniform1iARB(contours_on_uniform_loc,
                  LineDrawingTexture::get_draw_contours());
   glUniform1iARB(sugcontours_on_uniform_loc,
                  LineDrawingTexture::get_draw_sugcontours());
                              
#endif // GL_ARB_shader_objects
   
}

void
LineDrawingGLSLMode::setup_for_drawing_inside_dl(const Patch *patch)
{
      
#ifdef GL_ARB_shader_objects
   
   glUseProgramObjectARB(prog_handle);
   
   // Setting various parameters:
   glUniform1fARB(feature_size_uniform_loc,
                  static_cast<GLfloat>(patch->mesh()->avg_len()));
   glUniform1fARB(width_uniform_loc, 1.0f);
      
#endif // GL_ARB_shader_objects
   
}

void
LineDrawingGLSLMode::after_drawing_outside_dl(const Patch *patch)
{
   
#ifdef GL_ARB_shader_objects
   
   // Turn off shaders so that fixed functionality and ARB assembly programs
   // will work:
   glUseProgramObjectARB(0);
   
#endif // GL_ARB_shader_objects
   
}

void
LineDrawingGLSLMode::StripCB::faceCB(CBvert* v, CBface* f)
{
   
   using mlib::Wvec;
   
   // normal
   Wvec n;
   glNormal3dv(f->vert_normal(v,n).data());

   if (v->has_color())
      GL_COL(v->color(), alpha*v->alpha());

   // Send curvature data as vertex attributes:
   
   Wvec pdir1 = v->pdir1();
   Wvec pdir2 = v->pdir2();
   
   glVertexAttrib3fARB(mode->pdir1_attrib_loc,
                       static_cast<GLfloat>(pdir1[0]),
                       static_cast<GLfloat>(pdir1[1]),
                       static_cast<GLfloat>(pdir1[2]));
   glVertexAttrib3fARB(mode->pdir2_attrib_loc,
                       static_cast<GLfloat>(pdir2[0]),
                       static_cast<GLfloat>(pdir2[1]),
                       static_cast<GLfloat>(pdir2[2]));
                       
   glVertexAttrib1fARB(mode->k1_attrib_loc, static_cast<GLfloat>(v->k1()));
   glVertexAttrib1fARB(mode->k2_attrib_loc, static_cast<GLfloat>(v->k2()));
   
   double *dcurv = &(v->dcurv_tensor().dcurv[0]);
   
   glVertexAttrib4fARB(mode->dcurv_tensor_attrib_loc,
                       static_cast<GLfloat>(dcurv[0]),
                       static_cast<GLfloat>(dcurv[1]),
                       static_cast<GLfloat>(dcurv[2]),
                       static_cast<GLfloat>(dcurv[3]));

   // vertex coords
   glVertex3dv(v->loc().data());
   
}

//----------------------------------------------------------------------------//

class LineDrawingRenderingModeSelectionPolicy {
   
   public:
   
      inline static RenderingMode *SelectRenderingMode();
   
};

inline RenderingMode*
LineDrawingRenderingModeSelectionPolicy::SelectRenderingMode()
{
   
   RenderingMode *mode = 0;
   
   if(GLExtensions::gl_arb_shader_objects_supported() &&
      GLExtensions::gl_arb_vertex_shader_supported() &&
      GLExtensions::gl_arb_fragment_shader_supported() &&
      GLExtensions::gl_nv_fragment_program_option_supported()){
      
      mode = new LineDrawingGLSLMode();
      
   } else if(GLExtensions::gl_arb_vertex_program_supported()){
      
      if(GLExtensions::gl_arb_fragment_program_supported()){
         
         if(GLExtensions::gl_arb_multitexture_supported()){
            
            mode = new LineDrawingVprogFprogNoSSDMode();
            
         }
         
      }
      
   }
   
   return mode;
   
}

typedef RenderingModeSingleton<LineDrawingRenderingModeSelectionPolicy>
        LineDrawingModeSingleton;

//----------------------------------------------------------------------------//

bool LineDrawingTexture::draw_in_color = false;
bool LineDrawingTexture::draw_contours = true;
bool LineDrawingTexture::draw_sugcontours = true;
float LineDrawingTexture::sugcontour_thresh = 0.0;

LineDrawingTexture::LineDrawingTexture(Patch* patch, StripCB* cb)
   : BasicTexture(patch, cb), solid_color_texture(0)
{
   
//    if(!LineDrawingModeSingleton::Instance().line_drawing_supported()){
//       
//       solid_color_texture = new SolidColorTexture(patch);
//       
//    }
   
}

LineDrawingTexture::~LineDrawingTexture()
{
   
   delete solid_color_texture;
   
}

int
LineDrawingTexture::draw(CVIEWptr& v)
{
   
   // Just draw a solid color if the "Line Drawing" style is not supported:
   if(!LineDrawingModeSingleton::Instance().supported()){
      
      // XXX - This is a temporary hack to remove use of the LineDrawingModeSingleton
      // from the LineDrawingTexture constructor due to static initialization
      // ordering problems.
      if(!solid_color_texture) solid_color_texture = new SolidColorTexture(_patch);
   
      return solid_color_texture->draw(v);
         
   }
   
   assert(cb() != 0);
   
   // XXX - This is a temporary hack to remove use of the LineDrawingModeSingleton
   // from the LineDrawingTexture constructor due to static initialization
   // ordering problems.
   if(!cb() || !dynamic_cast<LineDrawingRenderingMode::LineDrawingStripCB*>(cb()))
      set_cb(LineDrawingModeSingleton::Instance().get_new_strip_cb());
   
   if (_ctrl)
      return _ctrl->draw(v);
   cb()->alpha = alpha();

   // XXX - dumb hack
   check_patch_texture_map();

   // this is a no-op unless needed:
   // (don't put it inside display list creation):
   _patch->apply_texture();

   // set gl state (lighting, shade model)
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

   // set color (affects GL_CURRENT_BIT):
   GL_COL(_patch->color(), alpha()); // GL_CURRENT_BIT
   
   LineDrawingModeSingleton::Instance().setup_for_drawing_outside_dl(_patch);
      
   // Execute display list if it's valid:
   if (BasicTexture::dl_valid(v)) {
      
      BasicTexture::draw(v);
      
   } else { // Otherwise build a new display list:

      // Try to generate a display list:
      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);
      
      LineDrawingModeSingleton::Instance().setup_for_drawing_inside_dl(_patch);
   
      // draw the triangle strips
      _patch->draw_tri_strips(_cb);
      
      LineDrawingModeSingleton::Instance().after_drawing_inside_dl(_patch);
   
      // End the display list here:
      if(_dl.dl(v)){
         
         _dl.close_dl(v);
   
         // The display list is built; now execute it:
         BasicTexture::draw(v);
      }
   
   }
   
   LineDrawingModeSingleton::Instance().after_drawing_outside_dl(_patch);

   // Restore gl state:
   glPopAttrib();

   return _patch->num_faces();

}

//----------------------------------------------------------------------------//

/*!
 *  \brief Create a texture with a black line of the given width.
 *
 *  Call this after binding a valid texture object.  That texture object will
 *  have its image data replaced with a 1024x1024 pixel image containing a black
 *  line of width \p width.
 *
 *  This function automatically computes mipmaps and turns on trilinear filtering
 *  (i.e. minification filter GL_LINEAR_MIPMAP_LINEAR).
 *
 */
void
maketexture(double width, double fadeout)
{
   
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   
   int texsize = 1024;
   static unsigned char *texture = new unsigned char[texsize*texsize];
   
   int miplevel = 0;
   
   while (texsize) {
      
      for (int i = 0; i < texsize*texsize; i++) {
         
         double x = (double) (i%texsize) - 0.5 * texsize + 0.5;
         double y = (double) (i/texsize) - 0.5 * texsize + 0.5;
         double val = 1;
         
         if (texsize >= 4){
            
            if (fabs(x) < width && y > 0.0){
               
               val = sqr(max(1.0 - y, 0.0));
               val = (1.0*fadeout*miplevel + val)/(1.0 + fadeout*miplevel);
               
            }
            
         }
         
         texture[i] = min(max(int(256.0 * val), 0), 255);
         
      }

      /* test- write maketexture to file to edit
      char buf[256]; sprintf(buf, "mktex%d.ppm", texsize);
      FILE *f = fopen(buf, "wb");
      fprintf(f, "P6\n%d %d\n255\n", texsize, texsize);
      for(int i=0;i<texsize*texsize;i++){
        for(int j=0;j<3;j++) fwrite( &texture[i], 1, 1, f);
      }
      fclose(f);
       */

      glTexImage2D(GL_TEXTURE_2D, miplevel, GL_LUMINANCE, texsize, texsize, 0,
                   GL_LUMINANCE, GL_UNSIGNED_BYTE, texture);
              
      texsize >>= 1;
      ++miplevel;
      
   }

   float bgcolor[] = { 1, 1, 1, 1 };
   glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bgcolor);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

   //alexni- try this extension
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   
#ifdef GL_EXT_texture_filter_anisotropic
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
#endif

}

//----------------------------------------------------------------------------//

/* Utility Functions */

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

#ifdef GL_ARB_shader_objects

GLhandleARB
load_ARB_shader(const char *header, GLenum shader_type, const char *shader_string)
{
   
   GLhandleARB shader_handle = glCreateShaderObjectARB(shader_type);
   
   glShaderSourceARB(shader_handle, 1, &shader_string, 0);
   
   glCompileShaderARB(shader_handle);
   
   GLint compile_status;
   
   glGetObjectParameterivARB(shader_handle, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);
   
   if(!compile_status){
      
      cerr << header
           << " Error while compiling shader.  Printing info log..." << endl;
      
      print_ARB_infolog(shader_handle);
      
      glDeleteObjectARB(shader_handle);
      
      shader_handle = 0;
      
   }
   
   return shader_handle;
   
}

bool
link_ARB_shader(const char *header, GLhandleARB prog_handle)
{
   
   glLinkProgramARB(prog_handle);
   
   GLint link_status;
   
   glGetObjectParameterivARB(prog_handle, GL_OBJECT_LINK_STATUS_ARB, &link_status);
   
   if(!link_status){
      
      cerr << header
           << " Error while linking shader program.  Printing info log..." << endl;
     
     print_ARB_infolog(prog_handle);
      
   }
   
   return link_status != 0;
   
}

void
print_ARB_infolog(GLhandleARB object)
{
   
   GLint infolog_length;
   
   glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infolog_length);
   
   char *infolog = new char[infolog_length + 1];
   
   GLint actual_infolog_length;
   
   glGetInfoLogARB(object, infolog_length, &actual_infolog_length, infolog);
   
   cerr << infolog << endl;
   
   delete [] infolog;
   
}

#endif // GL_ARB_shader_objects
