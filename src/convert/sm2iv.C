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
/**********************************************************************
 * iv2sm.C:
 **********************************************************************/
#include "mi.H"
#include "patch.H"
#include "stripcb.H"

char header[] =
"#Inventor V2.1 ascii\n"
"\n"
"Separator {\n"
"    ShapeHints {\n"
"       vertexOrdering  COUNTERCLOCKWISE\n"
"       faceType        CONVEX\n"
"    }\n"
"    IndexedTriangleStripSet {\n"
"       vertexProperty         VertexProperty {\n"
"\n";

Cstr_ptr comma(", ");
        
void
write_verts(const BMESH &mesh, ostream &os)
{
    os <<
"          vertex       [";
    const int num = mesh.nverts();
    for (int i = 0; i < num; i++) {
       CWpt &loc = mesh.bv(i)->loc();
       if (i != 0) {
          os << "                          ";
       }
       os << loc[0] << " " << loc[1] << " " << loc[2];
       if (i != num - 1) os << "," << endl;
    }
    os << "]" << endl;
}

void
init_strips(const BMESH &mesh)
{
//   VIEWptr view = new VIEWsimp;
//   VIEW::push(view);

   ((BMESH &) mesh).make_patch_if_needed();
   for (int p = 0; p < mesh.patches().num(); p++) {
      mesh.patches()[p]->build_tri_strips();
   }
//   VIEW::pop();
}

class Formatter {
   protected:
      ostream *_os;
      const    int _width;
      int      _pos;
   public:
      Formatter(ostream *os, int width) : _os(os), _width(width), _pos(0) {}
      void write(Cstr_ptr &str) {
         if (_pos + (int)str->len() > _width) {
            *_os << endl;
            _pos = 0;
         }
         *_os << str;
         _pos += str->len();
      }
};

class IVNormalIterator : public StripCB {
   protected:
      Formatter *_form;
   public:
      IVNormalIterator(Formatter *form) : _form(form) {}

      virtual void faceCB(CBvert *, CBface *f) {
         CWvec &norm = f->norm();
         _form->write(norm[0]);
         _form->write(" ");
         _form->write(norm[1]);
         _form->write(" ");
         _form->write(norm[2]);
         _form->write(comma);
      }
};

void
write_normals(const BMESH &mesh, ostream &os)
{
   Formatter out(&os, 77);
   out.write("          normal     [");
   IVNormalIterator iter(&out);
   for (int p = 0; p < mesh.patches().num(); p++) {
      Patch *patch = mesh.patches()[p];
      patch->draw_tri_strips(&iter);
   }
   os << "]" << endl;
   os << "            normalBinding  PER_FACE" << endl;
}

class IVTriStripIterator : public StripCB {
   protected:
      ostream   *_os;
      Formatter *_form;
      int        _left;

   public:
      IVTriStripIterator(Formatter *form) : _form(form),_left(0) {}
      virtual void begin_faces(TriStrip *str) {
         _form->write(str_ptr(str->verts()[0]->index()) + comma);
         _form->write(str_ptr(str->verts()[1]->index()) + comma);
         _left--;
      }
      virtual void faceCB(CBvert *v, CBface *) {
         _form->write(str_ptr(v->index()) + comma);
      }
      virtual void end_faces(TriStrip *) {
         _form->write(str_ptr("-1"));
         if (_left) _form->write(comma);
      }
      void set_left(int left) { _left = left;}
};

void
write_strips(const BMESH &mesh, ostream &os)
{
    const str_ptr intro("          coordIndex [");
    Formatter out(&os, 77);
    out.write(intro);
    
    IVTriStripIterator iter(&out);

    // from Bface::draw_strips()
    for (int p = 0; p < mesh.patches().num(); p++) {
       Patch *patch = mesh.patches()[p];
       iter.set_left(patch->num_tri_strips());
       patch->draw_tri_strips(&iter);
       // If there are more patches, write out ", "
       if (p < mesh.patches().num() - 1) os << ", ";
    }
    os << "]" << endl;
}

void
write_mesh(const BMESH &mesh, ostream &os)
{
   init_strips(mesh); // Initialize triangle strips

   os << header;
   write_verts(mesh, os);
   write_normals(mesh, os);
   os << "        texCoord [  ]" << endl;
   os << "        orderedRGBA [  ]" << endl;
   os << "        }" << endl;
   os << endl;
   write_strips(mesh, os);
   os << "   }" << endl;
   os << "}" << endl;
}

int 
main(int argc, char *argv[])
{
   if (argc != 1)
   {
      err_msg("Usage: %s < mesh.sm > mesh.iv", argv[0]);
      return 1;
   }

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   // Write it out
   write_mesh(*mesh, cout);
   return 0;
}

/* end of file subdivide.C */
