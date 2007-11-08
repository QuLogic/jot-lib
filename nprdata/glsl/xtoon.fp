//xtoon.fp

uniform sampler2D toonTex;
varying vec2 coord;        //coord to access for color


void main (void)
{
      gl_FragColor = texture2D(toonTex,coord);
      
}