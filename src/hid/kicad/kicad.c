/*!
 * \file src/hid/kicad/kicad.c
 *
 * \brief Exports a KiCad pcb file.
 *
 * \copyright (C) 2024 Michael Martens
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2020 PCB Contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 * <hr>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "rats.h"
#include "find.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid/common/hidnogui.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static HID_Attribute kicad_options[] = {
/* %start-doc options "82 KiCad Creation"
@ftable @code
@item --kicad_file <string>
Name of the KiCad output file.
Parameter @code{<string>} can include a path.
@end ftable
%end-doc
*/
  {"kicadfile", "Name of the KiCad output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_kicad_file 0
};

#define NUM_OPTIONS (sizeof(kicad_options)/sizeof(kicad_options[0]))

static HID_Attr_Val kicad_values[NUM_OPTIONS];

static const char *kicad_filename;

/*!
 * \brief Get export options.
 */
static HID_Attribute *
kicad_get_export_options (int *n)
{
	static char *last_kicad_filename = 0;

	if (PCB)
		derive_default_filename (PCB->Filename, &kicad_options[HA_kicad_file], ".kicad_pcb", &last_kicad_filename);

//  if (!kicad_options[HA_attrs].default_val.str_value)
//    kicad_options[HA_attrs].default_val.str_value = strdup("attribs");

	if (n)
		*n = NUM_OPTIONS;

	return kicad_options;
}


static char*
kicad_uuid(char* uuid)
{
	const char* uuidchars = "0123456789abcdef";
	for(int i=0; i<8; i++)
		uuid[i] = uuidchars[rand()%16];
	uuid[8] = '-';
	for(int i=9; i<13; i++)
		uuid[i] = uuidchars[rand()%16];
	uuid[13] = '-';
	for(int i=14; i<18; i++)
		uuid[i] = uuidchars[rand()%16];
	uuid[18] = '-';
	for(int i=19; i<23; i++)
		uuid[i] = uuidchars[rand()%16];
	uuid[23] = '-';
	for(int i=24; i<36; i++)
		uuid[i] = uuidchars[rand()%16];
	uuid[36] = '\0';
	return uuid;
}

typedef struct
{
	int net_id;
	char net_name[16];
} net_descriptor;

typedef struct
{
	void* item;
	net_descriptor* net;
} net_assignment;

net_assignment* kicad_net_assign;
int kicad_num_net_assign;
int kicad_max_net_assign;

void kicad_add_net_assign(void* item, net_descriptor* net)
{
	if(kicad_num_net_assign >= kicad_max_net_assign)
	{
		const int grow = 1000;
		net_assignment* tmp = kicad_net_assign;
		kicad_net_assign = calloc(kicad_max_net_assign + grow, sizeof(net_assignment));
		memcpy(kicad_net_assign, tmp, kicad_max_net_assign * sizeof(net_assignment));
		kicad_max_net_assign += grow;
		free(tmp);
	}
	kicad_net_assign[kicad_num_net_assign].item = item;
	kicad_net_assign[kicad_num_net_assign].net  = net;
	kicad_num_net_assign++;
}


void kicad_add_net(int type, void* ptr, net_descriptor* net)
{
	ClearFlagOnAllObjects(FOUNDFLAG, true);
	LookupConnectionByPin(type, ptr);

	ELEMENT_LOOP (PCB->Data);
	{
		PAD_LOOP (element);
		{
			if (!TEST_FLAG (VISITFLAG, pad))
			{
				if (TEST_FLAG (FOUNDFLAG, pad))
				{
					kicad_add_net_assign(pad, net);
					SET_FLAG (VISITFLAG, pad);
				}
			}
		}
		END_LOOP; /* Pad. */
		PIN_LOOP (element);
		{
			if (!TEST_FLAG (VISITFLAG, pin))
			{
				if (TEST_FLAG (FOUNDFLAG, pin))
				{
					kicad_add_net_assign(pin, net);
					SET_FLAG (VISITFLAG, pin);
				}
			}
		}
		END_LOOP; /* Pin. */
	}
	END_LOOP; /* Element */

	VIA_LOOP (PCB->Data);
	{
		if (!TEST_FLAG (VISITFLAG, via))
		{
			if (TEST_FLAG (FOUNDFLAG, via))
			{
				kicad_add_net_assign(via, net);
				SET_FLAG (VISITFLAG, via);
			}
		}
	}
  END_LOOP; /* Via. */


	LAYER_LOOP(PCB->Data, max_copper_layer)
	{
		LINE_LOOP(layer)
		{
			if (!TEST_FLAG (VISITFLAG, line))
			{
				if (TEST_FLAG (FOUNDFLAG, line))
				{
					kicad_add_net_assign(line, net);
					SET_FLAG (VISITFLAG, line);
				}
			}
		}
		END_LOOP;

		ARC_LOOP(layer)
		{
			if (!TEST_FLAG (VISITFLAG, arc))
			{
				if (TEST_FLAG (FOUNDFLAG, arc))
				{
					kicad_add_net_assign(arc, net);
					SET_FLAG (VISITFLAG, arc);
				}
			}
		}
		END_LOOP;

		POLYGON_LOOP(layer)
		{
			if (!TEST_FLAG (VISITFLAG, polygon))
			{
				if (TEST_FLAG (FOUNDFLAG, polygon))
				{
					kicad_add_net_assign(polygon, net);
					SET_FLAG (VISITFLAG, polygon);
				}
			}
		}
		END_LOOP;
	}
	END_LOOP;

}

net_descriptor* kicad_get_net_assign(void* item)
{
	int i;
	for(i=0; i<kicad_num_net_assign; i++)
	{
		if(item == kicad_net_assign[i].item)
		{
			return kicad_net_assign[i].net;
		}
	}
	return 0;
}

/* Adopted from bom.c */
static double
xyToAngle (double x, double y, bool morethan2pins)
{
  double d = atan2 (-y, x) * 180.0 / M_PI;

  /* IPC 7351 defines different rules for 2 pin elements */
  if (morethan2pins)
    {
      /* Multi pin case:
       * Output 0 degrees if pin1 in is top left or top, i.e. between angles of
       * 80 to 170 degrees.
       * Pin #1 can be at dead top (e.g. certain PLCCs) or anywhere in the top
       * left.
       */
      if (d < -100)
        return 90; /* -180 to -100 */
      else if (d < -10)
        return 180; /* -100 to -10 */
      else if (d < 80)
        return 270; /* -10 to 80 */
      else if (d < 170)
        return 0; /* 80 to 170 */
      else
        return 90; /* 170 to 180 */
    }
  else
    {
      /* 2 pin element:
       * Output 0 degrees if pin #1 is in top left or left, i.e. in sector
       * between angles of 95 and 185 degrees.
       */
      if (d < -175)
        return 0; /* -180 to -175 */
      else if (d < -85)
        return 90; /* -175 to -85 */
      else if (d < 5)
        return 180; /* -85 to 5 */
      else if (d < 95)
        return 270; /* 5 to 95 */
      else
        return 0; /* 95 to 180 */
    }
}

/* Adopted from bom.c */
#define MAXREFPINS 32
static char *reference_pin_names[] = {"1", "2", "A1", "A2", "B1", "B2", 0};

double kicad_get_rotation(ElementType *element)
{
	double theta = 0.0;

	char* fixed_rotation;
	fixed_rotation = AttributeGetFromList (&element->Attributes, "xy-fixed-rotation");

	if(fixed_rotation)
	{
		/* The user specified a fixed rotation */
		theta = atof(fixed_rotation);
	}
	else
	{
		Coord x, y;
		int pin_cnt;
		int pinfound[MAXREFPINS];
		double pinx[MAXREFPINS];
		double piny[MAXREFPINS];
		double pinangle[MAXREFPINS];
		int rpindex;

		double pin1x, pin1y;
		/* Initialize our pin count and our totals for finding the centroid. */

		pin_cnt = 0;
		for (rpindex = 0; rpindex < MAXREFPINS; rpindex++)
			pinfound[rpindex] = 0;

		/*
		 * Iterate over the pins and pads keeping a running count of how
		 * many pins/pads total and the sum of x and y coordinates
		 *
		 * While we're at it, store the location of pin/pad #1 and #2 if
		 * we can find them.
		 */

		PIN_LOOP (element);
		{
			pin_cnt++;

			for (rpindex = 0; reference_pin_names[rpindex]; rpindex++)
			{
				if (NSTRCMP (pin->Number, reference_pin_names[rpindex]) == 0)
				{
					pinx[rpindex] = (double) pin->X;
					piny[rpindex] = (double) pin->Y;
					pinangle[rpindex] = 0.0; /* pins have no notion of angle */
					pinfound[rpindex] = 1;
				}
			}
		}
		END_LOOP;

		PAD_LOOP (element);
		{
			pin_cnt++;

			for (rpindex = 0; reference_pin_names[rpindex]; rpindex++)
			{
				if (NSTRCMP (pad->Number, reference_pin_names[rpindex]) == 0)
				{
					double padcentrex, padcentrey;
					padcentrex = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
					padcentrey = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
					pinx[rpindex] = padcentrex;
					piny[rpindex] = padcentrey;
					/*
					 * NOTE: We swap the Y points because in PCB, the Y-axis
					 * is inverted.  Increasing Y moves down.  We want to deal
					 * in the usual increasing Y moves up coordinates though.
					 */
					pinangle[rpindex] = (180.0 / M_PI) * atan2 (pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
					pinfound[rpindex]=1;
				}
			}
		}
		END_LOOP;

		if (pin_cnt > 0)
		{
			x = element->MarkX;
			y = element->MarkY;


			/* Find first reference pin not at the  centroid  */
			for (rpindex = 0;
					 reference_pin_names[rpindex];
					 rpindex++)
			{
				if (pinfound[rpindex])
				{
					/* Recenter pin "#1" onto the axis which cross at the part
						 centroid */
					pin1x = pinx[rpindex] - x;
					pin1y = piny[rpindex] - y;

					/* flip x, to reverse rotation for elements on back */
					if (FRONT (element) != 1)
							pin1x = -pin1x;

					/* if only 1 pin, use pin 1's angle */
					if (pin_cnt == 1)
					{
						theta = pinangle[rpindex];
						break;
					}
					else if ((pin1x != 0.0) || (pin1y != 0.0))
					{
						theta = xyToAngle (pin1x, pin1y, pin_cnt > 2);
						break;
					}
				}
			}
		}
	}
	return theta;
}

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

/*!
 * \brief Print the file.
 */
static int
kicad_print (void)
{
	int i;
	FILE *fp;
	char uuid[37];

	net_descriptor* net_descs = calloc(100, sizeof(net_descriptor));
	int num_net_descs = 0;
	int max_net_descs = 100;

	kicad_net_assign = calloc(1000, sizeof(net_assignment));
	kicad_num_net_assign = 0;
	kicad_max_net_assign = 1000;

	fp = fopen (kicad_filename, "wb");

	if (!fp)
	{
		gui->log ((_("Cannot open file %s for writing\n")), kicad_filename);
		return 1;
	}

	const char* kicad_header =
	"(kicad_pcb\n"
	"\t(version 2024108)\n"
	"\t(generator \"MMGEDATRANSLATOR\")\n"
	"\t(generator_version \"1.0\")\n"
	"\t(general\n\t\t(thickness 1.6)\n\t)\n";

	const char* kicad_pagesettings =
	"\t(paper \"User\" %f %f)\n";

	const char* kicad_layers =
	"\t(layers\n"
	"\t\t(0 \"F.Cu\" signal)\n"
	"\t\t(1 \"In1.Cu\" signal)\n"
	"\t\t(2 \"In2.Cu\" signal)\n"
	"\t\t(31 \"B.Cu\" signal)\n"
	"\t\t(32 \"B.Adhes\" user)\n"
	"\t\t(33 \"F.Adhes\" user)\n"
	"\t\t(34 \"B.Paste\" user)\n"
	"\t\t(35 \"F.Paste\" user)\n"
	"\t\t(36 \"B.SilkS\" user)\n"
	"\t\t(37 \"F.SilkS\" user)\n"
	"\t\t(38 \"B.Mask\" user)\n"
	"\t\t(39 \"F.Mask\" user)\n"
	"\t\t(40 \"Dwgs.User\" user)\n"
	"\t\t(41 \"Cmts.User\" user)\n"
	"\t\t(42 \"Eco1.User\" user)\n"
	"\t\t(43 \"Eco2.User\" user)\n"
	"\t\t(44 \"Edge.Cuts\" user)\n"
	"\t\t(45 \"Margin\" user)\n"
	"\t\t(46 \"B.CrtYd\" user \"B.Courtyard\")\n"
	"\t\t(47 \"F.CrtYd\" user \"F.Courtyard\")\n"
	"\t\t(48 \"B.Fab\" user)\n"
	"\t\t(49 \"F.Fab\" user)\n"
	"\t\t(50 \"User.1\" user)\n"
	"\t\t(51 \"User.2\" user)\n"
	"\t\t(52 \"User.3\" user)\n"
	"\t\t(53 \"User.4\" user)\n"
	"\t\t(54 \"User.5\" user)\n"
	"\t\t(55 \"User.6\" user)\n"
	"\t\t(56 \"User.7\" user)\n"
	"\t\t(57 \"User.8\" user)\n"
	"\t\t(58 \"User.9\" user)\n"
  "\t)\n";


	fprintf (fp, kicad_header);
	fprintf (fp, kicad_pagesettings, COORD_TO_MM(PCB->MaxWidth), COORD_TO_MM(PCB->MaxHeight));
	fprintf (fp, kicad_layers);

	int netnum = 0;
	strcpy(net_descs[num_net_descs].net_name, "0");
	fprintf (fp, "\t(net %d \"%s\")\n", netnum, net_descs[num_net_descs].net_name);
	num_net_descs++;
	netnum++;


	for (i=0; i<PCB->NetlistLib.MenuN; i++)
	{
		char* netname = PCB->NetlistLib.Menu[i].Name;
		while(*netname == ' ') netname++;

		if(num_net_descs >= max_net_descs)
		{
			const int grow = 100;
			net_descriptor* tmp = net_descs;
			net_descs = calloc(max_net_descs + grow, sizeof(net_descriptor));
			memcpy(net_descs, tmp, max_net_descs * sizeof(net_descriptor));
			max_net_descs += grow;
			free(tmp);
		}

		net_descs[num_net_descs].net_id = netnum;
		strncpy(net_descs[num_net_descs].net_name, netname, 16);
		fprintf (fp, "\t(net %d \"%s\")\n", netnum, net_descs[num_net_descs].net_name);
		num_net_descs++;
		netnum++;
	}

	ELEMENT_LOOP (PCB->Data);
	{
		const char* kicad_footprint_header =
		"\t(footprint \"geda:%s\"\n"
		"\t\t(layer \"%s\")\n"
		"\t\t(uuid \"%s\")\n"
		"\t\t(at %f %f %f)\n";

		char* clayer = "F.Cu";
		char* slayer = "F.SilkS";
		char* flayer = "F.Fab";
		char* hidename = "no";
		char* mirror = "";

		int onsolder = TEST_FLAG (ONSOLDERFLAG, element);
		if (onsolder)
		{
			clayer = "B.Cu";
			slayer = "B.SilkS";
			flayer = "B.Fab";
			mirror = "mirror";
		}

		if(TEST_FLAG(HIDENAMEFLAG, element))
		{
			hidename = "yes";
		}

		float ex, ey, rot, sinphi, cosphi;
		ex = COORD_TO_MM(element->MarkX);
		ey = COORD_TO_MM(element->MarkY);

		sinphi = 0.0;
		cosphi = 1.0;
		rot = kicad_get_rotation(element);
		if(rot == 0.0)
		{
			sinphi =  0.0;
			cosphi =  1.0;
		}
		else if(rot == 90.0)
		{
			sinphi =  1.0;
			cosphi =  0.0;
		}
		else if(rot == 180.0)
		{
			sinphi =  0.0;
			cosphi = -1.0;

		}
		else if(rot == 270.0)
		{
			sinphi = -1.0;
			cosphi =  0.0;
		}
		else
		{
			float phi = rot * M_PI / 180.0;
			sinphi = sin(phi);
			cosphi = cos(phi);
		}

		float x, y, xr, yr;
		int trot;

		fprintf (fp, kicad_footprint_header, element->Name[0].TextString, clayer, kicad_uuid(uuid), ex, ey, rot);

		const char* kicad_element_reftext =
		"\t\t(property \"Reference\" \"%s\"\n"
		"\t\t\t(at %f %f %d)\n"
		"\t\t\t(unlocked yes)\n"
		"\t\t\t(layer \"%s\")\n"
		"\t\t\t(hide %s)\n"
		"\t\t\t(uuid \"%s\")\n"
		"\t\t\t(effects\n"
		"\t\t\t\t(font\n"
		"\t\t\t\t\t(size 1 0.8)\n"
		"\t\t\t\t\t(thickness 0.18)\n"
		"\t\t\t\t)\n"
		"\t\t\t\t(justify left top %s)\n"
		"\t\t\t)\n"
		"\t\t)\n";

		xr = COORD_TO_MM(element->Name[1].X) - ex;
		yr = COORD_TO_MM(element->Name[1].Y) - ey;
		x =  xr * cosphi - yr * sinphi;
		y =  xr * sinphi + yr * cosphi;
		trot = element->Name[1].Direction * 90;
		if(onsolder)
			trot = 180 - trot;

		fprintf (fp, kicad_element_reftext, element->Name[1].TextString,
																				x, y,
																				trot,
																				slayer,
																				hidename,
																				kicad_uuid(uuid), mirror);

		const char* kicad_element_valuetext =
		"\t\t(property \"Value\" \"%s\"\n"
		"\t\t\t(at %f %f %d)\n"
		"\t\t\t(layer \"%s\")\n"
		"\t\t\t(hide yes)\n"
		"\t\t\t(uuid \"%s\")\n"
		"\t\t\t(effects\n"
		"\t\t\t\t(font\n"
		"\t\t\t\t\t(size 1 1)\n"
		"\t\t\t\t\t(thickness 0.1)\n"
		"\t\t\t\t)\n"
		"\t\t\t)\n"
		"\t\t)\n";

		xr = COORD_TO_MM(element->Name[2].X) - ex;
		yr = COORD_TO_MM(element->Name[2].Y) - ey;
		x =  xr * cosphi - yr * sinphi;
		y =  xr * sinphi + yr * cosphi;
		fprintf (fp, kicad_element_valuetext, element->Name[2].TextString,
																				x, y,
																				element->Name[2].Direction * 90,
																				flayer,
																				kicad_uuid(uuid));

		const char* kicad_element_footprinttext =
		"\t\t(property \"Footprint\" \"geda:%s\"\n"
		"\t\t\t(at %f %f %d)\n"
		"\t\t\t(layer \"%s\")\n"
		"\t\t\t(hide yes)\n"
		"\t\t\t(uuid \"%s\")\n"
		"\t\t\t(effects\n"
		"\t\t\t\t(font\n"
		"\t\t\t\t\t(size 1 1)\n"
		"\t\t\t\t\t(thickness 0.1)\n"
		"\t\t\t\t)\n"
		"\t\t\t)\n"
		"\t\t)\n";
		xr = COORD_TO_MM(element->Name[2].X) - ex;
		yr = COORD_TO_MM(element->Name[2].Y) - ey;
		x =  xr * cosphi - yr * sinphi;
		y =  xr * sinphi + yr * cosphi;
		fprintf (fp, kicad_element_footprinttext, element->Name[0].TextString,
																				x, y,
																				element->Name[2].Direction * 90,
																				flayer,
																				kicad_uuid(uuid));

		PIN_LOOP (element)
		{
			char* kind = "thru_hole";

			net_descriptor* net = 0;
			if(!TEST_FLAG (VISITFLAG, pin))
			{
				int i, j;
				char nodename[256];
				sprintf(nodename, "%s-%s", element->Name[1].TextString, pin->Number);
				for (i=0; i<PCB->NetlistLib.MenuN; i++)
				{
					for (j=0; j<PCB->NetlistLib.Menu[i].EntryN; j++)
					{
						if (strcmp (PCB->NetlistLib.Menu[i].Entry[j].ListEntry, nodename) == 0)
						{
								/*printf(" in [%s]\n", PCB->NetlistLib.Menu[i].Name);*/
								net = &net_descs[i+1];
								break;
						}
					}
				}
				kicad_add_net(PIN_TYPE, pin, net);
			}
			else
			{
				net = kicad_get_net_assign(pin);
			}

			if(net == 0)
			{
				net = &net_descs[0];
			}

			if (TEST_FLAG (HOLEFLAG, pin))
			{
				kind = "np_thru_hole";
			}
			char* shape = "circle";
			char* chamfer = 0;
			if (TEST_FLAG (SQUAREFLAG, pin))
			{
				shape = "rect";
			}
			else if (TEST_FLAG (OCTAGONFLAG, pin))
			{
				shape = "rect";
				chamfer = "(chamfer_ratio 0.29365) (chamfer top_left top_right bottom_left bottom_right)";
			}

			double xr, yr, x, y, thick, mask, clear, drill;
			xr  = COORD_TO_MM(pin->X) - ex;
			yr  = COORD_TO_MM(pin->Y) - ey;
			x =  xr * cosphi - yr * sinphi;
			y =  xr * sinphi + yr * cosphi;
			thick = COORD_TO_MM(pin->Thickness);
			mask  = COORD_TO_MM(pin->Mask);
			clear = COORD_TO_MM(pin->Clearance);
			drill = COORD_TO_MM(pin->DrillingHole);

			fprintf (fp, "\t\t(pad \"%s\" %s %s\n", pin->Number, kind, shape);
			if(chamfer)
			{
				fprintf (fp, "\t\t\t%s\n", chamfer);
			}
			fprintf (fp, "\t\t\t(at %f %f)\n", x, y);
			fprintf (fp, "\t\t\t(size %f %f)\n", thick, thick);
			fprintf (fp, "\t\t\t(drill %f)\n", drill);
			fprintf (fp, "\t\t\t(layers \"*.Cu\" \"*.Mask\")\n");
			fprintf (fp, "\t\t\t(solder_mask_margin %f)\n", (mask - thick)/2.0);
			fprintf (fp, "\t\t\t(clearance %f)\n", clear/2);
			if (TEST_ANY_THERMS(pin))
			{
				int l;
				int thermal = 1;
				gui->log ((_("Pin with thermal\n")));
				for(l = 0; l<max_copper_layer; l++)
				{
					int tstyle = GET_THERM (l, pin);
					gui->log ((_("Thermal on layer %d - Type: %d\n")), l, tstyle);
					switch(tstyle)
					{
						case 0:
							continue;
						case 3:
							thermal = 2;
							break;
						default:
							break;
					}
					break;
				}
				fprintf (fp, "\t\t\t(zone_connect %d)\n", thermal);
				fprintf (fp, "\t\t\t(thermal_gap %f)\n", clear/2);
			}
			fprintf (fp, "\t\t\t(net %d \"%s\")\n", net->net_id, net->net_name);
			fprintf (fp, "\t\t\t(uuid %s)\n", kicad_uuid(uuid));
			fprintf (fp, "\t\t)\n");
    }
    END_LOOP;

		PAD_LOOP (element)
    {
			if(pad->Thickness > 0)
			{
				net_descriptor* net = 0;
				if(!TEST_FLAG (VISITFLAG, pad))
				{
					int i, j;
					char nodename[256];
					sprintf(nodename, "%s-%s", element->Name[1].TextString, pad->Number);
					for (i=0; i<PCB->NetlistLib.MenuN; i++)
					{
						for (j=0; j<PCB->NetlistLib.Menu[i].EntryN; j++)
						{
							if (strcmp (PCB->NetlistLib.Menu[i].Entry[j].ListEntry, nodename) == 0)
							{
									/*printf(" in [%s]\n", PCB->NetlistLib.Menu[i].Name);*/
									net = &net_descs[i+1];
									break;
							}
						}
					}
					kicad_add_net(PAD_TYPE, pad, net);
				}
				else
				{
					net = kicad_get_net_assign(pad);
				}

				if(net == 0)
				{
					net = &net_descs[0];
				}


				char* shape = "oval";
				char* chamfer = 0;
				if (TEST_FLAG (SQUAREFLAG, pad))
				{
					shape = "rect";
				}
				else if (TEST_FLAG (OCTAGONFLAG, pad))
				{
					shape = "rect";
					chamfer = "(chamfer_ratio 0.29365) (chamfer top_left top_right bottom_left bottom_right)";
				}

				char* layer = "\"F.Cu\"";
				char* paste = "\"F.Paste\"";
				char* maskl = "\"F.Mask\"";

				if (TEST_FLAG (ONSOLDERFLAG, pad))
				{
					layer = "\"B.Cu\"";
					paste = "\"B.Paste\"";
					maskl = "\"B.Mask\"";
				}
				if (TEST_FLAG (NOPASTEFLAG, pad))
				{
					paste = "";
				}

				double x1, x2, y1, y2, xr1, xr2, yr1, yr2, dx, dy, thick, mask, clear;
				xr1 = COORD_TO_MM(pad->Point1.X) - ex;
				xr2 = COORD_TO_MM(pad->Point2.X) - ex;
				yr1 = COORD_TO_MM(pad->Point1.Y) - ey;
				yr2 = COORD_TO_MM(pad->Point2.Y) - ey;
				x1 =  xr1 * cosphi - yr1 * sinphi;
				y1 =  xr1 * sinphi + yr1 * cosphi;
				x2 =  xr2 * cosphi - yr2 * sinphi;
				y2 =  xr2 * sinphi + yr2 * cosphi;
				dx = x2 - x1;
				dy = y2 - y1;
				thick = COORD_TO_MM(pad->Thickness);
				mask  = COORD_TO_MM(pad->Mask);
				clear = COORD_TO_MM(pad->Clearance);

				double x, y, angle, w, h;
				x = (x1 + x2)/2.0;
				y = (y1 + y2)/2.0;
				angle = rot;
				if(fabs(dx) <= 0.0001 && fabs(dy) <= 0.0001)
				{
					w = thick;
					h = thick;
				}
				else if(fabs(dx) <= 0.0001)
				{
					w = fabs(dy) + thick;
					h = thick;
					angle += 90.0;
				}
				else if(fabs(dy) <= 0.0001)
				{
					w = fabs(dx) + thick;
					h = thick;
				}
				else
				{
					angle += atan(dx / dy) * 180.0 / M_PI;
					h = sqrt(dx * dx + dy * dy) + thick;
					w = thick;
				}

				fprintf (fp, "\t\t(pad \"%s\" smd %s\n", pad->Number, shape);
				if(chamfer)
				{
					fprintf (fp, "\t\t\t%s\n", chamfer);
				}
				fprintf (fp, "\t\t\t(at %f %f %f)\n", x, y, angle);
				fprintf (fp, "\t\t\t(size %f %f)\n", w, h);
				fprintf (fp, "\t\t\t(layers %s %s %s)\n", layer, paste, maskl);
				fprintf (fp, "\t\t\t(solder_mask_margin %f)\n", (mask - thick)/2.0);
				fprintf (fp, "\t\t\t(clearance %f)\n", clear/2.0);
				fprintf (fp, "\t\t\t(net %d \"%s\")\n", net->net_id, net->net_name);
				fprintf (fp, "\t\t\t(uuid %s)\n", kicad_uuid(uuid));
				fprintf (fp, "\t\t)\n");
			}
			/*
			else if(pad->Mask > 0)
			{

			}
			*/
		}
		END_LOOP;

		ELEMENTLINE_LOOP(element)
		{
			double xr1, yr1, xr2, yr2, x1, y1, x2, y2, thick;
			xr1 = COORD_TO_MM(line->Point1.X) - ex;
			xr2 = COORD_TO_MM(line->Point2.X) - ex;
			yr1 = COORD_TO_MM(line->Point1.Y) - ey;
			yr2 = COORD_TO_MM(line->Point2.Y) - ey;
			x1 =  xr1 * cosphi - yr1 * sinphi;
			y1 =  xr1 * sinphi + yr1 * cosphi;
			x2 =  xr2 * cosphi - yr2 * sinphi;
			y2 =  xr2 * sinphi + yr2 * cosphi;
			thick = COORD_TO_MM(line->Thickness);
			fprintf (fp, "\t\t(fp_line\n");
			fprintf (fp, "\t\t\t(start %f %f)\n", x1, y1);
			fprintf (fp, "\t\t\t(end %f %f)\n", x2, y2);
			fprintf (fp, "\t\t\t(stroke\n\t\t\t\t(width %f)\n\t\t\t\t(type default)\n\t\t\t)\n", thick);
			fprintf (fp, "\t\t\t(layer \"%s\")\n", slayer);
			fprintf (fp, "\t\t\t(uuid %s)\n", kicad_uuid(uuid));
			fprintf (fp, "\t\t)\n");
		}
		END_LOOP;

		fprintf (fp, (_("\t)\n")));
	}
	END_LOOP;

	VIA_LOOP(PCB->Data)
	{
		const char* kicad_via =
		"\t(via %s\n"
		"\t\t(at %f %f)\n"
		"\t\t(size %f)\n"
		"\t\t(drill %f)\n"
		"\t\t(layers %s)\n"
		"\t\t(net %d)\n"
		"\t\t(uuid \"%s\")\n"
		"\t)\n";

		float x, y, size, drill;
		x = COORD_TO_MM(via->X);
		y = COORD_TO_MM(via->Y);
		size = COORD_TO_MM(via->Thickness);
		drill = COORD_TO_MM(via->DrillingHole);

		char* type = "";

		net_descriptor* net = kicad_get_net_assign(via);
		if(net == 0)
		{
			net = &net_descs[0];
		}

		fprintf (fp, kicad_via, type, x, y, size, drill, "\"F.Cu\" \"B.Cu\"", net->net_id, kicad_uuid(uuid));
	}
	END_LOOP;

	LAYER_LOOP(PCB->Data, max_copper_layer + SILK_LAYER)
	{
		int layeridx = n;
		int group = GetLayerGroupNumberByNumber(layeridx);
		int is_copper = 0;

		gui->log ((_("Processing layer %s - Type: %d\n")), layer->Name, layer->Type);

		char  namebuf[32];
		char* layername = "F.Cu";

		switch(layer->Type)
		{
			case LT_SILK:
				if(group == GetLayerGroupNumberBySide(TOP_SIDE))
				{
					layername = "F.SilkS";
				}
				else if (group == GetLayerGroupNumberBySide(BOTTOM_SIDE))
				{
					layername = "B.SilkS";
				}
				break;
			case LT_OUTLINE:
				layername = "Edge.Cuts";
				break;
			case LT_COPPER:
				if(group == GetLayerGroupNumberBySide(TOP_SIDE))
				{
					layername = "F.Cu";
					is_copper = 1;
				}
				else if (group == GetLayerGroupNumberBySide(BOTTOM_SIDE))
				{
					layername = "B.Cu";
					is_copper = 1;
				}
				else if(strcmp(layer->Name, "outline") == 0)
				{
					layername = "Edge.Cuts";
				}
				else
				{
					sprintf(namebuf, "In%d.Cu", group);
					layername = namebuf;
					is_copper = 1;
				}
				break;
			case LT_NOTES:
				layername = "Cmts.User";
			default:
				gui->log ((_("Unsupported layer type %d, skipping\n")), layer->Type);
				continue;
		}

		if(is_copper)
		{
			LINE_LOOP(layer)
			{
				const char* kicad_segment =
				"\t(segment\n"
				"\t\t(start %f %f)\n"
				"\t\t(end %f %f)\n"
				"\t\t(width %f)\n"
				"\t\t(layer \"%s\")\n"
				"\t\t(net %d)\n"
				"\t\t(uuid \"%s\")\n"
				"\t)\n";

				double x1, y1, x2, y2, thick;
				x1 = COORD_TO_MM(line->Point1.X);
				x2 = COORD_TO_MM(line->Point2.X);
				y1 = COORD_TO_MM(line->Point1.Y);
				y2 = COORD_TO_MM(line->Point2.Y);
				thick = COORD_TO_MM(line->Thickness);

				net_descriptor* net = kicad_get_net_assign(line);

				if(net == 0)
				{
					net = &net_descs[0];
				}

				fprintf (fp, kicad_segment, x1, y1, x2, y2, thick, layername, net->net_id, kicad_uuid(uuid));
			}
			END_LOOP;

			ARC_LOOP(layer)
			{
				const char* kicad_arc =
				"\t(arc\n"
				"\t\t(start %f %f)\n"
//				"\t\t(mid %f %f)\n"
				"\t\t(end %f %f)\n"
				"\t\t(angle %f)\n"
				"\t\t(width %f)\n"
				"\t\t(layer \"%s\")\n"
				"\t\t(net %d)\n"
				"\t\t(uuid \"%s\")\n"
				"\t)\n";

				double x, y, width, height, startangle, delta, thick;
				x = COORD_TO_MM(arc->X);
				y = COORD_TO_MM(arc->Y);
				width  = COORD_TO_MM(arc->Width);
				height = COORD_TO_MM(arc->Height);
				startangle = arc->StartAngle * M_PI / 180.0;
				delta      = arc->Delta * M_PI / 180.0;
				thick      = COORD_TO_MM(arc->Thickness);

				//double radius, xstart, ystart,xend, yend;
				double radius, xend, yend;
				radius = (width + height) / 2.0;
				//xstart = x - radius * cos(startangle);
				//ystart = y + radius * sin(startangle);
				xend   = x - radius * cos(startangle + delta);
				yend   = y + radius * sin(startangle + delta);

				net_descriptor* net = kicad_get_net_assign(arc);

				if(net == 0)
				{
					net = &net_descs[0];
				}

				//fprintf (fp, kicad_arc, xstart, ystart, x, y, xend, yend, thick, layername, net->net_id, kicad_uuid(uuid));
				fprintf (fp, kicad_arc, x, y, xend, yend, arc->Delta, thick, layername, net->net_id, kicad_uuid(uuid));
			}
			END_LOOP;

			POLYGON_LOOP(layer)
			{
				const char* kicad_zone_header =
				"\t(zone\n"
				"\t\t(net %d)\n"
				"\t\t(net_name \"%s\")\n"
				"\t\t(layer \"%s\")\n"
				"\t\t(uuid \"%s\")\n"
				"\t\t(hatch edge 0.508)\n"
				"\t\t(connect_pads no (clearance 0.25))\n"
				"\t\t(min_thickness 0.1)\n"
				"\t\t(fill yes\n"
				"\t\t\t(island_removal_mode %d)\n"
				"\t\t\t(island_area_min %f)\n"
				"\t\t)\n"
				"\t\t(polygon\n\t\t\t(pts\n";

				const char* kicad_zone_footer =
				"\t\t\t)\n"
				"\t\t)\n"
				"\t)\n";

				int np = 0;

				net_descriptor* net = kicad_get_net_assign(polygon);
				if(net == 0)
				{
					net = &net_descs[0];
				}

				int full = 0;
				if(TEST_FLAG(FULLPOLYFLAG, polygon))
				{
					full = 2;
				}

				double min_poly_area = COORD_TO_MM(COORD_TO_MM(PCB->IsleArea));

				fprintf (fp, kicad_zone_header, net->net_id, net->net_name, layername, kicad_uuid(uuid), full, min_poly_area);

				int PointN = polygon->HoleIndexN > 0 ? polygon->HoleIndex[0] : polygon->PointN;
				for (int n = 0; n < PointN; n++)
				{
					PointType* point = &(polygon)->Points[n];
					double x, y;
					x = COORD_TO_MM(point->X);
					y = COORD_TO_MM(point->Y);
					if(np == 0)
					{
						fprintf (fp, "\t\t\t\t(xy %f %f)", x, y);
					}
					else
					{
						fprintf (fp, " (xy %f %f)", x, y);
					}
					np++;
					if(np > 6)
					{
						np = 0;
						fprintf (fp, "\n");
					}
				}

				if(np != 0)
				{
					fprintf (fp, "\n");
				}
				fprintf (fp, kicad_zone_footer);

				for(int h = 0; h < polygon->HoleIndexN; h++)
				{
					const char* kicad_keepout_header =
					"\t(zone\n"
					"\t\t(net %d)\n"
					"\t\t(net_name \"\")\n"
					"\t\t(layer \"%s\")\n"
					"\t\t(uuid \"%s\")\n"
					"\t\t(hatch edge 0.508)\n"
					"\t\t(connect_pads no (clearance 0.25))\n"
					"\t\t(min_thickness 0.1)\n"
					"\t\t(keepout\n"
					"\t\t\t(tracks allowed)\n"
					"\t\t\t(vias allowed)\n"
					"\t\t\t(pads allowed)\n"
					"\t\t\t(copperpour not_allowed)\n"
					"\t\t\t(footprints allowed)\n"
					"\t\t)\n"
					"\t\t(fill yes\n"
//					"\t\t\t(island_removal_mode 2)\n"
//					"\t\t\t(island_area_min %f)\n"
					"\t\t)\n"
					"\t\t(polygon\n\t\t\t(pts\n";

					const char* kicad_keepout_footer =
					"\t\t\t)\n"
					"\t\t)\n"
					"\t)\n";

					fprintf (fp, kicad_keepout_header, net->net_id, layername, kicad_uuid(uuid));

					int EndPointN = (h+1) < polygon->HoleIndexN ? polygon->HoleIndex[h+1] : polygon->PointN;
					for (int n = polygon->HoleIndex[h]; n < EndPointN; n++)
					{
						PointType* point = &(polygon)->Points[n];
						double x, y;
						x = COORD_TO_MM(point->X);
						y = COORD_TO_MM(point->Y);
						if(np == 0)
						{
							fprintf (fp, "\t\t\t\t(xy %f %f)", x, y);
						}
						else
						{
							fprintf (fp, " (xy %f %f)", x, y);
						}
						np++;
						if(np > 6)
						{
							np = 0;
							fprintf (fp, "\n");
						}
					}

					if(np != 0)
					{
						fprintf (fp, "\n");
					}
					fprintf (fp, kicad_keepout_footer);
				}
			}
			END_LOOP;
		}
		else
		{
			LINE_LOOP(layer)
			{
				const char* kicad_gr_line =
				"\t(gr_line\n"
				"\t\t(start %f %f)\n"
				"\t\t(end %f %f)\n"
				"\t\t(stroke\n\t\t\t(width %f)\n\t\t\t(type solid)\n\t\t)\n"
				"\t\t(layer \"%s\")\n"
				"\t\t(uuid \"%s\")\n"
				"\t)\n";

				double x1, y1, x2, y2, thick;
				x1 = COORD_TO_MM(line->Point1.X);
				x2 = COORD_TO_MM(line->Point2.X);
				y1 = COORD_TO_MM(line->Point1.Y);
				y2 = COORD_TO_MM(line->Point2.Y);
				thick = COORD_TO_MM(line->Thickness);

				fprintf (fp, kicad_gr_line, x1, y1, x2, y2, thick, layername, kicad_uuid(uuid));
			}
			END_LOOP;

			ARC_LOOP(layer)
			{
				const char* kicad_gr_arc =
				"\t(gr_arc\n"
				"\t\t(start %f %f)\n"
				//"\t\t(mid %f %f)\n"
				"\t\t(end %f %f)\n"
				"\t\t(angle %f)\n"
				"\t\t(stroke\n\t\t\t(width %f)\n\t\t\t(type solid)\n\t\t)\n"
				"\t\t(layer \"%s\")\n"
				"\t\t(uuid \"%s\")\n"
				"\t)\n";

				double x, y, width, height, startangle, delta, thick;
				x = COORD_TO_MM(arc->X);
				y = COORD_TO_MM(arc->Y);
				width  = COORD_TO_MM(arc->Width);
				height = COORD_TO_MM(arc->Height);
				startangle = arc->StartAngle * M_PI / 180.0;
				delta      = arc->Delta * M_PI / 180.0;
				thick      = COORD_TO_MM(arc->Thickness);

				//double radius, xstart, ystart, xmid, ymid, xend, yend;
				double radius, xend, yend;
				radius = (width + height) / 2.0;
//				xstart = x - radius * cos(startangle);
//				ystart = y + radius * sin(startangle);
//				xmid   = x - radius * cos(startangle + 0.5 * delta);
//				ymid   = y + radius * sin(startangle + 0.5 * delta);
				xend   = x - radius * cos(startangle + delta);
				yend   = y + radius * sin(startangle + delta);

				//fprintf (fp, kicad_gr_arc, xstart, ystart, xmid, ymid, xend, yend, thick, layername, kicad_uuid(uuid));
				fprintf (fp, kicad_gr_arc, x, y, xend, yend, arc->Delta, thick, layername, kicad_uuid(uuid));
			}
			END_LOOP;

			TEXT_LOOP(layer)
			{
				const char* kicad_gr_text =
				"\t(gr_text \"%s\"\n"
				"\t\t(at %f %f %d)\n"
				"\t\t(layer \"%s\")\n"
				"\t\t(uuid \"%s\")\n"
				"\t\t(effects\n"
				"\t\t\t(font\n"
				"\t\t\t\t(size %f %f)\n"
				"\t\t\t\t(thickness 0.18)\n"
				"\t\t\t)\n"
				"\t\t\t(justify left top %s)\n"
				"\t\t)\n"
				"\t)\n";

				double x, y, theight, twidth;
				int angle;
				x = COORD_TO_MM(text->X);
				y = COORD_TO_MM(text->Y);
				theight = 1.0 * text->Scale / 100.0;
				twidth  = 0.8 * text->Scale / 100.0;
				angle   = text->Direction * 90;

				char* mirror = "";
				if(TEST_FLAG(ONSOLDERFLAG, text))
				{
					mirror = "mirror";
					angle = 180 - angle;
				}

				fprintf (fp, kicad_gr_text, text->TextString,
									x,
									y,
									angle,
									layername,
									kicad_uuid(uuid), theight, twidth, mirror);
			}
			END_LOOP;
		}
	}
	END_LOOP;

	fprintf (fp, ")\n");

	ClearFlagOnAllObjects(VISITFLAG, true);
	free(kicad_net_assign);
	kicad_net_assign = 0;
	kicad_max_net_assign = 0;
	free(net_descs);

	fclose (fp);

	return (0);
}

/*!
 * \brief Do export the KiCad layout.
 */
static void
kicad_do_export (HID_Attr_Val * options)
{
	int i;
	int save_ons[MAX_ALL_LAYER];

	if (!options)
	{
		kicad_get_export_options (0);

		for (i = 0; i < NUM_OPTIONS; i++)
			kicad_values[i] = kicad_options[i].default_val;

		options = kicad_values;
	}

	kicad_filename = options[HA_kicad_file].str_value;

	if (!kicad_filename)
		kicad_filename = "pcb-out.kicad_pcb";

	hid_save_and_show_layer_ons (save_ons);
	kicad_print ();
	hid_restore_layer_ons (save_ons);
}

/*!
 * \brief Parse command line arguments.
 */
static void
kicad_parse_arguments (int *argc, char ***argv)
{
	hid_register_attributes (kicad_options,
		sizeof (kicad_options) / sizeof (kicad_options[0]));

	hid_parse_command_line (argc, argv);
}

HID kicad_hid;

/*!
 * \brief Initialise the exporter HID.
 */
void
hid_kicad_init ()
{
	memset (&kicad_hid, 0, sizeof (HID));

	common_nogui_init (&kicad_hid);

	kicad_hid.struct_size         = sizeof (HID);
	kicad_hid.name                = "kicad";
	kicad_hid.description         = "Exports a KiCad pcb file format";
	kicad_hid.exporter            = 1;

	kicad_hid.get_export_options  = kicad_get_export_options;
	kicad_hid.do_export           = kicad_do_export;
	kicad_hid.parse_arguments     = kicad_parse_arguments;

	hid_register_hid (&kicad_hid);
}

/* EOF */
