/*
 * bltTreeViewCmd.c --
 *
 *	This module implements an hierarchy widget for the BLT toolkit.
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
 *	The "treeview" widget was created by George A. Howlett.
 *      Extensive cleanups and enhancements by Peter MacDonald.
 */

/*
 * TODO:
 *
 * BUGS:
 *   1.  "open" operation should change scroll offset so that as many
 *	 new entries (up to half a screen) can be seen.
 *   2.  "open" needs to adjust the scrolloffset so that the same entry
 *	 is seen at the same place.
 */
#include "bltInt.h"

#ifndef NO_TREEVIEW

#include "bltTreeView.h"
#include "bltList.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define CLAMP(val,low,hi)	\
	(((val) < (low)) ? (low) : ((val) > (hi)) ? (hi) : (val))

static TreeViewCompareProc ExactCompare, GlobCompare, RegexpCompare, InlistCompare;
static TreeViewApplyProc ShowEntryApplyProc, HideEntryApplyProc, 
	MapAncestorsApplyProc, FixSelectionsApplyProc;
static Tk_LostSelProc LostSelection;
static int SelectEntryApplyProc( TreeView *tvPtr, TreeViewEntry *entryPtr, TreeViewColumn *columnPtr);
static int GetEntryFromObj2( TreeView *tvPtr, Tcl_Obj *objPtr, TreeViewEntry **entryPtrPtr);
static int TagDefine( TreeView *tvPtr, Tcl_Interp *interp, char *tagName);

extern Blt_CustomOption bltTreeViewIconsOption;
extern Blt_CustomOption bltTreeViewUidOption;
extern Blt_CustomOption bltTreeViewTreeOption;
extern Blt_CustomOption bltTreeViewStyleOption;

extern Blt_ConfigSpec bltTreeViewButtonSpecs[];
extern Blt_ConfigSpec bltTreeViewSpecs[];
extern Blt_ConfigSpec bltTreeViewEntrySpecs[];

#define TAG_UNKNOWN	 (1<<0)
#define TAG_RESERVED	 (1<<1)
#define TAG_USER_DEFINED (1<<2)

#define TAG_SINGLE	(1<<3)
#define TAG_MULTIPLE	(1<<4)
#define TAG_LIST	(1<<5)
#define TAG_ALL		(1<<6)
#define TAG_ROOTCHILDREN	(1<<7)

/*
 *----------------------------------------------------------------------
 *
 * SkipSeparators --
 *
 *	Moves the character pointer past one of more separators.
 *
 * Results:
 *	Returns the updates character pointer.
 *
 *----------------------------------------------------------------------
 */
static char *
SkipSeparators(path, separator, length)
    char *path, *separator;
    int length;
{
    while ((path[0] == separator[0]) && 
	   (strncmp(path, separator, length) == 0)) {
	path += length;
    }
    return path;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteNode --
 *
 *	Delete the node and its descendants.  Don't remove the root
 *	node, though.  If the root node is specified, simply remove
 *	all its children.
 *
 *---------------------------------------------------------------------- 
 */
static void
DeleteNode(tvPtr, node)
    TreeView *tvPtr;
    Blt_TreeNode node;
{
    Blt_TreeNode root;

    if (!Blt_TreeTagTableIsShared(tvPtr->tree)) {
	Blt_TreeClearTags(tvPtr->tree, node);
    }
    root = tvPtr->rootNode;
    if (node == root) {
	Blt_TreeNode next;
	/* Don't delete the root node. Simply clean out the tree. */
	for (node = Blt_TreeFirstChild(node); node != NULL; node = next) {
	    next = Blt_TreeNextSibling(node);
	    Blt_TreeDeleteNode(tvPtr->tree, node);
	}	    
    } else if (Blt_TreeIsAncestor(root, node)) {
	Blt_TreeDeleteNode(tvPtr->tree, node);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SplitPath --
 *
 *	Returns the trailing component of the given path.  Trailing
 *	separators are ignored.
 *
 * Results:
 *	Returns the string of the tail component.
 *
 *----------------------------------------------------------------------
 */
static int
SplitPath(tvPtr, path, depthPtr, compPtrPtr)
    TreeView *tvPtr;
    char *path;
    int *depthPtr;
    char ***compPtrPtr;
{
    int skipLen, pathLen;
    int depth, listSize;
    char **components;
    register char *p;
    char *sep;

    if (tvPtr->pathSep == SEPARATOR_LIST) {
	if (Tcl_SplitList(tvPtr->interp, path, depthPtr, compPtrPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    pathLen = strlen(path);

    skipLen = strlen(tvPtr->pathSep);
    path = SkipSeparators(path, tvPtr->pathSep, skipLen);
    depth = pathLen / skipLen;

    listSize = (depth + 1) * sizeof(char *);
    components = Blt_Malloc(listSize + (pathLen + 1));
    assert(components);
    p = (char *)components + listSize;
    strcpy(p, path);

    sep = strstr(p, tvPtr->pathSep);
    depth = 0;
    while ((*p != '\0') && (sep != NULL)) {
	*sep = '\0';
	components[depth++] = p;
	p = SkipSeparators(sep + skipLen, tvPtr->pathSep, skipLen);
	sep = strstr(p, tvPtr->pathSep);
    }
    if (*p != '\0') {
	components[depth++] = p;
    }
    components[depth] = NULL;
    *depthPtr = depth;
    *compPtrPtr = components;
    return TCL_OK;
}


static TreeViewEntry *
LastEntry(tvPtr, entryPtr, mask)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    unsigned int mask;
{
    Blt_TreeNode next;
    TreeViewEntry *nextPtr;

    next = Blt_TreeLastChild(entryPtr->node);
    while (next != NULL) {
	nextPtr = Blt_NodeToEntry(tvPtr, next);
	if ((nextPtr->flags & mask) != mask) {
             entryPtr = nextPtr;
             break;
	}
	entryPtr = nextPtr;
	next = Blt_TreeLastChild(next);
    }
    return entryPtr;
}

static TreeViewEntry *
LastNode(tvPtr, entryPtr, mask)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    unsigned int mask;
{
    TreeViewEntry *nextPtr;

    while ((nextPtr = LastEntry(tvPtr, entryPtr, mask)) != NULL &&
        nextPtr != entryPtr) {
	entryPtr = nextPtr;
	if ((nextPtr->flags & ENTRY_CLOSED) && entryPtr != tvPtr->rootPtr) {
	    break;
	}
    }
    return entryPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * ShowEntryApplyProc --
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShowEntryApplyProc(tvPtr, entryPtr)
    TreeView *tvPtr;		/* Not used. */
    TreeViewEntry *entryPtr;
{
    entryPtr->flags &= ~ENTRY_HIDDEN;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * HideEntryApplyProc --
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HideEntryApplyProc(tvPtr, entryPtr)
    TreeView *tvPtr;		/* Not used. */
    TreeViewEntry *entryPtr;
{
    entryPtr->flags |= ENTRY_HIDDEN;
    return TCL_OK;
}

static void
MapAncestors(tvPtr, entryPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
{
    while (entryPtr && entryPtr != tvPtr->rootPtr) {
	entryPtr = Blt_TreeViewParentEntry(entryPtr);
	if (entryPtr && entryPtr->flags & (ENTRY_CLOSED | ENTRY_HIDDEN)) {
	    tvPtr->flags |= TV_LAYOUT;
	    entryPtr->flags &= ~(ENTRY_CLOSED | ENTRY_HIDDEN);
	} 
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MapAncestorsApplyProc --
 *
 *	If a node in mapped, then all its ancestors must be mapped also.
 *	This routine traverses upwards and maps each unmapped ancestor.
 *	It's assumed that for any mapped ancestor, all it's ancestors
 *	will already be mapped too.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
static int
MapAncestorsApplyProc(tvPtr, entryPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
{
    /*
     * Make sure that all the ancestors of this entry are mapped too.
     */
    while (entryPtr != tvPtr->rootPtr) {
	entryPtr = Blt_TreeViewParentEntry(entryPtr);
	if ((entryPtr->flags & (ENTRY_HIDDEN | ENTRY_CLOSED)) == 0) {
	    break;		/* Assume ancestors are also mapped. */
	}
	entryPtr->flags &= ~(ENTRY_HIDDEN | ENTRY_CLOSED);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FindPath --
 *
 *	Finds the node designated by the given path.  Each path
 *	component is searched for as the tree is traversed.
 *
 *	A leading character string is trimmed off the path if it
 *	matches the one designated (see the -trimleft option).
 *
 *	If no separator is designated (see the -separator
 *	configuration option), the path is considered a Tcl list.
 *	Otherwise the each component of the path is separated by a
 *	character string.  Leading and trailing separators are
 *	ignored.  Multiple separators are treated as one.
 *
 * Results:
 *	Returns the pointer to the designated node.  If any component
 *	can't be found, NULL is returned.
 *
 *----------------------------------------------------------------------
 */
static TreeViewEntry *
FindPath(tvPtr, rootPtr, path)
    TreeView *tvPtr;
    TreeViewEntry *rootPtr;
    char *path;
{
    Blt_TreeNode child;
    char **compArr;
    char *name;
    int nComp;
    register char **p;
    TreeViewEntry *entryPtr;

    /* Trim off characters that we don't want */
    if (tvPtr->trimLeft != NULL) {
	register char *s1, *s2;
	
	/* Trim off leading character string if one exists. */
	for (s1 = path, s2 = tvPtr->trimLeft; *s2 != '\0'; s2++, s1++) {
	    if (*s1 != *s2) {
		break;
	    }
	}
	if (*s2 == '\0') {
	    path = s1;
	}
    }
    if (*path == '\0') {
	return rootPtr;
    }
    name = path;
    entryPtr = rootPtr;
    if (tvPtr->pathSep == SEPARATOR_NONE) {
        child = Blt_TreeFindChildRev(entryPtr->node, name, tvPtr->insertFirst);
	if (child == NULL) {
	    goto error;
	}
	return Blt_NodeToEntry(tvPtr, child);
    }

    if (SplitPath(tvPtr, path, &nComp, &compArr) != TCL_OK) {
	return NULL;
    }
    for (p = compArr; *p != NULL; p++) {
	name = *p;
	child = Blt_TreeFindChildRev(entryPtr->node, name, tvPtr->insertFirst);
	if (child == NULL) {
	    Blt_Free(compArr);
	    goto error;
	}
	entryPtr = Blt_NodeToEntry(tvPtr, child);
    }
    Blt_Free(compArr);
    return entryPtr;
 error:
    {
	Tcl_DString dString;

        Tcl_DStringInit(&dString);
	Blt_TreeViewGetFullName(tvPtr, entryPtr, FALSE, &dString);
	Tcl_AppendResult(tvPtr->interp, "can't find node \"", name,
		 "\" in parent node \"", Tcl_DStringValue(&dString), "\"", 
		(char *)NULL);
	Tcl_DStringFree(&dString);
    }
    return NULL;

}

/*
 *----------------------------------------------------------------------
 *
 * NodeToObj --
 *
 *	Converts a node pointer to a string representation.
 *	The string contains the node's index which is unique.
 *
 * Results:
 *	The string representation of the node is returned.  Note that
 *	the string is stored statically, so that callers must save the
 *	string before the next call to this routine overwrites the
 *	static array again.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj *
NodeToObj(node)
    Blt_TreeNode node;
{
    return Tcl_NewIntObj(Blt_TreeNodeId(node));
    /*char string[200];

    sprintf(string, "%d", Blt_TreeNodeId(node));
    return Tcl_NewStringObj(string, -1);*/
}


static int
GetEntryFromSpecialId(tvPtr, string, entryPtrPtr)
    TreeView *tvPtr;
    char *string;
    TreeViewEntry **entryPtrPtr;
{
    Blt_TreeNode node;
    TreeViewEntry *fromPtr, *entryPtr;
    char c;

    entryPtr = NULL;
    fromPtr = tvPtr->fromPtr;
    if (fromPtr == NULL) {
	fromPtr = tvPtr->focusPtr;
    } 
    if (fromPtr == NULL) {
	fromPtr = tvPtr->rootPtr;
    }
    c = string[0];
    if (c == '@') {
	int x, y;

	if (Blt_GetXY(tvPtr->interp, tvPtr->tkwin, string, &x, &y) == TCL_OK) {
	    entryPtr = Blt_TreeViewNearestEntry(tvPtr, x, y, TRUE);
	}
    } else if ((c == 'b') && (strcmp(string, "bottom") == 0)) {
	if (tvPtr->flatView) {
	    entryPtr = FLATIND(tvPtr, tvPtr->nEntries - 1);
	} else {
	    entryPtr = LastNode(tvPtr, tvPtr->rootPtr, ENTRY_MASK);
	}
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        entryPtr = LastNode(tvPtr, tvPtr->rootPtr, 0);
    } else if ((c == 't') && (strcmp(string, "tail") == 0)) {
        entryPtr = LastNode(tvPtr, tvPtr->rootPtr, 0);
    } else if ((c == 't') && (strcmp(string, "top") == 0)) {
	if (tvPtr->flatView) {
	    entryPtr = FLATIND(tvPtr,0);
	} else {
	    entryPtr = tvPtr->rootPtr;
	}
        if (entryPtr != NULL && entryPtr == tvPtr->rootPtr &&
            tvPtr->flags & TV_HIDE_ROOT) {
             entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK);
        }
     } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	entryPtr = LastEntry(tvPtr, tvPtr->rootPtr, ENTRY_MASK);
    } else if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
	entryPtr = tvPtr->selAnchorPtr;
     } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
         entryPtr = tvPtr->activePtr;
     } else if ((c == 'f') && (strcmp(string, "focus") == 0)) {
	entryPtr = tvPtr->focusPtr;
	if ((entryPtr == tvPtr->rootPtr) && (tvPtr->flags & TV_HIDE_ROOT)) {
	    entryPtr = Blt_TreeViewNextEntry(tvPtr->rootPtr, ENTRY_MASK);
	}
    } else if ((c == 'r') && (strcmp(string, "root") == 0)) {
	entryPtr = tvPtr->rootPtr;
    } else if ((c == 'p') && (strcmp(string, "parent") == 0)) {
	if (fromPtr != tvPtr->rootPtr) {
	    entryPtr = Blt_TreeViewParentEntry(fromPtr);
	}
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
	/* Can't trust picked item, if entries have been 
	 * added or deleted. */
	if (!(tvPtr->flags & TV_DIRTY)) {
	    ClientData context;

	    context = Blt_GetCurrentContext(tvPtr->bindTable);
	    
	    if ((context == ITEM_ENTRY) || 
		(context == ITEM_ENTRY_BUTTON) ||
		((uintptr_t)context >= (uintptr_t)ITEM_STYLE)) {
		entryPtr = Blt_GetCurrentItem(tvPtr->bindTable);
	    }
	}
    } else if ((c == 'u') && (strcmp(string, "up") == 0)) {
	entryPtr = fromPtr;
	if (tvPtr->flatView) {
	    int i;

	    i = entryPtr->flatIndex - 1;
	    if (i >= 0) {
		entryPtr = FLATIND( tvPtr, i);
	    }
	} else {
	    entryPtr = Blt_TreeViewPrevEntry(fromPtr, ENTRY_MASK);
	    if (entryPtr == NULL) {
		entryPtr = fromPtr;
	    }
	    if ((entryPtr == tvPtr->rootPtr) && 
		(tvPtr->flags & TV_HIDE_ROOT)) {
		entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK);
	    }
	}
    } else if ((c == 'd') && (strcmp(string, "down") == 0)) {
	entryPtr = fromPtr;
	if (tvPtr->flatView) {
	    int i;

	    i = entryPtr->flatIndex + 1;
	    if (i < tvPtr->nEntries) {
		entryPtr = FLATIND(tvPtr, i);
	    }
	} else {
	    entryPtr = Blt_TreeViewNextEntry(fromPtr, ENTRY_MASK);
	    if (entryPtr == NULL) {
		entryPtr = fromPtr;
	    }
	    if ((entryPtr == tvPtr->rootPtr) && 
		(tvPtr->flags & TV_HIDE_ROOT)) {
		entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK);
	    }
	}
    } else if ((c == 'p') && (strcmp(string, "prev") == 0)) {
	entryPtr = fromPtr;
	if (tvPtr->flatView) {
	    int i;

	    i = entryPtr->flatIndex - 1;
	    if (i < 0) {
		i = tvPtr->nEntries - 1;
	    }
	    entryPtr = FLATIND( tvPtr, i);
	} else {
	    entryPtr = Blt_TreeViewPrevEntry(fromPtr, ENTRY_MASK);
	    if (entryPtr == NULL) {
		entryPtr = LastEntry(tvPtr, tvPtr->rootPtr, ENTRY_MASK);
	    }
	    if ((entryPtr == tvPtr->rootPtr) && 
		(tvPtr->flags & TV_HIDE_ROOT)) {
		entryPtr = LastEntry(tvPtr, tvPtr->rootPtr, ENTRY_MASK);
		/*entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK); */
	    }
	}
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
	entryPtr = fromPtr;
	if (tvPtr->flatView) {
	    int i;

	    i = entryPtr->flatIndex + 1; 
	    if (i >= tvPtr->nEntries) {
		i = 0;
	    }
	    entryPtr = FLATIND(tvPtr, i);
	} else {
	    entryPtr = Blt_TreeViewNextEntry(fromPtr, ENTRY_MASK);
	    if (entryPtr == NULL) {
		if (tvPtr->flags & TV_HIDE_ROOT) {
		    entryPtr = Blt_TreeViewNextEntry(tvPtr->rootPtr,ENTRY_MASK);
		} else {
		    entryPtr = tvPtr->rootPtr;
		}
	    }
	}
    } else if ((c == 'n') && (strcmp(string, "nextsibling") == 0)) {
	node = Blt_TreeNextSibling(fromPtr->node);
	if (node != NULL) {
	    entryPtr = Blt_NodeToEntry(tvPtr, node);
	}
    } else if ((c == 'p') && (strcmp(string, "prevsibling") == 0)) {
	node = Blt_TreePrevSibling(fromPtr->node);
	if (node != NULL) {
	    entryPtr = Blt_NodeToEntry(tvPtr, node);
	}
    } else if ((c == 'v') && (strcmp(string, "view.top") == 0)) {
	if (tvPtr->nVisible > 0) {
	    entryPtr = tvPtr->visibleArr[0];
	}
    } else if ((c == 'v') && (strcmp(string, "view.bottom") == 0)) {
	if (tvPtr->nVisible > 0) {
	    entryPtr = tvPtr->visibleArr[tvPtr->nVisible - 1];
	} 
    } else {
	return TCL_ERROR;
    }
    *entryPtrPtr = entryPtr;
    return TCL_OK;
}

static int
GetTagInfo(tvPtr, tagName, infoPtr)
    TreeView *tvPtr;
    char *tagName;
    TreeViewTagInfo *infoPtr;
{
    static int cnt = 0;
    
    infoPtr->tagType = TAG_RESERVED | TAG_SINGLE;
    infoPtr->entryPtr = NULL;

    if (strcmp(tagName, "all") == 0) {
	infoPtr->entryPtr = tvPtr->rootPtr;
	infoPtr->tagType |= (TAG_ALL|TAG_MULTIPLE);

        infoPtr->node = infoPtr->entryPtr->node;
        infoPtr->inode = infoPtr->node->inode;
    } else if (strcmp(tagName, "nonroot") == 0) {
        infoPtr->entryPtr = Blt_TreeViewNextEntry(tvPtr->rootPtr, 0);
	infoPtr->tagType |= (TAG_ALL|TAG_MULTIPLE);
         if (infoPtr->entryPtr) {
             infoPtr->node = infoPtr->entryPtr->node;
             infoPtr->inode = infoPtr->node->inode;
         }
     } else if (strcmp(tagName, "rootchildren") == 0) {
         infoPtr->entryPtr = Blt_TreeViewNextEntry(tvPtr->rootPtr, 0);
         infoPtr->tagType |= (TAG_ROOTCHILDREN|TAG_MULTIPLE);
         if (infoPtr->entryPtr) {
             infoPtr->node = infoPtr->entryPtr->node;
             infoPtr->inode = infoPtr->node->inode;
         }
     } else {
        /* TODO: is this broken: are nodes not being found for tags.  FIXED???. */
	Blt_HashTable *tablePtr;

	tablePtr = Blt_TreeTagHashTable(tvPtr->tree, tagName);
	if (tablePtr != NULL) {
	    Blt_HashEntry *hPtr;
	    
	    infoPtr->tagType = TAG_USER_DEFINED; /* Empty tags are not
						  * an error. */
	    hPtr = Blt_FirstHashEntry(tablePtr, &infoPtr->cursor); 
	    if (hPtr != NULL) {
		Blt_TreeNode node;

		node = Blt_GetHashValue(hPtr);
		infoPtr->entryPtr = Blt_NodeToEntry(tvPtr, node);
                if (infoPtr->entryPtr == NULL && cnt++ == 0) {
                    /* fprintf(stderr, "Dangling node: %d\n", node->inode); */
                }
                if (infoPtr->entryPtr != NULL) {
                    infoPtr->node = infoPtr->entryPtr->node;
                    infoPtr->inode = infoPtr->node->inode;
                }
                if (Blt_TreeNodeDeleted(infoPtr->entryPtr->node)) {
                    return TCL_ERROR;
                }
                if (tablePtr->numEntries > 1) {
                    infoPtr->tagType |= TAG_MULTIPLE;
                }
	    }
	}  else {
	    infoPtr->tagType = TAG_UNKNOWN;
	    Tcl_AppendResult(tvPtr->interp, "can't find tag or id \"", tagName, 
		"\" in \"", Tk_PathName(tvPtr->tkwin), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

TreeViewEntry *
Blt_TreeViewFirstTaggedEntry(infoPtr)	
    TreeViewTagInfo *infoPtr;
{
    return infoPtr->entryPtr;
}

TreeViewEntry *
Blt_TreeViewNextTaggedEntry(infoPtr) 
    TreeViewTagInfo *infoPtr;
{
    TreeViewEntry *entryPtr;
    static int cnt = 0;
    Blt_TreeNode node;

    entryPtr = NULL;
    if (infoPtr->entryPtr != NULL) {
	TreeView *tvPtr = infoPtr->entryPtr->tvPtr;

         if (infoPtr->tagType == TAG_LIST) {
             int inode, i;
             i = ++infoPtr->idx;

             if (i >= infoPtr->objc) {
                 return NULL;
             }
             if (Tcl_GetIntFromObj(tvPtr->interp, infoPtr->objv[i], &inode) != TCL_OK) {
                 return NULL;
             }
             node = Blt_TreeGetNode(tvPtr->tree, inode);
             infoPtr->entryPtr = Blt_NodeToEntry(tvPtr, node);
             return infoPtr->entryPtr;
         }
         
         if (infoPtr->tagType & TAG_ALL) {
             if (Blt_TreeNodeDeleted(infoPtr->node) || (infoPtr->inode != infoPtr->node->inode)) {
                 return NULL;
             }
             entryPtr = Blt_TreeViewNextEntry(infoPtr->entryPtr, 0);
             if (entryPtr != NULL) {
                 infoPtr->node =entryPtr->node;
                 infoPtr->inode = infoPtr->node->inode;
             }
             
         } else if (infoPtr->tagType & TAG_ROOTCHILDREN) {
             if (Blt_TreeNodeDeleted(infoPtr->node) || (infoPtr->inode != infoPtr->node->inode)) {
                 return NULL;
             }
             entryPtr = Blt_TreeViewNextSibling(infoPtr->entryPtr, 0);
             if (entryPtr != NULL) {
                 infoPtr->node =entryPtr->node;
                 infoPtr->inode = infoPtr->node->inode;
             }
         } else if (infoPtr->tagType & TAG_MULTIPLE) {
	    Blt_HashEntry *hPtr;
	    if (infoPtr->tPtr && infoPtr->tPtr->refCount<=1) {
                /* Tag was deleted. */
                return NULL;
            }
	    hPtr = Blt_NextHashEntry(&infoPtr->cursor);
	    if (hPtr != NULL) {

		node = Blt_GetHashValue(hPtr);
		entryPtr = Blt_NodeToEntry(tvPtr, node);
		/*TODO: sometimes this fails. Fixed tag deleted problem. */
		if (entryPtr == NULL && cnt++ == 0) {
		    /*fprintf(stderr, "dangling node: %d\n", node->inode); */
		}
	    }
	} 
	infoPtr->entryPtr = entryPtr;
    }
    return entryPtr;
}

/*ARGSUSED*/
void
Blt_TreeViewGetTags(interp, tvPtr, entryPtr, list)
    Tcl_Interp *interp;		/* Not used. */
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    Blt_List list;
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Blt_TreeTagEntry *tPtr;

    for (hPtr = Blt_TreeFirstTag(tvPtr->tree, &cursor); hPtr != NULL; 
	hPtr = Blt_NextHashEntry(&cursor)) {
	tPtr = Blt_GetHashValue(hPtr);
	hPtr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)entryPtr->node);
	if (hPtr != NULL) {
	    Blt_ListAppend(list, Blt_TreeViewGetUid(tvPtr, tPtr->tagName),0);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AddTag --
 *
 *---------------------------------------------------------------------- 
 */
static int
AddTag(tvPtr, node, tagName)
    TreeView *tvPtr;
    Blt_TreeNode node;
    char *tagName;
{
    TreeViewEntry *entryPtr;

    if (strcmp(tagName, "root") == 0 || strcmp(tagName, "all") == 0 ||
        strcmp(tagName, "nonroot") == 0 || strcmp(tagName, "rootchildren") == 0) {
	Tcl_AppendResult(tvPtr->interp, "can't add reserved tag \"",
			 tagName, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (isdigit(UCHAR(tagName[0]))) {
	Tcl_AppendResult(tvPtr->interp, "invalid tag \"", tagName, 
		"\": can't start with digit", (char *)NULL);
	return TCL_ERROR;
    } 
    if (isdigit(UCHAR(tagName[0]))) {
	Tcl_AppendResult(tvPtr->interp, "invalid tag \"", tagName, 
		"\": can't start with digit", (char *)NULL);
	return TCL_ERROR;
    } 
    if (tagName[0] == '@') {
	Tcl_AppendResult(tvPtr->interp, "invalid tag \"", tagName, 
		"\": can't start with \"@\"", (char *)NULL);
	return TCL_ERROR;
    } 
    tvPtr->fromPtr = NULL;
    if (GetEntryFromSpecialId(tvPtr, tagName, &entryPtr) == TCL_OK) {
	Tcl_AppendResult(tvPtr->interp, "invalid tag \"", tagName, 
		"\": is a special id", (char *)NULL);
	return TCL_ERROR;
    }
    /* Add the tag to the node. */
    return Blt_TreeAddTag(tvPtr->tree, node, tagName);
}
    
int
Blt_TreeViewFindTaggedEntries(tvPtr, objPtr, infoPtr)	
    TreeView *tvPtr;
    Tcl_Obj *objPtr;
    TreeViewTagInfo *infoPtr;
{
    char *tagName;
    TreeViewEntry *entryPtr;

    memset(infoPtr, 0, sizeof(*infoPtr));
    infoPtr->init = 1;
    tagName = Tcl_GetString(objPtr); 
    infoPtr->tvPtr = tvPtr;
    tvPtr->fromPtr = NULL;
    if (tagName[0] == 0) {
        infoPtr->tagType = TAG_LIST;
        infoPtr->entryPtr = NULL;
        infoPtr->objc = 0;
        infoPtr->idx = 0;
        return TCL_OK;
    }
    if (strstr(tagName, "->") != NULL) {
        if (GetEntryFromObj2(tvPtr, objPtr, &infoPtr->entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        infoPtr->tagType = (TAG_RESERVED | TAG_SINGLE);
        return TCL_OK;
    }

    if (isdigit(UCHAR(tagName[0]))) {
	int inode;
        Blt_TreeNode node;
        char *cp = tagName;
        int i;

        while (isdigit(UCHAR(*cp)) && *cp != 0) {
            cp++;
        }
        if (*cp != 0) {
            if (Tcl_ListObjGetElements(tvPtr->interp, objPtr, &infoPtr->objc,
                &infoPtr->objv) != TCL_OK) {
                    return TCL_ERROR;
            }
            if (infoPtr->objc<1) {
                return TCL_ERROR;
            }
            for (i=infoPtr->objc-1; i>=0; i--) {
                if (Tcl_GetIntFromObj(tvPtr->interp, infoPtr->objv[i], &inode) != TCL_OK) {
                    return TCL_ERROR;
                }
                /* No use checking nodes here as they can later be deleted. */
                /*node = Blt_TreeGetNode(tvPtr->tree, inode);
                if (node == NULL) {
                    Tcl_AppendResult(interp, "unknown node: ", Blt_Itoa(inode), 0);
                    return TCL_ERROR;
                }*/
            }
            node = Blt_TreeGetNode(tvPtr->tree, inode);
            infoPtr->objPtr = objPtr;
            Tcl_IncrRefCount(objPtr);
            infoPtr->entryPtr = Blt_NodeToEntry(tvPtr, node);
            infoPtr->tagType = TAG_LIST;
            infoPtr->idx = 0;
            return TCL_OK;
        }

        if (Tcl_GetIntFromObj(tvPtr->interp, objPtr, &inode) != TCL_OK) {
	    return TCL_ERROR;
        }
        node = Blt_TreeGetNode(tvPtr->tree, inode);
        infoPtr->entryPtr = Blt_NodeToEntry(tvPtr, node);
	infoPtr->tagType = (TAG_RESERVED | TAG_SINGLE);
    } else if (GetEntryFromSpecialId(tvPtr, tagName, &entryPtr) == TCL_OK) {
	infoPtr->entryPtr = entryPtr;
	infoPtr->tagType = (TAG_RESERVED | TAG_SINGLE);
    } else {
	if (GetTagInfo(tvPtr, tagName, infoPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
        if ((infoPtr->tagType & TAG_USER_DEFINED)) {
            infoPtr->tPtr = Blt_TreeTagHashEntry(tvPtr->tree, tagName);
            Blt_TreeTagRefIncr(infoPtr->tPtr);
        }
    }
    return TCL_OK;
}


/* Cleanup and release resources. */
int
Blt_TreeViewDoneTaggedEntries(infoPtr)	
    TreeViewTagInfo *infoPtr;
{
    if (infoPtr->init == 1) {
        infoPtr->init = 0;
        if (infoPtr->objPtr != NULL) {
            Tcl_DecrRefCount(infoPtr->objPtr);
            infoPtr->objPtr = NULL;
        }
        if ((infoPtr->tagType & TAG_USER_DEFINED) && infoPtr->tPtr) {
            Blt_TreeTagRefDecr(infoPtr->tPtr);
            infoPtr->tPtr = NULL;
        }
    }
    return TCL_OK;
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
	} else if (isdigit(UCHAR(*p))) {
	    int inode;
	    
	    if (Tcl_GetInt(interp, p, &inode) != TCL_OK) {
		node = NULL;
	    } else {
		node = Blt_TreeGetNode(tree, inode);
	    }
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
 * GetEntryFromObj2 --
 *
 *	Converts a string into node pointer.  The string may be in one
 *	of the following forms:
 *
 *	    NNN			- inode.
 *	    "active"		- Currently active node.
 *	    "anchor"		- anchor of selected region.
 *	    "current"		- Currently picked node in bindtable.
 *	    "focus"		- The node currently with focus.
 *	    "root"		- Root node.
 *	    "end"		- Last open node in the entire hierarchy.
 *	    "next"		- Next open node from the currently active
 *				  node. Wraps around back to top.
 *	    "prev"		- Previous open node from the currently active
 *				  node. Wraps around back to bottom.
 *	    "up"		- Next open node from the currently active
 *				  node. Does not wrap around.
 *	    "down"		- Previous open node from the currently active
 *				  node. Does not wrap around.
 *	    "nextsibling"	- Next sibling of the current node.
 *	    "prevsibling"	- Previous sibling of the current node.
 *	    "parent"		- Parent of the current node.
 *	    "view.top"		- Top of viewport.
 *	    "view.bottom"	- Bottom of viewport.
 *	    @x,y		- Closest node to the specified X-Y position.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	The pointer to the node is returned via nodePtr.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *----------------------------------------------------------------------
 */
static int
GetEntryFromObj2(tvPtr, objPtr, entryPtrPtr)
    TreeView *tvPtr;
    Tcl_Obj *objPtr;
    TreeViewEntry **entryPtrPtr;
{
    Tcl_Interp *interp;
    char *string;
    TreeViewTagInfo info = {0};
    char *p;
    char save;
    int result;
    Blt_TreeNode node;

    interp = tvPtr->interp;

    string = Tcl_GetString(objPtr);
    *entryPtrPtr = NULL;
    p = strstr(string, "->");
    if (isdigit(UCHAR(string[0]))) {
	int inode;

         if (p != NULL) {

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
	node = Blt_TreeGetNode(tvPtr->tree, inode);
	if (node == NULL) {
            Tcl_AppendResult(interp, "unknown entry: ", string, 0);
            return TCL_ERROR;
        }
        if (p != NULL) {
            node = ParseModifiers(interp, tvPtr->tree, node, p);
            if (node == NULL) {
                Tcl_AppendResult(interp, "can't find tag or id:", string, 0);
                return TCL_ERROR;
            }
        }

        *entryPtrPtr = Blt_NodeToEntry(tvPtr, node);
	return TCL_OK;		/* Node Id. */
    }
    if (p == NULL) {
        if (GetEntryFromSpecialId(tvPtr, string, entryPtrPtr) == TCL_OK) {
            return TCL_OK;		/* Special Id. */
        }
        if (GetTagInfo(tvPtr, string, &info) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        save = *p;
        *p = '\0';
        info.tagType = 0;
        result = GetEntryFromSpecialId(tvPtr, string, &info.entryPtr);
        if (result != TCL_OK) {
            result = GetTagInfo(tvPtr, string, &info);
        }
        *p = save;
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
        if (info.tagType & TAG_MULTIPLE) {
            Tcl_AppendResult(interp, "more than one entry tagged as \"", string, 
                "\"", (char *)NULL);
                return TCL_ERROR;
        }
        node = ParseModifiers(interp, tvPtr->tree, info.entryPtr->node, p);
        if (node == NULL) {
            Tcl_AppendResult(interp, "can't find tag or id:", string, 0);
            return TCL_ERROR;
        }
        info.entryPtr = Blt_NodeToEntry(tvPtr, node);
    }
    if (info.tagType & TAG_MULTIPLE) {
	Tcl_AppendResult(interp, "more than one entry tagged as \"", string, 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    *entryPtrPtr = info.entryPtr;
    return TCL_OK;		/* Singleton tag. */
}

static int
GetEntryFromObj(tvPtr, objPtr, entryPtrPtr)
    TreeView *tvPtr;
    Tcl_Obj *objPtr;
    TreeViewEntry **entryPtrPtr;
{
    tvPtr->fromPtr = NULL;
    return GetEntryFromObj2(tvPtr, objPtr, entryPtrPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_TreeViewGetEntry --
 *
 *	Returns an entry based upon its index.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	The pointer to the node is returned via nodePtr.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *----------------------------------------------------------------------
 */
int
Blt_TreeViewGetEntry(tvPtr, objPtr, entryPtrPtr)
    TreeView *tvPtr;
    Tcl_Obj *objPtr;
    TreeViewEntry **entryPtrPtr;
{
    TreeViewEntry *entryPtr;

    if (GetEntryFromObj(tvPtr, objPtr, &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (entryPtr == NULL) {
	Tcl_ResetResult(tvPtr->interp);
	Tcl_AppendResult(tvPtr->interp, "can't find entry \"", 
		Tcl_GetString(objPtr), "\" in \"", Tk_PathName(tvPtr->tkwin), 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    *entryPtrPtr = entryPtr;
    return TCL_OK;
}

static Blt_TreeNode 
GetNthNode(parent, position)
    Blt_TreeNode parent;
    int position;
{
    Blt_TreeNode node;
    int count;

    count = 0;
    for(node = Blt_TreeFirstChild(parent); node != NULL; 
	node = Blt_TreeNextSibling(node)) {
	if (count++ == position) {
	    return node;
	}
    }
    return Blt_TreeLastChild(parent);
}

static TreeViewEntry *
GetNthEntry(
    TreeViewEntry *parentPtr,
    int position,
    unsigned int mask)
{
    TreeViewEntry *entryPtr;
    int count;

    count = 0;
    for(entryPtr = Blt_TreeViewFirstChild(parentPtr, mask); entryPtr != NULL; 
	entryPtr = Blt_TreeViewNextSibling(entryPtr, mask)) {
	if (count++ == position) {
	    return entryPtr;
	}
    }
    return Blt_TreeViewLastChild(parentPtr, mask);
}

/*
 * Preprocess the command string for percent substitution.
 */
void
Blt_TreeViewPercentSubst(tvPtr, entryPtr, columnPtr, command, value, resultPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    char *command;
    char *value;
    Tcl_DString *resultPtr;
{
    register char *last, *p;
    char *fullName;
    Tcl_DString dString;
    TreeViewValue *valuePtr;
    int one = (command[0] == '%' && strlen(command)==2);

    /*
     * Get the full path name of the node, in case we need to
     * substitute for it.
     */
    Tcl_DStringInit(&dString);
    fullName = Blt_TreeViewGetFullName(tvPtr, entryPtr, TRUE, &dString);
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
	    case 'W':		/* Widget name */
		string = Tk_PathName(tvPtr->tkwin);
		if (!one) {
                      Tcl_DStringAppendElement(resultPtr, string);
                      string = NULL;
                }
		break;
	    case 'F':		/* Formatted value of the data. */
		if (entryPtr && columnPtr &&
		((valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr))) &&
		valuePtr->textPtr) {
                      Tcl_DString dStr;
                      Tcl_DStringInit(&dStr);
                      Blt_TextLayoutValue( valuePtr->textPtr, &dStr);
                      if (!one) {
                          Tcl_DStringAppendElement(resultPtr, Tcl_DStringValue(&dStr));
                      } else {
                          Tcl_DStringAppend(resultPtr, Tcl_DStringValue(&dStr), -1);
                      }
                      Tcl_DStringFree(&dStr);
                      string = NULL;
                      break;
                  }
	    case 'V':		/* Value */
	        string = value;
                if (!one) {
                   Tcl_DStringAppendElement(resultPtr, value);
	           string = NULL;
	        }
		break;
	    case 'P':		/* Full pathname */
	        string = fullName;
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
		break;
	    case 'p':		/* Name of the node */
		string = (entryPtr?GETLABEL(entryPtr):"");
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
		break;
	    case '#':		/* Node identifier */
		string = (entryPtr?Blt_Itoa(Blt_TreeNodeId(entryPtr->node)):"-1");
		break;
	    case 'C':		/* Column key */
		string = (columnPtr?columnPtr->key:"??");
                if (!one) {
                    Tcl_DStringAppendElement(resultPtr, string);
                    string = NULL;
                }
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
 * SelectEntryApplyProc --
 *
 *	Sets the selection flag for a node.  The selection flag is
 *	set/cleared/toggled based upon the flag set in the treeview
 *	widget.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
static int
SelectEntryApplyProc(tvPtr, entryPtr, columnPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
{
    Blt_HashEntry *hPtr;

    switch (tvPtr->flags & TV_SELECT_MASK) {
    case TV_SELECT_CLEAR:
	Blt_TreeViewDeselectEntry(tvPtr, entryPtr, columnPtr);
	break;

    case TV_SELECT_SET:
	Blt_TreeViewSelectEntry(tvPtr, entryPtr, columnPtr);
	break;

    case TV_SELECT_TOGGLE:
        if ((tvPtr->selectMode & SELECT_MODE_CELLMASK) && columnPtr != NULL) {
            if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
                Blt_TreeViewDeselectEntry(tvPtr, entryPtr, columnPtr);
            } else {
                Blt_TreeViewSelectEntry(tvPtr, entryPtr, columnPtr);
            }
        } else {
            hPtr = Blt_FindHashEntry(&tvPtr->selectTable, (char *)entryPtr);
            if (hPtr != NULL) {
                Blt_TreeViewDeselectEntry(tvPtr, entryPtr, columnPtr);
            } else {
                Blt_TreeViewSelectEntry(tvPtr, entryPtr, columnPtr);
            }
        }
	break;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EventuallyInvokeSelectCmd --
 *
 *      Queues a request to execute the -selectcommand code associated
 *      with the widget at the next idle point.  Invoked whenever the
 *      selection changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Tcl code gets executed for some application-specific task.
 *
 *----------------------------------------------------------------------
 */
static void
EventuallyInvokeSelectCmd(tvPtr)
    TreeView *tvPtr;
{
    if (!(tvPtr->flags & TV_SELECT_PENDING)) {
	tvPtr->flags |= TV_SELECT_PENDING;
	Tcl_DoWhenIdle(Blt_TreeViewSelectCmdProc, tvPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_TreeViewPruneSelection --
 *
 *	The root entry being deleted or closed.  Deselect any of its
 *	descendants that are currently selected. 
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If any of the entry's descendants are deselected the widget
 *	is redrawn and the a selection command callback is invoked
 *	(if there's one configured).
 *
 *----------------------------------------------------------------------
 */
void
Blt_TreeViewPruneSelection(tvPtr, rootPtr)
    TreeView *tvPtr;
    TreeViewEntry *rootPtr;
{
    Blt_ChainLink *linkPtr, *nextPtr;
    TreeViewEntry *entryPtr;
    int selectionChanged;

    /* 
     * Check if any of the currently selected entries are a descendant
     * of of the current root entry.  Deselect the entry and indicate
     * that the treeview widget needs to be redrawn.
     */
    selectionChanged = FALSE;
    for (linkPtr = Blt_ChainFirstLink(tvPtr->selChainPtr); linkPtr != NULL; 
	 linkPtr = nextPtr) {
	nextPtr = Blt_ChainNextLink(linkPtr);
	entryPtr = Blt_ChainGetValue(linkPtr);
	if (Blt_TreeIsAncestor(rootPtr->node, entryPtr->node)) {
	    Blt_TreeViewDeselectEntry(tvPtr, entryPtr, NULL);
	    selectionChanged = TRUE;
	}
    }
    if (selectionChanged) {
	Blt_TreeViewEventuallyRedraw(tvPtr);
	if (tvPtr->selectCmd != NULL) {
	    EventuallyInvokeSelectCmd(tvPtr);
	}
    }
}


/*
 * --------------------------------------------------------------
 *
 * TreeView operations
 *
 * --------------------------------------------------------------
 */

/*ARGSUSED*/
static int
FocusOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    if (objc == 3) {
	TreeViewEntry *entryPtr;

	if (GetEntryFromObj(tvPtr, objv[2], &entryPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((entryPtr != NULL) && (entryPtr != tvPtr->focusPtr)) {
	    if (entryPtr->flags & ENTRY_HIDDEN) {
		/* Doesn't make sense to set focus to a node you can't see. */
		MapAncestors(tvPtr, entryPtr);
	    }
	    /* Changing focus can only affect the visible entries.  The
	     * entry layout stays the same. */
	    if (tvPtr->focusPtr != NULL) {
		tvPtr->focusPtr->flags |= ENTRY_REDRAW;
	    } 
	    entryPtr->flags |= ENTRY_REDRAW;
	    tvPtr->flags |= TV_SCROLL;
	    tvPtr->focusPtr = entryPtr;
	}
	Blt_TreeViewEventuallyRedraw(tvPtr);
    }
    Blt_SetFocusItem(tvPtr->bindTable, tvPtr->focusPtr, ITEM_ENTRY);
    if (tvPtr->focusPtr != NULL) {
	Tcl_SetObjResult(interp, NodeToObj(tvPtr->focusPtr->node));
    }
    return TCL_OK;
}

/*
 * .t entry isset entry col
 *
 */
/*ARGSUSED*/
static int
EntryIssetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    Tcl_Obj *objPtr;
    int rc;

    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) 
        != TCL_OK || columnPtr == NULL) {
        return TCL_ERROR;
    }
    if (columnPtr == &tvPtr->treeColumn) {
        Tcl_AppendResult(interp, "can not use tree column", 0);
        return TCL_ERROR;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK ||
        entryPtr == NULL) {
        return TCL_ERROR;
    }

    rc = (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree, entryPtr->node, 
        columnPtr->key, &objPtr) == TCL_OK);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
    return TCL_OK;
}


/*
 * .t entry isvisible entry
 *
 */
/*ARGSUSED*/
static int
EntryIsvisibleOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int rc;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK ||
        entryPtr == NULL) {
        return TCL_ERROR;
    }
    rc = Blt_TreeViewEntryIsHidden(entryPtr);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(!rc));
    return TCL_OK;
}

/*
 * .t entry ismapped entry
 *
 */
/*ARGSUSED*/
static int
EntryIsmappedOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int rc;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK ||
        entryPtr == NULL) {
        return TCL_ERROR;
    }
    rc = Blt_TreeViewEntryIsMapped(entryPtr);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
    return TCL_OK;
}

/*
 * .t entry unset ENTRY KEY
 *
 */
/*ARGSUSED*/
static int
EntryUnsetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    char *left;

    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_TreeViewGetColumnKey(interp, tvPtr, objv[4], &columnPtr, &left) 
        != TCL_OK || columnPtr == NULL) {
        return TCL_ERROR;
    }

    if (left == NULL) {
        if (Blt_TreeUnsetValueByKey(tvPtr->interp, tvPtr->tree, entryPtr->node, 
            columnPtr->key) != TCL_OK) {
            Tcl_ResetResult(interp);
            return TCL_OK;
        }
        Blt_TreeViewDeleteValue(entryPtr, columnPtr->key);
    } else {
        if (Blt_TreeUnsetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
            Tcl_GetString(objv[4])) != TCL_OK) {
            Tcl_ResetResult(interp);
            return TCL_OK;
        }
        Blt_TreeViewAddValue(entryPtr, columnPtr);
    }
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
    entryPtr->flags |= ENTRY_DIRTY;
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 * .t entry set ENTRY KEY ?VALUE?
 *
 */
/*ARGSUSED*/
static int
EntrySetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    int n, result;
    char *left, *string;

    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    if (Blt_TreeViewGetColumnKey(interp, tvPtr, objv[4], &columnPtr, &left) 
        != TCL_OK || columnPtr == NULL) {
        return TCL_ERROR;
    }

    if (objc == 5) {   
        Tcl_Obj *objPtr;
        if (Blt_TreeGetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
            string /*columnPtr->key*/, &objPtr) != TCL_OK) {
                Tcl_ResetResult(interp);
                return TCL_OK;
        }
        Tcl_SetObjResult(interp, objPtr);
        return TCL_OK;
    }
    
    if (objc % 2) {
        Tcl_AppendResult( interp, "odd number of arguments", 0);
        return TCL_ERROR;
    }

    Tcl_Preserve(entryPtr);
    result = TCL_OK;
    if (objc == 6) {
        result = Blt_TreeSetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
            string /*columnPtr->key*/, objv[5]);
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
            Tcl_Release(entryPtr);
            return TCL_ERROR;
        }
        if (result != TCL_OK) {
            Tcl_Release(entryPtr);
            return TCL_ERROR;
        }
        Blt_TreeViewAddValue(entryPtr, columnPtr);
        Tcl_SetObjResult(interp, objv[5]);
        Blt_TreeViewEventuallyRedraw(tvPtr);
        return TCL_OK;
    }
    
    n = 4;
    while (n<objc) {
        string = Tcl_GetString(objv[n]);
        result = Blt_TreeSetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
            string /*columnPtr->key*/, objv[n+1]);
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
            Tcl_Release(entryPtr);
            return TCL_ERROR;
        }
        if (result != TCL_OK) {
            break;
        }
        Blt_TreeViewAddValue(entryPtr, columnPtr);
        n += 2;
        if (n>=objc) break;
        if (Blt_TreeViewGetColumnKey(interp, tvPtr, objv[n], &columnPtr, &left) 
            != TCL_OK || columnPtr == NULL) {
            result = TCL_ERROR;
            break;
        }
    }
    Tcl_Release(entryPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return result;
}

/*
 * .t entry incr ENTRY KEY ?AMOUNT?
 *
 */
/*ARGSUSED*/
static int
EntryIncrOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    double dVal, dIncr = 1.0;
    int iVal, iIncr = 1, isInt = 0;
    Tcl_Obj *objPtr, *valueObjPtr;
    char *left, *string;

    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    if (Blt_TreeViewGetColumnKey(interp, tvPtr, objv[4], &columnPtr, &left) 
        != TCL_OK || columnPtr == NULL) {
        return TCL_ERROR;
    }
    
    if (Blt_TreeGetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
        string /* columnPtr->key */, &valueObjPtr) != TCL_OK) {
            return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(NULL, valueObjPtr, &iVal) == TCL_OK &&
         (objc <= 5 || Tcl_GetIntFromObj(NULL, objv[5], &iIncr) == TCL_OK)) {
        isInt = 1;
    } else {
        if (objc > 5 && Tcl_GetDoubleFromObj(interp, objv[5], &dIncr) != TCL_OK) {
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

    if (Blt_TreeSetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
        string /*columnPtr->key*/, objPtr) != TCL_OK) {
            return TCL_ERROR;
    }
    Blt_TreeViewAddValue(entryPtr, columnPtr);
    Tcl_SetObjResult(interp, objPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}


/*
 * .t entry get ENTRY ?KEY? ?DEFAULT?
 *
 */
/*ARGSUSED*/
static int
EntryGetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    char *key;
    Tcl_Obj *objPtr;

    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_Preserve(entryPtr);
    if (objc<=4) {
        Blt_ChainLink *linkPtr;
        TreeViewColumn *columnPtr;
        Tcl_Obj *listObjPtr;
        
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
            linkPtr = Blt_ChainNextLink(linkPtr)) {
            columnPtr = Blt_ChainGetValue(linkPtr);
            if (columnPtr->hidden) {
                continue;
            }
            if (Blt_TreeGetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
                columnPtr->key, &objPtr) != TCL_OK) {
                objPtr = Tcl_NewStringObj("", -1);
            }
            if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
                Tcl_Release(entryPtr);
                Tcl_DecrRefCount(listObjPtr);
                return TCL_ERROR;
            }
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
        Tcl_Release(entryPtr);
        return TCL_OK;
    }
    key = Tcl_GetString(objv[4]);
    Tcl_Release(entryPtr);
    if (Blt_TreeGetValue(tvPtr->interp, tvPtr->tree, entryPtr->node, 
        key, &objPtr) != TCL_OK) {
        if (objc != 6) {
            return TCL_ERROR;
        }
        Tcl_SetObjResult(interp, objv[5]);
        return TCL_OK;
    }
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 * .t entry value ENTRY ?KEY?
 *
 */
/*ARGSUSED*/
static int
EntryValueOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr = NULL;
    TreeViewValue *valuePtr;
    Blt_ChainLink *linkPtr;
    Tcl_Obj *objPtr, *listObjPtr;

    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc>4 && Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) 
        != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_Preserve(entryPtr);
    if (columnPtr != NULL) {
        valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
            Tcl_Release(entryPtr);
            return TCL_ERROR;
        }
        Tcl_Release(entryPtr);
        if (valuePtr && valuePtr->textPtr) {
            Tcl_DString dStr;
            Tcl_DStringInit(&dStr);
            Blt_TextLayoutValue( valuePtr->textPtr, &dStr);
            objPtr = Tcl_NewStringObj( Tcl_DStringValue(&dStr), -1);
            Tcl_DStringFree(&dStr);
            Tcl_SetObjResult(tvPtr->interp, objPtr);
            return TCL_OK;
        }
    
        if (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree, entryPtr->node, 
            columnPtr->key, &objPtr) != TCL_OK) {
            Tcl_ResetResult(interp);
        } else {
            Tcl_SetObjResult(interp, objPtr);
        }
        return TCL_OK;
    }
    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for(linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
	linkPtr = Blt_ChainNextLink(linkPtr)) {
	columnPtr = Blt_ChainGetValue(linkPtr);
	if (columnPtr->hidden) {
	    continue;
	}
        valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
             Tcl_Release(entryPtr);
             Tcl_DecrRefCount(listObjPtr);
             return TCL_ERROR;
         }
         if (valuePtr && valuePtr->textPtr) {
             Tcl_DString dStr;
             Tcl_DStringInit(&dStr);
             Blt_TextLayoutValue( valuePtr->textPtr, &dStr);
             objPtr = Tcl_NewStringObj( Tcl_DStringValue(&dStr), -1);
             Tcl_DStringFree(&dStr);
             Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
             continue;
         }
         if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
             Tcl_Release(entryPtr);
             Tcl_DecrRefCount(listObjPtr);
             return TCL_ERROR;
         }
         if (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree, entryPtr->node, 
             columnPtr->key, &objPtr) != TCL_OK) {
             objPtr = Tcl_NewStringObj("",0);
         }
         Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
     }
     Tcl_Release(entryPtr);
     Tcl_SetObjResult(interp, listObjPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * BboxOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BboxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    register int i;
    TreeViewEntry *entryPtr;
    int height, yBot;
    int left, top, right, bottom;
    int screen;
    int lWidth;
    char *string;

    if (tvPtr->flags & TV_LAYOUT) {
	/*
	 * The layout is dirty.  Recompute it now, before we use the
	 * world dimensions.  But remember, the "bbox" operation isn't
	 * valid for hidden entries (since they're not visible, they
	 * don't have world coordinates).
	 */
	Blt_TreeViewComputeLayout(tvPtr);
    }
    left = tvPtr->worldWidth;
    top = tvPtr->worldHeight;
    right = bottom = 0;

    screen = FALSE;
    string = Tcl_GetString(objv[2]);
    if ((string[0] == '-') && (strcmp(string, "-screen") == 0)) {
        screen = TRUE;
        objc--, objv++;
    }
    for (i = 2; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if ((string[0] == 'a') && (strcmp(string, "all") == 0)) {
	    left = top = 0;
	    right = tvPtr->worldWidth;
	    bottom = tvPtr->worldHeight;
	    break;
	}
	if (GetEntryFromObj(tvPtr, objv[i], &entryPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (entryPtr == NULL) {
	    continue;
	}
	if (entryPtr->flags & ENTRY_HIDDEN) {
	    continue;
	}
	yBot = entryPtr->worldY + entryPtr->height;
	height = VPORTHEIGHT(tvPtr);
	if ((yBot <= tvPtr->yOffset) &&
	    (entryPtr->worldY >= (tvPtr->yOffset + height))) {
	    continue;
	}
	if (bottom < yBot) {
	    bottom = yBot;
	}
	if (top > entryPtr->worldY) {
	    top = entryPtr->worldY;
	}
	lWidth = ICONWIDTH(DEPTH(tvPtr, entryPtr->node));
	if (right < (entryPtr->worldX + entryPtr->width + lWidth)) {
	    right = (entryPtr->worldX + entryPtr->width + lWidth);
	}
	if (left > entryPtr->worldX) {
	    left = entryPtr->worldX;
	}
    }

    if (screen) {
#if 0
        width = VPORTWIDTH(tvPtr);
	height = VPORTHEIGHT(tvPtr);
	/*
	 * Do a min-max text for the intersection of the viewport and
	 * the computed bounding box.  If there is no intersection, return
	 * the empty string.
	 */
          if (((right < tvPtr->xOffset) &&
	    (left >= (tvPtr->xOffset + width))) ||
	    ((bottom < tvPtr->yOffset) &&
	    (top >= (tvPtr->yOffset + height)))) {
	    return TCL_OK;
	}
	/* Otherwise clip the coordinates at the view port boundaries. */
	if (left < tvPtr->xOffset) {
	    left = tvPtr->xOffset;
	} else if (right > (tvPtr->xOffset + width)) {
	    right = tvPtr->xOffset + width;
	}
	if (top < tvPtr->yOffset) {
	    top = tvPtr->yOffset;
	} else if (bottom > (tvPtr->yOffset + height)) {
	    bottom = tvPtr->yOffset + height;
	}
#endif	
	left = SCREENX(tvPtr, left), top = SCREENY(tvPtr, top);
	right = SCREENX(tvPtr, right), bottom = SCREENY(tvPtr, bottom);
    }
    if ((left <= right) && (top <= bottom)) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(left));
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(top));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewIntObj(right - left));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewIntObj(bottom - top));
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

static void
DrawButton(tvPtr, entryPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
{
    Drawable drawable;
    int sx, sy, dx, dy;
    int width, height;
    int left, right, top, bottom;

    dx = SCREENX(tvPtr, entryPtr->worldX) + entryPtr->buttonX;
    dy = SCREENY(tvPtr, entryPtr->worldY) + entryPtr->buttonY;
    width = tvPtr->button.width;
    height = tvPtr->button.height;

    top = tvPtr->titleHeight + tvPtr->insetY;
    bottom = Tk_Height(tvPtr->tkwin) - tvPtr->insetY;
    left = tvPtr->insetX;
    right = Tk_Width(tvPtr->tkwin) - tvPtr->insetX;

    if (((dx + width) < left) || (dx > right) ||
	((dy + height) < top) || (dy > bottom)) {
	return;			/* Value is clipped. */
    }
    drawable = Tk_GetPixmap(tvPtr->display, Tk_WindowId(tvPtr->tkwin), 
	width, height, Tk_Depth(tvPtr->tkwin));
    /* Draw the background of the value. */
    Blt_TreeViewDrawButton(tvPtr, entryPtr, drawable, 0, 0);

    /* Clip the drawable if necessary */
    sx = sy = 0;
    if (dx < left) {
	width -= left - dx;
	sx += left - dx;
	dx = left;
    }
    if ((dx + width) >= right) {
	width -= (dx + width) - right;
    }
    if (dy < top) {
	height -= top - dy;
	sy += top - dy;
	dy = top;
    }
    if ((dy + height) >= bottom) {
	height -= (dy + height) - bottom;
    }
    XCopyArea(tvPtr->display, drawable, Tk_WindowId(tvPtr->tkwin), 
      tvPtr->lineGC, sx, sy, width,  height, dx, dy);
    Tk_FreePixmap(tvPtr->display, drawable);
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonActivateOp --
 *
 *	Selects the button to appear active.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonActivateOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *oldPtr, *newPtr;
    char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
	newPtr = NULL;
    } else if (GetEntryFromObj(tvPtr, objv[3], &newPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (tvPtr->treeColumn.hidden) {
	return TCL_OK;
    }
    if (tvPtr->button.reqSize==0) {
        return TCL_OK;
    }
    if ((newPtr != NULL) && !(newPtr->flags & ENTRY_HAS_BUTTON)) {
	newPtr = NULL;
    }
    oldPtr = tvPtr->activeButtonPtr;
    tvPtr->activeButtonPtr = newPtr;
    if (!(tvPtr->flags & TV_REDRAW) && (newPtr != oldPtr)) {
	if ((oldPtr != NULL) && (oldPtr != tvPtr->rootPtr)) {
	    DrawButton(tvPtr, oldPtr);
	}
	if ((newPtr != NULL) && (newPtr != tvPtr->rootPtr)) {
	    DrawButton(tvPtr, newPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonBindOp --
 *
 *	  .t bind tag sequence command
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonBindOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    ClientData object;
    char *string;

    string = Tcl_GetString(objv[3]);
    /* Assume that this is a binding tag. */
    object = Blt_TreeViewButtonTag(tvPtr, string);
    if (object == NULL) {
        return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, tvPtr->bindTable, object, 
	objc - 4, objv + 4);
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonCgetOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonCgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Blt_TreeViewOptsInit(tvPtr);
    return Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
	bltTreeViewButtonSpecs, (char *)tvPtr, objv[3], 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .h button configure option value
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for tvPtr; old resources get freed, if there
 *	were any.  The hypertext is redisplayed.
 *
 *----------------------------------------------------------------------
 */
static int
ButtonConfigureOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_TreeViewOptsInit(tvPtr);
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
	    bltTreeViewButtonSpecs, (char *)tvPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
		bltTreeViewButtonSpecs, (char *)tvPtr, objv[3], 0);
    }
    if (Blt_ConfigureWidgetFromObj(tvPtr->interp, tvPtr->tkwin, 
		bltTreeViewButtonSpecs, objc - 3, objv + 3, (char *)tvPtr, 
		BLT_CONFIG_OBJV_ONLY, NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    if (tvPtr->tile != NULL) {
        Blt_SetTileChangedProc(tvPtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    if (tvPtr->selectTile != NULL) {
        Blt_SetTileChangedProc(tvPtr->selectTile, Blt_TreeViewTileChangedProc, tvPtr);
    }

    Blt_TreeViewConfigureButtons(tvPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonOp --
 *
 *	This procedure handles button operations.
 *
 * Results:
 *	A standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
static Blt_OpSpec buttonOps[] =
{
    {"activate", 1, (Blt_Op)ButtonActivateOp, 4, 4, "tagOrId",},
    {"bind", 1, (Blt_Op)ButtonBindOp, 4, 6, "tagName ?sequence command?",},
    {"cget", 2, (Blt_Op)ButtonCgetOp, 4, 4, "option",},
    {"configure", 2, (Blt_Op)ButtonConfigureOp, 3, 0, "?option value?...",},
};

static int nButtonOps = sizeof(buttonOps) / sizeof(Blt_OpSpec);

static int
ButtonOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nButtonOps, buttonOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (tvPtr, interp, objc, objv);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * CgetOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Blt_TreeViewOptsInit(tvPtr);
    return Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, bltTreeViewSpecs,
	(char *)tvPtr, objv[2], 0);
}

static int
CloseTreeEntry(TreeView *tvPtr, TreeViewEntry *entryPtr) {
    if (Blt_TreeViewFirstChild(entryPtr, 0) != NULL &&
        entryPtr != tvPtr->rootPtr) {
        return Blt_TreeViewCloseEntry(tvPtr, entryPtr);
    }
    return TCL_OK;
}


/*ARGSUSED*/
static int
CloseOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewTagInfo info = {0};
    int recurse, trees, result;
    register int i;

    recurse = FALSE;
    trees = FALSE;

    while (objc > 2) {
	char *string;
	int length;

	string = Tcl_GetStringFromObj(objv[2], &length);
	if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-recurse", length) == 0)) {
	    objv++, objc--;
	    recurse = TRUE;
	} else if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-trees", length) == 0)) {
	    objv++, objc--;
	    trees = TRUE;
	} else break;
    }
    for (i = 2; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    /* 
	     * Clear the selections for any entries that may have become
	     * hidden by closing the node.  
	     */
	    Blt_TreeViewPruneSelection(tvPtr, entryPtr);
	    
	    /*
	     * -----------------------------------------------------------
	     *
	     *  Check if either the "focus" entry or selection anchor
	     *  is in this hierarchy.  Must move it or disable it before
	     *  we close the node.  Otherwise it may be deleted by a Tcl
	     *  "close" script, and we'll be left pointing to a bogus
	     *  memory location.
	     *
	     * -----------------------------------------------------------
	     */
	    if ((tvPtr->focusPtr != NULL) && 
		(Blt_TreeIsAncestor(entryPtr->node, tvPtr->focusPtr->node))) {
		tvPtr->focusPtr = entryPtr;
		Blt_SetFocusItem(tvPtr->bindTable, tvPtr->focusPtr, ITEM_ENTRY);
	    }
	    if ((tvPtr->selAnchorPtr != NULL) && 
		(Blt_TreeIsAncestor(entryPtr->node, 
				    tvPtr->selAnchorPtr->node))) {
		tvPtr->selMarkPtr = tvPtr->selAnchorPtr = NULL;
	    }
	    if ((tvPtr->activePtr != NULL) && 
		(Blt_TreeIsAncestor(entryPtr->node, tvPtr->activePtr->node))) {
		tvPtr->activePtr = entryPtr;
	    }
	    if (trees) {
		result = Blt_TreeViewApply(tvPtr, entryPtr, 
					   CloseTreeEntry, 0);
	    } else if (recurse) {
		result = Blt_TreeViewApply(tvPtr, entryPtr, 
					   Blt_TreeViewCloseEntry, 0);
	    } else {
		result = Blt_TreeViewCloseEntry(tvPtr, entryPtr);
	    }
	    if (result != TCL_OK) {
                 tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
                 Blt_TreeViewDoneTaggedEntries(&info);
                 return TCL_ERROR;
	    }	
	}
        Blt_TreeViewDoneTaggedEntries(&info);
     }
    /* Closing a node may affect the visible entries and the 
     * the world layout of the entries. */
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * 	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or reconfigure)
 *	the widget.
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for tvPtr; old resources get freed, if there
 *	were any.  The widget is redisplayed.
 *
 *----------------------------------------------------------------------
 */
static int
ConfigureOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_TreeViewOptsInit(tvPtr);
    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, bltTreeViewSpecs,
		(char *)tvPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
		bltTreeViewSpecs, (char *)tvPtr, objv[2], 0);
    } 
    if (Blt_ConfigureWidgetFromObj(interp, tvPtr->tkwin, bltTreeViewSpecs, 
	objc - 2, objv + 2, (char *)tvPtr, BLT_CONFIG_OBJV_ONLY, NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_TreeViewUpdateWidget(interp, tvPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (tvPtr->tile != NULL) {
        Blt_SetTileChangedProc(tvPtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    if (tvPtr->selectTile != NULL) {
        Blt_SetTileChangedProc(tvPtr->selectTile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
CurselectionOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;	/* Not used. */
{
#if 0
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    Tcl_Obj *listObjPtr, *objPtr;

    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    TreeViewValue *valuePtr;
    Blt_ChainLink *linkPtr;

    if (tvPtr->selectMode & SELECT_MODE_CELLMASK) {
        Tcl_AppendResult(interp, "-selectmode must not be 'cell' or 'multicell'", 0);
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    for (hPtr = Blt_FirstHashEntry(&tvPtr->selectTable, &cursor);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        entryPtr = (TreeViewEntry *)Blt_GetHashKey(&tvPtr->selectTable, hPtr);
        objPtr = NodeToObj(entryPtr->node);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }


    Tcl_SetObjResult(interp, listObjPtr);
    
    return TCL_OK;
#else
    TreeViewEntry *entryPtr;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (tvPtr->flags & TV_SELECT_SORTED) {
	Blt_ChainLink *linkPtr;

	for (linkPtr = Blt_ChainFirstLink(tvPtr->selChainPtr); linkPtr != NULL;
	     linkPtr = Blt_ChainNextLink(linkPtr)) {
	    entryPtr = Blt_ChainGetValue(linkPtr);
	    objPtr = NodeToObj(entryPtr->node);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
			
	}
    } else {
	for (entryPtr = tvPtr->rootPtr; entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK)) {
	    if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, NULL)) {
		objPtr = NodeToObj(entryPtr->node);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
#endif    
}

/*
 *----------------------------------------------------------------------
 *
 * BindOp --
 *
 *	  .t bind tagOrId sequence command
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    ClientData object;
    TreeViewEntry *entryPtr;
    char *string;

    /*
     * Entries are selected by id only.  All other strings are
     * interpreted as a binding tag.
     */
    object = NULL;
    string = Tcl_GetString(objv[2]);
    if (isdigit(UCHAR(string[0]))) {
	Blt_TreeNode node;
	int inode;

	if (Tcl_GetIntFromObj(tvPtr->interp, objv[2], &inode) != TCL_OK) {
	    return TCL_ERROR;
	}
	node = Blt_TreeGetNode(tvPtr->tree, inode);
	object = Blt_NodeToEntry(tvPtr, node);
    } else if (GetEntryFromSpecialId(tvPtr, string, &entryPtr) == TCL_OK) {
	if (entryPtr != NULL) {
	    return TCL_OK;	/* Special id doesn't currently exist. */
	}
	object = entryPtr;
    } else {
	/* Assume that this is a binding tag. */
	object = Blt_TreeViewEntryTag(tvPtr, string);
    }
    if (object == NULL) {
        Tcl_AppendResult(interp, "unknown object", string, 0);
        return TCL_ERROR;
    }
    return Blt_ConfigureBindingsFromObj(interp, tvPtr->bindTable, object, 
	 objc - 3, objv + 3);
}


/*ARGSUSED*/
static int
EditOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    char *string;
    int isRoot, isTest;
    int x, y;
    int rootX, rootY;

    Tk_GetRootCoords(tvPtr->tkwin, &rootX, &rootY);

    isRoot = isTest = FALSE;
    while (objc>2) {
        string = Tcl_GetString(objv[2]);
        if (strcmp("-root", string) == 0) {
            isRoot = TRUE;
            objv++, objc--;
        } else if (strcmp("-test", string) == 0) {
            isTest = TRUE;
            objv++, objc--;
        } else if (strcmp("-noscroll", string) == 0) {
            tvPtr->noScroll = 1;
            if (objc == 3) { return TCL_OK;}
            objv++, objc--;
        } else if (strcmp("-scroll", string) == 0) {
            tvPtr->noScroll = 0;
            if (objc == 3) { return TCL_OK;}
            objv++, objc--;
        } else {
            break;
        }
    }
    if (objc != 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
			" ?-root? x y\"", (char *)NULL);
	return TCL_ERROR;
			 
    }
    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (isRoot) {
	x -= rootX;
	y -= rootY;
    }
    entryPtr = Blt_TreeViewNearestEntry(tvPtr, x, y, FALSE);
    if (entryPtr != NULL) {
	Blt_ChainLink *linkPtr;
	TreeViewColumn *columnPtr;
	int worldX;

	worldX = WORLDX(tvPtr, x);
	for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
	     linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
	    columnPtr = Blt_ChainGetValue(linkPtr);
	    if (!columnPtr->editable) {
		continue;		/* Column isn't editable. */
	    }
	    if ((worldX >= columnPtr->worldX) && 
		(worldX < (columnPtr->worldX + columnPtr->width))) {
		TreeViewValue *valuePtr;
		
		valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
		if (valuePtr != NULL) {
		    TreeViewStyle *stylePtr;
		    int retVal = isTest;
		    
		    stylePtr = CHOOSE3(tvPtr->stylePtr,columnPtr->stylePtr, valuePtr->stylePtr);
		    if (stylePtr->classPtr->editProc != NULL) {
			if ((*stylePtr->classPtr->editProc)(tvPtr, entryPtr, 
				    valuePtr, stylePtr, x, y, &retVal) != TCL_OK) {
			    return TCL_ERROR;
			}
			Blt_TreeViewEventuallyRedraw(tvPtr);
		    }
		    Tcl_SetObjResult(interp, Tcl_NewIntObj(retVal));
		    return TCL_OK;
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryActivateOp --
 *
 *	Selects the entry to appear active.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryActivateOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *newPtr, *oldPtr;
    char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
	newPtr = NULL;
    } else if (GetEntryFromObj(tvPtr, objv[3], &newPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (tvPtr->treeColumn.hidden) {
	return TCL_OK;
    }
    oldPtr = tvPtr->activePtr;
    tvPtr->activePtr = newPtr;
    if (!(tvPtr->flags & TV_REDRAW) && (newPtr != oldPtr)) {
	Drawable drawable;
	int x, y;
	
	drawable = Tk_WindowId(tvPtr->tkwin);
	if (oldPtr != NULL) {
	    x = SCREENX(tvPtr, oldPtr->worldX);
	    if (!tvPtr->flatView) {
	        int hr, level;
	        level = DEPTH(tvPtr, oldPtr->node);
		x += ICONWIDTH(level);
                hr = ((tvPtr->flags & TV_HIDE_ROOT) ? 1 : 0);
                if (!(tvPtr->lineWidth>0 || tvPtr->button.reqSize>0 || level>hr)) {
                    x = 2;
                }
              }
	    y = SCREENY(tvPtr, oldPtr->worldY);
	    oldPtr->flags |= ENTRY_ICON;
	    Blt_TreeViewDrawIcon(tvPtr, oldPtr, drawable, x, y, 1);
	}
	if (newPtr != NULL) {
	    x = SCREENX(tvPtr, newPtr->worldX);
	    if (!tvPtr->flatView) {
	        int hr, level;
	        level = DEPTH(tvPtr, newPtr->node);
                 x += ICONWIDTH(level);
                hr = ((tvPtr->flags & TV_HIDE_ROOT) ? 1 : 0);
                if (!(tvPtr->lineWidth>0 || tvPtr->button.reqSize>0 || level>hr)) {
                    x = 2;
                }
	    }
	    y = SCREENY(tvPtr, newPtr->worldY);
	    newPtr->flags |= ENTRY_ICON;
	    Blt_TreeViewDrawIcon(tvPtr, newPtr, drawable, x, y, 1);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryCgetOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryCgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_TreeViewOptsInit(tvPtr);
    return Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
		bltTreeViewEntrySpecs, (char *)entryPtr, objv[4], 0);
}

/*
 *----------------------------------------------------------------------
 *
 * EntryConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .h entryconfigure node node node node option value
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for tvPtr; old resources get freed, if there
 *	were any.  The hypertext is redisplayed.
 *
 *----------------------------------------------------------------------
 */
static int
EntryConfigureOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int nIds, configObjc;
    Tcl_Obj *CONST *configObjv;
    register int i;
    TreeViewEntry *entryPtr;
    TreeViewTagInfo info = {0};
    char *string;

    /* Figure out where the option value pairs begin */
    for (i = 3; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
    }
    nIds = i-3;		  /* # of tags or ids specified */
    if (nIds<1) {
        Tcl_AppendResult(interp, "no ids specified", 0);
        return TCL_ERROR;
    }
    objc -= 3, objv += 3;
    configObjc = objc - nIds;	/* # of options specified */
    configObjv = objv + nIds;	/* Start of options in objv  */

    Blt_TreeViewOptsInit(tvPtr);
    for (i = 0; i < nIds; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    if (configObjc == 0) {
		return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
			bltTreeViewEntrySpecs, (char *)entryPtr, 
			(Tcl_Obj *)NULL, 0);
	    } else if (configObjc == 1) {
		return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
			bltTreeViewEntrySpecs, (char *)entryPtr, 
			configObjv[0], 0);
	    }
	    if (Blt_TreeViewConfigureEntry(tvPtr, entryPtr, configObjc, 
		configObjv, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
                Blt_TreeViewDoneTaggedEntries(&info);
		return TCL_ERROR;
	    }
	}
         Blt_TreeViewDoneTaggedEntries(&info);
     }
    tvPtr->flags |= (TV_DIRTY | TV_LAYOUT | TV_SCROLL | TV_RESORT|TV_DIRTYALL);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryIsOpenOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsBeforeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *e1Ptr, *e2Ptr;
    int bool;

    if ((Blt_TreeViewGetEntry(tvPtr, objv[3], &e1Ptr) != TCL_OK) ||
	(Blt_TreeViewGetEntry(tvPtr, objv[4], &e2Ptr) != TCL_OK)) {
	return TCL_ERROR;
    }
    bool = Blt_TreeIsBefore(e1Ptr->node, e2Ptr->node);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryIsHiddenOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsHiddenOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int bool;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = (entryPtr->flags & ENTRY_HIDDEN);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryIsLeafOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsLeafOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int bool;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = Blt_TreeViewIsLeaf(entryPtr);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * EntryIsOpenOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsOpenOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int bool;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = ((entryPtr->flags & ENTRY_CLOSED) == 0);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryParentOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    entryPtr = Blt_TreeViewParentEntry(entryPtr);
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryUpOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr, *fromPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    fromPtr = entryPtr;
    if (tvPtr->flatView) {
        int i;

        i = entryPtr->flatIndex - 1;
        if (i >= 0) {
            entryPtr = FLATIND(tvPtr,i);
        }
    } else {
        entryPtr = Blt_TreeViewPrevEntry(fromPtr, ENTRY_MASK);
        if (entryPtr == NULL) {
            entryPtr = fromPtr;
        }
        if ((entryPtr == tvPtr->rootPtr) && 
            (tvPtr->flags & TV_HIDE_ROOT)) {
            entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK);
        }
    }
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryDepthOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(DEPTH(tvPtr, entryPtr->node)));
    }
    return TCL_OK;
}


/*ARGSUSED*/
static int
EntryDownOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr, *fromPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    fromPtr = entryPtr;
    if (tvPtr->flatView) {
        int i;

        i = entryPtr->flatIndex + 1;
        if (i < tvPtr->nEntries) {
            entryPtr = FLATIND(tvPtr, i);
        }
    } else {
        entryPtr = Blt_TreeViewNextEntry(fromPtr, ENTRY_MASK);
        if (entryPtr == NULL) {
            entryPtr = fromPtr;
        }
        if ((entryPtr == tvPtr->rootPtr) && 
        (tvPtr->flags & TV_HIDE_ROOT)) {
            entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK);
        }
    }
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryExistsOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    
    int exists;
    
    if (objc==5 && Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    exists = (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) == TCL_OK);

    if (exists && objc == 5) {
        if (!Blt_TreeValueExists(tvPtr->tree, entryPtr->node, Tcl_GetString(objv[4]))) {
            exists = FALSE;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(exists));
    return TCL_OK;
}


/*ARGSUSED*/
static int
EntryPrevOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr, *fromPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    fromPtr = entryPtr;
    if (tvPtr->flatView) {
        int i;

        i = entryPtr->flatIndex - 1;
        if (i < 0) {
            i = tvPtr->nEntries - 1;
        }
        entryPtr = FLATIND(tvPtr, i);
    } else {
        entryPtr = Blt_TreeViewPrevEntry(fromPtr, ENTRY_MASK);
        if (entryPtr == NULL) {
            entryPtr = LastEntry(tvPtr, tvPtr->rootPtr, ENTRY_MASK);
        }
        if ((entryPtr == tvPtr->rootPtr) && 
            (tvPtr->flags & TV_HIDE_ROOT)) {
            /*entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK); */
            entryPtr = LastEntry(tvPtr, tvPtr->rootPtr, ENTRY_MASK);
        }
    }
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryRelabelOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    char *string;
    if ((tvPtr->flags & TV_ALLOW_DUPLICATES) == 0) {
        Tcl_AppendResult(interp, "must enable -allowduplicates to use relabel", 0);
        return TCL_ERROR;
    }

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    Blt_TreeRelabelNode(tvPtr->tree, entryPtr->node, string);
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntrySelectOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    Tcl_DString dStr;
    int rc;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((entryPtr != NULL) && (entryPtr != tvPtr->focusPtr)) {
        if (entryPtr->flags & ENTRY_HIDDEN) {
            /* Doesn't make sense to set focus to a node you can't see. */
            MapAncestors(tvPtr, entryPtr);
        }
        /* Changing focus can only affect the visible entries.  The
        * entry layout stays the same. */
        if (tvPtr->focusPtr != NULL) {
            tvPtr->focusPtr->flags |= ENTRY_REDRAW;
        } 
        entryPtr->flags |= ENTRY_REDRAW;
        tvPtr->flags |= TV_SCROLL;
        tvPtr->focusPtr = entryPtr;
    }
    Tcl_DStringInit(&dStr);
    Tcl_DStringAppend(&dStr, "::blt::tv::MoveFocus ", -1);
    Tcl_DStringAppend(&dStr, Tk_PathName(tvPtr->tkwin), -1);
    Tcl_DStringAppend(&dStr, " focus", -1);
    rc = Tcl_GlobalEval(interp, Tcl_DStringValue(&dStr));
    Tcl_DStringFree(&dStr);
    return rc;
}

/*ARGSUSED*/
static int
EntrySiblingOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr = NULL, *fromPtr;
    Blt_TreeNode node;
    int next = 1;

    if (objc>4) {
        const char *cp;
        cp = Tcl_GetString(objv[3]);
        if (!strcmp(cp, "-before")) {
            next = 0;
        } else {
            Tcl_AppendResult(interp, "expected \"-before\"", 0);
            return TCL_ERROR;
        }
        objc--;
        objv++;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &fromPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (next) {
        node = Blt_TreeNextSibling(fromPtr->node);
        if (node != NULL) {
            entryPtr = Blt_NodeToEntry(tvPtr, node);
        }
    } else {
        node = Blt_TreePrevSibling(fromPtr->node);
        if (node != NULL) {
            entryPtr = Blt_NodeToEntry(tvPtr, node);
        }
    }

    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryNextOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (tvPtr->flatView) {
        int i;

        i = entryPtr->flatIndex + 1; 
        if (i >= tvPtr->nEntries) {
            i = 0;
        }
        entryPtr = FLATIND(tvPtr, i);
    } else {
        entryPtr = Blt_TreeViewNextEntry(entryPtr, ENTRY_MASK);
        if (entryPtr == NULL) {
            if (tvPtr->flags & TV_HIDE_ROOT) {
                entryPtr = Blt_TreeViewNextEntry(tvPtr->rootPtr,ENTRY_MASK);
            } else {
                entryPtr = tvPtr->rootPtr;
            }
        }
    }
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
EntryChildrenOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *parentPtr;
    Tcl_Obj *listObjPtr, *objPtr;
    unsigned int mask;

    mask = 0;
    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &parentPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 4) {
	TreeViewEntry *entryPtr;

	for (entryPtr = Blt_TreeViewFirstChild(parentPtr, mask); 
	     entryPtr != NULL;
	     entryPtr = Blt_TreeViewNextSibling(entryPtr, mask)) {
	    objPtr = NodeToObj(entryPtr->node);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else if (objc == 6) {
	TreeViewEntry *entryPtr, *lastPtr, *firstPtr;
	int firstPos, lastPos;
	int nNodes;

	if ((Blt_GetPositionFromObj(interp, objv[4], &firstPos) != TCL_OK) ||
	    (Blt_GetPositionFromObj(interp, objv[5], &lastPos) != TCL_OK)) {
	    return TCL_ERROR;
	}
	nNodes = Blt_TreeNodeDegree(parentPtr->node);
	if (nNodes == 0) {
	    return TCL_OK;
	}
	if ((lastPos == END) || (lastPos >= nNodes)) {
	    lastPtr = Blt_TreeViewLastChild(parentPtr, mask);
	} else {
	    lastPtr = GetNthEntry(parentPtr, lastPos, mask);
	}
	if ((firstPos == END) || (firstPos >= nNodes)) {
	    firstPtr = Blt_TreeViewLastChild(parentPtr, mask);
	} else {
	    firstPtr = GetNthEntry(parentPtr, firstPos, mask);
	}
	if ((lastPos != END) && (firstPos > lastPos)) {
	    for (entryPtr = lastPtr; entryPtr != NULL; 
		entryPtr = Blt_TreeViewPrevEntry(entryPtr, mask)) {
		objPtr = NodeToObj(entryPtr->node);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		if (entryPtr == firstPtr) {
		    break;
		}
	    }
	} else {
	    for (entryPtr = firstPtr; entryPtr != NULL; 
		 entryPtr = Blt_TreeViewNextEntry(entryPtr, mask)) {
		objPtr = NodeToObj(entryPtr->node);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		if (entryPtr == lastPtr) {
		    break;
		}
	    }
	}
    } else {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " ",
			 Tcl_GetString(objv[1]), " ", 
			 Tcl_GetString(objv[2]), " tagOrId ?first last?", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * EntryDeleteOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryDeleteOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 5) {
	int entryPos;
	Blt_TreeNode node;
	/*
	 * Delete a single child node from a hierarchy specified 
	 * by its numeric position.
	 */
	if (Blt_GetPositionFromObj(interp, objv[3], &entryPos) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (entryPos >= (int)Blt_TreeNodeDegree(entryPtr->node)) {
	    return TCL_OK;	/* Bad first index */
	}
	if (entryPos == END) {
	    node = Blt_TreeLastChild(entryPtr->node);
	} else {
	    node = GetNthNode(entryPtr->node, entryPos);
	}
	DeleteNode(tvPtr, node);
    } else {
	int firstPos, lastPos;
	Blt_TreeNode node, first, last, next;
	int nEntries;
	/*
	 * Delete range of nodes in hierarchy specified by first/last
	 * positions.
	 */
	if ((Blt_GetPositionFromObj(interp, objv[4], &firstPos) != TCL_OK) ||
	    (Blt_GetPositionFromObj(interp, objv[5], &lastPos) != TCL_OK)) {
	    return TCL_ERROR;
	}
	nEntries = Blt_TreeNodeDegree(entryPtr->node);
	if (nEntries == 0) {
	    return TCL_OK;
	}
	if (firstPos == END) {
	    firstPos = nEntries - 1;
	}
	if (firstPos >= nEntries) {
	    Tcl_AppendResult(interp, "first position \"", 
		Tcl_GetString(objv[4]), " is out of range", (char *)NULL);
	    return TCL_ERROR;
	}
	if ((lastPos == END) || (lastPos >= nEntries)) {
	    lastPos = nEntries - 1;
	}
	if (firstPos > lastPos) {
	    Tcl_AppendResult(interp, "bad range: \"", Tcl_GetString(objv[4]), 
		" > ", Tcl_GetString(objv[5]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	first = GetNthNode(entryPtr->node, firstPos);
	last = GetNthNode(entryPtr->node, lastPos);
	for (node = first; node != NULL; node = next) {
	    next = Blt_TreeNextSibling(node);
	    DeleteNode(tvPtr, node);
	    if (node == last) {
		break;
	    }
	}
    }
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntrySizeOp --
 *
 *	Counts the number of entries at this node.
 *
 * Results:
 *	A standard Tcl result.  If an error occurred TCL_ERROR is
 *	returned and interp->result will contain an error message.
 *	Otherwise, TCL_OK is returned and interp->result contains
 *	the number of entries.
 *
 *----------------------------------------------------------------------
 */
static int
EntrySizeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int length, sum, recurse;
    char *string;

    recurse = FALSE;
    string = Tcl_GetStringFromObj(objv[3], &length);
    if ((string[0] == '-') && (length > 1) &&
	(strncmp(string, "-recurse", length) == 0)) {
	objv++, objc--;
	recurse = TRUE;
    }
    if (objc != 4) {
	Tcl_AppendResult(interp, "wrong or missing args", (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (recurse) {
	sum = Blt_TreeSize(entryPtr->node);
    } else {
	sum = Blt_TreeNodeDegree(entryPtr->node);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(sum));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EntryOp --
 *
 *	This procedure handles entry operations.
 *
 * Results:
 *	A standard Tcl result.
 *
 *----------------------------------------------------------------------
 */

static Blt_OpSpec entryOps[] =
{
    {"activate", 1, (Blt_Op)EntryActivateOp, 4, 4, "tagOrId",},
    /*bbox*/
    /*bind*/
    {"cget", 2, (Blt_Op)EntryCgetOp, 5, 5, "tagOrId option",},
    {"children", 2, (Blt_Op)EntryChildrenOp, 4, 6, 
	"tagOrId firstPos lastPos",},
    /*close*/
    {"configure", 2, (Blt_Op)EntryConfigureOp, 4, 0,
	"tagOrId ?tagOrId...? ?option value?...",},
    {"delete", 3, (Blt_Op)EntryDeleteOp, 5, 6, "tagOrId firstPos ?lastPos?",},
    {"depth",  3, (Blt_Op)EntryDepthOp, 4, 4, "tagOrId",},
    {"down",   2, (Blt_Op)EntryDownOp, 4, 4, "tagOrId",},
    {"exists", 1, (Blt_Op)EntryExistsOp, 4, 5, "tagOrId ?col?",},
    /*focus*/
    /*hide*/
    {"get", 2, (Blt_Op)EntryGetOp, 4, -1, "tagOrId ?key? ?default?",}, 
    {"incr", 3, (Blt_Op)EntryIncrOp, 5, 6, "tagOrId key ?amount?",}, 
    /*index*/
    {"isbefore", 3, (Blt_Op)EntryIsBeforeOp, 5, 5, "tagOrId tagOrId",},
    {"ishidden", 3, (Blt_Op)EntryIsHiddenOp, 4, 4, "tagOrId",},
    {"isleaf", 3, (Blt_Op)EntryIsLeafOp, 4, 4, "tagOrId",}, 
    {"ismapped", 3, (Blt_Op)EntryIsmappedOp, 4, 4, "tagOrId",}, 
    {"isopen", 3, (Blt_Op)EntryIsOpenOp, 4, 4, "tagOrId",},
    {"isset", 3, (Blt_Op)EntryIssetOp, 5, 5, "tagOrId col",}, 
    {"isvisible", 3, (Blt_Op)EntryIsvisibleOp, 4, 4, "tagOrId",}, 
    /*move*/
    /*nearest*/
    /*open*/
    {"next",    2, (Blt_Op)EntryNextOp, 4, 4, "tagOrId",},
    {"parent",  2, (Blt_Op)EntryParentOp, 4, 4, "tagOrId",},
    {"prev",    2, (Blt_Op)EntryPrevOp, 4, 4, "tagOrId",},
    {"relabel",  2, (Blt_Op)EntryRelabelOp, 5, 5, "tagOrId newLabel",},
    {"select",  2, (Blt_Op)EntrySelectOp, 4, 4, "tagOrId",},
    /*see*/
    {"set", 2, (Blt_Op)EntrySetOp, 5, -1, "tagOrId col ?value ...?",}, 
    /*show*/
    {"sibling", 3, (Blt_Op)EntrySiblingOp, 4, 5, "?-before? tagOrId",},
    {"size", 3, (Blt_Op)EntrySizeOp, 4, 5, "?-recurse? tagOrId",},
    {"unset", 2, (Blt_Op)EntryUnsetOp, 5, 5, "tagOrId col",}, 
    {"up",   1, (Blt_Op)EntryUpOp, 4, 4, "tagOrId",},
    {"value", 2, (Blt_Op)EntryValueOp, 4, 5, "tagOrId ?col?",}, 
    /*toggle*/
};
static int nEntryOps = sizeof(entryOps) / sizeof(Blt_OpSpec);

static int
EntryOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nEntryOps, entryOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (tvPtr, interp, objc, objv);
    return result;
}

/*ARGSUSED*/
static int
ExactCompare(interp, name, patternPtr, nocase)
    Tcl_Interp *interp;		/* Not used. */
    char *name;
    Tcl_Obj *patternPtr;
    int nocase;
{
    char *pattern = Tcl_GetString(patternPtr);
    if (!nocase) {
        return (strcmp(name, pattern) == 0);
    } else {
        return (strcasecmp(name, pattern) == 0);
    }
}

/*ARGSUSED*/
static int
GlobCompare(interp, name, patternPtr, nocase)
    Tcl_Interp *interp;		/* Not used. */
    char *name;
    Tcl_Obj *patternPtr;
    int nocase;
{
    char *pattern = Tcl_GetString(patternPtr);
    return Tcl_StringCaseMatch(name, pattern, nocase);
}

static int
RegexpCompare(interp, name, patternPtr, nocase)
    Tcl_Interp *interp;
    char *name;
    Tcl_Obj *patternPtr;
    int nocase;
{
    Tcl_DString dStr;
    int len, i, result;
    char *cp;
    
    Tcl_Obj *namePtr;
    if (!nocase) {
        namePtr = Tcl_NewStringObj(name, -1);
        result = Tcl_RegExpMatchObj(interp, namePtr, patternPtr);
    } else {
        len = strlen(name);
        Tcl_DStringInit(&dStr);
        Tcl_DStringSetLength(&dStr, len + 1);
        cp = Tcl_DStringValue(&dStr);
        for (i=0; i<len; i++) {
            cp[i] = tolower(name[i]);
        }
        cp[len] = 0;
        namePtr = Tcl_NewStringObj(cp, len);
        result = Tcl_RegExpMatchObj(interp, namePtr, patternPtr);
        Tcl_DStringFree(&dStr);
    }
    Tcl_DecrRefCount(namePtr);
    return result;
}

static int
InlistCompare(interp, name, patternPtr, nocase)
    Tcl_Interp *interp;		/* Not used. */
    char *name;
    Tcl_Obj *patternPtr;
    int nocase;
{
    Tcl_Obj **objv;
    int objc, i;
    char *pattern;
             
    if (Tcl_ListObjGetElements(interp, patternPtr, &objc, &objv) != TCL_OK) {
        return 1;
    }
    for (i = 0; i < objc; i++) {
        pattern = Tcl_GetString(objv[i]);
        if (!nocase) {
            if (strcmp(name, pattern) == 0) {
                return 1;
            }
        } else {
            if (strcasecmp(name, pattern) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FindOp --
 *
 *	Find one or more nodes based upon the pattern provided.
 *
 * Results:
 *	A standard Tcl result.  The interpreter result will contain a
 *	list of the node serial identifiers.
 *
 *----------------------------------------------------------------------
 */
static int
FindOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *firstPtr, *lastPtr, *afterPtr;
    TreeViewCompareProc *compareProc;
    TreeViewIterProc *nextProc;
    int invertMatch;		/* normal search mode (matching entries) */
    Tcl_Obj *namePattern/*, *fullPattern*/;
    char *string, *addTag, *withTag, *withoutTag, *curValue;
    register int i;
    Blt_List options;
    Blt_ListNode node;
    register TreeViewEntry *entryPtr;
    TreeViewEntry *topPtr = NULL;
    Tcl_Obj *listObjPtr, *objPtr, *command, *iObj;
    Tcl_Obj *vObj = NULL, *execObj = NULL;
    TreeViewColumn *columnPtr = NULL, *retColPtr = NULL;
    char *retPctPtr = NULL;
    Tcl_DString fullName, dStr, fStr;
    int nMatches, maxMatches, result, length, ocnt, useformat,userow, isvis;
    int depth, maxdepth, mindepth, mask, istree, isleaf, invis, ismap, uselabel;
    int isopen, isclosed, notop, nocase, isfull, cmdLen, docount, optInd, reldepth;
    int isnull, retLabel, isret;
    char *colStr, *keysub;
    Tcl_Obj *cmdObj, *cmdArgs;
    Tcl_Obj **aobjv;
    int aobjc;

    enum optInd {
        OP_ADDTAG, OP_AFTER, OP_CMDARGS,
        OP_COMMAND, OP_COLUMN, OP_COUNT, OP_DEPTH, OP_EXACT,
        OP_EXEC, OP_GLOB, OP_INLIST,
        OP_INVERT, OP_ISCLOSED, OP_ISNULL, OP_ISHIDDEN,
        OP_ISLEAF, OP_ISMAPPED, OP_ISOPEN, OP_ISTREE, OP_LIMIT,
        OP_MAXDEPTH, OP_MINDEPTH, OP_NAME, OP_NOCASE, OP_NOTOP,
        OP_OPTION, OP_REGEXP, OP_RELDEPTH, OP_RETURN, OP_TOP,
        OP_USEFORMAT, OP_USELABEL, OP_USEPATH, OP_USEROW,
        OP_VAR, OP_VISIBLE, OP_WITHTAG, OP_WITHOUTTAG
    };
    static char *optArr[] = {
        "-addtag",  "-after", "-cmdargs",
        "-command", "-column", "-count", "-depth", "-exact",
        "-exec", "-glob", "-inlist", "-invert", "-isclosed", "-isempty", "-ishidden",
        "-isleaf", "-ismapped", "-isopen", "-istree", "-limit",
        "-maxdepth", "-mindepth", "-name", "-nocase", "-notop",
        "-option", "-regexp", "-reldepth", "-return", "-top",
        "-useformat", "-uselabel", "-usepath", "-userow",
        "-var", "-visible", "-withtag", "-withouttag",
        0
    };
    isnull = retLabel = isret = 0;
    afterPtr = NULL;
    listObjPtr = NULL;
    invertMatch = FALSE;
    maxMatches = 0;
    namePattern = NULL;
    command = NULL;
    compareProc = ExactCompare;
    nextProc = Blt_TreeViewNextEntry;
    options = Blt_ListCreate(BLT_ONE_WORD_KEYS);
    withTag = withoutTag = addTag = NULL;
    mask = istree = isleaf = isopen = isclosed = docount = 0;
    depth = maxdepth = mindepth = -1;
    uselabel = 0, userow = 0;
    invis = 0, isvis = 0;
    ismap = 0;
    reldepth = notop = 0;
    nocase = 0, isfull = 0, useformat = 0;
    keysub = colStr = NULL;
    curValue = NULL;
    cmdObj = NULL;
    cmdArgs = NULL;

    entryPtr = tvPtr->rootPtr;
    Blt_TreeViewOptsInit(tvPtr);

    Tcl_DStringInit(&fullName);
    Tcl_DStringInit(&dStr);
    Tcl_DStringInit(&fStr);
    /*
     * Step 1:  Process flags for find operation.
     */
    for (i = 2; i < objc; i++) {
	string = Tcl_GetStringFromObj(objv[i], &length);
	if (string[0] != '-') {
	    break;
	}
        if (length == 2 && string[0] == '-' && string[0] == '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[i], optArr, "option",
            TCL_EXACT, &optInd) != TCL_OK) {
                return TCL_ERROR;
        }
        switch (optInd) {
        case OP_RELDEPTH:
	    reldepth = 1;
	    break;
        case OP_EXACT:
	    compareProc = ExactCompare;
	    break;
        case OP_COUNT:
	    docount = 1;
	    break;
        case OP_NOCASE:
	    nocase = 1;
	    break;
        case OP_NOTOP:
	    notop = 1;
	    break;
        case OP_USELABEL:
	    uselabel = 1;
	    break;
        case OP_USEFORMAT:
	    useformat = 1;
	    break;
        case OP_ISOPEN:
	    isopen = 1;
	    break;
        case OP_ISLEAF:
	    isleaf = 1;
	    break;
        case OP_ISTREE:
	    istree = 1;
	    break;
        case OP_USEPATH:
	    isfull = 1;
	    break;
        case OP_USEROW:
	    userow = 1;
	    break;
        case OP_ISHIDDEN:
	    invis = 1;
	    break;
        case OP_ISMAPPED:
	    ismap = 1;
	    break;
        case OP_ISNULL:
	    isnull = 1;
	    break;
        case OP_VISIBLE:
	    mask = ENTRY_MASK;
	    isvis = 1;
	    break;
        case OP_INLIST:
	    compareProc = InlistCompare;
	    break;
        case OP_REGEXP:
	    compareProc = RegexpCompare;
	    break;
        case OP_GLOB:
	    compareProc = GlobCompare;
	    break;
        case OP_INVERT:
	    invertMatch = TRUE;
	    break;
        case OP_ISCLOSED:
	    isclosed = 1;
	    break;
	    
	/* All the rest of these take an argument. */
        case OP_AFTER:
	    if (++i >= objc) { goto missingArg; }
            if (Blt_TreeViewGetEntry(tvPtr, objv[i], &afterPtr) != TCL_OK) {
                goto error;
            }
	    break;
        case OP_VAR:
	    if (++i >= objc) { goto missingArg; }
	    vObj = objv[i];
	    break;
        case OP_EXEC:
	    if (++i >= objc) { goto missingArg; }
	    execObj = objv[i];
	    break;
        case OP_TOP:
	    if (++i >= objc) { goto missingArg; }
            if (Blt_TreeViewGetEntry(tvPtr, objv[i], &topPtr) != TCL_OK) {
                goto error;
            }
	    break;
        case OP_RETURN:
            isret = 1;
	    if (++i >= objc) { goto missingArg; }
	    if (Blt_TreeViewGetColumn(NULL, tvPtr, objv[i], &retColPtr) 
                != TCL_OK) {
                if (strlen(Tcl_GetString(objv[i]))==0) {
                    retLabel = 1;
                } else if ('%' == *Tcl_GetString(objv[i])) {
                    retPctPtr = Tcl_GetString(objv[i]);
                } else {
                    Tcl_AppendResult(interp, "-return is not a column or a percent subst", 0);

                    goto error;
                }
            }
            if (retColPtr == &tvPtr->treeColumn) {
                retColPtr = NULL;
            }
	    break;
        case OP_COLUMN:
	    if (++i >= objc) { goto missingArg; }
	    if (Blt_TreeViewGetColumnKey(interp, tvPtr, objv[i], &columnPtr, &keysub) 
	       != TCL_OK) {
               goto error;
            }
            if (columnPtr == &tvPtr->treeColumn) {
                columnPtr = NULL;
            } else if (keysub) {
                colStr = Tcl_GetString(objv[i]);
            }
	    break;
        case OP_NAME:
	    if (++i >= objc) { goto missingArg; }
	    namePattern = objv[i];
	    break;
        case OP_LIMIT:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp, objv[i], &maxMatches) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (maxMatches < 0) {
		Tcl_AppendResult(interp, "bad match limit \"",
		    Tcl_GetString(objv[i]),
		    "\": should be a positive number", (char *)NULL);
                goto error;
	    }
	    break;
	    
        case OP_WITHOUTTAG:
	    if (++i >= objc) { goto missingArg; }
	    withoutTag = Tcl_GetString(objv[i]);
	    break;
	    
        case OP_WITHTAG:
	    if (++i >= objc) { goto missingArg; }
             withTag = Tcl_GetString(objv[i]);
             break;
             
        case OP_ADDTAG:
	    if (++i >= objc) { goto missingArg; }
             addTag = Tcl_GetString(objv[i]);
             if (TagDefine(tvPtr, interp, addTag) != TCL_OK) {
                 goto error;
             }
             break;
             
        case OP_CMDARGS:
            if (cmdArgs != NULL) {
                Tcl_AppendResult(interp, "duplicate -cmdargs", 0);
                goto error;
            }
            if (++i >= objc) { goto missingArg; }
            cmdArgs = objv[i];
            break;
        case OP_COMMAND:
            if (cmdObj != NULL) {
                Tcl_AppendResult(interp, "duplicate -command", 0);
                goto error;
            }
            if (++i >= objc) { goto missingArg; }
            cmdObj = objv[i];
	    break;
	    
        case OP_DEPTH:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp,objv[i],&depth) != TCL_OK) {
                goto error;
	    }
	    break;
        case OP_MAXDEPTH:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp,objv[i],&maxdepth) != TCL_OK) {
	        return TCL_OK;
                goto error;
	    }
	    break;
        case OP_MINDEPTH:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp,objv[i],&mindepth) != TCL_OK) {
                goto error;
	    }
	    break;
	    
	case OP_OPTION:
	    /*
	     * Verify that the switch is actually an entry configuration
	     * option.
	     */
	    if ((i + 2) >= objc) {
		goto missingArg;
	    }
	    i++;
	    if (Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
		  bltTreeViewEntrySpecs, (char *)entryPtr, objv[i], 0) 
		!= TCL_OK) {
		Tcl_ResetResult(interp);
                goto error;
	    }
	    /* Save the option in the list of configuration options */
	    node = Blt_ListGetNode(options, (char *)objv[i]);
	    if (node == NULL) {
		node = Blt_ListCreateNode(options, (char *)objv[i]);
		Blt_ListAppendNode(options, node);
	    }
	    i++;
	    Blt_ListSetValue(node, Tcl_GetString(objv[i]));
	}
    }
    if (isnull) {
        if (namePattern != NULL) {
            Tcl_AppendResult(interp, "can not use -isempty & -name", (char *)NULL);
            goto error;
        }
        if (columnPtr == NULL) {
            Tcl_AppendResult(interp, "must use -isempty with -column", (char *)NULL);
            goto error;
        }
        if (invertMatch) {
            Tcl_AppendResult(interp, "can not use -isempty & -invert", (char *)NULL);
            goto error;
        }
        if (isret) {
            Tcl_AppendResult(interp, "can not use -isempty & -return", (char *)NULL);
            goto error;
        }
        if (command) {
            Tcl_AppendResult(interp, "can not use -isempty & -command", (char *)NULL);
            goto error;
        }
    }
    if (docount && (retColPtr != NULL || retLabel)) {
        Tcl_AppendResult(interp, "can not use -count & -return", (char *)NULL);
        goto error;
    }
    if (cmdObj != NULL) {
        command = Tcl_DuplicateObj(cmdObj);
        Tcl_IncrRefCount(command);
        iObj = Tcl_NewIntObj(0);
        if (Tcl_ListObjAppendElement(interp, command, iObj) != TCL_OK) {
            Tcl_DecrRefCount(iObj);
            goto error;
        }
        Tcl_ListObjLength(interp, command, &cmdLen);
        aobjc = 0;
        if (cmdArgs != NULL) {
            int ai;
            if (Tcl_ListObjGetElements(interp, cmdArgs, &aobjc, &aobjv) != TCL_OK) {
                goto error;
            }
            for (ai = 0; ai < aobjc; ai++) {
                TreeViewColumn *sretColPtr;
                if (Blt_TreeViewGetColumn(interp, tvPtr, aobjv[ai], &sretColPtr) != TCL_OK) {
                    goto error;
                }
                
                iObj = Tcl_NewStringObj("",-1);
                if (Tcl_ListObjAppendElement(interp, command, iObj) != TCL_OK) {
                    Tcl_DecrRefCount(iObj);
                    goto error;
                }
            }
        }
    }
    
    if (columnPtr) {
        if (namePattern && (userow|isfull)) {
            Tcl_AppendResult(interp, "can not use -usepath|-userow & -column", (char *)NULL);
            goto error;
        }
       /* if (namePattern == NULL && execObj == NULL && command == NULL) {
            Tcl_AppendResult(interp, "-column must use -name/-exec/-command", (char *)NULL);
            goto error;
        }*/
        if (uselabel) {
            Tcl_AppendResult(interp, "can not use -uselabel & -column", (char *)NULL);
            goto error;
        }
    }
    if (vObj != NULL) {
        if (execObj == NULL) {
            Tcl_AppendResult(interp, "must use -exec with -var", (char *)NULL);
            goto error;
        }
    }
    if (namePattern && isfull &&  uselabel) {
        Tcl_AppendResult(interp, "can not use -uselabel & -usepath", (char *)NULL);
        goto error;
    }

    if ((objc - i) > 2) {
	Tcl_AppendResult(interp, "too many args", (char *)NULL);
        goto error;
    }
    /*
     * Step 2:  Find the range of the search.  Check the order of two
     *		nodes and arrange the search accordingly.
     *
     *	Note:	Be careful to treat "end" as the end of all nodes, instead
     *		of the end of visible nodes.  That way, we can search the
     *		entire tree, even if the last folder is closed.
     */
    if (topPtr != NULL) {
        firstPtr = topPtr;
    } else {
        firstPtr = tvPtr->rootPtr;	/* Default to root node */
    }
    if (reldepth) {
        if (depth>=0) {
            depth += firstPtr->node->depth;
        }
        if (mindepth>=0) {
            mindepth += firstPtr->node->depth;
        }
        if (maxdepth>=0) {
            maxdepth += firstPtr->node->depth;
        }
    }
    lastPtr = LastEntry(tvPtr, firstPtr, 0);
    if (afterPtr != NULL) {
        TreeViewEntry *nfPtr;
        nfPtr = (*nextProc)(afterPtr, mask);
        if (nfPtr != NULL) { firstPtr = nfPtr; }
    }

    if (i < objc) {
	string = Tcl_GetString(objv[i]);
	if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
	    firstPtr = LastEntry(tvPtr, tvPtr->rootPtr, 0);
	} else if (Blt_TreeViewGetEntry(tvPtr, objv[i], &firstPtr) != TCL_OK) {
            goto error;
	}
	i++;
    }
    if (i < objc) {
	string = Tcl_GetString(objv[i]);
	if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
	    lastPtr = LastEntry(tvPtr, tvPtr->rootPtr, 0);
	} else if (Blt_TreeViewGetEntry(tvPtr, objv[i], &lastPtr) != TCL_OK) {
            goto error;
	}
    }
    if (Blt_TreeIsBefore(lastPtr->node, firstPtr->node)) {
	nextProc = Blt_TreeViewPrevEntry;
    }
    if (notop || (mask && ((tvPtr->flags & TV_HIDE_ROOT)) && firstPtr == tvPtr->rootPtr)) {
        firstPtr = Blt_TreeViewNextEntry(firstPtr, mask);
    } else if (mask && (firstPtr->flags & mask)) {
        firstPtr = (*nextProc)(firstPtr, mask);
    }
    nMatches = 0;

    /*
     * Step 3:	Search through the tree and look for nodes that match the
     *		current pattern specifications.  Save the name of each of
     *		the matching nodes.
     */
    if (!docount) {
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    }
    for (entryPtr = firstPtr; entryPtr != NULL; 
	 entryPtr = (*nextProc) (entryPtr, mask)) {

        if (topPtr != NULL && entryPtr != topPtr) {
            if (DEPTH(tvPtr, entryPtr->node) <= DEPTH(tvPtr, topPtr->node)) {
                break;
            }
        }	
	if (invis && (!(entryPtr->flags & ENTRY_HIDDEN))) {
	       goto nextEntry;
	}
	
	if (ismap &&  !Blt_TreeViewEntryIsMapped(entryPtr)) {
           goto nextEntry;
	}
	
	if (istree &&  Blt_TreeViewIsLeaf(entryPtr)) {
            goto nextEntry;
	}     
	if (isleaf &&  !Blt_TreeViewIsLeaf(entryPtr)) {
            goto nextEntry;
	}     
	if (depth >=0 && entryPtr->node && entryPtr->node->depth != depth) {
            goto nextEntry;
        }
        if (maxdepth >=0 && entryPtr->node && entryPtr->node->depth > maxdepth) {
            goto nextEntry;
        }
        if (mindepth >=0 && entryPtr->node && entryPtr->node->depth < mindepth) {
            goto nextEntry;
        }
        if (isopen && ((entryPtr->flags & ENTRY_CLOSED) != 0)) {
            goto nextEntry;
        }
        if (isclosed && ((entryPtr->flags & ENTRY_CLOSED) == 0)) {
            goto nextEntry;
        }

        if (columnPtr) {
            TreeViewValue *valuePtr;
            if (useformat &&
            ((valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr))) &&
                valuePtr->textPtr) {
                Blt_TextLayoutValue( valuePtr->textPtr, &dStr);
                curValue = Tcl_DStringValue(&dStr);
                
            } else {
                if (colStr != NULL) {
                    if (Blt_TreeGetValue(NULL, tvPtr->tree, entryPtr->node, 
                        colStr, &objPtr) != TCL_OK) {
                        if (!isnull) {
                            goto nextEntry;
                        } else {
                            goto dochecks;
                        }
                    } else if (isnull) {
                        goto nextEntry;
                    }
                } else if (Blt_TreeGetValueByKey(NULL, tvPtr->tree, entryPtr->node, 
                    columnPtr->key, &objPtr) != TCL_OK) {
                    if (!isnull) {
                        goto nextEntry;
                    } else {
                        goto dochecks;
                    }
                } else if (isnull) {
                    goto nextEntry;
                }
                curValue = Tcl_GetString(objPtr);
            }
        } else if (useformat && entryPtr->textPtr != NULL) {
            Blt_TextLayoutValue( entryPtr->textPtr, &dStr);
            curValue = Tcl_DStringValue(&dStr);
        } else if (uselabel && entryPtr->labelUid != NULL) {
            curValue = entryPtr->labelUid;
        } else {
            curValue = Blt_TreeNodeLabel(entryPtr->node);
        }
	if (namePattern != NULL) {
	    if (userow) {
                Blt_ChainLink *linkPtr;
                TreeViewColumn *colPtr;
        
                Tcl_DStringSetLength(&fStr,0);
                for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr);
                    linkPtr != NULL;
                    linkPtr = Blt_ChainNextLink(linkPtr)) {
                    colPtr = Blt_ChainGetValue(linkPtr);
                    if (colPtr->hidden && (isvis)) continue;
                    if (colPtr == &tvPtr->treeColumn) {
                        Tcl_DStringAppend(&fStr, curValue, -1);
                        continue;
                    }
                    if (Blt_TreeGetValue(tvPtr->interp, tvPtr->tree,
                        entryPtr->node, colPtr->key, &objPtr) == TCL_OK) {
                        Tcl_DStringAppend(&fStr, " ", -1);
                        Tcl_DStringAppend(&fStr, Tcl_GetString(objPtr), -1);
                    }
                }
                 
                curValue = Tcl_DStringValue(&fStr);
                result = (*compareProc) (interp, curValue, namePattern, nocase);
            } else if (isfull == 0) {
                result = (*compareProc)(interp, curValue, namePattern, nocase);
            } else {
                curValue = Blt_TreeViewGetFullName(tvPtr, entryPtr, FALSE, &fullName);
                result = (*compareProc) (interp, curValue, namePattern, nocase);
            }
            if (result == invertMatch) {
                goto nextEntry;	/* Failed to match */
            }
        }
dochecks:
	if (withTag != NULL) {
	    result = Blt_TreeHasTag(tvPtr->tree, entryPtr->node, withTag);
	    if (!result) {
		goto nextEntry;	/* Failed to match */
	    }
	}
	if (withoutTag != NULL) {
	    result = Blt_TreeHasTag(tvPtr->tree, entryPtr->node, withoutTag);
	    if (result) {
		goto nextEntry;	/* Failed to match */
	    }
	}
        Blt_TreeViewOptsInit(tvPtr);
        ocnt = 0;
        for (node = Blt_ListFirstNode(options); node != NULL;
	    node = Blt_ListNextNode(node)) {
	    Tcl_Obj *kPtr;
	    ocnt++;
	    kPtr = (Tcl_Obj *)Blt_ListGetKey(node);
	    Tcl_ResetResult(interp);
	    Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
		bltTreeViewEntrySpecs, (char *)entryPtr, kPtr, 0);
            Blt_TreeViewOptsInit(tvPtr);
	    objPtr = Tcl_GetObjResult(interp);
	    result = (*compareProc) (interp, Tcl_GetString(objPtr), kPtr, nocase);
           if (result == invertMatch) {
               break;
           }
	}
        if (ocnt && result == invertMatch) {
            goto nextEntry;	/* Failed to match */
        }
	/* 
	 * Someone may actually delete the current node in the "exec"
	 * callback.  Preserve the entry.
	 */
	Tcl_Preserve(entryPtr);
	if (execObj != NULL) {
            if (vObj != NULL) {
                Tcl_Obj *intObj;
                intObj = Tcl_NewIntObj(Blt_TreeNodeId(entryPtr->node));
                Tcl_IncrRefCount(intObj);
                if (Tcl_ObjSetVar2(interp, vObj, NULL, intObj, 0) == NULL) {
                    Tcl_DecrRefCount(intObj);
                    goto error;
                }
                Tcl_DecrRefCount(intObj);
                result = Tcl_EvalObjEx(interp, execObj, 0);
            } else {
                Tcl_DString cmdString;

                Tcl_DStringFree(&cmdString);
                Blt_TreeViewPercentSubst(tvPtr, entryPtr, NULL, Tcl_GetString(execObj), curValue, &cmdString);
                result = Tcl_GlobalEval(interp, Tcl_DStringValue(&cmdString));
                Tcl_DStringFree(&cmdString);
            }
            if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
                Tcl_Release(entryPtr);
                goto error;
            }
            Blt_TreeViewOptsInit(tvPtr);
            if (result == TCL_CONTINUE) {
		Tcl_Release(entryPtr);
		goto nextEntry;
	    }
	    if (result != TCL_OK) {
		Tcl_Release(entryPtr);
		goto error;
	    }
            if (!docount) {
                Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_GetObjResult(interp));
            }
            goto finishnode;
	}
	if (command != NULL) {
             Tcl_Obj **cobjv;
             int cobjc, ai;
             
             iObj = Tcl_NewIntObj(Blt_TreeNodeId(entryPtr->node));
             if (Tcl_ListObjReplace(interp, command, cmdLen-1, 1, 1, &iObj) != TCL_OK) {
                 Tcl_Release(entryPtr);
                 goto error;
             }
             for (ai = 0; ai < aobjc; ai++) {
                 if (Blt_TreeGetValue(NULL, tvPtr->tree, entryPtr->node,
                    Tcl_GetString(aobjv[ai]), &iObj) != TCL_OK) {
                    iObj = Tcl_NewStringObj("", 0);
                 }
                 if (Tcl_ListObjReplace(interp, command, cmdLen+ai, 1, 1, &iObj) != TCL_OK) {
                     Tcl_Release(entryPtr);
                     goto error;
                 }
             }
             if (Tcl_ListObjGetElements(interp, command, &cobjc, &cobjv) != TCL_OK) {
                 Tcl_Release(entryPtr);
                 goto error;
             }
             result = Tcl_EvalObjv(interp, cobjc, cobjv, 0);
             if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
                  result = TCL_ERROR;
             } else if (result == TCL_RETURN) {
                 int eRes;
                 if (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp),
                     &eRes) != TCL_OK) {
                     result = TCL_ERROR;
                     goto error;
                 }
                 result = TCL_OK;
                 if (eRes != 0) {
                     if (!docount) {
                        objPtr = NodeToObj(entryPtr->node);
                        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                     }
                     goto finishnode;
                 } else {
                     goto nextEntry;
                 }
             }
             
             if (result != TCL_OK) {
                 Tcl_Release(entryPtr);
                 goto error;
             }
             goto finishnode;
         }
	/* A NULL node reference in an entry indicates that the entry
	 * was deleted, but its memory not released yet. */
	if (entryPtr->node != NULL) {
             if (retLabel) {
                 objPtr = Tcl_NewStringObj(Blt_TreeNodeLabel(entryPtr->node), -1);
             } else if (retColPtr != NULL) {
                 if (Blt_TreeViewGetData(entryPtr, retColPtr->key, &objPtr)
                 != TCL_OK) {
                     objPtr = Tcl_NewStringObj("", -1);
                 } 
             } else if (retPctPtr != NULL) {
                 Tcl_DString cmdString;
                 
                 Tcl_DStringInit(&cmdString);

                 Blt_TreeViewPercentSubst(tvPtr, entryPtr, columnPtr, retPctPtr, curValue, &cmdString);
                 objPtr = Tcl_NewStringObj(Tcl_DStringValue(&cmdString), -1);
                 Tcl_DStringFree(&cmdString);

             } else {
                /* Finally, save the matching node name. */
                objPtr = NodeToObj(entryPtr->node);
            }
            if (!docount) {
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
            if (addTag != NULL) {
                if (AddTag(tvPtr, entryPtr->node, addTag) != TCL_OK) {
                    Tcl_Release(entryPtr);
                    goto error;
                }
            }
	}
finishnode:
	Tcl_Release(entryPtr);
	nMatches++;
	if ((nMatches == maxMatches) && (maxMatches > 0)) {
	    break;
	}
      nextEntry:
	if (entryPtr == lastPtr) {
	    break;
	}
    }
    if (command != NULL) {
        Tcl_DecrRefCount(command);
    }
    Tcl_ResetResult(interp);
    Blt_ListDestroy(options);
    if (docount) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(nMatches));
    } else {
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Tcl_DStringFree(&fStr);
    Tcl_DStringFree(&dStr);
    Tcl_DStringFree(&fullName);
    return TCL_OK;

missingArg:
    if (command != NULL) {
        Tcl_DecrRefCount(command);
    }
    Tcl_AppendResult(interp, "missing argument for find option \"",
        Tcl_GetString(objv[i]), "\"", (char *)NULL);
error:
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    Tcl_DStringFree(&fStr);
    Tcl_DStringFree(&dStr);
    Tcl_DStringFree(&fullName);
    Blt_ListDestroy(options);
    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * GetOp --
 *
 *	Converts one or more node identifiers to its path component.
 *	The path may be either the single entry name or the full path
 *	of the entry.
 *
 * Results:
 *	A standard Tcl result.  The interpreter result will contain a
 *	list of the convert names.
 *
 *----------------------------------------------------------------------
 */
static int
GetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewTagInfo info = {0};
    TreeViewEntry *entryPtr;
    int useFullName;
    int useLabels = 0;
    register int i;
    Tcl_DString dString1, dString2;
    int count, single;
    char *string;

    useFullName = FALSE;
    while (objc > 2) {

	string = Tcl_GetString(objv[2]);
	if ((string[0] == '-') && (strcmp(string, "-full") == 0)) {
	    useFullName = TRUE;
	    objv++, objc--;
	} else if ((string[0] == '-') && (strcmp(string, "-labels") == 0)) {
	    useFullName = TRUE;
	    useLabels = TRUE;
	    objv++, objc--;
	} else {
	    break;
	}
    }
    Tcl_DStringInit(&dString1);
    Tcl_DStringInit(&dString2);
    single = 0;
    count = 0;
    for (i = 2; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
             Tcl_DStringFree(&dString1);
             Tcl_DStringFree(&dString2);
             return TCL_ERROR;
	}
        if (i==2 && objc<=3) {
            string = Tcl_GetString(objv[2]);
            single = (isdigit(UCHAR(string[0])) && strchr(string,' ')==NULL);
        } else {
            single = 0;
        }

	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    Tcl_DStringSetLength(&dString2, 0);
	    count++;
	    if (entryPtr->node == NULL) {
		Tcl_DStringAppendElement(&dString1, "");
		continue;
	    }
	    if (useFullName) {
		Blt_TreeViewGetFullName(tvPtr, entryPtr, useLabels, &dString2);
		Tcl_DStringAppendElement(&dString1, 
			 Tcl_DStringValue(&dString2));
	    } else {
	        if (single && count == 1) {
		    Tcl_DStringAppend(&dString2, 
			 Blt_TreeNodeLabel(entryPtr->node), -1);
		}
		Tcl_DStringAppendElement(&dString1, 
			 Blt_TreeNodeLabel(entryPtr->node));
	    }
	}
        Blt_TreeViewDoneTaggedEntries(&info);
     }
    /* This handles the single element list problem. */
    if (count == 1 && single) {
	Tcl_DStringResult(interp, &dString2);
	Tcl_DStringFree(&dString1);
    } else {
	Tcl_DStringResult(interp, &dString1);
	Tcl_DStringFree(&dString2);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SearchAndApplyToTree --
 *
 *	Searches through the current tree and applies a procedure
 *	to matching nodes.  The search specification is taken from
 *	the following command-line arguments:
 *
 *      ?-exact? ?-glob? ?-regexp? ?-invert?
 *      ?-data string?
 *      ?-name string?
 *      ?-path?
 *      ?-depth N?
 *      ?-mindepth N?
 *      ?-maxdepth N?
 *      ?--?
 *      ?inode...?
 *
 * Results:
 *	A standard Tcl result.  If the result is valid, and if the
 *      nonmatchPtr is specified, it returns a boolean value
 *      indicating whether or not the search was inverted.  This
 *      is needed to fix things properly for the "hide invert"
 *      case.
 *
 *----------------------------------------------------------------------
 */

static int
SearchAndApplyToTree(tvPtr, interp, objc, objv, proc, nonMatchPtr)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
    TreeViewApplyProc *proc;
    int *nonMatchPtr;		/* returns: inverted search indicator */
{
    TreeViewCompareProc *compareProc;
    int invertMatch;		/* normal search mode (matching entries) */
    Tcl_Obj *namePattern;
    register int i;
    int length, depth = -1, maxdepth = -1, mindepth = -1, noArgs, usefull = 0;
    int result, nocase, uselabel = 0, optInd, ocnt;
    char *curValue;
    Blt_List options;
    TreeViewEntry *entryPtr;
    register Blt_ListNode node;
    char *string, *colStr = NULL, *keysub = NULL;
    char *withTag, *withoutTag;
    Tcl_Obj *objPtr;
    TreeViewTagInfo info = {0};
    TreeViewColumn *columnPtr;
    Tcl_DString fullName;

    enum optInd {
        OP_COLUMN, OP_DEPTH, OP_EXACT, OP_GLOB, OP_INLIST, OP_INVERT,
        OP_MAXDEPTH, OP_MINDEPTH, OP_NAME, OP_NOCASE, OP_OPTION,
        OP_REGEXP, OP_USELABEL, OP_USEPATH,
        OP_WITHTAG, OP_WITHOUTTAG
    };
    static char *optArr[] = {
        "-column", "-depth", "-exact", "-glob", "-inlist", "-invert",
        "-maxdepth", "-mindepth", "-name", "-nocase", "-option",
        "-regexp", "-uselabel", "-usepath",
        "-withtag", "-withouttag",
        0
    };


    Tcl_DStringInit(&fullName);

    options = Blt_ListCreate(BLT_ONE_WORD_KEYS);
    invertMatch = FALSE;
    namePattern = NULL;
    compareProc = ExactCompare;
    withTag = NULL;
    withoutTag = NULL;
    columnPtr = NULL;
    nocase = 0;

    entryPtr = tvPtr->rootPtr;
    for (i = 2; i < objc; i++) {
	string = Tcl_GetStringFromObj(objv[i], &length);
	if (string[0] != '-') {
	    break;
	}
         if (length == 2 && string[0] == '-' && string[0] == '-') {
             break;
         }
         if (Tcl_GetIndexFromObj(interp, objv[i], optArr, "option",
            0, &optInd) != TCL_OK) {
                return TCL_ERROR;
        }
        switch (optInd) {
        case OP_EXACT:
	    compareProc = ExactCompare;
	    break;
        case OP_NOCASE:
	    nocase = 1;
	    break;
        case OP_USELABEL:
	    uselabel = 1;
	    break;
        case OP_USEPATH:
	    usefull = 1;
	    break;
        case OP_REGEXP:
	    compareProc = RegexpCompare;
	    break;
        case OP_INLIST:
	    compareProc = InlistCompare;
	    break;
        case OP_GLOB:
	    compareProc = GlobCompare;
	    break;
        case OP_INVERT:
	    invertMatch = TRUE;
	    break;
	    
	/* All the rest of these take arguments. */
        case OP_COLUMN:
	    if (++i >= objc) { goto missingArg; }
	    if (Blt_TreeViewGetColumnKey(interp, tvPtr, objv[i], &columnPtr, &keysub) 
	       != TCL_OK) {
               goto error;
            }
            if (columnPtr == &tvPtr->treeColumn) {
                columnPtr = NULL;
            } else if (keysub) {
                colStr = Tcl_GetString(objv[i]);
	    }
	    break;
	    
        case OP_NAME:
	    if (++i >= objc) { goto missingArg; }
	    namePattern = objv[i];
	    break;
	    
        case OP_WITHOUTTAG:
	    if (++i >= objc) { goto missingArg; }
	    withoutTag = Tcl_GetString(objv[i]);
	    break;
	    
        case OP_WITHTAG:
	    if (++i >= objc) { goto missingArg; }
             withTag = Tcl_GetString(objv[i]);
             break;
             
        case OP_DEPTH:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp,objv[i],&depth) != TCL_OK) {
                goto error;
	    }
	    break;
        case OP_MAXDEPTH:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp,objv[i],&maxdepth) != TCL_OK) {
	        return TCL_OK;
                goto error;
	    }
	    break;
        case OP_MINDEPTH:
	    if (++i >= objc) { goto missingArg; }
	    if (Tcl_GetIntFromObj(interp,objv[i],&mindepth) != TCL_OK) {
                goto error;
	    }
	    break;
	    
	case OP_OPTION:
	    /*
	     * Verify that the switch is actually an entry configuration
	     * option.
	     */
	    if ((i + 2) >= objc) {
		goto missingArg;
	    }
	    i++;
	    if (Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
		  bltTreeViewEntrySpecs, (char *)entryPtr, objv[i], 0) 
		!= TCL_OK) {
		Tcl_ResetResult(interp);
                goto error;
	    }
	    /* Save the option in the list of configuration options */
	    node = Blt_ListGetNode(options, (char *)objv[i]);
	    if (node == NULL) {
		node = Blt_ListCreateNode(options, (char *)objv[i]);
		Blt_ListAppendNode(options, node);
	    }
	    i++;
	    Blt_ListSetValue(node, Tcl_GetString(objv[i]));
	}
    }

    noArgs = (i >= objc);
    if (columnPtr) {
        if (!namePattern) {
            Tcl_AppendResult(interp, "must use -name with -column", (char *)NULL);
            goto error;
        }
        if (namePattern && usefull) {
            Tcl_AppendResult(interp, "can not use -usepath & -column", (char *)NULL);
            goto error;
        }
        if (uselabel) {
            Tcl_AppendResult(interp, "can not use -uselabel & -column", (char *)NULL);
            goto error;
        }
    }
    if (namePattern && usefull &&  uselabel) {
        Tcl_AppendResult(interp, "can not use -uselabel & -usepath", (char *)NULL);
        goto error;
    }

    for ( ;; i++) {
        if (noArgs) {
            if (i>objc) break;
            info.entryPtr = tvPtr->rootPtr;
            info.tagType = TAG_ALL;
            info.node = info.entryPtr->node;
        } else {
            if (i>=objc) break;
            if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
                goto error;
            }
        }
        for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
            entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {

            if (entryPtr == tvPtr->rootPtr && (info.tagType & TAG_ALL)) {
                entryPtr = Blt_TreeViewNextTaggedEntry(&info);
                if (entryPtr == NULL) break;
            }
	    if (depth >=0 && entryPtr->node && entryPtr->node->depth != depth) {
	        continue;
	    }
	    if (maxdepth >=0 && entryPtr->node && entryPtr->node->depth > maxdepth) {
	        continue;
	    }
	    if (mindepth >=0 && entryPtr->node && entryPtr->node->depth < mindepth) {
	        continue;
	    }
            if (columnPtr) {
                if (colStr) {
                    if (Blt_TreeGetValue(NULL, tvPtr->tree, entryPtr->node, colStr,
                    &objPtr) != TCL_OK)
                    continue;
                } else if (Blt_TreeGetValueByKey(NULL, tvPtr->tree, entryPtr->node, 
                    columnPtr->key, &objPtr) != TCL_OK) {
                    continue;
                }

                curValue = Tcl_GetString(objPtr);
            } else if (uselabel && entryPtr->labelUid != NULL) {
                curValue = entryPtr->labelUid;
            } else {
                curValue = Blt_TreeNodeLabel(entryPtr->node);
            }
            if (namePattern != NULL) {
                if (usefull == 0) {
                    result = (*compareProc) (interp, curValue, namePattern, nocase);
                    if (result == invertMatch) {
                        continue;	/* Failed to match */
                    }
                } else {
                    Blt_TreeViewGetFullName(tvPtr, entryPtr, FALSE, &fullName);
                    result = (*compareProc) (interp, Tcl_DStringValue(&fullName), 
                        namePattern, nocase);
                    if (result == invertMatch) {
                        continue;	/* Failed to match */
                    }
                }
	    }
	    if (withTag != NULL) {
		result = Blt_TreeHasTag(tvPtr->tree, entryPtr->node, withTag);
		if (!result) {
		    continue;	/* Failed to match */
		}
	    }
	    if (withoutTag != NULL) {
		result = Blt_TreeHasTag(tvPtr->tree, entryPtr->node, withoutTag);
		if (result) {
		    continue;	/* Failed to match */
		}
	    }
	    ocnt = 0;
	    for (node = Blt_ListFirstNode(options); node != NULL;
		node = Blt_ListNextNode(node)) {
		Tcl_Obj *kPtr;
		ocnt++;
		kPtr = (Tcl_Obj *)Blt_ListGetKey(node);
		Tcl_ResetResult(interp);
                Blt_TreeViewOptsInit(tvPtr);
                if (Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
			bltTreeViewEntrySpecs, (char *)entryPtr, kPtr, 0) 
		    != TCL_OK) {
		    goto error;	/* This shouldn't happen. */
		}
		objPtr = Tcl_GetObjResult(interp);
		result = (*compareProc)(interp, Tcl_GetString(objPtr), kPtr, nocase);
		if (result == invertMatch) {
		    break;	/* Failed to match */
		}
	    }
            /* if (result == invertMatch) {
                continue;
            }*/
            /* Finally, apply the procedure to the node */
	    (*proc) (tvPtr, entryPtr);
	}
        Blt_TreeViewDoneTaggedEntries(&info);
	Tcl_ResetResult(interp);
    }
    if (nonMatchPtr != NULL) {
	*nonMatchPtr = invertMatch;	/* return "inverted search" status */
    }
    Blt_ListDestroy(options);
    Tcl_DStringFree(&fullName);
    return TCL_OK;

missingArg:
    Tcl_AppendResult(interp, "missing pattern for search option \"",
        Tcl_GetString(objv[i]), "\"", (char *)NULL);
error:
    Blt_TreeViewDoneTaggedEntries(&info);
    Tcl_DStringFree(&fullName);
    Blt_ListDestroy(options);
    return TCL_ERROR;

}

static int
FixSelectionsApplyProc(tvPtr, entryPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
{
    if (entryPtr->flags & ENTRY_HIDDEN) {
	Blt_TreeViewDeselectEntry(tvPtr, entryPtr, NULL);
	if ((tvPtr->focusPtr != NULL) &&
	    (Blt_TreeIsAncestor(entryPtr->node, tvPtr->focusPtr->node))) {
	    if (entryPtr != tvPtr->rootPtr) {
		entryPtr = Blt_TreeViewParentEntry(entryPtr);
		tvPtr->focusPtr = (entryPtr == NULL) 
		    ? tvPtr->focusPtr : entryPtr;
		Blt_SetFocusItem(tvPtr->bindTable, tvPtr->focusPtr, ITEM_ENTRY);
	    }
	}
	if ((tvPtr->selAnchorPtr != NULL) &&
	    (Blt_TreeIsAncestor(entryPtr->node, tvPtr->selAnchorPtr->node))) {
	    tvPtr->selMarkPtr = tvPtr->selAnchorPtr = NULL;
	}
	if ((tvPtr->activePtr != NULL) &&
	    (Blt_TreeIsAncestor(entryPtr->node, tvPtr->activePtr->node))) {
	    tvPtr->activePtr = NULL;
	}
	Blt_TreeViewPruneSelection(tvPtr, entryPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * HideOp --
 *
 *	Hides one or more nodes.  Nodes can be specified by their
 *      inode, or by matching a name or data value pattern.  By
 *      default, the patterns are matched exactly.  They can also
 *      be matched using glob-style and regular expression rules.
 *
 * Results:
 *	A standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
static int
HideOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int status, nonmatching;

    status = SearchAndApplyToTree(tvPtr, interp, objc, objv, 
	HideEntryApplyProc, &nonmatching);

    if (status != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * If this was an inverted search, scan back through the
     * tree and make sure that the parents for all visible
     * nodes are also visible.  After all, if a node is supposed
     * to be visible, its parent can't be hidden.
     */
    if (nonmatching) {
	Blt_TreeViewApply(tvPtr, tvPtr->rootPtr, MapAncestorsApplyProc, 0);
    }
    /*
     * Make sure that selections are cleared from any hidden
     * nodes.  This wasn't done earlier--we had to delay it until
     * we fixed the visibility status for the parents.
     */
    Blt_TreeViewApply(tvPtr, tvPtr->rootPtr, FixSelectionsApplyProc, 0);

    /* Hiding an entry only effects the visible nodes. */
    tvPtr->flags |= (TV_LAYOUT | TV_SCROLL | TV_UPDATE | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ShowOp --
 *
 *	Mark one or more nodes to be exposed.  Nodes can be specified
 *	by their inode, or by matching a name or data value pattern.  By
 *      default, the patterns are matched exactly.  They can also
 *      be matched using glob-style and regular expression rules.
 *
 * Results:
 *	A standard Tcl result.
 *
 *----------------------------------------------------------------------
 */
static int
ShowOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    if (SearchAndApplyToTree(tvPtr, interp, objc, objv, ShowEntryApplyProc,
	    (int *)NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    tvPtr->flags |= (TV_LAYOUT | TV_SCROLL | TV_UPDATE|TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IndexOp --
 *
 *	Converts one of more words representing indices of the entries
 *	in the treeview widget to their respective serial identifiers.
 *
 * Results:
 *	A standard Tcl result.  Interp->result will contain the
 *	identifier of each inode found. If an inode could not be found,
 *	then the serial identifier will be the empty string.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    char *string;
    TreeViewEntry *fromPtr;
    int usePath, quiet;

    quiet = FALSE;
    usePath = FALSE;
    fromPtr = NULL;
    string = Tcl_GetString(objv[2]);
    while (string[0] == '-' && objc>3) {
        if ((strcmp(string, "-path") == 0)) {
    	   usePath = TRUE;
    	   objv++, objc--;
        } else if ((strcmp(string, "-quiet") == 0)) {
    	   quiet = TRUE;
    	   objv++, objc--;
        } else if ((strcmp(string, "-at") == 0)) {
    	   if (Blt_TreeViewGetEntry(tvPtr, objv[3], &fromPtr) != TCL_OK) {
    	      return TCL_ERROR;
    	   }
    	   objv += 2, objc -= 2;
        } else {
            break;
        }
        string = Tcl_GetString(objv[2]);
    }
    if (objc != 3) {
    	Tcl_AppendResult(interp, "wrong # args: should be \"", 
    		Tcl_GetString(objv[0]), 
    		" ?-at tagOrId? ?-path? ?-quiet? index\"", 
    		(char *)NULL);
    	return TCL_ERROR;
    }
    tvPtr->fromPtr = fromPtr;
    if (usePath) {
	if (fromPtr == NULL) {
	    fromPtr = tvPtr->rootPtr;
	}
	string = Tcl_GetString(objv[2]);
	entryPtr = FindPath(tvPtr, fromPtr, string);
	if (entryPtr != NULL) {
	    Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
	} else if (quiet) {
             Tcl_ResetResult(interp);
             return TCL_OK;
         } else {
	    return TCL_ERROR;
	}
    } else {
        if (tvPtr->fromPtr == NULL) {
            tvPtr->fromPtr = tvPtr->focusPtr;
        }
        if (tvPtr->fromPtr == NULL) {
            tvPtr->fromPtr = tvPtr->rootPtr;
        }
        if ((GetEntryFromObj2(tvPtr, objv[2], &entryPtr) == TCL_OK)) {
             if (entryPtr != NULL && entryPtr->node != NULL) {
                 Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
             }
	} else if (quiet) {
             Tcl_ResetResult(interp);
             return TCL_OK;
	} else {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * InsertOp --
 *
 *	Add new entries into a hierarchy.  If no node is specified,
 *	new entries will be added to the root of the hierarchy.
 *
 *----------------------------------------------------------------------
 */
static int
InsertOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_TreeNode node, parent;
    int insertPos;
    int depth, count;
    char *path;
    Tcl_Obj *CONST *options;
    Tcl_Obj *listObjPtr;
    char **compArr = NULL;
    register char **p;
    register int n;
    TreeViewEntry *rootPtr;
    char *string, *subPath;
    int nLen, idx, useid, oLen;
    int sobjc, tobjc;
    Tcl_Obj **sobjv;
    Tcl_Obj **tobjv;
    TreeViewStyle *stylePtr;
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    Tcl_DString dStr;
    int optSkips, i, m, start, nOptions/*, inode*/;

    useid = -1;
    /*inode = -1;*/
    optSkips = 0;
    sobjc = 0;
    tobjc = 0;
    entryPtr = NULL;
    rootPtr = tvPtr->rootPtr;
    
    if (objc == 2) {
	Tcl_AppendResult(interp, "missing position argument", (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_GetPositionFromObj(interp, objv[2], &insertPos) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        char *cp = Tcl_GetString(objv[i]);
        if (cp[0] == '-') break;
    }
    start = i;
    nOptions = (objc<4?0:objc - i);
    options = objv + start;

    if (nOptions%2) {
        Tcl_AppendResult(interp, "odd number of options", 0);
        return TCL_ERROR;
    }
    for (count = start; count < objc; count += 2) {
        string = Tcl_GetString(objv[count]);
        if (string[0] != '-') {
            Tcl_AppendResult(interp, "option must start with a dash: ", string, 0);
            return TCL_ERROR;
        }
        if (!strcmp("-tags", string)) {
            if (Tcl_ListObjGetElements(interp, objv[count+1], &tobjc, &tobjv) != TCL_OK) {
                return TCL_ERROR;
            }
	        
        } else if (!strcmp("-styles", string)) {
            if (Tcl_ListObjGetElements(interp, objv[count+1], &sobjc, &sobjv) != TCL_OK) {
                return TCL_ERROR;
            }
            if (sobjc%2) {
                Tcl_AppendResult(interp, "odd arguments for \"-styles\" flag", 0);
                return TCL_ERROR;
            }
            for (n=0; n<sobjc; n += 2) {
                if (Blt_TreeViewGetColumn(interp, tvPtr, sobjv[n], &columnPtr) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (columnPtr == &tvPtr->treeColumn) {
                    Tcl_AppendResult(interp, "tree column invalid in \"-styles\" flag", 0);
                    return TCL_ERROR;
                }
                if (Blt_TreeViewGetStyleMake(interp, tvPtr,
                    Tcl_GetString(sobjv[n+1]), &stylePtr, columnPtr, NULL,
                    NULL) != TCL_OK) {
                    return TCL_ERROR;
                }
                stylePtr->refCount--;
            }
                
        } else if (!strcmp("-at", string)) {
            if (Blt_TreeViewGetEntry(tvPtr, objv[count+1], &rootPtr) != TCL_OK) {
                return TCL_ERROR;
            }
                
        } else if (!strcmp("-node", string)) {
            if (Tcl_GetIntFromObj(tvPtr->interp, objv[count+1], &useid) != TCL_OK) {
                return TCL_ERROR;
            }
        } else {
            continue;
        }
        optSkips++;
    }
    Tcl_DStringInit(&dStr);
    node = NULL;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    m = 2;
    if (objc == 3) {
        path = "#auto";
        options = NULL;
        oLen = 5;
        goto addpath;
    }
    while (++m < start && m<objc) {
	path = Tcl_GetStringFromObj(objv[m], &oLen);

        if (oLen == 0) {
            path = "#auto";
            oLen = 5;
        }
addpath:
        node = NULL;
        /*inode = -1;*/
	if (oLen == 5
            && (tvPtr->pathSep == SEPARATOR_NONE || tvPtr->pathSep == NULL)
            && path[0] == '#' && strcmp(path, "#auto") == 0) {
                
	    parent = rootPtr->node;
	    if (useid>0) {
                node = Blt_TreeCreateNodeWithId(tvPtr->tree, parent,
                     Blt_Itoa(useid), useid, insertPos);
                if (node != NULL) {
                    goto makeent;
                }
                Tcl_AppendResult(interp, "node already exists", 0);
                goto error;
            }
            node = Blt_TreeCreateNode(tvPtr->tree, parent, NULL, 
                    insertPos);
            if (node == NULL) {
                Tcl_AppendResult(interp, "node create failed", 0);
                goto error;
            }
            if ((tvPtr->flags & TV_ALLOW_DUPLICATES) != 0) {
                if (Blt_TreeRelabelNode2(node, Blt_Itoa(Blt_TreeNodeId(node))) != TCL_OK) {
                    goto error;
                }
                goto makeent;
            }
            if (Blt_TreeFindChildRev(parent, Blt_Itoa(Blt_TreeNodeId(node)),
                    tvPtr->insertFirst) == NULL) {
                if (Blt_TreeRelabelNode2(node, Blt_Itoa(Blt_TreeNodeId(node))) != TCL_OK) {
                    goto error;
                }
                goto makeent;
            }
            /* Should never happen. */
            idx = (parent == tvPtr->rootNode ? tvPtr->nextIdx : tvPtr->nextSubIdx);
            for (;;) {
                idx++;
                if (Blt_TreeFindChildRev(parent, Blt_Itoa(idx),
                    tvPtr->insertFirst) == NULL) {
                    if (Blt_TreeRelabelNode2(node, Blt_Itoa(idx)) != TCL_OK) {
                        goto error;
                    }
                    goto makeent;
                }
            }
	}
	if (tvPtr->trimLeft != NULL) {
	    register char *s1, *s2;

	    /* Trim off leading character string if one exists. */
	    for (s1 = path, s2 = tvPtr->trimLeft; *s2 != '\0'; s2++, s1++) {
		if (*s1 != *s2) {
		    break;
		}
	    }
	    if (*s2 == '\0') {
		path = s1;
	    }
	}
	/*
	 * Split the path and find the parent node of the path.
	 */
	compArr = &path;
	depth = 1;
	if (tvPtr->pathSep != SEPARATOR_NONE) {
	    if (SplitPath(tvPtr, path, &depth, &compArr) != TCL_OK) {
		goto error;
	    }
	    if (depth == 0 && compArr != &path) {
		if (compArr) { Blt_Free(compArr); }
		compArr = NULL;
		continue;		/* Root already exists. */
	    }
	}
	parent = rootPtr->node;
	depth--;		

	/* Verify each component in the path preceding the tail.  */
	for (n = 0, p = compArr; n < depth; n++, p++) {
            node = Blt_TreeFindChildRev(parent, *p, tvPtr->insertFirst);
	    if (node == NULL) {
		if ((tvPtr->flags & TV_FILL_ANCESTORS) == 0) {
		    Tcl_AppendResult(interp, "can't find path component \"",
		         *p, "\" in \"", path, "\"", (char *)NULL);
		    goto error;
		}
		node = Blt_TreeCreateNode(tvPtr->tree, parent, *p, END);
		if (node == NULL) {
		    goto error;
		}
	    }
	    parent = node;
	}
	node = NULL;
	subPath = *p;
        nLen = strlen(subPath);
        if (nLen>=5 && (tvPtr->flags & TV_ALLOW_DUPLICATES) == 0 && strcmp(subPath+nLen-5,"#auto") == 0) {
            
             nLen -= 5;
             idx = (parent == tvPtr->rootNode ? tvPtr->nextIdx : tvPtr->nextSubIdx);
             for (;;) {
                 Tcl_DStringSetLength(&dStr, 0);
                 Tcl_DStringAppend(&dStr, *p, nLen);
                 Tcl_DStringAppend(&dStr, Blt_Itoa(idx), -1);
                 idx++;
                 node = Blt_TreeFindChildRev(parent, Tcl_DStringValue(&dStr),
                    tvPtr->insertFirst);
                 if (node == NULL) break;
             }
             if (parent == tvPtr->rootNode) {
                 tvPtr->nextIdx = idx;
             }
             subPath = Tcl_DStringValue(&dStr);
         } else if (((tvPtr->flags & TV_ALLOW_DUPLICATES) == 0) && 
	    (Blt_TreeFindChildRev(parent, subPath, tvPtr->insertFirst) != NULL)) {
	    Tcl_AppendResult(interp, "entry \"", subPath, "\" already exists in \"",
		 path, "\"", (char *)NULL);
	    goto error;
	}
         if (useid<=0) {
             node = Blt_TreeCreateNode(tvPtr->tree, parent, subPath, insertPos);
         } else {
             node = Blt_TreeCreateNodeWithId(tvPtr->tree, parent, subPath, useid, insertPos);
             useid++;
         }
	if (node == NULL) {
	    Tcl_AppendResult(interp, "failed to create node: ", subPath, 0);
	    goto error;
	}
makeent:
        /*inode = node->inode;*/
        if (Blt_TreeViewCreateEntry(tvPtr, node, 0, options, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            goto opterror;
        }
        entryPtr = Blt_NodeToEntry(tvPtr, node);
        Tcl_Preserve(entryPtr);
        for (i = 0; i<nOptions; i+=2) {
            char *str;
            int cRes;
            
            str = Tcl_GetString(options[i]);
            if (strcmp("-tags",str) && strcmp("-styles",str) && strcmp("-at",str) && strcmp("-node",str)) {
                cRes = Blt_TreeViewConfigureEntry(tvPtr, entryPtr, 2, options+i, 0);
                if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
                    Tcl_Release(entryPtr);
                    node = NULL;
                    goto error;
                }

                if (cRes != TCL_OK) {
                    Tcl_Release(entryPtr);
                    Blt_TreeViewFreeEntry(tvPtr, entryPtr);
                    goto error;
                }
            }
        }
        if (Blt_TreeInsertPost(tvPtr->tree, node) == NULL) {
            Tcl_Release(entryPtr);
            goto error;
        }
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
            Tcl_Release(entryPtr);
            node = NULL;
            goto error;
        }
        if (compArr != &path && compArr) {
	    Blt_Free(compArr);
	    compArr = NULL;
	}
	Tcl_ListObjAppendElement(interp, listObjPtr, NodeToObj(node));
        for (n=0; n<sobjc; n += 2) {
            TreeViewValue *valuePtr;
             
            if (entryPtr == NULL) {
                entryPtr = Blt_NodeToEntry(tvPtr, node);
            }

            Blt_TreeViewGetColumn(interp, tvPtr, sobjv[n], &columnPtr);
            for (valuePtr = entryPtr->values; valuePtr != NULL; 
                valuePtr = valuePtr->nextPtr) {
                if (valuePtr->columnPtr == columnPtr) {
                    if (Blt_TreeViewGetStyle(interp, tvPtr, Tcl_GetString(sobjv[n+1]), &stylePtr) == TCL_OK) {
                        stylePtr->refCount++;
                        valuePtr->stylePtr = stylePtr;
                    }
                    break;
                }
            }
        }
        for (n=0; n<tobjc; n++) {
            int tRes;
            
            tRes = AddTag(tvPtr, node, Tcl_GetString(tobjv[n]));
            if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
                Tcl_Release(entryPtr);
                node = NULL;
                goto error;
            }
            if (tRes != TCL_OK) {
                Tcl_Release(entryPtr);
                goto error;
            }
        }
        Tcl_Release(entryPtr);
    }
    tvPtr->flags |= (TV_LAYOUT | TV_SCROLL | TV_DIRTY | TV_RESORT);
    if (compArr && compArr != &path) {
        Blt_Free(compArr);
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    Tcl_DStringFree(&dStr);
    return TCL_OK;

  opterror:
    if (strncmp(Tcl_GetStringResult(interp), "unknown option", 14) == 0) {
        Tcl_AppendResult(interp, "\nshould be one of one of the insert options: -at, -isopen, -node, -styles, -tags\n or one of the entry options:  ", 0);
        Blt_FormatSpecOptions(interp, bltTreeViewEntrySpecs);
    }
  error:
    Tcl_DStringFree(&dStr);
    if (compArr && compArr != &path) {
	Blt_Free(compArr);
    }
    Tcl_DecrRefCount(listObjPtr);
    /* if (node != NULL && inode > = 0 && tvPtr->tree) {
        node = Blt_TreeGetNode(tvPtr->tree, inode)
    }*/
    if (node != NULL) {
	DeleteNode(tvPtr, node);
    }
    return TCL_ERROR;
}

#ifdef notdef
/*
 *----------------------------------------------------------------------
 *
 * AddOp -- UNUSED.
 *
 *	Add new entries into a hierarchy.  If no node is specified,
 *	new entries will be added to the root of the hierarchy.
 *
 *----------------------------------------------------------------------
 */

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
    int insertPos;
    Blt_TreeNode parent;
} InsertData;

static Blt_SwitchSpec insertSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", Blt_Offset(InsertData, insertPos), 0, 
	&afterSwitch},
    {BLT_SWITCH_INT_NONNEGATIVE, "-at", Blt_Offset(InsertData, insertPos), 0},
    {BLT_SWITCH_CUSTOM, "-before", Blt_Offset(InsertData, insertPos), 0,
	&beforeSwitch},
    {BLT_SWITCH_END, NULL, 0, 0}
};

static int
AddOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_TreeNode node, parent;
    int insertPos;
    int depth, count;
    char *path;
    Tcl_Obj *CONST *options;
    Tcl_Obj *listObjPtr;
    char **compArr;
    register char **p;
    register int n;
    TreeViewEntry *rootPtr;
    char *string;

    memset(&data, 0, sizeof(data));
    data.maxDepth = -1;
    data.cmdPtr = cmdPtr;

    /* Process any leading switches  */
    i = Blt_ProcessObjSwitches(interp, addSwitches, objc - 2, objv + 2, 
	     (char *)&data, BLT_CONFIG_OBJV_PARTIAL);
    if (i < 0) {
	return TCL_ERROR;
    }
    i += 2;
    /* Should have at the starting node */
    if (i >= objc) {
	Tcl_AppendResult(interp, "starting node argument is missing", 
		(char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[i], &rootPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    objv += i, objc -= i;
    node = NULL;

    /* Process sections of path ?options? */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    while (objc > 0) {
	path = Tcl_GetString(objv[0]);
	objv++, objc--;
	/*
	 * Count the option-value pairs that follow.  Count until we
	 * spot one that looks like an entry name (i.e. doesn't start
	 * with a minus "-").
	 */
	for (count = 0; count < objc; count += 2) {
	    if (!Blt_ObjIsOption(interp, bltTreeViewEntrySpecs, objv[count], 0)) {
		break;
	    }
	}
	if (count > objc) {
	    count = objc;
	}
	options = objv;
	objc -= count, objv += count;

	if (tvPtr->trimLeft != NULL) {
	    register char *s1, *s2;

	    /* Trim off leading character string if one exists. */
	    for (s1 = path, s2 = tvPtr->trimLeft; *s2 != '\0'; s2++, s1++) {
		if (*s1 != *s2) {
		    break;
		}
	    }
	    if (*s2 == '\0') {
		path = s1;
	    }
	}
	/*
	 * Split the path and find the parent node of the path.
	 */
	compArr = &path;
	depth = 1;
	if (tvPtr->pathSep != SEPARATOR_NONE) {
	    if (SplitPath(tvPtr, path, &depth, &compArr) != TCL_OK) {
		goto error;
	    }
	    if (depth == 0) {
		Blt_Free(compArr);
		continue;		/* Root already exists. */
	    }
	}
	parent = rootPtr->node;
	depth--;		

	/* Verify each component in the path preceding the tail.  */
	for (n = 0, p = compArr; n < depth; n++, p++) {
	    node = Blt_TreeFindChildRev(parent, *p, tvPtr->insertFirst);
	    if (node == NULL) {
		if ((tvPtr->flags & TV_FILL_ANCESTORS) == 0) {
		    Tcl_AppendResult(interp, "can't find path component \"",
		         *p, "\" in \"", path, "\"", (char *)NULL);
		    goto error;
		}
		node = Blt_TreeCreateNode(tvPtr->tree, parent, *p, END);
		if (node == NULL) {
		    goto error;
		}
	    }
	    parent = node;
	}
	node = NULL;
	if (((tvPtr->flags & ) == 0) && 
	    (Blt_TreeFindChildRev(parent, *p, tvPtr->insertFirst) != NULL)) {
	    Tcl_AppendResult(interp, "entry \"", *p, "\" already exists in \"",
		 path, "\"", (char *)NULL);
	    goto error;
	}
	node = Blt_TreeCreateNode(tvPtr->tree, parent, *p, insertPos);
	if (node == NULL) {
	    goto error;
	}
	if (Blt_TreeViewCreateEntry(tvPtr, node, count, options, 0) != TCL_OK) {
	    goto error;
	}
	if (compArr != &path) {
	    Blt_Free(compArr);
	}
	Tcl_ListObjAppendElement(interp, listObjPtr, NodeToObj(node));
    }
    tvPtr->flags |= (TV_LAYOUT | TV_SCROLL | TV_DIRTY | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;

  error:
    if (compArr != &path) {
	Blt_Free(compArr);
    }
    Tcl_DecrRefCount(listObjPtr);
    if (node != NULL) {
	DeleteNode(tvPtr, node);
    }
    return TCL_ERROR;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Deletes nodes from the hierarchy. Deletes one or more entries 
 *	(except root). In all cases, nodes are removed recursively.
 *
 *	Note: There's no need to explicitly clean up Entry structures 
 *	      or request a redraw of the widget. When a node is 
 *	      deleted in the tree, all of the Tcl_Objs representing
 *	      the various data fields are also removed.  The treeview 
 *	      widget store the Entry structure in a data field. So it's
 *	      automatically cleaned up when FreeEntryInternalRep is
 *	      called.
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
DeleteOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewTagInfo info = {0};
    TreeViewEntry *entryPtr;
    register int i;

    for (i = 2; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    if (entryPtr == tvPtr->rootPtr) {
		Blt_TreeNode next, node;

		/* 
		 *   Don't delete the root node.  We implicitly assume
		 *   that even an empty tree has at a root.  Instead
		 *   delete all the children regardless if they're closed
		 *   or hidden.
		 */
		for (node = Blt_TreeFirstChild(entryPtr->node); node != NULL; 
		     node = next) {
		    next = Blt_TreeNextSibling(node);
		    DeleteNode(tvPtr, node);
		}
	    } else {
		DeleteNode(tvPtr, entryPtr->node);
	    }
	}
        Blt_TreeViewDoneTaggedEntries(&info);
     }
    if (objc == 3 && !strcmp(Tcl_GetString(objv[2]), "all")) {
        tvPtr->nextIdx = 1;
        tvPtr->nextSubIdx = 1;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * MoveOp --
 *
 *	Move an entry into a new location in the hierarchy.
 *
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MoveOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Blt_TreeNode parent;
    TreeViewEntry *srcPtr, *destPtr;
    char c;
    int action;
    char *string;
    TreeViewTagInfo info = {0};

#define MOVE_INTO	(1<<0)
#define MOVE_BEFORE	(1<<1)
#define MOVE_AFTER	(1<<2)
    string = Tcl_GetString(objv[3]);
    c = string[0];
    if ((c == 'i') && (strcmp(string, "into") == 0)) {
	action = MOVE_INTO;
    } else if ((c == 'b') && (strcmp(string, "before") == 0)) {
	action = MOVE_BEFORE;
    } else if ((c == 'a') && (strcmp(string, "after") == 0)) {
	action = MOVE_AFTER;
    } else {
	Tcl_AppendResult(interp, "bad position \"", string,
	    "\": should be into, before, or after", (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[4], &destPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[2], &info) != TCL_OK) {
        return TCL_ERROR;
    }
    for (srcPtr = Blt_TreeViewFirstTaggedEntry(&info); srcPtr != NULL; 
	 srcPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	/* Verify they aren't ancestors. */
	if (Blt_TreeIsAncestor(srcPtr->node, destPtr->node)) {
	    Tcl_DString dString;
	    char *path;
	    
            Tcl_DStringInit(&dString);
	    path = Blt_TreeViewGetFullName(tvPtr, srcPtr, 1, &dString);
	    Tcl_AppendResult(interp, "can't move node: \"", path, 
			"\" is an ancestor of \"", Tcl_GetString(objv[4]), 
			"\"", (char *)NULL);
	    Tcl_DStringFree(&dString);
            Blt_TreeViewDoneTaggedEntries(&info);
	    return TCL_ERROR;
	}
	parent = Blt_TreeNodeParent(destPtr->node);
	if (parent == NULL) {
	    action = MOVE_INTO;
	}
	switch (action) {
	case MOVE_INTO:
	    Blt_TreeMoveNode(tvPtr->tree, srcPtr->node, destPtr->node, 
			     (Blt_TreeNode)NULL);
	    break;
	    
	case MOVE_BEFORE:
	    Blt_TreeMoveNode(tvPtr->tree, srcPtr->node, parent, destPtr->node);
	    break;
	    
	case MOVE_AFTER:
	    Blt_TreeMoveNode(tvPtr->tree, srcPtr->node, parent, 
			     Blt_TreeNextSibling(destPtr->node));
	    break;
	}
    }
    Blt_TreeViewDoneTaggedEntries(&info);
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/* .t nearest ?-root? ?-strict? x y ?var? */
/*ARGSUSED*/
static int
NearestOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr = NULL;
    TreeViewButton *buttonPtr = &tvPtr->button;
    int x, y, ox, oy;			/* Screen coordinates of the test point. */
    register TreeViewEntry *entryPtr;
    int isRoot, isStrict;
    char *string;

    isRoot = FALSE;
    isStrict = TRUE;
    while (objc>2) {
        string = Tcl_GetString(objv[2]);
        if (strcmp("-root", string) == 0) {
            isRoot = TRUE;
            objv++, objc--;
        } else if (strcmp("-strict", string) == 0) {
            isStrict = FALSE;
            objv++, objc--;
        } else break;
    }
    if (objc < 4 || objc > 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		" ?-root? ?-strict? x y ?var?\"", (char *)NULL);
	return TCL_ERROR;
			 
    }
    if ((Tk_GetPixelsFromObj(interp, tvPtr->tkwin, objv[2], &x) != TCL_OK) ||
	(Tk_GetPixelsFromObj(interp, tvPtr->tkwin, objv[3], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    ox = x;
    oy = y;
    if (tvPtr->nVisible == 0) {
	return TCL_OK;
    }
    if (isRoot) {
	int rootX, rootY;

	Tk_GetRootCoords(tvPtr->tkwin, &rootX, &rootY);
	x -= rootX;
	y -= rootY;
    }
    entryPtr = Blt_TreeViewNearestEntry(tvPtr, x, y, isStrict);
    if (entryPtr == NULL) {
	return TCL_OK;
    }
    columnPtr = Blt_TreeViewNearestColumn(tvPtr, x, y, NULL);
    x = WORLDX(tvPtr, x);
    y = WORLDY(tvPtr, y);

    if (objc > 4) {
	char *where;
	int labelX, labelY, depth, height, ly, lx, butEnd = 0, entryHeight, isTree;
	TreeViewIcon icon;
	
        entryHeight = MAX3(entryPtr->lineHeight, entryPtr->iconHeight, 
             tvPtr->button.height);
        isTree = (columnPtr && columnPtr == &tvPtr->treeColumn);

	where = "";
	
        if ((tvPtr->flags & TV_SHOW_COLUMN_TITLES)
             && oy<(tvPtr->insetY+tvPtr->titleHeight)) {
            TreeViewColumn *cPtr = columnPtr;
            
            ly = oy;
            lx = ox;
             
            if (cPtr->tW && (lx >= cPtr->tX) && (lx < (cPtr->tX + cPtr->tW)) &&
            (ly >= cPtr->tY) && (ly < (cPtr->tY + cPtr->tH))) {
                where = "titlelabel";
                goto done;
            }
		    
            if (cPtr->iW && (lx >= cPtr->iX) && (lx < (cPtr->iX + cPtr->iW)) &&
            (ly >= cPtr->iY) && (ly < (cPtr->iY + cPtr->iH))) {
                where = "titleicon";
                goto done;
            }
            
            where = "title";
            goto done;
	}
	if (isTree && entryPtr->flags & ENTRY_HAS_BUTTON) {
	    int buttonX, buttonY;

	    buttonX = entryPtr->worldX + entryPtr->buttonX;
	    buttonY = entryPtr->worldY + entryPtr->buttonY;
	    butEnd = entryPtr->buttonX + buttonPtr->width;
	    if ((x >= buttonX) && (x < (buttonX + buttonPtr->width)) &&
		(y >= buttonY) && (y < (buttonY + buttonPtr->height))) {
		where = "button";
		goto done;
	    }
	} 
	depth = DEPTH(tvPtr, entryPtr->node);

	icon = Blt_TreeViewGetEntryIcon(tvPtr, entryPtr);
	if (isTree && icon != NULL) {
	    int iconWidth, iconHeight;
	    int iconX, iconY;
	    
	    /* entryHeight = MAX(entryPtr->iconHeight, tvPtr->button.height); */
	    butEnd = entryPtr->buttonX + buttonPtr->width;
	    iconHeight = TreeViewIconHeight(icon);
	    iconWidth = TreeViewIconWidth(icon);
	    iconX = entryPtr->worldX + butEnd;
	    iconY = entryPtr->worldY + tvPtr->leader/2;
	    if (tvPtr->flatView) {
		iconX += (ICONWIDTH(0) - iconWidth) / 2;
	    } else {
		iconX += (ICONWIDTH(depth + 1) - iconWidth) / 2;
	    }	    
	    iconY += (entryHeight - iconHeight) / 2;
	    if ((x >= iconX) && (x <= (iconX + iconWidth)) &&
		(y >= iconY) && (y < (iconY + iconHeight))) {
		where = "icon";
		goto done;
	    }
	}
	labelX = entryPtr->worldX + ICONWIDTH(depth);
	labelY = entryPtr->worldY + (icon == NULL ? 0 : tvPtr->leader/2);
	if (!tvPtr->flatView) {
	    labelX += ICONWIDTH(depth + 1) + 4;
	}
	height = entryPtr->labelHeight;
	ly = y;
        if (height < entryHeight) {
             ly -= (entryHeight - height) / 2;
        }

	if (isTree && (x >= labelX) && (x < (labelX + entryPtr->labelWidth)) &&
	    (ly >= labelY) && (ly < (labelY + entryPtr->labelHeight))) {
	    where = "label";
	}
	
	if (columnPtr && isTree == 0) {
             TreeViewValue *vPtr;
		
             vPtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
             if (vPtr == NULL) goto done;
             ly = y - tvPtr->yOffset + tvPtr->titleHeight;
             lx = x - tvPtr->xOffset;
             
             if (vPtr->tW && (lx >= vPtr->tX) && (lx < (vPtr->tX + vPtr->tW)) &&
             (ly >= vPtr->tY) && (ly < (vPtr->tY + vPtr->tH))) {
                 where = "datalabel";
                 goto done;
             }
		    
             if (vPtr->iW && (lx >= vPtr->iX) && (lx < (vPtr->iX + vPtr->iW)) &&
             (ly >= vPtr->iY) && (ly < (vPtr->iY + vPtr->iH))) {
                 where = "dataicon";
                 goto done;
             }

	}
    done:
	if (Tcl_SetVar(interp, Tcl_GetString(objv[4]), where, 
		TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}
    }
    Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    return TCL_OK;
}


static int
OpenTreeEntry(TreeView *tvPtr, TreeViewEntry *entryPtr) {
    if (!Blt_TreeViewIsLeaf(entryPtr)) {
        return Blt_TreeViewOpenEntry(tvPtr, entryPtr);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
OpenOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewTagInfo info = {0};
    int recurse, trees, parent, result;
    register int i;

    recurse = FALSE;
    trees = FALSE;
    parent = FALSE;
    while (objc > 2) {
	int length;
	char *string;

	string = Tcl_GetStringFromObj(objv[2], &length);
	if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-recurse", length) == 0)) {
	    objv++, objc--;
	    recurse = TRUE;
	} else if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-trees", length) == 0)) {
	    objv++, objc--;
	    trees = TRUE;
	} else if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-parent", length) == 0)) {
	    objv++, objc--;
	    parent = TRUE;
	} else break;
    }
    for (i = 2; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    if (parent) {
                 while ((entryPtr = Blt_TreeViewParentEntry(entryPtr))) {
                     if (entryPtr) {
                         result = Blt_TreeViewOpenEntry(tvPtr, entryPtr);
                     }
                 }
                 continue;
            }

	    if (trees) {
		result = Blt_TreeViewApply(tvPtr, entryPtr, 
					   OpenTreeEntry, 0);
	    } else if (recurse) {
		result = Blt_TreeViewApply(tvPtr, entryPtr, 
					   Blt_TreeViewOpenEntry, 0);
	    } else {
		result = Blt_TreeViewOpenEntry(tvPtr, entryPtr);
	    }
	    if (result != TCL_OK) {
                 tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
                 Blt_TreeViewDoneTaggedEntries(&info);
                 return TCL_ERROR;
	    }
	    /* Make sure ancestors of this node aren't hidden. */
	    MapAncestors(tvPtr, entryPtr);
	}
        Blt_TreeViewDoneTaggedEntries(&info);
    }
    /*FIXME: This is only for flattened entries.  */
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RangeOp --
 *
 *	Returns the node identifiers in a given range.
 *
 *----------------------------------------------------------------------
 */
static int
RangeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr, *firstPtr, *lastPtr;
    unsigned int mask;
    int length;
    Tcl_Obj *listObjPtr, *objPtr;
    char *string;

    mask = 0;
    string = Tcl_GetStringFromObj(objv[2], &length);
    if ((string[0] == '-') && (length > 1) && 
	(strncmp(string, "-open", length) == 0)) {
	objv++, objc--;
	mask |= ENTRY_CLOSED;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[2], &firstPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 3) {
	if (Blt_TreeViewGetEntry(tvPtr, objv[3], &lastPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	lastPtr = LastEntry(tvPtr, firstPtr, mask);
    }    
    if (mask & ENTRY_CLOSED) {
	if (firstPtr->flags & ENTRY_HIDDEN) {
	    Tcl_AppendResult(interp, "first node \"", Tcl_GetString(objv[2]), 
		"\" is hidden.", (char *)NULL);
	    return TCL_ERROR;
	}
	if (lastPtr->flags & ENTRY_HIDDEN) {
	    Tcl_AppendResult(interp, "last node \"", Tcl_GetString(objv[3]), 
		"\" is hidden.", (char *)NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * The relative order of the first/last markers determines the
     * direction.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (Blt_TreeIsBefore(lastPtr->node, firstPtr->node)) {
	for (entryPtr = lastPtr; entryPtr != NULL; 
	     entryPtr = Blt_TreeViewPrevEntry(entryPtr, mask)) {
	    objPtr = NodeToObj(entryPtr->node);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (entryPtr == firstPtr) {
		break;
	    }
	}
    } else {
	for (entryPtr = firstPtr; entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextEntry(entryPtr, mask)) {
	    objPtr = NodeToObj(entryPtr->node);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (entryPtr == lastPtr) {
		break;
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ScanOp --
 *
 *	Implements the quick scan.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ScanOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    int x, y;
    char c;
    int length;
    int oper;
    char *string;
    Tk_Window tkwin;

#define SCAN_MARK	1
#define SCAN_DRAGTO	2
    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    tkwin = tvPtr->tkwin;
    if ((c == 'm') && (strncmp(string, "mark", length) == 0)) {
	oper = SCAN_MARK;
    } else if ((c == 'd') && (strncmp(string, "dragto", length) == 0)) {
	oper = SCAN_DRAGTO;
    } else {
	Tcl_AppendResult(interp, "bad scan operation \"", string,
	    "\": should be either \"mark\" or \"dragto\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((Blt_GetPixelsFromObj(interp, tkwin, objv[3], 0, &x) != TCL_OK) ||
	(Blt_GetPixelsFromObj(interp, tkwin, objv[4], 0, &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (oper == SCAN_MARK) {
	tvPtr->scanAnchorX = x;
	tvPtr->scanAnchorY = y;
	tvPtr->scanX = tvPtr->xOffset;
	tvPtr->scanY = tvPtr->yOffset;
    } else {
	int worldX, worldY;
	int dx, dy;

	dx = tvPtr->scanAnchorX - x;
	dy = tvPtr->scanAnchorY - y;
	worldX = tvPtr->scanX + (10 * dx);
	worldY = tvPtr->scanY + (10 * dy);

	if (worldX < 0) {
	    worldX = 0;
	} else if (worldX >= tvPtr->worldWidth) {
	    worldX = tvPtr->worldWidth - tvPtr->xScrollUnits;
	}
	if (worldY < 0) {
	    worldY = 0;
	} else if (worldY >= tvPtr->worldHeight) {
	    worldY = tvPtr->worldHeight - tvPtr->yScrollUnits;
	}
	tvPtr->xOffset = worldX;
	tvPtr->yOffset = worldY;
	tvPtr->flags |= TV_SCROLL;
	Blt_TreeViewEventuallyRedraw(tvPtr);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SeeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int width, height;
    int x, y;
    Tk_Anchor anchor;
    int left, right, top, bottom;
    char *string;

    string = Tcl_GetString(objv[2]);
    anchor = TK_ANCHOR_W;	/* Default anchor is West */
    if ((string[0] == '-') && (strcmp(string, "-anchor") == 0)) {
	if (objc == 3) {
	    Tcl_AppendResult(interp, "missing \"-anchor\" argument",
		(char *)NULL);
	    return TCL_ERROR;
	}
	if (Tk_GetAnchorFromObj(interp, objv[3], &anchor) != TCL_OK) {
	    return TCL_ERROR;
	}
	objc -= 2, objv += 2;
    }
    if (objc == 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
	    Tcl_GetString(objv[0]),
	    "see ?-anchor anchor? tagOrId\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (GetEntryFromObj(tvPtr, objv[2], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (entryPtr == NULL || tvPtr->noScroll) {
	return TCL_OK;
    }
    if (entryPtr->flags & ENTRY_HIDDEN) {
	MapAncestors(tvPtr, entryPtr);
	tvPtr->flags |= TV_SCROLL;
	/*
	 * If the entry wasn't previously exposed, its world coordinates
	 * aren't likely to be valid.  So re-compute the layout before
	 * we try to see the viewport to the entry's location.
	 */
	Blt_TreeViewComputeLayout(tvPtr);
    }
    width = VPORTWIDTH(tvPtr);
    height = VPORTHEIGHT(tvPtr);

    /*
     * XVIEW:	If the entry is left or right of the current view, adjust
     *		the offset.  If the entry is nearby, adjust the view just
     *		a bit.  Otherwise, center the entry.
     */
    left = tvPtr->xOffset;
    right = tvPtr->xOffset + width;

    switch (anchor) {
    case TK_ANCHOR_W:
    case TK_ANCHOR_NW:
    case TK_ANCHOR_SW:
	x = 0;
	break;
    case TK_ANCHOR_E:
    case TK_ANCHOR_NE:
    case TK_ANCHOR_SE:
	x = entryPtr->worldX + entryPtr->width + 
	    ICONWIDTH(DEPTH(tvPtr, entryPtr->node)) - width;
	break;
    default:
	if (entryPtr->worldX < left) {
	    x = entryPtr->worldX;
	} else if ((entryPtr->worldX + entryPtr->width) > right) {
	    x = entryPtr->worldX + entryPtr->width - width;
	} else {
	    x = tvPtr->xOffset;
	}
	break;
    }
    /*
     * YVIEW:	If the entry is above or below the current view, adjust
     *		the offset.  If the entry is nearby, adjust the view just
     *		a bit.  Otherwise, center the entry.
     */
    top = tvPtr->yOffset;
    bottom = tvPtr->yOffset + height;

    switch (anchor) {
    case TK_ANCHOR_N:
    case TK_ANCHOR_NE:
    case TK_ANCHOR_NW:
	y = entryPtr->worldY;
	break;
    case TK_ANCHOR_CENTER:
	y = entryPtr->worldY - (height / 2);
	break;
    case TK_ANCHOR_S:
    case TK_ANCHOR_SE:
    case TK_ANCHOR_SW:
	y = entryPtr->worldY + entryPtr->height - height;
	break;
    default:
	if (entryPtr->worldY < top) {
	    y = entryPtr->worldY;
	} else if ((entryPtr->worldY + entryPtr->height) > bottom) {
	    y = entryPtr->worldY + entryPtr->height - height;
	} else {
	    y = tvPtr->yOffset;
	}
	break;
    }
    if ((y != tvPtr->yOffset) || (x != tvPtr->xOffset)) {
	/* tvPtr->xOffset = x; */
	tvPtr->yOffset = y;
	tvPtr->flags |= TV_SCROLL;
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

void
Blt_TreeViewClearSelection(tvPtr)
    TreeView *tvPtr;
{
    if (tvPtr->selectMode & SELECT_MODE_CELLMASK) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch cursor;
        TreeViewValue *valuePtr;
        TreeViewEntry *entryPtr;
        TreeViewColumn *columnPtr;
        Blt_ChainLink *linkPtr;

        for (hPtr = Blt_FirstHashEntry(&tvPtr->selectTable, &cursor);
            hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
            entryPtr = (TreeViewEntry *)Blt_GetHashKey(&tvPtr->selectTable, hPtr);
            for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
                linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
                columnPtr = Blt_ChainGetValue(linkPtr);
                valuePtr = (TreeViewValue *)Blt_TreeViewFindValue(entryPtr, columnPtr);
                if (valuePtr != NULL) {
                    valuePtr->selected = 0;
                }
            }
        }
    }
    Blt_DeleteHashTable(&tvPtr->selectTable);
    Blt_InitHashTable(&tvPtr->selectTable, BLT_ONE_WORD_KEYS);
    Blt_ChainReset(tvPtr->selChainPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    if (tvPtr->selectCmd != NULL) {
	EventuallyInvokeSelectCmd(tvPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * LostSelection --
 *
 *	This procedure is called back by Tk when the selection is grabbed
 *	away.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is
 *	marked as not containing a selection.
 *
 *----------------------------------------------------------------------
 */
static void
LostSelection(clientData)
    ClientData clientData;	/* Information about the widget. */
{
    TreeView *tvPtr = clientData;

    if ((tvPtr->flags & TV_SELECT_EXPORT) == 0) {
	return;
    }
    Blt_TreeViewClearSelection(tvPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SelectRange --
 *
 *	Sets the selection flag for a range of nodes.  The range is
 *	determined by two pointers which designate the first/last
 *	nodes of the range.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
static int
SelectRange(tvPtr, fromPtr, toPtr)
    TreeView *tvPtr;
    TreeViewEntry *fromPtr, *toPtr;
{
    if (tvPtr->flatView) {
	register int i;

	if (fromPtr->flatIndex > toPtr->flatIndex) {
	    for (i = fromPtr->flatIndex; i >= toPtr->flatIndex; i--) {
		SelectEntryApplyProc(tvPtr, tvPtr->flatArr[i], NULL);
	    }
	} else {
	    for (i = fromPtr->flatIndex; i <= toPtr->flatIndex; i++) {
		SelectEntryApplyProc(tvPtr, tvPtr->flatArr[i], NULL);
	    }
	}	    
    } else {
	TreeViewEntry *entryPtr;
	TreeViewIterProc *proc;
	/* From the range determine the direction to select entries. */

	proc = (Blt_TreeIsBefore(toPtr->node, fromPtr->node)) 
	    ? Blt_TreeViewPrevEntry : Blt_TreeViewNextEntry;
	for (entryPtr = fromPtr; entryPtr != NULL;
	     entryPtr = (*proc)(entryPtr, ENTRY_MASK)) {
	    SelectEntryApplyProc(tvPtr, entryPtr, NULL);
	    if (entryPtr == toPtr) {
		break;
	    }
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionAnchorOp --
 *
 *	Sets the selection anchor to the element given by a index.
 *	The selection anchor is the end of the selection that is fixed
 *	while dragging out a selection with the mouse.  The index
 *	"anchor" may be used to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    
    columnPtr = NULL;

    if (objc<=3) {
        Tcl_Obj *listObjPtr, *objPtr;
        if (tvPtr->selAnchorPtr == NULL) return TCL_OK;
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        objPtr = NodeToObj(tvPtr->selAnchorPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        if (tvPtr->selAnchorCol != NULL) {
            objPtr = Tcl_NewStringObj( tvPtr->selAnchorCol->key, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 4) {
        if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    /* Set both the anchor and the mark. Indicates that a single entry
     * is selected. */
    tvPtr->selAnchorPtr = entryPtr;
    tvPtr->selMarkPtr = NULL;
    tvPtr->selAnchorCol = columnPtr;
    if (entryPtr != NULL) {
	Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * SelectionClearallOp
 *
 *	Clears the entire selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearallOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;	/* Not used. */
{
    Blt_TreeViewClearSelection(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *	Returns 1 if the element indicated by index is currently
 *	selected, 0 if it isn't.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int bool;
    TreeViewColumn *columnPtr;
    
    columnPtr = NULL;

    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 4) {
        if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    bool = Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionCellsOp
 *
 *	Returns pairs of node and col for selected cells.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionCellsOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    Tcl_Obj *listObjPtr, *objPtr;

    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    TreeViewValue *valuePtr;
    Blt_ChainLink *linkPtr;

    if (!(tvPtr->selectMode & SELECT_MODE_CELLMASK)) {
        Tcl_AppendResult(interp, "-selectmode not 'cell' or 'multicell'", 0);
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    for (hPtr = Blt_FirstHashEntry(&tvPtr->selectTable, &cursor);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        entryPtr = (TreeViewEntry *)Blt_GetHashKey(&tvPtr->selectTable, hPtr);
        for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
            linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
                
            columnPtr = Blt_ChainGetValue(linkPtr);
            valuePtr = (TreeViewValue *)Blt_TreeViewFindValue(entryPtr, columnPtr);
            if (valuePtr != NULL && valuePtr->selected) {
                objPtr = NodeToObj(entryPtr->node);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                objPtr = Tcl_NewStringObj(columnPtr->key, -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
    }


    Tcl_SetObjResult(interp, listObjPtr);
    
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * SelectionMarkOp --
 *
 *	Sets the selection mark to the element given by a index.
 *	The selection anchor is the end of the selection that is movable
 *	while dragging out a selection with the mouse.  The index
 *	"mark" may be used to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    
    columnPtr = NULL;

    if (GetEntryFromObj(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 4) {
        if (Blt_TreeViewGetColumn(interp, tvPtr, objv[5], &columnPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (tvPtr->selAnchorPtr == NULL) {
	Tcl_AppendResult(interp, "selection anchor must be set first", 
		 (char *)NULL);
	return TCL_ERROR;
    }
    if (tvPtr->selMarkPtr != entryPtr) {
	Blt_ChainLink *linkPtr, *nextPtr;
	TreeViewEntry *selectPtr;

	/* Deselect entry from the list all the way back to the anchor. */
	for (linkPtr = Blt_ChainLastLink(tvPtr->selChainPtr); linkPtr != NULL; 
	     linkPtr = nextPtr) {
	    nextPtr = Blt_ChainPrevLink(linkPtr);
	    selectPtr = Blt_ChainGetValue(linkPtr);
	    if (selectPtr == tvPtr->selAnchorPtr) {
		break;
	    }
	    Blt_TreeViewDeselectEntry(tvPtr, selectPtr, NULL);
	}
	tvPtr->flags &= ~TV_SELECT_MASK;
	tvPtr->flags |= TV_SELECT_SET;
	SelectRange(tvPtr, tvPtr->selAnchorPtr, entryPtr);
	Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
	tvPtr->selMarkPtr = entryPtr;

	Blt_TreeViewEventuallyRedraw(tvPtr);
	if (tvPtr->selectCmd != NULL) {
	    EventuallyInvokeSelectCmd(tvPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionPresentOp
 *
 *	Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *	A standard Tcl result.  interp->result will contain a
 *	boolean string indicating if there is a selection.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    int bool;

    bool = (Blt_ChainGetLength(tvPtr->selChainPtr) > 0);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(bool));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionSetOp
 *
 *	Selects, deselects, or toggles all of the elements in the
 *	range between first and last, inclusive, without affecting the
 *	selection state of elements outside that range.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *firstPtr, *lastPtr;
    TreeViewColumn *columnPtr;
    char *string;

    columnPtr = NULL;
    tvPtr->flags &= ~TV_SELECT_MASK;
    string = Tcl_GetString(objv[2]);
    switch (string[0]) {
    case 's':
	tvPtr->flags |= TV_SELECT_SET;
	break;
    case 'c':
	tvPtr->flags |= TV_SELECT_CLEAR;
	break;
    case 't':
	tvPtr->flags |= TV_SELECT_TOGGLE;
	break;
    }
    if (Blt_TreeViewGetEntry(tvPtr, objv[3], &firstPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((firstPtr->flags & ENTRY_HIDDEN) && 
	(!(tvPtr->flags & TV_SELECT_CLEAR))) {
	Tcl_AppendResult(interp, "can't select hidden node \"", 
		Tcl_GetString(objv[3]), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    lastPtr = firstPtr;
    if (objc > 4) {
	if (Blt_TreeViewGetEntry(tvPtr, objv[4], &lastPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((lastPtr->flags & ENTRY_HIDDEN) && 
		(!(tvPtr->flags & TV_SELECT_CLEAR))) {
	    Tcl_AppendResult(interp, "can't select hidden node \"", 
		     Tcl_GetString(objv[4]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (objc > 5) {
        if (Blt_TreeViewGetColumn(interp, tvPtr, objv[5], &columnPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (firstPtr == lastPtr) {
	SelectEntryApplyProc(tvPtr, firstPtr, columnPtr);
    } else {
	SelectRange(tvPtr, firstPtr, lastPtr, columnPtr);
    }
    /* Set both the anchor and the mark. Indicates that a single entry
     * is selected. */
    if (tvPtr->selAnchorPtr == NULL) {
	tvPtr->selAnchorPtr = firstPtr;
    }
    if (tvPtr->flags & TV_SELECT_EXPORT) {
	Tk_OwnSelection(tvPtr->tkwin, XA_PRIMARY, LostSelection, tvPtr);
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    if (tvPtr->selectCmd != NULL) {
	EventuallyInvokeSelectCmd(tvPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *	This procedure handles the individual options for text
 *	selections.  The selected text is designated by start and end
 *	indices into the text pool.  The selected segment has both a
 *	anchored and unanchored ends.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"anchor", 1, (Blt_Op)SelectionAnchorOp, 3, 5, "?tagOrId? ?column?",},
    {"cells", 1, (Blt_Op)SelectionCellsOp, 3, 3, "",},
    {"clear", 5, (Blt_Op)SelectionSetOp, 4, 6, "first ?last? ?column?",},
    {"clearall", 6, (Blt_Op)SelectionClearallOp, 3, 3, "",},
    {"includes", 1, (Blt_Op)SelectionIncludesOp, 4, 5, "tagOrId  ?column?",},
    {"mark", 1, (Blt_Op)SelectionMarkOp, 4, 5, "tagOrId ?column?",},
    {"present", 1, (Blt_Op)SelectionPresentOp, 3, 3, "",},
    {"set", 1, (Blt_Op)SelectionSetOp, 4, 6, "first ?last? ?column?",},
    {"toggle", 1, (Blt_Op)SelectionSetOp, 4, 6, "first ?last? ?column?",},
};
static int nSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int
SelectionOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nSelectionOps, selectionOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (tvPtr, interp, objc, objv);
    return result;
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
TagForgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    register int i;

    for (i = 3; i < objc; i++) {
	if (Blt_TreeForgetTag(tvPtr->tree, Tcl_GetString(objv[i])) != TCL_OK) {
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
TagNamesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    /* objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj("nonroot", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj("rootchildren", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);*/
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;
	Blt_TreeTagEntry *tPtr;

	objPtr = Tcl_NewStringObj("root", -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	for (hPtr = Blt_TreeFirstTag(tvPtr->tree, &cursor); hPtr != NULL;
	     hPtr = Blt_NextHashEntry(&cursor)) {
	    tPtr = Blt_GetHashValue(hPtr);
	    objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	register int i;
	TreeViewEntry *entryPtr;
	Blt_List list;
	Blt_ListNode listNode;

	for (i = 3; i < objc; i++) {
	    if (Blt_TreeViewGetEntry(tvPtr, objv[i], &entryPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    list = Blt_ListCreate(BLT_ONE_WORD_KEYS);
	    Blt_TreeViewGetTags(interp, tvPtr, entryPtr, list);
	    for (listNode = Blt_ListFirstNode(list); listNode != NULL; 
		 listNode = Blt_ListNextNode(listNode)) {
		objPtr = Tcl_NewStringObj(Blt_ListGetKey(listNode), -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	    Blt_ListDestroy(list);
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TagNodesOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagNodesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Blt_HashTable nodeTable;
    Blt_TreeNode node;
    TreeViewTagInfo info = {0};
    Tcl_Obj *listObjPtr;
    Tcl_Obj *objPtr;
    TreeViewEntry *entryPtr;
    int isNew;
    register int i;

    Blt_InitHashTable(&nodeTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
             Tcl_ResetResult(interp);
             Blt_TreeViewDoneTaggedEntries(&info);
             continue;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    Blt_CreateHashEntry(&nodeTable, (char *)entryPtr->node, &isNew);
	}
        Blt_TreeViewDoneTaggedEntries(&info);
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
}

static int
TagDefine(tvPtr, interp, tagName)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    char *tagName;
{
    TreeViewEntry *entryPtr;

    if (strcmp(tagName, "root") == 0 || strcmp(tagName, "all") == 0 ||
    strcmp(tagName, "nonroot") == 0 || strcmp(tagName, "rootchildren") == 0) {
        Tcl_AppendResult(interp, "can't add reserved tag \"", tagName, "\"", 
            (char *)NULL);
            return TCL_ERROR;
    }
    if (isdigit(UCHAR(tagName[0]))) {
        Tcl_AppendResult(interp, "invalid tag \"", tagName, 
            "\": can't start with digit", (char *)NULL);
            return TCL_ERROR;
    }
    if (strstr(tagName,"->") != NULL) {
        Tcl_AppendResult(tvPtr->interp, "invalid tag \"", tagName, 
            "\": can't contain \"->\"", (char *)NULL);
            return TCL_ERROR;
    }
    if (tagName[0] == '@') {
        Tcl_AppendResult(tvPtr->interp, "invalid tag \"", tagName, 
            "\": can't start with \"@\"", (char *)NULL);
            return TCL_ERROR;
    } 
    if (GetEntryFromSpecialId(tvPtr, tagName, &entryPtr) == TCL_OK) {
        Tcl_AppendResult(interp, "invalid tag \"", tagName, 
            "\": is a special id", (char *)NULL);
            return TCL_ERROR;
    }
    return Blt_TreeAddTag(tvPtr->tree, NULL, tagName);
}

/*
 *----------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagAddOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    register int i;
    char *tagName;
    TreeViewTagInfo info = {0};

    tagName = Tcl_GetString(objv[3]);
    TagDefine(tvPtr, interp, tagName);
    for (i = 4; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    if (AddTag(tvPtr, entryPtr->node, tagName) != TCL_OK) {
                Blt_TreeViewDoneTaggedEntries(&info);
		return TCL_ERROR;
	    }
         }
         Blt_TreeViewDoneTaggedEntries(&info);
     }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *---------------------------------------------------------------------- 
 */
static int
TagExistsOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    char *tagName;
    TreeViewTagInfo info = {0};
    int exists = 0;

    if (objc == 4) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[3], &info) == TCL_OK) {
	    exists = 1;
	} else {
	    Tcl_ResetResult(interp);
	}
        Blt_TreeViewDoneTaggedEntries(&info);
    } else {
        tagName = Tcl_GetString(objv[3]);
        if (GetEntryFromObj(tvPtr, objv[4], &entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        exists = Blt_TreeHasTag(tvPtr->tree, entryPtr->node, tagName);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(exists));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
TagDeleteOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    char *tagName;
    int result;
    Blt_HashTable *tablePtr;
    TreeViewTagInfo info = {0};

    tagName = Tcl_GetString(objv[3]);
    tablePtr = Blt_TreeTagHashTable(tvPtr->tree, tagName);
    if (tablePtr != NULL) {
        register int i;
        Blt_HashEntry *hPtr;
	TreeViewEntry *entryPtr;

        for (i = 4; i < objc; i++) {
	    if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info)!= TCL_OK) {
		return TCL_ERROR;
	    }
	    for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); 
		entryPtr != NULL; 
		entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	        hPtr = Blt_FindHashEntry(tablePtr, (char *)entryPtr->node);
	        if (hPtr != NULL) {
                     result = Blt_TreeTagDelTrace(tvPtr->tree, entryPtr->node, tagName);
                     if (result != TCL_OK) {
                         if (result != TCL_BREAK) {
                             Blt_TreeViewDoneTaggedEntries(&info);
                             return TCL_ERROR;
                         }
                         continue;
                     }
                     Blt_DeleteHashEntry(tablePtr, hPtr);
	        }
	   }
           Blt_TreeViewDoneTaggedEntries(&info);
       }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TagOp --
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec tagOps[] = {
    {"add", 1, (Blt_Op)TagAddOp, 4, 0, "tag id...",},
    {"delete", 2, (Blt_Op)TagDeleteOp, 5, 0, "tag id...",},
    {"exists", 2, (Blt_Op)TagExistsOp, 4, 5, "tag ?id?",}, 
    {"forget", 1, (Blt_Op)TagForgetOp, 4, 0, "tag...",},
    {"names", 2, (Blt_Op)TagNamesOp, 3, 0, "?id...?",}, 
    {"nodes", 2, (Blt_Op)TagNodesOp, 4, 0, "tag ?tag...?",},
};

static int nTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nTagOps, tagOps, BLT_OP_ARG2, objc, objv, 
	0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc)(tvPtr, interp, objc, objv);
    return result;
}

/*ARGSUSED*/
static int
ToggleOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewTagInfo info = {0};
    int result = TCL_OK;

    if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[2], &info) != TCL_OK) {
	return TCL_ERROR;
    }
    for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL && result == TCL_OK; 
	 entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	if (entryPtr == NULL) {
            Blt_TreeViewDoneTaggedEntries(&info);
	    return TCL_OK;
	}
	if (entryPtr->flags & ENTRY_CLOSED) {
	    result = Blt_TreeViewOpenEntry(tvPtr, entryPtr);
	} else {
	    Blt_TreeViewPruneSelection(tvPtr, entryPtr);
	    if ((tvPtr->focusPtr != NULL) && 
		(Blt_TreeIsAncestor(entryPtr->node, tvPtr->focusPtr->node))) {
		tvPtr->focusPtr = entryPtr;
		Blt_SetFocusItem(tvPtr->bindTable, tvPtr->focusPtr, ITEM_ENTRY);
	    }
	    if ((tvPtr->selAnchorPtr != NULL) &&
		(Blt_TreeIsAncestor(entryPtr->node, 
				    tvPtr->selAnchorPtr->node))) {
		tvPtr->selAnchorPtr = NULL;
	    }
	    result = Blt_TreeViewCloseEntry(tvPtr, entryPtr);
	}
    }
    Blt_TreeViewDoneTaggedEntries(&info);
    if (result == TCL_OK) {
        tvPtr->flags |= TV_SCROLL;
        Blt_TreeViewEventuallyRedraw(tvPtr);
    }
    return result;
}

static int
XViewOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int width, worldWidth;

    width = VPORTWIDTH(tvPtr);
    worldWidth = tvPtr->worldWidth;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	/*
	 * Note that we are bounding the fractions between 0.0 and 1.0
	 * to support the "canvas"-style of scrolling.
	 */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	fract = (double)tvPtr->xOffset / worldWidth;
	fract = CLAMP(fract, 0.0, 1.0);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	fract = (double)(tvPtr->xOffset + width) / worldWidth;
	fract = CLAMP(fract, 0.0, 1.0);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (tvPtr->noScroll) { return TCL_OK; }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &tvPtr->xOffset,
	    worldWidth, width, tvPtr->xScrollUnits, tvPtr->scrollMode) 
	    != TCL_OK) {
	return TCL_ERROR;
    }
    tvPtr->flags |= TV_XSCROLL;
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

static int
YViewOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int height, worldHeight;

    height = VPORTHEIGHT(tvPtr);
    worldHeight = tvPtr->worldHeight;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* Report first and last fractions */
	fract = (double)tvPtr->yOffset / worldHeight;
	fract = CLAMP(fract, 0.0, 1.0);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	fract = (double)(tvPtr->yOffset + height) / worldHeight;
	fract = CLAMP(fract, 0.0, 1.0);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (tvPtr->noScroll) { return TCL_OK; }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &tvPtr->yOffset,
	    worldHeight, height, tvPtr->yScrollUnits, tvPtr->scrollMode)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    tvPtr->flags |= TV_SCROLL;
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 * --------------------------------------------------------------
 *
 * Blt_TreeViewWidgetInstCmd --
 *
 * 	This procedure is invoked to process commands on behalf of
 *	the treeview widget.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 * --------------------------------------------------------------
 */
static Blt_OpSpec treeViewOps[] =
{
    {"bbox", 2, (Blt_Op)BboxOp, 3, 0, "tagOrId...",}, 
    {"bind", 2, (Blt_Op)BindOp, 3, 5, "tagName ?sequence command?",}, 
    {"button", 2, (Blt_Op)ButtonOp, 2, 0, "args",},
    {"cget", 2, (Blt_Op)CgetOp, 3, 3, "option",}, 
    {"close", 2, (Blt_Op)CloseOp, 2, 0, "?-recurse? ?-parent? tagOrId...",}, 
    {"column", 3, (Blt_Op)Blt_TreeViewColumnOp, 2, 0, "oper args",}, 
    {"configure", 3, (Blt_Op)ConfigureOp, 2, 0, "?option value?...",},
    {"curselection", 2, (Blt_Op)CurselectionOp, 2, 2, "",},
    {"delete", 1, (Blt_Op)DeleteOp, 2, 0, "tagOrId ?tagOrId...?",}, 
    {"edit", 2, (Blt_Op)EditOp, 3, 0, "?-root|-test|-noscroll|-scroll? ?x y?",},
    {"entry", 2, (Blt_Op)EntryOp, 2, 0, "oper args",},
    {"find", 3, (Blt_Op)FindOp, 2, 0, "?switches? ?first last?",}, 
    {"focus", 2, (Blt_Op)FocusOp, 2, 3, "?tagOrId?",}, 
    {"get", 1, (Blt_Op)GetOp, 2, 0, "?-full? ?-labels? tagOrId ?tagOrId...?",}, 
    {"hide", 1, (Blt_Op)HideOp, 2, 0, "?switches? ?tagOrId...?",},
    {"index", 3, (Blt_Op)IndexOp, 3, 8, "?-at tagOrId? ?-path? ?-quiet? string",},
    {"insert", 3, (Blt_Op)InsertOp, 3, 0, "?-at tagOrId? ?-styles styleslist? ?-tags taglist? position label ?label...? ?option value?",},
    {"move", 1, (Blt_Op)MoveOp, 5, 5, "tagOrId into|before|after tagOrId",},
    {"nearest", 1, (Blt_Op)NearestOp, 4, 7, "?-root? ?-strict? x y ?varName?",}, 
    {"open", 1, (Blt_Op)OpenOp, 2, 0, "?-recurse? ?-parent? tagOrId...",}, 
    {"range", 1, (Blt_Op)RangeOp, 4, 5, "?-open? tagOrId tagOrId",},
    {"scan", 2, (Blt_Op)ScanOp, 5, 5, "dragto|mark x y",},
    {"see", 3, (Blt_Op)SeeOp, 3, 5, "?-anchor anchor? tagOrId",},
    {"selection", 3, (Blt_Op)SelectionOp, 2, 0, "oper args",},
    {"show", 2, (Blt_Op)ShowOp, 2, 0, "?switches? ?tagOrId...?",},
    {"sort", 2, (Blt_Op)Blt_TreeViewSortOp, 2, 0, "args",},
    {"style", 2, (Blt_Op)Blt_TreeViewStyleOp, 2, 0, "args",},
    {"tag", 2, (Blt_Op)TagOp, 2, 0, "oper args",},
    {"toggle", 2, (Blt_Op)ToggleOp, 3, 3, "tagOrId",},
    {"xview", 1, (Blt_Op)XViewOp, 2, 5, "?moveto fract? ?scroll number what?",},
    {"yview", 1, (Blt_Op)YViewOp, 2, 5, "?moveto fract? ?scroll number what?",},
};

static int nTreeViewOps = sizeof(treeViewOps) / sizeof(Blt_OpSpec);

int
Blt_TreeViewWidgetInstCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about the widget. */
    Tcl_Interp *interp;		/* Interpreter to report errors back to. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST *objv;	/* Vector of argument strings. */
{
    Blt_Op proc;
    TreeView *tvPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, nTreeViewOps, treeViewOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Blt_TreeViewChanged(tvPtr);
    Tcl_Preserve(tvPtr);
    result = (*proc) (tvPtr, interp, objc, objv);
    Tcl_Release(tvPtr);
    return result;
}

#endif /* NO_TREEVIEW */
