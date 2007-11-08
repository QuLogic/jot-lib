varying vec3 normal;

void main() 
{
   vec4 color;
   vec3 n = normalize(normal);

   color.rgb = (n + 1.0) / 2.0;
   color.a = 1.0;

   gl_FragDepth = gl_FragCoord.z;
   gl_FragColor = color;
}

// normal.fp
