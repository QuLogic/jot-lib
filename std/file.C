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

#include "file.H" 

//////////////////////////////////////////////////////
// rename_()
//////////////////////////////////////////////////////
bool rename_(Cstr_ptr &old_name, Cstr_ptr &new_name)
{   
   int ret;

#ifdef WIN32
   ret = rename(**old_name, **new_name);
#else
   //XXX - Untested
   ret = rename(**old_name, **new_name);
#endif
   
   if (ret != 0)
   {
      cerr << "rename_() - ERROR!! Couldn't rename: '" << old_name << "' to: '" << new_name << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// remove_()
//////////////////////////////////////////////////////
bool remove_(Cstr_ptr &file)
{   
   int ret;

#ifdef WIN32
   ret = remove(**file);
#else
   //XXX - Untested
   ret = remove(**file);
#endif
   
   if (ret != 0)
   {
      cerr << "remove_() - ERROR!! Couldn't remove: '" << file << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// rmdir_()
//////////////////////////////////////////////////////
bool rmdir_(Cstr_ptr &dir)
{   
   int ret;

#ifdef WIN32
   ret = _rmdir(**dir);
#else
   //XXX - Untested
   ret = rmdir(**dir);
#endif
   
   if (ret != 0)
   {
      cerr << "rmdir_() - ERROR!! Couldn't rmdir: '" << dir << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// mkdir_()
//////////////////////////////////////////////////////
bool mkdir_(Cstr_ptr &dir)
{   
   int ret;

#ifdef WIN32
   ret = _mkdir(**dir);
#else
   //XXX - Untested
   ret = mkdir(**dir, S_IRUSR  | S_IWUSR  | S_IXUSR |
                      S_IRGRP/*| S_IWGRP*/| S_IXGRP |
                      S_IROTH/*| S_IWOTH*/| S_IXOTH );
#endif
   
   if (ret != 0)
   {
      cerr << "mkdir_() - ERROR!! Couldn't mkdir: '" << dir << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// chdir_()
//////////////////////////////////////////////////////
bool chdir_(Cstr_ptr &new_dir)
{   
   int ret;

#ifdef WIN32
   ret = _chdir(**new_dir);
#else
   //XXX - Untested
   ret = chdir(**new_dir);
#endif
   
   if (ret != 0)
   {
      cerr << "chdir_() - ERROR!! Couldn't CHDIR to: '" << new_dir << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

#define CWD_BUF 1024
//////////////////////////////////////////////////////
// getcwd_()
//////////////////////////////////////////////////////
str_ptr
getcwd_()
{
   char *ret, cwd[CWD_BUF];

#ifdef WIN32
   ret = _getcwd(cwd,CWD_BUF);
#else
   //XXX - Untested
   ret = getcwd(cwd,CWD_BUF);
#endif
   
   if (!ret)
   {
      cerr << "getcwd_() - ERROR!! Couldn't retreive CWD!\n";
      return NULL_STR;
   }
   else
   {
      return cwd;
   }
}

