#ifndef HAVE_ENDIAN_H
	#define HAVE_ENDIAN_H

	unsigned short bswap_16(unsigned short x) {
		return (x >> 8) | (x << 8);
	}

	unsigned int bswap_32(unsigned int x) {
		return (bswap_16(x & 0xffff) << 16) | (bswap_16( x >> 16));
	}

	unsigned long long bswap_64(unsigned long long x) {
		return (((unsigned long long) bswap_32(x & 0xffffffffull)) << 32) | (bswap_32(x >> 32));
	}
#else
	void __endian_ok();
#endif
