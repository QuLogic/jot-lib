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
#ifndef APPEAR_H_IS_INCLUDED
#define APPEAR_H_IS_INCLUDED

/*!
 *  \file appear.H
 *  \brief Contains the definition of the APPEAR class.
 *
 *  \sa appear.C
 *
 */

#include "disp/color.H"
#include "geom/texture.H"
#include "mlib/points.H"
#include "net/data_item.H"
#include "net/stream.H"
#include "std/config.H"

/*!
 *  \brief Generic class representing appearance properties associated
 *  with a piece of surface.
 *  
 *  Properties include:
 *   - Ambient color
 *   - Diffuse color (refered to as just color)
 *   - Specular color
 *   - Shininess
 *   - Transparency
 *   - Texture map
 *   - Texture transform.
 *
 */
class APPEAR {
   
 protected:
   
   STDdstream &decode_local(STDdstream &ds);
   STDdstream &format_local(STDdstream &ds) const;

   COLOR          _ambient_color;      //!< Ambient color
   bool           _has_ambient_color;  //!< true if it has ambient color
   COLOR          _color;              //!< Diffuse color
   bool           _has_color;          //!< true if it has diffuse color
   COLOR          _specular_color;     //!< Specular color
   bool           _has_specular_color; //!< true if it has specular color
   double         _shininess;          //!< Specular shininess
   bool           _has_shininess;      //!< true if it has shininess
   double         _transp;             //!< transparency (AKA alpha, in OpenGL)
   bool           _has_transp;         //!< true if it has transparency
   TEXTUREptr     _texture;            //!< What the texture is
   bool           _has_texture;        //!< true if we are textured
   mlib::Wtransf  _tex_xform;          //!< Texture transformation
      
 public:
   
   //! \name Constructors
   //@{
   
   APPEAR() :
      _ambient_color(COLOR::white),
      _has_ambient_color(false),
      _color(COLOR::white),
      _has_color(false),
      _specular_color(COLOR::white),
      _has_specular_color(false),
      _shininess(Config::get_var_dbl("SHININESS",100.0)),
      _has_shininess(false),
      _transp(1.0),
      _has_transp(false),
      _has_texture(false) {}
      
   APPEAR(APPEAR *a) : 
      _ambient_color(a->_ambient_color),
      _has_ambient_color(a->_has_ambient_color),
      _color(a->_color),
      _has_color(a->_has_color),
      _specular_color(a->_specular_color),
      _has_specular_color(a->_has_specular_color),
      _shininess(a->_shininess),
      _has_shininess(a->_has_shininess),
      _transp(a->_transp),
      _has_transp(a->_has_transp),
      _texture(a->_texture),
      _has_texture(a->_has_texture),
      _tex_xform(a->_tex_xform) {}
      
   virtual ~APPEAR() {}
      
   //@}
      
   //! \name Property Accessors
   //@{
      
   virtual void set_ambient_color(CCOLOR &c)
      { _has_ambient_color = true; _ambient_color = c; }
   virtual void unset_ambient_color()
      { _has_ambient_color = false; }
   virtual bool has_ambient_color() const
      { return _has_ambient_color; }
   virtual CCOLOR &ambient_color() const
      { return _has_ambient_color ? _ambient_color : color(); }
      
   virtual void set_color(CCOLOR &c)
      { _has_color = true;_color = c; }
   virtual void unset_color()
      { _has_color = false; }
   virtual bool has_color() const
      { return _has_color; }
   virtual CCOLOR &color() const
      { return _has_color ? _color : COLOR::white; }
      
   virtual void set_specular_color(CCOLOR &c)
      { _has_specular_color = true;_specular_color = c; }
   virtual void unset_specular_color()
      { _has_specular_color = false; }
   virtual bool has_specular_color() const
      { return _has_specular_color; }
   virtual CCOLOR &specular_color() const
      { return _has_specular_color ? _specular_color : COLOR::white; }
      
   virtual void set_shininess(double s)
      { _has_shininess = true; _shininess = s; }
   virtual void unset_shininess()
      { _has_shininess = false; }
   virtual bool has_shininess() const
      { return _has_shininess; }
   virtual double shininess() const {
      return _has_shininess ? _shininess :
         Config::get_var_dbl("SHININESS",100.0);
   }
      
   virtual void set_transp(double t)
      { _has_transp = true; _transp = t; }
   virtual void unset_transp()
      { _has_transp = false; }
   virtual bool has_transp() const
      { return _has_transp; }
   virtual double transp() const
      { return _has_transp ? _transp : 1.0; }
      
   virtual void set_texture(CTEXTUREptr& t)
      {_texture = t; _has_texture = (!!t); }
   virtual void unset_texture()
      { _has_texture = false; }
   virtual bool has_texture() const
      { return _has_texture; }
   virtual CTEXTUREptr &texture() const
      { return _texture; }
         
   virtual mlib::CWtransf &tex_xform() const
      { return _tex_xform; }
   virtual void set_tex_xform(mlib::CWtransf &t)
      { _tex_xform = t; }
      
   //@}
      
   //! \name TAG Helper Functions
   //@{
      
   void get_transp (TAGformat &d)       { *d >>_transp; _has_transp = true; }
   void get_color  (TAGformat &d)       { *d >>_color;  _has_color  = true; }
   void get_texture(TAGformat &d);
   void put_color  (TAGformat &d) const { if (_has_color)  d.id() <<_color;}
   void put_transp (TAGformat &d) const { if (_has_transp) d.id()<<_transp;}
   void put_texture(TAGformat &d) const;
      
   //@}
   
};

class APPEAR_list;
typedef const APPEAR_list CAPPEAR_list;

/*!
 *  \brief An ARRAY of APPEAR pointers that conveniently lets you call
 *  selected methods of APPEAR on a whole bunch of APPEARs in just
 *  a single line of code!
 *
 */
class APPEAR_list : public ARRAY<APPEAR*> {
   
 public:
 
   APPEAR_list(int num=0) : ARRAY<APPEAR*>(num) {}
   APPEAR_list(APPEAR* a) : ARRAY<APPEAR*>(1)   { add(a); }
      
   //******** COLOR ********
   void set_color(CCOLOR &c) {
      for (int k=0; k<num(); k++)
         (*this)[k]->set_color(c);
   }
   void unset_color() {
      for (int k=0; k<num(); k++)
         (*this)[k]->unset_color();
   }
      
   //******** TEXTURE ********
   void set_texture(CTEXTUREptr& t) {
      for (int k=0; k<num(); k++)
         (*this)[k]->set_texture(t);
   }
   void unset_texture() {
      for (int k=0; k<num(); k++)
         (*this)[k]->unset_texture();
   }
      
   //******** TEXTURE TRANSFORM ********
   void set_tex_xform(mlib::CWtransf &t) {
      for (int k=0; k<num(); k++)
         (*this)[k]->set_tex_xform(t);
   }
      
   //******** TRANSPARENCY ********
   void set_transp(double t) {
      for (int k=0; k<num(); k++)
         (*this)[k]->set_transp(t);
   }
   void unset_transp() {
      for (int k=0; k<num(); k++)
         (*this)[k]->unset_transp();
   }
   bool has_transp() {
      // if any of them have transparency, return true
      for (int k=0; k<num(); k++)
         if ((*this)[k]->has_transp())
            return 1;
      return 0;
   }
};

#endif // APPEAR_H_IS_INCLUDED
