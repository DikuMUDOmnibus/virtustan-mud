/* ************************************************************************
*   File: color.cpp                                     Part of Bylins    *
*  Usage: interprets inline colour codes                                  *
*  Name: Easy Colour v2.2                                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*  Modifications Copyright Trevor Man 1997                                *
*  Based on the Easy Color patch by mud@proimages.proimages.com           *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#define CNRM  "\x1B[0;0m"
#define CBLK  "\x1B[0;30m"
#define CRED  "\x1B[0;31m"
#define CGRN  "\x1B[0;32m"
#define CYEL  "\x1B[0;33m"
#define CBLU  "\x1B[0;34m"
#define CMAG  "\x1B[0;35m"
#define CCYN  "\x1B[0;36m"
#define CWHT  "\x1B[0;37m"
#define CNUL  ""

#define BBLK  "\x1B[1;30m"
#define BRED  "\x1B[1;31m"
#define BGRN  "\x1B[1;32m"
#define BYEL  "\x1B[1;33m"
#define BBLU  "\x1B[1;34m"
#define BMAG  "\x1B[1;35m"
#define BCYN  "\x1B[1;36m"
#define BWHT  "\x1B[1;37m"

#define BKBLK  "\x1B[40m"
#define BKRED  "\x1B[41m"
#define BKGRN  "\x1B[42m"
#define BKYEL  "\x1B[43m"
#define BKBLU  "\x1B[44m"
#define BKMAG  "\x1B[45m"
#define BKCYN  "\x1B[46m"
#define BKWHT  "\x1B[47m"

#define CAMP  "&"
#define CSLH  "\\"

#define CUDL  "\x1B[4m"		// Underline ANSI code
#define CFSH  "\x1B[5m"		/* Flashing ANSI code.  Change to #define CFSH "" if
* you want to disable flashing colour codes
*/
#define CRVS  "\x1B[7m"		// Reverse video ANSI code

const char *COLOURLIST[] = { CNRM, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
							 BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
							 BKRED, BKGRN, BKYEL, BKBLU, BKMAG, BKCYN, BKWHT,
							 CAMP, CSLH, BKBLK, CBLK, CFSH, CRVS, CUDL, BBLK
						   };

#define MAX_COLORS 30

int isnum(char s)
{
	return ((s >= '0') && (s <= '9'));
}

int is_colour(char code)
{
	switch (code)
	{
		// Normal colours
	case 'k':
		return 25;
		break;		// Black
	case 'r':
		return 1;
		break;		// Red
	case 'g':
		return 2;
		break;		// Green
	case 'y':
		return 3;
		break;		// Yellow
	case 'b':
		return 4;
		break;		// Blue
	case 'm':
		return 5;
		break;		// Magenta
	case 'c':
		return 6;
		break;		// Cyan
	case 'w':
		return 7;
		break;		// White

		// Bold colours
	case 'K':
		return 29;
		break;		// Bold black (Just for completeness)
	case 'R':
		return 8;
		break;		// Bold red
	case 'G':
		return 9;
		break;		// Bold green
	case 'Y':
		return 10;
		break;		// Bold yellow
	case 'B':
		return 11;
		break;		// Bold blue
	case 'M':
		return 12;
		break;		// Bold magenta
	case 'C':
		return 13;
		break;		// Bold cyan
	case 'W':
		return 14;
		break;		// Bold white

		// Background colours
	case '0':
		return 24;
		break;		// Black background
	case '1':
		return 15;
		break;		// Red background
	case '2':
		return 16;
		break;		// Green background
	case '3':
		return 17;
		break;		// Yellow background
	case '4':
		return 18;
		break;		// Blue background
	case '5':
		return 19;
		break;		// Magenta background
	case '6':
		return 20;
		break;		// Cyan background
	case '7':
		return 21;
		break;		// White background

		// Misc characters
//	case '&':
//		return 22;
//		break;		// The & character
	case '\\':
		return 23;
		break;		// The \ character

		// Special codes
	case 'n':
		return 0;
		break;		// Normal
	case 'f':
		return 26;
		break;		// Flash
	case 'v':
		return 27;
		break;		// Reverse video
	case 'u':
		return 28;
		break;		// Underline (Only for mono screens)
	case 'q':
		return -2;
		break;		// set default color to current
	case 'Q':
		return -3;
		break;		// set default color back to normal
	case 'e':
		return -4;
		break;		// print error when color not reseted to normal

	default:
		return -1;
		break;
	}
	return -1;
}

int proc_color(char *inbuf, int colour)
{
	register int j = 0, p = 0;
	int k, max, c = 0, tmp = 0, nc = 0; // normal colour CNRM by default
	bool show_all = false;
	char out_buf[MAX_SOCK_BUF * 2];

	if (inbuf == NULL)
		return -1;

	int len = strlen(inbuf);
	if (len == 0)
		return -2;

	while (j < len)
	{
		// WorM: ������� ����� &S � &s, ������ � ����� ������ ��� ��������� ������ � _
		if (inbuf[j] == '&' && ((!show_all && inbuf[j + 1] == 'S') || (show_all && inbuf[j + 1] == 's')))
		{
			show_all = !show_all;
			j += 2;
			continue;
		}
		if (!show_all && (inbuf[j] == '\\') && (inbuf[j + 1] == 'c')
				&& isnum(inbuf[j + 2]) && isnum(inbuf[j + 3]))
		{
			c = (inbuf[j + 2] - '0') * 10 + inbuf[j + 3] - '0';
			j += 4;
		}
		else if (!show_all && (inbuf[j] == '&') && !((tmp = is_colour(inbuf[j + 1])) == -1))
		{
			j += 2;
			if (tmp == -4)
			{
				// ���� &e ������� ��� �����.����� ���� ��� �� ������� ����� ������ &n
				if (c != 0 && c != nc)
				{
					out_buf[p++] = '!';
					out_buf[p++] = '!';
					out_buf[p++] = '!';
				}
			}
			else
			{
		        	// WorM: ���� &q ��������� ������� ���� ��� ����������� &n ������ 0 ����� ���������� ���
				(tmp < 0 ? nc : c) = (tmp == -2 ? c : (tmp == -3 ? 0 : tmp));
			}
			if (tmp < 0)
				continue;
		}
		else
		{
			out_buf[p] = (!show_all && inbuf[j] == '_' ? ' ' : inbuf[j]);
			j++;
			p++;
			continue;
		}
		if (c >= MAX_COLORS)
			c = 0;
		max = strlen(COLOURLIST[(c == 0 && nc != 0 ? nc : c)]);
		if (colour || max == 1)
			for (k = 0; k < max; k++)
			{
				out_buf[p] = COLOURLIST[(c == 0 && nc != 0 ? nc : c)][k];
				p++;
			}
	}
	out_buf[p] = '\0';

	strcpy(inbuf, out_buf);
	if (j > len)
		return j;
	return 0;
}