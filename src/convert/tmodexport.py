#####################################
# TMOD Export Script v1.0.7 - 5/11/02
#####################################
# Send complaints to rkalnins
#####################################

import Blender210
import Blender 
import math
import string

#Defaults
fps = 12
destname = "scene"
destpath = "d:\\coding\\jot\\anim\\"

status = "Press ESC to exit...                                                   "

no_save_prefix = ["Proto","Data"]
no_update_prefix = ["Water_"]

#********************************
#********************************
# CLASS: TMODExport
#********************************
#********************************
class TMODExport:

#############################
# __init__
#############################
	def __init__(self):
		global destpath
		global destname

		#XXX - Find a newer version calls for these 4 values...
		display = Blender210.getDisplaySettings()
		self.startFrame = display.startFrame;
		self.endFrame = display.endFrame;
		self.x = display.xResolution
		self.y = display.yResolution

		self.mesh_objs = []
		self.lamp_objs = []
		self.cam_objs = []

		#Pre-cache the meshes in the mesh_objs
		#as we'll leak memory otherwise... 
		#XXX-No worky!
		#self.meshes = []

		print "---------TMOD Export---------"

		for o in Blender.Object.Get():
			try:
				if (o.data != None):
					if (type(o.data) == Blender.Types.NMeshType):
						print "Found mesh object '%s'" % o.name
						ignore = 0
						for ig in no_save_prefix:
							if o.name.startswith(ig): ignore = 1
						if (ignore):
							print ("Ignoring object '%s' matching no_save_prefix list" % o.name)
						else:
							self.mesh_objs.append(o)
							#self.meshes.append(Blender.NMesh.GetRawFromObject(o.name))
					elif (type(o.data) == Blender.Types.BlockType):
						if o.data.block_type == "Camera":
							self.cam_objs.append(o)
							print "Found camera object '%s'" % o.name
						elif o.data.block_type == "Lamp":
							self.lamp_objs.append(o)
							print "Found lamp object '%s'" % o.name
						else:
							print ("Ignoring object '%s' of unknown type '%s'" %
										(o.name,o.data.block_type))
				else:
					print "Ignoring object '%s' with null data" % o.name
			except:
				print "Ignoring object '%s' lacking data field" % o.name
					
		#if len(self.mesh_objs) == 0:
			#print "\nWarning!! No meshes found."

		if len(self.cam_objs) == 0:
			print "\nWarning!! No camera found."

		if len(self.cam_objs) > 1:
			print "\nWarning!! More than one camera found. Using first one!!!"

		print "-----------------------------"

#############################
# writeLamp
#############################
	def writeLamp(self, lamp_obj, file):
		
		lamp = Blender.Lamp.Get(lamp_obj.name)

		#XXX - Not handling lamps at this time!!!

		#x = lampobj.matrix[3][0] / lampobj.matrix[3][3]
		#y = lampobj.matrix[3][1] / lampobj.matrix[3][3]
		#z = lampobj.matrix[3][2] / lampobj.matrix[3][3]
		#self.file.write('LightSource "pointlight" %s ' % num +
		#				'"from" [%s %s %s] ' % (x, y, z) +
		#				'"lightcolor" [%s %s %s] ' % (lamp.R, lamp.G, lamp.B) +
		#				'"intensity" 50\n')

#############################
# writeMatrix
#############################
	def writeMatrix(self, matrix, file):
		file.write("{{%s %s %s %s }{%s %s %s %s }{%s %s %s %s }{%s %s %s %s }}" %
						(matrix[0][0], matrix[2][0],-matrix[1][0], matrix[3][0],
						 matrix[0][2], matrix[2][2],-matrix[1][2], matrix[3][2],
						-matrix[0][1],-matrix[2][1], matrix[1][1],-matrix[3][1],
						 matrix[0][3], matrix[2][3],-matrix[1][3], matrix[3][3]))

#############################
# writeMesh
#############################
	def writeMesh(self, mesh, file, objname, update):
		global destname
	
		#VERTS
		file.write("%s\n" % (len(mesh.verts)))
		for i in xrange(len(mesh.verts)):
			vertex = mesh.verts[i]
			file.write("%s %s %s\n" % (vertex.co[0], vertex.co[2], -vertex.co[1]))
		file.write('\n')	

		if not update:
			
			#FACES

			#Count faces
			numf=0
			for i in xrange(len(mesh.faces)):
				if len(mesh.faces[i].v) == 4: 
					numf = numf + 2
				elif len(mesh.faces[i].v) == 3: 
					numf = numf + 1
				else:
					print "Bad face has %d verts" % (len(mesh.faces[i].v))
			file.write("%s\n" % (numf))
			
			#Write faces
			for i in xrange(len(mesh.faces)):
				face = mesh.faces[i]
				if len(face.v)==3 or len(face.v)==4:  #quad
					file.write("%s %s %s \n" % (face.v[0].index, face.v[1].index, face.v[2].index))
					if len(face.v) == 4:  #quad
						#second triangle
						file.write("%s %s %s \n" % (face.v[0].index, face.v[2].index, face.v[3].index))
			
			#CREASES
			file.write("\n0\n")

			#POLYLINES
			file.write("\n0\n")

			#UV
			#Note these other great undocumented calls!
			if mesh.hasFaceUV() or mesh.hasVertUV():

				file.write("\n#BEGIN TEX_COORDS2\n")

				#Count faces with UV
				#XXX - Assume they all do for now
				numuv=0
				for i in xrange(len(mesh.faces)):
					if 1: #<<<insert face uv test here>>>
						if len(mesh.faces[i].v) == 4: 
							numuv = numuv + 2
						elif len(mesh.faces[i].v) == 3: 
							numuv = numuv + 1
						#else:
							#print "Bad uv face has %d verts" % (len(mesh.faces[i].v))
				file.write("%d\n" % (numuv))

				#Write UVs
				j = 0 #tri index
				for i in xrange(len(mesh.faces)):
					face = mesh.faces[i]
					if len(face.v)==4 or len(face.v)==3: 
						if 1: #<<<insert face uv test here>>>
							if (mesh.hasFaceUV()):
								file.write("%d < %f %f > < %f %f > < %f %f >\n" % 
										( j,
										  face.uv[0][0], face.uv[0][1],
  										  face.uv[1][0], face.uv[1][1],
										  face.uv[2][0], face.uv[2][1]))
								j = j + 1
								if len(face.v)==4: 
									file.write("%d < %f %f > < %f %f > < %f %f >\n" % 
										( j,
										  face.uv[0][0], face.uv[0][1],
  										  face.uv[2][0], face.uv[2][1],
										  face.uv[3][0], face.uv[3][1]))
									j = j + 1
							elif (mesh.hasVertUV()):
								file.write("%d < %f %f > < %f %f > < %f %f >\n" % 
										( j,
										  face.v[0].uvco[0], face.v[0].uvco[1],
  										  face.v[1].uvco[0], face.v[1].uvco[1],
										  face.v[2].uvco[0], face.v[2].uvco[1]))
								j = j + 1
								if len(face.v)==4: 
									file.write("%d < %f %f > < %f %f > < %f %f >\n" % 
										( j,
										  face.v[0].uvco[0], face.v[0].uvco[1],
  										  face.v[2].uvco[0], face.v[2].uvco[1],
										  face.v[3].uvco[0], face.v[3].uvco[1]))
									j = j + 1
							#else:
								#Huh?!
						else:
							if len(face.v)==4:
								j = j + 2
							elif len(face.v)==3:
								j = j + 1

				file.write("#END TEX_COORDS2\n")

			#External Annotations!!
			file.write("\n#BEGIN PATCH\n")
			file.write("0\n")
			file.write("\n")
			file.write("0\n")
			file.write("#BEGIN GTEXTURE\n")
			file.write("NPRTexture\n")
			file.write("{\n")
			file.write("\tnpr_data_file\t{ %s-%s }\n" % (destname, string.lower(objname)))
			file.write("\t}\n")
			file.write("#END GTEXTURE\n")
			file.write("\n#BEGIN PATCHNAME\n")
			file.write("patch-0\n")
			file.write("#END PATCHNAME\n")
			file.write("#BEGIN COLOR\n")
			file.write("< 1 1 1 >\n")
			file.write("#END COLOR\n")
			file.write("#END PATCH\n")


#############################
# writeTexbody
#############################
	def writeTexbody(self, mesh_obj, num, file, update, tabs):
			global destpath
			global destname

			self.writeTabs(file,tabs)
			file.write("TEXBODY\t{\n")

			self.writeTabs(file,tabs+1)
			file.write("name\t%s\n" % (destname + "-" + mesh_obj.name))				

			self.writeTabs(file,tabs+1)
			file.write("xform\t")
			self.writeMatrix(mesh_obj.matrix,file)
			file.write("\n")

			self.writeTabs(file,tabs+1)
			file.write("xfdef\t{ DEFINER\n")
			self.writeTabs(file,tabs+1)
			file.write("\tDEFINER\t{\n")
			self.writeTabs(file,tabs+1)
			file.write("\t\tout_mask\t1\n")
			self.writeTabs(file,tabs+1)
			file.write("\t\tinputs\t{ }\n")
			self.writeTabs(file,tabs+1)
			file.write("\t\t} }\n")

			#color - hacked to (1 1 1)
			self.writeTabs(file,tabs+1)
			file.write("color\t{1 1 1 }\n")

			do_mesh = 1
			if (update):
				for nu in no_update_prefix:
					if (mesh_obj.name.startswith(nu)): do_mesh=0
				if (do_mesh):
					objfilename = destname + "-%s[%05d].sm" % (string.lower(mesh_obj.name), num)
					objfile	 = open(destpath + objfilename,'w')
					self.writeTabs(file,tabs+1)
					file.write("mesh_update_file\t{ %s }\n" % (objfilename))				
			else:
				objfilename = destname + "-%s.sm" % (string.lower(mesh_obj.name))
				objfile	 = open(destpath + objfilename,'w')
				self.writeTabs(file,tabs+1)
				file.write("mesh_file\t{ %s }\n" % (objfilename))				
	
			self.writeTabs(file,tabs+1)
			file.write("}\n")

			if (do_mesh):
				print("  Writing '%s'" % (objfilename))

				#The cached mesh doesn't get updated at each frame...
				#mesh = self.meshes[self.mesh_objs.index(mesh_obj)]
				mesh = Blender.NMesh.GetRawFromObject(mesh_obj.name)
				self.writeMesh(mesh,objfile,mesh_obj.name,update)

				objfile.write('\n')
				objfile.close()
			#else:
				#print ("Ignoring mesh update for '%s' matching no_update_prefix list" % mesh_obj.name)



#############################
# writeGeom
#############################
	def writeGeom(self, mesh_obj, num, file, update):
		
		if (update):
			file.write("UPDATE_GEOM\t{ %s\n" % (destname + "-" + mesh_obj.name))
			self.writeTexbody(mesh_obj,num,file,update,1)
			file.write("\t}\n")
		else:
			self.writeTexbody(mesh_obj,num,file,update,0)
			file.write("CREATE\t{ %s\n" % (destname + "-" + mesh_obj.name))
			file.write("\t}\n")
	   

#############################
# writeTabs
#############################
	def writeTabs(self,file,num):
		for n in xrange(num):
			file.write("\t")


#############################
# writeHeader
#############################
	def writeHeader(self,file):
		file.write("#jot\n\n")

#############################
# writeCamera
#############################
	def writeCamera(self,file):
		
		if len(self.cam_objs) > 0:
			camobj = self.cam_objs[0]
			#camera = Blender.Camera.Get(camobj.name)
			camera = camobj.data

			aspect = self.x / float(self.y)
			focal = aspect * 2.0 * 16.0 / camera.Lens * 0.1
			file.write("CHNG_CAM\t{ {%f %f %f }{%f %f %f }{%f %f %f }{%f %f %f }%f %d %f\n\t}\n" %
							(camobj.matrix[3][0],
							 camobj.matrix[3][2],
							 -camobj.matrix[3][1],

							 camobj.matrix[3][0] - camobj.matrix[2][0],
							 camobj.matrix[3][2] - camobj.matrix[2][2], 
							 -camobj.matrix[3][1] + camobj.matrix[2][1],


							 camobj.matrix[3][0] + camobj.matrix[1][0], 
							 camobj.matrix[3][2] + camobj.matrix[1][2], 
							 -camobj.matrix[3][1] - camobj.matrix[1][1],


							 camobj.matrix[3][0] - camobj.matrix[2][0],
							 camobj.matrix[3][2] - camobj.matrix[2][2],
							 -camobj.matrix[3][1] + camobj.matrix[2][1],

							 focal, 
							 1,		#persp
							 2.25))	#stero sep

#############################
# writeWindow
#############################
	def writeWindow(self,file):

		file.write("CHNG_WIN\t{ %d %d %d %d \n\t}\n" %
						(3,
						 29,
						 self.x, 
						 self.y))

#############################
# writeView
#############################
	def writeView(self,file):
		global fps
		global destname

		file.write("CHNG_VIEW\t{\n")
		file.write("\tVIEW\t{\n")
		file.write("\t\tview_animator\t{\n")
		file.write("\t\t\tAnimator\t{\n")
		file.write("\t\t\t\tfps\t%d\n" % (fps))
		file.write("\t\t\t\tstart_frame\t%d\n" % (self.startFrame))
		file.write("\t\t\t\tend_frame\t%d\n" % (self.endFrame))
		file.write("\t\t\t\tname\t{ %s }\n" % (destname))
		file.write("\t\t\t\t} }\n")
		file.write("\t\tview_data_file\t{ %s }\n" % (destname))
		file.write("\t\t}\n")
		file.write("\t}\n")
		
#############################
# writeFrame
#############################
	def writeFrame(self, frame, num, update):

		Blender.Set('curframe',frame)

		if (update):
			filename = "%s[%05d].tmod" % (destname,num)
		else:
			filename = "%s.tmod" % (destname)
		file	 = open(destpath + filename, "w")

		print(" Writing '%s'" % (filename))

		self.writeHeader(file)

		for m in self.mesh_objs:
			self.writeGeom(m, num, file, update)

		for l in self.lamp_objs:
			self.writeLamp(l, file)

		self.writeCamera(file)

		if not update:
			self.writeWindow(file)
			self.writeView(file)

		file.close()

#############################
# export
#############################
	def export(self):
		global status

		print "Exporting..."

		status = "Exporting Base Frame                                                "
		Blender.Draw.Draw()

		num = self.startFrame;
		self.writeFrame(self.startFrame, num, 0)
		
		for frame in xrange(self.startFrame, self.endFrame + 1):
			status = ("Exporting Frame %d of %d to %d                                         " %
							( frame, self.startFrame, self.endFrame ))
			Blender.Draw.Draw()
			self.writeFrame(frame, num, 1)
			num += 1
		
		status = "Press ESC to exit...                                                   "

		print "DONE!"

#############################
# GUI Stuff
#############################

gui_fps =  None
gui_name=  None
gui_path = None
gui_go =   None

def gui_draw():
	global gui_fps, gui_name, gui_path, gui_go
	global status

	Blender.BGL.glClearColor(0.5,0.5,0.5,0.0)
	Blender.BGL.glClear(Blender.BGL.GL_COLOR_BUFFER_BIT)

	gui_fps =  Blender.Draw.Number("FPS: ",  1, 10,  10, 70,  30, fps, 1, 60, "Frames per second.")
	gui_name = Blender.Draw.String("Name: ", 2, 90,  10, 180, 30, destname, 20, "Base file name for output.")
	gui_path = Blender.Draw.String("Path: ", 3, 10,  50, 260, 30, destpath, 200, "Destination path for output. With trailing slash!")
	gui_go =   Blender.Draw.Button("GO! ",   4, 280, 10, 70,  70, "Export!!!")

	Blender.BGL.glRasterPos2i(10, 100)
	Blender.Draw.Text(status)

def gui_event(evt, val):
	if (evt == Blender.Draw.ESCKEY and not val): Blender.Draw.Exit()

def gui_control_event(evt):
	global gui_fps, gui_name, gui_path, gui_go
	global fps, destname, destpath

	#FPS
	if (evt == 1):
		fps = gui_fps.val
	#NAME
	elif (evt == 2):
		destname = gui_name.val
	#PATH
	elif (evt == 3):
		destpath = gui_path.val
	#GO
	elif (evt == 4):
		tmodexport = TMODExport()
		tmodexport.export()
	else:
		print "Unknown event!?!??"

#############################
# Register GUI Event Loop
#############################

Blender.Draw.Register(gui_draw, gui_event, gui_control_event)
    

