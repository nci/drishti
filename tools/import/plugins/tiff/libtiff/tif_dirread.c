/* $Id: tif_dirread.c,v 1.82 2006/03/16 12:24:53 dron Exp $ */

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
 *
 *  modified 09/05/07	Ellis		fix crash in TIFFFetchRationalArray
 *  modified 10/16/08	vunger		fixed byte-swapping, added printDir() debug routine
 *
 */

/*
 * TIFF Library.
 *
 * Directory Read Support Routines.
 */
#include "tiffiop.h"

#define	IGNORE	0		/* tag placeholder used below */

#ifdef HAVE_IEEEFP
# define	TIFFCvtIEEEFloatToNative(tif, n, fp)
# define	TIFFCvtIEEEDoubleToNative(tif, n, dp)
#else
extern	void TIFFCvtIEEEFloatToNative(TIFF*, uint32, float*);
extern	void TIFFCvtIEEEDoubleToNative(TIFF*, uint32, double*);
#endif

static	int EstimateStripByteCounts(TIFF*, TIFFDirEntry*, uint64);
static	void MissingRequired(TIFF*, const char*);
static	int CheckDirCount(TIFF*, TIFFDirEntry*, uint64);
static	tsize_t TIFFFetchData(TIFF*, TIFFDirEntry*, char*);
static	tsize_t TIFFFetchString(TIFF*, TIFFDirEntry*, char*);
static	float TIFFFetchRational(TIFF*, TIFFDirEntry*);
static	int TIFFFetchNormalTag(TIFF*, TIFFDirEntry*);
static	int TIFFFetchPerSampleShorts(TIFF*, TIFFDirEntry*, uint16*);
static	int TIFFFetchPerSampleLongs(TIFF*, TIFFDirEntry*, uint32*);
static	int TIFFFetchPerSampleAnys(TIFF*, TIFFDirEntry*, double*);
static	int TIFFFetchShortArray(TIFF*, TIFFDirEntry*, uint16*);
static	int TIFFFetchLongArray(TIFF*, TIFFDirEntry*, uint32*);
static	int TIFFFetchLong8Array(TIFF*, TIFFDirEntry*, uint64*);
static	int TIFFFetchRefBlackWhite(TIFF*, TIFFDirEntry*);
static	float TIFFFetchFloat(TIFF*, TIFFDirEntry*);
static	int TIFFFetchFloatArray(TIFF*, TIFFDirEntry*, float*);
static	int TIFFFetchDoubleArray(TIFF*, TIFFDirEntry*, double*);
static	int TIFFFetchAnyArray(TIFF*, TIFFDirEntry*, double*);
static	int TIFFFetchShortPair(TIFF*, TIFFDirEntry*);
static	void ChopUpSingleUncompressedStrip(TIFF*);
static void printDir(TIFF*, TIFFDirEntry*, int);

/*
 * Read the next TIFF directory from a file
 * and convert it to the internal format.
 * We read directories sequentially.
 */
int
TIFFReadDirectory(TIFF* tif)
{
	static const char module[] = "TIFFReadDirectory";

	int n;
	TIFFDirectory* td;
	TIFFDirEntry *dp, *dir = NULL;
	uint16 iv;
	uint32 v;
	const TIFFFieldInfo* fip;
	size_t fix;
	uint64 dircount;			/* 16-bit or 64-bit directory count */
	toff_t nextdiroff;			/* 32-bit or 64-bit directory offset */
	int diroutoforderwarning = 0;
	toff_t* new_dirlist;

	tif->tif_diroff = tif->tif_nextdiroff;
	if (tif->tif_diroff == 0)		/* no more directories */
		return (0);

	/*
	 * XXX: Trick to prevent IFD looping. The one can create TIFF file
	 * with looped directory pointers. We will maintain a list of already
	 * seen directories and check every IFD offset against this list.
	 */
	for (n = 0; n < tif->tif_dirnumber; n++) {
		if (tif->tif_dirlist[n] == tif->tif_diroff)
			return (0);
	}
	tif->tif_dirnumber++;
	new_dirlist = (toff_t *)_TIFFrealloc(tif->tif_dirlist,
					tif->tif_dirnumber * sizeof(toff_t));
	if (!new_dirlist) {
		TIFFErrorExt(tif->tif_clientdata, module,
			  "%s: Failed to allocate space for IFD list",
			  tif->tif_name);
		return (0);
	}
	tif->tif_dirlist = new_dirlist;
	tif->tif_dirlist[tif->tif_dirnumber - 1] = tif->tif_diroff;

	/*
	 * Cleanup any previous compression state.
	 */
	(*tif->tif_cleanup)(tif);
	tif->tif_curdir++;
	if (!isMapped(tif)) {
		if (!SeekOK(tif, tif->tif_diroff)) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Seek error accessing TIFF directory",
                            tif->tif_name);
			return (0);
		}
		if (!ReadOK(tif, &dircount, TIFFDirCntLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Can not read TIFF directory count (%d)",
                            tif->tif_name, TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirCnt(tif,&dircount);
		dir = (TIFFDirEntry *)_TIFFCheckMalloc(tif, TIFFGetDirCnt(tif,dircount), TDIREntryLen(tif),
						"to read TIFF directory");
		if (dir == NULL)
			return (0);

		if (!ReadOK(tif, dir, TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module,
                                  "%.100s: Can not read TIFF directory (%d)",
                                  tif->tif_name, TIFFGetErrno(tif));
			goto bad;
		}
		/*
		 * Read offset to next directory for sequential scans.
		 */
		(void) ReadOK(tif, &nextdiroff, TIFFDirOffLen(tif));
	} else {
		toff_t off = tif->tif_diroff;

		if (off + TIFFDirCntLen(tif) > tif->tif_size) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Can not read TIFF directory count",
                            tif->tif_name);
			return (0);
		}
		_TIFFmemcpy(&dircount, tif->tif_base + off, TIFFDirCntLen(tif));
		off += TIFFDirCntLen(tif);
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirCnt(tif,&dircount);
		dir = (TIFFDirEntry *)_TIFFCheckMalloc(tif, TIFFGetDirCnt(tif,dircount), TDIREntryLen(tif),
						"to read TIFF directory");
		if (dir == NULL)
			return (0);
		if (off + TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif) > tif->tif_size) {
			TIFFErrorExt(tif->tif_clientdata, module,
                                  "%s: Can not read TIFF directory",
                                  tif->tif_name);
			goto bad;
		}
		_TIFFmemcpy(dir, tif->tif_base + off, TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif));
		off += TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif);
		if (off + TIFFDirOffLen(tif) <= tif->tif_size)
			_TIFFmemcpy(&nextdiroff, tif->tif_base+off, TIFFDirOffLen(tif));
	}
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabDirOff(tif, &nextdiroff);
	tif->tif_nextdiroff = TIFFGetDirOff(tif,nextdiroff);

	tif->tif_flags &= ~TIFF_BEENWRITING;	/* reset before new dir */
	/*
	 * Setup default value and then make a pass over
	 * the fields to check type and tag information,
	 * and to extract info required to size data
	 * structures.  A second pass is made afterwards
	 * to read in everthing not taken in the first pass.
	 */
	td = &tif->tif_dir;
	/* free any old stuff and reinit */
	TIFFFreeDirectory(tif);
	TIFFDefaultDirectory(tif);
	/*
	 * Electronic Arts writes gray-scale TIFF files
	 * without a PlanarConfiguration directory entry.
	 * Thus we setup a default value here, even though
	 * the TIFF spec says there is no default value.
	 */
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	/*
	 * Sigh, we must make a separate pass through the
	 * directory for the following reason:
	 *
	 * We must process the Compression tag in the first pass
	 * in order to merge in codec-private tag definitions (otherwise
	 * we may get complaints about unknown tags).  However, the
	 * Compression tag may be dependent on the SamplesPerPixel
	 * tag value because older TIFF specs permited Compression
	 * to be written as a SamplesPerPixel-count tag entry.
	 * Thus if we don't first figure out the correct SamplesPerPixel
	 * tag value then we may end up ignoring the Compression tag
	 * value because it has an incorrect count value (if the
	 * true value of SamplesPerPixel is not 1).
	 *
	 * It sure would have been nice if Aldus had really thought
	 * this stuff through carefully.
	 */ 
	for (dp = dir, n = TIFFGetDirCnt(tif,dircount); n > 0; 
		n--, TDIREntryNext(tif,dp)) {
		if (tif->tif_flags & TIFF_SWAB) {
			TIFFSwabShort(&dp->s.tdir_tag);
			TIFFSwabShort(&dp->s.tdir_type);
			TDIRSwabEntryCount(tif,dp);
			TDIRSwabEntryOff(tif,dp);
		}
		//Debug printDir(tif, dp, 0);
		if (dp->s.tdir_tag == TIFFTAG_SAMPLESPERPIXEL) {
			if (!TIFFFetchNormalTag(tif, dp))
				goto bad;
			dp->s.tdir_tag = IGNORE;
		}
	}
	/*
	 * First real pass over the directory.
	 */
	fix = 0;
	for (dp = dir, n = TIFFGetDirCnt(tif,dircount); n > 0; n--, TDIREntryNext(tif,dp)) {
		if (fix >= tif->tif_nfields || dp->s.tdir_tag == IGNORE)
			continue;
		/*
		 * Silicon Beach (at least) writes unordered
		 * directory tags (violating the spec).  Handle
		 * it here, but be obnoxious (maybe they'll fix it?).
		 */
		if (dp->s.tdir_tag < tif->tif_fieldinfo[fix]->field_tag) {
			if (!diroutoforderwarning) {
				TIFFWarningExt(tif->tif_clientdata, module,
	"%s: invalid TIFF directory; tags are not sorted in ascending order",
					       tif->tif_name);
				diroutoforderwarning = 1;
			}
			fix = 0;			/* O(n^2) */
		}
		while (fix < tif->tif_nfields &&
		       tif->tif_fieldinfo[fix]->field_tag < dp->s.tdir_tag)
			fix++;
		if (fix >= tif->tif_nfields ||
		    tif->tif_fieldinfo[fix]->field_tag != dp->s.tdir_tag) {

					TIFFWarningExt(tif->tif_clientdata,
						       module,
                        "%s: unknown field with tag %d (0x%x) encountered",
						       tif->tif_name,
						       dp->s.tdir_tag,
						       dp->s.tdir_tag);

                    TIFFMergeFieldInfo(tif,
                                       _TIFFCreateAnonFieldInfo(tif,
						dp->s.tdir_tag,
						(TIFFDataType) dp->s.tdir_type),
				       1 );
                    fix = 0;
                    while (fix < tif->tif_nfields &&
                           tif->tif_fieldinfo[fix]->field_tag < dp->s.tdir_tag)
			fix++;
		}
		/*
		 * Null out old tags that we ignore.
		 */
		if (tif->tif_fieldinfo[fix]->field_bit == FIELD_IGNORE) {
	ignore:
			dp->s.tdir_tag = IGNORE;
			continue;
		}
		/*
		 * Check data type.  There may be multiple info entries for tag.
		 */
		while (1) {
			fip = tif->tif_fieldinfo[fix];
			if (fip->field_tag != dp->s.tdir_tag ||
				fix >= tif->tif_nfields) {
					TIFFWarningExt(tif->tif_clientdata, module,
					    "%s: wrong data type %d for \"%s\"; tag ignored",
					    tif->tif_name, dp->s.tdir_type,
					    tif->tif_fieldinfo[fix-1]->field_name);
				goto ignore;
			}
			if (fip->field_type == TIFF_ANY ||
				fip->field_type == dp->s.tdir_type)
				break;
			fix++;
		}

		/*
		 * Check count if known in advance.
		 */
		if (fip->field_readcount != TIFF_VARIABLE
		    && fip->field_readcount != TIFF_VARIABLE2) {
			uint32 expected = (fip->field_readcount == TIFF_SPP) ?
			    (uint32) td->td_samplesperpixel :
			    (uint32) fip->field_readcount;
			if (!CheckDirCount(tif, dp, expected))
				goto ignore;
		}

		switch (dp->s.tdir_tag) {
		case TIFFTAG_COMPRESSION:
			/*
			 * The 5.0 spec says the Compression tag has
			 * one value, while earlier specs say it has
			 * one value per sample.  Because of this, we
			 * accept the tag if one value is supplied.
			 */
			if (TDIRGetEntryCount(tif,dp) == 1) {
				if (dp->s.tdir_type == TIFF_LONG) {
					if (!TIFFFetchLongArray(tif, dp, &v) ||
					    !TIFFSetField(tif, dp->s.tdir_tag, (uint16)v))
					    goto bad;
				} else {	    /* TIFF_SHORT */
					if (!TIFFFetchShortArray(tif, dp, &iv) ||
					    !TIFFSetField(tif, dp->s.tdir_tag, (uint16)iv))
					    goto bad;
				}
				break;
			/* XXX: workaround for broken TIFFs */
			} else if (dp->s.tdir_type == TIFF_LONG) {
				if (!TIFFFetchPerSampleLongs(tif, dp, &v) ||
				    !TIFFSetField(tif, dp->s.tdir_tag, (uint16)v))
					goto bad;
			} else {		   /* TIFF_SHORT */
				if (!TIFFFetchPerSampleShorts(tif, dp, &iv) ||
				    !TIFFSetField(tif, dp->s.tdir_tag, iv))
					goto bad;
			}
			dp->s.tdir_tag = IGNORE;
			break;
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_STRIPBYTECOUNTS:
		case TIFFTAG_TILEOFFSETS:
		case TIFFTAG_TILEBYTECOUNTS:
			TIFFSetFieldBit(tif, fip->field_bit);
			break;
		case TIFFTAG_IMAGEWIDTH:
		case TIFFTAG_IMAGELENGTH:
		case TIFFTAG_IMAGEDEPTH:
		case TIFFTAG_TILELENGTH:
		case TIFFTAG_TILEWIDTH:
		case TIFFTAG_TILEDEPTH:
		case TIFFTAG_PLANARCONFIG:
		case TIFFTAG_ROWSPERSTRIP:
		case TIFFTAG_EXTRASAMPLES:
			if (!TIFFFetchNormalTag(tif, dp))
				goto bad;
			dp->s.tdir_tag = IGNORE;
			break;
		}
	}

	/*
	 * Allocate directory structure and setup defaults.
	 */
	if (!TIFFFieldSet(tif, FIELD_IMAGEDIMENSIONS)) {
		MissingRequired(tif, "ImageLength");
		goto bad;
	}
	/* 
 	 * Setup appropriate structures (by strip or by tile)
	 */
	if (!TIFFFieldSet(tif, FIELD_TILEDIMENSIONS)) {
		td->td_nstrips = TIFFNumberOfStrips(tif);
		td->td_tilewidth = td->td_imagewidth;
		td->td_tilelength = td->td_rowsperstrip;
		td->td_tiledepth = td->td_imagedepth;
		tif->tif_flags &= ~TIFF_ISTILED;
	} else {
		td->td_nstrips = TIFFNumberOfTiles(tif);
		tif->tif_flags |= TIFF_ISTILED;
	}
	if (!td->td_nstrips) {
		TIFFErrorExt(tif->tif_clientdata, module,
			     "%s: cannot handle zero number of %s",
			     tif->tif_name, isTiled(tif) ? "tiles" : "strips");
		goto bad;
	}
	td->td_stripsperimage = td->td_nstrips;
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		td->td_stripsperimage /= td->td_samplesperpixel;
	if (!TIFFFieldSet(tif, FIELD_STRIPOFFSETS)) {
		MissingRequired(tif,
				isTiled(tif) ? "TileOffsets" : "StripOffsets");
		goto bad;
	}

	/*
	 * Second pass: extract other information.
	 */
	for (dp = dir, n = TIFFGetDirCnt(tif,dircount); n > 0; n--, TDIREntryNext(tif,dp)) {
		if (dp->s.tdir_tag == IGNORE)
			continue;
		switch (dp->s.tdir_tag) {
		case TIFFTAG_MINSAMPLEVALUE:
		case TIFFTAG_MAXSAMPLEVALUE:
		case TIFFTAG_BITSPERSAMPLE:
		case TIFFTAG_DATATYPE:
		case TIFFTAG_SAMPLEFORMAT:
			/*
			 * The 5.0 spec says the Compression tag has
			 * one value, while earlier specs say it has
			 * one value per sample.  Because of this, we
			 * accept the tag if one value is supplied.
			 *
                         * The MinSampleValue, MaxSampleValue, BitsPerSample
                         * DataType and SampleFormat tags are supposed to be
                         * written as one value/sample, but some vendors
                         * incorrectly write one value only -- so we accept
                         * that as well (yech). Other vendors write correct
			 * value for NumberOfSamples, but incorrect one for
			 * BitsPerSample and friends, and we will read this
			 * too.
			 */
			if (TDIRGetEntryCount(tif,dp) == 1) {
				if (dp->s.tdir_type == TIFF_LONG) {
					if (!TIFFFetchLongArray(tif, dp, &v) ||
					    !TIFFSetField(tif, dp->s.tdir_tag, (uint16)v))
					    goto bad;
				} else {	    /* TIFF_SHORT */
					if (!TIFFFetchShortArray(tif, dp, &iv) ||
					    !TIFFSetField(tif, dp->s.tdir_tag, (uint16)iv))
					    goto bad;
				}
			/* XXX: workaround for broken TIFFs */
			} else if (dp->s.tdir_tag == TIFFTAG_BITSPERSAMPLE
				   && dp->s.tdir_type == TIFF_LONG) {
				if (!TIFFFetchPerSampleLongs(tif, dp, &v) ||
				    !TIFFSetField(tif, dp->s.tdir_tag, (uint16)v))
					goto bad;
			} else {		      /* TIFF_SHORT */
				if (!TIFFFetchPerSampleShorts(tif, dp, &iv) ||
				    !TIFFSetField(tif, dp->s.tdir_tag, iv))
					goto bad;
			}
			break;
		case TIFFTAG_SMINSAMPLEVALUE:
		case TIFFTAG_SMAXSAMPLEVALUE:
			{
				double dv = 0.0;
				if (!TIFFFetchPerSampleAnys(tif, dp, &dv) ||
				    !TIFFSetField(tif, dp->s.tdir_tag, dv))
					goto bad;
			}
			break;
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_TILEOFFSETS:
			td->td_stripoffstype = dp->s.tdir_type;
			td->td_stripoffscnt = TDIRGetEntryCount(tif, dp);
			td->td_stripoffsoff = TDIRGetEntryOff(tif, dp);
			/* buffer will be allocated when first offsets block is loaded */
			break;
		case TIFFTAG_STRIPBYTECOUNTS:
		case TIFFTAG_TILEBYTECOUNTS:
			td->td_stripbcstype = dp->s.tdir_type;
			td->td_stripbcscnt = TDIRGetEntryCount(tif, dp);
			td->td_stripbcsoff = TDIRGetEntryOff(tif, dp);
			/* buffer will be allocated when first byte counts block is loaded */
			break;
		case TIFFTAG_SUBIFD:
			{
				/*
				 * Store SubIFD array as 64-bit offsets.
				 */
				toff_t *doffB = _TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dp), sizeof (toff_t),
					"Array of 64-bit SubIFD offsets");

				if (dp->s.tdir_type == TIFF_LONG8 || dp->s.tdir_type == TIFF_IFD8) {
					if (!doffB || !TIFFFetchLong8Array(tif, dp, doffB)) {
						_TIFFfree(doffB);
						goto bad;
					}
				} else {	/* TIFF_LONG || TIFF_IFD */
					uint32 *doff = _TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dp), sizeof (uint32),
						"Array of 32-bit SubIFD offsets");
					uint16	si;

					if (!doffB || !doff || !TIFFFetchLongArray(tif, dp, doff)) {
						_TIFFfree(doff);
						_TIFFfree(doffB);
						goto bad;
					}
					for (si = 0; si < TDIRGetEntryCount(tif,dp); si++)
						doffB[si] = doff[si];
					_TIFFfree(doff);
				}
				TIFFSetField(tif, TIFFTAG_SUBIFD, TDIRGetEntryCount(tif,dp), doffB);
				_TIFFfree(doffB);
			}
			break;
		case TIFFTAG_COLORMAP:
		case TIFFTAG_TRANSFERFUNCTION:
			{
				char* cp;
				/*
				 * TransferFunction can have either 1x or 3x
				 * data values; Colormap can have only 3x
				 * items.
				 */
				v = 1L<<td->td_bitspersample;
				if (dp->s.tdir_tag == TIFFTAG_COLORMAP ||
				    TDIRGetEntryCount(tif,dp) != v) {
					if (!CheckDirCount(tif, dp, 3 * v))
						break;
				}
				v *= sizeof(uint16);
				cp = (char *)_TIFFCheckMalloc(tif,
							      TDIRGetEntryCount(tif,dp),
							      sizeof (uint16),
					"to read \"TransferFunction\" tag");
				if (cp != NULL) {
					if (TIFFFetchData(tif, dp, cp)) {
						/*
						 * This deals with there being
						 * only one array to apply to
						 * all samples.
						 */
						uint32 c = 1L << td->td_bitspersample;
						if (TDIRGetEntryCount(tif,dp) == c)
							v = 0L;
						TIFFSetField(tif, dp->s.tdir_tag,
						    cp, cp+v, cp+2*v);
					}
					_TIFFfree(cp);
				}
				break;
			}
		case TIFFTAG_PAGENUMBER:
		case TIFFTAG_HALFTONEHINTS:
		case TIFFTAG_YCBCRSUBSAMPLING:
		case TIFFTAG_DOTRANGE:
			(void) TIFFFetchShortPair(tif, dp);
			break;
		case TIFFTAG_REFERENCEBLACKWHITE:
			(void) TIFFFetchRefBlackWhite(tif, dp);
			break;
/* BEGIN REV 4.0 COMPATIBILITY */
		case TIFFTAG_OSUBFILETYPE:
			v = 0L;
			TIFFFetchShortArray(tif, dp, &iv);
			switch (iv) {
			case OFILETYPE_REDUCEDIMAGE:
				v = FILETYPE_REDUCEDIMAGE;
				break;
			case OFILETYPE_PAGE:
				v = FILETYPE_PAGE;
				break;
			}
			if (v)
				TIFFSetField(tif, TIFFTAG_SUBFILETYPE, v);
			break;
/* END REV 4.0 COMPATIBILITY */
		default:
			(void) TIFFFetchNormalTag(tif, dp);
			break;
		}
	}
	/*
	 * Verify Palette image has a Colormap.
	 */
	if (td->td_photometric == PHOTOMETRIC_PALETTE &&
	    !TIFFFieldSet(tif, FIELD_COLORMAP)) {
		MissingRequired(tif, "Colormap");
		goto bad;
	}
	/*
	 * Attempt to deal with a missing StripByteCounts tag.
	 */
	if (!TIFFFieldSet(tif, FIELD_STRIPBYTECOUNTS)) {
		/*
		 * Some manufacturers violate the spec by not giving
		 * the size of the strips.  In this case, assume there
		 * is one uncompressed strip of data.
		 */
		if ((td->td_planarconfig == PLANARCONFIG_CONTIG &&
		    td->td_nstrips > 1) ||
		    (td->td_planarconfig == PLANARCONFIG_SEPARATE &&
		     td->td_nstrips != td->td_samplesperpixel)) {
		    MissingRequired(tif, "StripByteCounts");
		    goto bad;
		}
		TIFFWarningExt(tif->tif_clientdata, module,
			"%s: TIFF directory is missing required "
			"\"%s\" field, calculating from imagelength",
			tif->tif_name,
		        _TIFFFieldWithTag(tif,TIFFTAG_STRIPBYTECOUNTS)->field_name);
		if (EstimateStripByteCounts(tif, dir, TIFFGetDirCnt(tif,dircount)) < 0)
		    goto bad;
/* 
 * Assume we have wrong StripByteCount value (in case of single strip) in
 * following cases:
 *   - it is equal to zero along with StripOffset;
 *   - it is larger than file itself (in case of uncompressed image);
 *   - it is smaller than the size of the bytes per row multiplied on the
 *     number of rows.  The last case should not be checked in the case of
 *     writing new image, because we may do not know the exact strip size
 *     until the whole image will be written and directory dumped out.
 */
#define	BYTECOUNTLOOKSBAD \
    ( (_TIFFGetByteCount(tif, 0) == 0 && _TIFFGetOffset(tif, 0) != 0) || \
      (td->td_compression == COMPRESSION_NONE && \
       _TIFFGetByteCount(tif, 0) > TIFFGetFileSize(tif) - _TIFFGetOffset(tif, 0)) || \
      (tif->tif_mode == O_RDONLY && \
       td->td_compression == COMPRESSION_NONE && \
       _TIFFGetByteCount(tif, 0) < TIFFScanlineSize(tif) * td->td_imagelength) )

	} else if (td->td_nstrips == 1 
                   && _TIFFGetOffset(tif, 0) != 0 
                   && BYTECOUNTLOOKSBAD) {
		/*
		 * XXX: Plexus (and others) sometimes give a value of zero for
		 * a tag when they don't know what the correct value is!  Try
		 * and handle the simple case of estimating the size of a one
		 * strip image.
		 */
		TIFFWarningExt(tif->tif_clientdata, module,
	"%s: Bogus \"%s\" field, ignoring and calculating from imagelength",
                            tif->tif_name,
		            _TIFFFieldWithTag(tif,TIFFTAG_STRIPBYTECOUNTS)->field_name);
		if(EstimateStripByteCounts(tif, dir, TIFFGetDirCnt(tif,dircount)) < 0)
		    goto bad;
	} else if (td->td_planarconfig == PLANARCONFIG_CONTIG
		   && td->td_nstrips > 2
		   && td->td_compression == COMPRESSION_NONE
		   && _TIFFGetByteCount(tif, 0) != _TIFFGetByteCount(tif, 1)) {
		/*
		 * XXX: Some vendors fill StripByteCount array with absolutely
		 * wrong values (it can be equal to StripOffset array, for
		 * example). Catch this case here.
		 */
		TIFFWarningExt(tif->tif_clientdata, module,
	"%s: Wrong \"%s\" field, ignoring and calculating from imagelength",
                            tif->tif_name,
		            _TIFFFieldWithTag(tif,TIFFTAG_STRIPBYTECOUNTS)->field_name);
		if (EstimateStripByteCounts(tif, dir, TIFFGetDirCnt(tif,dircount)) < 0)
		    goto bad;
	}
	if (dir) {
		_TIFFfree((char *)dir);
		dir = NULL;
	}
	if (!TIFFFieldSet(tif, FIELD_MAXSAMPLEVALUE))
		td->td_maxsamplevalue = (uint16)((1L<<td->td_bitspersample)-1);

	/*
	 * XXX: We can optimize checking for the strip bounds using the sorted
	 * bytecounts array. See also comments for TIFFAppendToStrip()
	 * function in tif_write.c.
	 */
	if (tif->tif_mode != O_RDONLY && td->td_nstrips > 1) {
		tstrip_t strip;

		td->td_stripbytecountsorted = 1;
		for (strip = 1; strip < td->td_nstrips; strip++) {
			if (_TIFFGetOffset(tif, strip - 1) > _TIFFGetOffset(tif, strip)) {
				td->td_stripbytecountsorted = 0;
				break;
			}
		}
	}

	/*
	 * Setup default compression scheme.
	 */
	if (!TIFFFieldSet(tif, FIELD_COMPRESSION))
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        /*
         * Some manufacturers make life difficult by writing
	 * large amounts of uncompressed data as a single strip.
	 * This is contrary to the recommendations of the spec.
         * The following makes an attempt at breaking such images
	 * into strips closer to the recommended 8k bytes.  A
	 * side effect, however, is that the RowsPerStrip tag
	 * value may be changed.
         */
	if (td->td_nstrips == 1 && td->td_compression == COMPRESSION_NONE &&
	    (tif->tif_flags & (TIFF_STRIPCHOP|TIFF_ISTILED)) == TIFF_STRIPCHOP)
		ChopUpSingleUncompressedStrip(tif);

	/*
	 * Reinitialize i/o since we are starting on a new directory.
	 */
	tif->tif_row = (uint32) -1;
	tif->tif_curstrip = (tstrip_t) -1;
	tif->tif_col = (uint32) -1;
	tif->tif_curtile = (ttile_t) -1;
	tif->tif_tilesize = (tsize_t) -1;

	tif->tif_scanlinesize = TIFFScanlineSize(tif);
	if (!tif->tif_scanlinesize) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: cannot handle zero scanline size",
			  tif->tif_name);
		return (0);
	}

	if (isTiled(tif)) {
		tif->tif_tilesize = TIFFTileSize(tif);
		if (!tif->tif_tilesize) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: cannot handle zero tile size",
				  tif->tif_name);
			return (0);
		}
	} else {
		if (!TIFFStripSize(tif)) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: cannot handle zero strip size",
				  tif->tif_name);
			return (0);
		}
	}
	return (1);
bad:
	if (dir)
		_TIFFfree(dir);
	return (0);
}

/* 
 * Read custom directory from the arbitrary offset.
 * The code is very similar to TIFFReadDirectory().
 */
int
TIFFReadCustomDirectory(TIFF* tif, toff_t diroff,
			const TIFFFieldInfo info[], size_t n)
{
	const char *module = "TIFFReadCustomDirectory";
	TIFFDirectory* td = &tif->tif_dir;
	TIFFDirEntry *dp, *dir = NULL;
	const TIFFFieldInfo* fip;
	size_t fix;
	uint16 i; 
	uint64 dircount;	/* 16-bit or 64-bit entry count */

	_TIFFSetupFieldInfo(tif, info, n);

	tif->tif_diroff = diroff;

	if (!isMapped(tif)) {
		if (!SeekOK(tif, diroff)) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Seek error accessing TIFF directory",
                            tif->tif_name);
			return (0);
		}
		if (!ReadOK(tif, &dircount, TIFFDirCntLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Can not read TIFF directory count (%d)",
                            tif->tif_name, TIFFGetErrno(tif));
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirCnt(tif,&dircount);
		dir = (TIFFDirEntry *)_TIFFCheckMalloc(tif, TIFFGetDirCnt(tif,dircount), TDIREntryLen(tif),
					"to read TIFF custom directory");
		if (dir == NULL)
			return (0);
		if (!ReadOK(tif, dir, TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif))) {
			TIFFErrorExt(tif->tif_clientdata, module,
                                  "%.100s: Can not read TIFF directory (%d)",
                                  tif->tif_name, TIFFGetErrno(tif));
			goto bad;
		}
	} else {
		toff_t off = diroff;

		if (off + TIFFDirCntLen(tif) > tif->tif_size) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Can not read TIFF directory count",
                            tif->tif_name);
			return (0);
		} else
			_TIFFmemcpy(&dircount, tif->tif_base + off, TIFFDirCntLen(tif));
		off += TIFFDirCntLen(tif);
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabDirCnt(tif,&dircount);
		dir = (TIFFDirEntry *)_TIFFCheckMalloc(tif, TIFFGetDirCnt(tif,dircount), TDIREntryLen(tif),
					"to read TIFF custom directory");
		if (dir == NULL)
			return (0);
		if (off + TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif) > tif->tif_size) {
			TIFFErrorExt(tif->tif_clientdata, module,
                                  "%s: Can not read TIFF directory",
                                  tif->tif_name);
			goto bad;
		} else {
			_TIFFmemcpy(dir, tif->tif_base + off,
				    TIFFGetDirCnt(tif,dircount) * TDIREntryLen(tif));
		}
	}

	TIFFFreeDirectory(tif);

	fix = 0;
	for (dp = dir, i = TIFFGetDirCnt(tif,dircount); i > 0; i--, TDIREntryNext(tif,dp)) {
		if (tif->tif_flags & TIFF_SWAB) {
			TIFFSwabShort(&dp->s.tdir_tag);
			TIFFSwabShort(&dp->s.tdir_type);
			TDIRSwabEntryCount(tif,dp);
			TDIRSwabEntryOff(tif,dp);
		}

		if (fix >= tif->tif_nfields || dp->s.tdir_tag == IGNORE)
			continue;

		while (fix < tif->tif_nfields &&
		       tif->tif_fieldinfo[fix]->field_tag < dp->s.tdir_tag)
			fix++;

		if (fix >= tif->tif_nfields ||
		    tif->tif_fieldinfo[fix]->field_tag != dp->s.tdir_tag) {

			TIFFWarningExt(tif->tif_clientdata, module,
                        "%s: unknown field with tag %d (0x%x) encountered",
				    tif->tif_name, dp->s.tdir_tag, dp->s.tdir_tag,
				    dp->s.tdir_type);

			TIFFMergeFieldInfo(tif,
					   _TIFFCreateAnonFieldInfo(tif,
						dp->s.tdir_tag,
						(TIFFDataType) dp->s.tdir_type),
					   1);

			fix = 0;
			while (fix < tif->tif_nfields &&
			       tif->tif_fieldinfo[fix]->field_tag < dp->s.tdir_tag)
				fix++;
		}
		/*
		 * Null out old tags that we ignore.
		 */
		if (tif->tif_fieldinfo[fix]->field_bit == FIELD_IGNORE) {
	ignore:
			dp->s.tdir_tag = IGNORE;
			continue;
		}
		/*
		 * Check data type.
		 */
		fip = tif->tif_fieldinfo[fix];
		while (dp->s.tdir_type != (unsigned short) fip->field_type
                       && fix < tif->tif_nfields) {
			if (fip->field_type == TIFF_ANY)	/* wildcard */
				break;
                        fip = tif->tif_fieldinfo[++fix];
			if (fix >= tif->tif_nfields ||
			    fip->field_tag != dp->s.tdir_tag) {
				TIFFWarningExt(tif->tif_clientdata, module,
			"%s: wrong data type %d for \"%s\"; tag ignored",
					    tif->tif_name, dp->s.tdir_type,
					    tif->tif_fieldinfo[fix-1]->field_name);
				goto ignore;
			}
		}
		/*
		 * Check count if known in advance.
		 */
		if (fip->field_readcount != TIFF_VARIABLE
		    && fip->field_readcount != TIFF_VARIABLE2) {
			uint32 expected = (fip->field_readcount == TIFF_SPP) ?
			    (uint32) td->td_samplesperpixel :
			    (uint32) fip->field_readcount;
			if (!CheckDirCount(tif, dp, expected))
				goto ignore;
		}

		(void) TIFFFetchNormalTag(tif, dp);
	}
	
	if (dir)
		_TIFFfree(dir);
	return 1;

bad:
	if (dir)
		_TIFFfree(dir);
	return 0;
}

/*
 * EXIF is important special case of custom IFD, so we have a special
 * function to read it.
 */
int
TIFFReadEXIFDirectory(TIFF* tif, toff_t diroff)
{
	size_t exifFieldInfoCount;
	const TIFFFieldInfo *exifFieldInfo =
		_TIFFGetExifFieldInfo(&exifFieldInfoCount);
	return TIFFReadCustomDirectory(tif, diroff, exifFieldInfo,
				       exifFieldInfoCount);
}

/*
 * Function estimates strip byte counts when they appear to be messed up
 * or missing.  This logic doesn't take into account the possibility of
 * BigTIFF files but that should be okay since any files which have this
 * problem were written before BigTIFF.
 */
static int
EstimateStripByteCounts(TIFF* tif, TIFFDirEntry* dir, uint64 dircount)
{
	static const char module[] = "EstimateStripByteCounts";

	register TIFFDirEntry *dp;
	register TIFFDirectory *td = &tif->tif_dir;
	uint16 i;

	/*
	 * Reallocate strip byte count buffer for correct number of entries
	 */
	_TIFFfree(td->td_stripbcsbuf);
	td->td_stripbcstype = TIFF_LONG;
	td->td_stripbcscnt = td->td_nstrips;
	td->td_stripbcsoff = 0;
	td->td_stripbcsbuf = (uint32*) _TIFFCheckMalloc(tif, td->td_nstrips, sizeof(uint32),
		"for \"StripByteCounts\" array");
	if (td->td_compression != COMPRESSION_NONE) {
		uint32 space = (uint32)(sizeof (TIFFHeader)
		    + sizeof (uint16)
		    + (dircount * TDIREntryLen(tif))
		    + sizeof (uint32));
		toff_t filesize = TIFFGetFileSize(tif);
		uint64 n;

		/* calculate amount of space used by indirect values */
		for (dp = dir, n = dircount; n > 0; n--, TDIREntryNext(tif,dp))
		{
			uint32 cc = TIFFDataWidth((TIFFDataType) dp->s.tdir_type);
			if (cc == 0) {
				TIFFErrorExt(tif->tif_clientdata, module,
			"%s: Cannot determine size of unknown tag type %d",
					  tif->tif_name, dp->s.tdir_type);
				return -1;
			}
			cc = cc * TDIRGetEntryCount(tif,dp);
			if (cc > sizeof (uint32))
				space += cc;
		}
		space = (int32) (filesize - space);
		if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
			space /= td->td_samplesperpixel;
		for (i = 0; i < td->td_nstrips; i++)
			_TIFFSetByteCount(tif, i, space);
		/*
		 * This gross hack handles the case were the offset to
		 * the last strip is past the place where we think the strip
		 * should begin.  Since a strip of data must be contiguous,
		 * it's safe to assume that we've overestimated the amount
		 * of data in the strip and trim this number back accordingly.
		 */ 
		i--;
		if (((toff_t)(_TIFFGetOffset(tif, i)+_TIFFGetByteCount(tif, i))) > filesize)
			_TIFFSetByteCount(tif, i, 
			    (uint32) (filesize - _TIFFGetOffset(tif, i)));
	} else {
		uint32 rowbytes = TIFFScanlineSize(tif);
		uint32 rowsperstrip = td->td_imagelength/td->td_stripsperimage;
		for (i = 0; i < td->td_nstrips; i++)
			_TIFFSetByteCount(tif, i, rowbytes*rowsperstrip);
	}
	TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
	if (!TIFFFieldSet(tif, FIELD_ROWSPERSTRIP))
		td->td_rowsperstrip = td->td_imagelength;
	return 1;
}

static void
MissingRequired(TIFF* tif, const char* tagname)
{
	static const char module[] = "MissingRequired";

	TIFFErrorExt(tif->tif_clientdata, module,
		  "%s: TIFF directory is missing required \"%s\" field",
		  tif->tif_name, tagname);
}

/*
 * Check the count field of a directory
 * entry against a known value.  The caller
 * is expected to skip/ignore the tag if
 * there is a mismatch.
 */
static int
CheckDirCount(TIFF* tif, TIFFDirEntry* dir, uint64 count)
{
	if (count > TDIRGetEntryCount(tif,dir)) {
		TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
	"incorrect count for field \"%s\" (%lu, expecting %lu); tag ignored",
		    _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name,
		    TDIRGetEntryCount(tif,dir), count);
		return (0);
	} else if (count < TDIRGetEntryCount(tif,dir)) {
		TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
	"incorrect count for field \"%s\" (%lu, expecting %lu); tag trimmed",
		    _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name,
		    TDIRGetEntryCount(tif,dir), count);
		return (1);
	}
	return (1);
}

/*
 * Fetch a contiguous directory item.
 */
static tsize_t
TIFFFetchData(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	int w = TIFFDataWidth((TIFFDataType) dir->s.tdir_type);
	tsize_t cc = TDIRGetEntryCount(tif,dir) * w;

	/* Check for overflow. */
	if (!TDIRGetEntryCount(tif,dir) || !w || cc / w != (tsize_t)TDIRGetEntryCount(tif,dir))
		goto bad;

	if (!isMapped(tif)) {
		if (!SeekOK(tif, TDIRGetEntryOff(tif,dir)))
			goto bad;
		if (!ReadOK(tif, cp, cc))
			goto bad;
	} else {
		/* Check for overflow. */
		if (TDIRGetEntryOff(tif,dir) + cc > tif->tif_size)
			goto bad;
		_TIFFmemcpy(cp, tif->tif_base + TDIRGetEntryOff(tif,dir), cc);
	}
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
			TIFFSwabArrayOfLong((uint32*) cp, 2*TDIRGetEntryCount(tif,dir));
			break;
		case TIFF_DOUBLE:
			TIFFSwabArrayOfDouble((double*) cp, TDIRGetEntryCount(tif,dir));
			break;
		}
	}
	return (cc);
bad:
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		     "Error fetching data for field \"%s\"",
		     _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name);
	return (tsize_t) 0;
}

/*
 * Fetch an ASCII item from the file.
 */
static tsize_t
TIFFFetchString(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif)) {
		if (tif->tif_flags & TIFF_SWAB)
			TDIRSwabEntryOff(tif,dir);
		_TIFFmemcpy(cp, TDIRAddrEntryOff(tif,dir),
			TDIRGetEntryCount(tif,dir) * sizeof(uint16));
		return (1);
	}
	return (TIFFFetchData(tif, dir, cp));
}

/*
 * Convert numerator+denominator to float.
 */
static int
cvtRational(TIFF* tif, TIFFDirEntry* dir, uint32 num, uint32 denom, float* rv)
{
	if (denom == 0) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "%s: Rational with zero denominator (num = %lu)",
		    _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name, num);
		return (0);
	} else {
		if (dir->s.tdir_type == TIFF_RATIONAL)
			*rv = ((float)num / (float)denom);
		else
			*rv = ((float)(int32)num / (float)(int32)denom);
		return (1);
	}
}

/*
 * Fetch a rational item from the file
 * at offset off and return the value
 * as a floating point number.
 */
static float
TIFFFetchRational(TIFF* tif, TIFFDirEntry* dir)
{
	uint32 l[2];
	float v;

	return (!TIFFFetchData(tif, dir, (char *)l) ||
	    !cvtRational(tif, dir, l[0], l[1], &v) ? 1.0f : v);
}

/*
 * Fetch a single floating point value
 * from the offset field and return it
 * as a native float.
 */
static float
TIFFFetchFloat(TIFF* tif, TIFFDirEntry* dir)
{
	float v;
        _TIFFmemcpy(&v, TDIRAddrEntryOff(tif,dir), sizeof(float));
	TIFFCvtIEEEFloatToNative(tif, 1, &v);
	return (v);
}

/*
 * Fetch an array of BYTE or SBYTE values.
 */
static int
TIFFFetchByteArray(TIFF* tif, TIFFDirEntry* dir, uint8* v)
{
    if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif)) {
	if (tif->tif_flags & TIFF_SWAB)
		TDIRSwabEntryOff(tif,dir);
	_TIFFmemcpy(v, TDIRAddrEntryOff(tif,dir),
		TDIRGetEntryCount(tif,dir));
        return (1);
    } else
        return (TIFFFetchData(tif, dir, (char*) v) != 0);	/* XXX */
}

/*
 * Fetch an array of SHORT or SSHORT values.
 */
static int
TIFFFetchShortArray(TIFF* tif, TIFFDirEntry* dir, uint16* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(uint16)) {
		if (tif->tif_flags & TIFF_SWAB) {
			// dir.s.tdir_offset is aleady swapped as a long
			// dir.h.tdir_offset is aleady swapped as a uint64
			// TIFFFetchShortArray() is called multiple times for the same tag,
			//  therefore do not store the word swapped code back into the DirEntry.
			int n = TDIRGetEntryCount(tif,dir);
			uint16* pSrc = (uint16*)TDIRAddrEntryOff(tif,dir);
			pSrc += ((TDIREntryOffLen(tif) / sizeof(uint16)) - 1); 
			while (n-- > 0) {
				*v++ = *pSrc--;
			}
		}
		else {
			_TIFFmemcpy(v, TDIRAddrEntryOff(tif,dir),
				TDIRGetEntryCount(tif,dir) * sizeof(uint16));
		}
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char *)v) != 0);
}

/*
 * Fetch a pair of SHORT or BYTE values. Some tags may have either BYTE
 * or SHORT type and this function works with both ones.
 */
static int
TIFFFetchShortPair(TIFF* tif, TIFFDirEntry* dir)
{
	switch (dir->s.tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
			{
			uint8 v[4];
			return TIFFFetchByteArray(tif, dir, v)
				&& TIFFSetField(tif, dir->s.tdir_tag, v[0], v[1]);
			}
		case TIFF_SHORT:
		case TIFF_SSHORT:
			{
			uint16 v[2];
			return TIFFFetchShortArray(tif, dir, v)
				&& TIFFSetField(tif, dir->s.tdir_tag, v[0], v[1]);
			}
		default:
			return 0;
	}
}

/*
 * Fetch an array of LONG or SLONG values.
 */
static int
TIFFFetchLongArray(TIFF* tif, TIFFDirEntry* dir, uint32* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(uint32)) {
		if (tif->tif_flags & TIFF_SWAB) {
			TDIRSwabEntryOff(tif,dir);
			TIFFSwabArrayOfLong(TDIRAddrEntryOff(tif,dir), 
				TDIRGetEntryCount(tif,dir));
		}
		_TIFFmemcpy(v, TDIRAddrEntryOff(tif,dir),
			TDIRGetEntryCount(tif,dir) * sizeof(uint32));
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char*) v) != 0);
}

/*
 * Fetch an array of LONG8 or SLONG8 values.
 */
static int
TIFFFetchLong8Array(TIFF* tif, TIFFDirEntry* dir, uint64* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(uint64)) {
		if (tif->tif_flags & TIFF_SWAB) {
			TDIRSwabEntryOff(tif,dir);
			TIFFSwabArrayOfLong8(TDIRAddrEntryOff(tif,dir), 
				TDIRGetEntryCount(tif,dir));
		}
		_TIFFmemcpy(v, TDIRAddrEntryOff(tif,dir),
			TDIRGetEntryCount(tif,dir) * sizeof(uint64));
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char*) v) != 0);
}

/*
 * Fetch an array of RATIONAL or SRATIONAL values.
 */
static int
TIFFFetchRationalArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	int ok = 0;
	uint32* l;

	l = (uint32*)_TIFFCheckMalloc(tif,
	    TDIRGetEntryCount(tif,dir), TIFFDataWidth((TIFFDataType) dir->s.tdir_type),
	    "to fetch array of rationals");
	if (l) {
		if (TIFFFetchLongArray(tif, dir, l)) {
			uint32 i;
			uint32 count = TDIRGetEntryCount(tif,dir) * 2;
			for (i = 0; i < count; i += 2) {
				ok = cvtRational(tif, dir, l[i], l[i+1], &v[i/2]);
				if (!ok)
					break;
			}
		}
		_TIFFfree((char *)l);
	}
	return (ok);
}

/*
 * Fetch an array of FLOAT values.
 */
static int
TIFFFetchFloatArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(float)) {
		if (tif->tif_flags & TIFF_SWAB) {
			TDIRSwabEntryOff(tif,dir);
			TIFFSwabArrayOfLong(TDIRAddrEntryOff(tif,dir), 
				TDIRGetEntryCount(tif,dir));
		}
		_TIFFmemcpy(v, TDIRAddrEntryOff(tif,dir),
			TDIRGetEntryCount(tif,dir) * sizeof(float));
	} else if (!TIFFFetchData(tif, dir, (char*) v))
		return (0);
	TIFFCvtIEEEFloatToNative(tif, TDIRGetEntryCount(tif,dir), v);
	return (1);
}

/*
 * Fetch an array of DOUBLE values.
 */
static int
TIFFFetchDoubleArray(TIFF* tif, TIFFDirEntry* dir, double* v)
{
	if (TDIRGetEntryCount(tif,dir) <= TDIREntryOffLen(tif) / sizeof(double)) {
		if (tif->tif_flags & TIFF_SWAB) {
			TDIRSwabEntryOff(tif,dir);
			TIFFSwabArrayOfLong8(TDIRAddrEntryOff(tif,dir), 
				TDIRGetEntryCount(tif,dir));\
		}
		_TIFFmemcpy(v, TDIRAddrEntryOff(tif,dir),
			TDIRGetEntryCount(tif,dir) * sizeof(double));
	} else if (!TIFFFetchData(tif, dir, (char*) v))
		return (0);
	TIFFCvtIEEEDoubleToNative(tif, TDIRGetEntryCount(tif,dir), v);
	return (1);
}

/*
 * Fetch an array of ANY values.  The actual values are
 * returned as doubles which should be able hold all the
 * types.  Yes, there really should be an tany_t to avoid
 * this potential non-portability ...  Note in particular
 * that we assume that the double return value vector is
 * large enough to read in any fundamental type.  We use
 * that vector as a buffer to read in the base type vector
 * and then convert it in place to double (from end
 * to front of course).
 *
 * This routine does not support 64-bit data types, which is
 * okay because they're not used for sample type.
 */
static int
TIFFFetchAnyArray(TIFF* tif, TIFFDirEntry* dir, double* v)
{
	int i;

	switch (dir->s.tdir_type) {
	case TIFF_BYTE:
	case TIFF_SBYTE:
		if (!TIFFFetchByteArray(tif, dir, (uint8*) v))
			return (0);
		if (dir->s.tdir_type == TIFF_BYTE) {
			uint8* vp = (uint8*) v;
			for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
				v[i] = vp[i];
		} else {
			int8* vp = (int8*) v;
			for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
				v[i] = vp[i];
		}
		break;
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (!TIFFFetchShortArray(tif, dir, (uint16*) v))
			return (0);
		if (dir->s.tdir_type == TIFF_SHORT) {
			uint16* vp = (uint16*) v;
			for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
				v[i] = vp[i];
		} else {
			int16* vp = (int16*) v;
			for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
				v[i] = vp[i];
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
		if (!TIFFFetchLongArray(tif, dir, (uint32*) v))
			return (0);
		if (dir->s.tdir_type == TIFF_LONG) {
			uint32* vp = (uint32*) v;
			for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
				v[i] = vp[i];
		} else {
			int32* vp = (int32*) v;
			for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
				v[i] = vp[i];
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (!TIFFFetchRationalArray(tif, dir, (float*) v))
			return (0);
		{ float* vp = (float*) v;
		  for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
			v[i] = vp[i];
		}
		break;
	case TIFF_FLOAT:
		if (!TIFFFetchFloatArray(tif, dir, (float*) v))
			return (0);
		{ float* vp = (float*) v;
		  for (i = TDIRGetEntryCount(tif,dir)-1; i >= 0; i--)
			v[i] = vp[i];
		}
		break;
	case TIFF_DOUBLE:
		return (TIFFFetchDoubleArray(tif, dir, (double*) v));
	default:
		/* TIFF_NOTYPE */
		/* TIFF_ASCII */
		/* TIFF_UNDEFINED */
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "cannot read TIFF_ANY type %d for field \"%s\"",
			     dir->s.tdir_type,
			     _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name);
		return (0);
	}
	return (1);
}

/*
 * Fetch a tag that is not handled by special case code.
 */
static int
TIFFFetchNormalTag(TIFF* tif, TIFFDirEntry* dp)
{
	static const char mesg[] = "to fetch tag value";
	int ok = 0;
	const TIFFFieldInfo* fip = _TIFFFieldWithTag(tif, dp->s.tdir_tag);

	if (TDIRGetEntryCount(tif,dp) > 1) {		/* array of values */
		char* cp = NULL;

		switch (dp->s.tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (uint8), mesg);
			ok = cp && TIFFFetchByteArray(tif, dp, (uint8*) cp);
			break;
		case TIFF_SHORT:
		case TIFF_SSHORT:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (uint16), mesg);
			ok = cp && TIFFFetchShortArray(tif, dp, (uint16*) cp);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_IFD:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (uint32), mesg);
			ok = cp && TIFFFetchLongArray(tif, dp, (uint32*) cp);
			break;
		case TIFF_LONG8:
		case TIFF_SLONG8:
		case TIFF_IFD8:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (uint64), mesg);
			ok = cp && TIFFFetchLong8Array(tif, dp, (uint64*) cp);
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (float), mesg);
			ok = cp && TIFFFetchRationalArray(tif, dp, (float*) cp);
			break;
		case TIFF_FLOAT:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (float), mesg);
			ok = cp && TIFFFetchFloatArray(tif, dp, (float*) cp);
			break;
		case TIFF_DOUBLE:
			cp = (char *)_TIFFCheckMalloc(tif,
			    TDIRGetEntryCount(tif,dp), sizeof (double), mesg);
			ok = cp && TIFFFetchDoubleArray(tif, dp, (double*) cp);
			break;
		case TIFF_ASCII:
		case TIFF_UNDEFINED:		/* bit of a cheat... */
			/*
			 * Some vendors write strings w/o the trailing
			 * NULL byte, so always append one just in case.
			 */
			cp = (char *)_TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dp) + 1,
						      1, mesg);
			if( (ok = (cp && TIFFFetchString(tif, dp, cp))) != 0 )
				cp[TDIRGetEntryCount(tif,dp)] = '\0';	/* XXX */
			break;
		}
		if (ok) {
			ok = (fip->field_passcount ?
			    TIFFSetField(tif, dp->s.tdir_tag, TDIRGetEntryCount(tif,dp), cp)
			  : TIFFSetField(tif, dp->s.tdir_tag, cp));
		}
		if (cp != NULL)
			_TIFFfree(cp);
	} else if (CheckDirCount(tif, dp, 1)) {	/* singleton value */
		switch (dp->s.tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
		case TIFF_SHORT:
		case TIFF_SSHORT:
			/*
			 * If the tag is also acceptable as a LONG or SLONG
			 * then TIFFSetField will expect an uint32 parameter
			 * passed to it (through varargs).  Thus, for machines
			 * where sizeof (int) != sizeof (uint32) we must do
			 * a careful check here.  It's hard to say if this
			 * is worth optimizing.
			 *
			 * NB: We use TIFFFieldWithTag here knowing that
			 *     it returns us the first entry in the table
			 *     for the tag and that that entry is for the
			 *     widest potential data type the tag may have.
			 */
			{ 
			  if (dp->s.tdir_type == TIFF_SHORT) {
			    uint16 v;
			    ok = (TIFFFetchShortArray(tif, dp, &v) &&
			          (fip->field_passcount ?
				    TIFFSetField(tif, dp->s.tdir_tag, 1, &v)
				  : TIFFSetField(tif, dp->s.tdir_tag, v))
			    );
			    break;
			  }
			}
			/* fall thru... */
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_IFD:
			{ uint32 v;
			  ok = (TIFFFetchLongArray(tif, dp, &v) &&
			    (fip->field_passcount ? 
			      TIFFSetField(tif, dp->s.tdir_tag, 1, &v)
			    : TIFFSetField(tif, dp->s.tdir_tag, v))
			  );
			}
			break;
		case TIFF_LONG8:
		case TIFF_SLONG8:
		case TIFF_IFD8:
			{ uint64 v;
			  ok = (TIFFFetchLong8Array(tif, dp, &v) &&
			    (fip->field_passcount ?
			      TIFFSetField(tif, dp->s.tdir_tag, 1, &v)
			    : TIFFSetField(tif, dp->s.tdir_tag, v))
			  );
			}
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
		case TIFF_FLOAT:
			{ float v = (dp->s.tdir_type == TIFF_FLOAT ? 
			      TIFFFetchFloat(tif, dp)
			    : TIFFFetchRational(tif, dp));
			  ok = (fip->field_passcount ?
			      TIFFSetField(tif, dp->s.tdir_tag, 1, &v)
			    : TIFFSetField(tif, dp->s.tdir_tag, v));
			}
			break;
		case TIFF_DOUBLE:
			{ double v;
			  ok = (TIFFFetchDoubleArray(tif, dp, &v) &&
			    (fip->field_passcount ?
			      TIFFSetField(tif, dp->s.tdir_tag, 1, &v)
			    : TIFFSetField(tif, dp->s.tdir_tag, v))
			  );
			}
			break;
		case TIFF_ASCII:
		case TIFF_UNDEFINED:		/* bit of a cheat... */
			{ char c[2];
			  if( (ok = (TIFFFetchString(tif, dp, c) != 0)) != 0 ) {
				c[1] = '\0';		/* XXX paranoid */
				ok = (fip->field_passcount ?
					TIFFSetField(tif, dp->s.tdir_tag, 1, c)
				      : TIFFSetField(tif, dp->s.tdir_tag, c));
			  }
			}
			break;
		}
	}
	return (ok);
}

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Fetch samples/pixel short values for 
 * the specified tag and verify that
 * all values are the same.
 */
static int
TIFFFetchPerSampleShorts(TIFF* tif, TIFFDirEntry* dir, uint16* pl)
{
    uint16 samples = tif->tif_dir.td_samplesperpixel;
    int status = 0;

    if (CheckDirCount(tif, dir, samples)) {
        uint16 buf[10];
        uint16* v = buf;

        if (TDIRGetEntryCount(tif,dir) > NITEMS(buf))
            v = (uint16*) _TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dir), sizeof(uint16),
                                      "to fetch per-sample values");
        if (v && TIFFFetchShortArray(tif, dir, v)) {
            uint16 i;
            int check_count = TDIRGetEntryCount(tif,dir);
            if( samples < check_count )
                check_count = samples;

            for (i = 1; i < check_count; i++)
                if (v[i] != v[0]) {
					TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
                              "Cannot handle different per-sample values for field \"%s\"",
                              _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name);
                    goto bad;
                }
            *pl = v[0];
            status = 1;
        }
      bad:
        if (v && v != buf)
            _TIFFfree(v);
    }
    return (status);
}

/*
 * Fetch samples/pixel long values for 
 * the specified tag and verify that
 * all values are the same.
 */
static int
TIFFFetchPerSampleLongs(TIFF* tif, TIFFDirEntry* dir, uint32* pl)
{
    uint16 samples = tif->tif_dir.td_samplesperpixel;
    int status = 0;

    if (CheckDirCount(tif, dir, (uint32) samples)) {
        uint32 buf[10];
        uint32* v = buf;

        if (TDIRGetEntryCount(tif,dir) > NITEMS(buf))
            v = (uint32*) _TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dir), sizeof(uint32),
                                      "to fetch per-sample values");
        if (v && TIFFFetchLongArray(tif, dir, v)) {
            uint16 i;
            int check_count = TDIRGetEntryCount(tif,dir);

            if( samples < check_count )
                check_count = samples;
            for (i = 1; i < check_count; i++)
                if (v[i] != v[0]) {
					TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
                              "Cannot handle different per-sample values for field \"%s\"",
                              _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name);
                    goto bad;
                }
            *pl = v[0];
            status = 1;
        }
      bad:
        if (v && v != buf)
            _TIFFfree(v);
    }
    return (status);
}

/*
 * Fetch samples/pixel ANY values for the specified tag and verify that all
 * values are the same.
 */
static int
TIFFFetchPerSampleAnys(TIFF* tif, TIFFDirEntry* dir, double* pl)
{
    uint16 samples = tif->tif_dir.td_samplesperpixel;
    int status = 0;

    if (CheckDirCount(tif, dir, (uint32) samples)) {
        double buf[10];
        double* v = buf;

        if (TDIRGetEntryCount(tif,dir) > NITEMS(buf))
            v = (double*) _TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dir), sizeof (double),
                                      "to fetch per-sample values");
        if (v && TIFFFetchAnyArray(tif, dir, v)) {
            uint16 i;
            int check_count = TDIRGetEntryCount(tif,dir);
            if( samples < check_count )
                check_count = samples;

            for (i = 1; i < check_count; i++)
                if (v[i] != v[0]) {
                    TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
                              "Cannot handle different per-sample values for field \"%s\"",
                              _TIFFFieldWithTag(tif, dir->s.tdir_tag)->field_name);
                    goto bad;
                }
            *pl = v[0];
            status = 1;
        }
      bad:
        if (v && v != buf)
            _TIFFfree(v);
    }
    return (status);
}
#undef NITEMS

/*
 * Get a particular strip/tile's offset.  Strip/tile offsets are loaded in "blocks" with
 * a maximum size.  Returns 0 if invalid.
 */
extern toff_t
_TIFFGetOffset(TIFF* tif, tstrip_t strip)
{
	TIFFDirectory *td = &tif->tif_dir;
	int	status = 1;
	uint32	blkno = strip / td->td_stripbufmax;
	uint32	tdi;

	if (strip >= td->td_nstrips)		/* range check tile/strip number */
		return (0);
	/*
	 * Check if this strip/tile's offset block is loaded, if not, load it
	 */
	if (blkno != td->td_stripoffsblk) {
		/*
		 * Load strip/tile's offsets block if possible.  Sometimes there are fewer
		 * offsets than the theoretical number - if so return zero.  The offsets block 
		 * is loaded using a directory entry modified to describe just the block's
		 * portion of the entire block table.
		 *
		 * Offsets can be LONG8, LONG, or SHORT, but are used internally as toff_t.
		 */
		TIFFDirEntry	offsent;	/* fake dir entry for I/O to file */
	
		_TIFFFlushOffsets(tif);		/* write out updated block if required */
		offsent.s.tdir_tag = (isTiled(tif) ? TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS);
		offsent.s.tdir_type = td->td_stripoffstype;
		TDIRSetEntryCount(tif, &offsent, 
			TIFFmin(td->td_stripbufmax, td->td_nstrips - blkno * td->td_stripbufmax));
		TDIRSetEntryOff(tif, &offsent,
			td->td_stripoffsoff + blkno * td->td_stripbufmax * TIFFDataWidth(offsent.s.tdir_type));
		if (!td->td_stripoffsbuf)
			td->td_stripoffsbuf = (toff_t*) _TIFFCheckMalloc(tif,
					TIFFmin(td->td_stripbufmax, td->td_nstrips), sizeof(toff_t), 
					"offsets array");
		if (blkno >= TIFFhowmany(td->td_stripoffscnt, td->td_stripbufmax)) {
			_TIFFmemset(td->td_stripoffsbuf, 0, TDIRGetEntryCount(tif,&offsent) * sizeof(toff_t));
		} else if (offsent.s.tdir_type == TIFF_LONG8) {
			status = TIFFFetchLong8Array(tif, &offsent, td->td_stripoffsbuf);
		} else if (offsent.s.tdir_type == TIFF_LONG) {
			if ((status = TIFFFetchLongArray(tif, &offsent, (uint32*) td->td_stripoffsbuf)) != 0)
				for (tdi = TDIRGetEntryCount(tif, &offsent); tdi-- > 0; )
					td->td_stripoffsbuf[tdi] = ((uint32*) td->td_stripoffsbuf)[tdi];
		} else {		/* TIFF_SHORT */
			if ((status = TIFFFetchShortArray(tif, &offsent, (uint16*) td->td_stripoffsbuf)) != 0)
				for (tdi = TDIRGetEntryCount(tif, &offsent); tdi-- > 0; )
					td->td_stripoffsbuf[tdi] = ((uint16*) td->td_stripoffsbuf)[tdi];
		}
		td->td_stripoffsblk = blkno;
	}
	/*
	 * Return tile/strip's offset
	 */
	if (status)
		return td->td_stripoffsbuf[strip % td->td_stripbufmax];
	else
		return (0);
}

/*
 * Get a particular strip/tile's byte count.  Strip/tile byte counts are loaded in "blocks" with
 * a maximum size.  Returns 0 if invalid.
 */
extern uint32
_TIFFGetByteCount(TIFF* tif, tstrip_t strip)
{
	TIFFDirectory *td = &tif->tif_dir;
	int	status = 1;
	uint32	blkno = strip / td->td_stripbufmax;
	uint32	tdi;

	if (strip >= td->td_nstrips)		/* range check tile/strip number */
		return (0);
	/*
	 * Check if this strip/tile's byte count block is loaded, if not, load it
	 */
	if (blkno != td->td_stripbcsblk) {
		/*
		 * Load strip/tile's byte count block if possible.  Sometimes there are fewer byte
		 * counts than the theoretical number - if so return zero.  The byte count block is
		 * loaded using a fake directory entry modified to describe just the block's
		 * portion of the entire byte count table.
		 *
		 * Byte counts can be LONG8, LONG, or SHORT, but are stored as uint32 (32-bit).  There's
		 * never a reason to use LONG8 but the BigTIFF spec provides for this possibilty; a 
		 * block of LONG8 byte counts must be read into a separate buffer because it may be 
		 * larger than the byte count block buffer.
		 */
		TIFFDirEntry	bcsent;		/* fake dir entry for I/O to file */

		_TIFFFlushByteCounts(tif);	/* write updated byte counts block if required */
		bcsent.s.tdir_tag = (isTiled(tif) ? TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS);
		bcsent.s.tdir_type = td->td_stripbcstype;
		TDIRSetEntryCount(tif, &bcsent, 
			TIFFmin(td->td_stripbufmax, td->td_nstrips - blkno * td->td_stripbufmax));
		TDIRSetEntryOff(tif, &bcsent,
			td->td_stripbcsoff + blkno * td->td_stripbufmax * TIFFDataWidth(bcsent.s.tdir_type));
		if (!td->td_stripbcsbuf)
			td->td_stripbcsbuf = (uint32*) _TIFFCheckMalloc(tif,
					TIFFmin(td->td_stripbufmax, td->td_nstrips), sizeof(uint32), 
					"byte counts array");
		if (blkno >= TIFFhowmany(td->td_stripbcscnt, td->td_stripbufmax)) {
			_TIFFmemset(td->td_stripbcsbuf, 0, TDIRGetEntryCount(tif, &bcsent) * sizeof(uint32));
		} else if (bcsent.s.tdir_type == TIFF_LONG8) {
			uint64 *bcsbuff = (uint64*) _TIFFCheckMalloc(tif, 
					TDIRGetEntryCount(tif, &bcsent), sizeof(uint64), 
					"64-bit byte counts array");
			if ((status = TIFFFetchLong8Array(tif, &bcsent, bcsbuff)) != 0)
				for (tdi = TDIRGetEntryCount(tif, &bcsent); tdi-- > 0; )
					td->td_stripbcsbuf[tdi] = (uint32) bcsbuff[tdi];
			_TIFFfree(bcsbuff);
		} else if (bcsent.s.tdir_type == TIFF_LONG) {
			status = TIFFFetchLongArray(tif, &bcsent, td->td_stripbcsbuf);
		} else {		/* TIFF_SHORT */
			if ((status = TIFFFetchShortArray(tif, &bcsent, (uint16*) td->td_stripbcsbuf)) != 0)
				for (tdi = TDIRGetEntryCount(tif, &bcsent); tdi-- > 0; )
					td->td_stripbcsbuf[tdi] = ((uint16*) td->td_stripbcsbuf)[tdi];
		}
		td->td_stripbcsblk = blkno;
	}
	/*
	 * Return tile/strip's offset
	 */
	if (status)
		return td->td_stripbcsbuf[strip % td->td_stripbufmax];
	else
		return (0);
}

/*
 * Fetch and set the RefBlackWhite tag.
 */
static int
TIFFFetchRefBlackWhite(TIFF* tif, TIFFDirEntry* dir)
{
	static const char mesg[] = "for \"ReferenceBlackWhite\" array";
	char* cp;
	int ok;

	if (dir->s.tdir_type == TIFF_RATIONAL)
		return (TIFFFetchNormalTag(tif, dir));
	/*
	 * Handle LONG's for backward compatibility.
	 */
	cp = (char *)_TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dir),
				      sizeof (uint32), mesg);
	if( (ok = (cp && TIFFFetchLongArray(tif, dir, (uint32*) cp))) != 0) {
		float* fp = (float*)
		    _TIFFCheckMalloc(tif, TDIRGetEntryCount(tif,dir), sizeof (float), mesg);
		if( (ok = (fp != NULL)) != 0 ) {
			uint32 i;
			for (i = 0; i < TDIRGetEntryCount(tif,dir); i++)
				fp[i] = (float)((uint32*) cp)[i];
			ok = TIFFSetField(tif, dir->s.tdir_tag, fp);
			_TIFFfree((char*) fp);
		}
	}
	if (cp)
		_TIFFfree(cp);
	return (ok);
}

/*
 * Replace a single strip (tile) of uncompressed data by
 * multiple strips (tiles), each approximately 8Kbytes.
 * This is useful for dealing with large images or
 * for dealing with machines with a limited amount
 * memory.
 */
static void
ChopUpSingleUncompressedStrip(TIFF* tif)
{
	register TIFFDirectory *td = &tif->tif_dir;
	uint32 bytecount = _TIFFGetByteCount(tif, 0);
	toff_t offset = _TIFFGetOffset(tif, 0);
	tsize_t rowbytes = TIFFVTileSize(tif, 1), 
		stripbytes;
	tstrip_t strip, nstrips, rowsperstrip;
	uint32* newcounts;
	toff_t* newoffsets;

	/*
	 * Make the rows hold at least one scanline, but fill specified amount
	 * of data if possible.
	 */
	if (rowbytes > STRIP_SIZE_DEFAULT) {
		stripbytes = rowbytes;
		rowsperstrip = 1;
	} else if (rowbytes > 0 ) {
		rowsperstrip = STRIP_SIZE_DEFAULT / rowbytes;
		stripbytes = rowbytes * rowsperstrip;
	}
        else
		return;

	/* 
	 * never increase the number of strips in an image
	 */
	if (rowsperstrip >= td->td_rowsperstrip)
		return;
	nstrips = (tstrip_t) TIFFhowmany(bytecount, stripbytes);
        if (nstrips == 0)		/* something is wonky, do nothing. */
		return;

	newcounts = (uint32*) _TIFFCheckMalloc(tif, nstrips, sizeof (uint32),
				"for chopped \"StripByteCounts\" array");
	newoffsets = (toff_t*) _TIFFCheckMalloc(tif, nstrips, sizeof (toff_t),
				"for chopped \"StripOffsets\" array");
	if (newcounts == NULL || newoffsets == NULL) {
	        /*
		 * Unable to allocate new strip information, give
		 * up and use the original one strip information.
		 */
		_TIFFfree(newcounts);
		_TIFFfree(newoffsets);
		return;
	}
	/*
	 * Fill the strip information arrays with new bytecounts and offsets
	 * that reflect the broken-up format.
	 */
	for (strip = 0; strip < nstrips; strip++) {
		if (stripbytes > (tsize_t) bytecount)
			stripbytes = bytecount;
		newcounts[strip] = stripbytes;
		newoffsets[strip] = offset;
		offset += stripbytes;
		bytecount -= stripbytes;
	}
	/*
	 * Replace old single strip info with multi-strip info.
	 */
	td->td_stripsperimage = td->td_nstrips = nstrips;
	TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);

	_TIFFfree(td->td_stripbcsbuf);
	_TIFFfree(td->td_stripoffsbuf);
	td->td_stripbcsbuf = newcounts;
	td->td_stripoffsbuf = newoffsets;
	td->td_stripbcsblk = 0;
	td->td_stripoffsblk = 0;
	td->td_stripbytecountsorted = 1;
}

// Debug
static void
printDir(TIFF* tif, TIFFDirEntry *dp, int offset16)
{
	int i;
	uint16 v[4];
	long* pOffset = (long*)TDIRAddrEntryOff(tif, dp);
	int numShorts = TDIRGetEntryCount(tif, dp);

	printf("\ntag =    %d\n", dp->s.tdir_tag);
	printf("count =  %d\n", dp->s.tdir_count);
	printf("offset = %d\t", *pOffset);
	if ((numShorts > 0) && (numShorts < 4)) {
		TIFFFetchShortArray(tif, dp, v);
		for (i = 0; i < numShorts; ++i) {
			printf("%d ", v[i]);
		}
	}
	else {
		// This indicates junk in the count field, and that the offset is not used for shorts
		printf ("n/a");
	}
	printf("\n");
}


/* vim: set ts=8 sts=8 sw=8 noet: */
