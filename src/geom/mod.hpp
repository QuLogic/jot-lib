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
#ifndef MOD_H_IS_INCLUDED
#define MOD_H_IS_INCLUDED

/*****************************************************************
 *  MOD:
 *
 *      A sequence number to determine if data in a
 *      dependency graph is up-to-date.
 *
 *****************************************************************/
#define CMOD const MOD
class MOD {
   static int   _TICK;
   static int   _START;
   int          _id;

 public:
   MOD()                                { _id = _TICK; }
   MOD(CMOD &id)                        { _id = id._id; }

   static void tick()                   { _START = ++_TICK; }
   bool   current()             const   { return _id >= _START; }
   int    val()                 const   { return _id; }
   MOD&   operator ++()                 { _id = ++_TICK; return *this; }
   int    operator ==(CMOD &id) const   { return _id == id._id; }
};

#endif // MOD_H_IS_INCLUDED

// end of file mod.H
