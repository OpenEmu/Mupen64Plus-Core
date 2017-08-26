/* Draw text on screen.
 * Requires freetype library.
 * Code is taken from "OpenGL source examples from the OpenGL Programming wikibook:
 * http://en.wikibooks.org/wiki/OpenGL_Programming"
 */

#include "TextDrawer.h"

using namespace graphics;

// Maximum texture width
#define MAXWIDTH 1024

TextDrawer g_textDrawer;

/**
 * The atlas struct holds a texture that contains the visible US-ASCII characters
 * of a certain font rendered with a certain character height.
 * It also contains an array that contains all the information necessary to
 * generate the appropriate vertex and texture coordinates for each character.
 *
 * After the constructor is run, you don't need to use any FreeType functions anymore.
 */
struct Atlas {
};

void TextDrawer::init()
{
}

void TextDrawer::destroy()
{
}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 */
void TextDrawer::drawText(const char *_pText, float _x, float _y) const
{
}

void TextDrawer::getTextSize(const char *_pText, float & _w, float & _h) const
{
}

void TextDrawer::setTextColor(float * _color)
{
}
