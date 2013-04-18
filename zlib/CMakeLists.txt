
SET(ZLIB_FILES
	adler32.C
	compress.C
	crc32.C
	gzio.C
	uncompr.C
	deflate.C
	trees.C
	zutil.C
	inflate.C
	infblock.C
	inftrees.C
	infcodes.C
	infutil.C
	inffast.C)

ADD_LIBRARY(zlib ${ZLIB_FILES})

# Compile as C (not C++)
SET_SOURCE_FILES_PROPERTIES(${ZLIB_FILES} PROPERTIES LANGUAGE "C" COMPILE_FLAGS "-x c")

