/*
** By the overt act of typing this comment, the author of this code
** releases it into the public domain.  No claim of copyright is made.
** In place of a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
***************************************************************************
** A ZIP archive virtual filesystem for Tcl.
**
** This package of routines enables Tcl to use a Zip file as
** a virtual file system.  Each of the content files of the Zip
** archive appears as a real file to Tcl.
**
** Converted to Tcl VFS by Peter MacDonald
**   peter@pdqi.com
**   http://pdqi.com
**
**
** Modified by Damon Courtney to complete the VFS work.
**
** @(#) $Id: zvfs.c,v 1.1.1.1 2002/01/27 17:44:02 cvs Exp $
*/

#include "tclInt.h"
#include "tclPort.h"
#include <zlib.h>

/*
 * Size of the decompression input buffer
 */
#define COMPR_BUF_SIZE   8192

#ifdef __WIN32__
#define NOCASE_PATHS 1
#else
#define NOCASE_PATHS 0
#endif

/*
 * All static variables are collected into a structure named "local".
 * That way, it is clear in the code when we are using a static
 * variable because its name begins with "local.".
 */
static struct {
    Tcl_HashTable fileHash;     /* One entry for each file in the ZVFS.  The
                                 * The key is the virtual filename.  The data
                                 * an an instance of the ZvfsFile structure.
                                 */
    Tcl_HashTable archiveHash;  /* One entry for each archive.
                                 * Key is the name.
                                 * Data is the ZvfsArchive structure.
                                 */
    int isInit;                 /* True after initialization */
} local;

/*
 * Each ZIP archive file that is mounted is recorded as an instance
 * of this structure
 */
typedef struct ZvfsArchive {
    int refCount;
    Tcl_Obj *zName;         /* Name of the archive */
    Tcl_Obj *zMountPoint;   /* Where this archive is mounted */
} ZvfsArchive;

/*
 * Particulars about each virtual file are recorded in an instance
 * of the following structure.
 */
typedef struct ZvfsFile {
    int refCount;             /* Reference count */
    Tcl_Obj *zName;           /* The full pathname of the virtual file */
    ZvfsArchive *pArchive;    /* The ZIP archive holding this file data */
    int iOffset;              /* Offset into the ZIP archive of the data */
    int nByte;                /* Uncompressed size of the virtual file */
    int nByteCompr;           /* Compressed size of the virtual file */
    int isdir;		      /* Set to 1 if directory */
    int timestamp;            /* Modification time */
    int iCRC;                 /* Cyclic Redundancy Check of the data */
    struct ZvfsFile *parent;  /* Parent directory. */
    Tcl_HashTable children;   /* For directory entries, a hash table of
                               * all of the files in the directory.
                               */
} ZvfsFile;

/*
 * Whenever a ZVFS file is opened, an instance of this structure is
 * attached to the open channel where it will be available to the
 * ZVFS I/O routines below.  All state information about an open
 * ZVFS file is held in this structure.
 */
typedef struct ZvfsChannelInfo {
    unsigned int nByte;       /* number of bytes of read uncompressed data */
    unsigned int nByteCompr;  /* number of bytes of unread compressed data */
    unsigned int nData;       /* total number of bytes of compressed data */
    int readSoFar;            /* Number of bytes read so far */
    long startOfData;         /* File position of data in ZIP archive */
    int isCompressed;         /* True data is compressed */
    Tcl_Channel chan;         /* Open to the archive file */
    unsigned char *zBuf;      /* buffer used by the decompressor */
    z_stream stream;          /* state of the decompressor */
} ZvfsChannelInfo;

/* The attributes defined for each file in the archive.
 * These are accessed via the 'file attributes' command in Tcl.
 */
static CONST char *ZvfsAttrs[] = {
    "-archive", "-compressedsize", "-crc", "-mount", "-offset",
    "-uncompressedsize", (char *)NULL
};

enum {
    ZVFS_ATTR_ARCHIVE, ZVFS_ATTR_COMPSIZE, ZVFS_ATTR_CRC,
    ZVFS_ATTR_MOUNT, ZVFS_ATTR_OFFSET, ZVFS_ATTR_UNCOMPSIZE
};

/* Forward declarations for the callbacks to the Tcl filesystem. */

static Tcl_FSPathInFilesystemProc       PathInFilesystem;
static Tcl_FSDupInternalRepProc		DupInternalRep;
static Tcl_FSFreeInternalRepProc	FreeInternalRep;
static Tcl_FSInternalToNormalizedProc	InternalToNormalized;
static Tcl_FSFilesystemPathTypeProc     FilesystemPathType;
static Tcl_FSFilesystemSeparatorProc    FilesystemSeparator;
static Tcl_FSStatProc                   Stat;
static Tcl_FSAccessProc                 Access;
static Tcl_FSOpenFileChannelProc        OpenFileChannel;
static Tcl_FSMatchInDirectoryProc       MatchInDirectory;
static Tcl_FSListVolumesProc            ListVolumes;
static Tcl_FSFileAttrStringsProc        FileAttrStrings;
static Tcl_FSFileAttrsGetProc           FileAttrsGet;
static Tcl_FSFileAttrsSetProc           FileAttrsSet;
static Tcl_FSChdirProc                  Chdir;

static Tcl_Filesystem zvfsFilesystem = {
    "zvfs",
    sizeof(Tcl_Filesystem),
    TCL_FILESYSTEM_VERSION_1,
    &PathInFilesystem,
    &DupInternalRep,
    &FreeInternalRep,
    &InternalToNormalized,
    NULL,			/* &CreateInternalRep, */
    NULL,                       /* &NormalizePath, */
    &FilesystemPathType,
    &FilesystemSeparator,
    &Stat,
    &Access,
    &OpenFileChannel,
    &MatchInDirectory,
    NULL,                       /* &Utime, */
    NULL,                       /* &Link, */
    &ListVolumes,
    &FileAttrStrings,
    &FileAttrsGet,
    &FileAttrsSet,
    NULL,			/* &CreateDirectory, */
    NULL,			/* &RemoveDirectory, */
    NULL,			/* &DeleteFile, */
    NULL,			/* &CopyFile, */
    NULL,			/* &RenameFile, */
    NULL,			/* &CopyDirectory, */
    NULL,                       /* &Lstat, */
    NULL,			/* &LoadFile, */
    NULL,			/* &GetCwd, */
    &Chdir
};


/*
 * Forward declarations describing the channel type structure for
 * opening and reading files inside of an archive.
 */
static Tcl_DriverCloseProc        DriverClose;
static Tcl_DriverInputProc        DriverInput;
static Tcl_DriverOutputProc       DriverOutput;
static Tcl_DriverSeekProc         DriverSeek;
static Tcl_DriverWatchProc        DriverWatch;
static Tcl_DriverGetHandleProc    DriverGetHandle;

static Tcl_ChannelType vfsChannelType = {
    "zvfs",                /* Type name.                                    */
    TCL_CHANNEL_VERSION_2, /* Set blocking/nonblocking behaviour. NULL'able */
    DriverClose,           /* Close channel, clean instance data            */
    DriverInput,           /* Handle read request                           */
    DriverOutput,          /* Handle write request                          */
    DriverSeek,            /* Move location of access point.      NULL'able */
    NULL,                  /* Set options.                        NULL'able */
    NULL,                  /* Get options.                        NULL'able */
    DriverWatch,           /* Initialize notifier                           */
    DriverGetHandle        /* Get OS handle from the channel.               */
};

/*
 * Macros to read 16-bit and 32-bit big-endian integers into the
 * native format of this local processor.  B is an array of
 * characters and the integer begins at the N-th character of
 * the array.
 */
#define INT16(B, N) (B[N] + (B[N+1]<<8))
#define INT32(B, N) (INT16(B,N) + (B[N+2]<<16) + (B[N+3]<<24))


/*
 *----------------------------------------------------------------------
 *
 * DosTimeDate --
 *
 * 	Convert DOS date and time from a zip archive into clock seconds.
 *
 * Results:
 * 	Clock seconds
 *
 *----------------------------------------------------------------------
 */

static time_t
DosTimeDate( int dosDate, int dosTime )
{
    time_t now;
    struct tm *tm;
    now=time(NULL);
    tm = localtime(&now);
    tm->tm_year=(((dosDate&0xfe00)>>9) + 80);
    tm->tm_mon=((dosDate&0x1e0)>>5)-1;
    tm->tm_mday=(dosDate & 0x1f);
    tm->tm_hour=(dosTime&0xf800)>>11;
    tm->tm_min=(dosTime&0x7e)>>5;
    tm->tm_sec=(dosTime&0x1f);
    return mktime(tm);
}

/*
 *----------------------------------------------------------------------
 *
 * StrDup --
 *
 * 	Create a copy of the given string and lower it if necessary.
 *
 * Results:
 * 	Pointer to the new string.  Space to hold the returned
 * 	string is obtained from Tcl_Alloc() and should be freed
 * 	by the calling function.
 *
 *----------------------------------------------------------------------
 */

char *
StrDup( char *str, int lower )
{
    int i, c, len;
    char *newstr;

    len = strlen(str);

    newstr = Tcl_Alloc( len + 1 );
    memcpy( newstr, str, len );
    newstr[len] = '\0';

    if( lower ) {
        for( i = 0; (c = newstr[i]) != 0; ++i )
        {
            if( isupper(c) ) {
                newstr[i] = tolower(c);
            }
        }
    }

    return newstr;
}

/*
 *----------------------------------------------------------------------
 *
 * CanonicalPath --
 *
 * 	Concatenate zTail onto zRoot to form a pathname.  After
 * 	concatenation, simplify the pathname by removing ".." and
 * 	"." directories.
 *
 * Results:
 * 	Pointer to the new pathname.  Space to hold the returned
 * 	path is obtained from Tcl_Alloc() and should be freed by
 * 	the calling function.
 *
 *----------------------------------------------------------------------
 */

static char *
CanonicalPath( const char *zRoot, const char *zTail )
{
    char *zPath;
    int i, j, c;
    int len = strlen(zRoot) + strlen(zTail) + 2;

#ifdef __WIN32__
    if( isalpha(zTail[0]) && zTail[1]==':' ){ zTail += 2; }
    if( zTail[0]=='\\' ){ zRoot = ""; zTail++; }
    if( zTail[0]=='\\' ){ zRoot = "/"; zTail++; } // account for UNC style path
#endif
    if( zTail[0]=='/' ){ zRoot = ""; zTail++; }
    if( zTail[0]=='/' ){ zRoot = "/"; zTail++; }  // account for UNC style path

    zPath = Tcl_Alloc( len );
    if( !zPath ) return NULL;

    sprintf( zPath, "%s/%s", zRoot, zTail );
    for( i = j = 0; (c = zPath[i]) != 0; i++ )
    {
#ifdef __WIN32__
        if( c == '\\' ) {
            c = '/';
        }
#endif
        if( c == '/' ) {
            int c2 = zPath[i+1];
            if( c2 == '/' ) continue;
            if( c2 == '.' ) {
                int c3 = zPath[i+2];
                if( c3 == '/' || c3 == 0 ) {
                    i++;
                    continue;
                }
                if( c3 == '.' && (zPath[i+3] == '.' || zPath[i+3] == 0) ) {
                    i += 2;
                    while( j > 0 && zPath[j-1] != '/' ) { j--; }
                    continue;
                }
            }
        }
        zPath[j++] = c;
    }

    if( j == 0 ) {
        zPath[j++] = '/';
    }

    zPath[j] = 0;

    return zPath;
}

/*
 *----------------------------------------------------------------------
 *
 * AbsolutePath --
 *
 * 	Construct an absolute pathname from the given pathname.  On
 * 	Windows, all backslash (\) characters are converted to
 * 	forward slash (/), and if NOCASE_PATHS is true, all letters
 * 	are converted to lowercase.  The drive letter, if present, is
 * 	preserved.
 *
 * Results:
 * 	Pointer to the new pathname.  Space to hold the returned
 * 	path is obtained from Tcl_Alloc() and should be freed by
 * 	the calling function.
 *
 *----------------------------------------------------------------------
 */

static char *
AbsolutePath( const char *z )
{
    int len;
    char *zResult;

    if( *z != '/'
#ifdef __WIN32__
        && *z != '\\' && (!isalpha(*z) || z[1] != ':' )
#endif
    ) {
        /* Case 1:  "z" is a relative path, so prepend the current
         * working directory in order to generate an absolute path.
         */
        Tcl_Obj *pwd = Tcl_FSGetCwd(NULL);
        zResult = CanonicalPath( Tcl_GetString(pwd), z );
        Tcl_DecrRefCount(pwd);
    } else {
        /* Case 2:  "z" is an absolute path already, so we
         * just need to make a copy of it.
         */
        zResult = StrDup( (char *)z, 0);
    }

    /* If we're on Windows, we want to convert all backslashes to
     * forward slashes.  If NOCASE_PATHS is true, we want to also
     * lower the alpha characters in the path.
     */
#if NOCASE_PATHS || defined(__WIN32__)
    {
        int i, c;
        for( i = 0; (c = zResult[i]) != 0; i++ )
        {
#if NOCASE_PATHS
            if( isupper(c) ) {
                zResult[i] = tolower(c);
            }
#endif
#ifdef __WIN32__
            if( c == '\\' ) {
                zResult[i] = '/';
            }
#endif
        }
    }
#endif /* NOCASE_PATHS || defined(__WIN32__) */

    len = strlen(zResult);
    /* Strip the trailing / from any directory. */
    if( zResult[len-1] == '/' ) {
        zResult[len-1] = 0;
    }

    return zResult;
}

/*
 *----------------------------------------------------------------------
 *
 * AddPathToArchive --
 *
 * 	Add the given pathname to the given archive.  zName is usually
 * 	the pathname pulled from the file header in a zip archive.  We
 * 	concatenate it onto the archive's mount point to obtain a full
 * 	path before adding it to our hash table.
 *
 * 	All parent directories of the given path will be created and
 * 	added to the hash table.
 *
 * Results:
 * 	Pointer to the new file structure or to the old file structure
 * 	if it already existed.  newPath will be true if this path is
 * 	new to this archive or false if we already had it.
 *
 *----------------------------------------------------------------------
 */

static ZvfsFile *
AddPathToArchive( ZvfsArchive *pArchive, char *zName, int *newPath )
{
    int i, len, isNew;
    char *zFullPath, *izFullPath;
    char *zParentPath, *izParentPath;
    Tcl_HashEntry *pEntry;
    Tcl_Obj *nameObj, *pathObj, *listObj;
    ZvfsFile *pZvfs, *parent = NULL;

    zFullPath = CanonicalPath( Tcl_GetString(pArchive->zMountPoint), zName );
    izFullPath = zFullPath;

    pathObj = Tcl_NewStringObj( zFullPath, -1 );
    Tcl_IncrRefCount( pathObj );

    listObj = Tcl_FSSplitPath( pathObj, &len );
    Tcl_IncrRefCount( listObj );
    Tcl_DecrRefCount( pathObj );

    /* Walk through all the parent directories of this
     * file and add them all to our archive.  This is
     * because some zip files don't store directory
     * entries in the archive, but we need to know all
     * of the directories to create the proper filesystem.
     */
    for( i = 1; i < len; ++i )
    {
        pathObj = Tcl_FSJoinPath( listObj, i );

        izParentPath = zParentPath  = Tcl_GetString(pathObj);
#if NOCASE_PATHS
        izParentPath = StrDup( zParentPath, 1 );
#endif
        pEntry = Tcl_CreateHashEntry( &local.fileHash, izParentPath, &isNew );
#if NOCASE_PATHS
        Tcl_Free( izParentPath );
#endif

        if( !isNew ) {
            /* We already have this directory in our archive. */
            parent = Tcl_GetHashValue( pEntry );
            continue;
        }

        Tcl_ListObjIndex( NULL, listObj, i-1, &nameObj );
        Tcl_IncrRefCount(nameObj);

        /* We don't have this directory in our archive yet.  Add it. */
        pZvfs = (ZvfsFile*)Tcl_Alloc( sizeof(*pZvfs) );
        pZvfs->refCount   = 1;
        pZvfs->zName      = nameObj;
        pZvfs->pArchive   = pArchive;
        pZvfs->isdir      = 1;
        pZvfs->iOffset    = 0;
        pZvfs->timestamp  = 0;
        pZvfs->iCRC       = 0;
        pZvfs->nByteCompr = 0;
        pZvfs->nByte      = 0;
        pZvfs->parent     = parent;
        Tcl_InitHashTable( &pZvfs->children, TCL_STRING_KEYS );

        Tcl_SetHashValue( pEntry, pZvfs );

        if( parent ) {
            /* Add this directory to its parent's list of children. */
            pEntry = Tcl_CreateHashEntry(&parent->children,zParentPath,&isNew);
            if( isNew ) {
                Tcl_SetHashValue( pEntry, pZvfs );
            }
        }

        parent = pZvfs;
    }

    /* Check to see if we already have this file in our archive. */
#if NOCASE_PATHS
    izFullPath = StrDup( zFullPath, 1 );
#endif
    pEntry = Tcl_CreateHashEntry(&local.fileHash, izFullPath, newPath);
#if NOCASE_PATHS
    Tcl_Free( izFullPath );
#endif

    if( *newPath ) {
        /* We don't have this file in our archive.  Add it. */
        Tcl_ListObjIndex( NULL, listObj, len-1, &nameObj );
        Tcl_IncrRefCount(nameObj);

        pZvfs = (ZvfsFile*)Tcl_Alloc( sizeof(*pZvfs) );
        pZvfs->refCount   = 1;
        pZvfs->zName      = nameObj;
        pZvfs->pArchive   = pArchive;

        Tcl_SetHashValue( pEntry, pZvfs );

        /* Add this path to its parent's list of children. */
        pEntry = Tcl_CreateHashEntry(&parent->children, zFullPath, &isNew);

        if( isNew ) {
            Tcl_SetHashValue( pEntry, pZvfs );
        }
    } else {
        /* We already have this file.  Set the pointer and return. */
        pZvfs = Tcl_GetHashValue( pEntry );
    }

    Tcl_DecrRefCount(listObj);
    Tcl_Free(zFullPath);

    return pZvfs;
}

/*
 *----------------------------------------------------------------------
 *
 * Zvfs_Mount --
 *
 * 	Read a zip archive and make entries in the file hash table for
 * 	all of the files in the archive.  If Zvfs has not been initialized,
 * 	it will be initialized here before mounting the archive.
 *
 * Results:
 * 	Standard Tcl result.
 *
 *----------------------------------------------------------------------
 */

int
Zvfs_Mount(
    Tcl_Interp *interp,      /* Leave error messages in this interpreter */
    CONST char *zArchive,    /* The ZIP archive file */
    CONST char *zMountPoint  /* Mount contents at this directory */
) {
    Tcl_Channel chan = NULL;   /* Used for reading the ZIP archive file */
    char *zArchiveName = 0;    /* A copy of zArchive */
    char *zFullMountPoint = 0; /* Absolute path to the mount point */
    int nFile;                 /* Number of files in the archive */
    int iPos;                  /* Current position in the archive file */
    int code = TCL_ERROR;      /* Return code */
    int update = 1;            /* Whether to update the mounts */
    ZvfsArchive *pArchive;     /* The ZIP archive being mounted */
    Tcl_HashEntry *pEntry;     /* Hash table entry */
    int isNew;                 /* Flag to tell use when a hash entry is new */
    unsigned char zBuf[100];   /* Buffer to read from the ZIP archive */
    ZvfsFile *pZvfs;           /* A new virtual file */
    Tcl_Obj *hashKeyObj = NULL;
    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
    Tcl_Obj *readObj = Tcl_NewObj();
    Tcl_IncrRefCount(readObj);

    if( !local.isInit ) {
        if( Zvfs_Init( interp ) == TCL_ERROR ) {
            Tcl_SetStringObj( resultObj, "failed to initialize zvfs", -1 );
            return TCL_ERROR;
        }
    }

    /* If zArchive is NULL, set the result to a list of all
     * mounted files.
     */
    if( !zArchive ) {
        Tcl_HashSearch zSearch;

        for( pEntry = Tcl_FirstHashEntry( &local.archiveHash,&zSearch );
                pEntry; pEntry = Tcl_NextHashEntry(&zSearch) )
        {
            if( (pArchive = Tcl_GetHashValue(pEntry)) ) {
                Tcl_ListObjAppendElement( interp, resultObj,
                        Tcl_DuplicateObj(pArchive->zName) );
            }
        }
        code = TCL_OK;
        update = 0;
        goto done;
    }

    /* If zMountPoint is NULL, set the result to the mount point
     * for the specified archive file.
     */
    if( !zMountPoint ) {
        int found = 0;
        Tcl_HashSearch zSearch;

        zArchiveName = AbsolutePath( zArchive );
        for( pEntry = Tcl_FirstHashEntry(&local.archiveHash,&zSearch);
                pEntry; pEntry = Tcl_NextHashEntry(&zSearch) )
        {
            pArchive = Tcl_GetHashValue(pEntry);
            if ( !strcmp( Tcl_GetString(pArchive->zName), zArchiveName ) ) {
                ++found;
                Tcl_SetStringObj( resultObj,
                        Tcl_GetString(pArchive->zMountPoint), -1 );
                break;
            }
        }

        if( !found ) {
            Tcl_SetStringObj( resultObj, "archive not mounted by zvfs", -1 );
        }

        code = found ? TCL_OK : TCL_ERROR;
        update = 0;
        goto done;
    }

    if( !(chan = Tcl_OpenFileChannel(interp, zArchive, "r", 0)) ) {
        goto done;
    }

    if(Tcl_SetChannelOption(interp, chan, "-translation", "binary") != TCL_OK) {
        goto done;
    }

    /* Read the "End Of Central Directory" record from the end of the
     * ZIP archive.
     */
    iPos = Tcl_Seek( chan, -22, SEEK_END );
    Tcl_Read( chan, zBuf, 22 );
    if( memcmp(zBuf, "\120\113\05\06", 4) ) {
        Tcl_SetStringObj( resultObj, "bad end of central directory record", -1);
        goto done;
    }

    zArchiveName    = AbsolutePath( zArchive );
    zFullMountPoint = AbsolutePath( zMountPoint );

    hashKeyObj = Tcl_NewObj();
    Tcl_IncrRefCount(hashKeyObj);
    Tcl_AppendStringsToObj( hashKeyObj, zArchiveName, ":", zFullMountPoint,
            (char *)NULL );

    pEntry = Tcl_CreateHashEntry( &local.archiveHash,
            Tcl_GetString(hashKeyObj), &isNew );

    if( !isNew ) {
        /* This archive is already mounted.  Set the result to
         * the current mount point and return.
         */
        pArchive = Tcl_GetHashValue(pEntry);
        code = TCL_OK;
        update = 0;
        goto done;
    }

    pArchive = (ZvfsArchive*)Tcl_Alloc(sizeof(*pArchive));
    pArchive->refCount    = 1;
    pArchive->zName       = Tcl_NewStringObj(zArchiveName,-1);
    pArchive->zMountPoint = Tcl_NewStringObj(zFullMountPoint,-1);
    Tcl_SetHashValue(pEntry, pArchive);

    /* Add the root mount point to our list of archive files as a directory. */
    pEntry = Tcl_CreateHashEntry(&local.fileHash, zFullMountPoint, &isNew);

    if( isNew ) {
        pZvfs = (ZvfsFile*)Tcl_Alloc( sizeof(*pZvfs) );
        pZvfs->refCount   = 1;
        pZvfs->zName      = Tcl_NewStringObj(zFullMountPoint,-1);
        pZvfs->pArchive   = pArchive;
        pZvfs->isdir      = 1;
        pZvfs->iOffset    = 0;
        pZvfs->timestamp  = 0;
        pZvfs->iCRC       = 0;
        pZvfs->nByteCompr = 0;
        pZvfs->nByte      = 0;
        pZvfs->parent     = NULL;
        Tcl_InitHashTable( &pZvfs->children, TCL_STRING_KEYS );

        Tcl_SetHashValue( pEntry, pZvfs );
    }

    /* Compute the starting location of the directory for the
     * ZIP archive in iPos then seek to that location.
     */
    nFile = INT16(zBuf,8);
    iPos -= INT32(zBuf,12);
    Tcl_Seek( chan, iPos, SEEK_SET );

    while( nFile-- > 0 )
    {
        int isdir = 0;
        int iData;              /* Offset to start of file data */
        int lenName;            /* Length of the next filename */
        int lenExtra;           /* Length of "extra" data for next file */
        int attributes;         /* DOS attributes */
        char *zName;
        char *zFullPath;        /* Full pathname of the virtual file */
        char *izFullPath;       /* Lowercase full pathname */
        ZvfsFile *parent;

        /* Read the next directory entry.  Extract the size of the filename,
         * the size of the "extra" information, and the offset into the archive
         * file of the file data.
         */
        Tcl_Read( chan, zBuf, 46 );
        if( memcmp(zBuf, "\120\113\01\02", 4) ) {
            Zvfs_Unmount( interp, zArchiveName );
            Tcl_SetStringObj( resultObj, "bad central file record", -1 );
            goto done;
        }

        lenName  = INT16(zBuf,28);
        lenExtra = INT16(zBuf,30) + INT16(zBuf,32);
        iData    = INT32(zBuf,42);

        /* Construct an entry in local.fileHash for this virtual file. */
        Tcl_ReadChars( chan, readObj, lenName, 0 );

        zName = Tcl_GetString(readObj);

        if( zName[--lenName] == '/' ) {
            isdir = 1;
            Tcl_SetObjLength( readObj, lenName );
        }

        pZvfs = AddPathToArchive( pArchive, zName, &isNew );

        pZvfs->isdir      = isdir;
        pZvfs->iOffset    = iData;
        pZvfs->timestamp  = DosTimeDate(INT16(zBuf, 14), INT16(zBuf, 12));
        pZvfs->iCRC       = INT32(zBuf, 16);
        pZvfs->nByteCompr = INT32(zBuf, 20);
        pZvfs->nByte      = INT32(zBuf, 24);

        /* If this is a directory we want to initialize the
         * hash table to store its children if it has any.
         */
        if( isNew && isdir ) {
            Tcl_InitHashTable( &pZvfs->children, TCL_STRING_KEYS );
        }

        /* Skip over the extra information so that the next read
         * will be from the beginning of the next directory entry.
         */
        Tcl_Seek( chan, lenExtra, SEEK_CUR );
    }

    code = TCL_OK;

done:
    if( chan ) Tcl_Close( interp, chan );

    if( readObj ) Tcl_DecrRefCount(readObj);
    if( hashKeyObj ) Tcl_DecrRefCount(hashKeyObj);

    if( zArchiveName ) Tcl_Free(zArchiveName);
    if( zFullMountPoint ) Tcl_Free(zFullMountPoint);

    if( code == TCL_OK && update ) {
        Tcl_FSMountsChanged( &zvfsFilesystem );
        Tcl_SetStringObj( resultObj, zMountPoint, -1 );
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Zvfs_Unmount --
 *
 * 	Unmount all the files in the given zip archive.  All the
 * 	entries in the file hash table for the archive are deleted
 * 	as well as the entry in the archive hash table.
 *
 * 	Any memory associated with the entries will be freed as well.
 *
 * Results:
 * 	Standard Tcl result.
 *
 *----------------------------------------------------------------------
 */

int
Zvfs_Unmount( Tcl_Interp *interp, CONST char *zMountPoint )
{
    int found = 0;
    ZvfsFile *pFile;
    ZvfsArchive *pArchive;
    Tcl_HashEntry *pEntry;
    Tcl_HashSearch zSearch;
    Tcl_HashEntry *fEntry;
    Tcl_HashSearch fSearch;

    for( pEntry = Tcl_FirstHashEntry( &local.archiveHash, &zSearch );
            pEntry; pEntry = Tcl_NextHashEntry(&zSearch) )
    {
        pArchive = Tcl_GetHashValue(pEntry);
        if( !Tcl_StringCaseMatch( zMountPoint,
                Tcl_GetString(pArchive->zMountPoint), NOCASE_PATHS ) ) continue;

        found++;

        for( fEntry = Tcl_FirstHashEntry( &local.fileHash, &fSearch );
                fEntry; fEntry = Tcl_NextHashEntry(&fSearch) )
        {
            pFile = Tcl_GetHashValue(fEntry);
            if( pFile->pArchive == pArchive ) {
                FreeInternalRep( (ClientData)pFile );
                Tcl_DeleteHashEntry(fEntry);
            }
        }

        Tcl_DeleteHashEntry(pEntry);
        Tcl_DecrRefCount(pArchive->zName);
        Tcl_DecrRefCount(pArchive->zMountPoint);
        Tcl_Free( (char *)pArchive );
    }

    if( !found ) {
        if( interp ) {
            Tcl_AppendStringsToObj( Tcl_GetObjResult(interp),
                    zMountPoint, " is not a zvfs mount", (char *)NULL );
        }
        return TCL_ERROR;
    }

    Tcl_FSMountsChanged( &zvfsFilesystem );

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ZvfsLookup --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Look into the file hash table for a given path and see if
 * 	it belongs to our filesystem.
 *
 * Results:
 * 	Pointer to the file structure or NULL if it was not found.
 *
 *----------------------------------------------------------------------
 */

static ZvfsFile *
ZvfsLookup( Tcl_Obj *pathPtr )
{
    char *zTrueName;
    Tcl_HashEntry *pEntry;

    zTrueName = AbsolutePath( Tcl_GetString(pathPtr) );
    pEntry = Tcl_FindHashEntry( &local.fileHash, zTrueName );
    Tcl_Free(zTrueName);

    return pEntry ? Tcl_GetHashValue(pEntry) : NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * GetZvfsFile --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	For a given pathPtr, return the internal representation
 * 	of the path for our filesystem.
 *
 * Results:
 * 	Pointer to the file structure or NULL if it was not found.
 *
 *----------------------------------------------------------------------
 */

static ZvfsFile *
GetZvfsFile( Tcl_Obj *pathPtr )
{
    ZvfsFile *pFile = (ZvfsFile *)Tcl_FSGetInternalRep(pathPtr,&zvfsFilesystem);
    return pFile == NULL || pFile->pArchive->refCount == 0 ? NULL : pFile;
}

/*
 *----------------------------------------------------------------------
 *
 * ZvfsFileMatchesType --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	See if the given ZvfsFile matches the type data given.
 *
 * Results:
 * 	1 if true, 0 if false
 *
 *----------------------------------------------------------------------
 */

static int
ZvfsFileMatchesType( ZvfsFile *pFile, Tcl_GlobTypeData *types )
{
    if( types ) {
        if( types->type & TCL_GLOB_TYPE_FILE && pFile->isdir ) {
            return 0;
        }

        if( types->type & (TCL_GLOB_TYPE_DIR | TCL_GLOB_TYPE_MOUNT)
                && !pFile->isdir ) {
            return 0;
        }

        if( types->type & TCL_GLOB_TYPE_MOUNT && pFile->parent ) {
            return 0;
        }
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * DriverExit --
 *
 * 	This function is called as an exit handler for the channel
 * 	driver.  If we do not set pInfo.chan to NULL, Tcl_Close()
 * 	will be called twice on that channel when Tcl_Exit runs.
 * 	This will lead to a core dump
 *
 * Results:
 * 	None
 *
 *----------------------------------------------------------------------
 */

static void
DriverExit( void *pArg )
{
    ZvfsChannelInfo *pInfo = (ZvfsChannelInfo*)pArg;
    pInfo->chan = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * DriverClose --
 *
 * 	Called when a channel is closed.
 *
 * Results:
 * 	Returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */

static int
DriverClose(
  ClientData  instanceData,    /* A ZvfsChannelInfo structure */
  Tcl_Interp *interp           /* The TCL interpreter */
) {
    ZvfsChannelInfo* pInfo = (ZvfsChannelInfo*)instanceData;

    if( pInfo->zBuf ){
        Tcl_Free(pInfo->zBuf);
        inflateEnd(&pInfo->stream);
    }

    if( pInfo->chan ){
        Tcl_Close(interp, pInfo->chan);
        Tcl_DeleteExitHandler(DriverExit, pInfo);
    }

    Tcl_Free((char*)pInfo);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DriverInput --
 *
 * 	The Tcl channel system calls this function on each read
 * 	from a channel.  The channel is opened into the actual
 * 	archive file, but the data is read from the individual
 * 	file entry inside the zip archive.
 *
 * Results:
 * 	Number of bytes read.
 *
 *----------------------------------------------------------------------
 */

static int
DriverInput (
  ClientData instanceData, /* The channel to read from */
  char *buf,               /* Buffer to fill */
  int toRead,              /* Requested number of bytes */
  int *pErrorCode          /* Location of error flag */
) {
    ZvfsChannelInfo* pInfo = (ZvfsChannelInfo*) instanceData;

    if( toRead > pInfo->nByte ) {
        toRead = pInfo->nByte;
    }

    if( toRead == 0 ) {
        return 0;
    }

    if( pInfo->isCompressed ) {
        int err = Z_OK;
        z_stream *stream = &pInfo->stream;
        stream->next_out = buf;
        stream->avail_out = toRead;
        while (stream->avail_out) {
            if (!stream->avail_in) {
                int len = pInfo->nByteCompr;
                if (len > COMPR_BUF_SIZE) {
                    len = COMPR_BUF_SIZE;
                }
                len = Tcl_Read(pInfo->chan, pInfo->zBuf, len);
                pInfo->nByteCompr -= len;
                stream->next_in = pInfo->zBuf;
                stream->avail_in = len;
            }

            err = inflate(stream, Z_NO_FLUSH);
            if (err) break;
        }

        if (err == Z_STREAM_END) {
            if ((stream->avail_out != 0)) {
                *pErrorCode = err; /* premature end */
                return -1;
            }
        } else if( err ) {
            *pErrorCode = err; /* some other zlib error */
            return -1;
        }
    } else {
        toRead = Tcl_Read(pInfo->chan, buf, toRead);
    }

    pInfo->nByte -= toRead;
    pInfo->readSoFar += toRead;
    *pErrorCode = 0;

    return toRead;
}

/*
 *----------------------------------------------------------------------
 *
 * DriverOutput --
 *
 * 	Called to write to a file.  Since this is a read-only file
 * 	system, this function will always return an error.
 *
 * Results:
 * 	Returns -1.
 *
 *----------------------------------------------------------------------
 */

static int
DriverOutput(
  ClientData instanceData,   /* The channel to write to */
  CONST char *buf,                 /* Data to be stored. */
  int toWrite,               /* Number of bytes to write. */
  int *pErrorCode            /* Location of error flag. */
) {
    *pErrorCode = EINVAL;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * DriverSeek --
 *
 * 	Seek along the open channel to another point.
 *
 * Results:
 * 	Offset into the file.
 *
 *----------------------------------------------------------------------
 */

static int
DriverSeek(
  ClientData instanceData,    /* The file structure */
  long offset,                /* Offset to seek to */
  int mode,                   /* One of SEEK_CUR, SEEK_SET or SEEK_END */
  int *pErrorCode             /* Write the error code here */
){
    ZvfsChannelInfo* pInfo = (ZvfsChannelInfo*) instanceData;

    switch( mode ) {
    case SEEK_CUR:
        offset += pInfo->readSoFar;
        break;
    case SEEK_END:
        offset += pInfo->readSoFar + pInfo->nByte;
        break;
    default:
        /* Do nothing */
        break;
    }

    if( !pInfo->isCompressed ){
        /* dont seek behind end of data */
	if (pInfo->nData < (unsigned long)offset) {
	    return -1;
        }

	/* do the job, save and check the result */
	offset = Tcl_Seek(pInfo->chan, offset + pInfo->startOfData, SEEK_SET);
	if (offset == -1) {
	    return -1;
        }

	 /* adjust the counters (use real offset) */
	pInfo->readSoFar = offset - pInfo->startOfData;
	pInfo->nByte = pInfo->nData - pInfo->readSoFar; 
    } else {
        if( offset<pInfo->readSoFar ) {
            z_stream *stream = &pInfo->stream;
            inflateEnd(stream);
            stream->zalloc   = (alloc_func)0;
            stream->zfree    = (free_func)0;
            stream->opaque   = (voidpf)0;
            stream->avail_in = 2;
            stream->next_in  = pInfo->zBuf;
            pInfo->zBuf[0]   = 0x78;
            pInfo->zBuf[1]   = 0x01;
            inflateInit(&pInfo->stream);
            Tcl_Seek(pInfo->chan, pInfo->startOfData, SEEK_SET);
            pInfo->nByte += pInfo->readSoFar;
            pInfo->nByteCompr = pInfo->nData;
            pInfo->readSoFar = 0;
        }

        while( pInfo->readSoFar < offset )
        {
            int toRead, errCode;
            char zDiscard[100];
            toRead = offset - pInfo->readSoFar;
            if( toRead>sizeof(zDiscard) ) toRead = sizeof(zDiscard);
            DriverInput(instanceData, zDiscard, toRead, &errCode);
        }
    }

    return pInfo->readSoFar;
}

/*
 *----------------------------------------------------------------------
 *
 * DriverWatch --
 *
 * 	Called to handle events on the channel.  Since zvfs files
 * 	don't generate events, this is a no-op.
 *
 * Results:
 * 	None
 *
 *----------------------------------------------------------------------
 */

static void
DriverWatch(
  ClientData instanceData,   /* Channel to watch */
  int mask                   /* Events of interest */
) {
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * DriverGetHandle --
 *
 * 	Retrieve a device-specific handle from the given channel.
 * 	Since we don't have a device-specific handle, this is a no-op.
 *
 * Results:
 * 	Returns TCL_ERROR.
 *
 *----------------------------------------------------------------------
 */

static int
DriverGetHandle(
  ClientData  instanceData,   /* Channel to query */
  int direction,              /* Direction of interest */
  ClientData* handlePtr       /* Space to the handle into */
) {
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * PathInFilesystem --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Check to see if the given path is part of our filesystem.
 * 	We check the file hash table for the path, and if we find
 * 	it, set clientDataPtr to the ZvfsFile pointer so that Tcl
 * 	will cache it for later.
 *
 * Results:
 * 	TCL_OK on success, or -1 on failure
 *
 *----------------------------------------------------------------------
 */

static int
PathInFilesystem( Tcl_Obj *pathPtr, ClientData *clientDataPtr )
{
    ZvfsFile *pFile = ZvfsLookup(pathPtr);

    if( pFile ) {
        *clientDataPtr = DupInternalRep((ClientData)pFile);
        return TCL_OK;
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * DupInternalRep --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Duplicate the ZvfsFile "native" rep of a path.
 *
 * Results:
 * 	Returns clientData, with refcount incremented.
 *
 *----------------------------------------------------------------------
 */

static ClientData
DupInternalRep( ClientData clientData )
{
    ZvfsFile *pFile = (ZvfsFile *)clientData;
    pFile->refCount++;
    return (ClientData)pFile;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeInternalRep --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Free one reference to the ZvfsFile "native" rep of a path.
 * 	When all references are gone, free the struct.
 *
 * Side effects:
 * 	May free memory.
 *
 *----------------------------------------------------------------------
 */

static void
FreeInternalRep( ClientData clientData )
{
    ZvfsFile *pFile = (ZvfsFile *)clientData;

    if (--pFile->refCount <= 0) {
        if( pFile->isdir ) {
            /* Delete the hash table containing the children
             * of this directory.  We don't need to free the
             * data for each entry in the table because they're
             * just pointers to the ZvfsFiles, and those will
             * be freed below.
             */
            Tcl_DeleteHashTable( &pFile->children );
        }
        Tcl_DecrRefCount(pFile->zName);
        Tcl_Free((char *)pFile);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InternalToNormalized --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	From a ZvfsFile representation, produce the path string rep.
 *
 * Results:
 * 	Returns a Tcl_Obj holding the string rep.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
InternalToNormalized( ClientData clientData )
{
    ZvfsFile *pFile = (ZvfsFile *)clientData;
    if( !pFile->parent ) {
        return Tcl_DuplicateObj( pFile->zName );
    } else {
        return Tcl_FSJoinToPath( pFile->parent->zName, 1, &pFile->zName );
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FilesystemPathType --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Used for informational purposes only.  Return a Tcl_Obj
 * 	which describes the "type" of path this is.  For our
 * 	little filesystem, they're all "zip".
 *
 * Results:
 * 	Tcl_Obj with 0 refCount
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
FilesystemPathType( Tcl_Obj *pathPtr )
{
    return Tcl_NewStringObj( "zip", -1 );
}

/*
 *----------------------------------------------------------------------
 *
 * FileSystemSeparator --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Return a Tcl_Obj describing the separator character for
 * 	our filesystem.  We like things the old-fashioned way,
 * 	so we'll just use /.
 *
 * Results:
 * 	Tcl_Obj with 0 refCount
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
FilesystemSeparator( Tcl_Obj *pathPtr )
{ 
    return Tcl_NewStringObj( "/", -1 );
}

/*
 *----------------------------------------------------------------------
 *
 * Stat --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Does a stat() system call for a zvfs file.  Fill the stat
 * 	buf with as much information as we have.
 *
 * Results:
 * 	0 on success, -1 on failure.
 *
 *----------------------------------------------------------------------
 */

static int
Stat( Tcl_Obj *pathPtr, Tcl_StatBuf *buf )
{
    ZvfsFile *pFile;

    if( !(pFile = GetZvfsFile(pathPtr)) ) {
        return -1;
    }

    memset(buf, 0, sizeof(*buf));
    if (pFile->isdir) {
        buf->st_mode = 040555;
    } else {
        buf->st_mode = 0100555;
    }

    buf->st_size  = pFile->nByte;
    buf->st_mtime = pFile->timestamp;
    buf->st_ctime = pFile->timestamp;
    buf->st_atime = pFile->timestamp;

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Access --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Does an access() system call for a zvfs file.
 *
 * Results:
 * 	0 on success, -1 on failure.
 *
 *----------------------------------------------------------------------
 */

static int
Access( Tcl_Obj *pathPtr, int mode )
{
    if( mode & 3 || !GetZvfsFile(pathPtr) ) return -1;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * OpenFileChannel --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Called when Tcl wants to open a file inside a zvfs file system.
 * 	We actually open the zip file back up and seek to the offset
 * 	of the given file.  The channel driver will take care of the
 * 	rest.
 *
 * Results:
 * 	New channel on success, NULL on failure.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Channel
OpenFileChannel( Tcl_Interp *interp, Tcl_Obj *pathPtr,
    int mode, int permissions )
{
    ZvfsFile *pFile;
    ZvfsChannelInfo *pInfo;
    Tcl_Channel chan;
    static int count = 1;
    char zName[50];
    unsigned char zBuf[50];

    if( !(pFile = GetZvfsFile(pathPtr)) ) {
        return NULL;
    }

    if(!(chan = Tcl_OpenFileChannel(interp,
                    Tcl_GetString(pFile->pArchive->zName), "r", 0))) {
        return NULL;
    }

    if( Tcl_SetChannelOption(interp, chan, "-translation", "binary") ) {
        /* this should never happen */
        Tcl_Close( NULL, chan );
        return NULL;
    }

    Tcl_Seek(chan, pFile->iOffset, SEEK_SET);
    Tcl_Read(chan, zBuf, 30);

    if( memcmp(zBuf, "\120\113\03\04", 4) ){
        if( interp ) {
            Tcl_SetStringObj( Tcl_GetObjResult(interp),
                    "bad central file record", -1 );
        }
        Tcl_Close( interp, chan );
        return NULL;
    }

    pInfo = (ZvfsChannelInfo*)Tcl_Alloc( sizeof(*pInfo) );
    pInfo->chan = chan;
    Tcl_CreateExitHandler(DriverExit, pInfo);
    pInfo->isCompressed = INT16(zBuf, 8);

    if( pInfo->isCompressed ) {
        z_stream *stream = &pInfo->stream;
        pInfo->zBuf      = Tcl_Alloc(COMPR_BUF_SIZE);
        stream->zalloc   = (alloc_func)0;
        stream->zfree    = (free_func)0;
        stream->opaque   = (voidpf)0;
        stream->avail_in = 2;
        stream->next_in  = pInfo->zBuf;
        pInfo->zBuf[0]   = 0x78;
        pInfo->zBuf[1]   = 0x01;
        inflateInit(&pInfo->stream);
    } else {
        pInfo->zBuf = 0;
    }

    pInfo->nByte      = INT32(zBuf,22);
    pInfo->nByteCompr = pInfo->nData = INT32(zBuf,18);
    pInfo->readSoFar  = 0;
    Tcl_Seek( chan, INT16(zBuf,26) + INT16(zBuf,28), SEEK_CUR );
    pInfo->startOfData = Tcl_Tell(chan);
    sprintf( zName, "zvfs%x%x", ((uintptr_t)pFile)>>12, count++ );

    return Tcl_CreateChannel( &vfsChannelType, zName, 
                                (ClientData)pInfo, TCL_READABLE );
}

/*
 *----------------------------------------------------------------------
 *
 * MatchInDirectory --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Called when Tcl is globbing around through the filesystem.
 * 	This function can be called when Tcl is looking for mount
 * 	points or when it is looking for files within a mount point
 * 	that it has already determined belongs to us.
 *
 * 	Any matching file in our filesystem is appended to the
 * 	result pointer.
 *
 * Results:
 * 	Standard Tcl result
 *
 *----------------------------------------------------------------------
 */

/* Function to process a 'MatchInDirectory()'.
 * If not implemented, then glob and recursive
 * copy functionality will be lacking in the filesystem.
 */
static int
MatchInDirectory(
    Tcl_Interp* interp,
    Tcl_Obj *result,
    Tcl_Obj *pathPtr,
    CONST char *pattern,
    Tcl_GlobTypeData *types
) {
    ZvfsFile *pFile;
    Tcl_HashEntry *pEntry;
    Tcl_HashSearch sSearch;

    if( types && types->type & TCL_GLOB_TYPE_MOUNT ) {
        /* Tcl is looking for a list of our mount points that
         * match the given pattern.  This is so that Tcl can
         * append vfs mounted directories to a list of actual
         * filesystem directories.
         */
        char *path, *zPattern;
        ZvfsArchive *pArchive;
        Tcl_Obj *patternObj = Tcl_NewObj();

        path = AbsolutePath( Tcl_GetString(pathPtr) );
        Tcl_AppendStringsToObj( patternObj, path, "/", pattern, (char *)NULL );
        Tcl_Free(path);
        zPattern = Tcl_GetString( patternObj );

        for( pEntry = Tcl_FirstHashEntry( &local.archiveHash, &sSearch );
                pEntry; pEntry = Tcl_NextHashEntry( &sSearch ) )
        {
            pArchive = Tcl_GetHashValue(pEntry);
            if( Tcl_StringCaseMatch( Tcl_GetString(pArchive->zMountPoint),
                        zPattern, NOCASE_PATHS ) ) {
                Tcl_ListObjAppendElement( NULL, result,
                        Tcl_DuplicateObj(pArchive->zMountPoint) );
            }
        }

        Tcl_DecrRefCount(patternObj);

        return TCL_OK;
    }

    if( !(pFile = GetZvfsFile(pathPtr)) ) {
        Tcl_SetStringObj( Tcl_GetObjResult(interp), "stale file handle", -1 );
        return TCL_ERROR;
    }

    if( !pattern ) {
        /* If pattern is null, Tcl is actually just checking to
         * see if this file exists in our filesystem.  Check to
         * make sure the path matches any type data and then
         * append it to the result and return.
         */
        if( ZvfsFileMatchesType( pFile, types ) ) {
            Tcl_ListObjAppendElement( NULL, result, pathPtr );
        }
        return TCL_OK;
    }

    /* We've determined that the requested path is in our filesystem,
     * so now we want to walk through the children of the directory
     * and find any that match the given pattern and type.  Any we
     * find will be appended to the result.
     */

    for( pEntry = Tcl_FirstHashEntry(&pFile->children, &sSearch);
        pEntry; pEntry = Tcl_NextHashEntry(&sSearch) )
    {
        char *zName;
        pFile = Tcl_GetHashValue(pEntry);
        zName = Tcl_GetString(pFile->zName);

        if( ZvfsFileMatchesType( pFile, types )
                && Tcl_StringCaseMatch(zName, pattern, NOCASE_PATHS) ) {
            Tcl_ListObjAppendElement( NULL, result,
                    Tcl_FSJoinToPath(pathPtr, 1, &pFile->zName ) );
        }
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListVolumes --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Called when Tcl is looking for a list of open volumes
 * 	for our filesystem.  The mountpoint for each open archive
 * 	is appended to a list object.
 *
 * Results:
 * 	A Tcl_Obj with 0 refCount
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
ListVolumes(void)
{
    Tcl_HashEntry *pEntry;    /* Hash table entry */
    Tcl_HashSearch zSearch;   /* Search all mount points */
    ZvfsArchive *pArchive;    /* The ZIP archive being mounted */
    Tcl_Obj *pVols = Tcl_NewObj();
  
    for( pEntry = Tcl_FirstHashEntry(&local.archiveHash,&zSearch);
            pEntry; pEntry = Tcl_NextHashEntry(&zSearch) )
    {
        pArchive = Tcl_GetHashValue(pEntry);

        Tcl_ListObjAppendElement( NULL, pVols,
                Tcl_DuplicateObj(pArchive->zMountPoint) );
    }

    return pVols;
}

/*
 *----------------------------------------------------------------------
 *
 * FileAttrStrings --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Return an array of strings for all of the possible
 * 	attributes for a file in zvfs.
 *
 * Results:
 * 	Pointer to ZvfsAttrs
 *
 *----------------------------------------------------------------------
 */

const char *CONST86 *
FileAttrStrings( Tcl_Obj *pathPtr, Tcl_Obj** objPtrRef )
{
    return ZvfsAttrs;
}

/*
 *----------------------------------------------------------------------
 *
 * FileAttrsGet --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Called for a "file attributes" command from Tcl
 * 	to return the attributes for a file in our filesystem.
 *
 * 	objPtrRef will point to a 0 refCount Tcl_Obj on success.
 *
 * Results:
 * 	Standard Tcl result
 *
 *----------------------------------------------------------------------
 */

static int
FileAttrsGet( Tcl_Interp *interp, int index,
    Tcl_Obj *pathPtr, Tcl_Obj **objPtrRef )
{
    char *zFilename;
    ZvfsFile *pFile;
    zFilename = Tcl_GetString(pathPtr);

    if( !(pFile = GetZvfsFile(pathPtr)) ) {
        return TCL_ERROR;
    }

    switch(index) {
    case ZVFS_ATTR_ARCHIVE:
        *objPtrRef= Tcl_DuplicateObj(pFile->pArchive->zName);
        return TCL_OK;
    case ZVFS_ATTR_COMPSIZE:
        *objPtrRef=Tcl_NewIntObj(pFile->nByteCompr);
        return TCL_OK;
    case ZVFS_ATTR_CRC:
        *objPtrRef=Tcl_NewIntObj(pFile->iCRC);
        return TCL_OK;
    case ZVFS_ATTR_MOUNT:
        *objPtrRef= Tcl_DuplicateObj(pFile->pArchive->zMountPoint);
        return TCL_OK;
    case ZVFS_ATTR_OFFSET:
        *objPtrRef= Tcl_NewIntObj(pFile->nByte);
        return TCL_OK;
    case ZVFS_ATTR_UNCOMPSIZE:
        *objPtrRef= Tcl_NewIntObj(pFile->nByte);
        return TCL_OK;
    default:
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FileAttrsSet --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Called to set the value of an attribute for the
 * 	given file.  Since we're a read-only filesystem, this
 * 	always returns an error.
 *
 * Results:
 * 	Returns TCL_ERROR
 *
 *----------------------------------------------------------------------
 */

static int
FileAttrsSet( Tcl_Interp *interp, int index,
                            Tcl_Obj *pathPtr, Tcl_Obj *objPtr )
{
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Chdir --
 *
 * 	Part of the "zvfs" Tcl_Filesystem.
 * 	Handles a chdir() call for the filesystem.  Tcl has
 * 	already determined that the directory belongs to us,
 * 	so we just need to check and make sure that the path
 * 	is actually a directory in our filesystem and not a
 * 	regular file.
 *
 * Results:
 * 	0 on success, -1 on failure.
 *
 *----------------------------------------------------------------------
 */

static int
Chdir( Tcl_Obj *pathPtr )
{
    ZvfsFile *zFile = GetZvfsFile(pathPtr);
    if( !zFile || !zFile->isdir ) return -1;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * MountObjCmd --
 *
 * 	This function implements the [zvfs::mount] command.
 *
 * 	zvfs::mount ?zipFile? ?mountPoint?
 *
 * 	Creates a new mount point to the given zip archive.
 * 	All files in the zip archive will be added to the
 * 	virtual filesystem and be available to Tcl as regular
 * 	files and directories.
 *
 * Results:
 * 	Standard Tcl result
 *
 *----------------------------------------------------------------------
 */

static int
MountObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
) {
    char *zipFile = NULL, *mountPoint = NULL;

    if( objc > 3 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "?zipFile? ?mountPoint?" );
        return TCL_ERROR;
    }

    if( objc > 1 ) {
        zipFile = Tcl_GetString( objv[1] );
    }

    if( objc > 2 ) {
        mountPoint = Tcl_GetString( objv[2] );
    }

    return Zvfs_Mount( interp, zipFile, mountPoint );
}

/*
 *----------------------------------------------------------------------
 *
 * UnmountObjCmd --
 *
 * 	This function implements the [zvfs::unmount] command.
 *
 * 	zvfs::unmount mountPoint
 *
 * 	Unmount the given mountPoint if it is mounted in our
 * 	filesystem.
 *
 * Results:
 * 	0 on success, -1 on failure.
 *
 *----------------------------------------------------------------------
 */

static int
UnmountObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
) {
    if( objc != 2 ) {
        Tcl_WrongNumArgs( interp, objc, objv, "mountPoint" );
        return TCL_ERROR;
    }

    return Zvfs_Unmount( interp, Tcl_GetString(objv[1]) );
}

/*
 *----------------------------------------------------------------------
 *
 * Zvfs_Init, Zvfs_SafeInit --
 *
 * 	Initialize the zvfs package.
 *
 * 	Safe interpreters do not receive the ability to mount and
 * 	unmount zip files.
 *
 * Results:
 * 	Standard Tcl result
 *
 *----------------------------------------------------------------------
 */

int
Zvfs_SafeInit( Tcl_Interp *interp )
{
#ifdef USE_TCL_STUBS
    if( Tcl_InitStubs( interp, "8.0", 0 ) == TCL_ERROR ) return TCL_ERROR;
#endif

    if( !local.isInit ) {
        /* Register the filesystem and initialize the hash tables. */
	Tcl_FSRegister( 0, &zvfsFilesystem );
	Tcl_InitHashTable( &local.fileHash, TCL_STRING_KEYS );
	Tcl_InitHashTable( &local.archiveHash, TCL_STRING_KEYS );

	local.isInit = 1;
    }

    Tcl_PkgProvide( interp, "zvfs", "1.0" );

    return TCL_OK;
}

int
Zvfs_Init( Tcl_Interp *interp )
{
    if( Zvfs_SafeInit( interp ) == TCL_ERROR ) return TCL_ERROR;

    if( !Tcl_IsSafe(interp) ) {
        Tcl_CreateObjCommand(interp, "zvfs::mount", MountObjCmd, 0, 0);
        Tcl_CreateObjCommand(interp, "zvfs::unmount", UnmountObjCmd, 0, 0);
    }

    return TCL_OK;
}
