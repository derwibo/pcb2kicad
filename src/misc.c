/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

static char *rcsid = "$Id$";

/* misc functions used by several modules
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <memory.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"

#include "crosshair.h"
#include "create.h"
#include "data.h"
#include "dialog.h"
#include "draw.h"
#include "file.h"
#include "gui.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "move.h"
#include "output.h"
#include "remove.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "set.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*	forward declarations	*/
static char *BumpName (char *);
static void RightAngles (int, float *, float *);
static void GetGridLockCoordinates (int, void *, void *, void *, Location *,
				    Location *);

/* ---------------------------------------------------------------------------
 * prints copyright information
 */
void
Copyright (void)
{
  printf ("\n"
	  "                COPYRIGHT for %s version %s\n\n"
	  "    PCB, interactive printed circuit board design\n"
	  "    Copyright (C) 1994,1995,1996,1997 Thomas Nau\n"
	  "    Copyright (C) 1998, 1999, 2000 Harry Eaton\n\n"
	  "    This program is free software; you can redistribute it and/or modify\n"
	  "    it under the terms of the GNU General Public License as published by\n"
	  "    the Free Software Foundation; either version 2 of the License, or\n"
	  "    (at your option) any later version.\n\n"
	  "    This program is distributed in the hope that it will be useful,\n"
	  "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	  "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	  "    GNU General Public License for more details.\n\n"
	  "    You should have received a copy of the GNU General Public License\n"
	  "    along with this program; if not, write to the Free Software\n"
	  "    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n",
	  Progname, VERSION);
  exit (0);
}

/* ---------------------------------------------------------------------------
 * prints usage message
 */
void
Usage (void)
{
  fprintf (stderr,
	   "\nUSAGE: %s [standard X options] [standard options] [layout]\n"
	   "or   : %s [standard X options] <exactly one special option>\n\n"
	   "standard options are:\n"
	   "  -alldirections:           enable 'all-direction' lines\n"
	   "  +alldirections:           force 45 degree lines\n"
	   "  +rubberband:              enable rubberband move and rotate\n"
	   "  -rubberband:              turn off rubberband move and rotate\n"
	   "  -backup <seconds>:        time between two backups\n"
	   "  -c <number>:              characters per output-line\n"
	   "  -fontfile <file>:         read default font from this file\n"
	   "  -lelement <command>:      command to copy element files to stdout,\n"
	   "                            %%f is set to the filename\n"
	   "                            %%p is set to the seachpath\n"
	   "  -lfile <command>:         command to copy layout files to stdout,\n"
	   "                            %%f is set to the filename\n"
	   "                            %%p is set to the seachpath\n"
	   "  -lfont <command>:         command to copy font files to stdout,\n"
	   "                            %%f is set to the filename\n"
	   "                            %%p is set to the seachpath\n"
	   "  -lg <layergroups>:        set layergroups of new layouts to this\n"
	   "  -libname <file>:          the name of the library\n"
	   "  -libpath <path>:          the library search-path\n"
	   "  -llib <command>:          command to copy elements from library to stdout,\n"
	   "                            %%a is set to 'template value package'\n"
	   "                            %%f is set to the filename\n"
	   "                            %%p is set to the searchpath\n"
	   "  -llibcont <command>:      command to list library contents,\n"
	   "                            %%f is set to the filename\n"
	   "                            %%p is set to the searchpath\n"
	   "  -loggeometry <geometry>:  set the geometry of the logging window\n"
	   "  -log:                     don't use the log window\n"
	   "  -pnl <value>:             maximum display length of pin names\n"
	   "  -pz <value>:              zoom factor for pinout windows\n"
	   "  -reset:                   reset connections after each element\n"
	   "  +reset:                   negation of '-reset'\n"
	   "  -ring:                    ring bell when connection lookup is done\n"
	   "  +ring:                    negation of '-r'\n"
	   "  -s:                       save the last command entered by user\n"
	   "  +s:                       negation of '-s'\n"
	   "  -save:                    always save data before it is lost\n"
	   "  +save:                    override data if required by command\n"
	   "  -sfile <command>:         command to copy stdin to layout file,\n"
	   "                            %%f is set to the filename\n"
	   "  -size <width>x<height>    size of a layout\n"
	   "  -v <value>:               sets the volume of the X speaker\n"
	   "special options are:\n"
	   "  -copyright:               prints copyright information\n"
	   "  -help:                    prints this message\n"
	   "  -version:                 prints the current version number\n",
	   Progname, Progname);
  exit (1);
}

/* Get Value returns a numeric value passed from the string and sets the
 * Boolean variable absolute to False if it leads with a +/- character
 */
float
GetValue (String * Params, Boolean * absolute, Cardinal Num)
{
  float value;
  /* if the first character is a sign we have to add the
   * value to the current one
   */
  if (**Params == '=')
    {
      *absolute = 1;
      value = atof ((*Params)+1);
    }
  else
    {
      if (isdigit (**Params))
        *absolute = True;
      else
        *absolute = False;
      value = atof (*Params);
    }
  if (Num == 3)
    {
      if (strncasecmp (*(Params + 1), "mm", 2) == 0)
	value *= MM_TO_COOR;
      else if (strncasecmp (*(Params + 1), "mil", 3) == 0)
	value *= 100;
    }
  return value;
}

/* ---------------------------------------------------------------------------
 * sets the bounding box of a polygons
 */
void
SetPolygonBoundingBox (PolygonTypePtr Polygon)
{
  Location minx, miny, maxx, maxy;

  minx = miny = MAX_COORD;
  maxx = maxy = 0;
  POLYGONPOINT_LOOP (Polygon, 
    {
      minx = MIN (minx, point->X);
      miny = MIN (miny, point->Y);
      maxx = MAX (maxx, point->X);
      maxy = MAX (maxy, point->Y);
    }
  );
  Polygon->BoundingBox.X1 = minx;
  Polygon->BoundingBox.Y1 = miny;
  Polygon->BoundingBox.X2 = maxx;
  Polygon->BoundingBox.Y2 = maxy;
}

/* ---------------------------------------------------------------------------
 * sets the bounding box of an elements
 */
void
SetElementBoundingBox (ElementTypePtr Element, FontTypePtr Font)
{
  Location minx, miny, maxx, maxy;
  float fminx, fminy, fmaxx, fmaxy;
  int angle1, angle2, angle;

  /* first update the text objects */
  ELEMENTTEXT_LOOP (Element, 
    {
      SetTextBoundingBox (Font, text);
    }
  );

  /* do not include the elementnames bounding box which
   * is handles seperatly
   */
  minx = miny = MAX_COORD;
  maxx = maxy = 0;
  ELEMENTLINE_LOOP (Element, 
    {
      minx = MIN (minx, line->Point1.X - line->Thickness / 2);
      miny = MIN (miny, line->Point1.Y - line->Thickness / 2);
      minx = MIN (minx, line->Point2.X - line->Thickness / 2);
      miny = MIN (miny, line->Point2.Y - line->Thickness / 2);
      maxx = MAX (maxx, line->Point1.X + line->Thickness / 2);
      maxy = MAX (maxy, line->Point1.Y + line->Thickness / 2);
      maxx = MAX (maxx, line->Point2.X + line->Thickness / 2);
      maxy = MAX (maxy, line->Point2.Y + line->Thickness / 2);
    }
  );
  PIN_LOOP (Element, 
    {
      minx = MIN (minx, pin->X - pin->Thickness / 2);
      miny = MIN (miny, pin->Y - pin->Thickness / 2);
      maxx = MAX (maxx, pin->X + pin->Thickness / 2);
      maxy = MAX (maxy, pin->Y + pin->Thickness / 2);
    }
  );
  ARC_LOOP (Element, 
    {
      /* arc->StartAngle is in [0,360], arc->Delta in [0,360] */
      angle1 = arc->StartAngle;
      angle2 = arc->StartAngle + arc->Delta;
      /* initialize limits */
      fminx =
	MIN (-cos (M180 * (float) angle1), -cos (M180 * (float) angle2));
      fmaxx =
	MAX (-cos (M180 * (float) angle1), -cos (M180 * (float) angle2));
      fminy = MIN (sin (M180 * (float) angle1), sin (M180 * (float) angle2));
      fmaxy = MAX (sin (M180 * (float) angle1), sin (M180 * (float) angle2));
      /* loop and check all angles n*180
       * with angle1 <= a <= angle2
       */
      for (angle = (angle1 / 180 + 1) * 180; angle < angle2; angle += 180)
	{
	  fminx = MIN (-cos (M180 * (float) angle), fminx);
	  fmaxx = MAX (-cos (M180 * (float) angle), fmaxx);
	}

      /* loop and check all angles n*180+90
       * with angle1 <= a <= angle2
       */
      for (angle = ((angle1 + 90) / 180) * 180 + 90; angle < angle2;
	   angle += 180)
	{
	  fminy = MIN (sin (M180 * (float) angle), fminy);
	  fmaxy = MAX (sin (M180 * (float) angle), fmaxy);
	}
      minx = MIN (minx, (int) (fminx * arc->Width) + arc->X);
      miny = MIN (miny, (int) (fminy * arc->Height) + arc->Y);
      maxx = MAX (maxx, (int) (fmaxx * arc->Width) + arc->X);
      maxy = MAX (maxy, (int) (fmaxy * arc->Height) + arc->Y);
    }
  );
  PAD_LOOP (Element, 
    {
      minx = MIN (minx, pad->Point1.X - pad->Thickness / 2);
      miny = MIN (miny, pad->Point1.Y - pad->Thickness / 2);
      minx = MIN (minx, pad->Point2.X - pad->Thickness / 2);
      miny = MIN (miny, pad->Point2.Y - pad->Thickness / 2);
      maxx = MAX (maxx, pad->Point1.X + pad->Thickness / 2);
      maxy = MAX (maxy, pad->Point1.Y + pad->Thickness / 2);
      maxx = MAX (maxx, pad->Point2.X + pad->Thickness / 2);
      maxy = MAX (maxy, pad->Point2.Y + pad->Thickness / 2);
    }
  );
  /* now we set the EDGE2FLAG of the pad if Point2
   * is closer to the outside edge than Point1
   */
  PAD_LOOP (Element, 
    {
      if (pad->Point1.Y == pad->Point2.Y)
	{
	  /* horizontal pad */
	  if (maxx - pad->Point2.X < pad->Point1.X - minx)
	    SET_FLAG (EDGE2FLAG, pad);
	  else
	    CLEAR_FLAG (EDGE2FLAG, pad);
	}
      else
	{
	  /* vertical pad */
	  if (maxy - pad->Point2.Y < pad->Point1.Y - miny)
	    SET_FLAG (EDGE2FLAG, pad);
	  else
	    CLEAR_FLAG (EDGE2FLAG, pad);
	}
    }
  );

  /* mark pins with component orientation */
  if ((maxx - minx) > (maxy - miny))
    {
      PIN_LOOP (Element, 
	{
	  SET_FLAG (EDGE2FLAG, pin);
	}
      );
    }
  else
    {
      PIN_LOOP (Element, 
	{
	  CLEAR_FLAG (EDGE2FLAG, pin);
	}
      );
    }

  Element->BoundingBox.X1 = minx;
  Element->BoundingBox.Y1 = miny;
  Element->BoundingBox.X2 = maxx;
  Element->BoundingBox.Y2 = maxy;
}

/* ---------------------------------------------------------------------------
 * creates the bounding box of a text object
 */
void
SetTextBoundingBox (FontTypePtr FontPtr, TextTypePtr Text)
{
  SymbolTypePtr symbol = FontPtr->Symbol;
  unsigned char *s = (unsigned char *) Text->TextString;
  Location width = 0, height = 0;

  /* calculate size of the bounding box */
  for (; s && *s; s++)
    if (*s <= MAX_FONTPOSITION && symbol[*s].Valid)
      {
	width += symbol[*s].Width + symbol[*s].Delta;
	height = MAX (height, (Location) symbol[*s].Height);
      }
    else
      {
	width +=
	  ((FontPtr->DefaultSymbol.X2 - FontPtr->DefaultSymbol.X1) * 6 / 5);
	height = (FontPtr->DefaultSymbol.Y2 - FontPtr->DefaultSymbol.Y1);
      }

  /* scale values and rotate them */
  width = width * Text->Scale / 100;
  height = height * Text->Scale / 100;

  /* set upper-left and lower-right corner;
   * swap coordinates if necessary (origin is already in 'swapped')
   * and rotate box
   */
  Text->BoundingBox.X1 = Text->X;
  Text->BoundingBox.Y1 = Text->Y;
  if (TEST_FLAG (ONSOLDERFLAG, Text))
    {
      Text->BoundingBox.X2 = Text->BoundingBox.X1 + SWAP_SIGN_X (width);
      Text->BoundingBox.Y2 = Text->BoundingBox.Y1 + SWAP_SIGN_Y (height);
      RotateBoxLowLevel (&Text->BoundingBox,
			 Text->X, Text->Y, (4 - Text->Direction) & 0x03);
    }
  else
    {
      Text->BoundingBox.X2 = Text->BoundingBox.X1 + width;
      Text->BoundingBox.Y2 = Text->BoundingBox.Y1 + height;
      RotateBoxLowLevel (&Text->BoundingBox,
			 Text->X, Text->Y, Text->Direction);
    }
}

/* ---------------------------------------------------------------------------
 * returns True if data area is empty
 */
Boolean
IsDataEmpty (DataTypePtr Data)
{
  Boolean hasNoObjects;
  Cardinal i;

  hasNoObjects = (Data->ViaN == 0);
  hasNoObjects &= (Data->ElementN == 0);
  for (i = 0; i < MAX_LAYER + 2; i++)
    hasNoObjects = hasNoObjects &&
      Data->Layer[i].LineN == 0 &&
      Data->Layer[i].ArcN == 0 &&
      Data->Layer[i].TextN == 0 && Data->Layer[i].PolygonN == 0;
  return (hasNoObjects);
}

/* ---------------------------------------------------------------------------
 * gets minimum and maximum coordinates
 * returns NULL if layout is empty
 */
BoxTypePtr
GetDataBoundingBox (DataTypePtr Data)
{
  static BoxType box;

  /* preset identifiers with highest and lowest possible values */
  box.X1 = box.Y1 = MAX_COORD;
  box.X2 = box.Y2 = -MAX_COORD;

  /* now scan for the lowest/highest X and Y coodinate */
  VIA_LOOP (Data, 
    {
      box.X1 = MIN (box.X1, via->X - via->Thickness / 2);
      box.Y1 = MIN (box.Y1, via->Y - via->Thickness / 2);
      box.X2 = MAX (box.X2, via->X + via->Thickness / 2);
      box.Y2 = MAX (box.Y2, via->Y + via->Thickness / 2);
    }
  );
  ELEMENT_LOOP (Data, 
    {
      box.X1 = MIN (box.X1, element->BoundingBox.X1);
      box.Y1 = MIN (box.Y1, element->BoundingBox.Y1);
      box.X2 = MAX (box.X2, element->BoundingBox.X2);
      box.Y2 = MAX (box.Y2, element->BoundingBox.Y2);
      {
	TextTypePtr text = &NAMEONPCB_TEXT (element);
	box.X1 = MIN (box.X1, text->BoundingBox.X1);
	box.Y1 = MIN (box.Y1, text->BoundingBox.Y1);
	box.X2 = MAX (box.X2, text->BoundingBox.X2);
	box.Y2 = MAX (box.Y2, text->BoundingBox.Y2);
      };
    }
  );
  ALLLINE_LOOP (Data, 
    {
      box.X1 = MIN (box.X1, line->Point1.X - line->Thickness / 2);
      box.Y1 = MIN (box.Y1, line->Point1.Y - line->Thickness / 2);
      box.X1 = MIN (box.X1, line->Point2.X - line->Thickness / 2);
      box.Y1 = MIN (box.Y1, line->Point2.Y - line->Thickness / 2);
      box.X2 = MAX (box.X2, line->Point1.X + line->Thickness / 2);
      box.Y2 = MAX (box.Y2, line->Point1.Y + line->Thickness / 2);
      box.X2 = MAX (box.X2, line->Point2.X + line->Thickness / 2);
      box.Y2 = MAX (box.Y2, line->Point2.Y + line->Thickness / 2);
    }
  );
  ALLARC_LOOP (Data, 
    {
      box.X1 = MIN (box.X1, arc->BoundingBox.X1);
      box.Y1 = MIN (box.Y1, arc->BoundingBox.Y1);
      box.X2 = MAX (box.X2, arc->BoundingBox.X2);
      box.Y2 = MAX (box.Y2, arc->BoundingBox.Y2);
    }
  );
  ALLTEXT_LOOP (Data, 
    {
      box.X1 = MIN (box.X1, text->BoundingBox.X1);
      box.Y1 = MIN (box.Y1, text->BoundingBox.Y1);
      box.X2 = MAX (box.X2, text->BoundingBox.X2);
      box.Y2 = MAX (box.Y2, text->BoundingBox.Y2);
    }
  );
  ALLPOLYGON_LOOP (Data, 
    {
      box.X1 = MIN (box.X1, polygon->BoundingBox.X1);
      box.Y1 = MIN (box.Y1, polygon->BoundingBox.Y1);
      box.X2 = MAX (box.X2, polygon->BoundingBox.X2);
      box.Y2 = MAX (box.Y2, polygon->BoundingBox.Y2);
    }
  );
  return (IsDataEmpty (Data) ? NULL : &box);
}

/* ---------------------------------------------------------------------------
 * centers the displayed PCB around the specified point (X,Y)
 * if Delta is false, X,Y are in absolute PCB coordinates
 * if Delta is true, simply move the center by an amount X, Y in screen
 * coordinates
 */
void
CenterDisplay (Location X, Location Y, Boolean Delta)
{
  Location x, y;

#ifdef DEBUGDISP
  Message ("CenterDisplay(%d, %d, %s)\n", X, Y, Delta ? "True" : "False");
#endif
  if (!Delta)
    {
      x = X - TO_PCB (Output.Width / 2);
      if (SWAP_IDENT)
        y = PCB->MaxHeight - Y - TO_PCB(Output.Height /2 );
      else
        y = Y - TO_PCB (Output.Height / 2);
    }
  else
    {
      x = Xorig + TO_PCB(X);
      y = Yorig + TO_PCB(Y);
    }
  Pan (x, y, True, True);
}

/* ---------------------------------------------------------------------------
 * transforms symbol coordinates so that the left edge of each symbol
 * is at the zero position. The y coordinates are moved so that min(y) = 0
 * 
 */
void
SetFontInfo (FontTypePtr Ptr)
{
  Cardinal i, j;
  SymbolTypePtr symbol;
  LineTypePtr line;
  Location totalminy = MAX_COORD;

  /* calculate cell with and height (is at least DEFAULT_CELLSIZE)
   * maximum cell width and height
   * minimum x and y position of all lines
   */
  Ptr->MaxWidth = DEFAULT_CELLSIZE;
  Ptr->MaxHeight = DEFAULT_CELLSIZE;
  for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
    {
      Location minx, miny, maxx, maxy;

      /* next one if the index isn't used or symbol is empty (SPACE) */
      if (!symbol->Valid || !symbol->LineN)
	continue;

      minx = miny = MAX_COORD;
      maxx = maxy = 0;
      for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
	{
	  minx = MIN (minx, line->Point1.X);
	  miny = MIN (miny, line->Point1.Y);
	  minx = MIN (minx, line->Point2.X);
	  miny = MIN (miny, line->Point2.Y);
	  maxx = MAX (maxx, line->Point1.X);
	  maxy = MAX (maxy, line->Point1.Y);
	  maxx = MAX (maxx, line->Point2.X);
	  maxy = MAX (maxy, line->Point2.Y);
	}

      /* move symbol to left edge */
      for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
	MOVE_LINE_LOWLEVEL (line, -minx, 0);

      /* set symbol bounding box with a minimum cell size of (1,1) */
      symbol->Width = maxx - minx + 1;
      symbol->Height = maxy + 1;

      /* check total min/max  */
      Ptr->MaxWidth = MAX (Ptr->MaxWidth, symbol->Width);
      Ptr->MaxHeight = MAX (Ptr->MaxHeight, symbol->Height);
      totalminy = MIN (totalminy, miny);
    }

  /* move coordinate system to the upper edge (lowest y on screen) */
  for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
    if (symbol->Valid)
      {
	symbol->Height -= totalminy;
	for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
	  MOVE_LINE_LOWLEVEL (line, 0, -totalminy);
      }

  /* setup the box for the default symbol */
  Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
  Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
  Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}

static void
GetNum (char **s, BDimension * num)
{
  *num = atoi (*s);
  while (isdigit (**s))
    (*s)++;
}


/* ----------------------------------------------------------------------
 * parses the routes definition string which is a colon seperated list of
 * comma seperated Name, Dimension, Dimension, Dimension, Dimension
 * e.g. Signal,20,40,20,10:Power,40,60,28,10:...
 */
int
ParseRouteString (char *s, RouteStyleTypePtr routeStyle, int scale)
{
  int i, style;
  char Name[256];

  memset (routeStyle, 0, NUM_STYLES * sizeof (RouteStyleType));
  for (style = 0; style < NUM_STYLES; style++, routeStyle++)
    {
      while (*s && isspace (*s))
	s++;
      for (i = 0; *s && *s != ','; i++)
	Name[i] = *s++;
      Name[i] = '\0';
      routeStyle->Name = MyStrdup (Name, "ParseRouteString()");
      if (!isdigit (*++s))
	goto error;
      GetNum (&s, &routeStyle->Thick);
      routeStyle->Thick *= scale;
      while (*s && isspace (*s))
	s++;
      if (*s++ != ',')
	goto error;
      while (*s && isspace (*s))
	s++;
      if (!isdigit (*s))
	goto error;
      GetNum (&s, &routeStyle->Diameter);
      routeStyle->Diameter *= scale;
      while (*s && isspace (*s))
	s++;
      if (*s++ != ',')
	goto error;
      while (*s && isspace (*s))
	s++;
      if (!isdigit (*s))
	goto error;
      GetNum (&s, &routeStyle->Hole);
      routeStyle->Hole *= scale;
      /* for backwards-compatibilty, we use a 10-mil default
       * for styles which omit the keepaway specification. */
      if (*s != ',')
	routeStyle->Keepaway = 1000;
      else
	{
	  s++;
	  while (*s && isspace (*s))
	    s++;
	  if (!isdigit (*s))
	    goto error;
	  GetNum (&s, &routeStyle->Keepaway);
	  routeStyle->Keepaway *= scale;
	  while (*s && isspace (*s))
	    s++;
	}
      if (style < NUM_STYLES - 1)
	{
	  while (*s && isspace (*s))
	    s++;
	  if (*s++ != ':')
	    goto error;
	}
    }
  return (0);
error:
  memset (routeStyle, 0, NUM_STYLES * sizeof (RouteStyleType));
  return (1);
}

/* ----------------------------------------------------------------------
 * parses the group definition string which is a colon seperated list of
 * comma seperated layer numbers (1,2,b:4,6,8,t)
 */
int
ParseGroupString (char *s, LayerGroupTypePtr LayerGroup)
{
  int group, member, layer;
  Boolean c_set = False,	/* flags for the two special layers to */
    s_set = False;		/* provide a default setting for old formats */

  /* clear struct */
  memset (LayerGroup, 0, sizeof (LayerGroupType));

  /* loop over all groups */
  for (group = 0; s && *s && group < MAX_LAYER; group++)
    {
      while (*s && isspace (*s))
	s++;

      /* loop over all group members */
      for (member = 0; *s; s++)
	{
	  /* ignore white spaces and get layernumber */
	  while (*s && isspace (*s))
	    s++;
	  switch (*s)
	    {
	    case 'c':
	    case 'C':
	      layer = MAX_LAYER + COMPONENT_LAYER;
	      c_set = True;
	      break;

	    case 's':
	    case 'S':
	      layer = MAX_LAYER + SOLDER_LAYER;
	      s_set = True;
	      break;

	    default:
	      if (!isdigit (*s))
		goto error;
	      layer = atoi (s) - 1;
	      break;
	    }
	  if (layer > MAX_LAYER + MAX (SOLDER_LAYER, COMPONENT_LAYER) ||
	      member >= MAX_LAYER + 1)
	    goto error;
	  LayerGroup->Entries[group][member++] = layer;
	  while (*++s && isdigit (*s));

	  /* ignore white spaces and check for seperator */
	  while (*s && isspace (*s))
	    s++;
	  if (!*s || *s == ':')
	    break;
	  if (*s != ',')
	    goto error;
	}
      LayerGroup->Number[group] = member;
      if (*s == ':')
	s++;
    }
  if (!s_set)
    LayerGroup->Entries[SOLDER_LAYER][LayerGroup->Number[SOLDER_LAYER]++] =
      MAX_LAYER + SOLDER_LAYER;
  if (!c_set)
    LayerGroup->
      Entries[COMPONENT_LAYER][LayerGroup->Number[COMPONENT_LAYER]++] =
      MAX_LAYER + COMPONENT_LAYER;
  return (0);

  /* reset structure on error */
error:
  memset (LayerGroup, 0, sizeof (LayerGroupType));
  return (1);
}

/* ---------------------------------------------------------------------------
 * quits application
 */
void
QuitApplication (void)
{
  /* save data if necessary */
  if (PCB->Changed && Settings.SaveInTMP)
    EmergencySave ();
  exit (0);
}

/* ---------------------------------------------------------------------------
 * creates a filename from a template
 * %f is replaced by the filename 
 * %p by the searchpath
 */
char *
EvaluateFilename (char *Template, char *Path, char *Filename, char *Parameter)
{
  static DynamicStringType command;
  char *p;

  DSClearString (&command);
  for (p = Template; *p; p++)
    {
      /* copy character or add string to command */
      if (*p == '%'
	  && (*(p + 1) == 'f' || *(p + 1) == 'p' || *(p + 1) == 'a'))
	switch (*(++p))
	  {
	  case 'a':
	    DSAddString (&command, Parameter);
	    break;
	  case 'f':
	    DSAddString (&command, Filename);
	    break;
	  case 'p':
	    DSAddString (&command, Path);
	    break;
	  }
      else
	DSAddCharacter (&command, *p);
    }
  DSAddCharacter (&command, '\0');
  return (MyStrdup (command.Data, "EvaluateFilename()"));
}

/* ---------------------------------------------------------------------------
 * concatenates directory and filename if directory != NULL,
 * expands them with a shell and returns the found name(s) or NULL
 */
char *
ExpandFilename (char *Dirname, char *Filename)
{
  static DynamicStringType answer;
  char *command;
  FILE *pipe;
  int c;

  /* allocate memory for commandline and build it */
  DSClearString (&answer);
  if (Dirname)
    {
      command = MyCalloc (strlen (Filename) + strlen (Dirname) + 7,
			  sizeof (char), "ExpandFilename()");
      sprintf (command, "echo %s/%s", Dirname, Filename);
    }
  else
    {
      command = MyCalloc (strlen (Filename) + 6, sizeof (char), "Expand()");
      sprintf (command, "echo %s", Filename);
    }

  /* execute it with shell */
  if ((pipe = popen (command, "r")) != NULL)
    {
      /* discard all but the first returned line */
      for (;;)
	{
	  if ((c = fgetc (pipe)) == EOF || c == '\n' || c == '\r')
	    break;
	  else
	    DSAddCharacter (&answer, c);
	}

      SaveFree (command);
      return (pclose (pipe) ? NULL : answer.Data);
    }

  /* couldn't be expanded by the shell */
  PopenErrorMessage (command);
  SaveFree (command);
  return (NULL);
}

/* ----------------------------------------------------------------------
 * releases pixmap used to draw output data
 */
void
ReleaseOffscreenPixmap (void)
{
  render = True;
  if (VALID_PIXMAP (Offscreen))
    XFreePixmap (Dpy, Offscreen);

  /* mark pixmap unusable */
  Offscreen = BadAlloc;
}

/* ---------------------------------------------------------------------------
 * returns the layer number for the passed pointer
 */
int
GetLayerNumber (DataTypePtr Data, LayerTypePtr Layer)
{
  int i;

  for (i = 0; i < MAX_LAYER + 2; i++)
    if (Layer == &Data->Layer[i])
      break;
  return (i);
}

/* ---------------------------------------------------------------------------
 * returns the layergroup number for the passed pointer
 */
int
GetLayerGroupNumberByPointer (LayerTypePtr Layer)
{
  return (GetLayerGroupNumberByNumber (GetLayerNumber (PCB->Data, Layer)));
}

/* ---------------------------------------------------------------------------
 * returns the layergroup number for the passed layernumber
 */
int
GetLayerGroupNumberByNumber (Cardinal Layer)
{
  int group, entry;

  for (group = 0; group < MAX_LAYER; group++)
    for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
      if (PCB->LayerGroups.Entries[group][entry] == Layer)
	return (group);

  /* since every layer belongs to a group it is safe to return
   * the value without boundary checking
   */
  return (group);
}

/* ---------------------------------------------------------------------------
 * returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
BoxTypePtr
GetObjectBoundingBox (int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  static BoxType box;

  switch (Type)
    {
    case VIA_TYPE:
      {
	PinTypePtr via = (PinTypePtr) Ptr1;

	box.X1 = via->X - via->Thickness / 2;
	box.Y1 = via->Y - via->Thickness / 2;
	box.X2 = via->X + via->Thickness / 2;
	box.Y2 = via->Y + via->Thickness / 2;
	break;
      }

    case LINE_TYPE:
      {
	LineTypePtr line = (LineTypePtr) Ptr2;

	box.X1 = MIN (line->Point1.X, line->Point2.X);
	box.Y1 = MIN (line->Point1.Y, line->Point2.Y);
	box.X2 = MAX (line->Point1.X, line->Point2.X);
	box.Y2 = MAX (line->Point1.Y, line->Point2.Y);
	box.X1 -= line->Thickness / 2;
	box.Y1 -= line->Thickness / 2;
	box.X2 += line->Thickness / 2;
	box.Y2 += line->Thickness / 2;
	break;
      }

    case ARC_TYPE:
      box = ((ArcTypePtr) Ptr2)->BoundingBox;
      break;

    case TEXT_TYPE:
    case ELEMENTNAME_TYPE:
      box = ((TextTypePtr) Ptr2)->BoundingBox;
      break;

    case POLYGON_TYPE:
      box = ((PolygonTypePtr) Ptr2)->BoundingBox;
      break;

    case ELEMENT_TYPE:
      box = ((ElementTypePtr) Ptr1)->BoundingBox;
      {
	TextTypePtr text = &NAMEONPCB_TEXT ((ElementTypePtr) Ptr1);
	if (text->BoundingBox.X1 < box.X1)
	  box.X1 = text->BoundingBox.X1;
	if (text->BoundingBox.Y1 < box.Y1)
	  box.Y1 = text->BoundingBox.Y1;
	if (text->BoundingBox.X2 > box.X2)
	  box.X2 = text->BoundingBox.X2;
	if (text->BoundingBox.Y2 > box.Y2)
	  box.Y2 = text->BoundingBox.Y2;
      }
      break;

    case PAD_TYPE:
      {
	PadTypePtr pad = (PadTypePtr) Ptr2;

	box.X1 = MIN (pad->Point1.X, pad->Point2.X);
	box.Y1 = MIN (pad->Point1.Y, pad->Point2.Y);
	box.X2 = MAX (pad->Point1.X, pad->Point2.X);
	box.Y2 = MAX (pad->Point1.Y, pad->Point2.Y);
	box.X1 -= pad->Thickness;
	box.Y1 -= pad->Thickness;
	box.Y2 += pad->Thickness;
	box.X2 += pad->Thickness;
	break;
      }

    case PIN_TYPE:
      {
	PinTypePtr pin = (PinTypePtr) Ptr2;

	box.X1 = pin->X - pin->Thickness / 2;
	box.Y1 = pin->Y - pin->Thickness / 2;
	box.X2 = pin->X + pin->Thickness / 2;
	box.Y2 = pin->Y + pin->Thickness / 2;
	break;
      }

    case LINEPOINT_TYPE:
      {
	PointTypePtr point = (PointTypePtr) Ptr3;

	box.X1 = box.X2 = point->X;
	box.Y1 = box.Y2 = point->Y;
	break;
      }

    case POLYGONPOINT_TYPE:
      {
	PointTypePtr point = (PointTypePtr) Ptr3;

	box.X1 = box.X2 = point->X;
	box.Y1 = box.Y2 = point->Y;
	break;
      }

    default:
      Message ("Request for bounding box of unsupported typ %d\n", Type);
      box.X1 = box.X2 = box.Y1 = box.Y2 = 0;
      break;
    }
  return (&box);
}

/* ---------------------------------------------------------------------------
 * computes the bounding box of an arc
 */
void
SetArcBoundingBox (ArcTypePtr Arc)
{
  BoxTypePtr box;
  register Location temp, width;

  box = GetArcEnds (Arc);
  temp = box->X1;
  box->X1 = MIN (box->X1, box->X2);
  box->X2 = MAX (box->X2, temp);
  temp = box->Y1;
  box->Y1 = MIN (box->Y1, box->Y2);
  box->Y2 = MAX (box->Y2, temp);
  width = Arc->Thickness / 2;
  box->X1 -= width;
  box->X2 += width;
  box->Y1 -= width;
  box->Y2 += width;
  Arc->BoundingBox = *box;
}

/* ---------------------------------------------------------------------------
 * resets the layerstack setting
 */
void
ResetStackAndVisibility (void)
{
  Cardinal i;

  for (i = 0; i < MAX_LAYER + 2; i++)
    {
      if (i < MAX_LAYER)
	LayerStack[i] = i;
      PCB->Data->Layer[i].On = True;
    }
  PCB->ElementOn = True;
  PCB->InvisibleObjectsOn = True;
  PCB->PinOn = True;
  PCB->ViaOn = True;
  PCB->RatOn = True;
}

/* ----------------------------------------------------------------------
 * returns pointer to current working directory.  If 'path' is not
 * NULL, then the current working directory is copied to the array
 * pointed to by 'path'
 */
char *
GetWorkingDirectory (char *path)
{
#if defined(SYSV) || defined(linux) || defined(__NetBSD__)
  return getcwd (path, MAXPATHLEN);
#else
  /* seems that some BSD releases lack of a prototype for getwd() */
  return getwd (path);
#endif

}

/* ---------------------------------------------------------------------------
 * writes a string to the passed file pointer
 * some special characters are quoted
 */
void
CreateQuotedString (DynamicStringTypePtr DS, char *S)
{
  DSClearString (DS);
  DSAddCharacter (DS, '"');
  while (*S)
    {
      if (*S == '"' || *S == '\\')
	DSAddCharacter (DS, '\\');
      DSAddCharacter (DS, *S++);
    }
  DSAddCharacter (DS, '"');
}

/* ---------------------------------------------------------------------------
 * returns the current possible grid factor or zero
 */
int
GetGridFactor (void)
{
  static int factor[] = { 1, 2, 5, 10 };
  int i, delta;

  /* check to see if we have to draw every point,
   * every 2nd, 5th or 10th
   */
  for (i = 0; i < ENTRIES (factor); i++)
    {
      delta = PCB->Grid * factor[i];
      if (TO_SCREEN (delta) >= MIN_GRID_DISTANCE)
	{
	  if (Settings.GridFactor != factor[i])
	    {
	      Settings.GridFactor = factor[i];
	      SetStatusLine ();
	    }
	  return (factor[i]);
	}
    }
  Settings.GridFactor = 0;
  SetStatusLine ();
  return (0);
}

static void
RightAngles (int Angle, float *cosa, float *sina)
{
  *cosa = (float) cos ((double) Angle * M180);
  *sina = (float) sin ((double) Angle * M180);
}

BoxTypePtr
GetArcEnds (ArcTypePtr Arc)
{
  static BoxType box;
  float ca, sa;

  RightAngles (Arc->StartAngle, &ca, &sa);
  box.X1 = Arc->X - Arc->Width * ca;
  box.Y1 = Arc->Y + Arc->Width * sa;
  RightAngles (Arc->StartAngle + Arc->Delta, &ca, &sa);
  box.X2 = Arc->X - Arc->Width * ca;
  box.Y2 = Arc->Y + Arc->Width * sa;
  return (&box);
}

static char *
BumpName (char *Name)
{
  int num;
  char c, *start;
  static char temp[256];

  start = Name;
  /* seek end of string */
  while (*Name != 0)
    Name++;
  /* back up to potential number */
  for (Name--; isdigit (*Name); Name--);
  Name++;
  if (*Name)
    num = atoi (Name) + 1;
  else
    num = 1;
  c = *Name;
  *Name = 0;
  sprintf (temp, "%s%d", start, num);
  /* if this is not our string, put back the blown character */
  if (start != temp)
    *Name = c;
  return (temp);
}

/*
 * make a unique name for the name on board 
 * this can alter the contents of the input string
 */
char *
UniqueElementName (DataTypePtr Data, char *Name)
{
  Boolean unique = True;
  /* null strings are ok */
  if (!Name || !*Name)
    return (Name);

  for (;;)
    {
      ELEMENT_LOOP (Data, 
	{
	  if (NAMEONPCB_NAME (element) &&
	      strcmp (NAMEONPCB_NAME (element), Name) == 0)
	    {
	      Name = BumpName (Name);
	      unique = False;
	      break;
	    }
	}
      );
      if (unique)
	return (Name);
      unique = True;
    }
}

static void
GetGridLockCoordinates (int type, void *ptr1,
			void *ptr2, void *ptr3, Location * x, Location * y)
{
  switch (type)
    {
    case VIA_TYPE:
      *x = ((PinTypePtr) ptr2)->X;
      *y = ((PinTypePtr) ptr2)->Y;
      break;
    case LINE_TYPE:
      *x = ((LineTypePtr) ptr2)->Point1.X;
      *y = ((LineTypePtr) ptr2)->Point1.Y;
      break;
    case TEXT_TYPE:
    case ELEMENTNAME_TYPE:
      *x = ((TextTypePtr) ptr2)->X;
      *y = ((TextTypePtr) ptr2)->Y;
      break;
    case ELEMENT_TYPE:
      *x = ((ElementTypePtr) ptr2)->MarkX;
      *y = ((ElementTypePtr) ptr2)->MarkY;
      break;
    case POLYGON_TYPE:
      *x = ((PolygonTypePtr) ptr2)->Points[0].X;
      *y = ((PolygonTypePtr) ptr2)->Points[0].Y;
      break;

    case LINEPOINT_TYPE:
    case POLYGONPOINT_TYPE:
      *x = ((PointTypePtr) ptr3)->X;
      *y = ((PointTypePtr) ptr3)->Y;
      break;
    case ARC_TYPE:
      {
	BoxTypePtr box;

	box = GetArcEnds ((ArcTypePtr) ptr2);
	*x = box->X1;
	*y = box->Y1;
	break;
      }
    }
}

void
AttachForCopy (Location PlaceX, Location PlaceY)
{
  BoxTypePtr box;
  Location mx, my;

  Crosshair.AttachedObject.RubberbandN = 0;
  /* dither the grab point so that the mark, center, etc
   * will end up on a grid coordinate
   */
  GetGridLockCoordinates (Crosshair.AttachedObject.Type,
			  Crosshair.AttachedObject.Ptr1,
			  Crosshair.AttachedObject.Ptr2,
			  Crosshair.AttachedObject.Ptr3, &mx, &my);
  mx = GRIDFIT_X (mx, PCB->Grid) - mx;
  my = GRIDFIT_Y (my, PCB->Grid) - my;
  Crosshair.AttachedObject.X = PlaceX - mx;
  Crosshair.AttachedObject.Y = PlaceY - my;
  SetLocalRef (PlaceX - mx, PlaceY - my, True);
  Crosshair.AttachedObject.State = STATE_SECOND;

  /* get boundingbox of object and set cursor range */
  box = GetObjectBoundingBox (Crosshair.AttachedObject.Type,
			      Crosshair.AttachedObject.Ptr1,
			      Crosshair.AttachedObject.Ptr2,
			      Crosshair.AttachedObject.Ptr3);
  SetCrosshairRange (Crosshair.AttachedObject.X - box->X1,
		     Crosshair.AttachedObject.Y - box->Y1,
		     PCB->MaxWidth - (box->X2 - Crosshair.AttachedObject.X),
		     PCB->MaxHeight - (box->Y2 - Crosshair.AttachedObject.Y));

  /* get all attached objects if necessary */
  if ((Settings.Mode != COPY_MODE) && TEST_FLAG (RUBBERBANDFLAG, PCB))
    LookupRubberbandLines (Crosshair.AttachedObject.Type,
			   Crosshair.AttachedObject.Ptr1,
			   Crosshair.AttachedObject.Ptr2,
			   Crosshair.AttachedObject.Ptr3);
  if (Settings.Mode != COPY_MODE &&
      (Crosshair.AttachedObject.Type == ELEMENT_TYPE ||
       Crosshair.AttachedObject.Type == VIA_TYPE ||
       Crosshair.AttachedObject.Type == LINE_TYPE ||
       Crosshair.AttachedObject.Type == LINEPOINT_TYPE))
    LookupRatLines (Crosshair.AttachedObject.Type,
		    Crosshair.AttachedObject.Ptr1,
		    Crosshair.AttachedObject.Ptr2,
		    Crosshair.AttachedObject.Ptr3);
}
