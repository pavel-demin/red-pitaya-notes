/* Written by Jean-Claude Wippler, as part of Tclkit.
 * March 2003 - placed in the public domain by the author.
 *
 * Interface to the "zlib" compression library
 */

#include "zlib.h"
#include <tcl.h>

typedef struct {
  z_stream stream;
  Tcl_Obj *indata;
} zlibstream;

static int
zstreamincmd(ClientData cd, Tcl_Interp *ip, int objc, Tcl_Obj *CONST objv[])
{
  zlibstream *zp = (zlibstream*) cd;
  int count = 0;
  int e, index;
  Tcl_Obj *obj;

  static CONST84 char* cmds[] = { "fill", "drain", NULL, };

  if (objc < 2 || objc > 3) {
    Tcl_WrongNumArgs(ip, 2, objv, "fill|drain data");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(ip, objv[1], cmds, "option", 0, &index) != TCL_OK)
    return TCL_ERROR;

  switch (index) {

    case 0: /* fill ?data? */
      if (objc >= 3) {
      	Tcl_IncrRefCount(objv[2]);
      	Tcl_DecrRefCount(zp->indata);
      	zp->indata = objv[2];
      	zp->stream.next_in = Tcl_GetByteArrayFromObj(zp->indata,
						  (int*) &zp->stream.avail_in);
      }
      Tcl_SetObjResult(ip, Tcl_NewIntObj(zp->stream.avail_in));
      break;

    case 1: /* drain count */
      if (objc != 3) {
      	Tcl_WrongNumArgs(ip, 2, objv, "count");
      	return TCL_ERROR;
      }
      if (Tcl_GetIntFromObj(ip, objv[2], &count) != TCL_OK)
      	return TCL_ERROR;
      obj = Tcl_GetObjResult(ip);
      Tcl_SetByteArrayLength(obj, count);
      zp->stream.next_out = Tcl_GetByteArrayFromObj(obj,
						  (int*) &zp->stream.avail_out);
      e = inflate(&zp->stream, Z_NO_FLUSH);
      if (e != 0 && e != Z_STREAM_END) {
      	Tcl_SetResult(ip, (char*) zError(e), TCL_STATIC);
      	return TCL_ERROR;
      }
      Tcl_SetByteArrayLength(obj, count - zp->stream.avail_out);
      break;
  }
  return TCL_OK;
}

void zstreamdelproc(ClientData cd)
{
  zlibstream *zp = (zlibstream*) cd;
  inflateEnd(&zp->stream);
  Tcl_DecrRefCount(zp->indata);
  Tcl_Free((void*) zp);
}

static int
ZlibCmd(ClientData dummy, Tcl_Interp *ip, int objc, Tcl_Obj *CONST objv[])
{
  int e = TCL_OK, index, dlen, wbits = -MAX_WBITS;
  long flag;
  Byte *data;
  z_stream stream;
  Tcl_Obj *obj = Tcl_GetObjResult(ip);

  static CONST84 char* cmds[] = {
    "adler32", "crc32", "compress", "deflate", "decompress", "inflate", 
    "sdecompress", "sinflate", NULL,
  };

  if (objc < 3 || objc > 4) {
    Tcl_WrongNumArgs(ip, 1, objv, "option data ?...?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(ip, objv[1], cmds, "option", 0, &index) != TCL_OK ||
      objc > 3 && Tcl_GetLongFromObj(ip, objv[3], &flag) != TCL_OK)
    return TCL_ERROR;

  data = Tcl_GetByteArrayFromObj(objv[2], &dlen);

  switch (index) {

    case 0: /* adler32 str ?start? -> checksum */
      if (objc < 4)
      	flag = (long) adler32(0, 0, 0);
      Tcl_SetLongObj(obj, (long) adler32((uLong) flag, data, dlen));
      return TCL_OK;

    case 1: /* crc32 str ?start? -> checksum */
      if (objc < 4)
      	flag = (long) crc32(0, 0, 0);
      Tcl_SetLongObj(obj, (long) crc32((uLong) flag, data, dlen));
      return TCL_OK;
      
    case 2: /* compress data ?level? -> data */
      wbits = MAX_WBITS;
    case 3: /* deflate data ?level? -> data */
      if (objc < 4)
      	flag = Z_DEFAULT_COMPRESSION;

      stream.avail_in = (uInt) dlen;
      stream.next_in = data;

      stream.avail_out = (uInt) dlen + dlen / 1000 + 12;
      Tcl_SetByteArrayLength(obj, stream.avail_out);
      stream.next_out = Tcl_GetByteArrayFromObj(obj, NULL);

      stream.zalloc = 0;
      stream.zfree = 0;
      stream.opaque = 0;

      e = deflateInit2(&stream, (int) flag, Z_DEFLATED, wbits,
			      MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
      if (e != Z_OK)
      	break;

      e = deflate(&stream, Z_FINISH);
      if (e != Z_STREAM_END) {
      	deflateEnd(&stream);
      	if (e == Z_OK) e = Z_BUF_ERROR;
      } else
      	e = deflateEnd(&stream);
      break;
      
    case 4: /* decompress data ?bufsize? -> data */
      wbits = MAX_WBITS;
    case 5: /* inflate data ?bufsize? -> data */
    {
      if (flag < 1) {
        Tcl_SetResult(ip, "invalid buffer size", TCL_STATIC);
        return TCL_ERROR;
      }

      if (objc < 4)
      	flag = 16 * 1024;

      for (;;) {
      	stream.zalloc = 0;
      	stream.zfree = 0;

      	/* +1 because ZLIB can "over-request" input (but ignore it) */
      	stream.avail_in = (uInt) dlen +  1;
      	stream.next_in = data;

      	stream.avail_out = (uInt) flag;
      	Tcl_SetByteArrayLength(obj, stream.avail_out);
      	stream.next_out = Tcl_GetByteArrayFromObj(obj, NULL);

      	/* Negative value suppresses ZLIB header */
      	e = inflateInit2(&stream, wbits);
      	if (e == Z_OK) {
      	  e = inflate(&stream, Z_FINISH);
      	  if (e != Z_STREAM_END) {
      	    inflateEnd(&stream);
      	    if (e == Z_OK) e = Z_BUF_ERROR;
      	  } else
      	    e = inflateEnd(&stream);
      	}

      	if (e == Z_OK || e != Z_BUF_ERROR) break;

      	Tcl_SetByteArrayLength(obj, 0);
      	flag *= 2;
      }

      break;
    }
      
    case 6: /* sdecompress cmdname -> */
      wbits = MAX_WBITS;
    case 7: /* sinflate cmdname -> */
    {
      zlibstream *zp = (zlibstream*) Tcl_Alloc(sizeof (zlibstream));
      zp->indata = Tcl_NewObj();
      Tcl_IncrRefCount(zp->indata);
      zp->stream.zalloc = 0;
      zp->stream.zfree = 0;
      zp->stream.opaque = 0;
      zp->stream.next_in = 0;
      zp->stream.avail_in = 0;
      inflateInit2(&zp->stream, wbits);
      Tcl_CreateObjCommand(ip, Tcl_GetStringFromObj(objv[2], 0), zstreamincmd,
      				(ClientData) zp, zstreamdelproc);
      return TCL_OK;
    }
  }

  if (e != Z_OK) {
    Tcl_SetResult(ip, (char*) zError(e), TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_SetByteArrayLength(obj, stream.total_out);
  return TCL_OK;
}

int Zlib_Init(Tcl_Interp *interp)
{
    Tcl_CreateObjCommand(interp, "zlib", ZlibCmd, 0, 0);
    return Tcl_PkgProvide( interp, "zlib", "1.1");
}
