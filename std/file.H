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
#ifndef FILE_H_IN_DA_HAUS
#define FILE_H_IN_DA_HAUS

#include "support.H" 

bool     rename_(Cstr_ptr &, Cstr_ptr &);
bool     remove_(Cstr_ptr &);
bool     rmdir_(Cstr_ptr &);
bool     mkdir_(Cstr_ptr &);
bool     chdir_(Cstr_ptr &);
str_ptr  getcwd_();

#endif //FILE_H_IN_DA_HAUS
