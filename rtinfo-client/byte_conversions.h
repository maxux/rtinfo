/* Useful byte conversion macros, not available on all platforms.
 * Copyright (C) 2009-2010 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef __ENDIAN_H
	#define __ENDIAN_H

	#ifdef HAVE_ENDIAN_H
		#include <endian.h>
	#else
		#if BYTE_ORDER == LITTLE_ENDIAN
			#ifndef be16toh
			#define be16toh(x) bswap_16 (x)
			#endif
			#ifndef htobe16
			#define htobe16(x) bswap_16 (x)
			#endif
			#ifndef be32toh
			#define be32toh(x) bswap_32 (x)
			#endif
			#ifndef htobe32
			#define htobe32(x) bswap_32 (x)
			#endif
			#ifndef be64toh
			#define be64toh(x) bswap_64 (x)
			#endif
			#ifndef htobe64
			#define htobe64(x) bswap_64 (x)
			#endif
			#ifndef le16toh
			#define le16toh(x) (x)
			#endif
			#ifndef htole16
			#define htole16(x) (x)
			#endif
			#ifndef le32toh
			#define le32toh(x) (x)
			#endif
			#ifndef htole32
			#define htole32(x) (x)
			#endif
			#ifndef le64toh
			#define le64toh(x) (x)
			#endif
			#ifndef htole64
			#define htole64(x) (x)
			#endif
		#else /* BYTE_ORDER == BIG_ENDIAN */
			#ifndef be16toh
			#define be16toh(x) (x)
			#endif
			#ifndef htobe16
			#define htobe16(x) (x)
			#endif
			#ifndef be32toh
			#define be32toh(x) (x)
			#endif
			#ifndef htobe32
			#define htobe32(x) (x)
			#endif
			#ifndef be64toh
			#define be64toh(x) (x)
			#endif
			#ifndef htobe64
			#define htobe64(x) (x)
			#endif
			#ifndef le16toh
			#define le16toh(x) bswap_16 (x)
			#endif
			#ifndef htole16
			#define htole16(x) bswap_16 (x)
			#endif
			#ifndef le32toh
			#define le32toh(x) bswap_32 (x)
			#endif
			#ifndef htole32
			#define htole32(x) bswap_32 (x)
			#endif
			#ifndef le64toh
			#define le64toh(x) bswap_64 (x)
			#endif
			#ifndef htole64
			#define htole64(x) bswap_64 (x)
			#endif
		#endif

		unsigned long long bswap_64(unsigned long long x);
		unsigned int bswap_32(unsigned int x);
		unsigned short bswap_16(unsigned short x);
	#endif
#endif
