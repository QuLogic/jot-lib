//----------------------------------------------
//Lightwave to Jot Format Conversion Function
//  Matthew Hibbs -- Princeton University
// This script is used in conjunction with pointposition.ls
// to create a set of jot files from a Lightwave scene.
//----------------------------------------------

@warnings
@version 2.5
@script generic

//Global Variables
firstFrame, lastFrame;
fileName, fullFileName, path, extension;
scene, cam;
fps;

generic {
    //Warn the use about needing to attach pointposition.ls to all objects
    //that are "real" and should be in the output
    info("You must attach the Displacement Map 'pointposition.ls' to every mesh object that you desire to be in the ouptut.  If you have not done this, please canel on the next screen.");

    //Get information from the user
    return if !UI();

    //Put the path in a temporary file for the pointposition DM to use
    outFile = File("C:\\jc.temp","w");
    outFile.writeln(path);
    outFile.writeln(firstFrame);
    outFile.close();

    //Force the user to make a preview
    info("Please make a preview of 'FIRST Frame' to 'FIRST Frame+1', and then 'End Preview.'");
    MakePreview();    

    //Initialize some other variables
    scene = Scene();
    cam   = Camera();
    fps   = scene.fps;

    //Output the main .jot file
    outFile = File(fullFileName,"w");
    outFile.writeln("#jot");
    outFile.writeln("");

    //--Object Information--
    cObj = getfirstitem(MESH);
    while (cObj) {
        //If the <meshname>[firstframe].sm.temp does not exist, don't use this mesh
        testFile = File(string(path,getName(cObj),"[",formatNumber(firstFrame),"].sm.temp"),"r");
        if (testFile && (cObj.pointCount() != 0)) {
            if (!cObj.isMesh()) {
                warn("Not a mesh: ",cObj);
            }   
            outFile.writeln("TEXBODY\t{");
            outFile.writeln("\tname\t",getName(cObj));
            outFile.writeln("\txform\t{{1 0 0 0}{0 1 0 0}{0 0 1 0}{0 0 0 1}}");
            outFile.writeln("\txfdef\t{ DEFINER");
            outFile.writeln("\t\tDEFINER\t{");
            outFile.writeln("\t\t\tout_mask\t1");
            outFile.writeln("\t\t\tinputs\t{ }");
            outFile.writeln("\t\t\t} }");
            outFile.writeln("\tcolor\t{1 1 1}");
            outFile.writeln("\tmesh_data_file\t{ ",getName(cObj),".sm }");
            outFile.writeln("\t}");
            outFile.writeln("CREATE\t{ ",getName(cObj));
            outFile.writeln("\t}");
        }//end if

        cObj = cObj.next(); //Get the next mesh object
    }//end while

    //--Camera Information--
    outputCameraParameters(outFile,firstFrame);

    //--Window Information--
    outFile.writeln(string("CHNG_WIN\t{ 4 29 ",scene.framewidth," ",scene.frameheight));
    outFile.writeln("\t}");

    //--View Information--
    outFile.writeln("CHNG_VIEW\t{");
    outFile.writeln("\tVIEW\t{");
    outFile.writeln("\t\tview_animator\t{");
    outFile.writeln("\t\t\tAnimator\t{");
    outFile.writeln("\t\t\t\tfps\t30");
    outFile.writeln("\t\t\t\tstart_frame\t",firstFrame);
    outFile.writeln("\t\t\t\tend_frame\t",lastFrame);
    outFile.writeln("\t\t\t\tname\t{ ",fileName," }");
    outFile.writeln("\t\t\t\t} }");
    outFile.writeln("\t\tview_data_file\t{ ",fileName," }");
    outFile.writeln("\t\t}");
    outFile.writeln("\t}");
    //Close the output file
    outFile.close();

    //Create Scene files for each frame
    for (i=firstFrame; i<=lastFrame; i++) {
        //Go to the frame
        GoToFrame(i);
        //Open the output file
        outFile = File(string(path,fileName,"[",formatNumber(i),"]",extension),"w");
        //Output the header info
        outFile.writeln("#jot");
        outFile.writeln("");
        //For each object, output the update geometry information
        cObj = getfirstitem(MESH);
        while (cObj) {
            //If the <meshname>[firstframe].sm.temp does not exist, don't use this mesh
            testFile = File(string(path,getName(cObj),"[",formatNumber(firstFrame),"].sm.temp"),"r");
            if (testFile && (cObj.pointCount() != 0)) {
                outFile.writeln("UPDATE_GEOM\t{ ",getName(cObj));
                outFile.writeln("\tTEXBODY {");
                outFile.writeln("\t\tname\t",getName(cObj));
                outFile.writeln("\t\txform\t{{1 0 0 0}{0 1 0 0}{0 0 1 0}{0 0 0 1}}");
                outFile.writeln("\t\txfdef\t{ DEFINER");
                outFile.writeln("\t\t\tDEFINER\t{");
                outFile.writeln("\t\t\t\tout_mask\t1");
                outFile.writeln("\t\t\t\tinputs\t{ }");
                outFile.writeln("\t\t\t} }");
                outFile.writeln("\t\tcolor\t{1 1 1}");
                outFile.writeln("\t\tmesh_data_update_file\t{ ",getName(cObj),"[",formatNumber(i),"].sm }");
                outFile.writeln("\t\t}");
                outFile.writeln("\t}");
            }//end if

            cObj = cObj.next(); //Get the next mesh object
        }//end while
        //Output the updated camera information
        outputCameraParameters(outFile,i);
        //Close the file
        outFile.close();
    }//end for

    //Force the user to make a preview
    info("Sorry to do this again, but please make a preview of 'FIRST Frame' to 'LAST Frame', and then 'End Preview.'");
    MakePreview();

    info("Finalizing conversion... This will take several minutes.");

    //For each mesh object, create the initial .sm file
    cObj = getfirstitem(MESH);
    needWarn = false;
    while (cObj) {
        //Open the objname[firstFrame].sm.temp file
        inFile = File(string(path,getName(cObj),"[",formatNumber(firstFrame),"].sm.temp"),"r");
        if (inFile && (cObj.pointCount() != 0)) {
            //Output the corrected [firstFrame].sm file
            outFile = File(string(path,getName(cObj),"[",formatNumber(firstFrame),"].sm"),"w");
            //Write over the first two lines as is
            outFile.writeln(inFile.read());
            outFile.writeln(inFile.read());
            //Write the next <pointCount> lines without the carriage return
            for (i=1; i<=(cObj.pointCount()); i++) {
                outFile.write(inFile.read());
            }
            //Finish out the [firstFrame].sm file
            outFile.nl();
            outFile.writeln("\t\t}}");
            outFile.writeln("\t}");
            outFile.close();
            inFile.close();
        }
        //Open the objname[firstFrame].sm.temp file again
        inFile = File(string(path,getName(cObj),"[",formatNumber(firstFrame),"].sm.temp"),"r");
        if (inFile && (cObj.pointCount() != 0)) {
            //Determine the layer of this object
            layerID = -1;
            ctr = 1;
            while (layerID == -1) {
                //-**-HACK - using first layer with the same number of points
                if (cObj.pointCount() == cObj.pointCount(ctr)) {
                    layerID = ctr;
                }
                ctr++;
            }
            //Now output the original .sm file
            outFile  = File(string(path,getName(cObj),".sm"),"w");
            //Write over the first two lines as is
            outFile.writeln(inFile.read());
            outFile.writeln(inFile.read());
            //Write the next <pointCount> lines without the carriage return
            for (i=1; i<=(cObj.pointCount()); i++) {
                outFile.write(inFile.read());
            }
            inFile.close();
            outFile.writeln("}}");
            //Create an array of point Positions
            numPts = 0; //Determine the number of points to look at
            for (i=1; i<=layerID; i++) {
                if (cObj.pointCount(i) != 1) {
                    numPts = numPts + cObj.pointCount(i);
                }
            }
            ctr = 1;
            for (i=1; i<=numPts; i++) {
                if (cObj.layer(cObj.points[i]) == layerID) {
                    pointPos[ctr] = cObj.position(cObj.points[i]);
                    ctr++;
                }
            }//end for
            //Output the face information
            outFile.write("\tfaces\t{ {");
            numPolys = 0; //Determine the number of polys to look at
            for (i=1; i<=layerID; i++) {
                if (cObj.polygonCount(i) != 1) {
                    numPolys = numPolys + cObj.polygonCount(i);
                }
            }
            for (i=1; i<=numPolys; i++) {
            //If the poly is in the right layer
            if (cObj.layer(cObj.polygons[i]) == layerID) {
                poly = cObj.polygons[i];
                //If the polygon has 3 vertices...
                if (cObj.vertexCount(poly)==3) {
                  outFile.write("{");
                     for (j=3; j>=1; j--) {
                        //Search through the pointPos list to find the vertex index
                        pos = cObj.position(cObj.vertex(poly,j));
                        pIdx = 1;
                        found = false;
                        while (!found && pIdx<=ctr) {
                            if (pos == pointPos[pIdx]) { found = true; }
                            else { pIdx = pIdx + 1; }
                        }//end while
                        pIdx = pIdx - 1; //Jot indices start at 0, not 1
                        if (found) { outFile.write(pIdx," "); }
                        else { outFile.write(-1," "); }
                    }//end for
                    outFile.write("}");
                }//end if
                //If the polygon has 4 vertices...
                else  {
                    if (cObj.vertexCount(poly)==4) {
                        //Need to create two triangles with idxs 123 & 341
                        firstIdx = -1;
                        outFile.write("{");
                        for (j=3; j>=1; j--) {
                            //Search through the pointPos list to find the vertex index
                            pos = cObj.position(cObj.vertex(poly,j));
                            pIdx = 1;
                            found = false;
                            while (!found && pIdx<=numPts) {
                                if (pos == pointPos[pIdx]) { found = true; }
                                else { pIdx = pIdx + 1; }
                            }//end while
                            pIdx = pIdx - 1; //Jot indices start at 0, not 1
                            if (j==1)  { firstIdx = pIdx; } //Save this to use later
                            if (found) { outFile.write(pIdx," "); }
                            else { outFile.write(-1," "); }
                        }//end for
                        outFile.write("}{",firstIdx," ");
                        for (j=4; j>=3; j--) {
                            //Search through the pointPos list to find the vertex index
                            pos = cObj.position(cObj.vertex(poly,j));
                            pIdx = 1;
                            found = false;
                            while (!found && pIdx<=numPts) {
                                if (pos == pointPos[pIdx]) { found = true; }
                                else { pIdx = pIdx + 1; }
                            }//end while
                            pIdx = pIdx - 1; //Jot indices start at 0, not 1
                            if (found) { outFile.write(pIdx," "); }
                            else { outFile.write(-1," "); }
                        }//end for
                        outFile.write(" }");
                    }//end if
                    else {
                        if (cObj.vertexCount(poly)!=2) {
                            needWarn=true;
                        }//end if
                    }//end else
                }//end else
            }//end if
            }//end for
            outFile.writeln("}}");
            //Output the rest of the file
            outFile.writeln("\tpatch\t{");
            outFile.writeln("\t\tPatch\t{");
            outFile.writeln("\t\t\tcur_tex\t0");
            outFile.writeln("\t\t\tpatchname\tpatch-0");
            outFile.writeln("\t\t\tcolor\t{1 1 1}");
            outFile.writeln("\t\t\ttexture\t{");
            outFile.writeln("\t\t\t\tNPRTexture\t{");
            outFile.writeln("\t\t\t\t\tnpr_data_file\t{ ",getName(cObj)," }");
            outFile.writeln("\t\t\t\t\t} }");
            outFile.writeln("\t\t\t} }");
            outFile.writeln("\t}");
            outFile.close();
        }//end if

        cObj = cObj.next(); //Get the next mesh object
    }//end while

    //If necessary, warn the user about polygon vertices
    if (needWarn) {
        warn("Only polygons with 3 or 4 vertices are supported.");
    }

    //Make the temporary file blank
    outFile = File("C:\\jc.temp","w");
    outFile.writeln("");
    outFile.close();

    info("Conversion Complete.");
}//end generic

outputCameraParameters : outFile, curFrame {
    //Output Camera Parameters
    cam = Camera();
    campos = cam.getPosition(curFrame/fps);
    camrot = cam.getRotation(curFrame/fps);
    camupvec  = cam.getUp(curFrame/fps);
    camtovec  = cam.getForward(curFrame/fps);
    outFile.write("CHNG_CAM\t{ {");
    //Position (From)
    outFile.write(-campos.x," ",campos.y," ",campos.z,"}{");
    //At (To) -- point rather than vector
    outFile.write(-(campos.x+camtovec.x)," ",(campos.y+camtovec.y)," ",(campos.z+camtovec.z),"}{");
    //Up -- point, not vector
    outFile.write(-(campos.x+camupvec.x)," ",(campos.y+camupvec.y)," ",(campos.z+camupvec.z),"}{");
    //Center -- same as At
    outFile.write((campos.x+camtovec.x)," ",(campos.y+camtovec.y)," ",(campos.z+camtovec.z),"}");
    //FocalLength PerspectiveFlag InterocularDistance
    outFile.writeln(cam.zoomFactor(curFrame/fps)*0.1/2," 1 2.25");
    outFile.writeln("\t}");
}

formatNumber : num {
    //Extend a number to have 5 digits (eg. 00002, 00121)
    if (num < 10) { return(string("0000",num)); }
    else {
        if (num < 100) { return(string("000",num)); }
        else {
            if (num < 1000) { return(string("00",num)); }
            else {
                if (num < 10000) { return(string("0",num)); }
                else { return(string(num)); }
    }}}//end 3 more else's
}//end formatNumber

getName : object {
    //Extract the objects name
    theName = object.name;
    //Remove 'bad' characters from the name (such as ':')
    tokens = parse(":",theName);
    //Reconstruct the name
    result = "";
    for (i = 1; i <= size(tokens); i++) {
        result = string(result,tokens[i]);
    }//end for
    return(result);
}//end getName

UI {
    //-- Do UI Stuff Here --
    scene = Scene();
    reqbegin("Lightwave to Jot Format Converter");
        reqsize(500,170);
        cFF = ctlinteger("Start Frame: ",scene.framestart);
        cLF = ctlinteger("End Frame: ",scene.frameend);
        ctltext("","Specify the .jot file that will contain the scene information");
        ctltext("","Several additional files will also be created in this directory");
        cP  = ctlfilename("Save Path: ",scene.filename,70,0);

        return false if !reqpost();

        firstFrame = getvalue(cFF);
        lastFrame  = getvalue(cLF);
        fullFileName   = fullpath(getvalue(cP));
    reqend();

    //Break the save file down into drive, path, file, extension
    pathTemp = split(fullFileName);
    //Verify that the file name is proper and form the path
    if (pathTemp[3].size() == 0) {
        pathTemp[3] = "out";
    }
    if (pathTemp[4] != ".jot") {
        pathTemp[4] = ".jot";
        fullFileName = string(pathTemp[1],pathTemp[2],pathTemp[3],pathTemp[4]);
    }
    extension = pathTemp[4];
    fileName = pathTemp[3];
    path = string(pathTemp[1],pathTemp[2]);

    return true;
}


