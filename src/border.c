//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "lists.h"
#include "game.h"
#include "mathutil.h"
#include "net.h"
#include "text.h"

#define BAR_HEIGHT 48
#define XDIM 320
#define YDIM 200

short DebugBorderShade = 0;

short RegBorderTest[] =
    {
    51, 53, 127, 128, 140, 145, 152, 197, 198, 201, 205, 206, 213, 218, 242, 243,
    245, 246, 247, 257, 2560, 2561, 2562, 2570, 2571, 2572, 2573, 2576, 2578,
    2579, 2580, 2581, 2582, 2583, 2584, 2585, 2593, 2594
    };
short SWBorderTest[] =
    {
    51, 53, 127, 128, 140, 145, 201, 205, 206, 213, 218,
    245, 2560, 2573, 2576, 2580, 2581, 2582, 2583, 2584, 2593
    };

#undef BORDER_TILE
#define BORDER_TILE \
	(SW_SHAREWARE ? \
	 	SWBorderTest[gs.BorderTile % SIZ(SWBorderTest)] : \
		RegBorderTest[gs.BorderTile % SIZ(RegBorderTest)] \
	)

#define f_320 FIXED(320,0)
#define f_200 FIXED(200,0)

#define X_TO_FIXED(val) (x_aspect_mul*(val))
#define Y_TO_FIXED(val) (y_aspect_mul*(val))

BOOL RedrawScreen = FALSE;

int f_xdim, f_ydim, x_pix_size, y_pix_size, x_aspect_mul, y_aspect_mul;
int CrosshairX, CrosshairY;

extern BOOL BorderAdjust;
BOOL GlobSpriteBoxUpdateEveryFrame = FALSE;

PANEL_SPRITEp
pSpawnFullScreenSpriteBox(PLAYERp pp, short id, short pic, short pri, int x, int y, short x1, short y1, short x2, short y2)
    {
    PANEL_SPRITEp psp;
    extern BOOL DrawBeforeView;

    psp = pSpawnSprite(pp, NULL, pri, x, y);

    psp->ID = id;
    psp->numpages = numpages;
    if (GlobSpriteBoxUpdateEveryFrame)
        {
        psp->numpages = 1;
        }
    psp->picndx = -1;
    psp->picnum = pic;
    psp->x1 = x1;
    psp->y1 = y1;
    psp->x2 = x2;
    psp->y2 = y2;
    psp->shade = DebugBorderShade;

    //SET(psp->flags, PANF_STATUS_AREA | PANF_KILL_AFTER_SHOW | PANF_IGNORE_START_MOST  | PANF_DRAW_BEFORE_VIEW | PANF_NOT_ALL_PAGES);
    SET(psp->flags, PANF_STATUS_AREA | PANF_KILL_AFTER_SHOW | PANF_IGNORE_START_MOST | PANF_DRAW_BEFORE_VIEW);
    //DrawBeforeView = TRUE;

    //SET(psp->flags, PANF_SCREEN_CLIP | PANF_KILL_AFTER_SHOW | PANF_IGNORE_START_MOST);

    return (psp);
    }

VOID SetCrosshair(VOID)
    {
    int wdx,wdy,x,y;

    wdx = ((windowx2-windowx1)/2);
    wdy = ((windowy2-windowy1)/2);
    x = windowx1 + wdx;
    y = windowy1 + wdy;

    CrosshairX = x / (xdim/320.0);
    CrosshairY = y / (ydim/200.0);

    // rotatesprite takes FIXED point number
    CrosshairX <<= 16;
    CrosshairY <<= 16;
    }


VOID
SetupAspectRatio(VOID)
    {
    f_xdim = FIXED(xdim, 0);
    f_ydim = FIXED(ydim, 0);

    x_pix_size = (f_320 / xdim);
    y_pix_size = (f_200 / ydim);

    x_aspect_mul = (f_xdim / 320);
    y_aspect_mul = (f_ydim / 200);
    }

VOID
SetConsoleDmost(VOID)
    {
    int ystart;
    int xstart;

    int i;
    int adj=0;

    // dont setup the startumost/dmost arrays if border is 0
    if (gs.BorderNum == BORDER_NONE || gs.BorderNum == BORDER_MINI_BAR)
        return;

    //
    // Set the whole thing to the size of the bar
    //

    ystart = f_ydim - Y_TO_FIXED(BAR_HEIGHT);

    if (ydim == 480 && gs.BorderNum == 2)
        adj = 1;

    //for (i = FIXED(0, 0); i < f_320; i += x_pix_size)
    for (i = 0; i < xdim; i++)
        // define picture
        // boundaries
        {
        startdmost[i] = MSW(ystart) + adj;
        }
    }

VOID ClearStartMost(VOID)
    {
    int i;

    for (i = 0; i < xdim; i++)
        startdmost[i] = ydim;

    memset(startumost, 0, sizeof(startumost));
    }

VOID
SetFragBar(PLAYERp pp)
    {
    short i, num_frag_bars;
    int y;
    extern SHORT OrigCommPlayers;
    PANEL_SPRITEp psp;

    if (CommPlayers <= 1)
        return;

    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
        return;

    // if player sprite has not been initialized we have no business
    // sticking a frag bar up.  Prevents processing from MenuLevel etc.
    if (!pp->SpriteP)
        return;

    num_frag_bars = ((OrigCommPlayers-1)/4)+1;

    for (i = windowx1; i <= windowx2; i++)
        {
        y = (tilesizy[FRAG_BAR] * num_frag_bars) - (2 * (num_frag_bars-1));
        y = y * (ydim/200.0);

        if (windowy1 < y)
            startumost[i] = y;
        }

     for (i = 0, y = 0; i < num_frag_bars; i++)
        {
        psp = pSpawnFullScreenSprite(pp, FRAG_BAR, PRI_MID, 0, y);
        SET(psp->flags, PANF_NON_MASKED|PANF_SCALE_TOP);
        y += tilesizy[FRAG_BAR] - 2;
        }

    // write each persons kill info to everybody
    TRAVERSE_CONNECT(i)
        {
        PlayerUpdateKills(Player + i, 0);
        DisplayFragNames(Player + i);
        }
    }

BOOL RectOverlap(short tx1, short ty1, short bx1, short by1, short tx2, short ty2, short bx2, short by2)
    {
    if (bx1 >= tx2)
        if (tx1 <= bx2)
            if (ty1 <= by2)
                if (by1 >= ty2)
                    return(TRUE);

    return(FALSE);
    }

VOID DrawBorderShade(PLAYERp pp, short shade_num, short wx1, short wy1, short wx2, short wy2)
    {
    short i,j,k,l;
    PANEL_SPRITEp psp;
    int dark_shade = 27 - (shade_num * 6);
    int light_shade = 20 - (shade_num * 6);

    for (i = 0; i < xdim; i += tilesizx[BORDER_TILE])
        {
        for (j = 0; j < ydim; j += tilesizy[BORDER_TILE])
            {
            k = i + tilesizx[BORDER_TILE];
            l = j + tilesizy[BORDER_TILE];

            if (RectOverlap(i, j, k, l, wx1 - 1, wy1 - 1, wx2 + 1, wy1))
                {
                // draw top box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_TOP, BORDER_TILE, PRI_BACK + 1, i, j, wx1 - 1, wy1 - 1, wx2 + 1, wy1);
                SET(psp->flags, PANF_NON_MASKED);
                psp->shade = dark_shade;
                psp->ID = ID_BORDER_SHADE;
                }

            if (RectOverlap(i, j, k, l, wx1 - 1, wy2, wx2 + 1, wy2 + 1))
                {
                // draw bottom box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_BOTTOM, BORDER_TILE, PRI_BACK + 1, i, j, wx1 - 1, wy2, wx2 + 1, wy2 + 1);
                SET(psp->flags, PANF_NON_MASKED);
                psp->shade = light_shade;
                psp->ID = ID_BORDER_SHADE;
                }
            if (RectOverlap(i, j, k, l, wx1 - 1, wy1 - 1, wx1, wy2 + 1))
                {
                // draw left box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_LEFT, BORDER_TILE, PRI_BACK + 1, i, j, wx1 - 1, wy1 - 1, wx1, wy2 + 1);
                SET(psp->flags, PANF_NON_MASKED);
                psp->shade = dark_shade;
                psp->ID = ID_BORDER_SHADE;
                }
            if (RectOverlap(i, j, k, l, wx2, wy1 - 1, wx2 + 1, wy2 + 1))
                {
                // draw right box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_RIGHT, BORDER_TILE, PRI_BACK + 1, i, j, wx2, wy1 - 1, wx2 + 1, wy2 + 1);
                SET(psp->flags, PANF_NON_MASKED);
                psp->shade = light_shade;
                psp->ID = ID_BORDER_SHADE;
                }
            }
        }
    }

VOID
BorderShade(PLAYERp pp, BOOL UNUSED(refresh))
    {
    int i, j, k, l, wx1, wx2, wy1, wy2;
    PANEL_SPRITEp psp;
    BYTE lines;

    wx1 = windowx1 - 1;
    wy1 = windowy1 - 1;
    wx2 = windowx2 + 1;
    wy2 = windowy2 + 1;

    for (lines = 0; lines < 4; lines++)
        {

        // make sure that these values dont go out of bound - which they do
        wx1 = max(wx1, 0);
        wx2 = min(wx2, xdim - 1);
        wy1 = max(wy1, 0);
        wy2 = min(wy2, MSW(f_ydim - mulscale16(Y_TO_FIXED(BAR_HEIGHT), PanelScale)) - 1);

        DrawBorderShade(pp, lines, wx1, wy1, wx2, wy2);
        // increase view size by one - dont do a set view though
        wx1--;
        wy1--;
        wx2++;
        wy2++;
        }
    }


BORDER_INFO BorderInfoValues[] =
    {
    // x,y,screensize
    {0, 0, 0},
    {0, 0, 0},
    {0, BAR_HEIGHT, 0},

    {0, BAR_HEIGHT, (1 * 16)},
    {0, BAR_HEIGHT, (2 * 16)},
    {0, BAR_HEIGHT, (3 * 16)},
    {0, BAR_HEIGHT, (4 * 16)},
    {0, BAR_HEIGHT, (5 * 16)},
    {0, BAR_HEIGHT, (6 * 16)},
    {0, BAR_HEIGHT, (7 * 16)},
    {0, BAR_HEIGHT, (8 * 16)},
    {0, BAR_HEIGHT, (9 * 16)},
    {0, BAR_HEIGHT, (10 * 16)},
    {0, BAR_HEIGHT, (11 * 16)},
    {0, BAR_HEIGHT, (12 * 16)}
    };


VOID DrawBorder(PLAYERp pp, short x, short y, short x2, short y2)
    {
    short i,j,k,l;
    short count = 0;
    PANEL_SPRITEp psp;

    for (i = 0; i < xdim; i += tilesizx[BORDER_TILE])
        {
        for (j = 0; j < ydim; j += tilesizy[BORDER_TILE])
            {
            k = i + tilesizx[BORDER_TILE];
            l = j + tilesizy[BORDER_TILE];

            if (RectOverlap(i, j, k, l, x, y, windowx1-1, y2))
                {
                // draw top box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_TOP, BORDER_TILE, PRI_BACK, i, j, x, y, windowx1-1, y2);
                SET(psp->flags, PANF_NON_MASKED);
                count++;
                }

            if (RectOverlap(i, j, k, l, windowx2+1, y, x2, y2))
                {
                // draw bottom box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_BOTTOM, BORDER_TILE, PRI_BACK, i, j, windowx2+1, y, x2, y2);
                SET(psp->flags, PANF_NON_MASKED);
                count++;
                }

            if (RectOverlap(i, j, k, l, windowx1, y, windowx2, windowy1-1))
                {
                // draw left box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_LEFT, BORDER_TILE, PRI_BACK, i, j, windowx1, y, windowx2, windowy1-1);
                SET(psp->flags, PANF_NON_MASKED);
                count++;
                }

            if (RectOverlap(i, j, k, l, windowx1, windowy2+1, windowx2, y2))
                {
                // draw right box of the border
                psp = pSpawnFullScreenSpriteBox(pp, ID_BORDER_RIGHT, BORDER_TILE, PRI_BACK, i, j, windowx1, windowy2+1, windowx2, y2);
                SET(psp->flags, PANF_NON_MASKED);
                count++;
                }
            }
        }
    }

VOID DrawPanelBorderSides(PLAYERp pp, short x, short y, short x2, short y2, short panl, short panr)
    {
    short i,j,k,l;
    short count = 0;
    PANEL_SPRITEp psp;

    for (i = 0; i < xdim; i += tilesizx[BORDER_TILE])
        {
        for (j = 0; j < ydim; j += tilesizy[BORDER_TILE])
            {
            k = i + tilesizx[BORDER_TILE];
            l = j + tilesizy[BORDER_TILE];

            if (RectOverlap(i, j, k, l, x, y, panl, y2))
                {
                psp = pSpawnFullScreenSpriteBox(pp, ID_PANEL_BORDER_LEFT, BORDER_TILE, PRI_BACK, i, j, x, y, panl, y2);
                SET(psp->flags, PANF_NON_MASKED);
                count++;
                }

            if (RectOverlap(i, j, k, l, panr, y, x2, y2))
                {
                psp = pSpawnFullScreenSpriteBox(pp, ID_PANEL_BORDER_RIGHT, BORDER_TILE, PRI_BACK, i, j, panr, y, x2, y2);
                SET(psp->flags, PANF_NON_MASKED);
                count++;
                }
            }
        }
    }

static
VOID BorderSetView(PLAYERp UNUSED(pp), int *Xdim, int *Ydim, int *ScreenSize)
    {
    void setview(int scrx1, int scry1, int scrx2, int scry2);
    int x, x2, y, y2;
    int xd, yd, sc;
    BORDER_INFO *b;

    BorderInfo = BorderInfoValues[gs.BorderNum];

    b = &BorderInfo;

    // figure out the viewing window x and y dimensions
    xd = f_xdim - X_TO_FIXED(b->Xdim);
    yd = f_ydim - mulscale16(Y_TO_FIXED(b->Ydim), PanelScale);
    sc = f_xdim - X_TO_FIXED(b->ScreenSize);
    *Xdim = MSW(xd);
    *Ydim = MSW(yd);
    *ScreenSize = MSW(sc);

    // figure out the viewing window x and y coordinates
    x = MSW(DIV2(xd - sc));
    x2 = MSW(DIV2(xd + sc));
    y = MSW(DIV2(yd - scale(sc, yd, xd)));
    y2 = MSW_ROUND(DIV2(yd + scale(sc, yd, xd)) + 32768);
    if (y2 > ydim)
        y2 = ydim;

    // global windowx1, windowx2, windowy1, windowy2 coords set here
    setview(x, y, x2-1, y2-1);
    SetCrosshair();
    }

//
// Redraw the border without changing the view
//

static VOID
BorderRefresh(PLAYERp pp)
    {
    int i, j;
    int x, x2, y, y2;
    BORDER_INFO *b;

    if (pp != Player + myconnectindex)
        return;

    if (!BorderAdjust)
        return;

    if (gs.BorderNum < BORDER_BAR)
        return;

    // Redraw the BORDER_TILE only if getting smaller
    BorderInfo = BorderInfoValues[gs.BorderNum];

    b = &BorderInfo;

    // A refresh does not change the view size so we dont need to do a
    // setview
    // We don't need the calculations for the border drawing boxes - its
    // the whole screen
    // minus the border if necessary

    // fill in the sides of the panel when the screen is wide
    if (gs.BorderNum >= BORDER_BAR && (widescreen || PanelScale < FIXED(1,0)))
        {
        int barw = scale(f_320, f_ydim, 200 * pixelaspect);
        int sidel, sider;

        x = 0;
        x2 = xdim - 1;

        y = MSW_ROUND(f_ydim - mulscale16(Y_TO_FIXED(b->Ydim), PanelScale) + 32768);
        y2 = ydim - 1;

        sidel = MSW_ROUND(DIV2(f_xdim - mulscale16(barw, PanelScale)));
        sider = MSW(DIV2(f_xdim + mulscale16(barw, PanelScale)));

        DrawPanelBorderSides(pp, x, y, x2, y2, sidel, sider);
        }

    // only need a border if border is > BORDER_BAR
    if (gs.BorderNum > BORDER_BAR)
        {
        // make sure that these values dont go out of bound - which they do
        x = 0;
        x2 = xdim - 1;

        y = 0;
        y2 = MSW_ROUND(f_ydim - mulscale16(Y_TO_FIXED(b->Ydim), PanelScale));

        DrawBorder(pp, x, y, x2, y2);

        // kill ALL outstanding (not yet drawn) border shade sprites before
        // doing more shading
        pKillScreenSpiteIDs(pp, ID_BORDER_SHADE);

        BorderShade(pp, TRUE);
        }
    }

//
// Redraw the whole screen
//

VOID SetBorder(PLAYERp pp, int value)
    {
    int diff;
    int Xdim, Ydim, ScreenSize;
    BOOL set_view = TRUE;
    PANEL_SPRITEp psp;

    if (pp != Player + myconnectindex)
        return;

    if (!BorderAdjust)
        return;

    if (value >= 0) // just refresh
        gs.BorderNum = value;

    if (gs.BorderNum < BORDER_NONE)
        {
        gs.BorderNum = BORDER_NONE;
        //return;
        }

    if (gs.BorderNum > (int)SIZ(BorderInfoValues) - 1)
        {
        gs.BorderNum = SIZ(BorderInfoValues) - 1;
        return;
        }

    BorderSetView(pp, &Xdim, &Ydim, &ScreenSize);

    if (gs.BorderNum >= BORDER_BAR)
        {
        BorderRefresh(pp);

        psp = pSpawnFullScreenSprite(pp, STATUS_BAR, PRI_BACK, 0, 200 - tilesizy[STATUS_BAR]);
        SET(psp->flags, PANF_NON_MASKED|PANF_SCALE_BOTTOM);
        PlayerUpdatePanelInfo(Player + screenpeek);
        }

    SetFragBar(pp);
    }

VOID
SetRedrawScreen(PLAYERp pp)
    {
    int i, j;
    //int x, x2, y, y2;
    BORDER_INFO *b;

    if (pp != Player + myconnectindex)
        return;

    if (!BorderAdjust)
        return;

    if (gs.BorderNum < BORDER_NONE)
        gs.BorderNum = BORDER_NONE;

    // Redraw the BORDER_TILE only if getting smaller
    BorderInfo = BorderInfoValues[gs.BorderNum];

    b = &BorderInfo;

    // test at redrawing the whole screen
    RedrawScreen = TRUE;
    }

