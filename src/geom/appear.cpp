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
 *  \file appear.C
 *  \brief Contains the implementation of the APPEAR class.
 *
 *  \sa appear.H
 *
 */

#include "appear.H"
#include "net/stream.H"
#include "net/net_types.H"
#include "geom/texturegl.H"
#include "std/config.H"

void
APPEAR::get_texture(TAGformat &d)
{
   _has_texture = 1;

   string texture_name;
   *d >> _tex_xform >> texture_name;
   _texture = nullptr;
   if (!texture_name.empty() && texture_name != "NO_TEXTURE") {
      // XXX - taken from CLI::get_file_relative
      if (texture_name[0] != '/' && texture_name[0] != '.')
         texture_name = Config::JOT_ROOT() + "/" + texture_name;
      _texture = make_shared<TEXTUREgl>(texture_name);
   }
}

void
APPEAR::put_texture(TAGformat &d) const
{
   if (!_has_texture)
      return;

   d.id() << _tex_xform;

   if (_texture) {
      string texture_name =_texture->file();
      // If texture name begins with JOT_ROOT, strip it off
      // XXX - does not work if filename is below JOT_ROOT but the path to
      // the file does not start with JOT_ROOT
      if (texture_name.find(Config::JOT_ROOT()) == 0) {
         cerr << texture_name << endl;
         const char *name = texture_name.substr(Config::JOT_ROOT().length()+1).c_str();
         // Strip extra slashes ("/")
         while (name && *name == '/' && *name != '\0') name++;
         texture_name = string(name);
      }
      *d << texture_name;
   } else *d << "NO_TEXTURE";

   d.end_id();
}
