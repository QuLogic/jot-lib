//----------------------------------------------
//PointPosition Displacement Map Function
//  Matthew Hibbs -- Princeton University
// This script is used in conjunction with JotConverter
// to create a set of jot files for a Lightwave scene.
//----------------------------------------------

@warnings
@version 2.5
@script displace

//Global Variables
theTime, theFrame, theID;
outFile, fileName;
startFrame;
mesh;
pointCtr;
path;
goOn;

//Creation & Deletion
create {}

delete {}

//NewTime
newtime : id, frame, time {
    theTime  = time;
    theFrame = frame;
    theID    = id;
    //Get the path from a temp file at C:\jc.temp
    inFile = File("C:\\jc.temp","r");
    if (!inFile) {
        //If unable to find the path variable, don't do anything
        goOn = false;
    }
    else {
        path = inFile.read();
        if ((path == "") || (!path)) {
            goOn = false;
        }
        else {
            goOn = true;
            startFrame = integer(inFile.read());
        }
        inFile.close();
    }
    //If the mesh ID is not null and goOn is true
    if (theID && goOn && (theID.pointCount() != 0)) {
        pointCtr = 0;
        //Open the output file
        mesh = theID;
        if (startFrame == theFrame) {
            fileName = string(path,getName(theID),"[",formatNumber(theFrame),"].sm.temp");
        }
        else {
            fileName = string(path,getName(theID),"[",formatNumber(theFrame),"].sm");
        }
        outFile  = File(fileName,"w");
        outFile.writeln("");
        outFile.writeln("LMESH\t{");
        outFile.write("\tvertices\t{ {");
    }
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

//Process
process : da {
    if (goOn) {
        pointCtr++;
        //If this is start frame, output one per line
        if (startFrame == theFrame) {
            //Output the point location
            if (pointCtr == 1) {
                outFile.writeln("\t{",-da.source[1]," ",da.source[2]," ",da.source[3],"}");
            }//end if
            else {
                outFile.writeln("{",-da.source[1]," ",da.source[2]," ",da.source[3],"}");
            }//end else
            //If this is the last point, close out the file
            if (pointCtr == mesh.pointCount()) {
                outFile.writeln("\t\t}}");
                outFile.writeln("\t}");
                outFile.close();
            }//end if
        }
        //If this is not the start frame, output all on one line
        else {
            //Output the point location
            if (pointCtr == 1) {
                outFile.write("\t{",-da.source[1]," ",da.source[2]," ",da.source[3],"}");
            }//end if
            else {
                outFile.write("{",-da.source[1]," ",da.source[2]," ",da.source[3],"}");
            }//end else
            //If this is the last point, close out the file
            if (pointCtr == mesh.pointCount()) {
                outFile.writeln("");
                outFile.writeln("\t\t}}");
                outFile.writeln("\t}");
                outFile.close();
            }//end if
        }
    }//end if
}



