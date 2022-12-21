/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash table */
// static BucketList hashTable[SIZE];
static ScopeList scopes[SIZE];
static int sidx = 0;
ScopeList currScope = NULL;
ScopeList tempScope = NULL;

/* the hash function */
static int hash(char *key)
{
  int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  {
    temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

ScopeList findScope(char *scope)
{
  for (int i = 0; i < SIZE; i++)
    if (strcmp(scope, scopes[i]->name) == 0)
      return scopes[i];
  return NULL;
}

ScopeList addScope(char *name)
{
  ScopeList newScope = (ScopeList)malloc(sizeof(struct ScopeListRec));
  newScope->name = name;
  newScope->parent = currScope;
  currScope = newScope;
  scopes[sidx++] = newScope;
  return newScope;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert(char *scope, char *name, ExpType type, int lineno, int loc, TreeNode *t)
{
  ScopeList insertScope = findScope(scope);
  int h = hash(name);
  BucketList l = insertScope->bucket[h];
  while ((l != NULL) && (strcmp(name, l->name) != 0))
    l = l->next;
  if (l == NULL) /* variable not yet in table */
  {
    l = (BucketList)malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->type = type;
    l->lines = (LineList)malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->lines->next = NULL;
    l->memloc = loc;
    l->treeNode = t;
    l->next = insertScope->bucket[h];
    insertScope->bucket[h] = l;
  }
  else /* found in table, so just add line number */
  {
    LineList t = l->lines;
    while (t->next != NULL)
      t = t->next;
    t->next = (LineList)malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
    l->treeNode = t;
  }
} /* st_insert */

/* Function st_lookup returns the memory
 * location of a variable or -1 if not found
 */
BucketList st_lookup(char *scope, char *name)
{
  ScopeList lookupScope = findScope(scope);
  int h = hash(name);
  while (lookupScope != NULL)
  {
    BucketList l = lookupScope->bucket[h];
    while ((l != NULL) && (strcmp(name, l->name) != 0))
      l = l->next;
    if (l != NULL)
    {
      tempScope = lookupScope;
      return l;
    }
    lookupScope = lookupScope->parent;
  }
  return NULL;
}
BucketList st_lookup_excluding_parent(char *scope, char *name)
{
  ScopeList lookupScope = findScope(scope);
  if (lookupScope == NULL)
    return NULL;
  int h = hash(name);
  BucketList l = lookupScope->bucket[h];
  while ((l != NULL) && (strcmp(name, l->name) != 0))
    l = l->next;
  if (l != NULL)
    return l;
  return NULL;
}

const char *typeToStr(ExpType type)
{
  switch (type)
  {
  case Void:
    return "Void";
  case Integer:
    return "Integer";
  case INTARR:
    return "Integer[]";
  default:
    return "unknown";
  }
}

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */
void printSymTab(FILE *listing)
{
  int i, j;
  fprintf(listing, "Variable Name  Type        Location  Scope      Line Numbers\n");
  fprintf(listing, "-------------  ----        --------  -----      ------------\n");
  for (j = 0; j < SIZE; ++j)
  {
    if (scopes[j] != NULL)
    {
      ScopeList scope = scopes[j];
      for (i = 0; i < SIZE; ++i)
      {
        if (scope->bucket[i] != NULL)
        {
          BucketList l = scope->bucket[i];
          while (l != NULL)
          {
            LineList t = l->lines;
            fprintf(listing, "%-14s ", l->name);
            fprintf(listing, "%-11s ", typeToStr(l->type));
            fprintf(listing, "%-8d  ", l->memloc);
            fprintf(listing, "%-2s  ", scope->name);
            while (t != NULL)
            {
              fprintf(listing, "%4d ", t->lineno);
              t = t->next;
            }
            fprintf(listing, "\n");
            l = l->next;
          }
        }
      }
    }
  }
} /* printSymTab */
