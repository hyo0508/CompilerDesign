/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"

/* SIZE is the size of the hash table */
#define SIZE 211

/* the list of line numbers of the source
 * code in which a variable is referenced
 */
typedef struct LineListRec
{
    int lineno;
    struct LineListRec *next;
} *LineList;

/* The record in the bucket lists for
 * each variable, including name,
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
{
    char *name;
    ExpType type;
    LineList lines;
    int memloc; /* memory location for variable */
    struct BucketListRec *next;
    TreeNode *treeNode;
} *BucketList;

/* The record for each scope,
 * including name, its bucket,
 * and parent scope.
 */
typedef struct ScopeListRec
{
    char *name;
    BucketList bucket[SIZE];
    struct ScopeListRec *parent;
    int location;
} *ScopeList;

extern ScopeList currScope;
extern ScopeList tempScope;

ScopeList findScope(char *scope);
ScopeList addScope(char *name);

void st_insert(char *scope, char *name, ExpType type, int lineno, int loc, TreeNode *t);
BucketList st_lookup(char *scope, char *name);
BucketList st_lookup_excluding_parent(char *scope, char *name);

void printSymTab(FILE *listing);

#endif
