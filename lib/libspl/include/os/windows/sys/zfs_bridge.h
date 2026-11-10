/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * CDDL HEADER END
 */

/*
 * The user-mode ZFS bridge.
 *
 * Without the kernel driver (no \\.\ZFS), the ZFS engine can run inside a
 * user-mode service instead (OpenZFS for Windows, user-mode edition).  It
 * answers the same ZFS ioctls on a local named pipe, so zpool.exe, zfs.exe
 * and everything built on libzfs work unchanged against either.
 *
 * One request per connection.  A request is a header followed by the
 * sections whose lengths it gives: the zfs_cmd_t, the src and conf
 * nvlists and the zc_history input string.  The response is a header
 * followed by the zfs_cmd_t, the zc_history output (POOL_GET_HISTORY) and
 * the zc_nvlist_dst contents, never longer than the capacities the
 * request named.  Buffer pointers inside zfs_cmd_t are the client's and
 * are only carried; the server substitutes its own.
 */

#ifndef _SPL_ZFS_BRIDGE_H
#define	_SPL_ZFS_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	ZFS_BRIDGE_PIPE		"\\\\.\\pipe\\OpenZFS-UserMode"
#define	ZFS_BRIDGE_PIPE_W	L"\\\\.\\pipe\\OpenZFS-UserMode"
#define	ZFS_BRIDGE_MAGIC	0x315a4252U	/* "RBZ1" */
#define	ZFS_BRIDGE_MAX_SECTION	(256ULL << 20)

#define	ZFS_BRIDGE_OP_IOCTL	1	/* zb_request: ZFS_IOC_* */
/* dst: "dataset\tmountpoint\tflags\n"..., flags 1 read-only, 2 noatime */
#define	ZFS_BRIDGE_OP_MNTTAB	2

/* Section indexes. */
#define	ZFS_BRIDGE_CMD		0
#define	ZFS_BRIDGE_SRC		1	/* request only */
#define	ZFS_BRIDGE_CONF		2	/* request only */
#define	ZFS_BRIDGE_HISTORY	3	/* request: input; response: output */
#define	ZFS_BRIDGE_DST		4	/* response only */
#define	ZFS_BRIDGE_SECTIONS	5

typedef struct zfs_bridge_hdr {
	uint32_t	zb_magic;
	uint32_t	zb_op;
	uint64_t	zb_request;
	int64_t		zb_error;	/* response: errno, 0 for success */
	uint64_t	zb_len[ZFS_BRIDGE_SECTIONS];
	uint64_t	zb_cap[ZFS_BRIDGE_SECTIONS];	/* request: room */
} zfs_bridge_hdr_t;

/*
 * Client side (libspl): one round trip.  sect[i] holds zb_len[i] bytes to
 * send; on return it holds the response's zb_len[i] bytes (at most
 * zb_cap[i], as the request said).  Returns 0, or the transport errno
 * (ENXIO when no service listens).
 */
extern int zfs_bridge_call(zfs_bridge_hdr_t *hdr,
    void *sect[ZFS_BRIDGE_SECTIONS]);

/* Whether an open(ZFS_DEV) descriptor is the bridge rather than \\.\ZFS. */
extern int zfs_bridge_is_fd(int fd);

#ifdef __cplusplus
}
#endif

#endif /* _SPL_ZFS_BRIDGE_H */
