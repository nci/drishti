#ifndef _NETCDF3_CONV
#define _NETCDF3_CONV_

#include <stddef.h> /* size_t, ptrdiff_t */
#include <errno.h>  /* netcdf functions sometimes return system errors */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The Interface
 */
#define nc_inq_format lnc3_inq_format
#define nc_inq_libvers lnc3_inq_libvers
#define nc_strerror lnc3_strerror
#define nc__create lnc3__create
#define nc_create lnc3_create
#define nc__open lnc3__open
#define nc_open lnc3_open
#define nc_set_fill lnc3_set_fill
#define nc_redef lnc3_redef
#define nc__enddef lnc3__enddef
#define nc_enddef lnc3_enddef
#define nc_sync lnc3_sync
#define nc_abort lnc3_abort
#define nc_close lnc3_close
#define nc_inq lnc3_inq
#define nc_inq_ndims lnc3_inq_ndims
#define nc_inq_nvars lnc3_inq_nvars
#define nc_inq_natts lnc3_inq_natts
#define nc_inq_unlimdim lnc3_inq_unlimdim
#define nc_inq_format lnc3_inq_format


/* Begin _dim */

#define nc_def_dim lnc3_def_dim
#define nc_inq_dimid lnc3_inq_dimid
#define nc_inq_dim lnc3_inq_dim
#define nc_inq_dimname lnc3_inq_dimname
#define nc_inq_dimlen lnc3_inq_dimlen
#define nc_rename_dim lnc3_rename_dim
/* End _dim */
/* Begin _att */
#define nc_inq_att lnc3_inq_att
#define nc_inq_attid lnc3_inq_attid
#define nc_inq_atttype lnc3_inq_atttype
#define nc_inq_attlen lnc3_inq_attlen
#define nc_inq_attname lnc3_inq_attname
#define nc_copy_att lnc3_copy_att
#define nc_rename_att lnc3_rename_att
#define nc_del_att lnc3_del_att
/* End _att */
/* Begin {put,get}_att */
#define nc_put_att lnc3_put_att
#define nc_get_att lnc3_get_att
#define nc_put_att_text lnc3_put_att_text
#define nc_get_att_text lnc3_get_att_text
#define nc_put_att_uchar lnc3_put_att_uchar
#define nc_get_att_uchar lnc3_get_att_uchar
#define nc_put_att_schar lnc3_put_att_schar
#define nc_get_att_schar lnc3_get_att_schar
#define nc_put_att_short lnc3_put_att_short
#define nc_get_att_short lnc3_get_att_short
#define nc_put_att_int lnc3_put_att_int
#define nc_get_att_int lnc3_get_att_int
#define nc_put_att_long lnc3_put_att_long
#define nc_get_att_long lnc3_get_att_long
#define nc_put_att_float lnc3_put_att_float
#define nc_get_att_float lnc3_get_att_float
#define nc_put_att_double lnc3_put_att_double
#define nc_get_att_double lnc3_get_att_double
  /* End {put,get}_att */
/* Begin _var */
#define nc_def_var lnc3_def_var
#define nc_inq_var lnc3_inq_var
#define nc_inq_varid lnc3_inq_varid
#define nc_inq_varname lnc3_inq_varname
#define nc_inq_vartype lnc3_inq_vartype
#define nc_inq_varndims lnc3_inq_varndims
#define nc_inq_vardimid lnc3_inq_vardimid
#define nc_inq_varnatts lnc3_inq_varnatts
#define nc_rename_var lnc3_rename_var
#define nc_copy_var lnc3_copy_var

/* support the old name for now */
#define ncvarcpy(ncid_in, varid, ncid_out) ncvarcopy((ncid_in), (varid), (ncid_out))

/* End _var */
/* Begin {put,get}_var1 */
#define nc_put_var1 lnc3_put_var1
#define nc_get_var1 lnc3_get_var1
#define nc_put_var1_text lnc3_put_var1_text
#define nc_get_var1_text lnc3_get_var1_text
#define nc_put_var1_uchar lnc3_put_var1_uchar
#define nc_get_var1_uchar lnc3_get_var1_uchar
#define nc_put_var1_schar lnc3_put_var1_schar
#define nc_get_var1_schar lnc3_get_var1_schar
#define nc_put_var1_short lnc3_put_var1_short
#define nc_get_var1_short lnc3_get_var1_short
#define nc_put_var1_int lnc3_put_var1_int
#define nc_get_var1_int lnc3_get_var1_int
#define nc_put_var1_long lnc3_put_var1_long
#define nc_get_var1_long lnc3_get_var1_long
#define nc_put_var1_float lnc3_put_var1_float
#define nc_get_var1_float lnc3_get_var1_float
#define nc_put_var1_double lnc3_put_var1_double
#define nc_get_var1_double lnc3_get_var1_double
/* End {put,get}_var1 */
/* Begin {put,get}_vara */
#define nc_put_vara lnc3_put_vara
#define nc_get_vara lnc3_get_vara
#define nc_put_vara_text lnc3_put_vara_text
#define nc_put_vara_text lnc3_put_vara_text
#define nc_get_vara_text lnc3_get_vara_text
#define nc_put_vara_uchar lnc3_put_vara_uchar
#define nc_get_vara_uchar lnc3_get_vara_uchar
#define nc_put_vara_schar lnc3_put_vara_schar
#define nc_get_vara_schar lnc3_get_vara_schar
#define nc_put_vara_short lnc3_put_vara_short
#define nc_get_vara_short lnc3_get_vara_short
#define nc_put_vara_int lnc3_put_vara_int
#define nc_get_vara_int lnc3_get_vara_int
#define nc_put_vara_long lnc3_put_vara_long
#define nc_get_vara_long lnc3_get_vara_long
#define nc_put_vara_float lnc3_put_vara_float
#define nc_get_vara_float lnc3_get_vara_float
#define nc_put_vara_double lnc3_put_vara_double
#define nc_get_vara_double lnc3_get_vara_double

/* End {put,get}_vara */
/* Begin {put,get}_vars */

#define nc_put_vars lnc3_put_vars
#define nc_get_vars lnc3_get_vars
#define nc_put_vars_text lnc3_put_vars_text
#define nc_get_vars_text lnc3_get_vars_text
#define nc_put_vars_uchar lnc3_put_vars_uchar
#define nc_get_vars_uchar lnc3_get_vars_uchar
#define nc_put_vars_schar lnc3_put_vars_schar
#define nc_get_vars_schar lnc3_get_vars_schar
#define nc_put_vars_short lnc3_put_vars_short
#define nc_get_vars_short lnc3_get_vars_short
#define nc_put_vars_int lnc3_put_vars_int
#define nc_get_vars_int lnc3_get_vars_int
#define nc_put_vars_long lnc3_put_vars_long
#define nc_get_vars_long lnc3_get_vars_long
#define nc_put_vars_float lnc3_put_vars_float
#define nc_get_vars_float lnc3_get_vars_float
#define nc_put_vars_double lnc3_put_vars_double
#define nc_get_vars_double lnc3_get_vars_double

/* End {put,get}_vars */
/* Begin {put,get}_varm */
#define nc_put_varm lnc3_put_varm
#define nc_get_varm lnc3_get_varm
#define nc_put_varm_text lnc3_put_varm_text
#define nc_get_varm_text lnc3_get_varm_text
#define nc_put_varm_uchar lnc3_put_varm_uchar
#define nc_get_varm_uchar lnc3_get_varm_uchar
#define nc_put_varm_schar lnc3_put_varm_schar
#define nc_get_varm_schar lnc3_get_varm_schar
#define nc_put_varm_short lnc3_put_varm_short
#define nc_get_varm_short lnc3_get_varm_short
#define nc_put_varm_int lnc3_put_varm_int
#define nc_get_varm_int lnc3_get_varm_int
#define nc_put_varm_long lnc3_put_varm_long
#define nc_get_varm_long lnc3_get_varm_long
#define nc_put_varm_float lnc3_put_varm_float
#define nc_get_varm_float lnc3_get_varm_float
#define nc_put_varm_double lnc3_put_varm_double
#define nc_get_varm_double lnc3_get_varm_double

/* End {put,get}_varm */
/* Begin {put,get}_var */

#define nc_put_var_text lnc3_put_var_text
#define nc_get_var_text lnc3_get_var_text
#define nc_put_var_uchar lnc3_put_var_uchar
#define nc_get_var_uchar lnc3_get_var_uchar
#define nc_put_var_schar lnc3_put_var_schar
#define nc_get_var_schar lnc3_get_var_schar
#define nc_put_var_short lnc3_put_var_short
#define nc_get_var_short lnc3_get_var_short
#define nc_put_var_int lnc3_put_var_int
#define nc_get_var_int lnc3_get_var_int
#define nc_put_var_long lnc3_put_var_long
#define nc_get_var_long lnc3_get_var_long
#define nc_put_var_float lnc3_put_var_float
#define nc_get_var_float lnc3_get_var_float
#define nc_put_var_double lnc3_put_var_double
#define nc_get_var_double lnc3_get_var_double
/* End {put,get}_var */

#define nc_put_att lnc3_put_att
#define nc_get_att lnc3_get_att

/* #ifdef _CRAYMPP */
/*
 * Public interfaces to better support
 * CRAY multi-processor systems like T3E.
 * A tip of the hat to NERSC.
 */
/*
 * It turns out we need to declare and define
 * these public interfaces on all platforms
 * or things get ugly working out the
 * FORTRAN interface. On !_CRAYMPP platforms,
 * these functions work as advertised, but you
 * can only use "processor element" 0.
 */

#define nc__create_mp lnc3__create_mp


#define nc__open_mp lnc3__open_mp


#define nc_delete_mp lnc3_delete_mp
#define nc_set_base_pe lnc3_set_base_pe
#define nc_inq_base_pe lnc3_inq_base_pe
/* #endif _CRAYMPP */

#if defined(__cplusplus)
}
#endif

#endif /* _NETCDF_ */
