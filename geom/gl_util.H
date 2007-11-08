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
#ifndef GL_UTIL_H_IS_INCLUDED
#define GL_UTIL_H_IS_INCLUDED

/*!
 *  \file gl_util.H
 *  \brief Contains several OpenGL related utility functions.
 *
 */

#include <cassert>

#include "std/support.H"
#include "glew/glew.H"

#include "disp/color.H"
#include "mlib/points.H"

using namespace mlib;

/*!
 *   \brief Representation of an array of 4 floats. Convenient for
 *   passing COLOR + alpha, Wpt, or Wvec to OpenGL:
 *
 *   Examples:
 *
 *     // Set current color and alpha:
 *     COLOR  c = Color::orange4; 
 *     double a = 0.7; // alpha
 *     glColor4fv(float4(c, a));
 *
 *     // Pass a position or direction to define light coordinates,
 *     // where the homogeneous coordinate h is 1 or 0, respectively:
 *     Wpt p; Wvec v;
 *     if (positional_light)
 *        glLightfv(GL_LIGHT0, GL_POSITION, float4(p)); // h == 1
 *     else
 *        glLightfv(GL_LIGHT0, GL_POSITION, float4(v)); // h == 0
 *
 */
class float4 {
 public:
   //! \name Constructors
   //@{   
   float4(CCOLOR& c, double alpha = 1.0) {
      _data[0] = static_cast<GLfloat>(c[0]);
      _data[1] = static_cast<GLfloat>(c[1]);
      _data[2] = static_cast<GLfloat>(c[2]);
      _data[3] = static_cast<GLfloat>(alpha);
   }
   float4(CWpt& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
      _data[2] = static_cast<GLfloat>(p[2]);
      _data[3] = 1;
   }
   float4(CWvec& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
      _data[2] = static_cast<GLfloat>(v[2]);
      _data[3] = 0;
   }
   //@}

   //! \brief Define implicit cast from float4 to GLfloat*:
   operator GLfloat* () { return _data; }

 private:
   GLfloat  _data[4];
};

/*!
 *   \brief Representation of an array of 3 floats. 
 *   Convenient for passing a Wpt, Wvec, or COLOR to OpenGL.
 *
 *   Examples:
 *
 *     // Send a Wvec to a GLSL shader
 *     Wvec v;
 *     glUniform3fv(float3(v));
 *
 */
class float3 {
 public:
   //! \name Constructors
   //@{   
   float3(CWpt& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
      _data[2] = static_cast<GLfloat>(p[2]);
   }
   float3(CWvec& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
      _data[2] = static_cast<GLfloat>(v[2]);
   }
   float3(CNDCZpt& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
      _data[2] = static_cast<GLfloat>(p[2]);
   }
   float3(CNDCZvec& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
      _data[2] = static_cast<GLfloat>(v[2]);
   }
   float3(CCOLOR& c) {
      _data[0] = static_cast<GLfloat>(c[0]);
      _data[1] = static_cast<GLfloat>(c[1]);
      _data[2] = static_cast<GLfloat>(c[2]);
   }
   //@}

   //! \brief Define implicit cast from float3 to GLfloat*:
   operator GLfloat* () { return _data; }

 private:
   GLfloat  _data[3];
};

/*!
 *   \brief Representation of an array of 2 floats. 
 *   Convenient for passing a PIXEL or other 2D type to OpenGL.
 *
 *   Examples:
 *
 *     // Send a PIXEL to a GLSL shader
 *     PIXEL p;
 *     glUniform2fv(float2(p));
 *
 */
class float2 {
 public:
   //! \name Constructors
   //@{   
   float2(CXYpt& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
   }
   float2(CXYvec& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
   }
   float2(CNDCpt& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
   }
   float2(CNDCvec& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
   }
   float2(CPIXEL& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
   }
   float2(CVEXEL& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
   }
   float2(CUVpt& p) {
      _data[0] = static_cast<GLfloat>(p[0]);
      _data[1] = static_cast<GLfloat>(p[1]);
   }
   float2(CUVvec& v) {
      _data[0] = static_cast<GLfloat>(v[0]);
      _data[1] = static_cast<GLfloat>(v[1]);
   }
   //@}

   //! \brief Define implicit cast from float2 to GLfloat*:
   operator GLfloat* () { return _data; }

 private:
   GLfloat  _data[2];
};

/*!
 *   \brief Representation of an array of 16 floats.  Really only seems
 *   applicable for Wtransf, thus that is the only variety provided.
 */
class float16 {
 public:
   // constructor for Wtransf.  If you want a different constructor, write it.
   float16(CWtransf& w) {
      _data[0]  = static_cast<GLfloat>(w[0][0]);
      _data[1]  = static_cast<GLfloat>(w[0][1]);
      _data[2]  = static_cast<GLfloat>(w[0][2]);
      _data[3]  = static_cast<GLfloat>(w[0][3]);
      _data[4]  = static_cast<GLfloat>(w[1][0]);
      _data[5]  = static_cast<GLfloat>(w[1][1]);
      _data[6]  = static_cast<GLfloat>(w[1][2]);
      _data[7]  = static_cast<GLfloat>(w[1][3]);
      _data[8]  = static_cast<GLfloat>(w[2][0]);
      _data[9]  = static_cast<GLfloat>(w[2][1]);
      _data[10] = static_cast<GLfloat>(w[2][2]);
      _data[11] = static_cast<GLfloat>(w[2][3]);
      _data[12] = static_cast<GLfloat>(w[3][0]);
      _data[13] = static_cast<GLfloat>(w[3][1]);
      _data[14] = static_cast<GLfloat>(w[3][2]);
      _data[15] = static_cast<GLfloat>(w[3][3]);
   }

   //! \brief Define implicit cast from float16 to GLfloat*:
   operator GLfloat* () { return _data; }

 private:
   GLfloat _data[16];
};

//! \brief Convenience function. E.g.: light_i(0) returns GL_LIGHT0
inline GLenum
light_i(int i)
{
   switch(i) {
    case 0:  return GL_LIGHT0; break;  
    case 1:  return GL_LIGHT1; break;
    case 2:  return GL_LIGHT2; break;  
    case 3:  return GL_LIGHT3; break;
    case 4:  return GL_LIGHT4; break;
    case 5:  return GL_LIGHT5; break;
    case 6:  return GL_LIGHT6; break;
    case 7:  return GL_LIGHT7; break;
    default: assert(0);
   }
   return GL_LIGHT0; // for the compiler
}

//! \brief Send a color and alpha value to OpenGL.
inline void 
GL_COL(CCOLOR &c, double a) 
{
   glColor4fv(float4(c,a));
}

//! \brief Set OpenGL material color values given a COLOR and an alpha value.
inline void
GL_MAT_COLOR(GLenum face, GLenum pname, const COLOR &c, double a)
{
   // Make sure the face parameter is valid:
   assert((face == GL_FRONT) ||
          (face == GL_BACK)  ||
          (face == GL_FRONT_AND_BACK));
   
   // Make sure the pname parameter is valid (it has to be a color
   // channel value):
   assert((pname == GL_AMBIENT)  ||
          (pname == GL_DIFFUSE)  ||
          (pname == GL_SPECULAR) ||
          (pname == GL_EMISSION) ||
          (pname == GL_AMBIENT_AND_DIFFUSE));
   
   glMaterialfv(face, pname, float4(c,a));
}

inline int
major_gl_version()
{
   // Return major number of OpenGL version as an integer.
   //
   // E.g.: For OpenGL 2.1, this returns 2.
   //
   // During static initialization calls to glGetString() may fail; 
   // in that case this returns 0.
   const GLubyte* str = glGetString(GL_VERSION);
   return (str && str[0]) ? int(str[0] - '0') : 0;
}

inline int
minor_gl_version()
{
   // Return minor number of OpenGL version as an integer.
   //
   // E.g.: For OpenGL 1.5, this returns 5.
   //
   // During static initialization calls to glGetString() may fail; 
   // in that case this returns 0.
   const GLubyte* str = glGetString(GL_VERSION);
   return (str && str[0] && str[1] && str[2] ) ? int(str[2] - '0') : 0;
}

#endif // GL_UTIL_H_IS_INCLUDED

// end of file gl_util.H
