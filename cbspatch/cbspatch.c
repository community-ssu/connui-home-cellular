/*
 *  operator-name-cbs-home-item (operator name item)
 *  Copyright (C) 2011 Jonathan Wilson
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  
 */

#include <stdio.h>

int main()
{
	FILE *f = fopen("/usr/lib/libsms.so.0.0.0","r+");
	if (!f) return 1;
	fseek(f,0xDD78,SEEK_SET);
	unsigned char c = 0x52;
	fwrite(&c,1,1,f);
	fseek(f,0xDD7C,SEEK_SET);
	fwrite(&c,1,1,f);
	c = 0xC3;
	fseek(f,0xDD7F,SEEK_SET);
	fwrite(&c,1,1,f);
	fclose(f);
	return 0;
}
