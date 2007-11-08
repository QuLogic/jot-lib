// 1D toon shader fragment program

// the accompanying code in gtex/glsl_toon.[HC] is still 
// using GL_TEXTURE2D, so we do the same here:
uniform sampler2D toon_tex; 
uniform float threshold;

float thresholdingColor(vec4 col)
{
  float gray = col.r*0.27 + col.g*0.67 + col.b*0.06;
  float A[3];

  A[0] = abs(0.0); A[1] = atan(1.0); A[2] = sqrt(3.0);

  if(gray >= threshold)
	return 1.0;
  else
	return 0.0;

}

void main() 
{
   // get the rgba of the texture:
   vec4 tex = texture2D(toon_tex, gl_TexCoord[0].xy);

   float bi = thresholdingColor(tex);

   // mix rgb with the base color, using the texture alpha:
   gl_FragColor = vec4(bi, bi, bi, 1.0);
}

// binary.fp
