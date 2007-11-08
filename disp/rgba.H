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
 * rgba.H
 *
 *      RGBA conversions.
 *
 *      Convenience methods to extract components of an RGBA
 *      color value stored as a 4-byte unsigned int; or to return
 *      the luminance of the color.
 *
 *      The R, G, and B components store red, green, and blue,
 *      respectively. The A component stores alpha
 *      (transparency). All components are 8-bit unsigned
 *      values. Alpha ranges from 0 (see-thru) to 255 (opaque).
 *****************************************************************/
#ifndef RGBA_H_IS_INCLUDED
#define RGBA_H_IS_INCLUDED

typedef unsigned int  uint;
typedef unsigned char uchar;

#if defined(WIN32) || defined(i386)
/****************************************
 * Intel platform: colors stored as ABGR
 ****************************************/
inline uint rgba_to_r(uint c) { return       c & 0xff; }
inline uint rgba_to_g(uint c) { return (c>> 8) & 0xff; }
inline uint rgba_to_b(uint c) { return (c>>16) & 0xff; }
inline uint rgba_to_a(uint c) { return (c>>24);        }
inline uint build_rgba(uchar r, uchar g, uchar b, uchar a=255U) {
   return ((a<<24) | (b<<16) | (g<<8) | r);
}

inline uint
grey_to_rgba(uint c)
{
   // treat c as an 8-bit value in range [0,255]. this is
   // enforced by masking off any bits outside the lowest 8
   return (0x10101*(c & 0xff) + 0xff000000);
}

#else
/****************************************
 * Non-Intel platform: colors stored RGBA
 ****************************************/
inline uint rgba_to_r(uint c) { return (c>>24);        }
inline uint rgba_to_g(uint c) { return (c>>16) & 0xff; }
inline uint rgba_to_b(uint c) { return (c>> 8) & 0xff; }
inline uint rgba_to_a(uint c) { return       c & 0xff; }
inline uint build_rgba(uchar r, uchar g, uchar b, uchar a=255U) {
   return ((r<<24) | (g<<16) | (b<<8) | a);
}

inline uint
grey_to_rgba(uint c)
{
   // treat c as an 8-bit value in range [0,255]. this is
   // enforced by masking off any bits outside the lowest 8
   return (0x1010100*(c & 0xff) + 255);
}

#endif

/****************************************
 * Both platforms
 ****************************************/
inline uint 
rgba_to_grey(uint c) 
{
   // return RGB luminance as a uint in range [0,255]
   return uint(.30*rgba_to_r(c) + .59*rgba_to_g(c) + .11*rgba_to_b(c));
}

// return componenents as doubles in range [0,1]:
inline double rgba_to_r_d(uint c) { return rgba_to_r(c)/255.0; }
inline double rgba_to_g_d(uint c) { return rgba_to_g(c)/255.0; }
inline double rgba_to_b_d(uint c) { return rgba_to_b(c)/255.0; }
inline double rgba_to_a_d(uint c) { return rgba_to_a(c)/255.0; }

inline double 
rgba_to_grey_d(uint c) 
{
   // return RGB luminance as a double in range [0,1]
   return (.30*rgba_to_r(c) + .59*rgba_to_g(c) + .11*rgba_to_b(c))/255.0;
}

inline uint
rgba_to_tone(uint c)
{
   // the same as rgba_to_grey() except we take into account the
   // transparency. return value is a uint in range [0,255]:
   return uint(rgba_to_a_d(c)*rgba_to_grey(c));
}

inline double
rgba_to_tone_d(uint c)
{
   // same as rgba_to_tone() except return value is a double in
   // range [0,1]:
   return rgba_to_grey_d(c) * rgba_to_a_d(c);
}

#endif // RGBA_H_IS_INCLUDED

/* end of file rgba.H */





