#ifndef DRAW_H
#define DRAW_H

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <GLUT/glut.h>

#include "utils.h"

int gWidth;
int gHeight;
int gBpp;

byte* gPixels;

void dHandle()
{
    GLenum format;

    // figure out data format
    // TODO: make this smarter
    if      (gBpp == 1)
        format = GL_LUMINANCE;
    else if (gBpp == 2)
        format = GL_LUMINANCE_ALPHA;
    else if (gBpp == 3)
        format = GL_RGB;
    else if (gBpp == 4)
        format = GL_RGBA;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glRasterPos2f(-1,1);
    glPixelZoom( 1, -1 );

    glDrawPixels(gWidth, gHeight, format, GL_UNSIGNED_BYTE, gPixels);

    glutSwapBuffers();
}

#endif