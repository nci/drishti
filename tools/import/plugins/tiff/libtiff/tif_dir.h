/* $Id: tif_dir.h,v 1.28 2005/12/26 14:31:25 dron Exp $ */

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

#ifndef _TIFFDIR_
#define	_TIFFDIR_
/*
 * ``Library-private'' Directory-related Definitions.
 */
#define	TIFF_STRIPBUFMAX (8*1024/sizeof(toff_t))  /* max offsets/byte counts in strip buffer */

/*
 * Internal format of a TIFF directory entry.
 */
typedef	struct {
#define	FIELD_SETLONGS	4
	/* bit vector of fields that are set */
	unsigned long	td_fieldsset[FIELD_SETLONGS];

	uint32	td_imagewidth, td_imagelength, td_imagedepth;
	uint32	td_tilewidth, td_tilelength, td_tiledepth;
	uint32	td_subfiletype;
	uint16	td_bitspersample;
	uint16	td_sampleformat;
	uint16	td_compression;
	uint16	td_photometric;
	uint16	td_threshholding;
	uint16	td_fillorder;
	uint16	td_orientation;
	uint16	td_samplesperpixel;
	uint32	td_rowsperstrip;
	uint16	td_minsamplevalue, td_maxsamplevalue;
	double	td_sminsamplevalue, td_smaxsamplevalue;
	float	td_xresolution, td_yresolution;
	uint16	td_resolutionunit;
	uint16	td_planarconfig;
	float	td_xposition, td_yposition;
	uint16	td_pagenumber[2];
	uint16*	td_colormap[3];
	uint16	td_halftonehints[2];
	uint16	td_extrasamples;
	uint16*	td_sampleinfo;
	tstrip_t td_stripsperimage;
	tstrip_t td_nstrips;		/* size of offset & bytecount arrays */
	
	uint32	td_stripbufmax;		/* max size of strip offset and byte count blocks */
	uint16	td_stripoffstype;	/* data type of offsets */
	uint32	td_stripoffscnt;	/* count of offsets in file */
	toff_t	td_stripoffsoff;	/* offset to offsets in file (0 = not placed) */
	int32	td_stripoffsblk;	/* currently loaded offsets block */
	toff_t* td_stripoffsbuf;	/* loaded strip offsets block pointer */
	int	td_stripoffsdirty;	/* whether current offsets block has been modified */
	uint16	td_stripbcstype;	/* data type of byte counts */
	uint32	td_stripbcscnt;		/* count of bytes counts in file */
	toff_t	td_stripbcsoff;		/* offset to bytes counts in file (0 = not placed */
	int32	td_stripbcsblk;		/* currently loaded byte counts block */
	uint32*	td_stripbcsbuf;		/* loaded strip byte counts block pointer */
	int	td_stripbcsdirty;	/* whether current byte counts block has been modified */
	int	td_stripbytecountsorted; /* is the bytecount array sorted ascending? */

	uint16	td_nsubifd;
	toff_t*	td_subifd;		/* subIFD array; values may not be set yet */
	/* YCbCr parameters */
	uint16	td_ycbcrsubsampling[2];
	uint16	td_ycbcrpositioning;
	/* Colorimetry parameters */
	uint16*	td_transferfunction[3];
	/* CMYK parameters */
	int	td_inknameslen;
	char*	td_inknames;

	int     td_customValueCount;
        TIFFTagValue *td_customValues;
} TIFFDirectory;

/*
 * Field flags used to indicate fields that have
 * been set in a directory, and to reference fields
 * when manipulating a directory.
 */

/*
 * FIELD_IGNORE is used to signify tags that are to
 * be processed but otherwise ignored.  This permits
 * antiquated tags to be quietly read and discarded.
 * Note that a bit *is* allocated for ignored tags;
 * this is understood by the directory reading logic
 * which uses this fact to avoid special-case handling
 */ 
#define	FIELD_IGNORE			0

/* multi-item fields */
#define	FIELD_IMAGEDIMENSIONS		1
#define FIELD_TILEDIMENSIONS		2
#define	FIELD_RESOLUTION		3
#define	FIELD_POSITION			4

/* single-item fields */
#define	FIELD_SUBFILETYPE		5
#define	FIELD_BITSPERSAMPLE		6
#define	FIELD_COMPRESSION		7
#define	FIELD_PHOTOMETRIC		8
#define	FIELD_THRESHHOLDING		9
#define	FIELD_FILLORDER			10
#define	FIELD_ORIENTATION		15
#define	FIELD_SAMPLESPERPIXEL		16
#define	FIELD_ROWSPERSTRIP		17
#define	FIELD_MINSAMPLEVALUE		18
#define	FIELD_MAXSAMPLEVALUE		19
#define	FIELD_PLANARCONFIG		20
#define	FIELD_RESOLUTIONUNIT		22
#define	FIELD_PAGENUMBER		23
#define	FIELD_STRIPBYTECOUNTS		24
#define	FIELD_STRIPOFFSETS		25
#define	FIELD_COLORMAP			26
#define	FIELD_EXTRASAMPLES		31
#define FIELD_SAMPLEFORMAT		32
#define	FIELD_SMINSAMPLEVALUE		33
#define	FIELD_SMAXSAMPLEVALUE		34
#define FIELD_IMAGEDEPTH		35
#define FIELD_TILEDEPTH			36
#define	FIELD_HALFTONEHINTS		37
#define FIELD_YCBCRSUBSAMPLING		39
#define FIELD_YCBCRPOSITIONING		40
#define	FIELD_TRANSFERFUNCTION		44
#define	FIELD_INKNAMES			46
#define	FIELD_SUBIFD			49
/*      FIELD_CUSTOM (see tiffio.h)     65 */
/* end of support for well-known tags; codec-private tags follow */
#define	FIELD_CODEC			66	/* base of codec-private tags */


/*
 * Pseudo-tags don't normally need field bits since they
 * are not written to an output file (by definition).
 * The library also has express logic to always query a
 * codec for a pseudo-tag so allocating a field bit for
 * one is a waste.   If codec wants to promote the notion
 * of a pseudo-tag being ``set'' or ``unset'' then it can
 * do using internal state flags without polluting the
 * field bit space defined for real tags.
 */
#define	FIELD_PSEUDO			0

#define	FIELD_LAST			(32*FIELD_SETLONGS-1)

#define BITn(n)				(((unsigned long)1L)<<((n)&0x1f)) 
#define BITFIELDn(tif, n)		((tif)->tif_dir.td_fieldsset[(n)/32]) 
#define TIFFFieldSet(tif, field)	(BITFIELDn(tif, field) & BITn(field)) 
#define TIFFSetFieldBit(tif, field)	(BITFIELDn(tif, field) |= BITn(field))
#define TIFFClrFieldBit(tif, field)	(BITFIELDn(tif, field) &= ~BITn(field))

#define	FieldSet(fields, f)		(fields[(f)/32] & BITn(f))
#define	ResetFieldBit(fields, f)	(fields[(f)/32] &= ~BITn(f))

#if defined(__cplusplus)
extern "C" {
#endif
extern	const TIFFFieldInfo *_TIFFGetFieldInfo(size_t *);
extern	const TIFFFieldInfo *_TIFFGetExifFieldInfo(size_t *);
extern	void _TIFFSetupFieldInfo(TIFF*, const TIFFFieldInfo[], size_t);
extern	void _TIFFPrintFieldInfo(TIFF*, FILE*);
extern	TIFFDataType _TIFFSampleToTagType(TIFF*);
extern  const TIFFFieldInfo* _TIFFFindOrRegisterFieldInfo( TIFF *tif,
							   ttag_t tag,
							   TIFFDataType dt );
extern  TIFFFieldInfo* _TIFFCreateAnonFieldInfo( TIFF *tif, ttag_t tag,
                                                 TIFFDataType dt );

#define _TIFFMergeFieldInfo	    TIFFMergeFieldInfo
#define _TIFFFindFieldInfo	    TIFFFindFieldInfo
#define _TIFFFindFieldInfoByName    TIFFFindFieldInfoByName
#define _TIFFFieldWithTag	    TIFFFieldWithTag
#define _TIFFFieldWithName	    TIFFFieldWithName

#if defined(__cplusplus)
}
#endif
#endif /* _TIFFDIR_ */

/* vim: set ts=8 sts=8 sw=8 noet: */
