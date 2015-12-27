
#include <stdint.h>

#include <tcl.h>

#include <blt.h>

/* ----------------------------------------------------------------- */

static int
McphaConvertObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  unsigned char *buffer, tmp[3];
  unsigned char *data;
  int length, size, i;

  Tcl_Obj *result;

  if(objc != 2)
  {
    Tcl_WrongNumArgs(interp, 1, objv, "data");
		return TCL_ERROR;
  }

  data = Tcl_GetStringFromObj(objv[1], &size);

  buffer = ckalloc((unsigned int) (size/2));

  tmp[2] = 0;
  length = 0;

  for(i = 1; i < size; i += 2)
  {
    tmp[0] = data[i - 1];
    tmp[1] = data[i];
  	buffer[length++] = strtoul(tmp, 0, 16);
  }

  result = Tcl_NewByteArrayObj(buffer, length);

  ckfree(buffer);

  Tcl_SetObjResult(interp, result);

  return TCL_OK;
}

/* ----------------------------------------------------------------- */

static int
McphaConvertBltObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  unsigned char *data;
  int size, width, length, i, j, pos, value;

  char *name;
  Blt_Vector *vec;

  if(objc != 4)
  {
    Tcl_WrongNumArgs(interp, 1, objv, "data width vector");
		return TCL_ERROR;
  }

  data = Tcl_GetByteArrayFromObj(objv[1], &size);

  if(TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &width))
  {
    Tcl_AppendResult(interp, "Parameter width is not an integer", NULL);
    return TCL_ERROR;
  }

  length = size / width;

  name =  Tcl_GetString(objv[3]);
  if(TCL_OK != Blt_GetVector(interp, name, &vec))
  {
    Tcl_AppendResult(interp, "Cannot find BLT vector", name, NULL);
    return TCL_ERROR;
  }

  if(Blt_VecSize(vec) < length)
  {
    Tcl_AppendResult(interp, "BLT vector size is less than the data length", NULL);
    return TCL_ERROR;
  }

  for(i = 0; i < length; ++i)
  {
    value = 0;
    for(j = 1; j <= width; ++j)
    {
      pos = i*width + width - j;
      value = (value << 8) | data[pos];
    }
    Blt_VecData(vec)[i] = (double)value;
  }

  Blt_ResetVector(vec, Blt_VecData(vec), length, Blt_VecSize(vec), TCL_STATIC);

  return TCL_OK;
}

/* ----------------------------------------------------------------- */

static int
McphaConvertOscObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int16_t *data;
  int size, length, i, j, pos, value;

  Tcl_DictSearch search;
  Tcl_Obj *key, *object;
  int done;

  char *name;
  Blt_Vector *vec[2];

  if(objc != 3)
  {
    Tcl_WrongNumArgs(interp, 1, objv, "data dict");
		return TCL_ERROR;
  }

  data = (int16_t *)Tcl_GetByteArrayFromObj(objv[1], &size);
  length = size / 4;

  if(TCL_OK != Tcl_DictObjFirst(interp, objv[2], &search, &key, &object, &done))
  {
    Tcl_AppendResult(interp, "Cannot read dict variable", name, NULL);
    return TCL_ERROR;
  }

  for(i = 0; (i < 2) && (!done); ++i, Tcl_DictObjNext(&search, &key, &object, &done))
  {
    name =  Tcl_GetString(object);
    if(TCL_OK != Blt_GetVector(interp, name, &vec[i]))
    {
      Tcl_DictObjDone(&search);
      Tcl_AppendResult(interp, "Cannot find BLT vector", name, NULL);
      return TCL_ERROR;
    }
    if(Blt_VecSize(vec[i]) < length)
    {
      Tcl_DictObjDone(&search);
      Tcl_AppendResult(interp, "BLT vector size is less than the data size", NULL);
      return TCL_ERROR;
    }
  }
  Tcl_DictObjDone(&search);

  for(i = 0; i < length; ++i)
  {
    Blt_VecData(vec[0])[i] = (double)data[2*i + 0];
    Blt_VecData(vec[1])[i] = (double)data[2*i + 1];
  }

  for(i = 0; i < 2; ++i)
  {
    Blt_ResetVector(vec[i], Blt_VecData(vec[i]), length, Blt_VecSize(vec[i]), TCL_STATIC);
  }

  return TCL_OK;
}

/* ----------------------------------------------------------------- */

static int
McphaFormatOscCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *name;
  int first, last, done, max, i, j;

  Tcl_DictSearch search;
  Tcl_Obj *key, *object, *result, *eol;

  Blt_Vector *vec[32];

  if(objc != 4)
  {
    Tcl_WrongNumArgs(interp, 1, objv, "dict first last");
		return TCL_ERROR;
  }

  if(TCL_OK != Tcl_DictObjFirst(interp, objv[1], &search, &key, &object, &done))
  {
    Tcl_AppendResult(interp, "Cannot read dict variable", name, NULL);
    return TCL_ERROR;
  }

  if(TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &first))
  {
    Tcl_AppendResult(interp, "Parameter first is not an integer", NULL);
    return TCL_ERROR;
  }

  if(TCL_OK != Tcl_GetIntFromObj(interp, objv[3], &last))
  {
    Tcl_AppendResult(interp, "Parameter last is not an integer", NULL);
    return TCL_ERROR;
  }

  if(last < first)
  {
    return TCL_OK;
  }

  for(i = 0; (i < 32) && (!done); ++i, Tcl_DictObjNext(&search, &key, &object, &done))
  {
    name =  Tcl_GetString(object);
    if(TCL_OK != Blt_GetVector(interp, name, &vec[i]))
    {
      Tcl_DictObjDone(&search);
      Tcl_AppendResult(interp, "Cannot find BLT vector", name, NULL);
      return TCL_ERROR;
    }
    if(Blt_VecLength(vec[i]) <= last)
    {
      last = Blt_VecLength(vec[i]) - 1;
    }
    max = i;
  }
  Tcl_DictObjDone(&search);

  if(first < 0)
  {
    first = 0;
  }

  result = Tcl_NewObj();
  eol = Tcl_NewStringObj("\n", -1);

  Tcl_IncrRefCount(result);
  Tcl_IncrRefCount(eol);

  for(i = first; i <= last; ++i)
  {
    for(j = 0; j <= max; ++j)
    {
      if(j > 0)
      {
        Tcl_AppendPrintfToObj(result, "\t%g", Blt_VecData(vec[j])[i]);
      }
      else
      {
        Tcl_AppendPrintfToObj(result, "%g", Blt_VecData(vec[j])[i]);
      }
    }
    Tcl_AppendObjToObj(result, eol);
  }

  Tcl_SetObjResult(interp, result);

  Tcl_DecrRefCount(eol);
  Tcl_DecrRefCount(result);

  return TCL_OK;
}

/* ----------------------------------------------------------------- */

static int
McphaIntegrateBltObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int length, i, xmin, xmax, flag;
  double entr, mean;

  char *name;
  Blt_Vector *vec;

  Tcl_Obj *result;

  if(objc != 5)
  {
    Tcl_WrongNumArgs(interp, 1, objv, "vector xmin xmax flag");
		return TCL_ERROR;
  }

  name =  Tcl_GetString(objv[1]);
  if(TCL_OK != Blt_GetVector(interp, name, &vec))
  {
    Tcl_AppendResult(interp, "Cannot find BLT vector", name, NULL);
    return TCL_ERROR;
  }

  if(TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &xmin))
  {
    Tcl_AppendResult(interp, "Parameter xmin is not an integer", NULL);
    return TCL_ERROR;
  }

  if(TCL_OK != Tcl_GetIntFromObj(interp, objv[3], &xmax))
  {
    Tcl_AppendResult(interp, "Parameter xmax is not an integer", NULL);
    return TCL_ERROR;
  }

  if(TCL_OK != Tcl_GetIntFromObj(interp, objv[4], &flag))
  {
    Tcl_AppendResult(interp, "Parameter flag is not an integer", NULL);
    return TCL_ERROR;
  }

  length = Blt_VecLength(vec);
  entr = 0.0;
  mean = 0.0;

  if(xmin < 0) xmin = 0;
  if(xmax >= length) xmax = length - 1;
  if(xmax < xmin) xmax = xmin;

  for(i = xmin; i <= xmax; ++i)
  {
    entr += Blt_VecData(vec)[i];
    mean += i * Blt_VecData(vec)[i];
  }

  if(flag)
  {
    result = Tcl_NewDoubleObj(entr > 0.0 ? mean / entr : 0.0);
  }
  else
  {
    result = Tcl_NewDoubleObj(entr);
  }

  Tcl_SetObjResult(interp, result);

  return TCL_OK;
}

/* ----------------------------------------------------------------- */

int
Mcpha_Init(Tcl_Interp *interp)
{
  Tcl_CreateObjCommand(interp, "mcpha::convert", McphaConvertObjCmd,
    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateObjCommand(interp, "mcpha::convertBlt", McphaConvertBltObjCmd,
    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateObjCommand(interp, "mcpha::convertOsc", McphaConvertOscObjCmd,
    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateObjCommand(interp, "mcpha::formatOsc", McphaFormatOscCmd,
    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateObjCommand(interp, "mcpha::integrateBlt", McphaIntegrateBltObjCmd,
    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  return Tcl_PkgProvide(interp, "mcpha", "0.1");
}
