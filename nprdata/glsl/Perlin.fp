// 1D toon shader fragment program

// the accompanying code in gtex/glsl_toon.[HC] is still 
// using GL_TEXTURE2D, so we do the same here:
uniform sampler2D toon_tex; 

void main() 
{
   // get the rgba of the texture:
   vec4 tex = texture2D(toon_tex, gl_TexCoord[0].xy);

   // mix rgb with the base color, using the texture alpha:
   gl_FragColor = vec4(mix(gl_Color.rgb, tex.rgb, tex.a),1.0);
}

// toon.fp
