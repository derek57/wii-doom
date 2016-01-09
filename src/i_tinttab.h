/*
========================================================================

                               DOOM RETRO
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM RETRO is a fork of CHOCOLATE DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM RETRO.

  DOOM RETRO is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM RETRO is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM RETRO. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM RETRO is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/


#if !defined(__I_TINTTAB__)
#define __I_TINTTAB__


extern byte             *tranmap;
extern byte             *tinttab;
extern byte             *tinttab5;
extern byte             *tinttab10;
extern byte             *tinttab15;
extern byte             *tinttab20;
extern byte             *tinttab25;
extern byte             *tinttab30;
extern byte             *tinttab33;
extern byte             *tinttab35;
extern byte             *tinttab40;
extern byte             *tinttab45;
extern byte             *tinttab50;
extern byte             *tinttab55;
extern byte             *tinttab60;
extern byte             *tinttab65;
extern byte             *tinttab66;
extern byte             *tinttab70;
extern byte             *tinttab75;
extern byte             *tinttab80;
extern byte             *tinttab85;
extern byte             *tinttab90;
extern byte             *tinttab95;
extern byte             *tinttabred;
extern byte             *tinttabredwhite1;
extern byte             *tinttabredwhite2;
extern byte             *tinttabgreen;
extern byte             *tinttabblue;
extern byte             *tinttabred33;
extern byte             *tinttabredwhite50;
extern byte             *tinttabgreen33;
extern byte             *tinttabblue33;


void I_InitTintTables(byte *palette);

#endif
