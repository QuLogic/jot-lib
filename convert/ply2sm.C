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

  Convert a PLY file to a jot .sm file.

  Modified from ply2iv.c in Greg Turk's PLY 1-1 code:
    ftp://graphics.stanford.edu/pub/zippack/ply-1.1.tar.Z

  -------------------------------------------------------

  Copyright (c) 1998 Georgia Institute of Technology.
  All rights reserved.

  Permission to use, copy, modify and distribute this software and its   
  documentation for any purpose is hereby granted without fee, provided   
  that the above copyright notice and this permission notice appear in   
  all copies of this software and that you do not sell the software.   

  THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,   
  EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY   
  WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.   

 *****************************************************************/

#include "mesh/lmesh.H"
#include "std/config.H"

#include "ply.H"

static bool debug = Config::get_var_bool("DEBUG_PLY2SM",false,true);

/* vertex and face definitions for a polygonal object */

typedef struct Vertex {
   float x,y,z;
   float r,g,b;
   float nx,ny,nz;
   void *other_props;       /* other properties */
} Vertex;

typedef struct Face {
   unsigned char nverts;    /* number of vertex indices in list */
   int *verts;              /* vertex index list */
   void *other_props;       /* other properties */
} Face;

char *elem_names[] = { /* list of the elements in the object */
   "vertex", "face"
};

PlyProperty vert_props[] = { /* list of property information for a vertex */
   {"x", Float32, Float32, offsetof(Vertex,x), 0, 0, 0, 0},
   {"y", Float32, Float32, offsetof(Vertex,y), 0, 0, 0, 0},
   {"z", Float32, Float32, offsetof(Vertex,z), 0, 0, 0, 0},
   {"r", Float32, Float32, offsetof(Vertex,r), 0, 0, 0, 0},
   {"g", Float32, Float32, offsetof(Vertex,g), 0, 0, 0, 0},
   {"b", Float32, Float32, offsetof(Vertex,b), 0, 0, 0, 0},
   {"nx", Float32, Float32, offsetof(Vertex,nx), 0, 0, 0, 0},
   {"ny", Float32, Float32, offsetof(Vertex,ny), 0, 0, 0, 0},
   {"nz", Float32, Float32, offsetof(Vertex,nz), 0, 0, 0, 0},
};

PlyProperty face_props[] = { /* list of property information for a face */
   {"vertex_indices", Int32, Int32, offsetof(Face,verts),
    1, Uint8, Uint8, offsetof(Face,nverts)},
};


/*** the PLY object ***/

static int nverts=0,nfaces=0;
static Vertex **vlist=0;
static Face **flist=0;

static PlyOtherProp *vert_other=0,*face_other=0;

static int per_vertex_color = 0;
static int has_normals = 0;


/******************************************************************************
Print out usage information.
******************************************************************************/
void
usage(char *progname)
{
   err_msg("usage: %s [flags] < in.ply > out.sm", progname);
}


/******************************************************************************
Read in the PLY file from standard in.
******************************************************************************/
void
read_file()
{
   /*** Read in the original PLY object ***/

   PlyFile *in_ply = read_ply (stdin);

   for (int i = 0; i < in_ply->num_elem_types; i++) {

      /* prepare to read the i'th list of elements */
      int elem_count=0;
      char *elem_name = setup_element_read_ply (in_ply, i, &elem_count);

      err_adv(debug, "%s: %d elements", elem_name, elem_count);

      if (equal_strings ("vertex", elem_name)) {

         /* create a vertex list to hold all the vertices */
         vlist = (Vertex **) malloc (sizeof (Vertex *) * elem_count);
         nverts = elem_count;

         /* set up for getting vertex elements */

         setup_property_ply (in_ply, &vert_props[0]);
         setup_property_ply (in_ply, &vert_props[1]);
         setup_property_ply (in_ply, &vert_props[2]);

         int j;
         for (j = 0; j < in_ply->elems[i]->nprops; j++) {
            PlyProperty *prop;
            prop = in_ply->elems[i]->props[j];
            if (equal_strings ("r", prop->name)) {
               setup_property_ply (in_ply, &vert_props[3]);
               per_vertex_color = 1;
            }
            if (equal_strings ("g", prop->name)) {
               setup_property_ply (in_ply, &vert_props[4]);
               per_vertex_color = 1;
            }
            if (equal_strings ("b", prop->name)) {
               setup_property_ply (in_ply, &vert_props[5]);
               per_vertex_color = 1;
            }
            if (equal_strings ("nx", prop->name)) {
               setup_property_ply (in_ply, &vert_props[6]);
               has_normals = 1;
            }
            if (equal_strings ("ny", prop->name)) {
               setup_property_ply (in_ply, &vert_props[7]);
               has_normals = 1;
            }
            if (equal_strings ("nz", prop->name)) {
               setup_property_ply (in_ply, &vert_props[8]);
               has_normals = 1;
            }
         }

         vert_other = get_other_properties_ply (in_ply, 
                                                offsetof(Vertex,other_props));

         /* grab all the vertex elements */
         for (j = 0; j < elem_count; j++) {
            vlist[j] = (Vertex *) malloc (sizeof (Vertex));
            vlist[j]->r = 1;
            vlist[j]->g = 1;
            vlist[j]->b = 1;
            get_element_ply (in_ply, (void *) vlist[j]);
         }
      }
      else if (equal_strings ("face", elem_name)) {

         /* create a list to hold all the face elements */
         flist = (Face **) malloc (sizeof (Face *) * elem_count);
         nfaces = elem_count;

         /* set up for getting face elements */

         setup_property_ply (in_ply, &face_props[0]);
         face_other = get_other_properties_ply (in_ply, 
                                                offsetof(Face,other_props));

         /* grab all the face elements */
         for (int j = 0; j < elem_count; j++) {
            flist[j] = (Face *) malloc (sizeof (Face));
            get_element_ply (in_ply, (void *) flist[j]);
         }
      }
      else
         get_other_element_ply (in_ply);
   }

   close_ply (in_ply);
   free_ply (in_ply);
}


inline void
add_face(LMESHptr& mesh, Face* f)
{
   assert(mesh && f);
   switch (f->nverts) {
    case 3:     // triangle
      mesh->add_face(f->verts[2], f->verts[1], f->verts[0]);
      break;
    case 4:     // quad
      mesh->add_quad(f->verts[3], f->verts[2], f->verts[1], f->verts[0]);
      break;
    default:    // other
      // XXX - should fix this to convert to triangles
      err_msg("ply2sm: can't add face: %d-gon", f->nverts);
   }
}

/******************************************************************************
Write out a jot .sm file.
******************************************************************************/
void
write_sm()
{
   LMESHptr mesh = new LMESH;
   
   int i=0;

   //******** Build the mesh ********

   err_adv(debug, "read ply file: %d vertices, %d faces\n", nverts, nfaces);

   err_adv(debug, "building mesh:");

   // Add vertices to mesh
   err_adv(debug, "  adding vertices...");
   for (i = 0; i < nverts; i++)
      mesh->add_vertex(Wpt(vlist[i]->x, vlist[i]->y, vlist[i]->z));
   err_adv(debug, "  done\n");

   // Add per-vertex colors if needed
   if (per_vertex_color) {
      err_adv(debug, "  adding colors...");
      for (i = 0; i < nverts; i++)
         mesh->bv(i)->set_color(COLOR(vlist[i]->r, vlist[i]->g, vlist[i]->b));
      err_adv(debug, "  done\n");
   }

   // Add faces
   err_adv(debug, "  adding faces...");
   for (i = 0; i < nfaces; i++)
      add_face(mesh, flist[i]);
   err_adv(debug, "  done\n");

   //******** Filter the mesh ********

   err_adv(debug, "filtering mesh...");

   // Remove any isolated vertices
   for (i=mesh->nverts()-1; i>=0; i--) {
      if (mesh->bv(i)->degree() == 0) {
         mesh->remove_vertex(mesh->bv(i));
      }
   }
   mesh->changed();

   // Remove duplicate vertices while we're at it
   mesh->remove_duplicate_vertices(false); // don't keep the bastards

   // Check for consistent orientation of normals
   bool is_bad = false;
   for (i=0; i<mesh->nedges(); i++)
      if (!mesh->be(i)->consistent_orientation())
         is_bad = true;
   if (is_bad)
      err_msg("Warning: inconsistently oriented triangles -- can't fix");

   // Optional: recenter mesh
   if (Config::get_var_bool("JOT_RECENTER"))
      mesh->recenter();

   // Optional: print stats
   if (Config::get_var_bool("JOT_PRINT_MESH"))
      mesh->print();

   err_adv(debug, "done\n");

   //******** Write mesh ********

   err_adv(debug, "writing mesh...");
   mesh->write_stream(cout);
   err_adv(debug, "done\n");
}

/******************************************************************************
Main program.
******************************************************************************/
int
main(int argc, char *argv[])
{
   char *s;
   char *progname;

   progname = argv[0];

   while (--argc > 0 && (*++argv)[0]=='-') {
      for (s = argv[0]+1; *s; s++)
         switch (*s) {
          default:
            usage (progname);
            exit (-1);
            break;
         }
   }

   err_adv(debug, "reading ply file...");
   read_file();
   err_adv(debug, "done\n");

   write_sm();

   return 0;
}

// end of file ply2sm.C
