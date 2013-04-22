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

// Static constructors can be the spawn of satan.  We don't know when
// str_ptr::null will be referenced before it has been initialized, so
// we want to make sure that we protect against this by using
// null_str()

// useful string constants:
STR         *STR::null = 0;
Cstr_ptr str_ptr::null(str_ptr::null_str());

// A string pool is just a hash table
HASH *STR::strpool = 0;

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

str_list
tokenize(
   Cstr_ptr &string,
   char      delim
   )
{
   str_list tokens;
   if (string  == NULL_STR) return tokens;
   
   char *buff = new char[strlen(**string) + 1];
   strcpy(buff, **string);
   char delimstr[2];
   delimstr[0] = delim;
   delimstr[1] = '\0';
   char *item = strtok(buff, delimstr);
   if (item) {
      tokens += str_ptr(item);
      while ((item = strtok(0, delimstr))) {
         tokens += str_ptr(item);
      }
   }
   delete [] buff;
   return tokens;
}

str_ptr
str_ptr::to_upper() const
{
   if ((*this) == NULL_STR) return NULL_STR;
   char buff[1024];
   strcpy(buff, **p_);
   const int len = strlen(buff);
   for (int i = 0; i < len; i++) {
      buff[i] = toupper(buff[i]);
   }
   return str_ptr(buff);
}

str_ptr
str_ptr::to_lower() const
{
   if ((*this) == NULL_STR) return NULL_STR;
   char buff[1024];
   strcpy(buff, **p_);
   const int len = strlen(buff);
   for (int i = 0; i < len; i++) {
      buff[i] = tolower(buff[i]);
   }
   return str_ptr(buff);
}


// Does this string contain any of the strings in str_list s?
bool
str_ptr::contains(Cstr_list &s) const
{
   for (int i = 0; i < s.num(); i++) {
      if (contains(s[i])) return 1;
   }
   return 0;
}
