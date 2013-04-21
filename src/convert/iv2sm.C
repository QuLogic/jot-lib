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
/*-----------------------------------------------------------
 *  This is an example from The Inventor Mentor,
 *  chapter 9, example 5.
 *
 *  Using a callback for generated primitives.
 *  A simple scene with a sphere is created.
 *  A callback is used to write out the triangles that
 *  form the sphere in the scene.
 *----------------------------------------------------------*/
#define ROOM_STANDALONE
#include <Inventor/SoDB.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <stdlib.h>

#include "std/support.H"
#include "std/stop_watch.H"
#include "mesh/bmesh.H"
#include "mesh/patch.H" //add patches -sginsber
#include "dev/dev.H"
#include "dev/devpoll.H"
#include "mesh/gtexture.H"

// Reference dev symbols
DEVice_2d *DEVice_2d::last = 0;
ARRAY<DEVpoll *> DEVpoll::_pollable;

// Hack to make sure BOOTH symbols are linked in
inline Wpt      RET_Wpt(CBOOTHpt &p)   { return Wpt     (p[0], p[1], p[2]); }
inline Wvec     RET_Wvec(CBOOTHvec &v) { return Wvec    (v[0], v[1], v[2]); }
inline BOOTHpt  RET_BOOTHpt(CWpt &p)   { return BOOTHpt (p[0], p[1], p[2]); }
inline BOOTHvec RET_BOOTHvec(CWvec &v) { return BOOTHvec(v[0], v[1], v[2]); }
Wpt     (*BOOTHpttoWpt)(CBOOTHpt &)    = RET_Wpt;
Wvec    (*BOOTHvectoWvec)(CBOOTHvec &) = RET_Wvec;
BOOTHpt (*WpttoBOOTHpt)(CWpt &)        = RET_BOOTHpt;
BOOTHvec(*WvectoBOOTHvec)(CWvec &)     = RET_BOOTHvec;

// #include "/u/lsh/space/piggy/debug.H"
void outputMesh(ostream &os);

int correct = 0;
int separate_files = 0;
int obj_num = 0;

#include <iostream.h>
class Triangle {
    private:
       int _verts[3];
    public:
       Triangle() {_verts[0]=_verts[1]=_verts[2]=-1;}
       Triangle(int x, int y, int z) { _verts[0]=x; _verts[1]=y;_verts[2]=z; }
      void set (int x, int y, int z) { _verts[0]=x; _verts[1]=y;_verts[2]=z; }
       const int *data() const {return _verts;}
             int operator[](int x)       {return _verts[x];}
             int operator[](int x) const {return _verts[x];}
       int operator ==(const Triangle &tri) {
          return tri._verts[0] == _verts[0] &&
                 tri._verts[1] == _verts[1] &&
                 tri._verts[2] == _verts[2];
       }
};

ostream &operator<<(ostream &os, const Triangle &tri) {
   return os << tri[0] << " " << tri[1] << " " << tri[2];
}

ARRAY<SbVec3f>  points(1024);
ARRAY<Triangle> tris(1024);
ARRAY<SbVec2f>  texcoords(1024);

//array for textures
//array for colors? no, only a vector

//CCOLOR objColor;
SbVec3f objColor;

int badtris = 0;
int reppts  = 0;
int duptris = 0;
ostream *output_stream = 0;

void
initialize_mesh()
{
   points.clear();
   texcoords.clear();
   tris.clear();   
   badtris = 0;
   reppts  = 0;
   duptris = 0;
}

// Function prototypes
void scenegraph_to_tris(SoNode *);
SoCallbackAction::Response shapeCallback(void *, 
   SoCallbackAction *, const SoNode *);

SoCallbackAction::Response materialCallback(void *,
   SoCallbackAction *, const SoNode *);

void printTriangleCallback(void *, SoCallbackAction *,
   const SoPrimitiveVertex *, const SoPrimitiveVertex *,
   const SoPrimitiveVertex *);
int printVertex(const SoPrimitiveVertex *, const SbMatrix &mat);

//////////////////////////////////////////////////////////////
// CODE FOR The Inventor Mentor STARTS HERE


void
scenegraph_to_tris(SoNode *root)
{
  //create a callbackaction that will send a callback
  //every time a given type of node is encountered while
  //traversing the scenegraph -sginsber
  SoCallbackAction myAction;
  //add a callback for a color block -sginsber
  myAction.addPreCallback(SoMaterial::getClassTypeId(),
			  materialCallback, NULL);
  
  //specify that a shapeCallback should be executed 
  //every time a shapenode is encountered -sginsber
  myAction.addPreCallback(SoShape::getClassTypeId(), 
			  shapeCallback, NULL);

  
     
  //now, call a printTriangleCallback whenever a triangle
  //node is encountered -sginsber
  myAction.addTriangleCallback(SoShape::getClassTypeId(), 
			       printTriangleCallback, NULL);
  
  myAction.apply(root);
}


void
add_state_coords(SoCallbackAction *cb)
{
  // cerr << "adding state coords" << endl; //-sginsber
   if (!points.empty()) {
      cerr << "add_state_coords - warning: removing unused points" << endl;
   }
   points.clear();
   const SbMatrix &mat = cb->getModelMatrix();
   int i;

   // Add coords
   int32_t num_coords = cb->getNumCoordinates();
   // cerr << "state coords: " << num_coords << endl;
   for (i = 0; i < num_coords; i++) {
      SbVec3f point;
      mat.multVecMatrix(cb->getCoordinate3(i), point);
      points += point;
   }

   // Add texture coords if any
   if (num_coords = cb->getNumTextureCoordinates()) {

      // cerr << "adding texture coords" << endl;
      if (!texcoords.empty()) {
         cerr << "add_state_coords - warning: "
              << "removing unused texcoords" << endl;
      }
      texcoords.clear();

      // XXX - should this be handled?
      const SbMatrix &texmat = cb->getTextureMatrix();

      // cerr << "tex coords: " << num_coords << endl;
      for (i = 0; i < num_coords; i++) {
         texcoords += cb->getTextureCoordinate2(i);
      }
   }
}

void
add_field_coords(SoCallbackAction *cb, const SoMFVec3f &coords)
{
  // cerr << "adding field coords" << endl; //-sginsber
   if (!points.empty()) {
      cerr << "add_field_coords - warning: removing unused points" << endl;
   }
   points.clear();
   const SbMatrix &mat = cb->getModelMatrix();
   int i;

   const SbVec3f *verts = coords.getValues(0);
   // cerr << "field coords: " << coords.getNum() << endl;//-sginsber
   for (i = 0; i < coords.getNum(); i++) {
      SbVec3f point;
      mat.multVecMatrix(verts[i], point);
      points += point;
   }
}

// Translates an SoIndexedFaceSet into a .sm triangles
// Note - only can handle nodes with 3-triangle faces (triangles)
void
indexed_face_set(SoIndexedFaceSet *fs, SoCallbackAction *cb)
{
   add_state_coords(cb);

   if (!tris.empty()) {
      cerr << "indexed_face_set - warning: removing unused tris" << endl;
   }
   tris.clear();

   // Add triangles
   const int32_t *tri_coords = fs->coordIndex.getValues(0);
   for (int i = 0; i < fs->coordIndex.getNum(); i+=4) {
      tris.add(Triangle(tri_coords[i], tri_coords[i+1], tri_coords[i+2]));
   }
}

//create a list of xyz vertices -sginsber
void
ind_tri_strip(SoIndexedTriangleStripSet *tristrip, SoCallbackAction *cb)
{
   if (tristrip->vertexProperty.getValue() != 0) {
     cerr << "ind_tri_strip: vertexProperty != 0" << endl;//-sginsber
      // Coordinates are in a SoVertexProperty
      SoNode *vertprop = tristrip->vertexProperty.getValue();
      add_field_coords(cb, ((SoVertexProperty *) vertprop)->vertex);
   } else {
     // cerr << "ind_tri_strip: vertexProperty == 0" << endl;
      add_state_coords(cb);
   }

   //vertices have been added to points array
   // cerr << "Points: " << points.num() << endl;
   if (!tris.empty()) {
      cerr << "indexed_face_set - warning: removing unused tris" << endl;
   }
   tris.clear();

   // Add triangles
   // (from soft/ml/oiv/nsg_inv_fields.C, map_faces_to_nsg())
   // cerr << "tristrip - tris" << endl;
   const int32_t *tri_coords = tristrip->coordIndex.getValues(0);
   int firstvert  = -1;
   int secondvert = -1;
   for (int i = 0; i < tristrip->coordIndex.getNum(); i++) {
      if (tri_coords[i] == -1) {
         firstvert = -1;
         secondvert = -1;
      } else {
         if (firstvert != -1 && secondvert != -1 && firstvert != secondvert) {
            tris.add(Triangle(firstvert, secondvert,tri_coords[i]));
         }
         firstvert = secondvert;
         secondvert = tri_coords[i];
      }
   }
}



//called when the color information node for an object is
//encountered. it seems jot can only handle diffuse color
//so thats all that is taken
SoCallbackAction::Response
materialCallback(
		 void             *,
		 SoCallbackAction * cbact,
		 const SoNode     * node)
{		
  
  // cerr << "Material Callback" << endl;
  
  if (separate_files) {
            
    //cerr << "correct material node type" << endl;
    SoMaterial* m = (SoMaterial *) node;
    
    //cerr << "name:" << m->getName().getString() << endl;
    //cerr << "type:" << m->getTypeId().getName().getString() << endl;
                 
    SoMFColor c = m->diffuseColor;    
   
    const SbColor col = *(c.getValues(0));
        
    // cerr << "found diffuse color: " << col[0] << " " << col[1] << " " << col[2] << endl;
    
    SbVec3f vec(col[0],col[1],col[2]);
   
    objColor = vec;        
    
  } 
  return SoCallbackAction::CONTINUE;
} 
 
 
//called whenever a shape node is encountered by the 
//SoCallbackAction traversal -sginsber
SoCallbackAction::Response
shapeCallback(
   void             *,
   SoCallbackAction * cbact,
   const SoNode     * node)
{

  // cerr << "Shape Callback" << endl; //-sginsber
  

  // Print the node name (if it exists) and address
  //if (! !node->getName())
  //  printf("named \"%s\" ", node->getName());
  //printf("at address %#x\n", node);
  
  
  //we output each object to a separate file. therefor, if tris 
  //is not empty, write it out and start a new file -sginsber
  if (separate_files) {    
    if (!tris.empty()) {
      outputMesh(*output_stream);
    }
    initialize_mesh();
    delete output_stream;
    char buff[1024];
    sprintf(buff, "obj%06d.sm", obj_num++);
    output_stream = new ofstream(buff);
  }
  
  if (separate_files) {
    // Currently only can do these shortucts if we are doing 1 obj/file
    // SoIndexedFaceSet w/ only triangles
    //so this is for the triangles -sginsber
    if (node->isOfType(SoIndexedFaceSet::getClassTypeId())) {
      // cerr << "shapeCallback(): node is of type IndexedFaceSet" << endl; //-sginsber
      SoIndexedFaceSet *fs = (SoIndexedFaceSet *) node;
      const int32_t *values = fs->coordIndex.getValues(0);
      bool ok = true;
      for (int i = 0; ok && i < fs->coordIndex.getNum(); i++) {
	if ((i + 1) % 4 ==  0) {
	  ok = values[i] == -1;
	} else ok = values[i] != -1;
      }
      if (ok) {
	indexed_face_set(fs, cbact);
	return SoCallbackAction::PRUNE;
      }
    }
    //and this is for the vertices -sginsber
    else if (node->isOfType(SoIndexedTriangleStripSet::getClassTypeId())) {
      // cerr << "shapeCallback(): node is of type SoIdexedTriangleStrip" << endl; //-sginsber
      ind_tri_strip((SoIndexedTriangleStripSet *)node, cbact);
      return SoCallbackAction::PRUNE;
    }      
    
  }
  
  return SoCallbackAction::CONTINUE;
}



void
printTriangleCallback(void *, SoCallbackAction *cb,
   const SoPrimitiveVertex *vertex1,
   const SoPrimitiveVertex *vertex2,
   const SoPrimitiveVertex *vertex3)
{
//   printf("Triangle:\n");
   int a, b, c;
   const SbMatrix &mat = cb->getModelMatrix();
   a = printVertex(vertex1, mat);
   b = printVertex(vertex2, mat);
   c = printVertex(vertex3, mat);
   
/*
   const SbVec3f &norm = vertex1->getNormal();
   // cerr << "Dot: " << norm.dot((vertex2->getPoint()-vertex1->getPoint())
         .cross(vertex3->getPoint() - vertex1->getPoint())) << " " ;
   switch (cb->getVertexOrdering()) {
       case SoShapeHints::UNKNOWN_ORDERING : cerr << "Unknown" << endl;
       break;
       case SoShapeHints::CLOCKWISE: cerr << "Clockwise" << endl;
       break;
       case SoShapeHints::COUNTERCLOCKWISE: cerr << "ccw" << endl;
       break;
   }
*/

   if (a == b || a == c ||  b == c) {
       badtris++;
   } else {
      tris.add(Triangle(a, b, c));
   }
}

int
printVertex(const SoPrimitiveVertex *vertex,
            const SbMatrix &mat)
{
   SbVec3f point;
   mat.multVecMatrix(vertex->getPoint(), point);

   // new plan: we add disconnected triangles,
   // then fix them up later
   
   points += point;
   return points.num() - 1;
/*
   int index = -1;
   const float eps = 1e-7;
//   const float eps = 1e-5;

   for (int i = points.num()-1; index == -1 && i >= 0; i--) {
      if (points[i].equals(point, eps)) {
         index = i;
      }
   }
   if (index == -1) {
      index = points.num();
      points += point;
      if (points.num() % 1000 == 0) {
         // fprintf(stderr, "%d vertices\n", points.num());
      }
   } else {
      reppts++;
   }
   return index;
*/
}

// CODE FOR The Inventor Mentor ENDS HERE
///////////////////////////////////////////////////////////////

void
outputMesh(ostream &os)
{
   // fprintf(stderr, "Starting to output\n");
   BMESHptr mesh = new BMESH;
  
   int i;
   for (i = 0; i < points.num(); i++) {
      const float *data = points[i].getValue();
      mesh->add_vertex(Wpt(data[0], data[1], data[2]));
   }
  
   for (i = 0; i < tris.num(); i++) {
      const int *data = tris[i].data();
      mesh->add_face(data[0], data[1], data[2]);
   }

   if (!texcoords.empty()) {
      const float* data = 0;
      for (i = 0; i < mesh->nfaces(); i++) {
         // for each face, set texture coordinates of
         // the 3 vertices of that face.
         Bface* f = mesh->bf(i);

         // 1st vertex
         data = texcoords[f->v(1)->index()].getValue();
         UVpt a(data[0], data[1]);

         // 2nd vertex
         data = texcoords[f->v(2)->index()].getValue();
         UVpt b(data[0], data[1]);

         // 3rd vertex
         data = texcoords[f->v(3)->index()].getValue();
         UVpt c(data[0], data[1]);

         f->set_tex_coords(a, b, c);
      }
   }

   mesh->changed(BMESH::TOPOLOGY_CHANGED);

   // Remove duplicate vertices
   mesh->remove_duplicate_vertices(false); // false = don't keep the bastards

   mesh->write_stream(os);
}

void
usage(char *arg)
{
   cerr << "Translates Open Inventor file from file.iv or stdin (if - is used)"
         << endl;
   cerr << endl;
   cerr << arg << " [-csh?] [file.iv | -]" << endl;
   cerr << "into one or more .sm mesh files" << endl;
   cerr << "   -c - Correct meshes so there are no duplicate vertices" << endl;
   cerr << "   -s - Output each object to a different .sm file" << endl;
   cerr << "   -h, -? - this message" << endl;
   exit(1);
}

int
main(int argc, char **argv)
{
   stop_watch timer;
   timer.set();

   // Initialize Inventor
   SoDB::init();
   int c = 0;
   while ((c = getopt(argc, argv, "cs?h")) != EOF) {
      switch (c) {
         case 'c': correct = 1;
       brcase 's': separate_files = 1;
       brcase '?':
         case 'h': usage(argv[0]);
      }
   }


   // Open and read input scene graph
   SoInput		in;
   SoNode		*root;
   if (argc == optind) {
      usage(argv[0]);
   } else if (strcmp(argv[optind], "-") == 0) {
     // from stdin
     in.setFilePointer(stdin);
   } else if (! in.openFile(argv[optind])) {
	fprintf(stderr, "%s: Cannot open %s\n", argv[0], argv[optind]);
	exit(1);
   }
   root = SoDB::readAll(&in);
   if (root == NULL) {
	fprintf(stderr, "%s: Problem reading data\n", argv[0]);
	exit(1);
    }

   root->ref();//reference all instances in scenegraph
   // Collect the triangles
   // fprintf(stderr, "Starting to convert (line 531)\n");
   scenegraph_to_tris(root);

   //call method that sets up textures
   //call method that sets up colors


   // Output the triangles to stdout
   if (output_stream) outputMesh(*output_stream);
   else outputMesh(cout);
   root->unref();

   timer.print_time();
   return 0;
}
