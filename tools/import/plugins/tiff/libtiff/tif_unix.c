/* $Id: tif_unix.c,v 1.12 2006/03/21 16:37:51 dron Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 *
 * BigTIFF modifications by Ole Eichhorn / Aperio Technologies (ole@aperio.com)
 *
 * 081016 vunger	Prototyped lseek() and lseek64()
 *
 */

/*
 * TIFF Library UNIX-specific Routines. These are should also work with the
 * Windows Common RunTime Library.
 */

#include "tif_config.h"

/*
 * Tell compiler to use 64-bit glibc functions.  off_t will be unsigned long long.
 */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#include "tiffiop.h"

#ifdef HAS_LSEEK64

// Ensure the proper prototype for lseek64()
// Without this, when it returns an offset of 0x80000000 or greater,
//   the MSBit will be extended (cast to a 64 bit signed int)
toff_t lseek64(int, toff_t, int);

#else

// Ensure proper prototype for lseek() (needed for apple/Darwin)
off_t lseek(int, off_t, int);

#endif



static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return ((tsize_t) read((int) fd, buf, (size_t) size));
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return ((tsize_t) write((int) fd, buf, (size_t) size));
}

#include <errno.h>

static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
#ifdef HAS_LSEEK64
	return ((toff_t) lseek64((int) fd, (toff_t) off, whence));
#else
	// On Apple/Darwin off_t is 64 bits
	return ((off_t) lseek((int) fd, (off_t) off, whence));
#endif
}

static int
_tiffCloseProc(thandle_t fd)
{
	return (close((int) fd));
}


static toff_t
_tiffSizeProc(thandle_t fd)
{
#ifdef HAS_FSTAT64
	struct stat64 sb;
	return (toff_t) (fstat64((int) fd, &sb) < 0 ? 0 : sb.st_size);
#else
	toff_t fsize;
	return ((fsize = lseek((int) fd, 0, SEEK_END)) < 0 ? 0 : fsize);
#endif
}

static int
_tiffErrnoProc(void)
{
	return errno;
}

#ifdef HAVE_MMAP
#include <sys/mman.h>

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	toff_t size = _tiffSizeProc(fd);
	if (size != (toff_t) -1) {
		*pbase = (tdata_t)
		    mmap(0, size, PROT_READ, MAP_SHARED, (int) fd, 0);
		if (*pbase != (tdata_t) -1) {
			*psize = size;
			return (1);
		}
	}
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) fd;
	(void) munmap(base, (toff_t) size);
}
#else /* !HAVE_MMAP */
static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) fd; (void) base; (void) size;
}
#endif /* !HAVE_MMAP */

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) fd,
	    _tiffReadProc, _tiffWriteProc,
	    _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
	    _tiffMapProc, _tiffUnmapProc);
	if (tif) {
		tif->tif_fd = fd;
		tif->tif_errnoproc = _tiffErrnoProc;
	}
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int m, fd;
        TIFF* tif;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);

/* for cygwin and mingw */        
#ifdef O_BINARY
        m |= O_BINARY;
#endif        
        
#ifdef HAS_OPEN64
	fd = open64(name, m, 0666);
#else
	fd = open(name, m);
#endif
	if (fd < 0) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	tif = TIFFFdOpen((int)fd, name, mode);
	if(!tif)
		close(fd);
	return tif;
}

#ifdef __WIN32__
#include <windows.h>
/*
 * Open a TIFF file with a Unicode filename, for read/writing.
 */
TIFF*
TIFFOpenW(const wchar_t* name, const char* mode)
{
	static const char module[] = "TIFFOpenW";
	int m, fd;
	int mbsize;
	char *mbname;
	TIFF* tif;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);

/* for cygwin and mingw */        
#ifdef O_BINARY
        m |= O_BINARY;
#endif        
        
	fd = _wopen(name, m, 0666);
	if (fd < 0) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	mbname = NULL;
	mbsize = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
	if (mbsize > 0) {
		mbname = _TIFFmalloc(mbsize);
		if (!mbname) {
			TIFFErrorExt(0, module,
			"Can't allocate space for filename conversion buffer");
			return ((TIFF*)0);
		}

		WideCharToMultiByte(CP_ACP, 0, name, -1, mbname, mbsize,
				    NULL, NULL);
	}

	tif = TIFFFdOpen((int)fd, (mbname != NULL) ? mbname : "<unknown>",
			 mode);
	
	_TIFFfree(mbname);
	
	if(!tif)
		close(fd);
	return tif;
}
#endif

void*
_TIFFmalloc(tsize_t s)
{
	return (malloc((size_t) s));
}

void
_TIFFfree(tdata_t p)
{
	free(p);
}

void*
_TIFFrealloc(tdata_t p, tsize_t s)
{
	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
unixWarningHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFwarningHandler = unixWarningHandler;

static void
unixErrorHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFerrorHandler = unixErrorHandler;
