// SPDX-License-Identifier: CDDL-1.0
/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * https://opensource.org/license/CDDL-1.0.
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, Joyent, Inc.
 * Copyright (c) 2011, 2019 by Delphix. All rights reserved.
 * Copyright (c) 2014 by Saso Kiselkov. All rights reserved.
 * Copyright 2017 Nexenta Systems, Inc.  All rights reserved.
 */

#include <sys/zfs_context.h>
#include <sys/arc_impl.h>

#ifdef _WIN32
/*
 * On Windows libzpool is also the engine of zfswin-um.exe, the user-mode
 * edition: a service that runs as long as the machine does, not a test.
 * The test values below let the ARC grow to half the memory and never
 * shrink it (GitHub issue #3).  A quarter of the memory at most, so the
 * whole process (the ARC, and the dbufs, dnodes and heap around it) stays
 * within the third the kernel driver allows, and memory given back when
 * Windows has less than arc_sys_free available (arc_init()).
 */
uint64_t
arc_default_max(uint64_t min, uint64_t allmem)
{
	return (MAX(allmem / 4, min));
}

uint64_t
arc_all_memory(void)
{
	return (ptob(physmem));
}

/*
 * Available memory (free and standby pages).  arc_adapt() asks for every
 * buffer it adds, so the system call is made at most every 10 ms.
 */
uint64_t
arc_free_memory(void)
{
	static volatile uint64_t avail, when;
	hrtime_t now = gethrtime();

	if (when == 0 || now - when > MSEC2NSEC(10)) {
		avail = ptob((uint64_t)sysconf(_SC_AVPHYS_PAGES));
		when = now;
	}
	return (avail);
}

int64_t
arc_available_memory(void)
{
	return ((int64_t)arc_free_memory() - (int64_t)arc_sys_free);
}

int
arc_memory_throttle(spa_t *spa, uint64_t reserve, uint64_t txg)
{
	(void) spa, (void) reserve, (void) txg;
	return (0);
}
#else

/*
 * Return a default max arc size based on the amount of physical memory.
 * This may be overridden by tuning the zfs_arc_max module parameter.
 */
uint64_t
arc_default_max(uint64_t min, uint64_t allmem)
{
	uint64_t size;

	if (allmem >= 1 << 30)
		size = allmem - (1 << 30);
	else
		size = min;
	return (MAX(allmem * 5 / 8, size));
}

int64_t
arc_available_memory(void)
{
	int64_t lowest = INT64_MAX;

	/* Every 100 calls, free a small amount */
	if (random_in_range(100) == 0)
		lowest = -1024;

	return (lowest);
}

int
arc_memory_throttle(spa_t *spa, uint64_t reserve, uint64_t txg)
{
	(void) spa, (void) reserve, (void) txg;
	return (0);
}

uint64_t
arc_all_memory(void)
{
	return (ptob(physmem) / 2);
}

uint64_t
arc_free_memory(void)
{
	return (random_in_range(arc_all_memory() * 20 / 100));
}
#endif /* _WIN32 */

void
arc_register_hotplug(void)
{
}

void
arc_unregister_hotplug(void)
{
}
