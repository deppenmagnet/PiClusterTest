/***************************************************************************
 *            identify_myself.h -> hole KnotenInfo aus /proc/cpuinfo
 *
 *  Di Oktober 28 11:47:02 2014
 *  Copyright  2014  Cluster-Controller
 *  <deppenmagnet@openmailbox.org>
 ****************************************************************************/
/*
 * Copyright (C) 2014  Cluster-Controller
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#ifndef __identify_myself_h__
#define __identify_myself_h__

#define AMD_OCTA_CORE	0
#define RASPBERRY_PI	1
#define BANANA_PI		2
#define NO_CPU_INFO		-1

void identify_myself(int *node_type);

#endif /* __identify_myself_h__ */
