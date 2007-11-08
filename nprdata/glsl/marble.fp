// Marble shader fragment program
// simple perlin test based on the toon shader 


//this shader program will be changing quite a bit
//as I devise more things to test
//It should not be relied upon to produce a consistant 
//effect from version to version

//Current version : Cow Patches Texture


// using GL_TEXTURE2D, so we do the same here:
uniform sampler2D toon_tex; 
uniform sampler3D sampler3D_perlin;

varying vec4 P;  // current position

void main() 
{
   

      
   vec4 Pos = P * 0.1 ; //scale down the texture coordinates
 
   
   // get the rgba of the toon texture
   vec4 tex = texture2D(toon_tex, gl_TexCoord[0].xy);
   
   //gets a set of perlin noise values at these coordinates

   vec4 marble = texture3D(sampler3D_perlin, Pos.xyz);
   
   float noise1 = marble.b-0.5;
   float noise2 = marble.g-0.5;
   float noise3 = marble.r-0.5;
   float noise4 = marble.a-0.5;
   

   //this produces solid black and white patches
   //looks best on the cow model
   
   //this test proves that the 3d noise tiles in all directions 
   //and is centered around 0.5 or more correctly 128 
   
   vec3 marble_col1;
   if (noise2<0.0)
   	marble_col1=vec3(0.0,0.0,0.0);
   else
   	marble_col1=vec3(0.9,0.9,0.9);
   	
   	
   
   //usefull stuff for other tests
   //vec3 marble_col2;
   //if (sin(noise2)>0.0)
    //	marble_col2=vec3(0.0,0.1,0.3);
   //else
   //	marble_col2=vec3(0.1,8.0,0.9);
   
   //vec3 marble_col  = mix(marble_col1,marble_col2,sin(noise3));

   // mix rgb with the base color, using the texture alpha:
   
   
   //final mix with the toon shading
   gl_FragColor = vec4(mix(marble_col1, tex.rgb, tex.a),1.0);

}

// toon.fp
