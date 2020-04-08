/* $Id: tif_dirwrite.c,v 1.34 2006/02/23 16:07:45 dron Exp $ */

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
 */

/*
 * TIFF Library.
 *
 * Directory Write Support Routines.
 */
#include "tiffiop.h"

#ifdef HAVE_IEEEFP
# define	TIFFCvtNativeToIEEEFloat(tif, n, fp)
# define	TIFFCvtNativeToIEEEDouble(tif, n, dp)
#else
extern	void TIFFCvtNativeToIEEEFloat(TIFF*, uint32, float*);
extern	void TIFFCvtNativeToIEEEDouble(TIFF*, uint32, double*);
#endif

static	int TIFFWriteNormalTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*);
static	void TIFFSetupShortLong(TIFF*, ttag_t, TIFFDirEntry*, uint32);
static	void TIFFSetupShort(TIFF*, ttag_t, TIFFDirEntry*, uint16);
static	int TIFFSetupShortPair(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleShorts(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleAnys(TIFF*, TIFFDataType, ttag_t, TIFFDirEntry*);
static	int TIFFWriteShortTable(TIFF*, ttag_t, TIFFDirEntry*, uint32, uint16**);
static	int TIFFWriteShortArray(TIFF*, TIFFDirEntry*, uint16*);
static	int TIFFWriteLongArray(TIFF *, TIFFDirEntry*, uint32*);
static	int TIFFWriteLong8Array(TIFF *, TIFFDirEntry*, uint64*);
static	int TIFFWriteRationalArray(TIFF *, TIFFDirEntry*, float*);
static	int TIFFWriteFloatArray(TIFF *, TIFFDirEntry*, float*);
static	int TIFFWriteDoubleArray(TIFF *, TIFFDirEntry*, double*);
static	int TIFFWriteByteArray(TIFF*, TIFFDirEntry*, char*);
static	int TIFFWriteAnyArray(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, double*);
static	int TIFFWriteTransferFunction(TIFF*, TIFFDirEntry*);
static	int TIFFWriteInkNames(TIFF*, TIFFDirEntry*);
static	int TIFFWriteData(TIFF*, TIFFDirEntry*, char*);
static	int _TIFFPlaceArray(TIFF*, toff_t*, TIFFDirEntry*);
static	int TIFFLinkDirectory(TIFF*);
static	int TIFFMakeBigTIFF(TIFF*);

#define	WriteRationalPair(type, tag1, v1, tag2, v2) {		\
	TIFFWriteRational((tif), (type), (tag1), (dir), (v1))	\
	TIFFWriteRational((tif), (type), (tag2), (dir)+1, (v2))	\
	(dir)++;						\
}
#define	TIFFWriteRational(tif, type, tag, dir, v)		\
	(dir)->s.tdir_tag = (tag);				\
	(dir)->s.tdir_type = (type);				\
	TDIRSetEntryCount(tif,dir, 1);				\
	if (!TIFFWriteRationalArray((tif), (dir), &(v)))	\
		goto bad;

/*
 * Write the contents of the current directory
 * to the specified file.  This routine doesn't
 * handle overwriting a directory with auxiliary
 * storage that's been changed.
 */
static int
_TIFFWriteDirectory(TIFF* tif, int done)
{
	uint64 dircount;	/* 16-bit or 64-bit directory entry count */
	toff_t diroff;		/* 32-bit or 64-bit directory offset */
	uint32 nfields;
	tsize_t dirsize;
	char* data;
	TIFFDirEntry* dir;
	TIFFDirectory* td;
	unsigned long b, fields[FIELD_SETLONGS];
	int fi, nfi;

	if (tif->tif_mode == O_RDONLY)
		return (1);
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (done)
	{
	    if (tif->tif_flags & TIFF_POSTENCODE) {
		    tif->tif_flags &= ~TIFF_POSTENCODE;
		    if (!(*tif->tif_postencode)(tif)) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				"Error post-encoding before directory write");
			    return (0);
		    }
	    }
	    (*tif->tif_close)(tif);		/* shutdown encoder */
	    /*
	     * Flush any data that might have been written
	     * by the compression close+cleanup routines.
	     */
	    if (tif->tif_rawcc > 0 && !TIFFFlushData1(tif)) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"Error flushing data before directory write");
		    return (0);
	    }
	    if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata) {
		    _TIFFfree(tif->tif_rawdata);
		    tif->tif_rawdata = NULL;
		    tif->tif_rawcc = 0;
		    tif->tif_rawdatasize = 0;
	    }
	    tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);
	}

	/*
	 * Flush current offsets and byte counts blocks (if required).  Will
	 * convert offsets to 64-bit if required, but BigTIFF transition
	 * happens below.
	 */
	if (!_TIFFFlushOffsets(tif))
		goto bad2;
	if (!_TIFFFlushByteCounts(tif))
		goto bad2;

	/*
	 * Loop to allow retry in case transition to BigTIFF required
	 */
	while (1) {

	/*
	 * Directory hasn't been placed yet, put
	 * it at the end of the file and link it
	 * into the existing directory structure.
	 */
	if (tif->tif_diroff == 0 && !TIFFLinkDirectory(tif))
		goto bad2;

	td = &tif->tif_dir;
	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= FIELD_LAST; b++)
		if (TIFFFieldSet(tif, b) && b != FIELD_CUSTOM)
			nfields += (b < FIELD_SUBFILETYPE ? 2 : 1);
        nfields += td->td_customValueCount;
	dirsize = nfields * TDIREntryLen(tif);
	data = (char*) _TIFFmalloc(dirsize);
	if (data == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "Cannot write directory, out of space");
		goto bad2;
	}
	_TIFFmemset(data, 0, dirsize);
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	_TIFFmemcpy(fields, td->td_fieldsset, sizeof (fields));
	/*
	 * Write out ExtraSamples tag only if
	 * extra samples are present in the data.
	 */
	if (FieldSet(fields, FIELD_EXTRASAMPLES) && !td->td_extrasamples) {
		ResetFieldBit(fields, FIELD_EXTRASAMPLES);
		nfields--;
		dirsize -= TDIREntryLen(tif);
	}								/*XXX*/

	tif->tif_dataoff = tif->tif_diroff + TIFFDirCntLen(tif) + dirsize + TIFFDirOffLen(tif);
	(void) TIFFSeekFile(tif, tif->tif_dataoff, SEEK_SET);
	dir = (TIFFDirEntry*) data;
	for (fi = 0, nfi = tif->tif_nfields; nfi > 0; nfi--, fi++) {
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[fi];

                /*
                ** For custom fields, we test to see if the custom field
                ** is set or not.  For normal fields, we just use the
                ** FieldSet test. 
                */
                if( fip->field_bit == FIELD_CUSTOM )
                {
                    int ci, is_set = FALSE;

                    for( ci = 0; ci < td->td_customValueCount; ci++ )
                        is_set |= (td->td_customValues[ci].info == fip);

                    if( !is_set )
                        continue;
                }
		else if (!FieldSet(fields, fip->field_bit))
                    continue;

                /*
                ** Handle other fields.
                */
		switch (fip->field_bit)
                {
		case FIELD_STRIPOFFSETS:
			dir->s.tdir_tag = (isTiled(tif) ? TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS);
			dir->s.tdir_type = td->td_stripoffstype;
			TDIRSetEntryCount(tif, dir, td->td_stripoffscnt);
			TDIRSetEntryOff(tif, dir, td->td_stripoffsoff);
			break;
		case FIELD_STRIPBYTECOUNTS:
			dir->s.tdir_tag = (isTiled(tif) ? TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS);
			dir->s.tdir_type = td->td_stripbcstype;
			TDIRSetEntryCount(tif, dir, td->td_stripbcscnt);
			TDIRSetEntryOff(tif, dir, td->td_stripbcsoff);
			break;
		case FIELD_ROWSPERSTRIP:
			TIFFSetupShortLong(tif, TIFFTAG_ROWSPERSTRIP,
			    dir, td->td_rowsperstrip);
			break;
		case FIELD_COLORMAP:
			if (!TIFFWriteShortTable(tif, TIFFTAG_COLORMAP, dir,
			    3, td->td_colormap))
				goto bad;
			break;
		case FIELD_IMAGEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_IMAGEWIDTH,
			    dir, td->td_imagewidth);
			TDIREntryNext(tif,dir);
			TIFFSetupShortLong(tif, TIFFTAG_IMAGELENGTH,
			    dir, td->td_imagelength);
			break;
		case FIELD_TILEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_TILEWIDTH,
			    dir, td->td_tilewidth);
			TDIREntryNext(tif,dir);
			TIFFSetupShortLong(tif, TIFFTAG_TILELENGTH,
			    dir, td->td_tilelength);
			break;
		case FIELD_COMPRESSION:
			TIFFSetupShort(tif, TIFFTAG_COMPRESSION,
			    dir, td->td_compression);
			break;
		case FIELD_PHOTOMETRIC:
			TIFFSetupShort(tif, TIFFTAG_PHOTOMETRIC,
			    dir, td->td_photometric);
			break;
		case FIELD_POSITION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XPOSITION, td->td_xposition,
			    TIFFTAG_YPOSITION, td->td_yposition);
			break;
		case FIELD_RESOLUTION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XRESOLUTION, td->td_xresolution,
			    TIFFTAG_YRESOLUTION, td->td_yresolution);
			break;
		case FIELD_BITSPERSAMPLE:
		case FIELD_MINSAMPLEVALUE:
		case FIELD_MAXSAMPLEVALUE:
		case FIELD_SAMPLEFORMAT:
			if (!TIFFWritePerSampleShorts(tif, fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_SMINSAMPLEVALUE:
		case FIELD_SMAXSAMPLEVALUE:
			if (!TIFFWritePerSampleAnys(tif,
			    _TIFFSampleToTagType(tif), fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_PAGENUMBER:
		case FIELD_HALFTONEHINTS:
		case FIELD_YCBCRSUBSAMPLING:
			if (!TIFFSetupShortPair(tif, fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_INKNAMES:
			if (!TIFFWriteInkNames(tif, dir))
				goto bad;
			break;
		case FIELD_TRANSFERFUNCTION:
			if (!TIFFWriteTransferFunction(tif, dir))
				goto bad;
			break;
		case FIELD_SUBIFD:
			/*
			 * Total hack: if this directory includes a SubIFD
			 * tag then force the next <n> directories to be
			 * written as ``sub directories'' of this one.  This
			 * is used to write things like thumbnails and
			 * image masks that one wants to keep out of the
			 * normal directory linkage access mechanism.
			 *
			 * If file is BigTIFF subdirectory offsets are written as 64-bit, 
			 * else as array of 32-bit offsets.  At this point the actual
			 * offset values are unknown (will be set in TIFFLinkDirectory).
			 */
			dir->s.tdir_tag = (uint16) fip->field_tag;
			TDIRSetEntryCount(tif,dir, (uint32) td->td_nsubifd);
			TDIRSetEntryOff(tif,dir, 0);
			if (td->td_nsubifd > 0) {
				tif->tif_flags |= TIFF_INSUBIFD;
				if (isBigTIFF(tif)) {
					toff_t *doff = _TIFFCheckMalloc(tif, td->td_nsubifd, sizeof(toff_t),
						"Allocate SubIFD offset array");
					if (!doff) 
						goto bad;
					_TIFFmemset(doff, 0, td->td_nsubifd * sizeof(toff_t));
					dir->b.tdir_type = TIFF_IFD8;
					if (td->td_nsubifd > TIFFDirOffLen(tif) / sizeof(toff_t))
						tif->tif_subifdoff = tif->tif_dataoff;
					else
						tif->tif_subifdoff = tif->tif_diroff + TIFFDirCntLen(tif) + 
							(char*) &dir->b.tdir_offset - data;
					if (!TIFFWriteLong8Array(tif, dir, (toff_t*) doff))
						goto bad;
					_TIFFfree(doff);
				} else {
					uint32 *doff = _TIFFCheckMalloc(tif, td->td_nsubifd, sizeof(uint32),
						"Allocate SubIFD offset array");
					if (!doff) 
						goto bad;
					_TIFFmemset(doff, 0, td->td_nsubifd * sizeof(uint32));
					dir->b.tdir_type = TIFF_LONG;
					if (td->td_nsubifd > TIFFDirOffLen(tif) / sizeof(uint32))
						tif->tif_subifdoff = tif->tif_dataoff;
					else
						tif->tif_subifdoff = tif->tif_diroff + TIFFDirCntLen(tif) + 
							(char*) &dir->s.tdir_offset - data;
					if (!TIFFWriteLongArray(tif, dir, doff))
						goto bad;
					_TIFFfree(doff);
				}
			}
			break;
		default:
			/* XXX: Should be fixed and removed. */
			if (fip->field_tag == TIFFTAG_DOTRANGE) {
				if (!TIFFSetupShortPair(tif, fip->field_tag, dir))
					goto bad;
			}
			else if (!TIFFWriteNormalTag(tif, dir, fip))
				goto bad;
			break;
		}
		TDIREntryNext(tif,dir);
                
                if( fip->field_bit != FIELD_CUSTOM )
                    ResetFieldBit(fields, fip->field_bit);
	}

	/*
	 * Check if BigTIFF now required based on data location.  If so reset 
	 * data offset to overwrite already-written data for directory, then
	 * convert file to BigTIFF and loop to recreate directory.
	 */
	if (isBigTIFF(tif) || !isBigOff(tif->tif_dataoff + sizeof(uint32)))
		break;

	_TIFFfree(data);
	tif->tif_dataoff = tif->tif_diroff;
	if (!TIFFMakeBigTIFF(tif))
		goto bad2;
	tif->tif_diroff = 0;
	}			/* bottom of BigTIFF retry loop */

	/*
	 * Write directory.
	 */
	tif->tif_curdir++;
	TIFFSetDirCnt(tif,dircount, (uint32) nfields);
	TIFFSetDirOff(tif,diroff,tif->tif_nextdiroff);
	if (tif->tif_flags & TIFF_SWAB) {
		/*
		 * The file's byte order is opposite to the
		 * native machine architecture.  We overwrite
		 * the directory information with impunity
		 * because it'll be released below after we
		 * write it to the file.  Note that all the
		 * other tag construction routines assume that
		 * we do this byte-swapping; i.e. they only
		 * byte-swap indirect data.
		 */
		uint32 di;
		for (dir = (TIFFDirEntry*) data, di = 0; di < nfields; TDIREntryNext(tif,dir), di++) {
			TIFFSwabShort(&dir->s.tdir_tag);
			TIFFSwabShort(&dir->s.tdir_type);
			TDIRSwabEntryCount(tif,dir);
			TDIRSwabEntryOff(tif,dir);
		}
		TIFFSwabDirCnt(tif,&dircount);
		TIFFSwabDirOff(tif,&diroff);
	}
	(void) TIFFSeekFile(tif, tif->tif_diroff, SEEK_SET);
	if (!WriteOK(tif, &dircount, TIFFDirCntLen(tif))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
			"Error writing directory count (%d)", TIFFGetErrno(tif));
		goto bad;
	}
	if (!WriteOK(tif, data, dirsize)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
			"Error writing directory contents (%d)", TIFFGetErrno(tif));
		goto bad;
	}
	if (!WriteOK(tif, &diroff, TIFFDirOffLen(tif))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
			"Error writing directory link (%d)", TIFFGetErrno(tif));
		goto bad;
	}
	if (done) {
		TIFFFreeDirectory(tif);
		tif->tif_flags &= ~TIFF_DIRTYDIRECT;
		(*tif->tif_cleanup)(tif);

		/*
		* Reset directory-related state for subsequent
		* directories.
		*/
		TIFFCreateDirectory(tif);
	}
	_TIFFfree(data);
	return (1);
bad:
	_TIFFfree(data);
bad2:
	return (0);
}
#undef WriteRationalPair

int
TIFFWriteDirectory(TIFF* tif)
{
	return _TIFFWriteDirectory(tif, TRUE);
}

/*
 * Similar to TIFFWriteDirectory(), writes the directory out
 * but leaves all data structures in memory so that it can be
 * written again.  This will make a partially written TIFF file
 * readable before it is successfully completed/closed.
 */ 
int
TIFFCheckpointDirectory(TIFF* tif)
{
	int rc;
	/* Setup the strips arrays, if they haven't already been. */
	if (tif->tif_dir.td_stripoffsbuf == NULL)
	    (void) TIFFSetupStrips(tif);
	rc = _TIFFWriteDirectory(tif, FALSE);
	(void) TIFFSetWriteOffset(tif, TIFFSeekFile(tif, 0, SEEK_END));
	return rc;
}

/*
 * Process tags that are not special cased.
 * TIFF_LONG8, TIFF_SLONG8, and TIFF_IFD8 are not supported.
 */
static int
TIFFWriteNormalTag(TIFF* tif, TIFFDirEntry* dir, const TIFFFieldInfo* fip)
{
	uint16 wc = (uint16) fip->field_writecount;
	uint32 wc2;

	dir->s.tdir_tag = (uint16) fip->field_tag;
	dir->s.tdir_type = (uint16) fip->field_type;
	TDIRSetEntryCount(tif,dir, wc);
	
	switch (fip->field_type) {
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (fip->field_passcount) {
			uint16* wp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &wp);
				TDIRSetEntryCount(tif,dir, wc2);
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &wp);
				TDIRSetEntryCount(tif,dir, wc);
			}
			if (!TIFFWriteShortArray(tif, dir, wp))
				return 0;
		} else {
			if (wc == 1) {
				uint16 sv;
				TIFFGetField(tif, fip->field_tag, &sv);
				TIFFWriteShortArray(tif, dir, &sv);
			} else {
				uint16* wp;
				TIFFGetField(tif, fip->field_tag, &wp);
				if (!TIFFWriteShortArray(tif, dir, wp))
					return 0;
			}
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
	case TIFF_IFD:
		if (fip->field_passcount) {
			uint32* lp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &lp);
				TDIRSetEntryCount(tif,dir, wc2);
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &lp);
				TDIRSetEntryCount(tif,dir, wc);
			}
			if (!TIFFWriteLongArray(tif, dir, lp))
				return 0;
		} else {
			if (wc == 1) {
				uint32 wp;
				/* XXX handle LONG->SHORT conversion */
				TIFFGetField(tif, fip->field_tag, &wp);
				TDIRSetEntryOff(tif,dir, wp);
			} else {
				uint32* lp;
				TIFFGetField(tif, fip->field_tag, &lp);
				if (!TIFFWriteLongArray(tif, dir, lp))
					return 0;
			}
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (fip->field_passcount) {
			float* fp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &fp);
				TDIRSetEntryCount(tif,dir, wc2);
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &fp);
				TDIRSetEntryCount(tif,dir, wc);
			}
			if (!TIFFWriteRationalArray(tif, dir, fp))
				return 0;
		} else {
			if (wc == 1) {
				float fv;
				TIFFGetField(tif, fip->field_tag, &fv);
				if (!TIFFWriteRationalArray(tif, dir, &fv))
					return 0;
			} else {
				float* fp;
				TIFFGetField(tif, fip->field_tag, &fp);
				if (!TIFFWriteRationalArray(tif, dir, fp))
					return 0;
			}
		}
		break;
	case TIFF_FLOAT:
		if (fip->field_passcount) {
			float* fp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &fp);
				TDIRSetEntryCount(tif,dir, wc2);
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &fp);
				TDIRSetEntryCount(tif,dir, wc);
			}
			if (!TIFFWriteFloatArray(tif, dir, fp))
				return 0;
		} else {
			if (wc == 1) {
				float fv;
				TIFFGetField(tif, fip->field_tag, &fv);
				if (!TIFFWriteFloatArray(tif, dir, &fv))
					return 0;
			} else {
				float* fp;
				TIFFGetField(tif, fip->field_tag, &fp);
				if (!TIFFWriteFloatArray(tif, dir, fp))
					return 0;
			}
		}
		break;
	case TIFF_DOUBLE:
		if (fip->field_passcount) {
			double* dp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &dp);
				TDIRSetEntryCount(tif,dir, wc2);
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &dp);
				TDIRSetEntryCount(tif,dir, wc);
			}
			if (!TIFFWriteDoubleArray(tif, dir, dp))
				return 0;
		} else {
			if (wc == 1) {
				double dv;
				TIFFGetField(tif, fip->field_tag, &dv);
				if (!TIFFWriteDoubleArray(tif, dir, &dv))
					return 0;
			} else {
				double* dp;
				TIFFGetField(tif, fip->field_tag, &dp);
				if (!TIFFWriteDoubleArray(tif, dir, dp))
					return 0;
			}
		}
		break;
	case TIFF_ASCII:
		{ 
                    char* cp;
                    if (fip->field_passcount)
                        TIFFGetField(tif, fip->field_tag, &wc, &cp);
                    else
                        TIFFGetField(tif, fip->field_tag, &cp);

		    TDIRSetEntryCount(tif,dir, (uint32) (strlen(cp) + 1));
                    if (!TIFFWriteByteArray(tif, dir, cp))
                        return (0);
		}
		break;

        case TIFF_BYTE:
        case TIFF_SBYTE:          
		if (fip->field_passcount) {
			char* cp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &cp);
				TDIRSetEntryCount(tif,dir, wc2);
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &cp);
				TDIRSetEntryCount(tif,dir, wc);
			}
			if (!TIFFWriteByteArray(tif, dir, cp))
				return 0;
		} else {
			if (wc == 1) {
				char cv;
				TIFFGetField(tif, fip->field_tag, &cv);
				if (!TIFFWriteByteArray(tif, dir, &cv))
					return 0;
			} else {
				char* cp;
				TIFFGetField(tif, fip->field_tag, &cp);
				if (!TIFFWriteByteArray(tif, dir, cp))
					return 0;
			}
		}
                break;

	case TIFF_UNDEFINED:
		{ char* cp;
		  if (wc == (unsigned short) TIFF_VARIABLE) {
			TIFFGetField(tif, fip->field_tag, &wc, &cp);
			TDIRSetEntryCount(tif,dir, wc);
		  } else if (wc == (unsigned short) TIFF_VARIABLE2) {
			TIFFGetField(tif, fip->field_tag, &wc2, &cp);
			TDIRSetEntryCount(tif,dir, wc2);
		  } else 
			TIFFGetField(tif, fip->field_tag, &cp);
		  if (!TIFFWriteByteArray(tif, dir, cp))
			return (0);
		}
		break;

        case TIFF_NOTYPE:
                break;
	}
	return (1);
}

/*
 * Setup a directory entry with either a SHORT
 * or LONG type according to the value.
 */
static void
TIFFSetupShortLong(TIFF* tif, ttag_t tag, TIFFDirEntry* dir, uint32 v)
{
	dir->s.tdir_tag = (uint16) tag;
	TDIRSetEntryCount(tif,dir, 1);
	if (v > 0xffffL) {
		dir->s.tdir_type = TIFF_LONG;
		TDIRSetEntryOff(tif,dir, v);
	} else {
		uint16 iv = (uint16) v;
		dir->s.tdir_type = TIFF_SHORT;
		TIFFWriteShortArray(tif, dir, &iv);
	}
}

/*
 * Setup a SHORT directory entry
 */
static void
TIFFSetupShort(TIFF* tif, ttag_t tag, TIFFDirEntry* dir, uint16 v)
{
	dir->s.tdir_tag = (uint16) tag;
	TDIRSetEntryCount(tif,dir, 1);
	dir->s.tdir_type = TIFF_SHORT;
	TIFFWriteShortArray(tif, dir, &v);
}
#undef MakeShortDirent

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Setup a directory entry that references a
 * samples/pixel array of SHORT values and
 * (potentially) write the associated indirect
 * values.
 */
static int
TIFFWritePerSampleShorts(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 buf[10], v;
	uint16* w = buf;
	uint16 i, samples = tif->tif_dir.td_samplesperpixel;
	int status;

	if (samples > NITEMS(buf)) {
		w = (uint16*) _TIFFmalloc(samples * sizeof (uint16));
		if (w == NULL) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "No space to write per-sample shorts");
			return (0);
		}
	}
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	
	dir->s.tdir_tag = (uint16) tag;
	dir->s.tdir_type = (uint16) TIFF_SHORT;
	TDIRSetEntryCount(tif,dir, samples);
	status = TIFFWriteShortArray(tif, dir, w);
	if (w != buf)
		_TIFFfree((char*) w);
	return (status);
}

/*
 * Setup a directory entry that references a samples/pixel array of ``type''
 * values and (potentially) write the associated indirect values.  The source
 * data from TIFFGetField() for the specified tag must be returned as double.
 */
static int
TIFFWritePerSampleAnys(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir)
{
	double buf[10], v;
	double* w = buf;
	uint16 i, samples = tif->tif_dir.td_samplesperpixel;
	int status;

	if (samples > NITEMS(buf)) {
		w = (double*) _TIFFmalloc(samples * sizeof (double));
		if (w == NULL) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "No space to write per-sample values");
			return (0);
		}
	}
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteAnyArray(tif, type, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree(w);
	return (status);
}
#undef NITEMS

/*
 * Setup a pair of shorts that are returned by
 * value, rather than as a reference to an array.
 */
static int
TIFFSetupShortPair(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 v[2];

	TIFFGetField(tif, tag, &v[0], &v[1]);

	dir->s.tdir_tag = (uint16) tag;
	dir->s.tdir_type = (uint16) TIFF_SHORT;
	TDIRSetEntryCount(tif,dir, 2);
	return (TIFFWriteShortArray(tif, dir, v));
}

/*
 * Setup a directory entry for an NxM table of shorts,
 * where M is known to be 2**bitspersample, and write
 * the associated indirect data.
 */
static int
TIFFWriteShortTable(TIFF* tif,
    ttag_t tag, TIFFDirEntry* dir, uint32 n, uint16** table)
{
	uint32 i;

	dir->s.tdir_tag = (uint16) tag;
	dir->s.tdir_type = (short) TIFF_SHORT;
	/* XXX -- yech, fool TIFFWriteData */
	TDIRSetEntryCount(tif,dir, (uint32) (1L<<tif->tif_dir.td_bitspersample));
	for (i = 0; i < n; i++)
		if (!TIFFWriteData(tif, dir, (char *)table[i]))
			return (0);
	TDIRSetEntryCount(tif,dir, TDIRGetEntryCount(tif,dir) * n);
	return (1);
}

/*
 * Write/copy data associated with an ASCII or opaque tag value.
 */
static int
TIFFWriteByteArray(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif)) {
		_TIFFmemcpy(TDIRAddrEntryOff(tif,dir), cp, 
			TDIRGetEntryCount(tif,dir));
		return (1);
	}
	return (TIFFWriteData(tif, dir, cp));
}

/*
 * Setup a directory entry of an array of SHORT
 * or SSHORT and write the associated indirect values.
 */
static int
TIFFWriteShortArray(TIFF* tif, TIFFDirEntry* dir, uint16* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(uint16)) {
		_TIFFmemcpy(TDIRAddrEntryOff(tif,dir), v, 
			TDIRGetEntryCount(tif,dir) * sizeof(uint16));
		return (1);
	}
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of LONG
 * or SLONG and write the associated indirect values.
 */
static int
TIFFWriteLongArray(TIFF* tif, TIFFDirEntry* dir, uint32* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(uint32)) {
		_TIFFmemcpy(TDIRAddrEntryOff(tif,dir), v, 
			TDIRGetEntryCount(tif,dir) * sizeof(uint32));
		return (1);
	}
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of LONG8 or SLONG8
 * and write the associated indirect values.
 */
static int
TIFFWriteLong8Array(TIFF* tif, TIFFDirEntry* dir, uint64* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(uint64)) {
		_TIFFmemcpy(TDIRAddrEntryOff(tif,dir), v, 
			TDIRGetEntryCount(tif,dir) * sizeof(uint64));
		return (1);
	}
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of RATIONAL
 * or SRATIONAL and write the associated indirect values.
 */
static int
TIFFWriteRationalArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	uint32 i;
	uint32* t;
	int status;

	t = (uint32*) _TIFFmalloc(2 * TDIRGetEntryCount(tif,dir) * sizeof (uint32));
	if (t == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "No space to write RATIONAL array");
		return (0);
	}
	for (i = 0; i < TDIRGetEntryCount(tif,dir); i++) {
		float fv = v[i];
		int sign = 1;
		uint32 den;

		if (fv < 0) {
			if (dir->s.tdir_type == TIFF_RATIONAL) {
				TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
					"\"%s\": Information lost writing value (%g) as (unsigned) RATIONAL",
					_TIFFFieldWithTag(tif,dir->s.tdir_tag)->field_name,
					fv);
				fv = 0;
			} else
				fv = -fv, sign = -1;
		}
		den = 1L;
		if (fv > 0) {
			while (fv < 1L<<(31-3) && den < 1L<<(31-3))
				fv *= 1<<3, den *= 1L<<3;
		}
		t[2*i+0] = (uint32) (sign * (fv + 0.5));
		t[2*i+1] = den;
	}
	status = TIFFWriteData(tif, dir, (char *)t);
	_TIFFfree((char*) t);
	return (status);
}

static int
TIFFWriteFloatArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	TIFFCvtNativeToIEEEFloat(tif, dir->s.tdir_count, v);
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(float)) {
		_TIFFmemcpy(TDIRAddrEntryOff(tif,dir), v, 
			TDIRGetEntryCount(tif,dir) * sizeof(float));
		return (1);
	}
	return (TIFFWriteData(tif, dir, (char*) v));
}

static int
TIFFWriteDoubleArray(TIFF* tif, TIFFDirEntry* dir, double* v)
{
	TIFFCvtNativeToIEEEDouble(tif, dir->s.tdir_count, v);
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(double)) {
		_TIFFmemcpy(TDIRAddrEntryOff(tif,dir), v, 
			TDIRGetEntryCount(tif,dir) * sizeof(double));
		return (1);
	}
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Write an array of ``type'' values for a specified tag (i.e. this is a tag
 * which is allowed to have different types, e.g. SMaxSampleType).
 * Internally the data values are represented as double since a double can
 * hold any of the TIFF tag types (yes, this should really be an abstract
 * type tany_t for portability).  The data is converted into the specified
 * type in a temporary buffer and then handed off to the appropriate array
 * writer.
 *
 * This routine does not support any of the 64-bit types.
 */
static int
TIFFWriteAnyArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, double* v)
{
	char buf[10 * sizeof(double)];
	char* w = buf;
	int i, status = 0;

	if (n * TIFFDataWidth(type) > sizeof buf) {
		w = (char*) _TIFFmalloc(n * TIFFDataWidth(type));
		if (w == NULL) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "No space to write array");
			return (0);
		}
	}

	dir->s.tdir_tag = (uint16) tag;
	dir->s.tdir_type = (uint16) type;
	TDIRSetEntryCount(tif,dir, n);

	switch (type) {
	case TIFF_BYTE:
		{ 
			uint8* bp = (uint8*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (uint8) v[i];
			if (!TIFFWriteByteArray(tif, dir, (char*) bp))
				goto out;
		}
		break;
	case TIFF_SBYTE:
		{ 
			int8* bp = (int8*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (int8) v[i];
			if (!TIFFWriteByteArray(tif, dir, (char*) bp))
				goto out;
		}
		break;
	case TIFF_SHORT:
		{
			uint16* bp = (uint16*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (uint16) v[i];
			if (!TIFFWriteShortArray(tif, dir, (uint16*)bp))
				goto out;
		}
		break;
	case TIFF_SSHORT:
		{ 
			int16* bp = (int16*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (int16) v[i];
			if (!TIFFWriteShortArray(tif, dir, (uint16*)bp))
				goto out;
		}
		break;
	case TIFF_LONG:
		{
			uint32* bp = (uint32*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (uint32) v[i];
			if (!TIFFWriteLongArray(tif, dir, bp))
				goto out;
		}
		break;
	case TIFF_SLONG:
		{
			int32* bp = (int32*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (int32) v[i];
			if (!TIFFWriteLongArray(tif, dir, (uint32*) bp))
				goto out;
		}
		break;
	case TIFF_FLOAT:
		{ 
			float* bp = (float*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (float) v[i];
			if (!TIFFWriteFloatArray(tif, dir, bp))
				goto out;
		}
		break;
	case TIFF_DOUBLE:
		return (TIFFWriteDoubleArray(tif, dir, v));
	default:
		/* TIFF_NOTYPE */
		/* TIFF_ASCII */
		/* TIFF_UNDEFINED */
		/* TIFF_RATIONAL */
		/* TIFF_SRATIONAL */
		goto out;
	}
	status = 1;
 out:
	if (w != buf)
		_TIFFfree(w);
	return (status);
}

static int
TIFFWriteTransferFunction(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;
	tsize_t n = (tsize_t) (1L << td->td_bitspersample) * sizeof (uint16);
	uint16** tf = td->td_transferfunction;
	int ncols;

	/*
	 * Check if the table can be written as a single column,
	 * or if it must be written as 3 columns.  Note that we
	 * write a 3-column tag if there are 2 samples/pixel and
	 * a single column of data won't suffice--hmm.
	 */
	switch (td->td_samplesperpixel - td->td_extrasamples) {
	default:	if (_TIFFmemcmp(tf[0], tf[2], n)) { ncols = 3; break; }
	case 2:		if (_TIFFmemcmp(tf[0], tf[1], n)) { ncols = 3; break; }
	case 1: case 0:	ncols = 1;
	}
	return (TIFFWriteShortTable(tif,
	    TIFFTAG_TRANSFERFUNCTION, dir, ncols, tf));
}

static int
TIFFWriteInkNames(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;

	dir->s.tdir_tag = TIFFTAG_INKNAMES;
	dir->s.tdir_type = (short) TIFF_ASCII;
	TDIRSetEntryCount(tif,dir, td->td_inknameslen);
	return (TIFFWriteByteArray(tif, dir, td->td_inknames));
}

/*
 * Write a contiguous directory item.
 */
static int
TIFFWriteData(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	tsize_t cc;

	if (tif->tif_flags & TIFF_SWAB) {
		switch (dir->s.tdir_type) {
		case TIFF_SHORT:
		case TIFF_SSHORT:
			TIFFSwabArrayOfShort((uint16*) cp, TDIRGetEntryCount(tif,dir));
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_IFD:
		case TIFF_FLOAT:
			TIFFSwabArrayOfLong((uint32*) cp, TDIRGetEntryCount(tif,dir));
			break;
		case TIFF_LONG8:
		case TIFF_SLONG8:
		case TIFF_IFD8:
			TIFFSwabArrayOfLong8((uint64*) cp, TDIRGetEntryCount(tif,dir));
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			TIFFSwabArrayOfLong((uint32*) cp, 2 * TDIRGetEntryCount(tif,dir));
			break;
		case TIFF_DOUBLE:
			TIFFSwabArrayOfDouble((double*) cp, TDIRGetEntryCount(tif,dir));
			break;
		}
	}
	TDIRSetEntryOff(tif,dir,tif->tif_dataoff);
	cc = TDIRGetEntryCount(tif,dir) * TIFFDataWidth((TIFFDataType) dir->s.tdir_type);
	if (SeekOK(tif, tif->tif_dataoff) &&
	    WriteOK(tif, cp, cc)) {
		tif->tif_dataoff += ((cc + 1) & ~1);
		return (1);
	}
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
		"Error writing data for field \"%s\" (%d)",
		_TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name, TIFFGetErrno(tif));
	return (0);
}

/*
 * Set strip/tile offset.  Offsets are accumulated in "blocks"; if strip/tile's
 * block is not current, then write previous block (if dirty), and read new block.
 */
extern void
_TIFFSetOffset(TIFF* tif, tstrip_t strip, toff_t offset)
{
	TIFFDirectory *td = &tif->tif_dir;
	uint32	blkno = strip / td->td_stripbufmax;

	if (strip >= td->td_nstrips)	/* range check strip/tile number */
		return;
	/*
	 * If need to switch to 64-bit offsets, check if have already placed offsets array,
	 * if so array must be converted.  Then start using 64-bit offsets.
	 */
	if (isBigOff(offset) && td->td_stripoffstype != TIFF_LONG8) {
		if (td->td_stripoffsoff) {
			/*
			 * Loop through all used offsets blocks, read from old (32-bit) array, 
			 * and write to new (64-bit) array.  The old array is abandoned in place.
			 */
			uint16	oldoffstype = td->td_stripoffstype;
			toff_t	oldoffsoff = td->td_stripoffsoff;
			toff_t	newoffsoff = 0;		/* force place on first flush */
			uint32	tdi;

			for (tdi = 0; tdi < TIFFhowmany(td->td_stripoffscnt, td->td_stripbufmax); tdi++) {
				/* read old 32-bit offsets block */
				td->td_stripoffstype = oldoffstype;
				td->td_stripoffsoff = oldoffsoff;
				_TIFFGetOffset(tif, tdi * td->td_stripbufmax);
				/* write new 64-bit offsets block */
				td->td_stripoffstype = TIFF_LONG8;
				td->td_stripoffsoff = newoffsoff;
				td->td_stripoffsdirty = 1;
				_TIFFFlushOffsets(tif);
				newoffsoff = td->td_stripoffsoff;
			}
		}
		td->td_stripoffstype = TIFF_LONG8;
	}
	/*
	 * Check if strip/tile's block is already loaded, if not, write out previous
	 * block and then load / initialize strip/tile's block.
	 */
	if (blkno != td->td_stripoffsblk)
		_TIFFGetOffset(tif, strip);	/* getting offset causes block to be loaded / init'ed */
	/*
	 * Set strip/tile's offset
	 */
	td->td_stripoffsbuf[strip % td->td_stripbufmax] = offset;
	td->td_stripoffsdirty = 1;	
	if (strip >= td->td_stripoffscnt)
		td->td_stripoffscnt = strip + 1;
}

/*
 * Write out current offsets block if dirty.  Allocate space for offsets array in file
 * if necessary (possibly converting file to BigTIFF in the process!)
 */
extern int
_TIFFFlushOffsets(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	int status = 1;
	uint32	tdi;
		
	if (td->td_stripoffsdirty) {			/* if need to write current block */
		/* write current block */
		TIFFDirEntry	offsent;		/* fake entry to write offsets blocks */

		if (!td->td_stripoffstype)
			td->td_stripoffstype = (isBigTIFF(tif) ? TIFF_LONG8 : TIFF_LONG);
		offsent.s.tdir_tag = (isTiled(tif) ? TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS);
		offsent.s.tdir_type = td->td_stripoffstype;
		/* place array in file if necessary */
		if (!_TIFFPlaceArray(tif, &td->td_stripoffsoff, &offsent))  
			return 0;
		TDIRSetEntryCount(tif, &offsent, TIFFmin(td->td_stripbufmax, 
					td->td_nstrips - td->td_stripoffsblk * td->td_stripbufmax));
		tif->tif_dataoff = td->td_stripoffsoff + 
				td->td_stripoffsblk * td->td_stripbufmax * TIFFDataWidth(offsent.s.tdir_type);
		if (offsent.s.tdir_type == TIFF_LONG8) {
			status = TIFFWriteLong8Array(tif, &offsent, td->td_stripoffsbuf);
		} else {	/* TIFF_LONG */
			for (tdi = 0; tdi < TDIRGetEntryCount(tif, &offsent); tdi++)
				((uint32*) td->td_stripoffsbuf)[tdi] = (uint32) td->td_stripoffsbuf[tdi];
			status = TIFFWriteLongArray(tif, &offsent, (uint32*) td->td_stripoffsbuf);
		}
		td->td_stripoffsdirty = 0;
	}
	return status;
}

/*
 * Set strip/tile byte count.  Byte counts are accumulated in "blocks"; if strip/tile's
 * block is not current, then write previous block (if dirty), and read new block.
 */
extern void
_TIFFSetByteCount(TIFF* tif, tstrip_t strip, uint32 bytecount)
{
	TIFFDirectory *td = &tif->tif_dir;
	uint32	blkno = strip / td->td_stripbufmax;

	if (strip >= td->td_nstrips)	/* range check strip/tile number */
		return;
	/*
	 * Check if strip/tile's block is already loaded, if not, write out previous
	 * block and then load / initialize strip/tile's block.
	 */
	if (blkno != td->td_stripbcsblk)
		_TIFFGetByteCount(tif, strip);	/* getting byte count causes block to be loaded / init'ed */
	/*
	 * Set strip/tile's byte count
	 */
	td->td_stripbcsbuf[strip % td->td_stripbufmax] = bytecount;
	td->td_stripbcsdirty = 1;	
	if (strip >= td->td_stripbcscnt)
		td->td_stripbcscnt = strip + 1;
}

/*
 * Write out current byte counts block if dirty.  Allocate space for byte counts array in file
 * if necessary (possibly converting file to BigTIFF in the process!)
 */
extern int
_TIFFFlushByteCounts(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	int status = 1;
		
	if (td->td_stripbcsdirty) {			/* if need to write current block */
		/* write current block */
		TIFFDirEntry	bcsent;			/* fake entry for writing block segments */
		
		if (!td->td_stripbcstype)
			td->td_stripbcstype = TIFF_LONG;
		bcsent.s.tdir_tag = (isTiled(tif) ? TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS);
		bcsent.s.tdir_type = TIFF_LONG;
		/* place array in file if necessary */
		if (!_TIFFPlaceArray(tif, &td->td_stripbcsoff, &bcsent))  
			return 0;
		TDIRSetEntryCount(tif, &bcsent, TIFFmin(td->td_stripbufmax, 
					td->td_nstrips - td->td_stripbcsblk * td->td_stripbufmax));
		tif->tif_dataoff = td->td_stripbcsoff + 
				td->td_stripbcsblk * td->td_stripbufmax * TIFFDataWidth(bcsent.s.tdir_type);
		status = TIFFWriteLongArray(tif, &bcsent, (uint32*) td->td_stripbcsbuf);
		td->td_stripbcsdirty = 0;
	}
	return status;
}

/*
 * Place strip/tile offsets or byte counts array in file.  Allocate space in file and
 * write to end of space to reserve.  (Assumes file system / library zeros intervening data.)
 */
static int
_TIFFPlaceArray(TIFF* tif, toff_t* off, TIFFDirEntry *dir)
{
	TIFFDirectory *td = &tif->tif_dir;

	if (!*off) {					/* if haven't placed array yet */
		uint64	zerobuff = 0;

		*off = tif->tif_curoff;			/* place the array in file */
		tif->tif_curoff += td->td_nstrips * TIFFDataWidth(dir->s.tdir_type);
		/* write zero at end of array */
		TDIRSetEntryCount(tif, dir, 1);
		tif->tif_dataoff = tif->tif_curoff - TIFFDataWidth(dir->s.tdir_type);
		if (!TIFFWriteData(tif, dir, (char*) &zerobuff))
			return 0;
	}
	return 1;
}

/*
 * Similar to TIFFWriteDirectory(), but if the directory has already
 * been written once, it is relocated to the end of the file, in case it
 * has changed in size.  Note that this will result in the loss of the 
 * previously used directory space. 
 */ 
int 
TIFFRewriteDirectory( TIFF *tif )
{
    static const char module[] = "TIFFRewriteDirectory";

    /* We don't need to do anything special if it hasn't been written. */
    if( tif->tif_diroff == 0 )
        return TIFFWriteDirectory( tif );

    /*
    ** Find and zero the pointer to this directory, so that TIFFLinkDirectory
    ** will cause it to be added after this directories current pre-link.
    */
    
    /* Is it the first directory in the file? */
    if (TIFFGetHdrDirOff(tif,tif->tif_header) == tif->tif_diroff) 
    {
        TIFFSetHdrDirOff(tif,tif->tif_header,0);
        tif->tif_diroff = 0;

        if (!SeekOK(tif, 0) ||
	    !WriteOK(tif, &tif->tif_header, sizeof(TIFFHeader))) {
	    TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
		"Error updating TIFF header (%d)", TIFFGetErrno(tif));
            return (0);
        }
    }
    else
    {
        toff_t	nextdir, off;
	toff_t	nextdiroff;		/* 32-bit or 64-bit directory offset */

	nextdir = TIFFGetHdrDirOff(tif,tif->tif_header);
	do {
		uint16 dircount;

		if (!SeekOK(tif, nextdir) ||
		    !ReadOK(tif, &dircount, sizeof (dircount))) {
			TIFFErrorExt(tif->tif_clientdata, module, 
				"Error fetching directory count (%d)", TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		(void) TIFFSeekFile(tif, TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif), SEEK_CUR);
		if (!ReadOK(tif, &nextdiroff, TIFFDirOffLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module, 
				"Error fetching directory link (%d)", TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirOff(tif,&nextdiroff);
		nextdir = TIFFGetDirOff(tif,nextdiroff);
	} while (nextdir != tif->tif_diroff && nextdir != 0);

        off = TIFFSeekFile(tif, 0, SEEK_CUR); /* get current offset */
        (void) TIFFSeekFile(tif, off - TIFFDirOffLen(tif), SEEK_SET);
        tif->tif_diroff = 0;
	if (!WriteOK(tif, &(tif->tif_diroff), TIFFDirOffLen(tif))) {
		TIFFErrorExt(tif->tif_clientdata, module, 
			"Error writing directory link (%d)", TIFFGetErrno(tif));
		return (0);
	}
    }

    /*
    ** Now use TIFFWriteDirectory() normally.
    */
    return TIFFWriteDirectory( tif );
}

/*
 * Link the current directory into the
 * directory chain for the file.
 */
static int
TIFFLinkDirectory(TIFF* tif)
{
	static const char module[] = "TIFFLinkDirectory";
	toff_t currdir, nextdir;
	toff_t diroff, nextdiroff;	/* 32-bit or 64-bit dir offsets */

	/*
	 * New directory will go at end of file; if file not BigTIFF and
	 * size is beyond 2^32, convert to BigTIFF now.
	 */
	tif->tif_diroff = ((TIFFSeekFile(tif, 0, SEEK_END) + 1) & ~(toff_t) 1);
	if (!isBigTIFF(tif) && isBigOff(tif->tif_diroff)) {
		if (!TIFFMakeBigTIFF(tif))
			return (0);
		tif->tif_diroff = ((TIFFSeekFile(tif, 0, SEEK_END) + 1) & ~(toff_t) 1);
	}

	TIFFSetDirOff(tif,diroff,tif->tif_diroff);
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabDirOff(tif,&diroff);

	/*
	 * Handle SubIFDs
	 */
        if (tif->tif_flags & TIFF_INSUBIFD) {
		(void) TIFFSeekFile(tif, tif->tif_subifdoff, SEEK_SET);
		if (!WriteOK(tif, &diroff, TIFFDirOffLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Error writing SubIFD directory link (%d)",
			    tif->tif_name, TIFFGetErrno(tif));
			return (0);
		}
		/*
		 * Advance to the next SubIFD or, if this is
		 * the last one configured, revert back to the
		 * normal directory linkage.
		 */
		if (--tif->tif_nsubifd)
			tif->tif_subifdoff += TIFFDirOffLen(tif);
		else
			tif->tif_flags &= ~TIFF_INSUBIFD;
		return (1);
	}

	/*
	 * First directory, overwrite offset in header.
	 */
	nextdir = TIFFGetHdrDirOff(tif,tif->tif_header);
	if (nextdir == 0) {
		TIFFSetHdrDirOff(tif,tif->tif_header, tif->tif_diroff);
		if (!SeekOK(tif, 0) ||
		    !WriteOK(tif, &tif->tif_header, sizeof(TIFFHeader)) ) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error writing TIFF header (%d)", TIFFGetErrno(tif));
			return (0);
		}
		return (1);
	}
	/*
	 * Not the first directory, search to the last and append.
	 */
	do {
		uint64 dircount;	/* 16-bit or 64-bit directory count */

		if (!SeekOK(tif, nextdir) ||
		    !ReadOK(tif, &dircount, TIFFDirCntLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module, 
				"Error fetching directory count (%d)", TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirCnt(tif,&dircount);
		currdir = nextdir + TIFFDirCntLen(tif) + TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif);
		TIFFSeekFile(tif, currdir, SEEK_SET);
		if (!ReadOK(tif, &nextdiroff, TIFFDirOffLen(tif)) ) {
			TIFFErrorExt(tif->tif_clientdata, module, 
				"Error fetching directory link (%d)", TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirOff(tif,&nextdiroff);
		nextdir = TIFFGetDirOff(tif,nextdiroff);
	} while (nextdir != 0);

        TIFFSeekFile(tif, currdir, SEEK_SET);
	if (!WriteOK(tif, &diroff, TIFFDirOffLen(tif)) ) {
		TIFFErrorExt(tif->tif_clientdata, module, 
			"Error writing directory link (%d)", TIFFGetErrno(tif));
		return (0);
	}
	return (1);
}

/*
 * Function to convert standard TIFF file to BigTIFF file.  Loop through all directories in
 * current file (including subdirectories linked via SubIFD arrays) and create corresponding
 * new directories with 64-bit arrays.  The old 32-bit directories are left in the file as
 * dead space.  The data for directory entries are reused by default.  SubIFD arrays require
 * special logic to convert from 32-bit to 64-bit offsets.  Note that only current directory
 * could need 64-bit block offsets, and they will have already been converted.
 *
 * Returns 1 if successful, 0 if cannot convert to BigTIFF (TIFF_NOBIGTIFF set 
 * or 32-bit API called).
 */
static int 
TIFFMakeBigTIFF(TIFF *tif)
{
	uint32	dirlink = TIFF_HEADER_DIROFF_S,
		diroff = tif->tif_header.s.tiff_diroff;
	toff_t	dirlinkB = TIFF_HEADER_DIROFF_B,
		diroffB;
	uint16	dircount, dirindex;
	uint64	dircountB;
	tsize_t	dirsize, dirsizeB;
	int	issubifd = 0;
	uint32	subifdcnt = 0;
	uint32	subifdlink;
	toff_t	subifdlinkB;
	char	*data, *dataB;
	TIFFDirEntry *dir, *dirB;

	/*
	 * Update flags and file header
	 */
	if (noBigTIFF(tif)) {
		TIFFErrorExt((tif)->tif_clientdata, 
			"TIFFCheckBigTIFF", "File > 2^32 and NO BigTIFF specified");
		return (0);
	}
	tif->tif_flags |= TIFF_ISBIGTIFF;
	tif->tif_header.b.tiff_version = TIFF_BIGTIFF_VERSION;
	tif->tif_header.b.tiff_offsize = 8;
	tif->tif_header.b.tiff_fill = 0;
	tif->tif_header.b.tiff_diroff = 0;
	if (tif->tif_flags & TIFF_SWAB) {
		TIFFSwabShort(&tif->tif_header.b.tiff_version);
		TIFFSwabShort(&tif->tif_header.b.tiff_offsize);
	}
	if (!SeekOK(tif, 0) ||
	    !WriteOK(tif, &tif->tif_header, sizeof(TIFFHeader))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
			"Error updating TIFF header (%d)", TIFFGetErrno(tif));
		return (0);
	}
	if (tif->tif_flags & TIFF_SWAB) {
		TIFFSwabShort(&tif->tif_header.b.tiff_version);
		TIFFSwabShort(&tif->tif_header.b.tiff_offsize);
	}

	/*
	 * Loop through all directories and rewrite as BigTIFF with 64-bit offsets.  This
	 * begins with main IFD chain but may divert to SubIFD arrays as needed.
	 */
	while (diroff != 0 && diroff != tif->tif_diroff) {
		if (!SeekOK(tif, diroff) ||
		    !ReadOK(tif, &dircount, sizeof(dircount))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error reading TIFF directory (%d)", TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		dircountB = dircount;
		if (!(data = _TIFFmalloc(dirsize = dircount * TIFFDirEntryLenS)) ||
		    !(dataB = _TIFFmalloc(dirsizeB = dircount * TIFFDirEntryLenB))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error allocating space for directory");
			return (0);
		}
		if (!SeekOK(tif, diroff + sizeof(dircount)) ||
		    !ReadOK(tif, data, dirsize)) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error reading TIFF directory (%d)", TIFFGetErrno(tif));
			goto error;
		}
		diroffB = tif->tif_dataoff;
		tif->tif_dataoff += sizeof(dircountB) + dirsizeB + sizeof(toff_t);

		for (dirindex = 0; dirindex < dircount; dirindex++) {
			dir = (TIFFDirEntry*) (data + dirindex * TIFFDirEntryLenS);
			dirB = (TIFFDirEntry*) (dataB + dirindex * TIFFDirEntryLenB);
			if (tif->tif_flags & TIFF_SWAB) {
				TIFFSwabShort(&dir->s.tdir_tag);
				TIFFSwabShort(&dir->s.tdir_type);
				TIFFSwabLong(&dir->s.tdir_count);
				TIFFSwabLong(&dir->s.tdir_offset);
			}
			dirB->b.tdir_tag = dir->s.tdir_tag;
			dirB->b.tdir_type = dir->s.tdir_type;
			dirB->b.tdir_count = dir->s.tdir_count;
			dirB->b.tdir_offset = 0;
			/*
			 * If data are in directory entry itself, copy data, else (data are pointed
			 * to by directory entry) copy pointer.  This is complicated by the fact that
			 * the old entry had 32-bits of space, and the new has 64-bits, so may have
			 * to read data pointed at by the old entry directly into the new entry.
			 */
			switch (dir->s.tdir_type) {
			case TIFF_UNDEFINED:
			case TIFF_BYTE:
			case TIFF_SBYTE:
			case TIFF_ASCII:
				if (dir->s.tdir_count <= sizeof(dir->s.tdir_offset))
					_TIFFmemcpy(&dirB->b.tdir_offset, &dir->s.tdir_offset, dir->s.tdir_count);
				else if (dir->s.tdir_count <= sizeof(dirB->b.tdir_count)) {
					TIFFSeekFile(tif, dir->s.tdir_offset, SEEK_SET);
					TIFFReadFile(tif, &dirB->b.tdir_offset, dir->s.tdir_count);
				} else
					dirB->b.tdir_offset = dir->s.tdir_offset;
				break;
			case TIFF_SHORT:
			case TIFF_SSHORT:
				if (dir->s.tdir_count <= sizeof(dir->s.tdir_offset) / sizeof(uint16))
					_TIFFmemcpy(&dirB->b.tdir_offset, &dir->s.tdir_offset, dir->s.tdir_count * sizeof(uint16));
				else if (dir->s.tdir_count <= sizeof(dirB->b.tdir_count) / sizeof(uint16)) {
					TIFFSeekFile(tif, dir->s.tdir_offset, SEEK_SET);
					TIFFReadFile(tif, &dirB->b.tdir_offset, dir->s.tdir_count * sizeof(uint16));
					if (tif->tif_flags & TIFF_SWAB)
						TIFFSwabArrayOfShort((uint16*) &dirB->b.tdir_offset, dir->s.tdir_count);
				} else
					dirB->b.tdir_offset = dir->s.tdir_offset;
				break;
			case TIFF_LONG:
			case TIFF_FLOAT:
			case TIFF_IFD:
				if (dir->s.tdir_count <= sizeof(dir->s.tdir_offset) / sizeof(uint32))
					_TIFFmemcpy(&dirB->b.tdir_offset, &dir->s.tdir_offset, dir->s.tdir_count * sizeof(uint32));
				else if (dir->s.tdir_count <= sizeof(dirB->b.tdir_count) / sizeof(uint32)) {
					TIFFSeekFile(tif, dir->s.tdir_offset, SEEK_SET);
					TIFFReadFile(tif, &dirB->b.tdir_offset, dir->s.tdir_count * sizeof(uint32));
					if (tif->tif_flags & TIFF_SWAB)
						TIFFSwabArrayOfLong((uint32*) &dirB->b.tdir_offset, dir->s.tdir_count);
				} else
					dirB->b.tdir_offset = dir->s.tdir_offset;
				break;
			case TIFF_RATIONAL:
			case TIFF_SRATIONAL:
				if (dir->s.tdir_count * 2 <= sizeof(dirB->b.tdir_offset) / sizeof(uint32)) {
					TIFFSeekFile(tif, dir->s.tdir_offset, SEEK_SET);
					TIFFReadFile(tif, &dirB->b.tdir_offset, dir->s.tdir_count * 2 * sizeof(uint32));
					if (tif->tif_flags & TIFF_SWAB)
						TIFFSwabArrayOfLong((uint32*) &dirB->b.tdir_offset, dir->s.tdir_count * 2);
				} else
					dirB->b.tdir_offset = dir->s.tdir_offset;
				break;
			default:
				dirB->b.tdir_offset = dir->s.tdir_offset;
				break;
			}
			/*
			 * Process special case of SUBIFD; must change data from 32-bit to 64-bit in 
			 * case new 64-bit directory offsets need to be added.
			 */
			switch (dirB->b.tdir_tag) {
			case TIFFTAG_SUBIFD:
				dirB->b.tdir_type = TIFF_IFD8;
				subifdcnt = dir->s.tdir_count;
				/*
				 * Set pointer to existing SubIFD array
				 */
				if (subifdcnt <= sizeof(dir->s.tdir_offset) / sizeof(uint32))
					subifdlink = diroff + sizeof(dircount) +
						(char*) &dir->s.tdir_offset - (char*) dir;
				else
					subifdlink = dir->s.tdir_offset;
				/*
				 * Initialize new SubIFD array, set pointer to it
				 */
				if (subifdcnt <= sizeof(dir->b.tdir_offset) / sizeof(toff_t)) {
					dir->b.tdir_offset = 0;
					subifdlinkB = diroffB + sizeof(dircountB) +
						(char*) &dir->b.tdir_offset - (char*) dirB;
				} else {
					toff_t *offB;

					if (!(offB = _TIFFmalloc(subifdcnt * sizeof(toff_t)))) {
						TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
							"Error allocating space for 64-bit SubIFDs");
						goto error;
					}
					_TIFFmemset(offB, 0, subifdcnt * sizeof(toff_t));
					TIFFWriteLong8Array(tif, dirB, offB);
					_TIFFfree(offB);
					subifdlinkB = dirB->b.tdir_offset;
					}
				break;
			}
			if (tif->tif_flags & TIFF_SWAB) {
				TIFFSwabShort(&dirB->b.tdir_tag);
				TIFFSwabShort(&dirB->b.tdir_type);
				TIFFSwabLong8(&dirB->b.tdir_count);
				TIFFSwabLong8(&dirB->b.tdir_offset);
			}
		}
		
		/*
		 * Chain new directory to previous
		 */
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong8(&dircountB);
		if (!SeekOK(tif, diroffB) ||
		    !WriteOK(tif, &dircountB, sizeof(dircountB)) ||
		    !SeekOK(tif, diroffB + sizeof(dircountB)) ||
		    !WriteOK(tif, dataB, dirsizeB)) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error writing TIFF directory (%d)", TIFFGetErrno(tif));
			goto error;
		}

		/*
		 * If directory is SubIFD update array in host directory, else add to
		 * main directory chain
		 */
		if (tif->tif_nsubifd &&
			tif->tif_subifdoff == subifdlink)
			tif->tif_subifdoff = subifdlinkB;

		if (!issubifd && 
			dirlinkB == TIFF_HEADER_DIROFF_B)
			tif->tif_header.b.tiff_diroff = diroffB;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong8(&diroffB);
		if (!SeekOK(tif, (issubifd ? subifdlinkB++ : dirlinkB)) ||
		    !WriteOK(tif, &diroffB, sizeof(diroffB))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error writing directory link (%d)", TIFFGetErrno(tif));
			goto error;
		}

		if (issubifd)
			subifdcnt--;
		else {
			dirlink = diroff + sizeof(dircount) + dirsize;
			dirlinkB = diroffB + sizeof(dircountB) + dirsizeB;
		}
		issubifd = (subifdcnt > 0);

		if (!SeekOK(tif, (issubifd ? subifdlink++ : dirlink)) ||
		    !ReadOK(tif, &diroff, sizeof(diroff))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
				"Error reading directory link (%d)", TIFFGetErrno(tif));
			goto error;
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&diroff);
		_TIFFfree(dataB);
		_TIFFfree(data);
	}

	/*
	 * Mark end of directory chain
	 */
	diroffB = 0;
	if (dirlinkB == TIFF_HEADER_DIROFF_B)
		tif->tif_header.b.tiff_diroff = diroffB;
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabLong8(&diroffB);
	if (!SeekOK(tif, dirlinkB) ||
	    !WriteOK(tif, &diroffB, sizeof(diroffB))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, 
			"Error writing directory link (%d)", TIFFGetErrno(tif));
		goto error;
	}

	return (1);

error:
	_TIFFfree(dataB);
	_TIFFfree(data);
	return (0);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
