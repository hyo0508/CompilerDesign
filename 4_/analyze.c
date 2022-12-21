/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

#define DEBUG 0

static int scopeFlag = 0;
static char *funcName = NULL;
BucketList l = NULL;
ScopeList sc = NULL;

void addInput();
void addOutput();

typedef enum
{
  UndecFunc,
  UndecVar,
  RedefSym,
  VoidVar,
  NoIntIdx,
  NoArrIdx,
  InvalCall,
  InvalReturn,
  InvalAssign,
  InvalOper,
  InvalCond,
} ErrorKind;

static void semanticError(ErrorKind err, char *name, int lineno)
{
  switch (err)
  {
  case UndecFunc:
    fprintf(listing, "Error: undeclared function \"%s\" is called at line %d\n", name, lineno);
    break;
  case UndecVar:
    fprintf(listing, "Error: undeclared variable \"%s\" is used at line %d\n", name, lineno);
    break;
  case RedefSym:
    fprintf(listing, "Error: Symbol \"%s\" is redefined at line %d\n", name, lineno);
    break;
  case VoidVar:
    fprintf(listing, "Error: The void-type variable is declared at line %d (name : \"%s\")\n", lineno, name);
    break;
  case NoIntIdx:
    fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indicies should be integer\n", lineno, name);
    break;
  case NoArrIdx:
    fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indexing can only allowed for int[] variables\n", lineno, name);
    break;
  case InvalCall:
    fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", lineno, name);
    break;
  case InvalReturn:
    fprintf(listing, "Error: Invalid return at line %d\n", lineno);
    break;
  case InvalAssign:
    fprintf(listing, "Error: invalid assignment at line %d\n", lineno);
    break;
  case InvalOper:
    fprintf(listing, "Error: invalid operation at line %d\n", lineno);
    break;
  case InvalCond:
    fprintf(listing, "Error: invalid condition at line %d\n", lineno);
    break;
  default:
    break;
  }
  Error = TRUE;
}

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void traverse(TreeNode *t,
                     void (*preProc)(TreeNode *),
                     void (*postProc)(TreeNode *))
{
  if (t != NULL)
  {
    preProc(t);
    {
      int i;
      for (i = 0; i < MAXCHILDREN; i++)
        traverse(t->child[i], preProc, postProc);
    }
    postProc(t);
    traverse(t->sibling, preProc, postProc);
  }
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode(TreeNode *t)
{
  if (t->nodekind == StmtK)
  {
    switch (t->kind.stmt)
    {
    case VarDeclK:
      if (st_lookup_excluding_parent(currScope, t->attr.name) != NULL)
      {
        semanticError(RedefSym, t->attr.name, t->lineno);
      }
      else
      {
        st_insert(currScope, t->attr.name, t->type, t->lineno, t);
      }
      break;
    case FunDeclK:
      if (st_lookup_excluding_parent(currScope, t->attr.name) != NULL)
      {
        semanticError(RedefSym, t->attr.name, t->lineno);
      }
      else
      {
        st_insert(currScope, t->attr.name, t->type, t->lineno, t);
        addScope(t->attr.name);
        scopeFlag = 1;
        funcName = t->attr.name;
      }
      break;
    case CompK:
      if (scopeFlag == 1)
      {
        scopeFlag = 0;
      }
      else
      {
        char buffer[64];
        sprintf(buffer, "%s:%d", funcName, t->lineno);
        addScope(buffer);
      }
      break;
    default:
      break;
    }
  }
  else if (t->nodekind == ExpK)
  {
    switch (t->kind.exp)
    {
    case ParamK:
      st_insert(currScope, t->attr.name, t->type, t->lineno, t);
      break;
    case IdK:
    case CallK:
      l = st_lookup(currScope, t->attr.name);
      if (l != NULL)
      {
        t->type = l->type;
        st_insert(l->scope, t->attr.name, t->type, t->lineno, t);
      }
      break;
    default:
      break;
    }
  }
}

static void postProc(TreeNode *t)
{
  if (t->nodekind == StmtK && t->kind.stmt == CompK)
    currScope = currScope->parent;
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode *syntaxTree)
{
  addScope("global");
  addInput();
  addOutput();
  traverse(syntaxTree, insertNode, postProc);
  if (TraceAnalyze)
  {
    fprintf(listing, "\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode *t)
{
  TreeNode *param = NULL;
  TreeNode *arg = NULL;
  ExpType lType;
  ExpType rType;
  ScopeList globalScope = findScope("global");
  if (t->nodekind == StmtK)
  {
    switch (t->kind.stmt)
    {
    case VarDeclK:
      if (t->type == Void || t->type == VoidArr)
      {
        semanticError(VoidVar, t->attr.name, t->lineno);
      }
      break;
    case CompK:
      currScope = currScope->parent;
      break;
    case IfK:
    case IfElseK:
    case WhileK:
      if (t->child[0]->type != Integer)
      {
        semanticError(InvalCond, "", t->lineno);
      }
      break;
    case ReturnK:
      sc = currScope;
      while (sc->parent != globalScope)
      {
        sc = sc->parent;
      }
      l = st_lookup(globalScope, sc->name);
      if ((t->child[0] != NULL && l->type == Void) ||
          (t->child[0] == NULL && l->type != Void) ||
          (t->child[0] != NULL && l->type != Void && t->child[0]->type != l->type))
      {
        semanticError(InvalReturn, "", t->lineno);
      }
      break;
    case AssignK:
      if (t->child[0]->type != t->child[1]->type)
      {
        semanticError(InvalAssign, "", t->lineno);
      }
      break;
    default:
      break;
    }
  }
  else if (t->nodekind == ExpK)
  {
    switch (t->kind.exp)
    {
    case OpK:
      lType = t->child[0]->type;
      rType = t->child[1]->type;

      if (lType == IntegerArr && t->child[0]->child[0] != NULL)
      {
        lType = Integer;
      }
      if (rType == IntegerArr && t->child[1]->child[0] != NULL)
      {
        rType = Integer;
      }

      if (lType != Integer || rType != Integer)
      {
        semanticError(InvalOper, "", t->lineno);
      }
      else
      {
        t->type = Integer;
      }
      break;
    case ConstK:
      t->type = Integer;
      break;
    case IdK:
      l = st_lookup(currScope, t->attr.name);
      if (l == NULL)
      {
        semanticError(UndecVar, t->attr.name, t->lineno);
        break;
      }
      t->type = l->type;
      if (t->child[0] == NULL)
      {
        break;
      }
      if (l->type == IntegerArr)
      {
        if (t->child[0]->type != Integer)
        {
          semanticError(NoIntIdx, t->attr.name, t->lineno);
        }
        t->type -= 2;
      }
      else if (l->type == Integer)
      {
        semanticError(NoArrIdx, t->attr.name, t->lineno);
      }
      break;
    case CallK:
      l = st_lookup(currScope, t->attr.name);
      if (l == NULL)
      {
        semanticError(UndecFunc, t->attr.name, t->lineno);
        break;
      }
      t->type = l->type;
      param = l->treeNode->child[0];
      arg = t->child[0];

      if (param->kind.exp == VoidParamK)
      {
        if (arg != NULL)
        {
          semanticError(InvalCall, t->attr.name, t->lineno);
        }
        break;
      }

      while (param || arg)
      {
        if (!param || !arg || param->type != arg->type)
        {
          semanticError(InvalCall, t->attr.name, t->lineno);
          break;
        }
        param = param->sibling;
        arg = arg->sibling;
      }
      break;
    default:
      break;
    }
  }
}

static void beforeCheckNode(TreeNode *t)
{
  if (t->nodekind == StmtK)
  {
    switch (t->kind.stmt)
    {
    case FunDeclK:
      currScope = findScope(t->attr.name);
      scopeFlag = 1;
      funcName = t->attr.name;
      break;
    case CompK:
      if (scopeFlag == 1)
      {
        scopeFlag = 0;
      }
      else
      {
        char buffer[64];
        sprintf(buffer, "%s:%d", funcName, t->lineno);
        currScope = findScope(buffer);
      }
    default:
      break;
    }
  }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode *syntaxTree)
{
  currScope = findScope("global");
  traverse(syntaxTree, beforeCheckNode, checkNode);
}

void addInput()
{
  TreeNode *t = newStmtNode(FunDeclK);
  TreeNode *param = newExpNode(VoidParamK);
  TreeNode *comp = newStmtNode(CompK);
  t->attr.name = "input";
  t->type = Integer;
  t->child[0] = param;
  t->child[1] = comp;
  t->lineno = 0;
  st_insert(currScope, t->attr.name, t->type, t->lineno, t);
}

void addOutput()
{
  TreeNode *t = newStmtNode(FunDeclK);
  TreeNode *param = newExpNode(ParamK);
  TreeNode *comp = newStmtNode(CompK);
  param->type = Integer;
  t->attr.name = "output";
  t->type = Void;
  t->child[0] = param;
  t->child[1] = comp;
  t->lineno = 0;
  st_insert(currScope, t->attr.name, t->type, t->lineno, t);
}
