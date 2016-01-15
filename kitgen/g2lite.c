
/*
  Copyright (c) 2007, Pavel Demin

  All rights reserved.

  Redistribution and use in source and binary forms,
  with or without modification, are permitted
  provided that the following conditions are met:

      * Redistributions of source code must retain
        the above copyright notice, this list of conditions
        and the following disclaimer.
      * Redistributions in binary form must reproduce
        the above copyright notice, this list of conditions
        and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the SRMlite nor the names of its
        contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <tcl.h>

#define G2_POS_MAX 512

enum
{
  G2_MODE_S    = 1,
  G2_MODE_AT   = 2,
  G2_MODE_VAR  = 4,
  G2_MODE_COPY = 8,
  G2_MODE_IGNORE_SPACES = 16
};

/* ----------------------------------------------------------------- */

typedef struct
{
  int pos;
  int level;
  unsigned char mode;
  Tcl_UniChar buffer[G2_POS_MAX + 8];
} g2state;

/* ----------------------------------------------------------------- */

static int G2liteObjCmdProc(ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[])
{
  int size = 0;
  int i = 0;
  int j = 0;
  Tcl_UniChar ch, tmp;
  Tcl_UniChar *inbuffer = NULL;
  Tcl_Obj *result = NULL;
  Tcl_Obj *command_begin = NULL;
  Tcl_Obj *command_append = NULL;
  Tcl_Obj *command_end = NULL;

  g2state g2;
  g2.pos = 0;
  g2.mode = G2_MODE_COPY;
  g2.level = 0;

  if(objc != 2)
  {
    Tcl_WrongNumArgs(interp, 1, objv, "string");
		return TCL_ERROR;
  }

  inbuffer = Tcl_GetUnicodeFromObj(objv[1], &size);

  result = Tcl_NewObj();
  command_begin = Tcl_NewStringObj("variable g2result {}\n", -1);
  command_append = Tcl_NewStringObj("\nappend g2result \"", -1);
  command_end = Tcl_NewStringObj("\nreturn $g2result", -1);

  Tcl_IncrRefCount(result);
  Tcl_IncrRefCount(command_begin);
  Tcl_IncrRefCount(command_append);
  Tcl_IncrRefCount(command_end);

  Tcl_AppendObjToObj(result, command_begin);

  while(i < size)
  {
    ch = inbuffer[i++];

    if(g2.pos >= G2_POS_MAX)
    {
      Tcl_AppendUnicodeToObj(result, g2.buffer, g2.pos);
      g2.pos = 0;
    }

    /* if current symbol is @ */
    if('@' == ch)
    {
      /* if previous symbol was @ => switch output mode */
      if(g2.mode & G2_MODE_AT)
      {
        if(g2.mode & G2_MODE_COPY)
        {
          Tcl_AppendUnicodeToObj(result, g2.buffer, g2.pos);
          Tcl_AppendObjToObj(result, command_append);
          g2.pos = 0;
        }
        else
        {
          g2.buffer[g2.pos++] = '\"';
          g2.buffer[g2.pos++] = '\n';
        }

        g2.mode ^= G2_MODE_COPY;
        g2.mode |= G2_MODE_IGNORE_SPACES;
      }

      g2.mode ^= G2_MODE_AT;
      continue;
    }

    /* current symbol is not @ */

    if(g2.mode & G2_MODE_IGNORE_SPACES)
    {
      g2.mode ^= G2_MODE_IGNORE_SPACES;

      /* if spaces => skip all spaces and new line character */
      tmp = ch;
      j = i;
      while(j < size && Tcl_UniCharIsSpace(tmp) && '\n' != tmp)
      {
        tmp = inbuffer[j++];
      }
      if('\n' == tmp)
      {
        i = j;
        continue;
      }
    }

    /* if previous symbol was single @ => output @ */
    if(g2.mode & G2_MODE_AT)
    {
      g2.mode ^= G2_MODE_AT;
      g2.buffer[g2.pos++] = '@';
    }

    /* if output mode is COPY => output current symbol */
    if(g2.mode & G2_MODE_COPY)
    {
      g2.buffer[g2.pos++] = ch;
      continue;
    }

    /* output mode is not COPY *

    /* if current symbol is $ */
    if('$' == ch)
    {
      /* if previous symbol was $ => switch to variable substitute mode*/
      if(g2.mode & G2_MODE_S)
      {
        g2.mode |= G2_MODE_VAR;
        g2.level = 0;

        g2.buffer[g2.pos++] = '$';
      }

      g2.mode ^= G2_MODE_S;
      continue;
    }

    /* current symbol is not $ */

    /* if previous symbol was single $ => output \$ */
    if(g2.mode & G2_MODE_S)
    {
      g2.buffer[g2.pos++] = '\\';
      g2.buffer[g2.pos++] = '$';
      g2.mode ^= G2_MODE_S;
    }

    /* if in variable substitute mode => output current symbol, count { and } */
    if(g2.mode & G2_MODE_VAR)
    {
      g2.buffer[g2.pos++] = ch;

      /* if current symbol is { => stay in variable substitute mode */
      if('{' == ch)
      {
        g2.level++;
      }
      else if('}' == ch)
      {
        g2.level--;
      }

      if(0 == g2.level)
      {
        g2.mode ^= G2_MODE_VAR;
      }

      continue;
    }

    /* not in variable substitute mode */

    switch(ch)
    {
      case '\\':
      case '{':
      case '}':
      case '[':
      case ']':
      case '\'':
      case '"':
        g2.buffer[g2.pos++] = '\\';
        break;
    }

    g2.buffer[g2.pos++] = ch;
  }

  /* if last symbol was single @ => output @ */
  if(g2.mode & G2_MODE_AT)
  {
    g2.buffer[g2.pos++] = '@';
  }

  /* if last symbol was single $ => output \$ */
  if(g2.mode & G2_MODE_S)
  {
    g2.buffer[g2.pos++] = '\\';
    g2.buffer[g2.pos++] = '$';
  }

  /* if output mode is not COPY => switch output mode */
  if(!(g2.mode & G2_MODE_COPY))
  {
    g2.buffer[g2.pos++] = '\"';
    g2.buffer[g2.pos++] = '\n';
  }

  if(g2.pos > 0)
  {
    Tcl_AppendUnicodeToObj(result, g2.buffer, g2.pos);
    g2.pos = 0;
  }

  Tcl_AppendObjToObj(result, command_end);

  Tcl_SetObjResult(interp, result);

  Tcl_DecrRefCount(command_end);
  Tcl_DecrRefCount(command_append);
  Tcl_DecrRefCount(command_begin);
  Tcl_DecrRefCount(result);

  return TCL_OK;
}

/* ----------------------------------------------------------------- */

int G2lite_Init(Tcl_Interp *interp)
{
    Tcl_CreateObjCommand(interp, "g2lite", G2liteObjCmdProc, 0, 0);
    return Tcl_PkgProvide(interp, "g2lite", "0.1");
}
