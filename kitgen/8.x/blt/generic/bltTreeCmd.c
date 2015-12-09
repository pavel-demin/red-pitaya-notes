
/*
 *
 * bltTreeCmd.c --
 *
 * Copyright 1998-1999 Lucent Technologies, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and warranty
 * disclaimer appear in supporting documentation, and that the names
 * of Lucent Technologies or any of their entities not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * Lucent Technologies disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability and
 * fitness.  In no event shall Lucent Technologies be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in
 * an action of contract, negligence or other tortuous action, arising
 * out of or in connection with the use or performance of this
 * software.
 *
 *	The "tree" data object was created by George A. Howlett.
 *      Massive modifications by Peter MacDonald
 *
 */

/*
  tree create t0 t1 t2
  tree names
  t0 destroy
     -or-
  tree destroy t0
  tree copy tree@node tree@node -recurse -tags

  tree move node after|before|into t2@node

  $t apply -recurse $root command arg arg			

  $t attach treename				

  $t children $n
  t0 copy node1 node2 node3 node4 node5 destName 
  $t delete $n...				
  $t depth $n
  $t dump $root
  $t dumpfile $root fileName
  $t dup $t2		
  $t find $root -name pat -name pattern
  $t firstchild $n
  $t get $n $key
  $t get $n $key(abc)
  $t index $n
  $t insert $parent $switches?
  $t isancestor $n1 $n2
  $t isbefore $n1 $n2
  $t isleaf $n
  $t lastchild $n
  $t move $n1 after|before|into $n2
  $t next $n
  $t nextsibling $n
  $t path $n1 $n2 $n3...
  $t parent $n
  $t previous $n
  $t prevsibling $n
  $t restore $root data -overwrite
  $t root ?$n?

  $t set $n $key $value ?$key $value?
  $t size $n
  $t slink $n $t2@$node				???
  $t sort -recurse $root		

  $t tag delete tag1 tag2 tag3...
  $t tag names
  $t tag nodes $tag
  $t tag set $n tag1 tag2 tag3...
  $t tag unset $n tag1 tag2 tag3...

  $t trace create $n $key how command		
  $t trace delete id1 id2 id3...
  $t trace names
  $t trace info $id

  $t unset $n key1 key2 key3...
  
  $t notify create -oncreate -ondelete -onmove command 
  $t notify create -oncreate -ondelete -onmove -onsort command arg arg arg 
  $t notify delete id1 id2 id3
  $t notify names
  $t notify info id

  for { set n [$t firstchild $node] } { $n >= 0 } { 
        set n [$t nextsibling $n] } {
  }
  foreach n [$t children $node] { 
	  
  }
  set n [$t next $node]
  set n [$t previous $node]

*/

#include <bltInt.h>

#ifndef NO_TREE

#include <bltTree.h>
#include <bltHash.h>
#include <bltList.h>
#include <bltVector.h>
#include "bltSwitch.h"
#include <ctype.h>

#define TREE_THREAD_KEY "BLT Tree Command Data"
#define TREE_MAGIC ((unsigned int) 0x46170277)

#ifndef WITH_SQLITE3
#define OMIT_SQLITE3_LOAD
#endif
enum TagTypes { TAG_TYPE_NONE, TAG_TYPE_ALL, TAG_TYPE_TAG, TAG_TYPE_LIST, TAG_TYPE_ROOTCHILDREN };

typedef struct {
    Blt_HashTable treeTable;	/* Hash table of trees keyed by address. */
    Tcl_Interp *interp;
} TreeCmdInterpData;

typedef struct {
    Tcl_Interp *interp;
    Tcl_Command cmdToken;	/* Token for tree's Tcl command. */
    Blt_Tree tree;		/* Token holding internal tree. */

    Blt_HashEntry *hashPtr;
    Blt_HashTable *tablePtr;

    TreeCmdInterpData *dataPtr;	/*  */

    int traceCounter;		/* Used to generate trace id strings.  */
    Blt_HashTable traceTable;	/* Table of active traces. Maps trace ids
				 * back to their TraceInfo records. */

    int notifyCounter;		/* Used to generate notify id strings. */
    Blt_HashTable notifyTable;	/* Table of event handlers. Maps notify ids
				 * back to their NotifyInfo records. */
    int oldLen;     /* Length of oldvalue before append/lappend. */
    int updTyp;     /* Type of update: 1=append, 2=lappend, 0=other. */
    int delete;
} TreeCmd;

typedef struct {
    TreeCmd *cmdPtr;
    Blt_TreeNode node;

    Blt_TreeTrace traceToken;

    char *withTag;		/* If non-NULL, the event or trace was
				 * specified with this tag.  This
				 * value is saved for informational
				 * purposes.  The tree's trace
				 * matching routines do the real
				 * checking, not the client's
				 * callback.  */

    char command[1];		/* Command prefix for the trace or notify
				 * Tcl callback.  Extra arguments will be
				 * appended to the end. Extra space will
				 * be allocated for the length of the string
				 */
} TraceInfo;

typedef struct {
    int init;
    int tagType;
    Blt_TreeNode root;
    Blt_HashSearch cursor;
    TreeCmd *cmdPtr;
    Tcl_Obj **objv, *objPtr;
    int objc;
    int idx;
    Blt_TreeNode node;
    Blt_TreeTagEntry *tPtr;
    int cnt;
    unsigned int inode;
} TagSearch;

typedef struct {
    TreeCmd *cmdPtr;
    int mask;
    Tcl_Obj **objv;
    int objc;
    Blt_TreeNode node;		/* Node affected by event. */
    Blt_TreeTrace notifyToken;
} NotifyInfo;


typedef struct {
    int mask;
} NotifyData;

static Blt_SwitchSpec notifySwitches[] = 
{
    {BLT_SWITCH_FLAG, "-bgerror", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_BGERROR},
    {BLT_SWITCH_FLAG, "-create", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_CREATE},
    {BLT_SWITCH_FLAG, "-delete", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_DELETE},
   {BLT_SWITCH_FLAG, "-disabletrace", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_TRACEACTIVE},
    {BLT_SWITCH_FLAG, "-get", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_GET},
    {BLT_SWITCH_FLAG, "-insert", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_INSERT},
    {BLT_SWITCH_FLAG, "-move", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_MOVE},
    {BLT_SWITCH_FLAG, "-movepost", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_MOVEPOST},
    {BLT_SWITCH_FLAG, "-sort", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_SORT},
    {BLT_SWITCH_FLAG, "-relabel", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_RELABEL},
    {BLT_SWITCH_FLAG, "-relabelpost", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_RELABELPOST},
    {BLT_SWITCH_FLAG, "-allevents", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_ALL},
    {BLT_SWITCH_FLAG, "-whenidle", Blt_Offset(NotifyData, mask), 0, 0, 
	TREE_NOTIFY_WHENIDLE},
    {BLT_SWITCH_END, NULL, 0, 0}
};

static Blt_SwitchParseProc StringToChild;
#define INSERT_BEFORE	(ClientData)0
#define INSERT_AFTER	(ClientData)1
static Blt_SwitchCustom beforeSwitch =
{
    StringToChild, (Blt_SwitchFreeProc *)NULL, INSERT_BEFORE,
};
static Blt_SwitchCustom afterSwitch =
{
    StringToChild, (Blt_SwitchFreeProc *)NULL, INSERT_AFTER,
};

typedef struct {
    char *label;
    int insertPos;
    int inode;
    Tcl_Obj *tags;
    Tcl_Obj *tags2;
    Tcl_Obj *dataPairs;
    Tcl_Obj *names;
    Tcl_Obj *values;
    Blt_TreeNode parent;
    int flags;
    int fixed;
} InsertData;

static Blt_SwitchSpec insertSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", Blt_Offset(InsertData, insertPos), 0, 
	&afterSwitch},
    /*{BLT_SWITCH_INT_NONNEGATIVE, "-at", Blt_Offset(InsertData, insertPos), 0},*/
    {BLT_SWITCH_CUSTOM, "-before", Blt_Offset(InsertData, insertPos), 0,
	&beforeSwitch},
    {BLT_SWITCH_OBJ, "-data", Blt_Offset(InsertData, dataPairs), 0},
    {BLT_SWITCH_BOOLEAN, "-fixed", Blt_Offset(InsertData, fixed), 0},
    {BLT_SWITCH_STRING, "-label", Blt_Offset(InsertData, label), 0},
    {BLT_SWITCH_OBJ, "-names", Blt_Offset(InsertData, names), 0},
    {BLT_SWITCH_INT_NONNEGATIVE, "-node", Blt_Offset(InsertData, inode), 0},
    {BLT_SWITCH_INT_NONNEGATIVE, "-pos", Blt_Offset(InsertData, insertPos), 0},
    {BLT_SWITCH_OBJ, "-pretags", Blt_Offset(InsertData, tags), 0},
    {BLT_SWITCH_OBJ, "-tags", Blt_Offset(InsertData, tags2), 0},
    {BLT_SWITCH_OBJ, "-values", Blt_Offset(InsertData, values), 0},
    {BLT_SWITCH_END, NULL, 0, 0}
};

#define PATTERN_EXACT	(1)
#define PATTERN_GLOB	(2)
#define PATTERN_MASK	(0x7)
#define PATTERN_NONE	(0)
#define PATTERN_REGEXP	(3)
#define PATTERN_INLIST	(4)
#define MATCH_INVERT		(1<<8)
#define MATCH_LEAFONLY		(1<<4)
#define MATCH_NOCASE		(1<<5)
#define MATCH_PATHNAME		(1<<6)
#define MATCH_NOTOP		(1<<7)
#define MATCH_TREEONLY		(1<<9)
#define MATCH_COUNT		(1<<10)
#define MATCH_FIXED		(1<<11)
#define MATCH_NOTFIXED		(1<<12)
#define MATCH_ARRAY		(1<<13)
#define MATCH_RELDEPTH		(1<<14)
#define MATCH_ISMODIFIED		(1<<15)
#define MATCH_STRICT		(1<<16)
#define MATCH_ISNULL		(1<<17)

typedef struct {
    TreeCmd *cmdPtr;		/* Tree to examine. */
    Tcl_Obj *listObjPtr;	/* List to accumulate the indices of 
				 * matching nodes. */
    Tcl_Obj **objv;		/* Command converted into an array of 
				 * Tcl_Obj's. */
    int objc;			/* Number of Tcl_Objs in above array. */

    int nMatches;		/* Current number of matches. */

    unsigned int flags;		/* See flags definitions above. */

    /* Integer options. */
    int maxMatches;		/* If > 0, stop after this many matches. */
    int depth;		/* Only nodes at this level . */
    int minDepth;		/* Only nodes at this level or higher. */
    int maxDepth;		/* If > 0, don't descend more than this
				 * many levels. */
    int order;			/* Order of search: Can be either
				 * TREE_PREORDER, TREE_POSTORDER,
				 * TREE_INORDER, TREE_BREADTHFIRST. */
    /* String options. */
   /* Blt_List patternList;	* List of patterns to compare with labels
				 * or values.  */
    char *addTag;		/* If non-NULL, tag added to selected nodes. */

    char **command;		/* Command split into a Tcl list. */
    char **cmdArgs;
    int argc;
    char *eval;		/* Command to eval with %T and %N substitutions. */

    Blt_List keyList;		/* List of key name patterns. */
    Tcl_Obj *withTag;
    Tcl_Obj *withoutTag;
    char *retKey;
    Blt_TreeNode startNode;
    Tcl_Obj *name;
    char *subKey;
    Tcl_RegExp *regPtr;
    Tcl_Obj *execVar, *execObj, *nodesObj;
    int keyCount, iskey;
} FindData;

static Blt_SwitchParseProc StringToOrder;
static Blt_SwitchCustom orderSwitch =
{
    StringToOrder, (Blt_SwitchFreeProc *)NULL, (ClientData)0,
};

static Blt_SwitchParseProc StringToPattern;
static Blt_SwitchFreeProc FreePatterns;

static Blt_SwitchCustom regexpSwitch =
{
    StringToPattern, FreePatterns, (ClientData)PATTERN_REGEXP,
};
static Blt_SwitchCustom globSwitch =
{
    StringToPattern, FreePatterns, (ClientData)PATTERN_GLOB,
};
static Blt_SwitchCustom exactSwitch =
{
    StringToPattern, FreePatterns, (ClientData)PATTERN_EXACT,
};

static Blt_SwitchParseProc FStringToNode;
static Blt_SwitchCustom fNodeSwitch =
{
    FStringToNode, (Blt_SwitchFreeProc *)NULL, (ClientData)0,
};


static Blt_SwitchSpec findSwitches[] = 
{
    {BLT_SWITCH_STRING, "-addtag", Blt_Offset(FindData, addTag), 0},
    {BLT_SWITCH_STRING, "-column", Blt_Offset(FindData, subKey), 0},
    {BLT_SWITCH_LIST, "-cmdargs", Blt_Offset(FindData, cmdArgs), 0},
    {BLT_SWITCH_LIST, "-command", Blt_Offset(FindData, command), 0},
    {BLT_SWITCH_FLAG, "-count", Blt_Offset(FindData, flags), 0, 0,
       MATCH_COUNT},
    {BLT_SWITCH_INT_NONNEGATIVE, "-depth", Blt_Offset(FindData, depth), 0},
    /*{BLT_SWITCH_CUSTOM, "-exact", Blt_Offset(FindData, patternList), 0,
        &exactSwitch}, */
    {BLT_SWITCH_FLAG, "-exact", Blt_Offset(FindData, flags), 0, 0, 
	PATTERN_EXACT},
    {BLT_SWITCH_OBJ, "-exec", Blt_Offset(FindData, execObj), 0},
    /*{BLT_SWITCH_CUSTOM, "-glob", Blt_Offset(FindData, patternList), 0, 
	&globSwitch},*/
    {BLT_SWITCH_FLAG, "-isarray", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_ARRAY},
    {BLT_SWITCH_FLAG, "-isempty", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_ISNULL},
    {BLT_SWITCH_FLAG, "-isfixed", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_FIXED},
    {BLT_SWITCH_FLAG, "-ismodified", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_ISMODIFIED},
    {BLT_SWITCH_FLAG, "-isnotfixed", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_NOTFIXED},
    {BLT_SWITCH_FLAG, "-glob", Blt_Offset(FindData, flags), 0, 0, 
	PATTERN_GLOB},
    {BLT_SWITCH_FLAG, "-invert", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_INVERT},
    {BLT_SWITCH_INT_NONNEGATIVE, "-keycount", Blt_Offset(FindData, keyCount), 0},
    {BLT_SWITCH_CUSTOM, "-key", Blt_Offset(FindData, keyList), 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyexact", Blt_Offset(FindData, keyList), 0, 
	&exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyglob", Blt_Offset(FindData, keyList), 0, 
	&globSwitch},
    {BLT_SWITCH_CUSTOM, "-keyregexp", Blt_Offset(FindData, keyList), 0, 
	&regexpSwitch},
    {BLT_SWITCH_FLAG, "-isleaf", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_LEAFONLY},
    {BLT_SWITCH_FLAG, "-istree", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_TREEONLY},
    {BLT_SWITCH_INT_NONNEGATIVE, "-limit", Blt_Offset(FindData, maxMatches), 0},
    {BLT_SWITCH_INT_NONNEGATIVE, "-maxdepth", Blt_Offset(FindData, maxDepth), 0},
    {BLT_SWITCH_INT_NONNEGATIVE, "-mindepth", Blt_Offset(FindData, minDepth), 0},
    {BLT_SWITCH_OBJ, "-name", Blt_Offset(FindData, name), 0},
    {BLT_SWITCH_FLAG, "-nocase", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_NOCASE},
    {BLT_SWITCH_OBJ, "-nodes", Blt_Offset(FindData, nodesObj), 0},
    {BLT_SWITCH_FLAG, "-notop", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_NOTOP},
    {BLT_SWITCH_CUSTOM, "-order", Blt_Offset(FindData, order), 0, &orderSwitch},
   /* {BLT_SWITCH_CUSTOM, "-regexp", Blt_Offset(FindData, patternList), 0, 
	&regexpSwitch}, */
    {BLT_SWITCH_FLAG, "-regexp", Blt_Offset(FindData, flags), 0, 0, 
	PATTERN_REGEXP},
    {BLT_SWITCH_FLAG, "-inlist", Blt_Offset(FindData, flags), 0, 0, 
	PATTERN_INLIST},
    {BLT_SWITCH_FLAG, "-reldepth", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_RELDEPTH},
    {BLT_SWITCH_STRING, "-return", Blt_Offset(FindData, retKey), 0},
    {BLT_SWITCH_FLAG, "-strict", Blt_Offset(FindData, flags), 0, 0, 
        MATCH_STRICT},
    {BLT_SWITCH_CUSTOM, "-top", Blt_Offset(FindData, startNode), 0, &fNodeSwitch},
    {BLT_SWITCH_FLAG, "-usepath", Blt_Offset(FindData, flags), 0, 0, 
	MATCH_PATHNAME},
    {BLT_SWITCH_OBJ, "-var", Blt_Offset(FindData, execVar), 0},
    {BLT_SWITCH_OBJ, "-withtag", Blt_Offset(FindData, withTag), 0},
    {BLT_SWITCH_OBJ, "-withouttag", Blt_Offset(FindData, withoutTag), 0},
    {BLT_SWITCH_END, NULL, 0, 0}
};

static Blt_SwitchParseProc StringToNode;
static Blt_SwitchCustom nodeSwitch =
{
    StringToNode, (Blt_SwitchFreeProc *)NULL, (ClientData)0,
    };


typedef struct {
    TreeCmd *cmdPtr;		/* Tree to move nodes. */
    Blt_TreeNode node;
    int movePos;
} MoveData;

static Blt_SwitchSpec moveSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", Blt_Offset(MoveData, node), 0, &nodeSwitch},
    {BLT_SWITCH_CUSTOM, "-before", Blt_Offset(MoveData, node), 0, &nodeSwitch},
    {BLT_SWITCH_OBJ, "-pos", Blt_Offset(MoveData, movePos), 0},
    {BLT_SWITCH_END, NULL, 0, 0}
};

typedef struct {
    Blt_TreeNode srcNode, destNode;
    Blt_Tree srcTree, destTree;
    TreeCmd *srcPtr, *destPtr;
    unsigned int flags;
    char *label;
} CopyData;

#define COPY_RECURSE	(1<<0)
#define COPY_TAGS	(1<<1)
#define COPY_OVERWRITE	(1<<2)
#define COPY_REVERSE (1<<3)

static Blt_SwitchSpec copySwitches[] = 
{
    {BLT_SWITCH_FLAG, "-reverse", Blt_Offset(CopyData, flags), 0, 0, 
	COPY_REVERSE},
    {BLT_SWITCH_STRING, "-label", Blt_Offset(CopyData, label), 0},
    {BLT_SWITCH_FLAG, "-recurse", Blt_Offset(CopyData, flags), 0, 0, 
	COPY_RECURSE},
    {BLT_SWITCH_FLAG, "-tags", Blt_Offset(CopyData, flags), 0, 0, 
	COPY_TAGS},
    {BLT_SWITCH_FLAG, "-overwrite", Blt_Offset(CopyData, flags), 0, 0, 
	COPY_OVERWRITE},
    {BLT_SWITCH_END, NULL, 0, 0}
};

typedef struct {
    TreeCmd *cmdPtr;		/* Tree to examine. */
    Tcl_Obj **preObjv;		/* Command converted into an array of 
				 * Tcl_Obj's. */
    int preObjc;		/* Number of Tcl_Objs in above array. */

    Tcl_Obj **postObjv;		/* Command converted into an array of 
				 * Tcl_Obj's. */
    int postObjc;		/* Number of Tcl_Objs in above array. */

    unsigned int flags;		/* See flags definitions above. */

    int maxDepth;		/* If > 0, don't descend more than this
				 * many levels. */
    /* String options. */
    Blt_List patternList;	/* List of label or value patterns. */
    char **preCmd;		/* Pre-command split into a Tcl list. */
    char **postCmd;		/* Post-command split into a Tcl list. */

    Blt_List keyList;		/* List of key-name patterns. */
    char *withTag;
} ApplyData;

static Blt_SwitchSpec applySwitches[] = 
{
    {BLT_SWITCH_LIST, "-precommand", Blt_Offset(ApplyData, preCmd), 0},
    {BLT_SWITCH_LIST, "-postcommand", Blt_Offset(ApplyData, postCmd), 0},
    {BLT_SWITCH_INT_NONNEGATIVE, "-depth", Blt_Offset(ApplyData, maxDepth), 0},
    {BLT_SWITCH_CUSTOM, "-exact", Blt_Offset(ApplyData, patternList), 0,
	&exactSwitch},
    {BLT_SWITCH_CUSTOM, "-glob", Blt_Offset(ApplyData, patternList), 0, 
	&globSwitch},
    {BLT_SWITCH_FLAG, "-invert", Blt_Offset(ApplyData, flags), 0, 0, 
	MATCH_INVERT},
    {BLT_SWITCH_CUSTOM, "-key", Blt_Offset(ApplyData, keyList), 0, 
	&exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyexact", Blt_Offset(ApplyData, keyList), 0, 
	&exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyglob", Blt_Offset(ApplyData, keyList), 0, 
	&globSwitch},
    {BLT_SWITCH_CUSTOM, "-keyregexp", Blt_Offset(ApplyData, keyList), 0, 
	&regexpSwitch},
    {BLT_SWITCH_FLAG, "-isleaf", Blt_Offset(ApplyData, flags), 0, 0, 
	MATCH_LEAFONLY},
    {BLT_SWITCH_FLAG, "-istree", Blt_Offset(ApplyData, flags), 0, 0, 
	MATCH_TREEONLY},
    {BLT_SWITCH_FLAG, "-nocase", Blt_Offset(ApplyData, flags), 0, 0, 
	MATCH_NOCASE},
    {BLT_SWITCH_CUSTOM, "-regexp", Blt_Offset(ApplyData, patternList), 0,
	&regexpSwitch},
    {BLT_SWITCH_STRING, "-tag", Blt_Offset(ApplyData, withTag), 0},
    {BLT_SWITCH_FLAG, "-usepath", Blt_Offset(ApplyData, flags), 0, 0, 
	MATCH_PATHNAME},
    {BLT_SWITCH_END, NULL, 0, 0}
};

typedef struct {
    unsigned int flags;
    Blt_HashTable idTable;
    Blt_TreeNode root;
    char *file;
    char *chan;
    char *tags, *notTags;
    Tcl_Obj *data, *keys, *notKeys, **kobjv, **nobjv;
    Tcl_Obj **tobjv, *addTags;
    int tobjc, kobjc, nobjc;
    Blt_HashTable tagTable;
} RestoreData;

#define RESTORE_NO_TAGS		(1<<0)
#define RESTORE_OVERWRITE	(1<<1)
#define RESTORE_NO_PATH		(1<<2)

static Blt_SwitchSpec restoreSwitches[] = 
{
    {BLT_SWITCH_FLAG, "-notags", Blt_Offset(RestoreData, flags), 0, 0, 
	RESTORE_NO_TAGS},
    {BLT_SWITCH_FLAG, "-overwrite", Blt_Offset(RestoreData, flags), 0, 0, 
	RESTORE_OVERWRITE},
    {BLT_SWITCH_OBJ, "-addtags", Blt_Offset(RestoreData, addTags), 0},
    {BLT_SWITCH_OBJ, "-data", Blt_Offset(RestoreData, data), 0},
    {BLT_SWITCH_STRING, "-channel", Blt_Offset(RestoreData, chan), 0},
    {BLT_SWITCH_STRING, "-file", Blt_Offset(RestoreData, file), 0},
    {BLT_SWITCH_OBJ, "-keys", Blt_Offset(RestoreData, keys), 0},
    {BLT_SWITCH_STRING, "-tag", Blt_Offset(RestoreData, tags), 0},
    {BLT_SWITCH_OBJ, "-skipkeys", Blt_Offset(RestoreData, notKeys), 0},
    {BLT_SWITCH_STRING, "-skiptag", Blt_Offset(RestoreData, notTags), 0},
    {BLT_SWITCH_END, NULL, 0, 0}
};

static Blt_SwitchSpec dumpSwitches[] = 
{
    {BLT_SWITCH_FLAG, "-notags", Blt_Offset(RestoreData, flags), 0, 0, 
	RESTORE_NO_TAGS},
    {BLT_SWITCH_FLAG, "-nopath", Blt_Offset(RestoreData, flags), 0, 0, 
	RESTORE_NO_PATH},
    {BLT_SWITCH_STRING, "-channel", Blt_Offset(RestoreData, chan), 0},
    {BLT_SWITCH_STRING, "-file", Blt_Offset(RestoreData, file), 0},
    {BLT_SWITCH_OBJ, "-keys", Blt_Offset(RestoreData, keys), 0},
    {BLT_SWITCH_STRING, "-tag", Blt_Offset(RestoreData, tags), 0},
    {BLT_SWITCH_OBJ, "-skipkeys", Blt_Offset(RestoreData, notKeys), 0},
    {BLT_SWITCH_STRING, "-skiptag", Blt_Offset(RestoreData, notTags), 0},
    {BLT_SWITCH_END, NULL, 0, 0}
};
static Blt_SwitchSpec dumptagSwitches[] = 
{
    {BLT_SWITCH_FLAG, "-notags", Blt_Offset(RestoreData, flags), 0, 0, 
	RESTORE_NO_TAGS},
    {BLT_SWITCH_FLAG, "-nopath", Blt_Offset(RestoreData, flags), 0, 0, 
	RESTORE_NO_PATH},
    {BLT_SWITCH_OBJ, "-keys", Blt_Offset(RestoreData, keys), 0},
    {BLT_SWITCH_STRING, "-tag", Blt_Offset(RestoreData, tags), 0},
    {BLT_SWITCH_OBJ, "-skipkeys", Blt_Offset(RestoreData, notKeys), 0},
    {BLT_SWITCH_STRING, "-skiptag", Blt_Offset(RestoreData, notTags), 0},
    {BLT_SWITCH_END, NULL, 0, 0}
};

static Blt_SwitchParseProc StringToFormat;
static Blt_SwitchCustom formatSwitch =
{
    StringToFormat, (Blt_SwitchFreeProc *)NULL, (ClientData)0,
};

typedef struct {
    int sort;			/* If non-zero, sort the nodes.  */
    int withParent;		/* If non-zero, add the parent node id 
				 * to the output of the command.*/
    int withId;			/* If non-zero, echo the node id in the
				 * output of the command. */
} PositionData;

#define POSITION_SORTED		(1<<0)

static Blt_SwitchSpec positionSwitches[] = 
{
    {BLT_SWITCH_FLAG, "-sort", Blt_Offset(PositionData, sort), 0, 0,
       POSITION_SORTED},
    {BLT_SWITCH_CUSTOM, "-format", 0, 0, &formatSwitch},
    {BLT_SWITCH_END, NULL, 0, 0}
};


static Tcl_InterpDeleteProc TreeInterpDeleteProc;
static Blt_TreeApplyProc MatchNodeProc, SortApplyProc;
static Blt_TreeApplyProc ApplyNodeProc;
static Blt_TreeTraceProc TreeTraceProc;
static Tcl_CmdDeleteProc TreeInstDeleteProc;
static Blt_TreeCompareNodesProc CompareNodes;

static Tcl_ObjCmdProc TreeObjCmd;
static Tcl_ObjCmdProc CompareDictionaryCmd;
#ifdef BLT_USE_UTIL_EXIT  
static Tcl_ObjCmdProc ExitCmd;
#endif
static Blt_TreeNotifyEventProc TreeEventProc;

static int GetNode _ANSI_ARGS_((TreeCmd *cmdPtr, Tcl_Obj *objPtr, 
	Blt_TreeNode *nodePtr));

static int nLines;



/*
 *----------------------------------------------------------------------
 *
 * StringToChild --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToChild(
    ClientData clientData,	/* Flag indicating if the node is
				 * considered before or after the
				 * insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    char *switchName,		/* Not used. */
    char *string,		/* String representation */
    char *record,		/* Structure record */
    int offset)			/* Offset to field in structure */
{
    InsertData *dataPtr = (InsertData *)record;
    Blt_TreeNode node;
    
    node = Blt_TreeFindChild(dataPtr->parent, string);
    if (node == NULL) {
	Tcl_AppendResult(interp, "can't find a child named \"", string, 
		 "\" in \"", Blt_TreeNodeLabel(dataPtr->parent), "\"",
		 (char *)NULL);	 
	return TCL_ERROR;
    }			  
    dataPtr->insertPos = Blt_TreeNodeDegree(node);
    if (clientData == INSERT_AFTER) {
	dataPtr->insertPos++;
    } 
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringToNode --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToNode(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    char *switchName,		/* Not used. */
    char *string,		/* String representation */
    char *record,		/* Structure record */
    int offset)			/* Offset to field in structure */
{
    MoveData *dataPtr = (MoveData *)record;
    Blt_TreeNode node;
    Tcl_Obj *objPtr;
    TreeCmd *cmdPtr = dataPtr->cmdPtr;
    int result;

    objPtr = Tcl_NewStringObj(string, -1);
    result = GetNode(cmdPtr, objPtr, &node);
    Tcl_DecrRefCount(objPtr);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    dataPtr->node = node;
    return TCL_OK;
}

static int
FStringToNode(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    char *switchName,		/* Not used. */
    char *string,		/* String representation */
    char *record,		/* Structure record */
    int offset)			/* Offset to field in structure */
{
    FindData *dataPtr = (FindData *)record;
    Blt_TreeNode node;
    Tcl_Obj *objPtr;
    TreeCmd *cmdPtr = dataPtr->cmdPtr;
    int result;

    objPtr = Tcl_NewStringObj(string, -1);
    result = GetNode(cmdPtr, objPtr, &node);
    Tcl_DecrRefCount(objPtr);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    dataPtr->startNode = node;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * StringToOrder --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToOrder(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    char *switchName,		/* Not used. */
    char *string,		/* String representation */
    char *record,		/* Structure record */
    int offset)			/* Offset to field in structure */
{
    int *orderPtr = (int *)(record + offset);
    char c;

    c = string[0];
    if ((c == 'b') && (strcmp(string, "breadthfirst") == 0)) {
	*orderPtr = TREE_BREADTHFIRST;
    } else if ((c == 'i') && (strcmp(string, "inorder") == 0)) {
	*orderPtr = TREE_INORDER;
    } else if ((c == 'p') && (strcmp(string, "preorder") == 0)) {
	*orderPtr = TREE_PREORDER;
    } else if ((c == 'p') && (strcmp(string, "postorder") == 0)) {
	*orderPtr = TREE_POSTORDER;
    } else {
	Tcl_AppendResult(interp, "bad order \"", string, 
		 "\": should be breadthfirst, inorder, preorder, or postorder",
		 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringToPattern --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToPattern(
    ClientData clientData,	/* Flag indicating type of pattern. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    char *switchName,		/* Not used. */
    char *string,		/* String representation */
    char *record,		/* Structure record */
    int offset)			/* Offset to field in structure */
{
    Blt_List *listPtr = (Blt_List *)(record + offset);

    if (*listPtr == NULL) {
	*listPtr = Blt_ListCreate(BLT_STRING_KEYS);
    }
    Blt_ListAppend(*listPtr, string, clientData);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreePatterns --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreePatterns(char *object)
{
    Blt_List list = (Blt_List)object;

    if (list != NULL) {
	Blt_ListDestroy(list);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * StringToFormat --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToFormat(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    char *switchName,		/* Not used. */
    char *string,		/* String representation */
    char *record,		/* Structure record */
    int offset)			/* Offset to field in structure */
{
    PositionData *dataPtr = (PositionData *)record;

    if (strcmp(string, "position") == 0) {
	dataPtr->withParent = FALSE;
	dataPtr->withId = FALSE;
    } else if (strcmp(string, "id+position") == 0) {
	dataPtr->withParent = FALSE;
	dataPtr->withId = TRUE;
    } else if (strcmp(string, "parent-at-position") == 0) {
	dataPtr->withParent = TRUE;
	dataPtr->withId = FALSE;
    } else if (strcmp(string, "id+parent-at-position") == 0) {
	dataPtr->withParent = TRUE;
	dataPtr->withId  = TRUE;
    } else {
	Tcl_AppendResult(interp, "bad format \"", string, 
 "\": should be position, parent-at-position, id+position, or id+parent-at-position",
		 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTreeCmdInterpData --
 *
 *---------------------------------------------------------------------- 
 */
static TreeCmdInterpData *
GetTreeCmdInterpData(Tcl_Interp *interp)
{
    TreeCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (TreeCmdInterpData *)
	Tcl_GetAssocData(interp, TREE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_Calloc(1, sizeof(TreeCmdInterpData));
	assert(dataPtr);
	dataPtr->interp = interp;
	Tcl_SetAssocData(interp, TREE_THREAD_KEY, TreeInterpDeleteProc,
		 dataPtr);
	Blt_InitHashTable(&dataPtr->treeTable, BLT_ONE_WORD_KEYS);
    }
    return dataPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTreeCmd --
 *
 *	Find the tree command associated with the Tcl command "string".
 *	
 *	We have to do multiple lookups to get this right.  
 *
 *	The first step is to generate a canonical command name.  If an
 *	unqualified command name (i.e. no namespace qualifier) is
 *	given, we should search first the current namespace and then
 *	the global one.  Most Tcl commands (like Tcl_GetCmdInfo) look
 *	only at the global namespace.
 *
 *	Next check if the string is 
 *		a) a Tcl command and 
 *		b) really is a command for a tree object.  
 *	Tcl_GetCommandInfo will get us the objClientData field that 
 *	should be a cmdPtr.  We can verify that by searching our hashtable 
 *	of cmdPtr addresses.
 *
 * Results:
 *	A pointer to the tree command.  If no associated tree command
 *	can be found, NULL is returned.  It's up to the calling routines
 *	to generate an error message.
 *
 *---------------------------------------------------------------------- 
 */
static TreeCmd *
GetTreeCmd(
    TreeCmdInterpData *dataPtr, 
    Tcl_Interp *interp, 
    CONST char *string)
{
    CONST char *name;
    Tcl_Namespace *nsPtr;
    Tcl_CmdInfo cmdInfo;
    Blt_HashEntry *hPtr;
    Tcl_DString dString;
    char *treeName;
    int result;

    /* Put apart the tree name and put is back together in a standard
     * format. */
    if (Blt_ParseQualifiedName(interp, string, &nsPtr, &name) != TCL_OK) {
	return NULL;		/* No such parent namespace. */
    }
    if (nsPtr == NULL) {
	nsPtr = Tcl_GetCurrentNamespace(interp);
    }
    Tcl_DStringInit(&dString);
    /* Rebuild the fully qualified name. */
    treeName = Blt_GetQualifiedName(nsPtr, name, &dString);
    result = Tcl_GetCommandInfo(interp, treeName, &cmdInfo);
    Tcl_DStringFree(&dString);

    if (!result) {
	return NULL;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->treeTable, 
			     (char *)(cmdInfo.objClientData));
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static Blt_TreeNode
MaxNode(Blt_Tree tree) {
    Blt_TreeNode node, root, mnode;
    int max = 0;
    
    mnode = root = Blt_TreeRootNode(tree);
    for (node = root; node != NULL; node = Blt_TreeNextNode(root, node)) {
        if (node->inode > max) {
            max = node->inode;
            mnode = node;
        }
    }
    return mnode;
}


static Blt_TreeNode 
ParseModifiers(
    Tcl_Interp *interp,
    Blt_Tree tree,
    Blt_TreeNode node,
    char *modifiers)
{
    char *p, *np;

    p = modifiers;
    do {
	p += 2;			/* Skip the initial "->" */
	np = strstr(p, "->");
	if (np != NULL) {
	    *np = '\0';
	}
	if ((*p == 'p') && (strcmp(p, "parentnode") == 0)) {
	    node = Blt_TreeNodeParent(node);
	} else if ((*p == 'f') && (strcmp(p, "firstchild") == 0)) {
	    node = Blt_TreeFirstChild(node);
	} else if ((*p == 'l') && (strcmp(p, "lastchild") == 0)) {
	    node = Blt_TreeLastChild(node);
	} else if ((*p == 'n') && (strcmp(p, "nextnode") == 0)) {
	    node = Blt_TreeNextNode(Blt_TreeRootNode(tree), node);
	} else if ((*p == 'n') && (strcmp(p, "nextsibling") == 0)) {
	    node = Blt_TreeNextSibling(node);
	} else if ((*p == 'p') && (strcmp(p, "prevnode") == 0)) {
	    node = Blt_TreePrevNode(Blt_TreeRootNode(tree), node);
	} else if ((*p == 'p') && (strcmp(p, "prevsibling") == 0)) {
	    node = Blt_TreePrevSibling(node);
	} else if ((*p == 'm') && (strcmp(p, "maxnode") == 0)) {
	    node = MaxNode(tree);
	/* } else if (isdigit(UCHAR(*p))) {
	    int inode;
	    
	    if (Tcl_GetInt(interp, p, &inode) != TCL_OK) {
		node = NULL;
	    } else {
		node = Blt_TreeGetNode(tree, inode);
	    }*/
	} else {
	    char *endp;

	    if (np != NULL) {
		endp = np - 1;
	    } else {
		endp = p + strlen(p) - 1;
	    }
            if ((*p == '\'') && (*endp == '\'')) {
                *endp = '\0';
                node = Blt_TreeFindChild(node, p + 1);
                *endp = '\'';

	    } else if ((*p == '"') && (*endp == '"')) {
		*endp = '\0';
		node = Blt_TreeFindChild(node, p + 1);
		*endp = '"';
	    } else {
		node = Blt_TreeFindChild(node, p);
	    }		
	}
	if (node == NULL) {
	    goto error;
	}
	if (np != NULL) {
	    *np = '-';		/* Repair the string */
	}
	p = np;
    } while (np != NULL);
    return node;
 error:
    if (np != NULL) {
	*np = '-';		/* Repair the string */
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * GetForeignNode --
 *
 *---------------------------------------------------------------------- 
 */
static int 
GetForeignNode(
    Tcl_Interp *interp,
    Blt_Tree tree,
    Tcl_Obj *objPtr,
    Blt_TreeNode *nodePtr)
{
    char c;
    Blt_TreeNode node;
    char *string;
    char *p;

    string = Tcl_GetString(objPtr);
    c = string[0];

    /* 
     * Check if modifiers are present.
     */
    p = strstr(string, "->");
    if (isdigit(UCHAR(c))) {
	int inode;

	if (p != NULL) {
	    char save;
	    int result;

	    save = *p;
	    *p = '\0';
	    result = Tcl_GetInt(interp, string, &inode);
	    *p = save;
	    if (result != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    if (Tcl_GetIntFromObj(interp, objPtr, &inode) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	node = Blt_TreeGetNode(tree, inode);
	if (p != NULL) {
	    node = ParseModifiers(interp, tree, node, p);
	}
	if (node != NULL) {
	    *nodePtr = node;
	    return TCL_OK;
	}
    }
    Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in ",
	 Blt_TreeName(tree), (char *)NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetNode --
 *
 *---------------------------------------------------------------------- 
 */
static int 
GetNode(TreeCmd *cmdPtr, Tcl_Obj *objPtr, Blt_TreeNode *nodePtr)
{
    Tcl_Interp *interp = cmdPtr->interp;
    Blt_Tree tree = cmdPtr->tree;
    char c;
    Blt_TreeNode node;
    char *string;
    char *p;
    int inode;

    string = Tcl_GetString(objPtr);
    c = string[0];

    /* 
     * Check if modifiers are present.
     */
    p = strstr(string, "->");
    if (isdigit(UCHAR(c))) {

	if (p != NULL) {
	    char save;
	    int result;

	    save = *p;
	    *p = '\0';
	    result = Tcl_GetInt(interp, string, &inode);
	    *p = save;
	    if (result != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    if (Tcl_GetIntFromObj(interp, objPtr, &inode) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	node = Blt_TreeGetNode(tree, inode);
    }  else if (cmdPtr != NULL) {
	char save;

	save = '\0';		/* Suppress compiler warning. */
	if (p != NULL) {
	    save = *p;
	    *p = '\0';
	}
	if ((strcmp(string, "all") == 0 )) {
	    if (Blt_TreeSize(Blt_TreeRootNode(tree)) > 1) {
		Tcl_AppendResult(interp, "more than one node tagged as \"", 
				 string, "\"", (char *)NULL);
		if (p != NULL) {
		    *p = save;
		}
		return TCL_ERROR;
	    }
	    node = Blt_TreeRootNode(tree);
	} else if ((strcmp(string, "all") == 0 )
	   || (strcmp(string, "rootchildren") == 0)) {
	    if (Blt_TreeSize(Blt_TreeRootNode(tree)) > 2) {
		Tcl_AppendResult(interp, "more than one node tagged as \"", 
				 string, "\"", (char *)NULL);
		if (p != NULL) {
		    *p = save;
		}
		return TCL_ERROR;
	    }
	    node = Blt_TreeRootNode(tree);
	} else if (strcmp(string, "root") == 0) {
	    node = Blt_TreeRootNode(tree);
	} else {
	    Blt_HashTable *tablePtr;
	    Blt_HashSearch cursor;
	    Blt_HashEntry *hPtr;
	    int result;

	    node = NULL;
	    result = TCL_ERROR;
	    tablePtr = Blt_TreeTagHashTable(cmdPtr->tree, string);
	    if (tablePtr == NULL) {
		Tcl_AppendResult(interp, "can't find tag or id \"", string, 
			"\" in ", Blt_TreeName(cmdPtr->tree), (char *)NULL);
	    } else if (tablePtr->numEntries > 1) {
		Tcl_AppendResult(interp, "more than one node tagged as \"", 
			 string, "\"", (char *)NULL);
	    } else if (tablePtr->numEntries <= 0) {
		Tcl_AppendResult(interp, "there is no node tagged as \"", 
			 string, "\"", (char *)NULL);
	    } else {
		hPtr = Blt_FirstHashEntry(tablePtr, &cursor);
		node = Blt_GetHashValue(hPtr);
		result = TCL_OK;
	    }
	    if (result == TCL_ERROR) {
		if (p != NULL) {
		    *p = save;
		}
		return TCL_ERROR;
	    }
	}
	if (p != NULL) {
	    *p = save;
	}
    }
    if (node != NULL) {
	if (p != NULL) {
	    node = ParseModifiers(interp, tree, node, p);
	    if (node == NULL) {
		goto error;
	    }
	}
	*nodePtr = node;
	return TCL_OK;
    }
 error:
    Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in ",
		 Blt_TreeName(tree), (char *)NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * FirstTaggedNode --
 *
 *	Returns the id of the first node tagged by the given tag in
 *	objPtr.  It basically hides much of the cumbersome special
 *	case details.  For example, the special tags "root" and "all"
 *      "nonroot"
 *	always exist, so they don't have entries in the tag hashtable.
 *	If it's a hashed tag (not "root" or "all"), we have to save
 *	the place of where we are in the table for the next call to
 *	NextTaggedNode.
 *
 *---------------------------------------------------------------------- 
 */
static Blt_TreeNode
FirstTaggedNode(TagSearch *cursorPtr)
{
    return cursorPtr->node;
}

static int
FindTaggedNodes(
    Tcl_Interp *interp,
    TreeCmd *cmdPtr,
    Tcl_Obj *objPtr,
    TagSearch *cursorPtr)
{
    Blt_TreeNode node, root;
    char *string;
    
    memset(cursorPtr, 0, sizeof(*cursorPtr));
    cursorPtr->init = 1;
    node = NULL;

    root = Blt_TreeRootNode(cmdPtr->tree);
    string = Tcl_GetString(objPtr);
    cursorPtr->tagType = TAG_TYPE_NONE;
    cursorPtr->root = root;

    /* Process strings with modifiers or digits as simple ids, not
     * tags. */
    if (string[0] == 0) {
        cursorPtr->node = NULL;
        return TCL_OK;
    }
    if ((strstr(string, "->") != NULL)) {
	if (GetNode(cmdPtr, objPtr, &node) != TCL_OK) {
	    return TCL_ERROR;
	}
        cursorPtr->node = node;
	return TCL_OK;
    }
    if (isdigit(UCHAR(*string))) {
        char *cp = string;
        int i, n;
        
        while (isdigit(UCHAR(*cp)) && *cp != 0) {
            cp++;
        }
        if (*cp != 0) {
            if (Tcl_ListObjGetElements(interp, objPtr, &cursorPtr->objc,
                &cursorPtr->objv) != TCL_OK) {
                return TCL_ERROR;
            }
            for (i=0; i<cursorPtr->objc; i++) {
                if (Tcl_GetIntFromObj(interp, cursorPtr->objv[i], &n) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
            if (GetNode(cmdPtr, cursorPtr->objv[0], &node) != TCL_OK) {
                return TCL_ERROR;
            }
            Tcl_IncrRefCount(objPtr);
            cursorPtr->objPtr = objPtr;
            cursorPtr->cmdPtr = cmdPtr;
            cursorPtr->tagType = TAG_TYPE_LIST;
            cursorPtr->idx = 0;
            cursorPtr->node = node;
            return TCL_OK;
        }
        if (GetNode(cmdPtr, objPtr, &node) != TCL_OK) {
            return TCL_ERROR;
        }
        cursorPtr->node = node;
        return TCL_OK;
    }
    if (strcmp(string, "all") == 0) {
	cursorPtr->tagType = TAG_TYPE_ALL;
        cursorPtr->node = root;
        cursorPtr->inode = cursorPtr->node->inode;
        return TCL_OK;
    } else if (strcmp(string, "nonroot") == 0)  {
        cursorPtr->tagType = TAG_TYPE_ALL;
        cursorPtr->node = Blt_TreeNextNode(root, root);
        if (cursorPtr->node != NULL) {
            cursorPtr->inode = cursorPtr->node->inode;
        }
        return TCL_OK;
    } else if (strcmp(string, "root") == 0)  {
        cursorPtr->node = root;
        return TCL_OK;
    } else if (strcmp(string, "rootchildren") == 0)  {
        cursorPtr->tagType = TAG_TYPE_ROOTCHILDREN;
        cursorPtr->node = Blt_TreeNextNode(root, root);
        if (cursorPtr->node != NULL) {
            cursorPtr->inode = cursorPtr->node->inode;
        }
        return TCL_OK;
    } else {
	Blt_HashTable *tablePtr;
	
	tablePtr = Blt_TreeTagHashTable(cmdPtr->tree, string);
	if (tablePtr != NULL) {
	    Blt_HashEntry *hPtr;
	    
	    cursorPtr->tagType = TAG_TYPE_TAG;
	    hPtr = Blt_FirstHashEntry(tablePtr, &(cursorPtr->cursor)); 
	    if (hPtr == NULL) {
                cursorPtr->node = NULL;
                return TCL_OK;
            }
            cursorPtr->tPtr = Blt_TreeTagHashEntry(cmdPtr->tree, string);
	    Blt_TreeTagRefIncr(cursorPtr->tPtr);
	    node = Blt_GetHashValue(hPtr);
            cursorPtr->node = node;
            if (node) {
                cursorPtr->inode = node->inode;
            }
            return TCL_OK;
	}
    }
    Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in ", 
	Blt_TreeName(cmdPtr->tree), (char *)NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NextTaggedNode --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_TreeNode
NextTaggedNode(Blt_TreeNode node, TagSearch *cursorPtr)
{
    if (cursorPtr->cnt++ > 100000000) {
        /* Prevent infinite loop. */
        return NULL;
    }
    if (cursorPtr->tagType == TAG_TYPE_LIST) {
        if (++cursorPtr->idx >= cursorPtr->objc) {
            return NULL;
        }
        if (GetNode(cursorPtr->cmdPtr, cursorPtr->objv[cursorPtr->idx], &node) != TCL_OK) {
            return NULL;
        }
        return node;
    }
    if (cursorPtr->tagType == TAG_TYPE_TAG) {
	Blt_HashEntry *hPtr;

        if (cursorPtr->tPtr && cursorPtr->tPtr->refCount<=1) {
            /* Tag was deleted. */
            return NULL;
        }
	hPtr = Blt_NextHashEntry(&(cursorPtr->cursor));
	if (hPtr == NULL) {
	    return NULL;
	}
	return Blt_GetHashValue(hPtr);
    }
    if (cursorPtr->tagType == TAG_TYPE_ALL
        || cursorPtr->tagType == TAG_TYPE_ROOTCHILDREN) {
        if (node != cursorPtr->node) {
            fprintf(stderr, "node mismatch in nexttag");
        }
        /* If node is deleted/reallocated we terminate */
        if (Blt_TreeNodeDeleted(node) || node->inode != cursorPtr->node->inode) {
            return NULL;
        }
        if (cursorPtr->tagType == TAG_TYPE_ROOTCHILDREN) {
            cursorPtr->node = Blt_TreeNextSibling(node);
        } else {
            cursorPtr->node = Blt_TreeNextNode(cursorPtr->root, node);
        }
        if (cursorPtr->node != NULL) {
            cursorPtr->inode = cursorPtr->node->inode;
        }
        return cursorPtr->node;
    }
    return NULL;
}

/*
 * Release tagged searches.
 */
static void
DoneTaggedNodes(TagSearch *cursorPtr) {
    if (cursorPtr->init == 1) {
        cursorPtr->init = 0;
        if (cursorPtr->objPtr) {
            Tcl_DecrRefCount(cursorPtr->objPtr);
            cursorPtr->objPtr = NULL;
        }
        if (cursorPtr->tPtr) {
            Blt_TreeTagRefDecr(cursorPtr->tPtr);
            cursorPtr->tPtr = NULL;
        }
    }
}


static int
AddTag(TreeCmd *cmdPtr, Blt_TreeNode node, CONST char *tagName)
{
     return Blt_TreeAddTag(cmdPtr->tree, node, tagName);
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteNode --
 *
 *---------------------------------------------------------------------- 
 */
static void
DeleteNode(TreeCmd *cmdPtr, Blt_TreeNode node)
{
    Blt_TreeNode root;

    if (!Blt_TreeTagTableIsShared(cmdPtr->tree)) {
	Blt_TreeClearTags(cmdPtr->tree, node);
    }
    root = Blt_TreeRootNode(cmdPtr->tree);
    if (node == root) {
	Blt_TreeNode next;
	/* Don't delete the root node. Simply clean out the tree. */
	for (node = Blt_TreeFirstChild(node); node != NULL; node = next) {
	    next = Blt_TreeNextSibling(node);
	    Blt_TreeDeleteNode(cmdPtr->tree, node);
	}	    
    } else if (Blt_TreeIsAncestor(root, node)) {
	Blt_TreeDeleteNode(cmdPtr->tree, node);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetNodePath --
 *
 *---------------------------------------------------------------------- 
 */
static char *
GetNodePathStr(
    TreeCmd *cmdPtr,
    Blt_TreeNode root, 
    Blt_TreeNode node,
    int rootFlag,		/* If non-zero, indicates to include
				 * the root name in the path */
    Tcl_DString *resultPtr,
    int lastOnly)
{
    char **nameArr;		/* Used to stack the component names. */
    char *staticSpace[64];
    register int i;
    int nLevels;

    nLevels = Blt_TreeNodeDepth(cmdPtr->tree, node) -
	Blt_TreeNodeDepth(cmdPtr->tree, root);
    if (rootFlag) {
	nLevels++;
    }
    if (nLevels > 64) {
	nameArr = Blt_Calloc(nLevels, sizeof(char *));
	assert(nameArr);
    } else {
	nameArr = staticSpace;
    }
    for (i = nLevels; i > 0; i--) {
	/* Save the name of each ancestor in the name array. 
	 * Note that we ignore the root. */
	if (lastOnly && i != nLevels) {
             nameArr[i - 1] = ".";
        } else {
             nameArr[i - 1] = Blt_TreeNodeLabel(node);
	}
	node = Blt_TreeNodeParent(node);
    }
    /* Append each the names in the array. */
    Tcl_DStringInit(resultPtr);
    for (i = 0; i < nLevels; i++) {
	Tcl_DStringAppendElement(resultPtr, nameArr[i]);
    }
    if (nameArr != staticSpace) {
	Blt_Free(nameArr);
    }
    return Tcl_DStringValue(resultPtr);
}

static char *
GetNodePath(
    TreeCmd *cmdPtr,
    Blt_TreeNode root, 
    Blt_TreeNode node,
    int rootFlag,		/* If non-zero, indicates to include
				 * the root name in the path */
    Tcl_DString *resultPtr)
{
    return GetNodePathStr(cmdPtr, root, node, rootFlag, resultPtr, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ParseNode5 --
 *
 *	Parses and creates a node based upon the first 3 fields of
 *	a five field entry.  This is the new restore file format.
 *
 *		parentId nodeId pathList dataList tagList
 *
 *	The purpose is to attempt to save and restore the node ids
 *	embedded in the restore file information.  The old format
 *	could not distinquish between two sibling nodes with the same
 *	label unless they were both leaves.  I'm trying to avoid
 *	dependencies upon labels.  
 *
 *	If you're starting from an empty tree, this obviously should
 *	work without a hitch.  We only need to map the file's root id
 *	to 0.  It's a little more complicated when adding node to an
 *	already full tree.  
 *
 *	First see if the node id isn't already in use.  Otherwise, map
 *	the node id (via a hashtable) to the real node. We'll need it
 *	later when subsequent entries refer to their parent id.
 *
 *	If a parent id is unknown (the restore file may be out of
 *	order), then follow plan B and use its path.
 *	
 *---------------------------------------------------------------------- 
 */
static Blt_TreeNode
ParseNode5(TreeCmd *cmdPtr, char **argv, RestoreData *dataPtr)
{
    Blt_HashEntry *hPtr;
    Blt_TreeNode node, parent;
    char **names;
    int nNames, isNew;
    int parentId, nodeId;

    if ((Tcl_GetInt(cmdPtr->interp, argv[0], &parentId) != TCL_OK) ||
	(Tcl_GetInt(cmdPtr->interp, argv[1], &nodeId) != TCL_OK) ||
	(Tcl_SplitList(cmdPtr->interp, argv[2], &nNames, &names) != TCL_OK)) {
	return NULL;
    }    

    if (parentId == -1) {	/* Dump marks root's parent as -1. */
	node = dataPtr->root;
	/* Create a mapping between the old id and the new node */
	hPtr = Blt_CreateHashEntry(&dataPtr->idTable, (char *)(intptr_t)nodeId, 
		   &isNew);
	Blt_SetHashValue(hPtr, node);
	Blt_TreeRelabelNode(cmdPtr->tree, node, names[0]);
    } else {
	/* 
	 * Check if the parent has been translated to another id.
	 * This can happen when there's a id collision with an
	 * existing node. 
	 */
	hPtr = Blt_FindHashEntry(&dataPtr->idTable, (char *)(intptr_t)parentId);
	if (hPtr != NULL) {
	    parent = Blt_GetHashValue(hPtr);
	} else {
	    /* Check if the id already exists in the tree. */
	    parent = Blt_TreeGetNode(cmdPtr->tree, parentId);
	    if (parent == NULL) {
		/* Parent id doesn't exist (partial restore?). 
		 * Plan B: Use the path to create/find the parent with 
		 *	   the requested parent id. */
		if (nNames > 1) {
		    int i;

		    for (i = 1; i < (nNames - 2); i++) {
			node = Blt_TreeFindChild(parent, names[i]);
			if (node == NULL) {
			    node = Blt_TreeCreateNode(cmdPtr->tree, parent, 
			      names[i], -1);
			}
			parent = node;
		    }
		    node = Blt_TreeFindChild(parent, names[nNames - 2]);
		    if (node == NULL) {
			node = Blt_TreeCreateNodeWithId(cmdPtr->tree, parent, 
				names[nNames - 2], parentId, -1);
		    }
		    parent = node;
		} else {
		    parent = dataPtr->root;
		}
	    }
	} 
	/* Check if old node id already in use. */
	node = NULL;
	if (dataPtr->flags & RESTORE_OVERWRITE &&
	    ((node = Blt_TreeFindChild(parent, names[nNames - 1])))) {
	    /* Create a mapping between the old id and the new node */
	    hPtr = Blt_CreateHashEntry(&dataPtr->idTable, (char *)(intptr_t)nodeId, 
				       &isNew);
	    Blt_SetHashValue(hPtr, node);
	}
	if (node == NULL) {
	    node = Blt_TreeGetNode(cmdPtr->tree, nodeId);
	    if (node != NULL) {
		node = Blt_TreeCreateNode(cmdPtr->tree, parent, 
					  names[nNames - 1], -1);
		/* Create a mapping between the old id and the new node */
		hPtr = Blt_CreateHashEntry(&dataPtr->idTable, (char *)(intptr_t)nodeId,
					   &isNew);
		Blt_SetHashValue(hPtr, node);
	    } else {
		/* Otherwise create a new node with the requested id. */
		node = Blt_TreeCreateNodeWithId(cmdPtr->tree, parent, 
						names[nNames - 1], nodeId, -1);
	    }
	}
    }
    Blt_Free(names);
    return node;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseNode3 --
 *
 *	Parses and creates a node based upon the first field of
 *	a three field entry.  This is the old restore file format.
 *
 *		pathList dataList tagList
 *
 *----------------------------------------------------------------------
 */
static Blt_TreeNode
ParseNode3(TreeCmd *cmdPtr, char **argv, RestoreData *dataPtr)
{
    Blt_TreeNode node, parent;
    char **names;
    int i;
    int nNames;
    
    if (Tcl_SplitList(cmdPtr->interp, argv[0], &nNames, &names) != TCL_OK) {
	return NULL;
    }
    node = parent = dataPtr->root;
    /*  Automatically create nodes as needed except for the last node.  */
    for (i = 0; i < (nNames - 1); i++) {
	node = Blt_TreeFindChild(parent, names[i]);
	if (node == NULL) {
	    node = Blt_TreeCreateNode(cmdPtr->tree, parent, names[i], -1);
	}
	parent = node;
    }
    if (nNames > 0) {
	/* 
	 * By default, create duplicate nodes (two sibling nodes with
	 * the same label), unless the -overwrite flag was set.
	 */
	node = NULL;
	if (dataPtr->flags & RESTORE_OVERWRITE) {
	    node = Blt_TreeFindChild(parent, names[i]);
	}
	if (node == NULL) {
	    node = Blt_TreeCreateNode(cmdPtr->tree, parent, names[i], -1);
	}
    }
    Blt_Free(names);
    return node;
}

static int
RestoreNode(TreeCmd *cmdPtr, int argc, char **argv, RestoreData *dataPtr)
{
    Blt_TreeNode node;
    Tcl_Obj *valueObjPtr;
    char **elemArr, *string, *key;
    int nElem, result;
    register int i;

    if ((argc != 3) && (argc != 5)) {
	Tcl_AppendResult(cmdPtr->interp, "line #", Blt_Itoa(nLines), 
		": wrong # elements in restore entry", (char *)NULL);
	return TCL_ERROR;
    }
    /* Parse the path name. */
    if (argc == 3) {
	node = ParseNode3(cmdPtr, argv, dataPtr);
	argv++;
    } else if (argc == 5) {
	node = ParseNode5(cmdPtr, argv, dataPtr);
	argv += 3;
    } else {
	Tcl_AppendResult(cmdPtr->interp, "line #", Blt_Itoa(nLines), 
		": wrong # elements in restore entry", (char *)NULL);
	return TCL_ERROR;
    }
    if (node == NULL) {
	return TCL_ERROR;
    }
    if (argv[0][0]) {
        /* Parse the key-value list. */
        if (Tcl_SplitList(cmdPtr->interp, argv[0], &nElem, &elemArr) != TCL_OK) {
            DeleteNode(cmdPtr, node);
            return TCL_ERROR;
        }
        if (nElem%2) {
            Tcl_AppendResult(cmdPtr->interp, "odd # of name/values in keys: ",
                argv[0], 0);
            Blt_Free(elemArr);
            DeleteNode(cmdPtr, node);
            return TCL_ERROR;
        }
        for (i = 0; i < nElem; i += 2) {
            int n, keep;
            key = elemArr[i];
            keep = 1;
            if (dataPtr->keys != NULL) {
                keep = 0;
                for (n=0; n<dataPtr->kobjc; n++) {
                    string = Tcl_GetString(dataPtr->kobjv[n]);
                    if (Tcl_StringMatch(key, string)==1) {
                        keep = 1;
                        break;
                    }
                }
            }
            if (dataPtr->notKeys != NULL) {
                for (n=0; n<dataPtr->nobjc; n++) {
                    string = Tcl_GetString(dataPtr->nobjv[n]);
                    if (Tcl_StringMatch(key, string)==1) {
                        keep = 0;
                        break;
                    }
                }
            }
            if (!keep) continue;
            
            if ((i + 1) < nElem) {
                valueObjPtr = Tcl_NewStringObj(elemArr[i + 1], -1);
            } else {
                valueObjPtr = Tcl_NewStringObj("",-1);
            }
            Tcl_IncrRefCount(valueObjPtr);
            result = Blt_TreeSetValue(cmdPtr->interp, cmdPtr->tree, node, 
                elemArr[i], valueObjPtr);
                Tcl_DecrRefCount(valueObjPtr);
                if (result != TCL_OK) {
                Blt_Free(elemArr);
                DeleteNode(cmdPtr, node);
                return TCL_ERROR;
            }
        }
        Blt_Free(elemArr);
    }
    if (argv[1][0] && (!(dataPtr->flags & RESTORE_NO_TAGS))) {
	/* Parse the tag list. */
	if (Tcl_SplitList(cmdPtr->interp, argv[1], &nElem, &elemArr) 
	    != TCL_OK) {
            DeleteNode(cmdPtr, node);
	    return TCL_ERROR;
	}
	for (i = 0; i < nElem; i++) {
             key = elemArr[i];
             if (dataPtr->tags != NULL && Tcl_StringMatch(key, dataPtr->tags)!=1) {
                 continue;
             }
             if (dataPtr->notTags != NULL && Tcl_StringMatch(key,dataPtr->notTags)==1) {
                 continue;
             }
             if (AddTag(cmdPtr, node, elemArr[i]) != TCL_OK) {
		Blt_Free(elemArr);
                DeleteNode(cmdPtr, node);
		return TCL_ERROR;
	    }
	}
	Blt_Free(elemArr);
    }
    for (i=0; i<dataPtr->tobjc; i++) {
        if (AddTag(cmdPtr, node, Tcl_GetString(dataPtr->tobjv[i])) != TCL_OK) {
            DeleteNode(cmdPtr, node);
            return TCL_ERROR;
        }
    }
    if (Blt_TreeInsertPost(cmdPtr->tree, node) == NULL) {
        DeleteNode(cmdPtr, node);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintNode --
 *
 *---------------------------------------------------------------------- 
 */
static void
PrintNode(
    TreeCmd *cmdPtr, 
    Blt_TreeNode root, 
    Blt_TreeNode node, 
    Tcl_DString *resultPtr,
    int tags,
    RestoreData *dataPtr
    )
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    char *pathName, *string;
    Tcl_DString dString;
    Tcl_Obj *valueObjPtr;
    register Blt_TreeKey key;
    Blt_TreeTagEntry *tPtr;
    Blt_TreeKeySearch keyIter;
    int keep, i;

    if (node == root) {
	Tcl_DStringAppendElement(resultPtr, "-1");
    } else {
	Blt_TreeNode parent;

	parent = Blt_TreeNodeParent(node);
	Tcl_DStringAppendElement(resultPtr, Blt_Itoa(Blt_TreeNodeId(parent)));
    }	
    Tcl_DStringAppendElement(resultPtr, Blt_Itoa(Blt_TreeNodeId(node)));

    pathName = GetNodePathStr(cmdPtr, root, node, TRUE, &dString,
        dataPtr->flags&RESTORE_NO_PATH);
    Tcl_DStringAppendElement(resultPtr, pathName);

    Tcl_DStringStartSublist(resultPtr);
    for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &keyIter); key != NULL; 
	 key = Blt_TreeNextKey(cmdPtr->tree, &keyIter)) {
	keep = 1;
        if (dataPtr->keys != NULL) {
            keep = 0;
            for (i=0; i<dataPtr->kobjc; i++) {
                string = Tcl_GetString(dataPtr->kobjv[i]);
                if (Tcl_StringMatch(key,string)==1) {
                    keep = 1;
                    break;
                }
            }
        }
        if (dataPtr->notKeys != NULL) {
            for (i=0; i<dataPtr->nobjc; i++) {
                string = Tcl_GetString(dataPtr->nobjv[i]);
                if (Tcl_StringMatch(key,string)==1) {
                    keep = 0;
                    break;
                }
            }
        }
        if (!keep) continue;
	if (Blt_TreeGetValueByKey((Tcl_Interp *)NULL, cmdPtr->tree, node, 
		key, &valueObjPtr) == TCL_OK) {
	    Tcl_DStringAppendElement(resultPtr, key);
	    Tcl_DStringAppendElement(resultPtr, Tcl_GetString(valueObjPtr));
	}
    }	    
    Tcl_DStringEndSublist(resultPtr);
    if (tags && dataPtr && dataPtr->tagTable.numEntries) {
        Tcl_DString *eStr;
        Blt_HashEntry *h2Ptr;
        
        eStr = NULL;
        h2Ptr = Blt_FindHashEntry(&dataPtr->tagTable, (char*)node);
        if (h2Ptr != NULL) {
            eStr = (Tcl_DString*)Blt_GetHashValue(h2Ptr);
        }
        Tcl_DStringAppendElement(resultPtr, eStr?Tcl_DStringValue(eStr):"");
    } else if (tags) {
        Tcl_DStringStartSublist(resultPtr);
        for (hPtr = Blt_TreeFirstTag(cmdPtr->tree, &cursor); hPtr != NULL; 
	    hPtr = Blt_NextHashEntry(&cursor)) {
            tPtr = Blt_GetHashValue(hPtr);
            if (Blt_FindHashEntry(&tPtr->nodeTable, (char *)node) != NULL) {
                Tcl_DStringAppendElement(resultPtr, tPtr->tagName);
            }
        }
        Tcl_DStringEndSublist(resultPtr);
    } else {
        Tcl_DStringAppendElement(resultPtr, "");
    }
    Tcl_DStringAppend(resultPtr, "\n", -1);
    Tcl_DStringFree(&dString);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintTraceFlags --
 *
 *---------------------------------------------------------------------- 
 */
static void
PrintTraceFlags(unsigned int flags, char *string)
{
    register char *p;

    p = string;
    if (flags & TREE_TRACE_READ) {
	*p++ = 'r';
    } 
    if (flags & TREE_TRACE_WRITE) {
	*p++ = 'w';
    } 
    if (flags & TREE_TRACE_UNSET) {
	*p++ = 'u';
    } 
    if (flags & TREE_TRACE_CREATE) {
	*p++ = 'c';
    } 
    if (flags & TREE_TRACE_EXISTS) {
        *p++ = 'e';
    } 
    if (flags & TREE_TRACE_TAGDELETE) {
        *p++ = 'd';
    } 
    if (flags & TREE_TRACE_TAGADD) {
        *p++ = 't';
    } 
    if (flags & TREE_TRACE_TAGMULTIPLE) {
        *p++ = 'm';
    } 
    *p = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * GetTraceFlags --
 *
 *---------------------------------------------------------------------- 
 */
static int
GetTraceFlags(char *string)
{
    register char *p;
    unsigned int flags;

    flags = 0;
    for (p = string; *p != '\0'; p++) {
	switch (*p) {
	case 'r':
	    flags |= TREE_TRACE_READ;
	    break;
	case 'w':
	    flags |= TREE_TRACE_WRITE;
	    break;
	case 'u':
	    flags |= TREE_TRACE_UNSET;
	    break;
	case 'c':
	    flags |= TREE_TRACE_CREATE;
	    break;
	case 'e':
	    flags |= TREE_TRACE_EXISTS;
	    break;
	case 't':
	    flags |= TREE_TRACE_TAGADD;
	    break;
	case 'm':
	    flags |= TREE_TRACE_TAGMULTIPLE;
	    break;
	case 'd':
	    flags |= TREE_TRACE_TAGDELETE;
	    break;
	default:
	    return -1;
	}
    }
    return flags;
}

/*
 *----------------------------------------------------------------------
 *
 * SetValues --
 *
 *---------------------------------------------------------------------- 
 */
static int
SetValues(TreeCmd *cmdPtr, Blt_TreeNode node, int objc, Tcl_Obj *CONST *objv)
{
    register int i;
    char *string;

    for (i = 0; i < objc; i += 2) {
	string = Tcl_GetString(objv[i]);
	if ((i + 1) == objc) {
	    Tcl_AppendResult(cmdPtr->interp, "missing value for field \"", 
		string, "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Blt_TreeSetValue(cmdPtr->interp, cmdPtr->tree, node, string, 
			     objv[i + 1]) != TCL_OK) {
	    return TCL_ERROR;
	}
     }
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * UpdateValues --
 *
 *---------------------------------------------------------------------- 
 */
static int
UpdateValues(TreeCmd *cmdPtr, Blt_TreeNode node, int objc, Tcl_Obj *CONST *objv)
{
    register int i;
    char *string;
    int result = TCL_OK;
    Tcl_DString dStr;
    Tcl_Interp *interp = cmdPtr->interp;

    if (objc % 2) {
        Tcl_AppendResult(interp, "odd # values", (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_DStringInit(&dStr);
    for (i = 0; i < objc; i += 2) {
	string = Tcl_GetString(objv[i]);
	if (Blt_TreeUpdateValue(interp, cmdPtr->tree, node, string, 
			     objv[i + 1]) != TCL_OK) {
            Tcl_DStringAppend(&dStr, Tcl_GetStringResult(interp), -1);
            Tcl_DStringAppend(&dStr, " ", -1);
            Tcl_ResetResult(interp);
	    result = TCL_ERROR;
	}
    }
    if (result != TCL_OK) {
        Tcl_DStringResult(interp, &dStr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * UnsetValues --
 *
 *---------------------------------------------------------------------- 
 */
static int
UnsetValues(TreeCmd *cmdPtr, Blt_TreeNode node, int objc, Tcl_Obj *CONST *objv)
{
    if (objc == 0) {
	Blt_TreeKey key;
	Blt_TreeKeySearch cursor;
	
	int cnt, n;
	/* Careful, trace could add key back on end. */
	cnt = Blt_TreeCountKeys(cmdPtr->tree, node);
        n = 0;
	for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &cursor);
	    key != NULL && n<=cnt;
	    n++, key = Blt_TreeNextKey(cmdPtr->tree, &cursor)) {
	    if (Blt_TreeUnsetValueByKey(cmdPtr->interp, cmdPtr->tree, node, 
			key) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    } else {
	register int i;

	for (i = 0; i < objc; i ++) {
	    if (Blt_TreeUnsetValue(cmdPtr->interp, cmdPtr->tree, node, 
		Tcl_GetString(objv[i])) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

static char *
GetFirstKey(Blt_List patternList)
{
    Blt_ListNode node;
    char *pattern;

    for (node = Blt_ListFirstNode(patternList); node != NULL; 
	node = Blt_ListNextNode(node)) {
		
	pattern = (char *)Blt_ListGetKey(node);
	return pattern;
    }
    return NULL;
}

static int
ComparePatternList(Blt_List patternList, char *string, int nocase)
{
    Blt_ListNode node;
    int result, type;
    char *pattern;

    result = FALSE;
    for (node = Blt_ListFirstNode(patternList); node != NULL; 
	node = Blt_ListNextNode(node)) {
		
	type = (intptr_t)Blt_ListGetValue(node);
	pattern = (char *)Blt_ListGetKey(node);
	switch (type) {
	case 0:
	case PATTERN_EXACT:
	    if (nocase) {
                 result = (strcasecmp(string, pattern) == 0);
             } else {
                result = (strcmp(string, pattern) == 0);
            }
	    break;
	    
	case PATTERN_GLOB:
	    result = (Tcl_StringCaseMatch(string, pattern, nocase) == 1);
	    break;
		    
	case PATTERN_REGEXP:
            if (nocase) {
                string = Blt_Strdup(string);
                strtolower(string);
            }
	    result = (Tcl_RegExpMatch((Tcl_Interp *)NULL, string, pattern) == 1); 
            if (nocase) {
                Blt_Free(string);
            }
	    break;
	}
    }
    return result;
}

static int
ComparePattern(FindData *findData, char *string)
{
    int result, type, nocase;
    char *pattern;
    int objc, i;
    Tcl_Obj **objv, *obj;

    nocase = (findData->flags & MATCH_NOCASE);
    result = FALSE;
    type = (findData->flags & PATTERN_MASK);
    pattern = Tcl_GetString(findData->name);
    switch (type) {
        case 0:
	case PATTERN_EXACT:
	    if (nocase) {
                result = (strcasecmp(string, pattern) == 0);
             } else {
                result = (strcmp(string, pattern) == 0);
            }
	    break;
	    
	case PATTERN_GLOB:
            result = (Tcl_StringCaseMatch(string, pattern, 1) == 1);
	    break;
		    
	case PATTERN_REGEXP:
            if (nocase) {
                string = Blt_Strdup(string);
                strtolower(string);
            }
            obj = Tcl_NewStringObj(string, -1);
	    result = (Tcl_RegExpMatchObj((Tcl_Interp *)NULL, obj, findData->name) == 1); 
	    Tcl_DecrRefCount(obj);
            if (nocase) {
	       Blt_Free(string);
            }
	    break;
	case PATTERN_INLIST:
            if (Tcl_ListObjGetElements(NULL, findData->name, &objc, &objv) != TCL_OK) {
                return 1;
            }
            for (i = 0; i < objc; i++) {
                pattern = Tcl_GetString(objv[i]);
                if (!nocase) {
                    if (strcmp(string, pattern) == 0) {
                        return 1;
                    }
                } else {
                    if (strcasecmp(string, pattern) == 0) {
                        return 1;
                    }
                }
            }
            
	    break;
    }
    return result;
}


static void
PercentSubst(
    FindData *dataPtr,
    Blt_TreeNode node,
    char *command,
    Tcl_DString *resultPtr)
{
    register char *last, *p;
    Tcl_DString dString;
    Blt_TreeKey key;
    TreeCmd *cmdPtr = dataPtr->cmdPtr;
    Blt_TreeKeySearch cursor;
    Tcl_Obj *valueObjPtr;
    unsigned int inode = node->inode;
    int one = (command[0] == '%' && strlen(command)==2);

    /*
     * Get the full path name of the node, in case we need to
     * substitute for it.
     */
    Tcl_DStringInit(&dString);
    Tcl_DStringInit(resultPtr);
    /* Append the widget name and the node .t 0 */
    for (last = p = command; *p != '\0'; p++) {
	if (*p == '%') {
	    char *string;
	    char buf[3];

	    if (p > last) {
		*p = '\0';
		Tcl_DStringAppend(resultPtr, last, -1);
		*p = '%';
	    }
	    switch (*(p + 1)) {
	    case '%':		/* Percent sign */
		string = "%";
		break;
	    case 'W':		/* Tree name */
                Tcl_DStringFree(&dString);
                Tcl_DStringInit(&dString);
                string = dataPtr->cmdPtr->tree->treeObject->name;
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'P':		/* Tree node path */
                string = Blt_TreeNodePath(node, &dString);
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'T':	  /* Tag path form. */
                string = Blt_TreeNodePathStr(node, &dString, ".", ".");
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'R':	  /* Root path form. */
                string = Blt_TreeNodePathStr(node, &dString, "root->", "->");
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'r':	  /* 0 root path form. */
                string = Blt_TreeNodePathStr(node, &dString, "0->", "->");
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'D':		/* The full value */
                Tcl_DStringSetLength(&dString, 0);
                for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &cursor);
                    key != NULL; 
                    key = Blt_TreeNextKey(cmdPtr->tree, &cursor)) {
                    if (Blt_TreeGetValue((Tcl_Interp *)NULL, cmdPtr->tree, node,
                        key, &valueObjPtr) == TCL_OK) {
                            
                        Tcl_DStringAppendElement(&dString, key);
                        Tcl_DStringAppendElement(&dString, Tcl_GetString(valueObjPtr));
                    }
                    if (Blt_TreeNodeDeleted(node) || node->inode != inode) {
                        break;
                    }
                }	    
                string = Tcl_DStringValue(&dString);
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'p':		/* The label */
                string = (node->label ? node->label : "");
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case 'V':		/* The value or label */
                if (dataPtr->keyList == NULL || ((string = GetFirstKey(dataPtr->keyList)) == NULL)) {
                    string = (node->label ? node->label : "");
                } else {
                    Tcl_Obj *objPtr;
                    if  (Blt_TreeGetValue(dataPtr->cmdPtr->interp, cmdPtr->tree, node, string, &objPtr) == TCL_OK) {
                        string = Tcl_GetString(objPtr);
                    } else {
                        string = "";
                    }
                    if (string == NULL) { string = ""; }
                }
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
                break;
	    case '#':		/* Node identifier */
                string = Blt_Itoa(Blt_TreeNodeId(node));
                break;
	    default:
		if (*(p + 1) == '\0') {
		    p--;
		}
		buf[0] = *p, buf[1] = *(p + 1), buf[2] = '\0';
		string = buf;
		break;
	    }
            if (string) {
                Tcl_DStringAppend(resultPtr, string, -1);
            }
	    p++;
	    last = p + 1;
	}
    }
    if (p > last) {
	*p = '\0';
	Tcl_DStringAppend(resultPtr, last, -1);
    }
    Tcl_DStringFree(&dString);
}


/*
 *----------------------------------------------------------------------
 *
 * MatchNodeProc --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
MatchNodeProc(Blt_TreeNode node, ClientData clientData, int order)
{
    FindData *dataPtr = clientData;
    Tcl_DString dString;
    TreeCmd *cmdPtr = dataPtr->cmdPtr;
    Tcl_Interp *interp = dataPtr->cmdPtr->interp;
    int result, invert, cntf;
    unsigned int inode;
    int strict, isnull;
    char *curValue = NULL;
    Tcl_Obj *curObj = NULL, *resObjPtr = NULL;
    
    isnull = ((dataPtr->flags&MATCH_ISNULL) != 0);
    strict = ((dataPtr->flags&MATCH_STRICT) != 0);
    cntf = ((dataPtr->flags & MATCH_COUNT) != 0);
    invert = (dataPtr->flags & MATCH_INVERT) ? TRUE : FALSE;
    if ((dataPtr->flags & MATCH_NOTOP) && node == dataPtr->startNode) {
        return TCL_OK;
    }
    if ((dataPtr->flags & MATCH_FIXED) && 
        (node->flags & TREE_NODE_FIXED_FIELDS) == 0) {
        return TCL_OK;
    }
    if ((dataPtr->flags & MATCH_NOTFIXED) && 
        (node->flags & TREE_NODE_FIXED_FIELDS) != 0) {
        return TCL_OK;
    }
    if ((dataPtr->flags & MATCH_ISMODIFIED) && 
        (node->flags & TREE_NODE_UNMODIFIED) != 0) {
        return TCL_OK;
    }
    if ((dataPtr->flags & MATCH_LEAFONLY) && (!Blt_TreeIsLeaf(node))) {
	return TCL_OK;
    }
    if ((dataPtr->flags & MATCH_TREEONLY) && (Blt_TreeIsLeaf(node))) {
        return TCL_OK;
    }
    if ((dataPtr->maxDepth >= 0) &&
	(Blt_TreeNodeDepth(cmdPtr->tree, node) > dataPtr->maxDepth )) {
	return TCL_OK;
    }
    if ((dataPtr->minDepth >= 0) &&
        (Blt_TreeNodeDepth(cmdPtr->tree, node) < dataPtr->minDepth)) {
	return TCL_OK;
    }
    if ((dataPtr->depth >= 0) &&
        (Blt_TreeNodeDepth(cmdPtr->tree, node) != dataPtr->depth)) {
	return TCL_OK;
    }
    if ((dataPtr->withTag != NULL) &&
        !Blt_TreeHasTag(cmdPtr->tree, node, Tcl_GetString(dataPtr->withTag))) {
        return TCL_OK;
    }
    if ((dataPtr->withoutTag != NULL) &&
        Blt_TreeHasTag(cmdPtr->tree, node, Tcl_GetString(dataPtr->withoutTag))) {
        return TCL_OK;
    }
    if ((dataPtr->keyCount >= 0) &&
        Blt_TreeCountKeys(cmdPtr->tree, node) != dataPtr->keyCount) {
        return TCL_OK;
    } 

    result = TRUE;
    Tcl_DStringInit(&dString);
    inode = node->inode;
    if (dataPtr->subKey != NULL) {
        int empty;
        
        empty = (Blt_TreeGetValue(interp, cmdPtr->tree, node, dataPtr->subKey, &curObj)
            == TCL_OK);
        if (empty == isnull) {
            Tcl_DStringFree(&dString);
            return TCL_OK;
        }
        if (curObj != NULL && dataPtr->name) {
            curValue = (curObj == NULL) ? "" : Tcl_GetString(curObj);
            result = ComparePattern(dataPtr, curValue);
        }
    } else if (dataPtr->keyList != NULL) {
	Blt_TreeKey key;
	Blt_TreeKeySearch cursor;

	result = FALSE;		/* It's false if no keys match. */
	for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &cursor);
	    key != NULL; key = Blt_TreeNextKey(cmdPtr->tree, &cursor)) {
             
            curObj = NULL;
	    result = ComparePatternList(dataPtr->keyList, key, 0);
	    if (!result) {
		continue;
	    }
            if ((dataPtr->flags & (MATCH_ARRAY))) {
                Blt_HashTable *tablePtr;
                int res;
                
                res = TCL_ERROR;
                if (Blt_TreeGetValue(interp, cmdPtr->tree, node, key, &curObj)
                    == TCL_OK) {
                    res = Blt_GetArrayFromObj(NULL, curObj, &tablePtr);
                }
                if (Blt_TreeNodeDeleted(node) || node->inode != inode) {
                    Tcl_DStringFree(&dString);
                    return TCL_OK;
                }
                if ((dataPtr->flags & MATCH_ARRAY) && res != TCL_OK) {
                    result = FALSE;
                    continue;
                }
            }
	    if (dataPtr->name != NULL) {

		if (Blt_TreeGetValue(interp, cmdPtr->tree, node, key, &curObj) != TCL_OK) {
                    Tcl_DStringFree(&dString);
                    return TCL_ERROR;
                }
		curValue = (curObj == NULL) ? "" : Tcl_GetString(curObj);
		result = ComparePattern(dataPtr, curValue);
		if (!result) {
		    continue;
		}
            }
	    break;
	}
    } else if (dataPtr->name != NULL) {	    

	if (dataPtr->flags & MATCH_PATHNAME) {
	    curValue = GetNodePath(cmdPtr, Blt_TreeRootNode(cmdPtr->tree),
		 node, FALSE, &dString);
	} else {
	    curValue = Blt_TreeNodeLabel(node);
	}
	result = ComparePattern(dataPtr, curValue);
    }
    Tcl_DStringFree(&dString);

    if (result != invert) {
	Tcl_Obj *objPtr;

	if (dataPtr->addTag != NULL) {
	    if (AddTag(cmdPtr, node, dataPtr->addTag) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
        if (dataPtr->eval != NULL && dataPtr->eval[0] != 0) {
            if (dataPtr->execVar != NULL) {
                Tcl_Obj *intObj;
                intObj = Tcl_NewIntObj(node->inode);
                Tcl_IncrRefCount(intObj);
                if (Tcl_ObjSetVar2(interp, dataPtr->execVar, NULL, intObj, 0) == NULL) {
                    Tcl_DecrRefCount(intObj);
                    return TCL_ERROR;
                }
                result = Tcl_EvalObjEx(interp, dataPtr->execObj, 0);
                Tcl_DecrRefCount(intObj);
               /* if (Blt_TreeNodeDeleted(node) || node->inode != inode) {
                    return TCL_ERROR;
                }*/
                if (cmdPtr->delete) {
                    return TCL_ERROR;
                }
            } else {
                Tcl_DString sString;
                PercentSubst( dataPtr, node, dataPtr->eval, &sString);
                result = Tcl_EvalEx(interp, Tcl_DStringValue(&sString), -1, TCL_EVAL_DIRECT);
                Tcl_DStringFree(&sString);
                /*if (Blt_TreeNodeDeleted(node) || node->inode != inode) {
                    return TCL_ERROR;
                }*/
                if (cmdPtr->delete) {
                    return TCL_ERROR;
                }
            }
            if (result != TCL_CONTINUE  && result != TCL_OK) {
                return result;
            }
            if (result == TCL_OK && cntf == 0) {
                resObjPtr =  Tcl_GetObjResult(interp);
            } else {
                return TCL_OK;
            }
        } else {
            if (dataPtr->retKey != NULL) {
                Tcl_Obj *oPtr;
                
                oPtr = NULL;
                if (dataPtr->retKey[0] == 0) {
                    objPtr = Tcl_NewStringObj(Blt_TreeNodeLabel(node), -1);
                } else {
                    if (dataPtr->iskey) {
                        if (Blt_TreeGetValue(strict?interp:NULL, cmdPtr->tree, node, dataPtr->retKey, &oPtr) != TCL_OK) {
                            if (strict) {
                                return TCL_ERROR;
                            }
                        }
                    }
                    if (oPtr != NULL) {
                        if (!cntf) {
                            objPtr = Tcl_NewStringObj(Tcl_GetString(oPtr),-1);
                        }
                    } else if (cntf == 0 && dataPtr->retKey[0] == '%' &&
                         strlen(dataPtr->retKey)>=2) {
                        Tcl_DString sString;
                        PercentSubst( dataPtr, node, dataPtr->retKey, &sString);
                        objPtr = Tcl_NewStringObj(Tcl_DStringValue(&sString), -1);
                        Tcl_DStringFree(&sString);
                    } else if (!cntf) {
                        objPtr = Tcl_NewStringObj("" ,-1);
                    }
                }
            } else if (!cntf) {
                objPtr = Tcl_NewIntObj(Blt_TreeNodeId(node));
            }
            if (!cntf) {
                resObjPtr = objPtr;
            }
        }
	if (dataPtr->objv != NULL) {
	    int ai;
	    char **p;
            objPtr = Tcl_NewIntObj(Blt_TreeNodeId(node));
            Tcl_DecrRefCount(dataPtr->objv[dataPtr->objc - 1]);
            Tcl_IncrRefCount(objPtr);
            dataPtr->objv[dataPtr->objc - 1] = objPtr;
            p = dataPtr->cmdArgs;
            for (ai = 0; ai < dataPtr->argc && *p != NULL; ai++, p++) {
                if (Blt_TreeGetValue(interp, cmdPtr->tree, node, *p, &objPtr) != TCL_OK) {
                    objPtr = Tcl_NewStringObj("", -1);
                }
                Tcl_DecrRefCount(dataPtr->objv[dataPtr->objc + ai]);
                Tcl_IncrRefCount(objPtr);
                dataPtr->objv[dataPtr->objc + ai] = objPtr;
            }
	    result = Tcl_EvalObjv(interp, dataPtr->objc + dataPtr->argc, dataPtr->objv, 0);
            if (cmdPtr->delete) {
                return TCL_ERROR;
            }
            if (result == TCL_RETURN) {
                int eRes;
                if (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp),
                    &eRes) != TCL_OK) {
                    return TCL_ERROR;
                }
                result = TCL_OK;
                if (eRes == 0) {
                    resObjPtr = NULL;
                } else if (cntf) {
                    goto finishNode;
                }
            } else {
                resObjPtr = NULL;
            }
            
	    if (result != TCL_OK) {
		return result;
	    }
	}
	if (resObjPtr != NULL) {
            Tcl_ListObjAppendElement(interp, dataPtr->listObjPtr, resObjPtr);
            finishNode:
            dataPtr->nMatches++;
            if ((dataPtr->maxMatches > 0) && 
            (dataPtr->nMatches >= dataPtr->maxMatches)) {
                return TCL_BREAK;
            }
        }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ApplyNodeProc --
 *
 *---------------------------------------------------------------------- 
 */
static int
ApplyNodeProc(Blt_TreeNode node, ClientData clientData, int order)
{
    ApplyData *dataPtr = clientData;
    TreeCmd *cmdPtr = dataPtr->cmdPtr;
    Tcl_Interp *interp = cmdPtr->interp;
    int invert, result;
    Tcl_DString dString;

    if ((dataPtr->flags & MATCH_LEAFONLY) && (!Blt_TreeIsLeaf(node))) {
	return TCL_OK;
    }
    if ((dataPtr->flags & MATCH_TREEONLY) && (Blt_TreeIsLeaf(node))) {
        return TCL_OK;
    }
    if ((dataPtr->maxDepth >= 0) &&
	(dataPtr->maxDepth < Blt_TreeNodeDepth(cmdPtr->tree, node))) {
	return TCL_OK;
    }
    Tcl_DStringInit(&dString);
    result = TRUE;
    if (dataPtr->keyList != NULL) {
	Blt_TreeKey key;
	Blt_TreeKeySearch cursor;

	result = FALSE;		/* It's false if no keys match. */
	for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &cursor);
	     key != NULL; key = Blt_TreeNextKey(cmdPtr->tree, &cursor)) {
	    
	    result = ComparePatternList(dataPtr->keyList, key, 0);
	    if (!result) {
		continue;
	    }
	    if (dataPtr->patternList != NULL) {
		char *string;
		Tcl_Obj *objPtr = NULL;

		if (Blt_TreeGetValue(interp, cmdPtr->tree, node, key, &objPtr) != TCL_OK) {
                    return TCL_ERROR;
                }
                string = (objPtr == NULL) ? "" : Tcl_GetString(objPtr);
		result = ComparePatternList(dataPtr->patternList, string, 
			 dataPtr->flags & MATCH_NOCASE);
		if (!result) {
		    continue;
		}
	    }
	    break;
	}
    } else if (dataPtr->patternList != NULL) {	    
	char *string;

	if (dataPtr->flags & MATCH_PATHNAME) {
	    string = GetNodePath(cmdPtr, Blt_TreeRootNode(cmdPtr->tree),
		 node, FALSE, &dString);
	} else {
	    string = Blt_TreeNodeLabel(node);
	}
	result = ComparePatternList(dataPtr->patternList, string, 
		dataPtr->flags & MATCH_NOCASE);		     
    }
    Tcl_DStringFree(&dString);
    if ((dataPtr->withTag != NULL) && 
	(!Blt_TreeHasTag(cmdPtr->tree, node, dataPtr->withTag))) {
	result = FALSE;
    }
    invert = (dataPtr->flags & MATCH_INVERT) ? 1 : 0;
    if (result != invert) {
	Tcl_Obj *objPtr;
	int res = TCL_OK;

	objPtr = Tcl_NewIntObj(Blt_TreeNodeId(node));
	if (order == TREE_PREORDER) {
	    dataPtr->preObjv[dataPtr->preObjc - 1] = objPtr;
	    res = Tcl_EvalObjv(interp, dataPtr->preObjc, dataPtr->preObjv, 0);
	} else if (order == TREE_POSTORDER) {
	    dataPtr->postObjv[dataPtr->postObjc - 1] = objPtr;
	    res = Tcl_EvalObjv(interp, dataPtr->postObjc, dataPtr->postObjv,0);
	}
        if (cmdPtr->delete) {
            return TCL_ERROR;
        }
	return res;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseTreeObject --
 *
 *---------------------------------------------------------------------- 
 */
static void
ReleaseTreeObject(TreeCmd *cmdPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    TraceInfo *tracePtr;
    NotifyInfo *notifyPtr;
    int i;

    Blt_TreeReleaseToken(cmdPtr->tree);
    /* 
     * When the tree token is released, all the traces and
     * notification events are automatically removed.  But we still
     * need to clean up the bookkeeping kept for traces. Clear all
     * the tags and trace information.  
     */
    for (hPtr = Blt_FirstHashEntry(&(cmdPtr->traceTable), &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	tracePtr = Blt_GetHashValue(hPtr);
	Blt_Free(tracePtr);
    }
    for (hPtr = Blt_FirstHashEntry(&(cmdPtr->notifyTable), &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	notifyPtr = Blt_GetHashValue(hPtr);
	for (i = 0; i < notifyPtr->objc - 2; i++) {
	    Tcl_DecrRefCount(notifyPtr->objv[i]);
	}
	Blt_Free(notifyPtr->objv);
	Blt_Free(notifyPtr);
    }
    cmdPtr->tree = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeTraceProc --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TreeTraceProc(
    ClientData clientData,
    Tcl_Interp *interp,
    Blt_TreeNode node,		/* Node that has just been updated. */
    Blt_TreeKey key,		/* Field that's updated. */
    unsigned int flags)
{
    TraceInfo *tracePtr = clientData; 
    Tcl_DString dsCmd, dsName;
    char string[5];
    char *qualName;
    int result;

    Tcl_DStringInit(&dsCmd);
    Tcl_DStringAppend(&dsCmd, tracePtr->command, -1);
    Tcl_DStringInit(&dsName);
    qualName = Blt_GetQualifiedName(
	Blt_GetCommandNamespace(interp, tracePtr->cmdPtr->cmdToken), 
	Tcl_GetCommandName(interp, tracePtr->cmdPtr->cmdToken), &dsName);
    Tcl_DStringAppendElement(&dsCmd, qualName);
    Tcl_DStringFree(&dsName);
    if (node != NULL) {
	Tcl_DStringAppendElement(&dsCmd, Blt_Itoa(Blt_TreeNodeId(node)));
    } else {
	Tcl_DStringAppendElement(&dsCmd, "");
    }
    Tcl_DStringAppendElement(&dsCmd, key);
    PrintTraceFlags(flags, string);
    Tcl_DStringAppendElement(&dsCmd, string);
    result = Tcl_Eval(interp, Tcl_DStringValue(&dsCmd));
    Tcl_DStringFree(&dsCmd);
    if (tracePtr->cmdPtr && tracePtr->cmdPtr->delete) {
        return TCL_ERROR;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeEventProc --
 *
 *---------------------------------------------------------------------- 
 */
static int
TreeEventProc(ClientData clientData, Blt_TreeNotifyEvent *eventPtr)
{
    TreeCmd *cmdPtr = clientData; 
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    NotifyInfo *notifyPtr;
    Blt_TreeNode node;
    char *string;

    switch (eventPtr->type) {
    case TREE_NOTIFY_CREATE:
	string = "-create";
	break;

    case TREE_NOTIFY_DELETE:
	node = Blt_TreeGetNode(cmdPtr->tree, eventPtr->inode);
	if (node != NULL) {
	    Blt_TreeClearTags(cmdPtr->tree, node);
	}
	string = "-delete";
	break;

    case TREE_NOTIFY_GET:
	string = "-get";
	break;
	
    case TREE_NOTIFY_MOVE:
	string = "-move";
	break;

    case TREE_NOTIFY_MOVEPOST:
	string = "-movepost";
	break;

    case TREE_NOTIFY_INSERT:
	string = "-insert";
	break;

    case TREE_NOTIFY_SORT:
	string = "-sort";
	break;

    case TREE_NOTIFY_RELABEL:
	string = "-relabel";
	break;

    case TREE_NOTIFY_RELABELPOST:
	string = "-relabelpost";
	break;

    default:
	/* empty */
	string = "???";
	break;
    }	

    for (hPtr = Blt_FirstHashEntry(&(cmdPtr->notifyTable), &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	notifyPtr = Blt_GetHashValue(hPtr);
	if (notifyPtr->mask & eventPtr->type) {
	    int result, mkAct = 0;
	    Tcl_Obj *flagObjPtr, *nodeObjPtr;

	    flagObjPtr = Tcl_NewStringObj(string, -1);
	    nodeObjPtr = Tcl_NewIntObj(eventPtr->inode);
	    Tcl_IncrRefCount(flagObjPtr);
	    Tcl_IncrRefCount(nodeObjPtr);
	    notifyPtr->objv[notifyPtr->objc - 1] = flagObjPtr;
	    notifyPtr->objv[notifyPtr->objc - 2] = nodeObjPtr;
            if ((notifyPtr->mask&TREE_NOTIFY_TRACEACTIVE)) {
                node = Blt_TreeGetNode(cmdPtr->tree, eventPtr->inode);
                if ((node->flags&TREE_TRACE_ACTIVE) == 0) {
                    node->flags |= TREE_TRACE_ACTIVE;
                    mkAct = 1;
                }
            }

	    result = Tcl_EvalObjv(cmdPtr->interp, notifyPtr->objc, 
		notifyPtr->objv, 0);
	    if (mkAct) {
	        node->flags &= ~TREE_TRACE_ACTIVE;
            }
	    Tcl_DecrRefCount(nodeObjPtr);
	    Tcl_DecrRefCount(flagObjPtr);
            if (cmdPtr->delete) {
                return TCL_ERROR;
            }
            if (result != TCL_OK) {
		/*Tcl_BackgroundError(cmdPtr->interp); */
		return result;
	    }
	    Tcl_ResetResult(cmdPtr->interp);
	}
    }
    return TCL_OK;
}


/* Tree command operations. */

/*
 *----------------------------------------------------------------------
 *
 * ApplyOp --
 *
 * t0 apply root -precommand {command} -postcommand {command}
 *
 *---------------------------------------------------------------------- 
 */
static int
ApplyOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    int result;
    Blt_TreeNode node;
    int i;
    Tcl_Obj **objArr;
    int count;
    ApplyData data;
    int order;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    memset(&data, 0, sizeof(data));
    data.maxDepth = -1;
    data.cmdPtr = cmdPtr;
    
    /* Process switches  */
    if (Blt_ProcessObjSwitches(interp, applySwitches, objc - 3, objv + 3, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	return TCL_ERROR;
    }
    order = 0;
    if (data.flags & MATCH_NOCASE) {
	Blt_ListNode listNode;

	for (listNode = Blt_ListFirstNode(data.patternList); listNode != NULL;
	     listNode = Blt_ListNextNode(listNode)) {
	    strtolower((char *)Blt_ListGetKey(listNode));
	}
    }
    if (data.preCmd != NULL) {
	char **p;

	count = 0;
	for (p = data.preCmd; *p != NULL; p++) {
	    count++;
	}
	objArr = Blt_Calloc((count + 1), sizeof(Tcl_Obj *));
	for (i = 0; i < count; i++) {
	    objArr[i] = Tcl_NewStringObj(data.preCmd[i], -1);
	    Tcl_IncrRefCount(objArr[i]);
	}
	data.preObjv = objArr;
	data.preObjc = count + 1;
	order |= TREE_PREORDER;
    }
    if (data.postCmd != NULL) {
	char **p;

	count = 0;
	for (p = data.postCmd; *p != NULL; p++) {
	    count++;
	}
	objArr = Blt_Calloc((count + 1), sizeof(Tcl_Obj *));
	for (i = 0; i < count; i++) {
	    objArr[i] = Tcl_NewStringObj(data.postCmd[i], -1);
	    Tcl_IncrRefCount(objArr[i]);
	}
	data.postObjv = objArr;
	data.postObjc = count + 1;
	order |= TREE_POSTORDER;
    }
    result = Blt_TreeApplyDFS(node, ApplyNodeProc, &data, order);
    if (data.preObjv != NULL) {
	for (i = 0; i < (data.preObjc - 1); i++) {
	    Tcl_DecrRefCount(data.preObjv[i]);
	}
	Blt_Free(data.preObjv);
    }
    if (data.postObjv != NULL) {
	for (i = 0; i < (data.postObjc - 1); i++) {
	    Tcl_DecrRefCount(data.postObjv[i]);
	}
	Blt_Free(data.postObjv);
    }
    Blt_FreeSwitches(interp, applySwitches, (char *)&data, 0);
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*ARGSUSED*/
static int
AncestorOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    int d1, d2, minDepth;
    register int i;
    Blt_TreeNode ancestor, node1, node2;

    if ((GetNode(cmdPtr, objv[2], &node1) != TCL_OK) ||
	(GetNode(cmdPtr, objv[3], &node2) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (node1 == node2) {
	ancestor = node1;
	goto done;
    }
    d1 = Blt_TreeNodeDepth(cmdPtr->tree, node1);
    d2 = Blt_TreeNodeDepth(cmdPtr->tree, node2);
    minDepth = MIN(d1, d2);
    if (minDepth == 0) {	/* One of the nodes is root. */
	ancestor = Blt_TreeRootNode(cmdPtr->tree);
	goto done;
    }
    /* 
     * Traverse back from the deepest node, until the both nodes are
     * at the same depth.  Check if the ancestor node found is the
     * other node.  
     */
    for (i = d1; i > minDepth; i--) {
	node1 = Blt_TreeNodeParent(node1);
    }
    if (node1 == node2) {
	ancestor = node2;
	goto done;
    }
    for (i = d2; i > minDepth; i--) {
	node2 = Blt_TreeNodeParent(node2);
    }
    if (node2 == node1) {
	ancestor = node1;
	goto done;
    }

    /* 
     * First find the mutual ancestor of both nodes.  Look at each
     * preceding ancestor level-by-level for both nodes.  Eventually
     * we'll find a node that's the parent of both ancestors.  Then
     * find the first ancestor in the parent's list of subnodes.  
     */
    for (i = minDepth; i > 0; i--) {
	node1 = Blt_TreeNodeParent(node1);
	node2 = Blt_TreeNodeParent(node2);
	if (node1 == node2) {
	    ancestor = node2;
	    goto done;
	}
    }
    Tcl_AppendResult(interp, "unknown ancestor", (char *)NULL);
    return TCL_ERROR;
 done:
    Tcl_SetIntObj(Tcl_GetObjResult(interp), Blt_TreeNodeId(ancestor));
    return TCL_OK;
}

#ifndef NO_ATTACHCMD
/*
 *----------------------------------------------------------------------
 *
 * AttachOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
AttachOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    if (objc >= 3) {
	CONST char *treeName;
	CONST char *name;
	Blt_Tree token;
	Tcl_Namespace *nsPtr;
	Tcl_DString dString;
	int result, notag = 0;

        if (objc == 3) {
            treeName = Tcl_GetString(objv[2]);
        } else {
            if (!strcmp("-notags", Tcl_GetString(objv[2]))) {
                Tcl_AppendResult(interp, "expected \"-notags\"", (char *)NULL);
                    return TCL_ERROR;
            }
	    treeName = Tcl_GetString(objv[3]);
	    notag = 1;
	}
	if (Blt_ParseQualifiedName(interp, treeName, &nsPtr, &name) 
	    != TCL_OK) {
	    Tcl_AppendResult(interp, "can't find namespace in \"", treeName, 
			     "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (nsPtr == NULL) {
	    nsPtr = Tcl_GetCurrentNamespace(interp);
	}
	treeName = Blt_GetQualifiedName(nsPtr, name, &dString);
         if (notag) {
             result = Blt_TreeGetToken(interp, treeName, &token);
         } else {
             result = Blt_TreeGetTokenTag(interp, treeName, &token);
	}
	Tcl_DStringFree(&dString);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
        Blt_TreeNotifyAttach(token);
	ReleaseTreeObject(cmdPtr);
	cmdPtr->tree = token;
    }
    Tcl_SetResult(interp, Blt_TreeName(cmdPtr->tree), TCL_VOLATILE);
    return TCL_OK;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * ChildrenOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
ChildrenOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int labels = 0;
    char *str;
    
    str = Tcl_GetString(objv[2]);
    if (!strcmp(str, "-labels")) {
        labels = 1;
        objc--;
        objv++;
    }
   
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_Obj *objPtr, *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (node = Blt_TreeFirstChild(node); node != NULL;
	     node = Blt_TreeNextSibling(node)) {
	    if (labels) {
                objPtr = Tcl_NewStringObj(node->label, -1);
            } else {
                objPtr = Tcl_NewIntObj(Blt_TreeNodeId(node));
            }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
    } else if (objc == 4) {
	int childPos;
	int inode, count;
	
	/* Get the node at  */
	if (Tcl_GetIntFromObj(interp, objv[3], &childPos) != TCL_OK) {
		return TCL_ERROR;
	}
	count = 0;
	inode = -1;
	for (node = Blt_TreeFirstChild(node); node != NULL;
	     node = Blt_TreeNextSibling(node)) {
	    if (count == childPos) {
	       if (labels) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(node->label, -1));
                    return TCL_OK;
                }
		inode = Blt_TreeNodeId(node);
		break;
	    }
	    count++;
	}
	Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
	return TCL_OK;
    } else if (objc == 5) {
	int firstPos, lastPos, count;
	Tcl_Obj *objPtr, *listObjPtr;
	char *string;

	firstPos = lastPos = Blt_TreeNodeDegree(node) - 1;
	string = Tcl_GetString(objv[3]);
	if ((strcmp(string, "end") != 0) &&
	    (Tcl_GetIntFromObj(interp, objv[3], &firstPos) != TCL_OK)) {
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[4]);
	if ((strcmp(string, "end") != 0) &&
	    (Tcl_GetIntFromObj(interp, objv[4], &lastPos) != TCL_OK)) {
	    return TCL_ERROR;
	}

	count = 0;
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (node = Blt_TreeFirstChild(node); node != NULL;
	     node = Blt_TreeNextSibling(node)) {
	    if ((count >= firstPos) && (count <= lastPos)) {
	       if (labels) {
                    objPtr = Tcl_NewStringObj(node->label, -1);
                } else {
                    objPtr = Tcl_NewIntObj(Blt_TreeNodeId(node));
                }
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	    count++;
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}



static Blt_TreeNode 
CopyNodes(
    CopyData *dataPtr,
    Blt_TreeNode node,		/* Node to be copied. */
    Blt_TreeNode parent)	/* New parent for the copied node. */
{
    Blt_TreeNode newNode;	/* Newly created copy. */
    char *label;
    int isNew = 0;

    newNode = NULL;
    label = Blt_TreeNodeLabel(node);
    if (dataPtr->flags & COPY_OVERWRITE) {
	newNode = Blt_TreeFindChild(parent, label);
    }
    if (newNode == NULL) {	/* Create node in new parent. */
        isNew = 1;
	newNode = Blt_TreeCreateNode(dataPtr->destTree, parent, label, -1);
         if (newNode == NULL) {
             return NULL;
         }
     }
    /* Copy the data fields. */
    {
	Blt_TreeKey key;
	Tcl_Obj *objPtr;
	Blt_TreeKeySearch cursor;

	for (key = Blt_TreeFirstKey(dataPtr->srcTree, node, &cursor); 
	     key != NULL; key = Blt_TreeNextKey(dataPtr->srcTree, &cursor)) {
	    if (Blt_TreeGetValueByKey((Tcl_Interp *)NULL, dataPtr->srcTree, 
			node, key, &objPtr) == TCL_OK) {
		Blt_TreeSetValueByKey((Tcl_Interp *)NULL, dataPtr->destTree, 
			newNode, Blt_TreeKeyGet(NULL, dataPtr->destTree->treeObject, key), objPtr);
	    } 
	}
    }
    /* Add tags to destination tree command. */
    if ((dataPtr->destPtr != NULL) && (dataPtr->flags & COPY_TAGS)) {
	Blt_TreeTagEntry *tPtr;
	Blt_HashEntry *hPtr, *h2Ptr;
	Blt_HashSearch cursor;

	for (hPtr = Blt_TreeFirstTag(dataPtr->srcTree, &cursor); 
		hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    tPtr = Blt_GetHashValue(hPtr);
	    h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)node);
	    if (h2Ptr != NULL) {
		if (AddTag(dataPtr->destPtr, newNode, tPtr->tagName)!= TCL_OK) {
		    return NULL;
		}
	    }
	}
    }
    if (isNew && Blt_TreeInsertPost(dataPtr->destTree, newNode) == NULL) {
        DeleteNode(dataPtr->srcPtr, newNode);
        return NULL;
    }
    if (dataPtr->flags & COPY_RECURSE) {
	Blt_TreeNode child;

	for (child = Blt_TreeFirstChild(node); child != NULL;
	     child = Blt_TreeNextSibling(child)) {
	    if (CopyNodes(dataPtr, child, newNode) == NULL) {
		return NULL;
	    }
	}
    }
    return newNode;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyOp --
 * 
 *	t0 copy node tree node 
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
CopyOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    TreeCmd *srcPtr, *destPtr;
    Blt_Tree srcTree, destTree;
    Blt_TreeNode srcNode, destNode, tNode;
    CopyData data;
    int nArgs, nSwitches;
    char *string;
    Blt_TreeNode root;
    register int i;

    if (GetNode(cmdPtr, objv[2], &srcNode) != TCL_OK) {
	return TCL_ERROR;
    }
    srcTree = cmdPtr->tree;
    srcPtr = cmdPtr;

    /* Find the first switch. */
    for(i = 3; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
    }
    nArgs = i - 2;
    nSwitches = objc - i;
    if (nArgs < 2) {
	string = Tcl_GetString(objv[0]);
	Tcl_AppendResult(interp, "must specify source and destination nodes: ",
			 "should be \"", string, 
			 " copy srcNode ?destTree? destNode ?switches?", 
			 (char *)NULL);
	return TCL_ERROR;
	
    }
    if (nArgs == 3) {
	/* 
	 * The tree name is either the name of a tree command (first choice)
	 * or an internal tree object.  
	 */
	string = Tcl_GetString(objv[3]);
	destPtr = GetTreeCmd(cmdPtr->dataPtr, interp, string);
	if (destPtr != NULL) {
	    destTree = destPtr->tree;
	} else {
	    /* Try to get the tree as an internal tree data object. */
	    if (Blt_TreeGetToken(interp, string, &destTree) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	objv++;
    } else {
	destPtr = cmdPtr;
	destTree = destPtr->tree;
    }

    root = NULL;
    if (destPtr == NULL) {
	if (GetForeignNode(interp, destTree, objv[3], &destNode) != TCL_OK) {
	    goto error;
	}
    } else {
	if (GetNode(destPtr, objv[3], &destNode) != TCL_OK) {
	    goto error;
	}
    }
    if (srcNode == destNode) {
	Tcl_AppendResult(interp, "source and destination nodes are the same",
		 (char *)NULL);	     
	goto error;
    }
    memset((char *)&data, 0, sizeof(data));
    /* Process switches  */
    if (Blt_ProcessObjSwitches(interp, copySwitches, nSwitches, objv + 4, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	goto error;
    }
    if (data.flags & COPY_REVERSE) {
        data.destPtr = srcPtr;
        data.destTree = srcTree;
        data.srcPtr = destPtr;
        data.srcTree = destTree;
        tNode = srcNode;
        srcNode = destNode;
        destNode = tNode;
    } else {
        data.destPtr = destPtr;
        data.destTree = destTree;
        data.srcPtr = srcPtr;
        data.srcTree = srcTree;
    }

    if ((srcTree == destTree) && (data.flags & COPY_RECURSE) &&
	(Blt_TreeIsAncestor(srcNode, destNode))) {    
	Tcl_AppendResult(interp, "can't make cyclic copy: ",
			 "source node is an ancestor of the destination",
			 (char *)NULL);	     
	goto error;
    }

    /* Copy nodes to destination. */
    root = CopyNodes(&data, srcNode, destNode);
    if (root != NULL) {
	Tcl_Obj *objPtr;

	objPtr = Tcl_NewIntObj(Blt_TreeNodeId(root));
	if (data.label != NULL) {
	    Blt_TreeRelabelNode(data.destTree, root, data.label);
	}
	Tcl_SetObjResult(interp, objPtr);
    }
 error:
    if (destPtr == NULL) {
	Blt_TreeReleaseToken(destTree);
    }
    return (root == NULL) ? TCL_ERROR : TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * DepthOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
DegreeOp(cmdPtr, interp, objc, objv)
    TreeCmd *cmdPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Blt_TreeNode node;
    int degree;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    degree = Blt_TreeNodeDegree(node);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), degree);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Deletes one or more nodes from the tree.  Nodes may be
 *	specified by their id (a number) or a tag.
 *	
 *	Tags have to be handled carefully here.  We can't use the
 *	normal GetTaggedNode, NextTaggedNode, etc. routines because
 *	they walk hashtables while we're deleting nodes.  Also,
 *	remember that deleting a node recursively deletes all its
 *	children. If a parent and its children have the same tag, its
 *	possible that the tag list may contain nodes than no longer
 *	exist. So save the node indices in a list and then delete 
 *	then in a second pass.
 *
 *---------------------------------------------------------------------- 
 */
static int
DeleteOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int i, len;
    char *string;
    
    for (i = 2; i < objc; i++) {
	string = Tcl_GetStringFromObj(objv[i], &len);
	if (len == 0) continue;
	if (isdigit(UCHAR(string[0]))) {
             char *cp = string;
             int n, m, vobjc;
             Tcl_Obj **vobjv;
        
             while (isdigit(UCHAR(*cp)) && *cp != 0) {
                 cp++;
             }
             if (*cp == ' ' || *cp == '\t') {
                 if (Tcl_ListObjGetElements(interp, objv[i], &vobjc, &vobjv)
                    != TCL_OK) {
                    return TCL_ERROR;
                 }
                 for (n=0; n<vobjc; n++) {
                     if (Tcl_GetIntFromObj(interp, vobjv[n], &m) != TCL_OK) {
                         return TCL_ERROR;
                     }
                 }
                 for (n=0; n<vobjc; n++) {
                     if (GetNode(cmdPtr, vobjv[n], &node) == TCL_OK) {
                         DeleteNode(cmdPtr, node);
                     } else {
                         Tcl_ResetResult(interp);
                     }
                 }
                 continue;
             }
                 
             if (GetNode(cmdPtr, objv[i], &node) != TCL_OK) {
                 return TCL_ERROR;
             }
             DeleteNode(cmdPtr, node);
	} else {
	    Blt_HashEntry *hPtr;
	    Blt_HashTable *tablePtr;
	    Blt_HashSearch cursor;
	    Blt_Chain *chainPtr;
	    Blt_ChainLink *linkPtr, *nextPtr;
	    int inode;

	    if ((strcmp(string, "all") == 0) || (strcmp(string, "nonroot") == 0)
		|| (strcmp(string, "root") == 0)
		|| (strcmp(string, "rootchildren") == 0)) {
		node = Blt_TreeRootNode(cmdPtr->tree);
		DeleteNode(cmdPtr, node);
		continue;
	    }
	    tablePtr = Blt_TreeTagHashTable(cmdPtr->tree, string);
	    if (tablePtr == NULL) {
		goto error;
	    }
	    /* 
	     * Generate a list of tagged nodes. Save the inode instead
	     * of the node itself since a pruned branch may contain
	     * more tagged nodes.  
	     */
	    chainPtr = Blt_ChainCreate();
	    for (hPtr = Blt_FirstHashEntry(tablePtr, &cursor); 
		hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
		node = Blt_GetHashValue(hPtr);
		Blt_ChainAppend(chainPtr, (ClientData)(intptr_t)Blt_TreeNodeId(node));
	    }   
	    /*  
	     * Iterate through this list to delete the nodes.  By
	     * side-effect the tag table is deleted and Uids are
	     * released.  
	     */
	    for (linkPtr = Blt_ChainFirstLink(chainPtr); linkPtr != NULL;
		 linkPtr = nextPtr) {
		nextPtr = Blt_ChainNextLink(linkPtr);
		inode = (intptr_t)Blt_ChainGetValue(linkPtr);
		node = Blt_TreeGetNode(cmdPtr->tree, inode);
		if (node != NULL) {
		    DeleteNode(cmdPtr, node);
		}
	    }
	    Blt_ChainDestroy(chainPtr);
	}
    }
    return TCL_OK;
 error:
    Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in ", 
		     Blt_TreeName(cmdPtr->tree), (char *)NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DepthOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
DepthOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int depth;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    depth = Blt_TreeNodeDepth(cmdPtr->tree, node);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), depth);
    return TCL_OK;
}

/* Build a temporary hash table to speed tag lookups. */
static void
MakeTagTable(
    Blt_Tree tree,
    Blt_HashTable *nTable,
    char *match,
    char *skip
)
{
    Blt_HashEntry *hPtr, *h2Ptr, *h3Ptr;
    Blt_HashSearch cursor, tcursor;
    Tcl_DString *eStr;
    Blt_TreeNode snode;
    Blt_TreeTagEntry *tPtr;
    int isNew;
    char *name;
    
    Blt_InitHashTable(nTable, BLT_ONE_WORD_KEYS);
    for (hPtr = Blt_TreeFirstTag(tree, &cursor); hPtr != NULL;         
        hPtr = Blt_NextHashEntry(&cursor)) {
        
        tPtr = Blt_GetHashValue(hPtr);
        name = tPtr->tagName;
        
        if (match!=NULL && Tcl_StringMatch(name, match)!=1) continue;
        if (skip!=NULL && Tcl_StringMatch(name, skip)==1) continue;
        
        for (h2Ptr = Blt_FirstHashEntry(&tPtr->nodeTable, &tcursor);
        h2Ptr != NULL; h2Ptr = Blt_NextHashEntry(&tcursor)) {
            snode = Blt_GetHashValue(h2Ptr);
            if (snode == NULL) continue;
            h3Ptr = Blt_CreateHashEntry(nTable, (char*)snode, &isNew);
            if (h3Ptr == NULL) continue;
            if (isNew) {
                eStr = (Tcl_DString*)Blt_Calloc(sizeof(Tcl_DString),1);
                Tcl_DStringInit(eStr);
                Blt_SetHashValue(h3Ptr, eStr);
            } else {
                eStr = (Tcl_DString*)Blt_GetHashValue(h3Ptr);
            }
            Tcl_DStringAppendElement(eStr, tPtr->tagName);
        }
    }
}


static void
FreeTagTable(
    Blt_HashTable *nTable
)
{
    Blt_HashEntry *h2Ptr;
    Blt_HashSearch tcursor;
    Tcl_DString *eStr;

    for (h2Ptr = Blt_FirstHashEntry(nTable, &tcursor);
        h2Ptr != NULL; h2Ptr = Blt_NextHashEntry(&tcursor)) {
        eStr = (Tcl_DString*)Blt_GetHashValue(h2Ptr);
        Tcl_DStringFree(eStr);
        Blt_Free(eStr);
    }
    Blt_DeleteHashTable(nTable);
}

/*
 *----------------------------------------------------------------------
 *
 * DumpOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
DumpOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode top;
    Tcl_Channel channel = NULL;
    Tcl_DString dString;
    int result, isfile = 0, op, tags, doTbl = 0;
    register Blt_TreeNode node;
    RestoreData data;
    
    memset((char *)&data, 0, sizeof(data));

    if (GetNode(cmdPtr, objv[2], &top) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 3) {
        /* Process switches  */
        if (Blt_ProcessObjSwitches(interp, dumpSwitches, objc - 3, objv + 3, 
            (char *)&data, BLT_SWITCH_EXACT) < 0) {
            return TCL_ERROR;
        }
    }
    tags = ((data.flags&RESTORE_NO_TAGS) == 0);
    if (data.file != NULL && data.chan != NULL) {
        Tcl_AppendResult(interp, "can not use both -file and -channel", 0);
        return TCL_ERROR;
    }
    if (data.file != NULL) {
        if (Tcl_IsSafe(interp)) {
            Tcl_AppendResult(interp, "can use -file in safe interp", 0);
            return TCL_ERROR;
        }
        channel = Tcl_OpenFileChannel(interp, data.file, "w", 0644);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        isfile = 1;
    } else if (data.chan != NULL) {
        op = 0;
        channel = Tcl_GetChannel(interp, data.chan, &op);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((op & TCL_WRITABLE) == 0) {
            Tcl_AppendResult(interp, "channel is not writable", 0);
            return TCL_ERROR;
        }
    }
    if (data.keys != NULL && Tcl_ListObjGetElements(interp, data.keys, &data.kobjc,
        &data.kobjv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (data.notKeys != NULL && Tcl_ListObjGetElements(interp, data.notKeys,
        &data.nobjc, &data.nobjv) != TCL_OK) {
        return TCL_ERROR;
    }
    /*if (data.keys != NULL && data.notKeys != NULL) {
        Tcl_AppendResult(interp, "can not use both -keys and -notkeys", 0);
        return TCL_ERROR;
    }*/

    if (tags && top->nChildren>0) {
        doTbl = 1;
        MakeTagTable(cmdPtr->tree, &data.tagTable, data.tags, data.notTags);
    }
    result = TCL_OK;
    Tcl_DStringInit(&dString);
    if (channel != NULL) {
        int cnt = 1;
        for (node = top; node != NULL && cnt > 0; node = Blt_TreeNextNode(top, node)) {
            PrintNode(cmdPtr, top, node, &dString, tags, &data);
            if (Tcl_DStringLength(&dString) >= 4096) {
                cnt = Tcl_Write(channel, Tcl_DStringValue(&dString), -1);
                Tcl_DStringSetLength(&dString, 0);
            }
        }
        if (cnt > 0 && Tcl_DStringLength(&dString) > 0) {
            cnt = Tcl_Write(channel, Tcl_DStringValue(&dString), -1);
        }
        Tcl_DStringFree(&dString);
        if (isfile) {
            Tcl_Close(interp, channel);
        }
        if (cnt <= 0) {
            result = TCL_ERROR;
        }
    } else {
        for (node = top; node != NULL; node = Blt_TreeNextNode(top, node)) {
            PrintNode(cmdPtr, top, node, &dString, tags, &data);
        }
        Tcl_DStringResult(interp, &dString);
    }
    if (doTbl) {
        FreeTagTable(&data.tagTable);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
ExistsOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int bool;
    
    bool = TRUE;
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	bool = FALSE;
    } else if (objc == 4) { 
	char *string;
	
	string = Tcl_GetString(objv[3]);
	if (!Blt_TreeValueExists(cmdPtr->tree, node, 
			     string)) {
	    bool = FALSE;
	}
    } 
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FindOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
FindOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node, child;
    FindData data;
    int result;
    Tcl_Obj **objArr;

   /* if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }*/
    memset(&data, 0, sizeof(data));
    data.cmdPtr = cmdPtr;
    data.startNode = node = Blt_TreeRootNode(cmdPtr->tree);
    data.maxDepth = -1;
    data.minDepth = -1;
    data.depth = -1;
    data.keyCount = -1;
    data.flags = 0;
    data.order = TREE_PREORDER;
    objArr = NULL;

    /* Process switches  */
    if (Blt_ProcessObjSwitches(interp, findSwitches, objc - 2, objv + 2, 
		     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	return TCL_ERROR;
    }
    if (data.nodesObj != NULL) {
        if (data.flags & MATCH_NOTOP) {
            Tcl_AppendResult(interp, "-nodes can not use -notop", 0);
            return TCL_ERROR;
        }
    }
    if (data.execObj != NULL && data.command != NULL) {
        Tcl_AppendResult(interp, "can not use both -command and -exec", 0);
        return TCL_ERROR;
    }
    if (data.execObj != NULL) {
        data.eval = Tcl_GetString(data.execObj);
    } else if (data.execVar != NULL) {
        Tcl_AppendResult(interp, "-var must be used with -exec", 0);
        return TCL_ERROR;
    }
    if (data.startNode != NULL) {
        node = data.startNode;
        if (data.nodesObj != NULL) {
            Tcl_AppendResult(interp, "-nodes can not use -top", 0);
            return TCL_ERROR;
        }
    }
    if (data.keyList == NULL) {
        if (data.flags & (MATCH_ARRAY)) {
            Tcl_AppendResult(interp, "-array must be used with -key", 0);
            result = TCL_ERROR;
            goto done;
        }
    }
    if (data.retKey != NULL) {

        if (data.flags & MATCH_COUNT) {
            Tcl_AppendResult(interp, "can not use -return and -count", 0);
            result = TCL_ERROR;
            goto done;
        }
        if (data.eval != NULL) {
            Tcl_AppendResult(interp, "can not use -return and -exec", 0);
            result = TCL_ERROR;
            goto done;
        }
        if (data.command != NULL) {
            Tcl_AppendResult(interp, "can not use -return and -command", 0);
            result = TCL_ERROR;
            goto done;
        }
        if (data.retKey[0] == '%') {
         } else if (Blt_TreeKeyGet(NULL, cmdPtr->tree->treeObject, data.retKey) != NULL) {
            data.iskey = 1;
         } else {
             Tcl_AppendResult(interp, "-return is not a key or a percent subst", 0);
             result = TCL_ERROR;
             goto done;
         }
    }

    if (data.flags & MATCH_RELDEPTH) {
        int dep;
        dep = Blt_TreeNodeDepth(cmdPtr->tree, node);
        if (data.maxDepth >= 0) {
            data.maxDepth += dep;
        }
        if (data.minDepth >= 0) {
            data.minDepth += dep;
        }
        if (data.depth >= 0) {
            data.depth += dep;
        }
    }
    /*if (data.maxDepth >= 0) {
	data.maxDepth += Blt_TreeNodeDepth(cmdPtr->tree, node);
    }*/
    /*if (data.flags & MATCH_NOCASE) {
	Blt_ListNode listNode;

	for (listNode = Blt_ListFirstNode(data.patternList); listNode != NULL;
	     listNode = Blt_ListNextNode(listNode)) {
	    strtolower((char *)Blt_ListGetKey(listNode));
	}
    }*/
    if (data.name != NULL && (data.flags & (MATCH_ARRAY))) {
        Tcl_AppendResult(interp, "-array must not be used with -name", 0);
        result = TCL_ERROR;
        goto done;
    }
    if (data.name == NULL && (data.flags & (PATTERN_REGEXP|PATTERN_GLOB|PATTERN_INLIST))) {
        Tcl_AppendResult(interp, "must provide -name with -regexp -glob -inlist", 0);
        result = TCL_ERROR;
        goto done;
    }
    if ((data.flags & (MATCH_ISNULL))) {
        if (data.subKey == NULL) {
            Tcl_AppendResult(interp, "-isempty must be used with -column", 0);
            result = TCL_ERROR;
            goto done;
        }
        if (data.name != NULL) {
            Tcl_AppendResult(interp, "-isempty can not be used with -name", 0);
            result = TCL_ERROR;
            goto done;
        }
        if ((data.flags & (MATCH_INVERT))) {
            Tcl_AppendResult(interp, "-isempty can not be used with -invert", 0);
            result = TCL_ERROR;
            goto done;
        }
        if (data.retKey) {
            Tcl_AppendResult(interp, "-isempty can not be used with -return", 0);
            result = TCL_ERROR;
            goto done;
        }
        if (data.command) {
            Tcl_AppendResult(interp, "-isempty can not be used with -command", 0);
            result = TCL_ERROR;
            goto done;
        }
    }
    if ((data.flags & PATTERN_MASK) == PATTERN_REGEXP) {
        if (Tcl_RegExpMatch(interp, "", Tcl_GetString(data.name)) == -1) {
            result = TCL_ERROR;
            goto done;
        }
    }
    if ((data.flags & PATTERN_MASK) == PATTERN_INLIST) {
        int cobjc;
        Tcl_Obj **cobjv;
        if (Tcl_ListObjGetElements(interp, data.name,
            &cobjc, &cobjv) != TCL_OK) {
            result = TCL_ERROR;
            goto done;
        }
    }
    if (data.addTag != NULL) {
        if (AddTag(cmdPtr, NULL, data.addTag) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (data.command != NULL) {
	int count, acnt;
	char **p;
	register int i;

	count = 0;
	acnt = 0;
	for (p = data.command; *p != NULL; p++) {
	    count++;
	}
        for (p = data.cmdArgs; *p != NULL; p++) {
            
            acnt++;
        }
	/* Leave room for node Id argument to be appended */
	objArr = Blt_Calloc(count + acnt + 2, sizeof(Tcl_Obj *));
	for (i = 0; i < count; i++) {
	    objArr[i] = Tcl_NewStringObj(data.command[i], -1);
	    Tcl_IncrRefCount(objArr[i]);
	}
        objArr[i] = Tcl_NewStringObj("", -1);
        Tcl_IncrRefCount(objArr[i]);
        i++;
        for (; i < (count+acnt+1); i++) {
	    objArr[i] = Tcl_NewStringObj("", 0);
	    Tcl_IncrRefCount(objArr[i]);
	}
	data.objv = objArr;
	data.objc = count + 1;
	data.argc = acnt;
    } else if (data.cmdArgs != NULL) {
        Tcl_AppendResult(interp, "-cmdargs must be used with -command", 0);
        result = TCL_ERROR;
        goto done;
    }
    data.listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    data.cmdPtr = cmdPtr;
    result = TCL_OK;
    if (data.nodesObj != NULL) {
        TagSearch tagIter = {0};

        if (FindTaggedNodes(interp, cmdPtr, data.nodesObj, &tagIter) != TCL_OK) {
            result = TCL_ERROR;
            goto done;
        }
        for (child = FirstTaggedNode(&tagIter);
            child != NULL && result == TCL_OK;
            child = NextTaggedNode(child, &tagIter)) {
            result = MatchNodeProc(child, &data, -1);
        }
        DoneTaggedNodes(&tagIter);
        
    } else if (data.order == TREE_BREADTHFIRST) {
	result = Blt_TreeApplyBFS(node, MatchNodeProc, &data);
    } else {
	result = Blt_TreeApplyDFS(node, MatchNodeProc, &data, data.order);
    }
    if (data.command != NULL) {
	Tcl_Obj **objPtrPtr;

	for (objPtrPtr = objArr; *objPtrPtr != NULL; objPtrPtr++) {
	    Tcl_DecrRefCount(*objPtrPtr);
	}
	Blt_Free(objArr);
    }
done:
    Blt_FreeSwitches(interp, findSwitches, (char *)&data, 0);
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (data.flags & MATCH_COUNT) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(data.nMatches));
    } else {
        Tcl_SetObjResult(interp, data.listObjPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FindChildOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
FindChildOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node, child;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    child = Blt_TreeFindChild(node, Tcl_GetString(objv[3]));
    if (child != NULL) {
	inode = Blt_TreeNodeId(child);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FirstChildOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
FirstChildOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreeFirstChild(node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
GetOp(
    TreeCmd *cmdPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;

    if (objc>2) {
        if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        node = Blt_TreeRootNode(cmdPtr->tree);
    }
    if (Blt_TreeNotifyGet(cmdPtr->tree, node) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc <= 3) {
	Blt_TreeKey key;
	Tcl_Obj *valueObjPtr, *listObjPtr;
	Blt_TreeKeySearch cursor;

	/* Add the key-value pairs to a new Tcl_Obj */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &cursor); key != NULL; 
	     key = Blt_TreeNextKey(cmdPtr->tree, &cursor)) {
	    if (Blt_TreeGetValue((Tcl_Interp *)NULL, cmdPtr->tree, node, key,
				 &valueObjPtr) == TCL_OK) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewStringObj(key, -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		Tcl_ListObjAppendElement(interp, listObjPtr, valueObjPtr);
	    }
	}	    
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    } else {
	Tcl_Obj *valueObjPtr;
	char *string;

	string = Tcl_GetString(objv[3]); 
	if (Blt_TreeGetValue((Tcl_Interp *)NULL, cmdPtr->tree, node, string,
		     &valueObjPtr) != TCL_OK) {
	    if (objc == 4) {
		Tcl_DString dString;
		char *path;

		Tcl_DStringInit(&dString);
		path = (cmdPtr->tree == NULL ? "" : GetNodePath(cmdPtr, Blt_TreeRootNode(cmdPtr->tree), node, FALSE, &dString));		
		Tcl_AppendResult(interp, "can't find field \"", string, 
			"\" in \"", path, "\"", (char *)NULL);
		Tcl_DStringFree(&dString);
		return TCL_ERROR;
	    } 
	    /* Default to given value */
	    valueObjPtr = objv[4];
	} 
	Tcl_SetObjResult(interp, valueObjPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IndexOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
IndexOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    inode = -1;
    if (GetNode(cmdPtr, objv[2], &node) == TCL_OK) {
	inode = Blt_TreeNodeId(node);
    } else {
	register int i;
	int nObjs;
	Tcl_Obj **objArr;
	Blt_TreeNode parent;
	char *string;

	if (Tcl_ListObjGetElements(interp, objv[2], &nObjs, &objArr) 
	    != TCL_OK) {
	    goto done;		/* Can't split object. */
	}
	parent = Blt_TreeRootNode(cmdPtr->tree);
	for (i = 0; i < nObjs; i++) {
	    string = Tcl_GetString(objArr[i]);
	    if (string[0] == '\0') {
		continue;
	    }
	    node = Blt_TreeFindChild(parent, string);
	    if (node == NULL) {
		goto done;	/* Can't find component */
	    }
	    parent = node;
	}
	inode = Blt_TreeNodeId(node);
    }
 done:
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * InsertOp --
 *
 *---------------------------------------------------------------------- 
 */

static int
InsertOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode parent, child;
    InsertData data;
    int inode, dobjc, tobjc, pobjc, nobjc, vobjc, i;
    Tcl_Obj **dobjv, **pobjv, **tobjv, **nobjv, **vobjv;

    child = NULL;
    /*if (!strcmp(Tcl_GetString(objv[2]), "end")) {
        parent = Blt_TreeRootNode(cmdPtr->tree);
    } else*/
    if (GetNode(cmdPtr, objv[2], &parent) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Initialize switch flags */
    memset(&data, 0, sizeof(data));
    data.insertPos = -1;	/* Default to append node. */
    data.parent = parent;
    data.inode = -1;

    if (Blt_ProcessObjSwitches(interp, insertSwitches, objc - 3, objv + 3, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	goto error;
    }
    if (data.tags != NULL) {
        if (Tcl_ListObjGetElements(interp, data.tags, &pobjc, &pobjv) 
        != TCL_OK) {
            goto error;		/* Can't split object. */
        }
    }
    if (data.tags2 != NULL) {
        if (Tcl_ListObjGetElements(interp, data.tags2, &tobjc, &tobjv) 
        != TCL_OK) {
            goto error;		/* Can't split object. */
        }
    }
    if (data.dataPairs != NULL) {
        if (Tcl_ListObjGetElements(interp, data.dataPairs, &dobjc, &dobjv) 
            != TCL_OK) {
            goto error;		/* Can't split object. */
        }
        if (dobjc%2) {
            Tcl_AppendResult(interp, "-data must have an even number of values",0);
            goto error;
        }
        if (data.names != NULL || data.values != NULL) {
            Tcl_AppendResult(interp, "-data incompatible with -names/-values",0);
            goto error;
        }
    }
    if (data.names != NULL) {
        if (Tcl_ListObjGetElements(interp, data.names, &nobjc, &nobjv) 
        != TCL_OK) {
            goto error;		/* Can't split object. */
        }
        if (data.dataPairs != NULL) {
            Tcl_AppendResult(interp, "-data incompatible with -names/-values",0);
            goto error;
        }
        if (data.values == NULL) {
            Tcl_AppendResult(interp, "-names must be used with -values",0);
            goto error;
        }
    }
    if (data.values != NULL) {
        if (Tcl_ListObjGetElements(interp, data.values, &vobjc, &vobjv) 
        != TCL_OK) {
            goto error;		/* Can't split object. */
        }
        if (data.dataPairs != NULL) {
            Tcl_AppendResult(interp, "-data incompatible with -names/-values",0);
            goto error;
        }
        if (data.names == NULL) {
            Tcl_AppendResult(interp, "-values must be used with -names",0);
            goto error;
        }
        if (vobjc != nobjc) {
            Tcl_AppendResult(interp, "-values and -names must be the same length",0);
            goto error;
        }
    }
    if (data.inode > 0) {
	Blt_TreeNode node;

	node = Blt_TreeGetNode(cmdPtr->tree, data.inode);
	if (node != NULL) {
	    Tcl_AppendResult(interp, "can't reissue node id \"", 
		Blt_Itoa(data.inode), "\": already exists.", (char *)NULL);
	    goto error;
	}
	child = Blt_TreeCreateNodeWithId(cmdPtr->tree, parent, data.label, 
		data.inode, data.insertPos);
    } else {
	child = Blt_TreeCreateNode(cmdPtr->tree, parent, data.label, 
		data.insertPos);
    }
    if (child == NULL) {
	Tcl_AppendResult(interp, "can't allocate new node", (char *)NULL);
	goto error;
    }
    inode = child->inode;
    if (data.label == NULL) {
	char string[200];

	sprintf(string, "%d", Blt_TreeNodeId(child));
	Blt_TreeRelabelNode2(child, string);
    } 
    if (data.tags != NULL) {

         for (i=0; i<pobjc; i++) {
	    if (AddTag(cmdPtr, child, Tcl_GetString(pobjv[i])) != TCL_OK) {
		goto error;
	    }
	}
    }
    if (data.dataPairs != NULL) {
	char *key;

	for (i=0; i<dobjc; i += 2) {
	    key = Tcl_GetString(dobjv[i]);
            if (Blt_TreeSetValue(interp, cmdPtr->tree, child, key, dobjv[i+1]) 
		!= TCL_OK) {
		goto error;
	    }
         }
    } else if (data.names != NULL) {
        char *key;

        for (i=0; i<nobjc; i++) {
            key = Tcl_GetString(nobjv[i]);
            if (Blt_TreeSetValue(interp, cmdPtr->tree, child, key, vobjv[i]) 
            != TCL_OK) {
                goto error;
            }
        }
    }
    if (data.tags2 != NULL) {
         for (i=0; i<tobjc; i++) {
	    if (AddTag(cmdPtr, child, Tcl_GetString(tobjv[i])) != TCL_OK) {
                goto error;
            }
        }
    }
    if (Blt_TreeInsertPost(cmdPtr->tree, child) == NULL) {
        goto error;
    }
    
    if (data.fixed || (cmdPtr->tree->treeObject->flags & TREE_FIXED_KEYS)) {
        child->flags |= TREE_NODE_FIXED_FIELDS;
    }

    Blt_FreeSwitches(interp, insertSwitches, (char *)&data, 0);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), Blt_TreeNodeId(child));
    return TCL_OK;

 error:
    if (child != NULL) {
        child = Blt_TreeGetNode(cmdPtr->tree, inode);
    }
    if (child != NULL) {
	DeleteNode(cmdPtr, child);
    }
    Blt_FreeSwitches(interp, insertSwitches, (char *)&data, 0);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateOp --
 *
 *---------------------------------------------------------------------- 
 */

static int
CreateOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode parent = NULL, child = NULL;
    int cnt = 0, i, n, m, vobjc, tobjc, dobjc, pobjc, optInd, iPos = -1, incr = 0;
    int hasoffs = 0, seqlabel = 0, seqVal = 0, hasstart = 0, start;
    int fixed = 0, hcnt = 0;
    char *prefix = NULL, *string;
    Tcl_Obj **vobjv = NULL;
    Tcl_Obj **tobjv = NULL;
    Tcl_Obj **dobjv = NULL;
    Tcl_Obj **pobjv = NULL;

    enum optInd {
        OP_DATA, OP_FIXED, OP_SEQLABEL, OP_NODES, OP_NUM, OP_OFFSET,
        OP_PARENT, OP_PATH, OP_POS, OP_PREFIX, OP_START, OP_TAGS
    };
    static char *optArr[] = {
        "-data", "-fixed", "-labelstart", "-nodes", "-num", "-offset",
        "-parent", "-path", "-pos", "-prefix", "-start", "-tags",
        0
    };

    i = 1;
    
    string = Tcl_GetString(objv[2]);
    while (objc>=3 && string[0] == '-') {
        if (Tcl_GetIndexFromObj(interp, objv[2], optArr, "option",
            0, &optInd) != TCL_OK) {
                return TCL_ERROR;
        }
        switch (optInd) {
        
        case OP_POS:
            if (objc<4) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &iPos) != TCL_OK) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_DATA:
            if (objc<4) { goto missingArg; }
            if (Tcl_ListObjGetElements(interp, objv[3], &dobjc, &dobjv) 
                != TCL_OK) {
                return TCL_ERROR;		/* Can't split object. */
            }
            if (dobjc%2) {
                Tcl_AppendResult(interp, "data must have even length", (char *)NULL);
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_FIXED:
            fixed = 1;
            objc -= 1;
            objv += 1;
            break;
        case OP_SEQLABEL:
            if (objc<4) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &seqVal) != TCL_OK) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            seqlabel = 1;
            break;
        case OP_OFFSET:
            if (objc<4) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &incr) != TCL_OK) {
                return TCL_ERROR;
            }
            hasoffs = 1;
            objc -= 2;
            objv += 2;
            break;
        case OP_NUM:
            if (objc<4) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &cnt) != TCL_OK) {
                return TCL_ERROR;
            }
            hcnt++;
            if (cnt<0 || cnt>10000000) {
                Tcl_AppendResult(interp, "count must be >= 0 and <= 10M", (char *)NULL);
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_NODES:
            hcnt++;
            if (objc<4) { goto missingArg; }
            if (Tcl_ListObjGetElements(interp, objv[3], &vobjc, &vobjv) 
                != TCL_OK) {
                return TCL_ERROR;		/* Can't split object. */
            }
            for (i=0; i<vobjc; i++)  {
                if (Tcl_GetIntFromObj(interp, vobjv[i], &n)) {
                    return TCL_ERROR;
                }
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_PARENT:
            if (objc<4) { goto missingArg; }
            if (GetNode(cmdPtr, objv[3], &parent) != TCL_OK) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_PREFIX:
            if (objc<4) { goto missingArg; }
            prefix = Tcl_GetString(objv[3]);
            objc -= 2;
            objv += 2;
            break;
        case OP_PATH:
            if (objc<4) { goto missingArg; }
            hcnt++;
            if (Tcl_ListObjGetElements(interp, objv[3], &pobjc, &pobjv) 
                != TCL_OK) {
                return TCL_ERROR;		/* Can't split object. */
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_START:
            if (objc<4) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &start) != TCL_OK) {
                return TCL_ERROR;
            }
            if (start<=0) {
                Tcl_AppendResult(interp, "start must be > 0", (char *)NULL);
                return TCL_ERROR;
            }
            hasstart = 1;
            objc -= 2;
            objv += 2;
            break;
        case OP_TAGS:
            if (objc<4) { goto missingArg; }
            if (Tcl_ListObjGetElements(interp, objv[3], &tobjc, &tobjv) 
                != TCL_OK) {
                return TCL_ERROR;		/* Can't split object. */
            }
            objc -= 2;
            objv += 2;
            break;
        default:
            return TCL_ERROR;
        }
    }
    if (hcnt != 1) {
        Tcl_AppendResult(interp, "must use exactly one of -nodes, -num or -path", 0);
        return TCL_ERROR;
    }
    if (vobjv != NULL) {
        cnt = vobjc;
    }
    if (pobjv != NULL) {
        cnt = pobjc;
        if (prefix != NULL) {
            Tcl_AppendResult(interp, "-prefix is incompatible with -path", 0);
            return TCL_ERROR;
        }
        if (seqlabel) {
            Tcl_AppendResult(interp, "-labelstart is incompatible with -path", 0);
            return TCL_ERROR;
        }
        if (parent != NULL) {
            Tcl_AppendResult(interp, "-parent is incompatible with -path", 0);
            return TCL_ERROR;
        }
    }
    if (hasoffs && vobjv == NULL) {
        Tcl_AppendResult(interp, "must use -nodes with -offset", 0);
        return TCL_ERROR;
    }

    if (parent == NULL) {
         parent = Blt_TreeRootNode(cmdPtr->tree);
    }
    if (prefix == NULL) {
        prefix = "";
    }

    for (n = 0, m = 0; n<cnt; n++, i++)  {
        char label[200];
        char *labStr;
        labStr = NULL;
        if (vobjv != NULL) {
            if (Tcl_GetIntFromObj(interp, vobjv[n], &i)) {
                return TCL_ERROR;
            }
            i += incr;
        }
        if (pobjv != NULL) {
            labStr = Tcl_GetString(pobjv[n]);
            child = Blt_TreeFindChild( parent, labStr);
            if (child != NULL) {
                parent = child;
                continue;
            }
        }
        if (vobjv != NULL) {
            child = Blt_TreeCreateNodeWithId(cmdPtr->tree, parent, labStr, i, iPos);
        } else if (hasstart) {
            child = Blt_TreeCreateNodeWithId(cmdPtr->tree, parent, labStr, n+start, iPos);
        } else {
            child = Blt_TreeCreateNode(cmdPtr->tree, parent, labStr, iPos);
        }
        if (child == NULL) {
            return TCL_ERROR;
        }
        if (pobjv != NULL) {
            parent = child;
        }
        if (labStr == NULL) {
            if (!seqlabel) {
                i = Blt_TreeNodeId(child);
            } else {
                i = n + seqVal;
            }
            if (prefix[0]) {
                sprintf(label, "%.100s%d", prefix, i);
            } else {
                sprintf(label, "%d", i);
            }
            Blt_TreeRelabelNode2(child, label);
        }
        if (dobjv != NULL) {
            int j;
            Blt_TreeKey key;
            for (j = 0; j<dobjc; j += 2) {
                key = Tcl_GetString(dobjv[j]);
                if (Blt_TreeSetValue(interp, cmdPtr->tree, child, key, dobjv[j+1]) 
                != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }
        if (tobjv != NULL) {
            int j;
            for (j = 0; j<tobjc; j++) {
                if (AddTag(cmdPtr, child, Tcl_GetString(tobjv[j])) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }
        if (Blt_TreeInsertPost(cmdPtr->tree, child) == NULL) {
            DeleteNode(cmdPtr, child);
            return TCL_ERROR;
        }
        if (fixed || (cmdPtr->tree->treeObject->flags & TREE_FIXED_KEYS)) {
            child->flags |= TREE_NODE_FIXED_FIELDS;
        }
        m++;
    }
    if (child != NULL) {
        Tcl_AppendResult(interp, Blt_Itoa(Blt_TreeNodeId(child)), 0);
    }
    return TCL_OK;

missingArg:
    Tcl_AppendResult(interp, "missing argument for populate option \"",
        Tcl_GetString(objv[3]), "\"", (char *)NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * IsAncestorOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
IsAncestorOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node1, node2;
    int bool;

    if ((GetNode(cmdPtr, objv[3], &node1) != TCL_OK) ||
	(GetNode(cmdPtr, objv[4], &node2) != TCL_OK)) {
	return TCL_ERROR;
    }
    bool = Blt_TreeIsAncestor(node1, node2);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IsBeforeOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
IsBeforeOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node1, node2;
    int bool;

    if ((GetNode(cmdPtr, objv[3], &node1) != TCL_OK) ||
	(GetNode(cmdPtr, objv[4], &node2) != TCL_OK)) {
	return TCL_ERROR;
    }
    bool = Blt_TreeIsBefore(node1, node2);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IsLeafOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
IsLeafOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;

    if (GetNode(cmdPtr, objv[3], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), Blt_TreeIsLeaf(node));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IsRootOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
IsRootOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int bool;

    if (GetNode(cmdPtr, objv[3], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = (node == Blt_TreeRootNode(cmdPtr->tree));
    Tcl_SetIntObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IsOp --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec isOps[] =
{
    {"ancestor", 1, (Blt_Op)IsAncestorOp, 5, 5, "node1 node2",},
    {"before", 1, (Blt_Op)IsBeforeOp, 5, 5, "node1 node2",},
    {"leaf", 1, (Blt_Op)IsLeafOp, 4, 4, "node",},
    {"root", 1, (Blt_Op)IsRootOp, 4, 4, "node",},
};

static int nIsOps = sizeof(isOps) / sizeof(Blt_OpSpec);

static int
IsOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nIsOps, isOps, BLT_OP_ARG2, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * KeysOp --
 *
 *	Returns the key names of values for a node or array value.
 *
 *---------------------------------------------------------------------- 
 */
static int
KeysOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_HashTable keyTable;
    Blt_TreeKey key;
    Blt_TreeKeySearch keyIter;
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr, *objPtr;
    register int i;
    int isNew, len;
    /*char *string;*/
    TagSearch tagIter = {0};

    Blt_InitHashTableWithPool(&keyTable, BLT_ONE_WORD_KEYS);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (i = 2; i < objc; i++) {
        /*string =*/ Tcl_GetStringFromObj(objv[i],&len);
        if (len == 0) continue;
        if (FindTaggedNodes(interp, cmdPtr, objv[i], &tagIter) != TCL_OK) {
            Blt_DeleteHashTable(&keyTable);
            Tcl_DecrRefCount(listObjPtr);
            return TCL_ERROR;
	}
	for ( node = FirstTaggedNode(&tagIter);
	 node != NULL; node = NextTaggedNode(node, &tagIter)) {
	    for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &keyIter); 
		 key != NULL; key = Blt_TreeNextKey(cmdPtr->tree, &keyIter)) {
		Blt_CreateHashEntry(&keyTable, key, &isNew);
                if (!isNew) continue;
                objPtr = Tcl_NewStringObj(key, -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
        DoneTaggedNodes(&tagIter);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Blt_DeleteHashTable(&keyTable);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * LabelOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
LabelOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 4) {
	if (Blt_TreeRelabelNode(cmdPtr->tree, node, Tcl_GetString(objv[3])) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Blt_TreeNodeLabel(node), -1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FixedOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
FixedOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int fixed;

    if (strlen(Tcl_GetString(objv[2]))) {
        if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc != 4) {
            fixed = ((node->flags & TREE_NODE_FIXED_FIELDS) != 0);
        } else {
            if (Tcl_GetIntFromObj(interp, objv[3], &fixed) != TCL_OK) {
                return TCL_ERROR;
            }
            if (fixed) {
                node->flags |= TREE_NODE_FIXED_FIELDS;
            } else {
                node->flags &= ~TREE_NODE_FIXED_FIELDS;
            }
        }
    } else {
        if (objc != 3) {
            fixed = ((cmdPtr->tree->treeObject->flags & TREE_FIXED_KEYS) != 0);
        } else {
            if (Tcl_GetIntFromObj(interp, objv[2], &fixed) != TCL_OK) {
                return TCL_ERROR;
            }
            if (fixed) {
                cmdPtr->tree->treeObject->flags |= TREE_FIXED_KEYS;
            } else {
                cmdPtr->tree->treeObject->flags &= ~TREE_FIXED_KEYS;
            }
        }
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(fixed));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DictsetOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
DictsetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    int dict;
    

    if (objc != 3) {
        dict = ((cmdPtr->tree->treeObject->flags & TREE_DICT_KEYS) != 0);
    } else {
	if (Tcl_GetIntFromObj(interp, objv[2], &dict) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (dict) {
             cmdPtr->tree->treeObject->flags |= TREE_DICT_KEYS;
	} else {
             cmdPtr->tree->treeObject->flags &= ~TREE_DICT_KEYS;
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(dict));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * LastChildOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
LastChildOp(cmdPtr, interp, objc, objv)
    TreeCmd *cmdPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreeLastChild(node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * MoveOp --
 *
 *	The big trick here is to not consider the node to be moved in
 *	determining it's new location.  Ideally, you would temporarily
 *	pull from the tree and replace it (back in its old location if
 *	something went wrong), but you could still pick the node by 
 *	its serial number.  So here we make lots of checks for the 
 *	node to be moved.
 * 
 *
 *---------------------------------------------------------------------- 
 */
static int
MoveOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode root, parent, node;
    Blt_TreeNode before;
    MoveData data;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetNode(cmdPtr, objv[3], &parent) != TCL_OK) {
	return TCL_ERROR;
    }
    root = Blt_TreeRootNode(cmdPtr->tree);
    if (node == root) {
	Tcl_AppendResult(interp, "can't move root node", (char *)NULL);
	return TCL_ERROR;
    }
    if (parent == node) {
	Tcl_AppendResult(interp, "can't move node to self", (char *)NULL);
	return TCL_ERROR;
    }
    data.node = NULL;
    data.cmdPtr = cmdPtr;
    data.movePos = -1;
    /* Process switches  */
    if (Blt_ProcessObjSwitches(interp, moveSwitches, objc - 4, objv + 4, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	return TCL_ERROR;
    }
    /* Verify they aren't ancestors. */
    if (Blt_TreeIsAncestor(node, parent)) {
	Tcl_AppendResult(interp, "can't move node: \"", 
		 Tcl_GetString(objv[2]), (char *)NULL);
	Tcl_AppendResult(interp, "\" is an ancestor of \"", 
		 Tcl_GetString(objv[3]), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    before = NULL;		/* If before is NULL, this appends the
				 * node to the parent's child list.  */

    if (data.node != NULL) {	/* -before or -after */
	if (Blt_TreeNodeParent(data.node) != parent) {
	    Tcl_AppendResult(interp, Tcl_GetString(objv[2]), 
		     " isn't the parent of ", Blt_TreeNodeLabel(data.node),
		     (char *)NULL);
	    return TCL_ERROR;
	}
	if (Blt_SwitchChanged(moveSwitches, interp, "-before", (char *)NULL)) {
	    before = data.node;
	    if (before == node) {
		Tcl_AppendResult(interp, "can't move node before itself", 
				 (char *)NULL);
		return TCL_ERROR;
	    }
	} else {
	    before = Blt_TreeNextSibling(data.node);
	    if (before == node) {
		Tcl_AppendResult(interp, "can't move node after itself", 
				 (char *)NULL);
		return TCL_ERROR;
	    }
	}
    } else if (data.movePos >= 0) { /* -at */
	int count;		/* Tracks the current list index. */
	Blt_TreeNode child;

	/* 
	 * If the node is in the list, ignore it when determining the
	 * "before" node using the -at index.  An index of -1 means to
	 * append the node to the list.
	 */
	count = 0;
	for(child = Blt_TreeFirstChild(parent); child != NULL; 
	    child = Blt_TreeNextSibling(child)) {
	    if (child == node) {
		continue;	/* Ignore the node to be moved. */
	    }
	    if (count == data.movePos) {
		before = child;
		break;		
	    }
	    count++;	
	}
    }
    if (Blt_TreeMoveNode(cmdPtr->tree, node, parent, before) != TCL_OK) {
	Tcl_AppendResult(interp, "can't move node ", Tcl_GetString(objv[2]), 
		 " to ", Tcl_GetString(objv[3]), (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NextOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NextOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreeNextNode(Blt_TreeRootNode(cmdPtr->tree), node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NextSiblingOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NextSiblingOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreeNextSibling(node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifyCreateOp --
 *
 *	tree0 notify create ?flags? command arg
 *---------------------------------------------------------------------- 
 */
static int
NotifyCreateOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    NotifyInfo *notifyPtr;
    NotifyData data;
    char *string;
    char idString[200];
    int isNew, nArgs;
    Blt_HashEntry *hPtr;
    int count;
    register int i;

    count = 0;
    for (i = 3; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (string[0] != '-') {
	    break;
	}
	count++;
    }
    data.mask = 0;
    /* Process switches  */
    if (Blt_ProcessObjSwitches(interp, notifySwitches, count, objv + 3, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	return TCL_ERROR;
    }
    notifyPtr = Blt_Calloc(1, sizeof(NotifyInfo));

    nArgs = objc - i;

    /* Stash away the command in structure and pass that to the notifier. */
    notifyPtr->objv = Blt_Calloc((nArgs + 2), sizeof(Tcl_Obj *));
    for (count = 0; i < objc; i++, count++) {
	Tcl_IncrRefCount(objv[i]);
	notifyPtr->objv[count] = objv[i];
    }
    notifyPtr->objc = nArgs + 2;
    notifyPtr->cmdPtr = cmdPtr;
    if (data.mask == 0) {
	data.mask = TREE_NOTIFY_ALL;
    }
    notifyPtr->mask = data.mask;

    sprintf(idString, "notify%d", cmdPtr->notifyCounter++);
    hPtr = Blt_CreateHashEntry(&(cmdPtr->notifyTable), idString, &isNew);
    Blt_SetHashValue(hPtr, notifyPtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), idString, -1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifyDeleteOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
NotifyDeleteOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    NotifyInfo *notifyPtr;
    Blt_HashEntry *hPtr;
    register int i, j;
    char *string;

    for (i = 3; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	hPtr = Blt_FindHashEntry(&(cmdPtr->notifyTable), string);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "unknown notify name \"", string, "\"", 
			     (char *)NULL);
	    return TCL_ERROR;
	}
	notifyPtr = Blt_GetHashValue(hPtr);
	Blt_DeleteHashEntry(&(cmdPtr->notifyTable), hPtr);
	for (j = 0; j < (notifyPtr->objc - 2); j++) {
	    Tcl_DecrRefCount(notifyPtr->objv[j]);
	}
	Blt_Free(notifyPtr->objv);
	Blt_Free(notifyPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifyInfoOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NotifyInfoOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    NotifyInfo *notifyPtr;
    Blt_HashEntry *hPtr;
    Tcl_DString dString;
    char *string;
    int i;

    string = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&(cmdPtr->notifyTable), string);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "unknown notify name \"", string, "\"", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    notifyPtr = Blt_GetHashValue(hPtr);

    Tcl_DStringInit(&dString);
    Tcl_DStringAppendElement(&dString, string);	/* Copy notify Id */
    Tcl_DStringStartSublist(&dString);
    if (notifyPtr->mask & TREE_NOTIFY_CREATE) {
	Tcl_DStringAppendElement(&dString, "-create");
    }
    if (notifyPtr->mask & TREE_NOTIFY_GET) {
        Tcl_DStringAppendElement(&dString, "-get");
    }
    if (notifyPtr->mask & TREE_NOTIFY_INSERT) {
        Tcl_DStringAppendElement(&dString, "-insert");
    }
    if (notifyPtr->mask & TREE_NOTIFY_DELETE) {
	Tcl_DStringAppendElement(&dString, "-delete");
    }
    if (notifyPtr->mask & TREE_NOTIFY_MOVE) {
	Tcl_DStringAppendElement(&dString, "-move");
    }
    if (notifyPtr->mask & TREE_NOTIFY_MOVEPOST) {
        Tcl_DStringAppendElement(&dString, "-movepost");
    }
    if (notifyPtr->mask & TREE_NOTIFY_SORT) {
	Tcl_DStringAppendElement(&dString, "-sort");
    }
    if (notifyPtr->mask & TREE_NOTIFY_RELABEL) {
	Tcl_DStringAppendElement(&dString, "-relabel");
    }
    if (notifyPtr->mask & TREE_NOTIFY_RELABELPOST) {
        Tcl_DStringAppendElement(&dString, "-relabelpost");
    }
    if (notifyPtr->mask & TREE_NOTIFY_WHENIDLE) {
        Tcl_DStringAppendElement(&dString, "-whenidle");
    }
    if (notifyPtr->mask & TREE_NOTIFY_TRACEACTIVE) {
        Tcl_DStringAppendElement(&dString, "-disabletrace");
    }
    if (notifyPtr->mask & TREE_NOTIFY_BGERROR) {
        Tcl_DStringAppendElement(&dString, "-bgerror");
    }
    Tcl_DStringEndSublist(&dString);
    Tcl_DStringStartSublist(&dString);
    for (i = 0; i < (notifyPtr->objc - 2); i++) {
	Tcl_DStringAppendElement(&dString, Tcl_GetString(notifyPtr->objv[i]));
    }
    Tcl_DStringEndSublist(&dString);
    Tcl_DStringResult(interp, &dString);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifyNamesOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NotifyNamesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *objPtr, *listObjPtr;
    char *notifyId;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&(cmdPtr->notifyTable), &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	notifyId = Blt_GetHashKey(&(cmdPtr->notifyTable), hPtr);
	objPtr = Tcl_NewStringObj(notifyId, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifyOp --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec notifyOps[] =
{
    {"create", 1, (Blt_Op)NotifyCreateOp, 4, 0, "?flags? command",},
    {"delete", 1, (Blt_Op)NotifyDeleteOp, 3, 0, "notifyId...",},
    {"info", 1, (Blt_Op)NotifyInfoOp, 4, 4, "notifyId",},
    {"names", 1, (Blt_Op)NotifyNamesOp, 3, 3, "",},
};

static int nNotifyOps = sizeof(notifyOps) / sizeof(Blt_OpSpec);

static int
NotifyOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nNotifyOps, notifyOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}


/*ARGSUSED*/
static int
ParentOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreeNodeParent(node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * PathOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
PathOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    Tcl_DString dString;
    char *prefix = NULL, *delim = NULL;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc >3) {
        delim = Tcl_GetString(objv[3]);
    }
    if (objc >4) {
        prefix = Tcl_GetString(objv[4]);
    }
    Tcl_DStringInit(&dString);
    if (objc>3) {
        Blt_TreeNodePathStr( node, &dString, prefix, delim);
    } else {
        GetNodePath(cmdPtr, Blt_TreeRootNode(cmdPtr->tree), node, FALSE, &dString); 
    }
    Tcl_DStringResult(interp, &dString);
    return TCL_OK;
}


static int
ComparePositions(Blt_TreeNode *n1Ptr, Blt_TreeNode *n2Ptr)
{
    if (*n1Ptr == *n2Ptr) {
        return 0;
    }
    if (Blt_TreeIsBefore(*n1Ptr, *n2Ptr)) {
        return -1;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * PositionOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
PositionOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    PositionData data;
    Blt_TreeNode *nodeArr, *nodePtr;
    Blt_TreeNode node;
    Blt_TreeNode parent, lastParent;
    Tcl_Obj *listObjPtr, *objPtr;
    int i;
    int position;
    Tcl_DString dString;
    int n;

    Tcl_DStringInit(&dString);
    memset((char *)&data, 0, sizeof(data));
    /* Process switches  */
    n = Blt_ProcessObjSwitches(interp, positionSwitches, objc - 2, objv + 2, 
	     (char *)&data, BLT_SWITCH_EXACT|BLT_SWITCH_OBJV_PARTIAL);
    if (n < 0) {
	return TCL_ERROR;
    }
    objc -= n + 2, objv += n + 2;

    /* Collect the node ids into an array */
    nodeArr = Blt_Calloc((objc + 1),  sizeof(Blt_TreeNode));
    for (i = 0; i < objc; i++) {
	if (GetNode(cmdPtr, objv[i], &node) != TCL_OK) {
	    Blt_Free(nodeArr);
	    return TCL_ERROR;
	}
	nodeArr[i] = node;
    }
    nodeArr[i] = NULL;

    if (data.sort) {		/* Sort the nodes by depth-first order 
				 * if requested. */
	qsort((char *)nodeArr, objc, sizeof(Blt_TreeNode), 
	      (QSortCompareProc *)ComparePositions);
    }

    position = 0;		/* Suppress compiler warning. */
    lastParent = NULL;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_DStringInit(&dString);
    for (nodePtr = nodeArr; *nodePtr != NULL; nodePtr++) {
	parent = Blt_TreeNodeParent(*nodePtr);
	if ((parent != NULL) && (parent == lastParent)) {
	    /* 
	     * Since we've sorted the nodes already, we can safely
	     * assume that if two consecutive nodes have the same
	     * parent, the first node came before the second. If
	     * this is the case, use the last node as a starting
	     * point.  
	     */
	    
	    /*
	     * Note that we start comparing from the last node,
	     * not its successor.  Some one may give us the same
	     * node more than once.  
	     */
	    node = *(nodePtr - 1); /* Can't get here unless there's
				    * more than one node. */
	    for(/*empty*/; node != NULL; node = Blt_TreeNextSibling(node)) {
		if (node == *nodePtr) {
		    break;
		}
		position++;
	    }
	} else {
	    /* The fallback is to linearly search through the
	     * parent's list of children, counting the number of
	     * preceding siblings. Except for nodes with many
	     * siblings (100+), this should be okay. */
	    position = Blt_TreeNodePosition(*nodePtr);
	}
	if (data.sort) {
	    lastParent = parent; /* Update the last parent. */
	}	    
	/* 
	 * Add an element in the form "parent -at position" to the
	 * list that we're generating.
	 */
	if (data.withId) {
	    objPtr = Tcl_NewIntObj(Blt_TreeNodeId(*nodePtr));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	if (data.withParent) {
	    char *string;

	    Tcl_DStringSetLength(&dString, 0); /* Clear the string. */
	    string = (parent == NULL) ? "" : Blt_Itoa(Blt_TreeNodeId(parent));
	    Tcl_DStringAppendElement(&dString, string);
	    Tcl_DStringAppendElement(&dString, "-at");
	    Tcl_DStringAppendElement(&dString, Blt_Itoa(position));
	    objPtr = Tcl_NewStringObj(Tcl_DStringValue(&dString), -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	} else {
	    objPtr = Tcl_NewIntObj(position);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    Tcl_DStringFree(&dString);
    Blt_Free(nodeArr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * PreviousOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
PreviousOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreePrevNode(Blt_TreeRootNode(cmdPtr->tree), node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*ARGSUSED*/
static int
PrevSiblingOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int inode;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    inode = -1;
    node = Blt_TreePrevSibling(node);
    if (node != NULL) {
	inode = Blt_TreeNodeId(node);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

static int
ReadEntry(
    Tcl_Interp *interp,
    Tcl_Channel channel,
    int *argcPtr,
    char ***argvPtr)
{
    Tcl_DString dString;
    int result, cmpl = 1;
    char *entry;

    if (*argvPtr != NULL) {
	Blt_Free(*argvPtr);
	*argvPtr = NULL;
    }
    Tcl_DStringInit(&dString);
    entry = NULL;
    while (Tcl_Gets(channel, &dString) > 0) {
	nLines++;
	Tcl_DStringAppend(&dString, "\n", 1);
	entry = Tcl_DStringValue(&dString);
	cmpl = 0;
	if (Tcl_CommandComplete(entry)) {
	    result = Tcl_SplitList(interp, entry, argcPtr, argvPtr);
	    Tcl_DStringFree(&dString);
	    return result;
	}
    }
    Tcl_DStringFree(&dString);
    if (entry == NULL) {
	*argcPtr = 0;		/* EOF */
	return TCL_OK;
    }
    Tcl_AppendResult(interp, "error reading file: ",
        (cmpl ? Tcl_PosixError(interp) : "missing brace"), (char *)NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * RestoreOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
RestoreOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode root;
    int nElem;
    char **elemArr, *entry, *eol, *next, saved;
    int result, isfile = 0, op, dd;
    Tcl_Channel channel = NULL;
    RestoreData data;

    if (GetNode(cmdPtr, objv[2], &root) != TCL_OK) {
	return TCL_ERROR;
    }
    memset((char *)&data, 0, sizeof(data));

    if (objc < 4) {
        Tcl_AppendResult(interp, "too few args", 0);
        return TCL_ERROR;
    }
    /* Process switches  */
    if (Blt_ProcessObjSwitches(interp, restoreSwitches, objc - 3, objv + 3, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	result = TCL_ERROR;
	goto done;
    }
    dd = 0;
    if (data.file == NULL) dd++;
    if (data.data == NULL) dd++;
    if (data.chan == NULL) dd++;
    if (dd != 2) {
        Tcl_AppendResult(interp, "one of -file, -data, -channel is required", 0);
        return TCL_ERROR;
    }
    if (data.file != NULL) {
        if (Tcl_IsSafe(interp)) {
            Tcl_AppendResult(interp, "can use -file in safe interp", 0);
            return TCL_ERROR;
        }
        channel = Tcl_OpenFileChannel(interp, data.file, "r", 0644);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        isfile = 1;
    }
    if (data.chan != NULL) {
        op = 0;
        channel = Tcl_GetChannel(interp, data.chan, &op);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((op & TCL_READABLE) == 0) {
            Tcl_AppendResult(interp, "channel is not readable", 0);
            return TCL_ERROR;
        }
    }
    if (data.data != NULL) {
        if (!Tcl_CommandComplete(Tcl_GetString(data.data))) {
            Tcl_AppendResult(interp, "data is not complete (missing brace?)", 0);
            return TCL_ERROR;
        }
    }
    if (data.addTags != NULL) {
        if (Tcl_ListObjGetElements(interp, data.addTags, &data.tobjc, &data.tobjv) 
            != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (data.keys != NULL && Tcl_ListObjGetElements(interp, data.keys, &data.kobjc,
        &data.kobjv) != TCL_OK) {
            return TCL_ERROR;
    }
    if (data.notKeys != NULL && Tcl_ListObjGetElements(interp, data.notKeys,
        &data.nobjc, &data.nobjv) != TCL_OK) {
            return TCL_ERROR;
    }
    /*if (data.keys != NULL && data.notKeys != NULL) {
        Tcl_AppendResult(interp, "can not use both -keys and -notkeys", 0);
        return TCL_ERROR;
    }*/

    Blt_InitHashTable(&data.idTable, BLT_ONE_WORD_KEYS);
    data.root = root;
    elemArr = NULL;
    nLines = 0;
    result = TCL_OK;
    if (channel != NULL) {
        for (;;) {
            result = ReadEntry(interp, channel, &nElem, &elemArr);
            if ((result != TCL_OK) || (nElem == 0)) {
                break;
            }
            result = RestoreNode(cmdPtr, nElem, elemArr, &data);
            if (result != TCL_OK) {
                break;
            }
        }
    } else {
        entry = eol = Tcl_GetString(data.data);
        next = entry;
        while (*eol != '\0') {
            /* Find the next end of line. */
            for (eol = next; (*eol != '\n') && (*eol != '\0'); eol++) {
                /*empty*/
            }
            /* 
            * Since we don't own the string (the Tcl_Obj could be shared),
                * save the current end-of-line character (it's either a NUL
                * or NL) so we can NUL-terminate the line for the call to
                * Tcl_SplitList and repair it when we're done.
                */
                saved = *eol;
                *eol = '\0';
                next = eol + 1;
                nLines++;
                if (Tcl_CommandComplete(entry)) {
                char **elArr;
                int nEl;
	    
                if (Tcl_SplitList(interp, entry, &nEl, &elArr) != TCL_OK) {
                    *eol = saved;
                    return TCL_ERROR;
                }
                if (nEl > 0) {
                    result = RestoreNode(cmdPtr, nEl, elArr, &data);
                    Blt_Free(elArr);
                    if (result != TCL_OK) {
                        *eol = saved;
                        break;
                    }
                }
                entry = next;
            }
            *eol = saved;
        }
    }
    Blt_DeleteHashTable(&data.idTable);
done:
    if (elemArr != NULL) {
	Blt_Free(elemArr);
    }
    if (isfile) {
        Tcl_Close(interp, channel);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RootOp -- Get/set the root.
 *
 *---------------------------------------------------------------------- 
 */
static int
RootOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode root;

    if (objc == 3) {
	Blt_TreeNode node;

	if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_TreeChangeRoot(cmdPtr->tree, node);
    }
    root = Blt_TreeRootNode(cmdPtr->tree);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), Blt_TreeNodeId(root));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NodeidOp -- Get/set the next id to be allocated.
 *
 *---------------------------------------------------------------------- 
 */
static int
NodeSeqOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    int num;

    if (objc == 3) {

	if (Tcl_GetIntFromObj(interp, objv[2], &num) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (num <= 0) {
             Tcl_AppendResult(interp, "must be > 0", 0);
             return TCL_ERROR;
         }
         cmdPtr->tree->treeObject->nextInode = num;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), cmdPtr->tree->treeObject->nextInode);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IncriOp -- Increment, with initalization to 0 if not exists.
 *
 *---------------------------------------------------------------------- 
 */
static int
IncriOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    double dVal, dIncr = 1.0;
    int iVal, iIncr = 1, isInt = 0, count = 0, len;
    Tcl_Obj *objPtr, *valueObjPtr;
    TagSearch cursor = {0};
    
    string = Tcl_GetStringFromObj(objv[2],&len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        iIncr = 1; isInt = 0;
        count++;
        string = Tcl_GetString(objv[3]); 
        if (Blt_TreeGetValue(NULL, cmdPtr->tree, node, string,
            &valueObjPtr) != TCL_OK) {
                if (Blt_TreeSetValue(NULL, cmdPtr->tree, node, string,
                Tcl_NewIntObj(0)) != TCL_OK ||
                Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
                &valueObjPtr) != TCL_OK) {
                    goto error;
            }
        }
        if (Tcl_GetIntFromObj(NULL, valueObjPtr, &iVal) == TCL_OK &&
        (objc <= 4 || Tcl_GetIntFromObj(NULL, objv[4], &iIncr) == TCL_OK)) {
            isInt = 1;
        } else {
            if (objc > 4 && Tcl_GetDoubleFromObj(interp, objv[4], &dIncr) != TCL_OK) {
                goto error;
            }
            if (Tcl_GetDoubleFromObj(interp, valueObjPtr, &dVal) != TCL_OK) {
                goto error;
            }
        }
        if (isInt) {
            iVal += iIncr;
            objPtr = Tcl_NewIntObj(iVal);
        } else {
            dVal += dIncr;
            objPtr = Tcl_NewDoubleObj(dVal);
        }
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, objPtr) != TCL_OK) {
            goto error;
        }
    }
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
    
error:    
    DoneTaggedNodes(&cursor);
    return TCL_ERROR;;
}
/*
 *----------------------------------------------------------------------
 *
 * IncrOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
IncrOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    double dVal, dIncr = 1.0;
    int iVal, iIncr = 1, isInt = 0;
    Tcl_Obj *objPtr, *valueObjPtr;
	
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]); 
    if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
        &valueObjPtr) != TCL_OK) {
            return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(NULL, valueObjPtr, &iVal) == TCL_OK &&
         (objc <= 4 || Tcl_GetIntFromObj(NULL, objv[4], &iIncr) == TCL_OK)) {
        isInt = 1;
    } else {
        if (objc > 4 && Tcl_GetDoubleFromObj(interp, objv[4], &dIncr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDoubleFromObj(interp, valueObjPtr, &dVal) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (isInt) {
        iVal += iIncr;
        objPtr = Tcl_NewIntObj(iVal);
    } else {
        dVal += dIncr;
        objPtr = Tcl_NewDoubleObj(dVal);
    }
    if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, objPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * IsSetOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
IsSetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    Tcl_Obj *valuePtr;
	
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]); 
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj((Blt_TreeGetValue(NULL, cmdPtr->tree, node, string, &valuePtr) == TCL_OK)));
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * LappendOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
LappendOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    Tcl_Obj *valuePtr;
    int len = 0, alloc = 0;
	
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]); 
    if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
        &valuePtr) != TCL_OK) {
            return TCL_ERROR;
    }
    if (objc<=4) return TCL_OK;
    if (valuePtr != NULL && Tcl_ListObjLength(interp, valuePtr, &len) != TCL_OK) {
        return TCL_ERROR;
    }
    if (!(node->flags & TREE_TRACE_ACTIVE)) {
        cmdPtr->updTyp = 2;
        if (valuePtr == NULL) {
            cmdPtr->oldLen = 0;
        } else {
            cmdPtr->oldLen = len;
        }
    }
    if (Tcl_IsShared(valuePtr)) {
        alloc = 1;
        valuePtr = Tcl_DuplicateObj(valuePtr);
    }

    if (Tcl_ListObjReplace(interp, valuePtr, len, 0, objc-4, objv+4) != TCL_OK) {
        if (alloc) {
            Tcl_DecrRefCount(valuePtr);
        }
        return TCL_ERROR;
    }
    /*for (i=4 ; i<objc ; i++) {
        if (Tcl_ListObjAppendElement(interp, valuePtr, objv[i]) != TCL_OK) {
            if (alloc) {
                Tcl_DecrRefCount(valuePtr);
            }
            return TCL_ERROR;
        }
    }*/
    if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, valuePtr) != TCL_OK) {
        if (alloc) {
            Tcl_DecrRefCount(valuePtr);
        }
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, valuePtr);
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * LappendiOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
LappendiOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    Tcl_Obj *valuePtr;
    int len = 0, alloc = 0, count = 0;
    TagSearch cursor = {0};
    
    string = Tcl_GetStringFromObj(objv[2],&len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    
    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        count++;
	
        string = Tcl_GetString(objv[3]); 
        if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
            &valuePtr) != TCL_OK) {
                if (Blt_TreeSetValue(NULL, cmdPtr->tree, node, string,
                Tcl_NewListObj(0, NULL)) != TCL_OK ||
                Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
                &valuePtr) != TCL_OK) {
                    DoneTaggedNodes(&cursor);
                    return TCL_ERROR;
            }
        }
        if (objc<=4) {
            DoneTaggedNodes(&cursor);
            return TCL_OK;
        }
        if (valuePtr != NULL && Tcl_ListObjLength(interp, valuePtr, &len) != TCL_OK) {
            DoneTaggedNodes(&cursor);
            return TCL_ERROR;
        }
        if (!(node->flags & TREE_TRACE_ACTIVE)) {
            cmdPtr->updTyp = 2;
            if (valuePtr == NULL) {
                cmdPtr->oldLen = 0;
            } else {
                cmdPtr->oldLen = len;
            }
        }
        if (Tcl_IsShared(valuePtr)) {
            alloc = 1;
            valuePtr = Tcl_DuplicateObj(valuePtr);
        }

        if (Tcl_ListObjReplace(interp, valuePtr, len, 0, objc-4, objv+4) != TCL_OK) {
            if (alloc) {
                Tcl_DecrRefCount(valuePtr);
            }
            DoneTaggedNodes(&cursor);
            return TCL_ERROR;
        }
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, valuePtr) != TCL_OK) {
            if (alloc) {
                Tcl_DecrRefCount(valuePtr);
            }
            DoneTaggedNodes(&cursor);
            return TCL_ERROR;
        }
    }
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * AppendOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
AppendOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    Tcl_Obj *valuePtr;
    int i, alloc = 0;
	
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]); 
    if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
        &valuePtr) != TCL_OK) {
            return TCL_ERROR;
    }
    if (objc<=4) return TCL_OK;
    if (!(node->flags & TREE_TRACE_ACTIVE)) {
        cmdPtr->updTyp = 1;
        if (valuePtr == NULL) {
            cmdPtr->oldLen = 0;
        } else {
            Tcl_GetStringFromObj(valuePtr, &cmdPtr->oldLen);
        }
    }
    if (Tcl_IsShared(valuePtr)) {
        alloc = 1;
        valuePtr = Tcl_DuplicateObj(valuePtr);
    }

    for (i=4 ; i<objc ; i++) {
        Tcl_AppendObjToObj(valuePtr, objv[i]);
    }
    if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, valuePtr) != TCL_OK) {
        if (alloc) {
            Tcl_DecrRefCount(valuePtr);
        }
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, valuePtr);
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * AppendiOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
AppendiOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string;
    Tcl_Obj *valuePtr;
    int i, alloc = 0, count = 0, len;
    TagSearch cursor = {0};
	
    string = Tcl_GetStringFromObj(objv[2],&len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    
    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL;
        node = NextTaggedNode(node, &cursor)) {
        count++;
        string = Tcl_GetString(objv[3]); 
        if (Blt_TreeGetValue(NULL, cmdPtr->tree, node, string,
            &valuePtr) != TCL_OK) {
                if (Blt_TreeSetValue(NULL, cmdPtr->tree, node, string,
                Tcl_NewStringObj("", -1)) != TCL_OK ||
                Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
                &valuePtr) != TCL_OK) {
                    DoneTaggedNodes(&cursor);
                    return TCL_ERROR;
            }
        }
        if (objc<=4) {
            DoneTaggedNodes(&cursor);
            return TCL_OK;
        }
        if (!(node->flags & TREE_TRACE_ACTIVE)) {
            cmdPtr->updTyp = 1;
            if (valuePtr == NULL) {
                cmdPtr->oldLen = 0;
            } else {
                Tcl_GetStringFromObj(valuePtr, &cmdPtr->oldLen);
            }
        }
        if (Tcl_IsShared(valuePtr)) {
            alloc = 1;
            valuePtr = Tcl_DuplicateObj(valuePtr);
        }

        for (i=4 ; i<objc ; i++) {
            Tcl_AppendObjToObj(valuePtr, objv[i]);
        }
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, valuePtr) != TCL_OK) {
            if (alloc) {
                Tcl_DecrRefCount(valuePtr);
            }
            DoneTaggedNodes(&cursor);
            return TCL_ERROR;
        }
    }
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * UpdateOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
updateOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv,
    int cmpstr)
{
    Blt_TreeNode node;
    char *string;
    Tcl_Obj *valuePtr;
    int i, result = TCL_OK;
    Tcl_DString dStr;
	
    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc%2) == 0) {
        Tcl_AppendResult(interp, "odd number of key/value pairs", 0);
        return TCL_ERROR;
    }
    if (objc<=3) { return TCL_OK; }
    if (!(node->flags & TREE_TRACE_ACTIVE)) {
        cmdPtr->updTyp = 0;
    }
    Tcl_DStringInit(&dStr);
    for (i=3; i<objc; i+=2) {
        string = Tcl_GetString(objv[i]); 
        if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string,
            &valuePtr) != TCL_OK) {
            result = TCL_ERROR;
            Tcl_DStringAppend(&dStr, Tcl_GetStringResult(interp), -1);
            Tcl_ResetResult(interp);
            continue;
        }
        if (cmpstr && valuePtr != NULL && strcmp(Tcl_GetString(objv[i+1]), Tcl_GetString(valuePtr)) == 0) continue;
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, objv[i+1]) != TCL_OK) {
            Tcl_DStringAppend(&dStr, Tcl_GetStringResult(interp), -1);
            Tcl_DStringAppend(&dStr, " ", -1);
            Tcl_ResetResult(interp);
            result = TCL_ERROR;
        }
    }
    if (result != TCL_OK) {
        Tcl_DStringResult(interp, &dStr);
    }
    return result;
}

static int
UpdateOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    return updateOp(cmdPtr, interp, objc, objv, 0);
}

static int
UpdatesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    return updateOp(cmdPtr, interp, objc, objv, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * SetOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
SetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    /*char *string;*/
    int count = 0, len;
    TagSearch cursor = {0};
	
    /*string =*/ Tcl_GetStringFromObj(objv[2],&len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    
    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        if (!(node->flags & TREE_TRACE_ACTIVE)) {
            cmdPtr->updTyp = 0;
        }
        count++;
        if (SetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
            DoneTaggedNodes(&cursor);
            return TCL_ERROR;
        }
    }
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IsModifiedOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
IsModifiedOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int ismod;
    Blt_TreeObject treeObjPtr = cmdPtr->tree->treeObject;
    TagSearch cursor = {0};
	
    if (objc == 2) {
        ismod = ((treeObjPtr->flags&TREE_UNMODIFIED) == 0);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(ismod));
        return TCL_OK;
    }
    
    if (objc == 3) {
        if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
            return TCL_ERROR;
        }
        ismod = ((node->flags&TREE_NODE_UNMODIFIED) == 0);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(ismod));
        return TCL_OK;
    }
    
    if (Tcl_GetBooleanFromObj(interp, objv[3], &ismod) != TCL_OK) {
        return TCL_ERROR;
    }

    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        if (ismod) {
            node->flags &= ~TREE_NODE_UNMODIFIED;
        } else {
            node->flags |= TREE_NODE_UNMODIFIED;
        }
    }
    if (!strcmp("all", Tcl_GetString(objv[2]))) {
        if (ismod) {
            treeObjPtr->flags &= ~TREE_UNMODIFIED;
        } else {
            treeObjPtr->flags |= TREE_UNMODIFIED;
        }
    }
    DoneTaggedNodes(&cursor);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SumOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
SumOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    char *string, *runKey = NULL;
    double start = 0, cur = 0, mindif = 1e-13;
    int istart = 0, icur = 0, len;
    int i, n, useint = 0, force = 0;
    Tcl_Obj *valuePtr, *valPtr;
    TagSearch cursor = {0};

    i = 2;
    while (i<objc) {
        string = Tcl_GetString(objv[i]);
        if (strcmp(string, "-runtotal") == 0) {
            if ((i+1)>=objc) {
                Tcl_AppendResult(interp, "missing value", (char *)NULL);
                return TCL_ERROR;
            }
            runKey = Tcl_GetString(objv[i+1]);
            i += 2;
        } else if (strcmp(string, "-int") == 0) {
            useint = 1;
            i += 1;
        } else if (strcmp(string, "-force") == 0) {
            force = 1;
            i += 1;
        } else if (strcmp(string, "-start") == 0) {
            if ((i+1)>=objc) {
                Tcl_AppendResult(interp, "missing value", (char *)NULL);
                return TCL_ERROR;
            }
            if (useint) {
                if (Tcl_GetIntFromObj(interp, objv[i+1], &istart) != TCL_OK) {
                    return TCL_ERROR;
                }
            } else {
                if (Tcl_GetDoubleFromObj(interp, objv[i+1], &start) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
            i += 2;
        } else if (strcmp(string, "-diff") == 0) {
            if ((i+1)>=objc) {
                Tcl_AppendResult(interp, "missing value", (char *)NULL);
                return TCL_ERROR;
            }
            if (Tcl_GetDoubleFromObj(interp, objv[i+1], &mindif) != TCL_OK) {
                return TCL_ERROR;
            }
            i += 2;
        } else if (string[0] == '-') {
            Tcl_AppendResult(interp, "option not one of: -runtotal -start -int", (char *)NULL);
            return TCL_ERROR;
        } else {
            break;
        }
    }
    if (useint) {
        if (start != 0 && istart == 0) {
            istart = (int)start;
        }
    }
    if ((i+2) != objc) {
        Tcl_AppendResult(interp, "usage: ?options? nodelst key", (char *)NULL);
        return TCL_ERROR;
    }

    string = Tcl_GetStringFromObj(objv[i+1], &len);
    if (len == 0) goto done;
    
    if (FindTaggedNodes(interp, cmdPtr, objv[i], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        
        for (n = i+1; n < objc; n++) {
            string = Tcl_GetString(objv[n]);
            if (Blt_TreeGetValue(NULL, cmdPtr->tree, node, string, &valuePtr) 
                != TCL_OK) {
                continue;
            }
            if (useint) {
                if (Tcl_GetIntFromObj(NULL, valuePtr, &icur) != TCL_OK) {
                    continue;
                }
                istart += icur;
            } else {
                if (Tcl_GetDoubleFromObj(NULL, valuePtr, &cur) != TCL_OK) {
                    continue;
                }
                start += cur;
            }
            if (runKey != NULL) {
                if (force == 0 && Blt_TreeGetValue(NULL, cmdPtr->tree, node,
                    runKey, &valPtr) == TCL_OK) {
                    if (useint) {
                        if (Tcl_GetIntFromObj(NULL, valPtr, &icur) == TCL_OK &&
                            istart == icur) {
                            continue;
                        }
                    } else {
                        if (Tcl_GetDoubleFromObj(NULL, valPtr, &cur) == TCL_OK &&
                            fabs(start - cur)<mindif) {
                            continue;
                        }
                    }
                }
                Blt_TreeSetValue(NULL, cmdPtr->tree, node, runKey,
                    (useint ? Tcl_NewIntObj(istart) : Tcl_NewDoubleObj(start)));
            }
        }
    }
done:
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, (useint ? Tcl_NewIntObj(istart) : Tcl_NewDoubleObj(start)));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ModifyOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
ModifyOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    /*char *string;*/
    int count = 0, result = TCL_OK, len;
    Tcl_DString dStr;
    TagSearch cursor = {0};

    if ((objc % 2) == 0) {
        Tcl_AppendResult(interp, "odd # values", (char *)NULL);
        return TCL_ERROR;
    }
    if (objc<=3) return TCL_OK;
    /*string =*/ Tcl_GetStringFromObj(objv[2], &len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    node = FirstTaggedNode(&cursor);
    if (node != NULL && !(node->flags & TREE_TRACE_ACTIVE)) {
        cmdPtr->updTyp = 0;
    }
    Tcl_DStringInit(&dStr);
    for (/* empty */; node != NULL; node = NextTaggedNode(node, &cursor)) {
        count++;
        if (UpdateValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
            Tcl_DStringAppend(&dStr, Tcl_GetStringResult(interp), -1);
            Tcl_DStringAppend(&dStr, " ", -1);
            Tcl_ResetResult(interp);
            result = TCL_ERROR;
        }
    }
    if (result != TCL_OK) {
        Tcl_DStringResult(interp, &dStr);
    } else {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    }
    DoneTaggedNodes(&cursor);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SizeOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
SizeOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), Blt_TreeSize(node));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TagForgetOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TagForgetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    register int i;

    for (i = 3; i < objc; i++) {
	if (Blt_TreeForgetTag(cmdPtr->tree, Tcl_GetString(objv[i])) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagNamesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_HashSearch cursor;
    Blt_TreeTagEntry *tPtr;
    Tcl_Obj *listObjPtr, *objPtr, *patPtr = NULL;
    char *string, *pattern = NULL, ptype = 0;
    int nocase = 0, result;

    while (objc>3) {
        string = Tcl_GetString(objv[3]);
        if (strcmp(string, "-glob") == 0 || strcmp(string, "-regexp") == 0) {
            if (objc == 4) {
                Tcl_AppendResult(interp, "missing pattern", 0);
                return TCL_ERROR;
            }
            ptype = string[1];
            patPtr = objv[4];
            pattern = Tcl_GetString(objv[4]);
            if (ptype == 'r') {
                result = Tcl_RegExpMatch(interp, "", pattern); 
                if (result == -1) {
                    return TCL_ERROR;
                }
            }
            objc -= 2;
            objv += 2;
        } else if (strcmp(string, "-nocase") == 0) {
            objc -= 1;
            objv += 1;
            nocase = 1;
        } else {
            break;
        }
    }
    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (0 && patPtr == NULL) {
        objPtr = Tcl_NewStringObj("all", -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("nonroot", -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("rootchildren", -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (objc == 3) {
	Blt_HashEntry *hPtr;

        if (0 && patPtr == NULL) {
            objPtr = Tcl_NewStringObj("root", -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
	for (hPtr = Blt_TreeFirstTag(cmdPtr->tree, &cursor); hPtr != NULL; 
	     hPtr = Blt_NextHashEntry(&cursor)) {
	    tPtr = Blt_GetHashValue(hPtr);
            if (pattern != NULL) {
                if (ptype == 'g') {
                    if (!Tcl_StringCaseMatch(tPtr->tagName, pattern, nocase)) {
                        continue;
                    }
                } else {
                    string = tPtr->tagName;
                    if (nocase) {
                        string = Blt_Strdup(string);
                        strtolower(string);
                    }
                    result = (Tcl_RegExpMatch(interp, string, pattern) == 1); 
                    if (nocase) {
                        Blt_Free(string);
                    }
                    if (result == 0) continue;
                }
            }
	    objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	register int i;
	Blt_TreeNode node;
	Blt_HashEntry *hPtr;
	Blt_HashTable uniqTable;
	int isNew;

	Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
	for (i = 3; i < objc; i++) {
	    if (GetNode(cmdPtr, objv[i], &node) != TCL_OK) {
		goto error;
	    }
	    if (node == Blt_TreeRootNode(cmdPtr->tree)) {
		Blt_CreateHashEntry(&uniqTable, "root", &isNew);
	    }
	    for (hPtr = Blt_TreeFirstTag(cmdPtr->tree, &cursor); hPtr != NULL;
		 hPtr = Blt_NextHashEntry(&cursor)) {
		tPtr = Blt_GetHashValue(hPtr);
	        if (Blt_TreeHasTag(cmdPtr->tree, node, tPtr->tagName)) {
		    Blt_CreateHashEntry(&uniqTable, tPtr->tagName, &isNew);
		}
	    }
	}
	for (hPtr = Blt_FirstHashEntry(&uniqTable, &cursor); hPtr != NULL;
	    hPtr = Blt_NextHashEntry(&cursor)) {
            string = (char*)Blt_GetHashKey(&uniqTable, hPtr);
            if (pattern != NULL) {
                if (ptype == 'g') {
                    if (!Tcl_StringCaseMatch(string, pattern, nocase)) {
                        continue;
                    }
                } else {
                    if (nocase) {
                        string = Blt_Strdup(string);
                        strtolower(string);
                    }
                    result = (Tcl_RegExpMatch(interp, string, pattern) == 1); 
                    if (nocase) {
                        Blt_Free(string);
                    }
                    if (result == 0) continue;
                }
            }
	    objPtr = Tcl_NewStringObj(string, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Blt_DeleteHashTable(&uniqTable);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TagLookupsOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagLookupsOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_HashEntry *hPtr, *h2Ptr;
    Blt_HashSearch cursor, tcursor;
    Blt_TreeNode node;
    Blt_TreeTagEntry *tPtr;
    Tcl_Obj *listObjPtr, *objPtr;
    Tcl_DString dStr;
    int result = TCL_OK, cnt;
    Blt_HashTable tagTable;
    Tcl_DString *eStr;
    char *string;
    
    if (objc == 4) {
        string = Tcl_GetString(objv[3]);
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        Tcl_DStringInit(&dStr);

        for (hPtr = Blt_TreeFirstTag(cmdPtr->tree, &cursor); hPtr != NULL;         
            hPtr = Blt_NextHashEntry(&cursor)) {
        
            tPtr = Blt_GetHashValue(hPtr);
            if (Tcl_StringMatch(tPtr->tagName, string)!=1) {
                continue;
            }

            objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            Tcl_DStringSetLength(&dStr, 0);
        
            cnt = 0;
            for (h2Ptr = Blt_FirstHashEntry(&tPtr->nodeTable, &tcursor); h2Ptr != NULL;
            h2Ptr = Blt_NextHashEntry(&tcursor)) {
                node = Blt_GetHashValue(h2Ptr);
                cnt++;
                if (cnt>1) {
                    Tcl_DStringAppend(&dStr, " ", -1);
                }
                Tcl_DStringAppend(&dStr, Blt_Itoa(node->inode), -1);
            }
        
            objPtr = Tcl_NewStringObj(Tcl_DStringValue(&dStr), -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_DStringFree(&dStr);
        Tcl_SetObjResult(interp, listObjPtr);
        return result;
    }
    
    MakeTagTable(cmdPtr->tree, &tagTable, 0, 0);

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    
    for (h2Ptr = Blt_FirstHashEntry(&tagTable, &tcursor);
        h2Ptr != NULL; h2Ptr = Blt_NextHashEntry(&tcursor)) {
        node = Blt_GetHashKey(&tagTable, h2Ptr);
        eStr = (Tcl_DString*)Blt_GetHashValue(h2Ptr);
        
        objPtr = Tcl_NewStringObj(Blt_Itoa(node->inode), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

        objPtr = Tcl_NewStringObj(Tcl_DStringValue(eStr), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        
        Tcl_DStringFree(eStr);
        Blt_Free(eStr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TagNodesOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagNodesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor = {0};
    Blt_HashTable nodeTable;
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr;
    Tcl_Obj *objPtr;
    int isNew;
    register int i;
	
    Blt_InitHashTable(&nodeTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        TagSearch tcursor = {0};
        
        if (FindTaggedNodes(interp, cmdPtr, objv[i], &tcursor) != TCL_OK) {
            Tcl_ResetResult(interp);
            DoneTaggedNodes(&tcursor);
            continue;
        }
        for (node = FirstTaggedNode(&tcursor);
            node != NULL; node = NextTaggedNode(node, &tcursor)) {
            Blt_CreateHashEntry(&nodeTable, (char *)node, &isNew);
	}
        DoneTaggedNodes(&tcursor);
     }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&nodeTable, &cursor); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&cursor)) {
	node = (Blt_TreeNode)Blt_GetHashKey(&nodeTable, hPtr);
	objPtr = Tcl_NewIntObj(Blt_TreeNodeId(node));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Blt_DeleteHashTable(&nodeTable);
    return TCL_OK;

/* error:
    Blt_DeleteHashTable(&nodeTable);
    return TCL_ERROR;*/
}

/*
 *----------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagExistsOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    char *string;
    int exists = 0;
    Blt_TreeNode node;
	
    string = Tcl_GetString(objv[3]);
    if (objc == 4) {
        if (strcmp(string, "all") == 0 || (strcmp(string, "root") == 0)
            || (strcmp(string, "nonroot") == 0)
            || (strcmp(string, "rootchildren") == 0)) {
            exists = 1;
        } else {
            TagSearch tcursor = {0};
            if (FindTaggedNodes(interp, cmdPtr, objv[3], &tcursor) == TCL_OK) {
                exists = 1;
            }
            DoneTaggedNodes(&tcursor);
        }
    } else {
        if (GetNode(cmdPtr, objv[4], &node) != TCL_OK) {
            return TCL_ERROR;
        }
        exists = Blt_TreeHasTag(cmdPtr->tree, node, string);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(exists));
    return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagAddOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    register int i;
    char *string;
    int count = 0;
    TagSearch cursor = {0};

    string = Tcl_GetString(objv[3]);
    if (isdigit(UCHAR(string[0]))) {
	Tcl_AppendResult(interp, "bad tag \"", string, 
		 "\": can't start with a digit", (char *)NULL);
	return TCL_ERROR;
    }
    if (strstr(string,"->") != NULL) {
        Tcl_AppendResult(cmdPtr->interp, "invalid tag \"", string, 
            "\": can't contain \"->\"", (char *)NULL);
            return TCL_ERROR;
    }
    if (string[0] == '@') {
        Tcl_AppendResult(cmdPtr->interp, "invalid tag \"", string, 
            "\": can't start with \"@\"", (char *)NULL);
            return TCL_ERROR;
    } 
    if ((strcmp(string, "all") == 0) || (strcmp(string, "root") == 0)
        || (strcmp(string, "nonroot") == 0)
        || (strcmp(string, "rootchildren") == 0)) {
	Tcl_AppendResult(cmdPtr->interp, "can't add reserved tag \"",
			 string, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 4) {
        return Blt_TreeAddTag(cmdPtr->tree, NULL, string);
    }
    for (i = 4; i < objc; i++) {
        string = Tcl_GetString(objv[3]);
        if (isdigit(UCHAR(string[0])) && strchr(string,' ')) {
        }
        if (FindTaggedNodes(interp, cmdPtr, objv[i], &cursor) != TCL_OK) {
            return TCL_ERROR;
        }
        for (node = FirstTaggedNode(&cursor);
            node != NULL;
            node = NextTaggedNode(node, &cursor)) {
	    count++;
	    if (AddTag(cmdPtr, node, string) != TCL_OK) {
                 DoneTaggedNodes(&cursor);
                 return TCL_ERROR;
	    }
	}
        DoneTaggedNodes(&cursor);
     }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagDeleteOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    char *string;
    Blt_HashTable *tablePtr;
    int count = 0;

    string = Tcl_GetString(objv[3]);
    if ((strcmp(string, "all") == 0) || (strcmp(string, "root") == 0)
        || (strcmp(string, "nonroot") == 0)
        || (strcmp(string, "childrenroot") == 0)) {
	/* Tcl_AppendResult(interp, "can't delete reserved tag \"", string, "\"", 
			 (char *)NULL);
        return TCL_ERROR; */
        Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
        return TCL_OK;
    }
    tablePtr = Blt_TreeTagHashTable(cmdPtr->tree, string);
    if (tablePtr != NULL) {
        register int i;
        Blt_TreeNode node;
        Blt_HashEntry *hPtr;
        TagSearch cursor = {0};
      
        for (i = 4; i < objc; i++) {
            if (FindTaggedNodes(interp, cmdPtr, objv[i], &cursor) != TCL_OK) {
                return TCL_ERROR;
            }
            for (node = FirstTaggedNode(&cursor);
                node != NULL; 	
		node = NextTaggedNode(node, &cursor)) {
	        hPtr = Blt_FindHashEntry(tablePtr, (char *)node);
	        if (hPtr != NULL) {
	            int result;
                    result = Blt_TreeTagDelTrace(cmdPtr->tree, node, string);
                    if (result != TCL_OK) {
                        if (result != TCL_BREAK) {
                            DoneTaggedNodes(&cursor);
                            return TCL_ERROR;
                        }
                        continue;
                    }
                    Blt_DeleteHashEntry(tablePtr, hPtr);
                    count++;
	        }
	   }
           DoneTaggedNodes(&cursor);
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TagDumpOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagDumpOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    register Blt_TreeNode root, node;
    Tcl_DString dString;
    int tags = 1, result = TCL_OK;
    TagSearch cursor = {0};
    RestoreData data;
    
    memset((char *)&data, 0, sizeof(data));

    Tcl_DStringInit(&dString);
    root = Blt_TreeRootNode(cmdPtr->tree);
    if (objc > 4) {
        /* Process switches  */
        if (Blt_ProcessObjSwitches(interp, dumptagSwitches, objc - 4, objv + 4, 
            (char *)&data, BLT_SWITCH_EXACT) < 0) {
                return TCL_ERROR;
        }
    }
    if (data.keys != NULL && Tcl_ListObjGetElements(interp, data.keys, &data.kobjc,
        &data.kobjv) != TCL_OK) {
            return TCL_ERROR;
    }
    if (data.notKeys != NULL && Tcl_ListObjGetElements(interp, data.notKeys,
        &data.nobjc, &data.nobjv) != TCL_OK) {
            return TCL_ERROR;
    }
    /*if (data.keys != NULL && data.notKeys != NULL) {
        Tcl_AppendResult(interp, "can not use both -keys and -notkeys", 0);
        return TCL_ERROR;
    }*/

    tags = ((data.flags&RESTORE_NO_TAGS) == 0);
    if (FindTaggedNodes(interp, cmdPtr, objv[3], &cursor) != TCL_OK) {
        result = TCL_ERROR;
        goto done;
    }
    if (tags) {
        MakeTagTable(cmdPtr->tree, &data.tagTable, data.tags, data.notTags);
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        PrintNode(cmdPtr, root, node, &dString, tags, &data);
    }
    DoneTaggedNodes(&cursor);
    if (tags) {
         FreeTagTable(&data.tagTable);
     }
done:
     Tcl_DStringResult(interp, &dString);
     Tcl_DStringFree(&dString);
     return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TagOp --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec tagOps[] = {
    {"add", 1, (Blt_Op)TagAddOp, 4, 0, "tag ?node?...",},
    {"delete", 2, (Blt_Op)TagDeleteOp, 5, 0, "tag node...",},
    {"dump", 2, (Blt_Op)TagDumpOp, 4, 0, "tag ?switches?",},
    {"exists", 2, (Blt_Op)TagExistsOp, 4, 5, "tag ?node?",},
    {"forget", 1, (Blt_Op)TagForgetOp, 4, 0, "tag...",},
    {"lookups", 2, (Blt_Op)TagLookupsOp, 3, 4, "?pattern?",},
    {"names", 2, (Blt_Op)TagNamesOp, 3, 0, "?-glob pat? ?-regexp pat? ?node...?",},
    {"nodes", 2, (Blt_Op)TagNodesOp, 4, 0, "tag ?tag...?",},
};

static int nTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nTagOps, tagOps, BLT_OP_ARG2, objc, objv, 
	0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceCreateOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TraceCreateOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_HashEntry *hPtr;
    Blt_TreeNode node;
    TraceInfo *tracePtr;
    char *string, *key, *command;
    char *tagName;
    char idString[200];
    int flags, isNew;
    int length, idle;

    idle = 0;
    if (objc > 7) {
        if (!strcmp("-bgerror", Tcl_GetString(objv[7]))) {
            idle = 1;
        } else {
            Tcl_AppendResult(interp, "expected \"-bgerror\": " , Tcl_GetString(objv[7]), 0);
            return TCL_ERROR;
        }
    }
    string = Tcl_GetString(objv[3]);
    if (isdigit(UCHAR(*string))) {
	if (GetNode(cmdPtr, objv[3], &node) != TCL_OK) {
	    return TCL_ERROR;
	}
	tagName = NULL;
    } else {
	tagName = Blt_Strdup(string);
	node = NULL;
    }
    key = Tcl_GetString(objv[4]);
    string = Tcl_GetString(objv[5]);
    flags = GetTraceFlags(string);
    if (flags < 0) {
	Tcl_AppendResult(interp, "unknown flag in \"", string, "\"", 
		     (char *)NULL);
	return TCL_ERROR;
    }
    command = Tcl_GetStringFromObj(objv[6], &length);
    /* Stash away the command in structure and pass that to the trace. */
    tracePtr = Blt_Calloc(1, length + sizeof(TraceInfo));
    strcpy(tracePtr->command, command);
    tracePtr->cmdPtr = cmdPtr;
    tracePtr->withTag = tagName;
    tracePtr->node = node;
    if (idle) {
        flags |= TREE_TRACE_BGERROR;
    }
    tracePtr->traceToken = Blt_TreeCreateTrace(cmdPtr->tree, node, key, tagName,
	flags, TreeTraceProc, tracePtr);

    sprintf(idString, "trace%d", cmdPtr->traceCounter++);
    hPtr = Blt_CreateHashEntry(&(cmdPtr->traceTable), idString, &isNew);
    Blt_SetHashValue(hPtr, tracePtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), idString, -1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceDeleteOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TraceDeleteOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    TraceInfo *tracePtr;
    Blt_HashEntry *hPtr;
    register int i;
    char *key;

    for (i = 3; i < objc; i++) {
	key = Tcl_GetString(objv[i]);
	hPtr = Blt_FindHashEntry(&(cmdPtr->traceTable), key);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "unknown trace \"", key, "\"", 
			     (char *)NULL);
	    return TCL_ERROR;
	}
	tracePtr = Blt_GetHashValue(hPtr);
	Blt_DeleteHashEntry(&(cmdPtr->traceTable), hPtr); 
	Blt_TreeDeleteTrace(tracePtr->traceToken);
	if (tracePtr->withTag != NULL) {
	    Blt_Free(tracePtr->withTag);
	}
	Blt_Free(tracePtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceNamesOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TraceNamesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(&(cmdPtr->traceTable), &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Tcl_AppendElement(interp, Blt_GetHashKey(&(cmdPtr->traceTable), hPtr));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceInfoOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TraceInfoOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    TraceInfo *tracePtr;
    struct Blt_TreeTraceStruct *tokenPtr;
    Blt_HashEntry *hPtr;
    Tcl_DString dString;
    char string[5];
    char *key;

    key = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&(cmdPtr->traceTable), key);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "unknown trace \"", key, "\"", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_DStringInit(&dString);
    tracePtr = Blt_GetHashValue(hPtr);
    if (tracePtr->withTag != NULL) {
	Tcl_DStringAppendElement(&dString, tracePtr->withTag);
    } else {
	int inode;

	inode = Blt_TreeNodeId(tracePtr->node);
	Tcl_DStringAppendElement(&dString, Blt_Itoa(inode));
    }
    tokenPtr = (struct Blt_TreeTraceStruct *)tracePtr->traceToken;
    Tcl_DStringAppendElement(&dString, tokenPtr->key);
    PrintTraceFlags(tokenPtr->mask, string);
    Tcl_DStringAppendElement(&dString, string);
    Tcl_DStringAppendElement(&dString, tracePtr->command);
    Tcl_DStringResult(interp, &dString);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceOp --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec traceOps[] =
{
    {"create", 1, (Blt_Op)TraceCreateOp, 7, 8, "node key how command ?-whenidle?",},
    {"delete", 1, (Blt_Op)TraceDeleteOp, 3, 0, "id...",},
    {"info", 1, (Blt_Op)TraceInfoOp, 4, 4, "id",},
    {"names", 1, (Blt_Op)TraceNamesOp, 3, 3, "",},
};

static int nTraceOps = sizeof(traceOps) / sizeof(Blt_OpSpec);

static int
TraceOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nTraceOps, traceOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TypeOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TypeOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *valueObjPtr;
    char *string;

    if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }

    string = Tcl_GetString(objv[3]);
    if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string, &valueObjPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (valueObjPtr->typePtr != NULL) {
	Tcl_SetResult(interp, (char *)(valueObjPtr->typePtr->name), TCL_VOLATILE);
    } else {
	Tcl_SetResult(interp, "string", TCL_STATIC);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * UnsetOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
UnsetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int count = 0;
    TagSearch cursor = {0};
	
    if (FindTaggedNodes(interp, cmdPtr, objv[2], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    node = FirstTaggedNode(&cursor);
    if (node && !(node->flags & TREE_TRACE_ACTIVE)) {
        cmdPtr->updTyp = 0;
    }
    for (/* empty */; node != NULL; node = NextTaggedNode(node, &cursor)) {
        if (UnsetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
            DoneTaggedNodes(&cursor);
            return TCL_ERROR;
        }
        count++;
    }
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}


typedef struct {
    TreeCmd *cmdPtr;
    unsigned int flags;
    int type;
    int mode;
    char *key;
    char *command;
} SortData;

#define SORT_RECURSE		(1<<2)
#define SORT_DECREASING		(1<<3)
#define SORT_PATHNAME		(1<<4)

enum SortTypes { SORT_DICTIONARY, SORT_REAL, SORT_INTEGER, SORT_ASCII, 
	SORT_COMMAND };

enum SortModes { SORT_FLAT, SORT_REORDER };

static Blt_SwitchSpec sortSwitches[] = 
{
    {BLT_SWITCH_FLAG, "-ascii", Blt_Offset(SortData, type), 0, 0, 
	SORT_ASCII},
    {BLT_SWITCH_STRING, "-command", Blt_Offset(SortData, command), 0},
    {BLT_SWITCH_FLAG, "-decreasing", Blt_Offset(SortData, flags), 0, 0, 
	SORT_DECREASING},
    {BLT_SWITCH_FLAG, "-dictionary", Blt_Offset(SortData, type), 0, 0, 
	SORT_DICTIONARY},
    {BLT_SWITCH_FLAG, "-integer", Blt_Offset(SortData, type), 0, 0, 
	SORT_INTEGER},
    {BLT_SWITCH_STRING, "-key", Blt_Offset(SortData, key), 0},
    {BLT_SWITCH_FLAG, "-real", Blt_Offset(SortData, type), 0, 0, 
	SORT_REAL},
    {BLT_SWITCH_FLAG, "-recurse", Blt_Offset(SortData, flags), 0, 0, 
	SORT_RECURSE},
    {BLT_SWITCH_VALUE, "-reorder", Blt_Offset(SortData, mode), 0, 0, 
	SORT_REORDER},
    {BLT_SWITCH_FLAG, "-usepath", Blt_Offset(SortData, flags), 0, 0, 
	SORT_PATHNAME},
    {BLT_SWITCH_END, NULL, 0, 0}
};

static SortData sortData;

static int
CompareNodes(Blt_TreeNode *n1Ptr, Blt_TreeNode *n2Ptr)
{
    TreeCmd *cmdPtr = sortData.cmdPtr;
    char *s1, *s2;
    int result;
    Tcl_DString dString1, dString2;

    s1 = s2 = "";
    result = 0;

    if (sortData.flags & SORT_PATHNAME) {
	Tcl_DStringInit(&dString1);
	Tcl_DStringInit(&dString2);
    }
    if (sortData.key != NULL) {
	Tcl_Obj *valueObjPtr;

	if (Blt_TreeGetValue((Tcl_Interp *)NULL, cmdPtr->tree, *n1Ptr, 
	     sortData.key, &valueObjPtr) == TCL_OK) {
	    s1 = Tcl_GetString(valueObjPtr);
	}
	if (Blt_TreeGetValue((Tcl_Interp *)NULL, cmdPtr->tree, *n2Ptr, 
	     sortData.key, &valueObjPtr) == TCL_OK) {
	    s2 = Tcl_GetString(valueObjPtr);
	}
    } else if (sortData.flags & SORT_PATHNAME)  {
	Blt_TreeNode root;
	
	root = Blt_TreeRootNode(cmdPtr->tree);
	s1 = GetNodePath(cmdPtr, root, *n1Ptr, FALSE, &dString1);
	s2 = GetNodePath(cmdPtr, root, *n2Ptr, FALSE, &dString2);
    } else {
	s1 = Blt_TreeNodeLabel(*n1Ptr);
	s2 = Blt_TreeNodeLabel(*n2Ptr);
    }
    switch (sortData.type) {
    case SORT_ASCII:
	result = strcmp(s1, s2);
	break;

    case SORT_COMMAND:
	if (sortData.command == NULL) {
	    result = Blt_DictionaryCompare(s1, s2);
	} else {
	    Tcl_DString dsCmd, dsName;
	    char *qualName;

	    result = 0;	/* Hopefully this will be Ok even if the
			 * Tcl command fails to return the correct
			 * result. */
	    Tcl_DStringInit(&dsCmd);
	    Tcl_DStringAppend(&dsCmd, sortData.command, -1);
	    Tcl_DStringInit(&dsName);
	    qualName = Blt_GetQualifiedName(
		Blt_GetCommandNamespace(cmdPtr->interp, cmdPtr->cmdToken), 
		Tcl_GetCommandName(cmdPtr->interp, cmdPtr->cmdToken), &dsName);
	    Tcl_DStringAppendElement(&dsCmd, qualName);
	    Tcl_DStringFree(&dsName);
	    Tcl_DStringAppendElement(&dsCmd, Blt_Itoa(Blt_TreeNodeId(*n1Ptr)));
	    Tcl_DStringAppendElement(&dsCmd, Blt_Itoa(Blt_TreeNodeId(*n2Ptr)));
	    Tcl_DStringAppendElement(&dsCmd, s1);
	    Tcl_DStringAppendElement(&dsCmd, s2);
	    result = Tcl_GlobalEval(cmdPtr->interp, Tcl_DStringValue(&dsCmd));
	    Tcl_DStringFree(&dsCmd);
	    
            if (cmdPtr->delete) {
                return TCL_ERROR;
            }
            if ((result != TCL_OK) ||
		(Tcl_GetInt(cmdPtr->interp, 
		    Tcl_GetStringResult(cmdPtr->interp), &result) != TCL_OK)) {
		Tcl_BackgroundError(cmdPtr->interp);
	    }
	    Tcl_ResetResult(cmdPtr->interp);
	}
	break;

    case SORT_DICTIONARY:
	result = Blt_DictionaryCompare(s1, s2);
	break;

    case SORT_INTEGER:
	{
	    int i1, i2;

	    if (Tcl_GetInt(NULL, s1, &i1) == TCL_OK) {
		if (Tcl_GetInt(NULL, s2, &i2) == TCL_OK) {
		    result = i1 - i2;
		} else {
		    result = -1;
		} 
	    } else if (Tcl_GetInt(NULL, s2, &i2) == TCL_OK) {
		result = 1;
	    } else {
		result = Blt_DictionaryCompare(s1, s2);
	    }
	}
	break;

    case SORT_REAL:
	{
	    double r1, r2;

	    if (Tcl_GetDouble(NULL, s1, &r1) == TCL_OK) {
		if (Tcl_GetDouble(NULL, s2, &r2) == TCL_OK) {
		    result = (r1 < r2) ? -1 : (r1 > r2) ? 1 : 0;
		} else {
		    result = -1;
		} 
	    } else if (Tcl_GetDouble(NULL, s2, &r2) == TCL_OK) {
		result = 1;
	    } else {
		result = Blt_DictionaryCompare(s1, s2);
	    }
	}
	break;
    }
    if (result == 0) {
	result = Blt_TreeNodeId(*n1Ptr) - Blt_TreeNodeId(*n2Ptr);
    }
    if (sortData.flags & SORT_DECREASING) {
	result = -result;
    } 
    if (sortData.flags & SORT_PATHNAME) {
	Tcl_DStringFree(&dString1);
	Tcl_DStringFree(&dString2);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SortApplyProc --
 *
 *	Sorts the subnodes at a given node.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortApplyProc(
    Blt_TreeNode node,
    ClientData clientData,
    int order)			/* Not used. */
{
    TreeCmd *cmdPtr = clientData;

    if (!Blt_TreeIsLeaf(node)) {
	Blt_TreeSortNode(cmdPtr->tree, node, CompareNodes);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * SortOp --
 *  
 *---------------------------------------------------------------------- 
 */
static int
SortOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode top;
    SortData data;
    int result;

    if (GetNode(cmdPtr, objv[2], &top) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Process switches  */
    memset(&data, 0, sizeof(data));
    data.cmdPtr = cmdPtr;
    if (Blt_ProcessObjSwitches(interp, sortSwitches, objc - 3, objv + 3, 
	     (char *)&data, BLT_SWITCH_EXACT) < 0) {
	return TCL_ERROR;
    }
    if (data.command != NULL) {
	data.type = SORT_COMMAND;
    }
    data.cmdPtr = cmdPtr;
    sortData = data;
    if (data.mode == SORT_FLAT) {
	Blt_TreeNode *p, *nodeArr, node;
	int nNodes;
	Tcl_Obj *objPtr, *listObjPtr;
	int i;

	if (data.flags & SORT_RECURSE) {
	    nNodes = Blt_TreeSize(top);
	} else {
	    nNodes = Blt_TreeNodeDegree(top);
	}
	nodeArr = Blt_Calloc(nNodes, sizeof(Blt_TreeNode));
	assert(nodeArr);
	p = nodeArr;
	if (data.flags & SORT_RECURSE) {
	    for(node = top; node != NULL; node = Blt_TreeNextNode(top, node)) {
		*p++ = node;
	    }
	} else {
	    for (node = Blt_TreeFirstChild(top); node != NULL;
		 node = Blt_TreeNextSibling(node)) {
		*p++ = node;
	    }
	}
	qsort((char *)nodeArr, nNodes, sizeof(Blt_TreeNode),
	      (QSortCompareProc *)CompareNodes);
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (p = nodeArr, i = 0; i < nNodes; i++, p++) {
	    objPtr = Tcl_NewIntObj(Blt_TreeNodeId(*p));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	Blt_Free(nodeArr);
	result = TCL_OK;
    } else {
	if (data.flags & SORT_RECURSE) {
	    result = Blt_TreeApply(top, SortApplyProc, cmdPtr);
	} else {
	    result = SortApplyProc(top, cmdPtr, TREE_PREORDER);
	}
    }
    Blt_FreeSwitches(interp, sortSwitches, (char *)&data, 0);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * WithOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
WithOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr = NULL, **vobjv, **kobjv;
    Tcl_Obj *keyList = NULL, *arrName = NULL, *aListObjPtr = NULL;

    Blt_TreeKey key;
    Tcl_Obj *valuePtr, *varObj, *nullObj = NULL, *nodeObj = NULL;
    Tcl_Obj *hashObj = NULL, *starObj = NULL;
    Blt_TreeKeySearch keyIter;
    int vobjc, kobjc, i, result = TCL_OK, len, cnt = 0/*, isar */;
    int nobreak = 0, noupdate = 0, unset = 0, init = 0, aLen;
    char *var, *string, *aName = NULL, *aPat = NULL;
    int klen, kl, j;
    int *keySet = NULL;
    unsigned int inode;
    TagSearch cursor = {0};

    varObj = objv[2];
    var = Tcl_GetString(varObj);

    while (objc > 5) {
        string = Tcl_GetStringFromObj(objv[3],&len);
        if (strcmp(string, "-break") == 0) {
            nobreak = 1;
            objc -= 1;
            objv += 1;
        } else if (strcmp(string, "-noupdate") == 0) {
            noupdate = 1;
            objc -= 1;
            objv += 1;
        } else if (strcmp(string, "-unset") == 0) {
            unset = 1;
            objc -= 1;
            objv += 1;
        } else if (strcmp(string, "-init") == 0) {
            if (objc<5) goto wrongargs;
            nullObj = objv[4];
            init = 1;
            objc -= 2;
            objv += 2;
        } else if (strcmp(string, "-glob") == 0) {
            if (objc<5) goto wrongargs;
            aPat = Tcl_GetString(objv[4]);
            objc -= 2;
            objv += 2;
        } else if (strcmp(string, "-keys") == 0) {
            if (objc<5) goto wrongargs;
            keyList = objv[4];
            objc -= 2;
            objv += 2;
        } else if (strcmp(string, "-array") == 0) {
            if (objc<5) goto wrongargs;
            arrName = objv[4];
            aName = Tcl_GetStringFromObj(objv[4], &aLen);
            objc -= 2;
            objv += 2;
        }  else {
            Tcl_AppendResult(interp, "expected -init, -keys, -array, -glob, -unset, -noupdate, or -break: ", string, 0);
            return TCL_ERROR;
        }
    }
    if (var[0] == 0 && unset && keyList == NULL) {
        Tcl_AppendResult(interp, "can not use -unset with empty var", 0);
        goto error;
    }
    if (aPat != NULL && keyList != NULL) {
        Tcl_AppendResult(interp, "can not use -keys and -glob", 0);
        return TCL_ERROR;
    }
    if (init && keyList == NULL) {
        Tcl_AppendResult(interp, "must use -keys with -init", 0);
        return TCL_ERROR;
    }
    if (objc != 5) {
        wrongargs:
        Tcl_AppendResult(interp, "wrong number of args ", 0);
        return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[3],&len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    
    if (FindTaggedNodes(interp, cmdPtr, objv[3], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    if (keyList != NULL) {
        keyList = Tcl_DuplicateObj(keyList);
        Tcl_IncrRefCount(keyList);
        if (Tcl_ListObjGetElements(interp, keyList, &kobjc, &kobjv) 
            != TCL_OK) {
            Tcl_DecrRefCount(keyList);
            return TCL_ERROR;
        }
        if (init && kobjc>0) {
            keySet = (int*) Blt_Calloc(kobjc, sizeof(int));
        }
    }
    if (nullObj != NULL) {
        nullObj = Tcl_DuplicateObj(nullObj);
        Tcl_IncrRefCount(nullObj);
    }
    hashObj = Tcl_NewStringObj("#",-1);
    starObj = Tcl_NewStringObj("*",-1);
    nodeObj = Tcl_NewIntObj(0);
    Tcl_IncrRefCount(hashObj);
    Tcl_IncrRefCount(starObj);
    Tcl_IncrRefCount(nodeObj);
    for (node = FirstTaggedNode(&cursor);
        node != NULL; result = TCL_OK, node = NextTaggedNode(node, &cursor)) {
            
        inode = node->inode;
        
        if (Blt_TreeNotifyGet(cmdPtr->tree, node) != TCL_OK) {
            goto error;
        }
               
        cnt++;

        if (unset) {
            if (var[0]) {
                Tcl_UnsetVar(interp, var, 0);
            } else {
                for (i=0; i<kobjc; i++) {
                    char *kstr;
                    kstr = Tcl_GetStringFromObj(kobjv[i], &klen);
                    Tcl_UnsetVar(interp, kstr, 0);
                }
            }
        }
        if (init) {
            if (keySet != NULL) {
                for (i=0; i<kobjc; i++) {
                    keySet[i] = 0;
                }
            } else {
                for (i=0; i<kobjc; i++) {
                    if (var[0]) {
                        Tcl_ObjSetVar2(interp, varObj, kobjv[i], nullObj, 0);
                    } else {
                        Tcl_ObjSetVar2(interp, kobjv[i], NULL, nullObj, 0);
                    }
                }
            }
        }
        if (var[0]) {
            if (nodeObj->refCount ==  2) {
                Tcl_DecrRefCount(nodeObj);
                Tcl_SetIntObj(nodeObj, Blt_TreeNodeId(node));
                Tcl_IncrRefCount(nodeObj);
            } else {
                Tcl_DecrRefCount(nodeObj);
                nodeObj = Tcl_NewIntObj(Blt_TreeNodeId(node));
                Tcl_IncrRefCount(nodeObj);
            }
            if (Tcl_ObjSetVar2(interp, varObj, hashObj, nodeObj, 0) == NULL) {
                goto error;
            }
        }
    
        if ((keyList == NULL && var[0] != 0) || noupdate == 0) {
            if (listObjPtr && Tcl_IsShared(listObjPtr)) {
                Tcl_DecrRefCount(listObjPtr);
                listObjPtr = Tcl_NewListObj(0, NULL);
                Tcl_IncrRefCount(listObjPtr);
            } else {
                if (listObjPtr != NULL) {
                    Tcl_SetListObj(listObjPtr, 0, NULL);
                } else {
                    listObjPtr = Tcl_NewListObj(0, NULL);
                    Tcl_IncrRefCount(listObjPtr);
                }
            }
        }
        
        /* isar = 0; */
        if (arrName != NULL) {
            kl = strlen(aName);
                
            aListObjPtr = Tcl_NewListObj(0, NULL);
            Tcl_IncrRefCount(aListObjPtr);
            if (Blt_TreeArrayNames(NULL, cmdPtr->tree, node, aName,
                aListObjPtr, aPat) != TCL_OK) {
                    if (Blt_TreeGetValue(NULL, cmdPtr->tree, node,
                    aName, &valuePtr) != TCL_OK || valuePtr == NULL) {
                        continue;
                }
                Tcl_AppendResult(interp, "failed to split \"", aName, "\" for node ", Blt_Itoa(Blt_TreeNodeId(node)), 0);
                goto error;
            }
            if (Tcl_ListObjGetElements(interp, aListObjPtr, &vobjc, &vobjv) 
            != TCL_OK) {
                goto error;		/* Can't split object. */
            }
            for (j=0; j<vobjc; j++) {
                char *skey;
                Tcl_Obj *skeyObj;
                skeyObj = vobjv[j];
                skey = Tcl_GetString(skeyObj);
                if (keyList != NULL) {
                    char *kstr;
                    kl = strlen(skey);
                    for (i=0; i<kobjc; i++) {
                        kstr = Tcl_GetStringFromObj(kobjv[i], &klen);
                        if (kl == klen && kstr[0] == skey[0] && strcmp(kstr, skey) == 0) {
                            break;
                        }
                    }
                    if (i>=kobjc) continue;
                    if (keySet != NULL) keySet[i] = 1;
                }
                if (listObjPtr != NULL) {
                    Tcl_ListObjAppendElement(interp, listObjPtr, skeyObj);
                }
                if (Blt_TreeGetArrayValue(interp, cmdPtr->tree, node, aName,
                    skey, &valuePtr) != TCL_OK) {
                        goto error;
                }
                if (var[0]) {
                    if (Tcl_ObjSetVar2(interp, varObj, skeyObj, valuePtr,0) == NULL) {
                        goto error;
                    }
                } else {
                    if (Tcl_ObjSetVar2(interp, skeyObj, NULL, valuePtr,0) == NULL) {
                        goto error;
                    }
                }
            }
            goto doit;
        }
        
        if (keyList != NULL) {
            Tcl_Obj *skeyObj;
            for (i=0; i<kobjc; i++) {
                skeyObj = kobjv[i];
                key = Tcl_GetStringFromObj(skeyObj, &klen);
                if (Blt_TreeGetValue(NULL, cmdPtr->tree, node, key,
                    &valuePtr) != TCL_OK) {
                        continue;
                }


                if (i>=kobjc) continue;
                if (aPat != NULL && !Tcl_StringMatch(key, aPat)) {
                    continue;
                }
                if (keySet != NULL) keySet[i] = 1;
                if (listObjPtr != NULL) {
                    Tcl_ListObjAppendElement(interp, listObjPtr, skeyObj);
                }
                if (var[0]) {
                    if (Tcl_ObjSetVar2(interp, varObj, skeyObj, valuePtr,0) == NULL) {
                        goto error;
                    }
                } else {
                    if (Tcl_ObjSetVar2(interp, skeyObj, NULL, valuePtr,0) == NULL) {
                        goto error;
                    }
                }

            }
        } else {
            for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &keyIter); key != NULL; 
            key = Blt_TreeNextKey(cmdPtr->tree, &keyIter)) {
                Tcl_Obj *skeyObj;
            
                skeyObj = NULL;
                
                if (keyList != NULL) {
                    char *kstr;
                    kl = strlen(key);
                    for (i=0; i<kobjc; i++) {
                        skeyObj = kobjv[i];
                        kstr = Tcl_GetStringFromObj(skeyObj, &klen);
                        if (kl == klen && kstr[0] == key[0] && strcmp(kstr, key) == 0) {
                            break;
                        }
                    }
                    if (i>=kobjc) continue;
                    if (keySet != NULL) keySet[i] = 1;
                }
                if (aPat != NULL && !Tcl_StringMatch(key, aPat)) {
                    continue;
                }
                if (skeyObj == NULL) {
                    skeyObj = Tcl_NewStringObj(key, -1);
                }
                if (listObjPtr != NULL) {
                    Tcl_ListObjAppendElement(interp, listObjPtr, skeyObj);
                }
                if (Blt_TreeGetValue(NULL, cmdPtr->tree, node, key, &valuePtr) 
                != TCL_OK) {
                    goto error;
                }

                /* Don't expose the array (maybe we should do this for all types?) */
                /*if (Blt_IsArrayObj(valuePtr)) {
                    isar = 1;
                    valuePtr = Tcl_NewStringObj(Tcl_GetString(valuePtr), -1);
                }*/
                if (var[0]) {
                    if (Tcl_ObjSetVar2(interp, varObj, skeyObj, valuePtr,0) == NULL) {
                        goto error;
                    }
                } else {
                    if (Tcl_ObjSetVar2(interp, skeyObj, NULL, valuePtr,0) == NULL) {
                        goto error;
                    }
                }
            }
        }

doit:
    
        if (init && keySet != NULL) {
            for (i=0; i<kobjc; i++) {
                if (keySet[i] == 0) {
                    if (var[0]) {
                        Tcl_ObjSetVar2(interp, varObj, kobjv[i], nullObj, 0);
                    } else {
                        Tcl_ObjSetVar2(interp, kobjv[i], NULL, nullObj, 0);
                    }
                }
            }
        }
        if (keyList == NULL && var[0]) {
            if (cnt > 1) {
                if (Tcl_ObjSetVar2(interp, varObj, starObj, listObjPtr, 0) == NULL) {
                    goto error;
                }
            } else {
                if (Tcl_SetVar2Ex(interp, var, "*", listObjPtr, 0) == NULL) {
                    goto error;
                }
            }
        }

        result = Tcl_EvalObjEx(interp, objv[4], 0);
        if (cmdPtr->delete) {
            goto error;
        }
        if (result != TCL_OK && result != TCL_BREAK && result != TCL_CONTINUE && result != TCL_RETURN) {
            break;
        } else if (noupdate == 0 && Blt_TreeNodeDeleted(node)==0  &&
            node->inode == inode) {
            Tcl_Obj *newValuePtr;
        
            if (Tcl_ListObjGetElements(interp, listObjPtr, &vobjc, &vobjv) 
            != TCL_OK) {
                goto error;		/* Can't split object. */
            }
            for (i = 0; i<vobjc; i++) {
                char *string1, *string2, *skey;
                int l1, l2;
                
                skey = Tcl_GetString(vobjv[i]);
                if (arrName != NULL) {
                    if (Blt_TreeGetArrayValue(NULL, cmdPtr->tree, node, aName,
                        skey, &valuePtr) != TCL_OK) {
                        continue;
                    }
                } else {
                    if (Blt_TreeGetValue(NULL, cmdPtr->tree, node,
                        skey, &valuePtr) != TCL_OK) {
                        continue;
                    }
                }
                if (var[0]) {
                    if ((newValuePtr = Tcl_ObjGetVar2(interp, varObj, vobjv[i], 0)) == NULL) {
                        Tcl_ResetResult(interp);
                        continue;
                    }
                    
                } else {
                    if ((newValuePtr = Tcl_ObjGetVar2(interp, vobjv[i], NULL, 0)) == NULL) {
                        Tcl_ResetResult(interp);
                        continue;
                    }
                }
                /*if (isar && Blt_IsArrayObj(valuePtr)) {
                } else {
                    if (newValuePtr == valuePtr) continue;
                }*/
                if (newValuePtr == valuePtr) continue;
                
                string1 = (char*) Tcl_GetStringFromObj(newValuePtr, &l1);
                string2 = (char*) Tcl_GetStringFromObj(valuePtr, &l2);
                if (l1 != l2 || memcmp(string1, string2, l1) != 0) {
                    if (arrName != NULL) {
                        if (Blt_TreeSetArrayValue(interp, cmdPtr->tree, node,
                            aName, skey, newValuePtr) != TCL_OK) {
                                goto error;
                        }
                    } else {
                        if (Blt_TreeSetValue(NULL, cmdPtr->tree, node,
                            Tcl_GetString(vobjv[i]), newValuePtr) != TCL_OK) {
                            goto error;
                        }
                    }
                }
            }
        }
        if (nobreak && result == TCL_CONTINUE) continue;
        if (result != TCL_OK) {
            break;
        }
    }
    if (nobreak && result == TCL_BREAK) {
        result = TCL_OK;
    }

    if (result == TCL_OK) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(cnt));
    }
doneall:
    if (listObjPtr != NULL) { Tcl_DecrRefCount(listObjPtr); }
    if (aListObjPtr != NULL) { Tcl_DecrRefCount(aListObjPtr); }
    if (nullObj != NULL) { Tcl_DecrRefCount(nullObj); }
    if (nodeObj != NULL) { Tcl_DecrRefCount(nodeObj); }
    if (starObj != NULL) { Tcl_DecrRefCount(starObj); }
    DoneTaggedNodes(&cursor);
    if (keyList != NULL) {
        Tcl_DecrRefCount(keyList);
    }
    if (keySet != NULL) {
        Blt_Free(keySet);
    }
    return result;
    
error:
    result = TCL_ERROR;
    goto doneall;
}

/*
 *----------------------------------------------------------------------
 *
 * ForeachOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
ForeachOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    int len, result;
    char *var/*, *string*/;
    TagSearch cursor = {0};

    var = Tcl_GetString(objv[2]);

    if (objc != 5) {
        Tcl_AppendResult(interp, "wrong number of args ", 0);
        return TCL_ERROR;
    }
    /*string =*/ Tcl_GetStringFromObj(objv[3],&len);
    if (len == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    }
    
    if (FindTaggedNodes(interp, cmdPtr, objv[3], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    result = TCL_OK;
    for (node = FirstTaggedNode(&cursor);
        node != NULL; result = TCL_OK, node = NextTaggedNode(node, &cursor)) {

        if (Tcl_SetVar2Ex(interp, var, 0, Tcl_NewIntObj(Blt_TreeNodeId(node)),
            0) == NULL) {
            goto error;
        }
        result = Tcl_EvalObjEx(interp, objv[4], 0);
        if (cmdPtr->delete) {
            goto error;
        }
        if (result == TCL_BREAK) { result = TCL_OK; break; }
        if (result == TCL_CONTINUE ) continue;
	if (result == TCL_ERROR) {
            Tcl_AppendResult(interp,
            "\n    (\"tree foreach\" body line ", Blt_Itoa(Tcl_GetErrorLine(interp)), ")\n", 0);
             break;
        }
        if (result != TCL_OK) {
            break;
        }
    }

    DoneTaggedNodes(&cursor);
    return result;
    
error:
    DoneTaggedNodes(&cursor);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns the names of keys for a node or array value.
 *
 *---------------------------------------------------------------------- 
 */
static int
NamesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr;
    
    if (objc>2) {
        if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        node = Blt_TreeRootNode(cmdPtr->tree);
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (objc >= 4) { 
	char *string;

	string = Tcl_GetString(objv[3]);
	if (Blt_TreeArrayNames(interp, cmdPtr->tree, node, string, listObjPtr,
	       objc>4?Tcl_GetString(objv[4]):NULL)
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Blt_TreeKey key;
	Tcl_Obj *objPtr;
	Blt_TreeKeySearch keyIter;

	for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &keyIter); key != NULL; 
	     key = Blt_TreeNextKey(cmdPtr->tree, &keyIter)) {
	    objPtr = Tcl_NewStringObj(key, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}	    
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
*----------------------------------------------------------------------
*
* ValuesOp --
*
*	Returns the values of keys for a node or array value.
*
*---------------------------------------------------------------------- 
*/
static int
ValuesOp(
TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
    {
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr;
    int names = 0;
    
    if (objc>2) {
        if (GetNode(cmdPtr, objv[2], &node) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        node = Blt_TreeRootNode(cmdPtr->tree);
    }
    if (objc>4 && Tcl_GetBooleanFromObj(interp, objv[4], &names) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (objc >= 4) { 
        char *string;

        string = Tcl_GetString(objv[3]);
        if (Blt_TreeArrayValues(interp, cmdPtr->tree, node, string, listObjPtr, names)
        != TCL_OK) {
            Tcl_DecrRefCount(listObjPtr);
            return TCL_ERROR;
        }
    } else {
        Blt_TreeKey key;
        Tcl_Obj *objPtr;
        Blt_TreeKeySearch keyIter;

        for (key = Blt_TreeFirstKey(cmdPtr->tree, node, &keyIter); key != NULL; 
        key = Blt_TreeNextKey(cmdPtr->tree, &keyIter)) {
            if (Blt_TreeGetValueByKey(interp, cmdPtr->tree, node, key, &objPtr) != TCL_OK) {
                Tcl_DecrRefCount(listObjPtr);
                return TCL_ERROR;
            }
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }	    
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * OldValueOp --
 *
 *	Returns the old value before last update
 *
 *---------------------------------------------------------------------- 
 */
static int
OldValueOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Tcl_Obj *objPtr = NULL;
    int len;
    
    if (objc>2) {
        Blt_TreeOldValue(interp, cmdPtr->tree, &objPtr, objv[2]);
        return TCL_OK;
    }
    Blt_TreeOldValue(interp, cmdPtr->tree, &objPtr, NULL);
    if (objPtr == NULL) {
        return TCL_OK;
    }
    /* If last update was append/lappend, return truncated value. */
    if (cmdPtr->updTyp == 1) {
        Tcl_GetStringFromObj(objPtr, &len);
        if (len>cmdPtr->oldLen && cmdPtr->oldLen>=0) {
            objPtr = Tcl_DuplicateObj(objPtr);
            Tcl_SetObjLength(objPtr, cmdPtr->oldLen);
        }
    } else if (cmdPtr->updTyp == 2) {
        if (Tcl_ListObjLength(interp, objPtr, &len) != TCL_OK) {
            return TCL_ERROR;
        }
        if (len>cmdPtr->oldLen && cmdPtr->oldLen>=0) {
            objPtr = Tcl_DuplicateObj(objPtr);
            if (Tcl_ListObjReplace(interp, objPtr, cmdPtr->oldLen, len-cmdPtr->oldLen, 0, NULL) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    } 
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

typedef struct SqlData {
    TreeCmd *cmdPtr;
    Blt_TreeNode parent;
    int insertPos;
    Tcl_Interp *interp;
    int cnt;
    int rc;
    int max;
    int llen;
    char *labelcol;
    char *tagcol;
    char *key;
    int rinc;
    char *nullstr;
    Tcl_Obj *tags;
    int vobjc;
    Tcl_Obj **vobjv;
    Tcl_Obj *skipcols;
    Tcl_Obj *treecols;
    Tcl_Obj *pathcol ;
    int fixed;
} SqlData;

#ifdef BLT_USE_OLD_SQLITE
/*
 * Callback for sqlload.
 *
 */
static int SqlCallback(SqlData *sqlPtr, int argc, char **argv, char **azColName)
{
    int i, rid, lid, n, tcid;
    Tcl_Interp *interp = sqlPtr->interp;
    TreeCmd *cmdPtr = sqlPtr->cmdPtr;
    Blt_TreeNode node;
    char string[200];
    Tcl_Obj *objPtr;
    Tcl_DString dStr;
    char *label = NULL;
    
    rid = -1;
    n = -1;
    lid = -1;
    tcid = -1;
    if (sqlPtr->rinc>0) {
        for(i=0; i<argc; i++){
            if (argv[i]==NULL) continue;
            if (azColName[i][0] == 'r' && !strcmp(azColName[i], "rowid")) {
                rid = atoi(argv[i])+sqlPtr->rinc;
                n = i;
                break;
            }
        }
    }
    if (sqlPtr->labelcol != NULL && sqlPtr->labelcol[0]) {
        for(i=0; i<argc; i++){
            if (argv[i]==NULL) continue;
            if (azColName[i][0] == sqlPtr->labelcol[0]
            && !strcmp(azColName[i], sqlPtr->labelcol)) {
                lid = i;
                label = argv[i];
                break;
            }
        }
    }
    if (sqlPtr->tagcol != NULL && sqlPtr->tagcol[0]) {
        for(i=0; i<argc; i++){
            if (argv[i]==NULL) continue;
            if (azColName[i][0] == sqlPtr->tagcol[0]
            && !strcmp(azColName[i], sqlPtr->agcol)) {
                tcid = i;
                break;
            }
        }
    }
    if (sqlPtr->tagcol != NULL) {
        if (AddTag(cmdPtr, node, Tcl_GetString(sqlPtr->vobjv[i])) != TCL_OK) {
            return 1;
        }
    }     


    if (rid>1) {
        node = Blt_TreeCreateNodeWithId(cmdPtr->tree, sqlPtr->parent, label,
            rid, sqlPtr->insertPos);
        if (node == NULL) {
            Tcl_AppendResult(interp, "node can not be created", 0);
            sqlPtr->rc = TCL_ERROR;
            return  1;
        }
    } else {
        node = Blt_TreeCreateNode(cmdPtr->tree, sqlPtr->parent, label,
            sqlPtr->insertPos);
        if (node == NULL) {
            Tcl_AppendResult(interp, "node can not be created", 0);
            sqlPtr->rc = TCL_ERROR;
            return 1;
        }
    }
    if (label == NULL) {
        sprintf(string, "%d", Blt_TreeNodeId(node));
        Blt_TreeRelabelNode2(node, string);
    }
    

    if (sqlPtr->key != NULL) {
        Tcl_DStringInit(&dStr);
        for(i=0; i<argc; i++){
            if (argv[i] == NULL && sqlPtr->nullstr == NULL) continue;
            if (n == i) continue;
            Tcl_DStringAppendElement(&dStr, azColName[i]);
            Tcl_DStringAppendElement(&dStr, argv[i]?argv[i]:sqlPtr->nullstr);
        }
        objPtr = Tcl_NewStringObj(Tcl_DStringValue(&dStr), -1);
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, sqlPtr->key, objPtr) 
            != TCL_OK) {
            Tcl_DecrRefCount(objPtr);
            sqlPtr->rc = TCL_ERROR;
            return 1;
        }
    } else {
        for(i=0; i<argc; i++){
            if (argv[i] == NULL && sqlPtr->nullstr == NULL) continue;
            if (n == i) continue; */
            objPtr = Tcl_NewStringObj(argv[i]?argv[i]:sqlPtr->nullstr, -1);
            if (Blt_TreeSetValue(interp, cmdPtr->tree, node, azColName[i], objPtr) 
            != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                sqlPtr->rc = TCL_ERROR;
                return 1;
            }
        }
    }
    if (sqlPtr->tags != NULL) {        
        for (i=0; i<sqlPtr->vobjc; i++) {
            if (AddTag(cmdPtr, node, Tcl_GetString(sqlPtr->vobjv[i])) != TCL_OK) {
                return 1;
            }
        }
    }
    if (Blt_TreeInsertPost(cmdPtr->tree, node) == NULL) {
        DeleteNode(cmdPtr, node);
        return 1;
    }
    if (sqlPtr->fixed) {
        node->flags |= TREE_NODE_FIXED_FIELDS;
    }
    
    sqlPtr->cnt++;
    if (sqlPtr->max > 0 && sqlPtr->cnt >= sqlPtr->max) {
        sqlPtr->rc = TCL_BREAK;
        return 1;
    }

    return 0;
}
#else
/*
 * Obj Callback for sqlload.
 *
 */
#if 0
static int SqlCallbackObj(SqlData *sqlPtr, int argc, Tcl_Obj **objv, Tcl_Obj **azColName)
{
    int i, j, rid, lid, n, tcid, vobjc, tobjc;
    Tcl_Interp *interp = sqlPtr->interp;
    TreeCmd *cmdPtr = sqlPtr->cmdPtr;
    Blt_TreeNode node;
    char string[200];
    Tcl_Obj *objPtr, **vobjv, **tobjv;
    Tcl_DString dStr;
    char *label = NULL, *cn, *ridStr = "";
    int treeCols[100], pcol = -1;
    
    rid = -1;
    n = -1;
    lid = -1;
    tcid = -1;
    if (sqlPtr->rinc>0) {
        for(i=0; i<argc; i++){
            if (objv[i]==NULL) continue;
            cn = Tcl_GetString(azColName[i]);
            if (cn[0] == 'r' && !strcmp(cn, "rowid")) {
                if (Tcl_GetIntFromObj(NULL, objv[i], &rid) == TCL_OK) {
                    rid += sqlPtr->rinc;
                    ridStr = Tcl_GetString(objv[i]);
                    n = i;
                    break;
                }
            }
        }
    }
    if (sqlPtr->labelcol != NULL && sqlPtr->labelcol[0]) {
        for(i=0; i<argc; i++){
            if (objv[i]==NULL) continue;
            cn = Tcl_GetString(azColName[i]);
            if (cn[0] == sqlPtr->labelcol[0]
            && !strcmp(cn, sqlPtr->labelcol)) {
                lid = i;
                label = (objv[i] ? Tcl_GetString(objv[i]) : sqlPtr->nullstr );
                break;
            }
        }
    }
    if (sqlPtr->treecols) {
            
        if (Tcl_ListObjGetElements(interp, sqlPtr->treecols, &tobjc, &tobjv) 
        != TCL_OK || tobjc>=100) {
            sqlPtr->rc = TCL_ERROR;
            return 1;
        }
        for (i=0; i<tobjc; i++) {
            treeCols[i] = -1;
        }
    }
    if (rid>=1) {
        node = Blt_TreeCreateNodeWithId(cmdPtr->tree, sqlPtr->parent, label,
            rid, sqlPtr->insertPos);
        if (node == NULL) {
            Tcl_AppendResult(interp, "node can not be created with id: ", ridStr, 0);
            sqlPtr->rc = TCL_ERROR;
            return  1;
        }
    } else {
        node = Blt_TreeCreateNode(cmdPtr->tree, sqlPtr->parent, label,
            sqlPtr->insertPos);
        if (node == NULL) {
            Tcl_AppendResult(interp, "node can not be created", 0);
            sqlPtr->rc = TCL_ERROR;
            return 1;
        }
    }
    if (label == NULL) {
        sprintf(string, "%d", Blt_TreeNodeId(node));
        Blt_TreeRelabelNode2(node, string);
    }
    

    Tcl_DStringInit(&dStr);
    for(i=0; i<argc; i++){
        int clen;
        if (objv[i] == NULL && sqlPtr->nullstr == NULL) continue;
        cn = Tcl_GetStringFromObj(azColName[i], &clen);
        if (sqlPtr->treecols) {
            for (j=0; j<tobjc; j++) {
                if (!strcmp(Tcl_GetString(azColName[i]), Tcl_GetString(tobjv[j]))) {
                    treeCols[j] = i;
                    break;
                }
            }
        }
        if (sqlPtr->pathcol) {
            if (!strcmp(Tcl_GetString(azColName[i]), Tcl_GetString(sqlPtr->pathcol))) {
                pcol = i;
            }
        }
        if (sqlPtr->tagcol != NULL && sqlPtr->tagcol[0] &&
            strcmp(cn, sqlPtr->tagcol) == 0) {
            if (AddTag(cmdPtr, node, Tcl_GetString(objv[i])) != TCL_OK) {
                goto error;
            }
            continue;
        }

        if (sqlPtr->skipcols) {
            char *sc;
            int slen;
            
            if (Tcl_ListObjGetElements(interp, sqlPtr->skipcols, &vobjc, &vobjv) 
            != TCL_OK) {
                goto error;
            }
            for (j=0; j<vobjc; j++) {
                sc = Tcl_GetStringFromObj(vobjv[j], &slen);
                if (slen == clen && *sc == *cn && strcmp(cn, sc) == 0) {
                    break;
                }
            }
            if (j<vobjc) continue;
        }
        if (sqlPtr->key != NULL) {
            Tcl_DStringAppendElement(&dStr, cn);
            Tcl_DStringAppendElement(&dStr, objv[i]?Tcl_GetString(objv[i]):sqlPtr->nullstr);
        } else {
            if (objv[i] != NULL) {
                objPtr = objv[i];
            } else {
                objPtr = Tcl_NewStringObj(sqlPtr->nullstr, -1);
            }
            if (Blt_TreeSetValue(interp, cmdPtr->tree, node, cn, objPtr) 
                != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                sqlPtr->rc = TCL_ERROR;
                goto error;;
            }
        }
    }
    if (sqlPtr->key != NULL) {
        /* Set all values in a single key. */
        objPtr = Tcl_NewStringObj(Tcl_DStringValue(&dStr), -1);
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, sqlPtr->key, objPtr) 
        != TCL_OK) {
            Tcl_DecrRefCount(objPtr);
            sqlPtr->rc = TCL_ERROR;
            goto error;
        }
    }

    if (sqlPtr->tags != NULL) {        
        for (i=0; i<sqlPtr->vobjc; i++) {
            if (AddTag(cmdPtr, node, Tcl_GetString(sqlPtr->vobjv[i])) != TCL_OK) {
                goto error;
            }
        }
    }
    
    if (sqlPtr->treecols) {
        Blt_TreeNode snode, parent = sqlPtr->parent;
        for (i=0; i<tobjc; i++) {
            if (treeCols[i] == -1) {
                Tcl_AppendResult(interp, "missing col in -treecols", 0);
                goto error;
            }
        }
        for (i=0; i<tobjc; i++) {
            char *tcp;
            tcp = Tcl_GetString(objv[treeCols[i]]);
            snode = Blt_TreeFindChild(parent, tcp);
            if (snode != NULL) {
                parent = snode;
                continue;
            }
            snode = Blt_TreeCreateNode(cmdPtr->tree, parent, tcp, -1);
            if (snode == NULL) {
                goto error;
            }
            parent = snode;
        }
        if (Blt_TreeMoveNode(cmdPtr->tree, node, parent, NULL) != TCL_OK) {
            Tcl_AppendResult(interp, "relocate failed in -treecols", 0);
            goto error;
        }
    }
    if (sqlPtr->pathcol && pcol >= 0) {
        Blt_TreeNode snode, parent = sqlPtr->parent;
        if (Tcl_ListObjGetElements(interp, vobjv[pcol], &tobjc, &tobjv) 
            != TCL_OK) {
            goto error;
        }
        for (i=0; i<tobjc; i++) {
            char *tcp;
            tcp = Tcl_GetString(tobjv[i]);
            snode = Blt_TreeFindChild(parent, tcp);
            if (snode != NULL) {
                parent = snode;
                continue;
            }
            snode = Blt_TreeCreateNode(cmdPtr->tree, parent, tcp, -1);
            if (snode == NULL) {
                goto error;
            }
            parent = snode;
        }
        if (Blt_TreeMoveNode(cmdPtr->tree, node, parent, NULL) != TCL_OK) {
            Tcl_AppendResult(interp, "relocate failed in -pathcol", 0);
            goto error;
        }
    }

    if (Blt_TreeInsertPost(cmdPtr->tree, node) == NULL) {
        goto error;
    }
    Tcl_DStringFree(&dStr);
    if (sqlPtr->fixed || (cmdPtr->tree->treeObject->flags & TREE_FIXED_KEYS)) {
        node->flags |= TREE_NODE_FIXED_FIELDS;
    }
    
    sqlPtr->cnt++;
    if (sqlPtr->max > 0 && sqlPtr->cnt >= sqlPtr->max) {
        sqlPtr->rc = TCL_BREAK;
        return 1;
    }
    return 0;
    
error:
    DeleteNode(cmdPtr, node);
    Tcl_DStringFree(&dStr);
    return 1;
}
#endif
#endif

#ifndef OMIT_SQLITE3_LOAD
/* #include <sqlite3.h> */

extern void *sqlite3_tclhandle();
extern int sqlite3_open();
extern char *sqlite3_errmsg();
extern void sqlite3_close(void *);
extern int sqlite3_exec_tclobj();
#endif

/*
 *----------------------------------------------------------------------
 *
 * SqlloadOp --
 *
 *	Load sql
 * TODO: use sqlite header
 *
 *---------------------------------------------------------------------- 
 */
static int
SqlloadOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
#ifdef OMIT_SQLITE3_LOAD
    Tcl_AppendResult(interp, "sqlload unsupported", 0);
    return TCL_ERROR;
#else     
    char *string, *cp;
    int rc, result, optInd, isFile = 0, sclen, tclen;
    void *db = NULL;
    Tcl_Obj *zErrPtr = NULL;
    /*const char *zErrMsg = 0; */
    SqlData data;
    Tcl_DString dStr;
    Tcl_Obj *CONST *oobjv = objv;
    
    enum optInd {
        OP_TAGS, OP_FIXED, OP_KEY, OP_LABELCOL, OP_ROWID, OP_MAX,
        OP_NULLSTR, OP_PARENT, OP_PATHCOL, OP_INSPOS, OP_SKIPCOL, OP_TAGCOL,
        OP_TREECOLS        
    };
    static char *optArr[] = {
        "-addtags", "-fixed", "-key", "-labelcol", "-maprowid", "-max",
        "-nullvalue", "-parent", "-pathcol", "-pos", "-skipcols", "-tagcol",
        "-treecols",
        0
    };

    data.cmdPtr = cmdPtr;
    data.insertPos = -1;
    data.parent = Blt_TreeRootNode(cmdPtr->tree);
    data.interp = interp;
    data.max = 100000;
    data.cnt = 0;
    data.rc = TCL_OK;
    data.rinc = 0;
    data.fixed = 0;
    data.key = NULL;
    data.labelcol = NULL;
    data.tagcol = NULL;
    data.skipcols = NULL;
    data.nullstr = NULL;
    data.tags = NULL;
    data.pathcol = NULL;
    data.treecols = NULL;
    
    result = TCL_OK;
    string = Tcl_GetString(objv[2]);
    if (objc<4) {
        Tcl_WrongNumArgs(interp, 2, objv, "?options? dbhfile selectstmt");
        return TCL_ERROR;
    }
    while (objc>4 && string[0] == '-') {
        if (Tcl_GetIndexFromObj(interp, objv[2], optArr, "option",
            0, &optInd) != TCL_OK) {
                return TCL_ERROR;
        }
        switch (optInd) {
        
        case OP_FIXED:
            data.fixed = 1;
            objc -= 1;
            objv += 1;
            break;
        
        case OP_INSPOS:
            if (objc<6) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &data.insertPos)) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;

        case OP_KEY:
            if (objc<6) { goto missingArg; }
            data.key = Tcl_GetString(objv[3]);
            objc -= 2;
            objv += 2;
            break;
        
        case OP_LABELCOL:
            if (objc<6) { goto missingArg; }
            data.labelcol = Tcl_GetStringFromObj(objv[3], &data.llen);
            objc -= 2;
            objv += 2;
            break;
        
        case OP_MAX:
            if (objc<6) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &data.max)) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
            
        case OP_NULLSTR:
            if (objc<6) { goto missingArg; }
            data.nullstr = Tcl_GetStringFromObj(objv[3], &data.llen);
            objc -= 2;
            objv += 2;
            break;
            
        case OP_ROWID:
            if (objc<6) { goto missingArg; }
            if (Tcl_GetIntFromObj(interp, objv[3], &data.rinc)) {
                return TCL_ERROR;
            }
            if (data.rinc<=0) {
                Tcl_AppendResult(interp, "-rowidmap must be > 0", 0);
                return TCL_ERROR;
            }

            objc -= 2;
            objv += 2;
            break;
            
        case OP_PARENT:
            if (objc<6) { goto missingArg; }
            if (GetNode(cmdPtr, objv[3], &data.parent) != TCL_OK) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
            
        case OP_SKIPCOL:
            if (objc<6) { goto missingArg; }
            if (Tcl_ListObjLength(interp, objv[3], &sclen) != TCL_OK) {
                return TCL_ERROR;
            }
            data.skipcols = objv[3];
            objc -= 2;
            objv += 2;
            break;
            
        case OP_TAGCOL:
            if (objc<6) { goto missingArg; }
            data.tagcol = Tcl_GetStringFromObj(objv[3], &data.llen);
            objc -= 2;
            objv += 2;
            break;
        
        case OP_TAGS:
            if (objc<6) { goto missingArg; }
            data.tags = objv[3];
            if (Tcl_ListObjGetElements(interp, data.tags, &data.vobjc, &data.vobjv)
                != TCL_OK) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_TREECOLS:
            if (objc<6) { goto missingArg; }
            if (Tcl_ListObjLength(interp, objv[3], &tclen) != TCL_OK) {
                return TCL_ERROR;
            }
            if (tclen>0) {
                data.treecols = objv[3];
            }
            objc -= 2;
            objv += 2;
            break;
        case OP_PATHCOL:
            if (objc<6) { goto missingArg; }
            data.pathcol = objv[3];
            objc -= 2;
            objv += 2;
            break;
        }
        if (objc>2) {
            string = Tcl_GetString(objv[2]);
        }

    }
    if (objc!=4) {
        Tcl_WrongNumArgs(interp, 2, oobjv, "?options? dbhfile selectstmt");
        return TCL_ERROR;
    }
    if (data.pathcol && data.treecols) {
        Tcl_AppendResult(interp, "can not use -pathcol and -treecols", 0);
        return TCL_ERROR;
    }
#ifndef OMIT_SQLITE3_HANDLE    
    db = sqlite3_tclhandle(interp, string);
#endif    
    if (db == NULL) {
        isFile = 1;
        Tcl_DStringInit(&dStr);
        Tcl_DStringAppendElement(&dStr, "file");
        Tcl_DStringAppendElement(&dStr, "exists");
        Tcl_DStringAppendElement(&dStr, string);
        if (Tcl_Eval( interp, Tcl_DStringValue(&dStr)) != TCL_OK) {
            return TCL_ERROR;
        }
        cp =  Tcl_GetStringResult(interp);
        if (cp[0] != '1') {
            Tcl_AppendResult(interp, "file must exists ", string, 0);
            return TCL_ERROR;
        }
        Tcl_ResetResult(interp);
        if (cmdPtr->delete) {
            return TCL_ERROR;
        }
        rc = sqlite3_open(string, &db);
        if (rc) {
            Tcl_AppendResult(interp, "database open failed for ", string, sqlite3_errmsg(db), 0);
            sqlite3_close(db);
            return TCL_ERROR;
        }
    }
    if (db == NULL) {
        Tcl_AppendResult(interp, "database open failed for ", string, 0);
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
#ifdef BLT_USE_OLD_SQLITE
    rc = sqlite3_exec(db, string, SqlCallback, &data, &zErrMsg);
#else
    rc = sqlite3_exec_tclobj(db, objv[3], SqlCallbackObj, &data, &zErrPtr);
#endif
    if (data.rc == TCL_OK && rc != 0 /* SQLITE_OK */ ){
        Tcl_AppendResult(interp, "SQL error: ", (zErrPtr?Tcl_GetString(zErrPtr):0), 0);
        result = TCL_ERROR;
    } else if (data.rc != TCL_BREAK) {
        result = data.rc;
    }
    if (isFile) {
        sqlite3_close(db);
    }
    if (result == TCL_OK) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(data.cnt));
    }
    if (zErrPtr != NULL) {
        Tcl_DecrRefCount(zErrPtr);
    }
    return result;
    
missingArg:
    if (zErrPtr != NULL) {
        Tcl_DecrRefCount(zErrPtr);
    }
    Tcl_AppendResult(interp, "missing argument for find option \"",
        Tcl_GetString(objv[3]), "\"", (char *)NULL);
    return TCL_ERROR;
#endif
}
/*
 *----------------------------------------------------------------------
 *
 * VecloadOp --
 *
 *	Returns the old value before last update
 *k
 *---------------------------------------------------------------------- 
 */
static int
VecloadOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Tcl_Obj *objPtr = NULL;
    int i, count = 0, len;
    Blt_TreeNode node;
    Blt_Vector *vec;
    double d;
    char *string;
    TagSearch cursor = {0};

    if (Blt_GetVector(interp, Tcl_GetString(objv[2]), &vec) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    if (objc==4) {
        /* 1-1 dump of inodes. */
        for (i=0; i<vec->numValues; i++) {
            d = vec->valueArr[i];
            node = Blt_TreeGetNode(cmdPtr->tree, i);
            if (node == NULL) continue;
            objPtr = Tcl_NewDoubleObj(d);
            count++;
            if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, 
                objPtr) != TCL_OK) {
                    return TCL_ERROR;
            }
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
        return TCL_OK;
    }
    string = Tcl_GetStringFromObj(objv[4],&len);
    if (len == 0) goto done;
    
    if (FindTaggedNodes(interp, cmdPtr, objv[4], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        count++;
        if (count > vec->numValues) {
            break;
        }
        d = vec->valueArr[count-1];
        objPtr = Tcl_NewDoubleObj(d);
        if (Blt_TreeSetValue(interp, cmdPtr->tree, node, string, 
            objPtr) != TCL_OK) {
                continue;
        }
    }
done:
    DoneTaggedNodes(&cursor);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}

static int
VecdumpOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Tcl_Obj *objPtr = NULL;
    int i, count = 0, max = 0, len;
    Blt_TreeNode node, root;
    Blt_Vector *vec;
    double d;
    char *string;
    TagSearch cursor = {0};

    if (Blt_GetVector(interp, Tcl_GetString(objv[2]), &vec) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    
    if (objc==4) {
        /* 1-1 dump of inodes. */
        node = MaxNode(cmdPtr->tree);
        root = Blt_TreeRootNode(cmdPtr->tree);

        max = node->inode;
        max++;
        if (max != vec->numValues) {
            if (Blt_ResizeVector(vec, max) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        for (i=0; i<vec->numValues; i++) {
            vec->valueArr[i] = 0.0;
        }
        for (node = root; node != NULL; node = Blt_TreeNextNode(root, node)) {
            i = node->inode;
            if (i>=vec->numValues) continue;
            node = Blt_TreeGetNode(cmdPtr->tree, i);
            if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string, 
                &objPtr) != TCL_OK) {
                    continue;
            }
            if (Tcl_GetDoubleFromObj(NULL, objPtr, &d) != TCL_OK) {
                continue;
            }
            vec->valueArr[i] = d;
            count++;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
        return TCL_OK;
    }
    string = Tcl_GetStringFromObj(objv[4],&len);
    if (len == 0) goto done;
    
    if (FindTaggedNodes(interp, cmdPtr, objv[4], &cursor) != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = FirstTaggedNode(&cursor);
        node != NULL; node = NextTaggedNode(node, &cursor)) {
        if (count >= vec->numValues) {
            if (Blt_ResizeVector(vec, count+100) != TCL_OK) {
                DoneTaggedNodes(&cursor);
                return TCL_ERROR;
            }
        }
        count++;
        vec->valueArr[count-1] = 0;
        if (Blt_TreeGetValue(interp, cmdPtr->tree, node, string, 
            &objPtr) != TCL_OK) {
                continue;
        }
        if (Tcl_GetDoubleFromObj(NULL, objPtr, &d) != TCL_OK) {
            continue;
        }
        vec->valueArr[count-1] = d;
    }
    DoneTaggedNodes(&cursor);
    if (Blt_ResizeVector(vec, count) != TCL_OK) {
        return TCL_ERROR;
    }
done:
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}


/*
 * --------------------------------------------------------------
 *
 * TreeInstObjCmd --
 *
 * 	This procedure is invoked to process commands on behalf of
 *	the tree object.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 * --------------------------------------------------------------
 */
static Blt_OpSpec treeOps[] =
{
    {"ancestor", 2, (Blt_Op)AncestorOp, 4, 4, "node1 node2",},
    {"append", 6, (Blt_Op)AppendOp, 4, 0, "node key string ?...?",},
    {"appendi", 7, (Blt_Op)AppendiOp, 4, 0, "node key string ?...?",},
    {"apply", 4, (Blt_Op)ApplyOp, 3, 0, "node ?switches?",},
#ifndef NO_ATTACHCMD
    {"attach", 4, (Blt_Op)AttachOp, 2, 4, "?-notags? ?tree?",},
#endif
    {"children", 2, (Blt_Op)ChildrenOp, 3, 6, "?-labels? node ?first? ?last?",},
    {"copy", 2, (Blt_Op)CopyOp, 4, 0, 
	"srcNode ?destTree? destNode ?switches?",},
    {"create", 3, (Blt_Op)CreateOp, 3, 0, "?switches?",},
    {"degree", 3, (Blt_Op)DegreeOp, 3, 3, "node",},
    {"delete", 3, (Blt_Op)DeleteOp, 3, 0, "node ?node...?",},
    {"depth", 3, (Blt_Op)DepthOp, 3, 3, "node",},
    {"dictset", 3, (Blt_Op)DictsetOp, 2, 3, "?makeDict?",},
    {"dump", 4, (Blt_Op)DumpOp, 3, 0, "node ?switches?",},
    {"exists", 1, (Blt_Op)ExistsOp, 3, 4, "node ?key?",},
    {"find", 4, (Blt_Op)FindOp, 2, 0, "?switches?",},
    {"findchild", 5, (Blt_Op)FindChildOp, 4, 4, "node name",},
    {"firstchild", 3, (Blt_Op)FirstChildOp, 3, 3, "node",},
    {"fixed", 3, (Blt_Op)FixedOp, 3, 4, "node ?isFixed?",},
    {"foreach", 2, (Blt_Op)ForeachOp, 5, 5, "var node script",},
    {"get", 1, (Blt_Op)GetOp, 3, 5, "node ?key? ?defaultValue?",},
    {"incr", 4, (Blt_Op)IncrOp, 4, 5, "node key ?amount?",},
    {"incri", 5, (Blt_Op)IncriOp, 4, 5, "node key ?amount?",},
    {"index", 3, (Blt_Op)IndexOp, 3, 3, "name",},
    {"insert", 3, (Blt_Op)InsertOp, 3, 0, "parent ?switches?",},
    {"is", 2, (Blt_Op)IsOp, 2, 0, "oper args...",},
    {"ismodified", 3, (Blt_Op)IsModifiedOp, 2, 4, "?nodeOrTag? ?bool?",},
    {"isset", 3, (Blt_Op)IsSetOp, 4, 4, "nodeOrTag key",},
    {"keys", 1, (Blt_Op)KeysOp, 3, 0, "node ?node...?",},
    {"label", 3, (Blt_Op)LabelOp, 3, 4, "node ?newLabel?",},
    {"lappend", 7, (Blt_Op)LappendOp, 4, 0, "node key value ?...?",},
    {"lappendi", 8, (Blt_Op)LappendiOp, 4, 0, "node key value ?...?",},
    {"lastchild", 3, (Blt_Op)LastChildOp, 3, 3, "node",},
    {"modify", 2, (Blt_Op)ModifyOp, 3, 0, "node ?key value...?",},
    {"move", 2, (Blt_Op)MoveOp, 4, 0, "node newParent ?switches?",},
    {"names", 2, (Blt_Op)NamesOp, 3, 5, "node ?key? ?pattern?",},
    {"next", 4, (Blt_Op)NextOp, 3, 3, "node",},
    {"nextsibling", 5, (Blt_Op)NextSiblingOp, 3, 3, "node",},
    {"nodeseq", 3, (Blt_Op)NodeSeqOp, 2, 3, "?num?",},
    {"notify", 3, (Blt_Op)NotifyOp, 2, 0, "args...",},
    {"oldvalue", 2, (Blt_Op)OldValueOp, 2, 3, "?value?",},
    {"parent", 3, (Blt_Op)ParentOp, 3, 3, "node",},
    {"path", 3, (Blt_Op)PathOp, 3, 5, "node ?delim? ?prefix?",},
    {"position", 3, (Blt_Op)PositionOp, 3, 0, "?switches? node...",},
    {"previous", 5, (Blt_Op)PreviousOp, 3, 3, "node",},
    {"prevsibling", 5, (Blt_Op)PrevSiblingOp, 3, 3, "node",},
    {"restore", 3, (Blt_Op)RestoreOp, 5, 0, "node ?switches?",},
    {"root", 2, (Blt_Op)RootOp, 2, 3, "?node?",},
    {"set", 3, (Blt_Op)SetOp, 3, 0, "node ?key value...?",},
    {"size", 2, (Blt_Op)SizeOp, 3, 3, "node",},
    {"sort", 2, (Blt_Op)SortOp, 3, 0, "node ?flags...?",},
    {"sqlload", 4, (Blt_Op)SqlloadOp, 4, 0, "db sql",},
    {"sum", 3, (Blt_Op)SumOp, 4, 0, "node key ?-runtotal key? ?-start num? ?-int?",},
    {"supdate", 3, (Blt_Op)UpdatesOp, 3, 0, "node ?key value ...?",},
    {"tag", 2, (Blt_Op)TagOp, 3, 0, "args...",},
    {"trace", 2, (Blt_Op)TraceOp, 2, 0, "args...",},
    {"type", 2, (Blt_Op)TypeOp, 4, 4, "node key",},
    {"unset", 2, (Blt_Op)UnsetOp, 3, 0, "node ?key...?",},
    {"update", 2, (Blt_Op)UpdateOp, 3, 0, "node ?key value ...?",},
    {"values", 2, (Blt_Op)ValuesOp, 3, 5, "node ?key? ?withnames?",},
    {"vecdump", 4, (Blt_Op)VecdumpOp, 4, 5, "vector key ?tag?",},
    {"vecload", 4, (Blt_Op)VecloadOp, 4, 5, "vector key ?tag?",},
    {"with", 2, (Blt_Op)WithOp, 5, 0, "avar ?-keys keylist? ?-array key? ?-noupdate? ?-unset? ?-break? node script",},
};

static int nTreeOps = sizeof(treeOps) / sizeof(Blt_OpSpec);

static int
TreeInstObjCmd(
    ClientData clientData,	/* Information about the widget. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST *objv)	/* Vector of argument strings. */
{
    Blt_Op proc;
    TreeCmd *cmdPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, nTreeOps, treeOps, BLT_OP_ARG1, objc, objv,
 	BLT_OP_LINEAR_SEARCH);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(cmdPtr);
    result = (*proc) (cmdPtr, interp, objc, objv);
    Tcl_Release(cmdPtr);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * TreeInstDeleteProc --
 *
 *	Deletes the command associated with the tree.  This is
 *	called only when the command associated with the tree is
 *	destroyed.
 *
 * Results:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
static void
TreeInstDeleteProc(ClientData clientData)
{
    TreeCmd *cmdPtr = clientData;

    ReleaseTreeObject(cmdPtr);
    if (cmdPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(cmdPtr->tablePtr, cmdPtr->hashPtr);
    }
    Blt_DeleteHashTable(&(cmdPtr->traceTable));
    Blt_Free(cmdPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * GenerateName --
 *
 *	Generates an unique tree command name.  Tree names are in
 *	the form "treeN", where N is a non-negative integer. Check 
 *	each name generated to see if it is already a tree. We want
 *	to recycle names if possible.
 *	
 * Results:
 *	Returns the unique name.  The string itself is stored in the
 *	dynamic string passed into the routine.
 *
 * ----------------------------------------------------------------------
 */
static CONST char *
GenerateName(
    Tcl_Interp *interp,
    CONST char *prefix, 
    CONST char *suffix,
    Tcl_DString *resultPtr)
{

    int n;
    Tcl_Namespace *nsPtr;
    char string[200];
    Tcl_CmdInfo cmdInfo;
    Tcl_DString dString;
    CONST char *treeName, *name;

    /* 
     * Parse the command and put back so that it's in a consistent
     * format.  
     *
     *	t1         <current namespace>::t1
     *	n1::t1     <current namespace>::n1::t1
     *	::t1	   ::t1
     *  ::n1::t1   ::n1::t1
     */
    treeName = NULL;		/* Suppress compiler warning. */
    Tcl_DStringInit(&dString);
    for (n = 0; n < INT_MAX; n++) {
	Tcl_DStringSetLength(&dString, 0);
	Tcl_DStringAppend(&dString, prefix, -1);
	sprintf(string, "tree%d", n);
	Tcl_DStringAppend(&dString, string, -1);
	Tcl_DStringAppend(&dString, suffix, -1);
	treeName = Tcl_DStringValue(&dString);
	if (Blt_ParseQualifiedName(interp, treeName, &nsPtr, &name) != TCL_OK) {
	    Tcl_AppendResult(interp, "can't find namespace in \"", treeName, 
		"\"", (char *)NULL);
            Tcl_DStringFree(&dString);
	    return NULL;
	}
	if (nsPtr == NULL) {
	    nsPtr = Tcl_GetCurrentNamespace(interp);
	}
	treeName = Blt_GetQualifiedName(nsPtr, name, resultPtr);
	/* 
	 * Check if the command already exists. 
	 */
	if (Tcl_GetCommandInfo(interp, (char *)treeName, &cmdInfo)) {
	    continue;
	}
	if (!Blt_TreeExists(interp, treeName)) {
	    /* 
	     * We want the name of the tree command and the underlying
	     * tree object to be the same. Check that the free command
	     * name isn't an already a tree object name.  
	     */
	    break;
	}
    }
    Tcl_DStringFree(&dString);
    return treeName;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeCreateOp --
 *
 * TODO: add support for -nocommand by storing in interp has table.
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TreeCreateOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    TreeCmdInterpData *dataPtr = clientData;
    CONST char *treeName, *string;
    Tcl_DString dString;
    Blt_Tree token;
    int fixed = 0;
    int keyhash = 0;
    int dict = 0;

    while (objc>=3) {
        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') break;
        if (string[1] == 'k' && strcmp(string, "-keyhash") == 0) {
            if (objc<4) {
                Tcl_AppendResult(interp, "missing value for -keyhash", 0);
                return TCL_ERROR;
            }
            if (Tcl_GetIntFromObj(interp, objv[3], &keyhash)) {
                return TCL_ERROR;
            }
            objc -= 2;
            objv += 2;
        } else if (string[1] == 'f' && strcmp(string, "-fixed") == 0) {
            fixed = 1;
            objc--;
            objv++;
        } else if (string[1] == 'd' && strcmp(string, "-dictset") == 0) {
            dict = 1;
            objc--;
            objv++;
        } else {
            Tcl_AppendResult(interp, "option not one of: -keyhash -fixed -dictset", 0);
            return TCL_ERROR;
        }
    }
    if (objc != 2 && objc != 3) {
        Tcl_AppendResult(interp, "too many args", 0);
        return TCL_ERROR;
    }
    treeName = NULL;
    Tcl_DStringInit(&dString);

    if (objc == 3) {
	treeName = Tcl_GetString(objv[2]);
    }
    if (treeName == NULL) {
	treeName = GenerateName(interp, "", "", &dString);
    } else {
	char *p;

	p = strstr(treeName, "#auto");
	if (p != NULL) {
	    *p = '\0';
	    treeName = GenerateName(interp, treeName, p + 5, &dString);
	    *p = '#';
	} else {
	    CONST char *name;
	    Tcl_CmdInfo cmdInfo;
	    Tcl_Namespace *nsPtr;

	    nsPtr = NULL;
	    /* 
	     * Parse the command and put back so that it's in a consistent
	     * format.  
	     *
	     *	t1         <current namespace>::t1
	     *	n1::t1     <current namespace>::n1::t1
	     *	::t1	   ::t1
	     *  ::n1::t1   ::n1::t1
	     */
	    if (Blt_ParseQualifiedName(interp, treeName, &nsPtr, &name) 
		!= TCL_OK) {
		Tcl_AppendResult(interp, "can't find namespace in \"", treeName,
			 "\"", (char *)NULL);
                Tcl_DStringFree(&dString);
		return TCL_ERROR;
	    }
	    if (nsPtr == NULL) {
		nsPtr = Tcl_GetCurrentNamespace(interp);
	    }
	    treeName = Blt_GetQualifiedName(nsPtr, name, &dString);
	    /* 
	     * Check if the command already exists. 
	     */
	    if (Tcl_GetCommandInfo(interp, (char *)treeName, &cmdInfo)) {
		Tcl_AppendResult(interp, "a command \"", treeName,
				 "\" already exists", (char *)NULL);
		goto error;
	    }
	    if (Blt_TreeExists(interp, treeName)) {
		Tcl_AppendResult(interp, "a tree \"", treeName, 
			"\" already exists", (char *)NULL);
		goto error;
	    }
	} 
    } 
    if (treeName == NULL) {
	goto error;
    }
    if (Blt_TreeCreate(interp, treeName, &token) == TCL_OK) {
	int isNew;
	TreeCmd *cmdPtr;

	cmdPtr = Blt_Calloc(1, sizeof(TreeCmd));
	assert(cmdPtr);
	cmdPtr->dataPtr = dataPtr;
	cmdPtr->tree = token;
	cmdPtr->interp = interp;
	cmdPtr->tree->treeObject->maxKeyList = keyhash;
	if (fixed) {
	   cmdPtr->tree->treeObject->flags |= TREE_FIXED_KEYS;
	}
        if (dict) {
             cmdPtr->tree->treeObject->flags |= TREE_DICT_KEYS;
        }
        Blt_InitHashTable(&(cmdPtr->traceTable), BLT_STRING_KEYS);
	Blt_InitHashTable(&(cmdPtr->notifyTable), BLT_STRING_KEYS);
	cmdPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)treeName, 
		(Tcl_ObjCmdProc *)TreeInstObjCmd, cmdPtr, TreeInstDeleteProc);
	cmdPtr->tablePtr = &dataPtr->treeTable;
	cmdPtr->hashPtr = Blt_CreateHashEntry(cmdPtr->tablePtr, (char *)cmdPtr,
	      &isNew);
	Blt_SetHashValue(cmdPtr->hashPtr, cmdPtr);
	Tcl_SetResult(interp, (char *)treeName, TCL_VOLATILE);
	Tcl_DStringFree(&dString);
	Blt_TreeCreateEventHandler(cmdPtr->tree, TREE_NOTIFY_ALL, 
	     TreeEventProc, cmdPtr);
	return TCL_OK;
    }
 error:
    Tcl_DStringFree(&dString);
    return TCL_ERROR;
}

static void
destroyTreeCmd(char *cmdObj) {
    TreeCmd *cmdPtr = (TreeCmd*) cmdObj;
    Tcl_DeleteCommandFromToken(cmdPtr->interp, cmdPtr->cmdToken);
}

/*
 *----------------------------------------------------------------------
 *
 * TreeDestroyOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TreeDestroyOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    TreeCmdInterpData *dataPtr = clientData;
    TreeCmd *cmdPtr;
    char *string;
    register int i;

    for (i = 2; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	cmdPtr = GetTreeCmd(dataPtr, interp, string);
	if (cmdPtr == NULL) {
	    Tcl_AppendResult(interp, "can't find a tree named \"", string,
			     "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	cmdPtr->delete = 1;
	Tcl_EventuallyFree(cmdPtr, destroyTreeCmd);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TreeNamesOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TreeNamesOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    TreeCmdInterpData *dataPtr = clientData;
    TreeCmd *cmdPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *objPtr, *listObjPtr;
    Tcl_DString dString;
    char *qualName;

    Tcl_DStringInit(&dString);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->treeTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	cmdPtr = Blt_GetHashValue(hPtr);
	qualName = Blt_GetQualifiedName(
		Blt_GetCommandNamespace(interp, cmdPtr->cmdToken), 
		Tcl_GetCommandName(interp, cmdPtr->cmdToken), &dString);
	if (objc == 3) {
	    if (Tcl_StringMatch(qualName, Tcl_GetString(objv[2])) != 1) {
		continue;
	    }
	}
	objPtr = Tcl_NewStringObj(qualName, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Tcl_DStringFree(&dString);
    return TCL_OK;
}

static int
TreeOpOp(clientData, interp, objc, objv)
    ClientData clientData;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int i, result, sub = 0;
    Tcl_Obj **nobjv;
    char *string, *s;
    TreeCmd *cmdPtr;
    TreeCmdInterpData *dataPtr = clientData;

    s = Tcl_GetString(objv[2]);
    string = Tcl_GetString(objv[3]);
    if (objc>4 && (strncmp(s,"tr",2) == 0 || strcmp(s,"tag") == 0 || strcmp(s,"is") == 0 || strncmp(s,"no",2) == 0)) {
        string = Tcl_GetString(objv[4]);
        cmdPtr = GetTreeCmd(dataPtr, interp, string);
        sub = 1;
    } else {
        cmdPtr = GetTreeCmd(dataPtr, interp, string);
    }
    if (cmdPtr == NULL) {
        /* TODO: lookup TreeObject and build a temp CmdPtr */
        Tcl_AppendResult(interp, "can't find a tree named \"", string,
            "\"", (char *)NULL);
            return TCL_ERROR;
    }
    nobjv = (Tcl_Obj **) ckalloc((unsigned)(objc) * sizeof(Tcl_Obj *));

    if (sub) {
        nobjv[0] = objv[4];
        nobjv[1] = objv[2];
        nobjv[2] = objv[3];
    } else {
        nobjv[0] = objv[3];
        nobjv[1] = objv[2];
    }
    for (i = 2+sub;  i < (objc-2);  i++) {
        nobjv[i] = objv[i+2];
    }
    nobjv[objc-2] = 0;
    result = TreeInstObjCmd((ClientData)cmdPtr, interp, objc-2, nobjv);
    ckfree((char*) nobjv);
    return result;
}



/*
 *----------------------------------------------------------------------
 *
 * TreeObjCmd --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec treeCmdOps[] =
{
    {"create", 1, (Blt_Op)TreeCreateOp, 2, 0, "?-keyhash N? ?-fixed? ?-dictset? ?name?",},
    {"destroy", 1, (Blt_Op)TreeDestroyOp, 3, 0, "name...",},
    {"names", 1, (Blt_Op)TreeNamesOp, 2, 3, "?pattern?...",},
    {"op", 1, (Blt_Op)TreeOpOp, 4, 0, "subcmd tree args ...",},
};

static int nCmdOps = sizeof(treeCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
TreeObjCmd(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    Blt_Op proc;

    proc = Blt_GetOpFromObj(interp, nCmdOps, treeCmdOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 * -----------------------------------------------------------------------
 *
 * TreeInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the "tree" command
 *	is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the hash table managing all tree names.
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
TreeInterpDeleteProc(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    TreeCmdInterpData *dataPtr = clientData;

    /* All tree instances should already have been destroyed when
     * their respective Tcl commands were deleted. */
    Blt_DeleteHashTable(&dataPtr->treeTable);
    Tcl_DeleteAssocData(interp, TREE_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*ARGSUSED*/
static int
CompareDictionaryCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    int result;
    char *s1, *s2;

    s1 = Tcl_GetString(objv[1]);
    s2 = Tcl_GetString(objv[2]);
    result = Blt_DictionaryCompare(s1, s2);
    result = (result > 0) ? -1 : (result < 0) ? 1 : 0;
    Tcl_SetIntObj(Tcl_GetObjResult(interp), result);
    return TCL_OK;
}

#ifdef BLT_USE_UTIL_EXIT  
/*ARGSUSED*/
static int
ExitCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST *objv)
{
    int code;

    if (Tcl_GetIntFromObj(interp, objv[1], &code) != TCL_OK) {
	return TCL_ERROR;
    }
#ifdef TCL_THREADS
    Tcl_Exit(code);
#else 
    exit(code);
#endif
    /*NOTREACHED*/
    return TCL_OK;
}
#endif

/*
 * -----------------------------------------------------------------------
 *
 * Blt_TreeInit --
 *
 *	This procedure is invoked to initialize the "tree" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 * ------------------------------------------------------------------------
 */
extern int bltTreeUseLocalKeys;
int
Blt_TreeInit(Tcl_Interp *interp)
{
    TreeCmdInterpData *dataPtr;	/* Interpreter-specific data. */
    static Blt_ObjCmdSpec cmdSpec = { 
	"tree", TreeObjCmd, 
    };
    static Blt_ObjCmdSpec compareSpec = { 
	"compare", CompareDictionaryCmd, 
    };
#ifdef BLT_USE_UTIL_EXIT  
    static Blt_ObjCmdSpec exitSpec = { 
	"exit", ExitCmd, 
    };
    if (Blt_InitObjCmd(interp, "blt::util", &exitSpec) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Blt_InitObjCmd(interp, "blt::util", &compareSpec) == NULL) {
	return TCL_ERROR;
    }

    dataPtr = GetTreeCmdInterpData(interp);
    cmdSpec.clientData = dataPtr;
    if (Blt_InitObjCmd(interp, "blt", &cmdSpec) == NULL) {
	return TCL_ERROR;
    }
    if (!Tcl_IsSafe(interp)) {
        Tcl_LinkVar(interp, "blt::treeKeysLocal", (char*)&bltTreeUseLocalKeys, TCL_LINK_INT);
    }
    return TCL_OK;
}

int
Blt_TreeCmdGetToken(
    Tcl_Interp *interp,
    CONST char *string,
    Blt_Tree  *treePtr)
{
    TreeCmdInterpData *dataPtr;
    TreeCmd *cmdPtr;

    dataPtr = GetTreeCmdInterpData(interp);
    cmdPtr = GetTreeCmd(dataPtr, interp, string);
    if (cmdPtr == NULL) {
	Tcl_AppendResult(interp, "can't find a tree associated with \"",
		 string, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    *treePtr = cmdPtr->tree;
    return TCL_OK;
}

/* Dump tree to dbm */
/* Convert node data to datablock */

#endif /* NO_TREE */

