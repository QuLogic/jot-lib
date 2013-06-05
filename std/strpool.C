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
#include "support.H"

#include <cctype>
#include <vector>
#include <string>
#include <algorithm>

#ifndef WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

//
// Gets a list of files in a certain directory
//
vector<string> dir_list(const string &path)
{
   vector<string> list;
#ifdef WIN32
   WIN32_FIND_DATA file;
   HANDLE hFile;
   
   const string dot(".");
   const string dotdot("..");
   string current = path+"*";
   
   if ((hFile = FindFirstFile(current.c_str(), &file)) != INVALID_HANDLE_VALUE)
   {
      while (true)
      {
         string fname(file.cFileName);
         if ((fname != dot) && (fname != dotdot)) list.push_back(fname);
         if (!FindNextFile(hFile, &file)) break;
      }
      FindClose(hFile);
   }
#else
   DIR *dir = 0;
   struct dirent *direntry;
   if (!path.empty() && (dir = opendir(path.c_str())))
   {
      const string dot(".");
      const string dotdot("..");
      struct stat statbuf;
      while ((direntry = readdir(dir)))
      {
         string file(direntry->d_name);
         if (file != dot && file != dotdot)
         {
            string path_to_file = path + "/" + file;
            if (!stat(path_to_file.c_str(), &statbuf) && (statbuf.st_mode & S_IFMT) == S_IFREG)
            {
               list.push_back(file);
            }
         }
      }
      closedir(dir);
   }
#endif
   std::sort(list.begin(), list.end());
   return list;
}

