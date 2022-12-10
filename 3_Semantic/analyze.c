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
  switch (t->nodekind)
  {
  case DeclK:
    t->type = t->child[0]->attr.type;
    switch (t->kind.decl)
    {
    case VarK:
    case VarArrK:
      if (st_lookup_excluding_parent(currScope->name, t->attr.name) != NULL)
      {
        semanticError(RedefSym, t->attr.name, t->lineno);
        break;
      }
      st_insert(currScope->name, t->attr.name, t->type, t->lineno, currScope->location++, t);
      break;
    case FuncK:
      funcName = t->attr.name;
      if (st_lookup_excluding_parent(currScope->name, t->attr.name) != NULL)
      {
        semanticError(RedefSym, t->attr.name, t->lineno);
        break;
      }
      st_insert(currScope->name, t->attr.name, t->type, t->lineno, currScope->location++, t);
      addScope(t->attr.name);
      scopeFlag = 1;
      break;
    default:
      break;
    }
    break;
  case TypeK:
    break;
  case ParamK:
    if (t->kind.param == VoidParamK)
      break;
    t->type = t->child[0]->attr.type;
    st_insert(currScope->name, t->attr.name, t->type, t->lineno, currScope->location++, t);
    break;
  case StmtK:
    if (t->kind.stmt == CompK)
    {
      if (scopeFlag == 1)
      {
        scopeFlag = 0;
      }
      else
      {
        addScope("temp");
      }
      t->attr.scopeName = currScope->name;
    }
    break;
  case ExpK:
    switch (t->kind.exp)
    {
    case ConstK:
    case OpK:
      t->type = Integer;
      break;
    case CallK:
    case IdK:
    case ArrIdK:
    {
      BucketList l = st_lookup(currScope->name, t->attr.name);
      if (l == NULL)
      {
        if (t->kind.exp == CallK)
        {
          semanticError(UndecFunc, t->attr.name, t->lineno);
        }
        else
        {
          semanticError(UndecVar, t->attr.name, t->lineno);
        }
        break;
      }
      t->type = l->type;
      if (t->type == INTARR && t->child[1] != NULL)
      {
        t->type = Integer;
      }
      st_insert(tempScope->name, t->attr.name, t->type, t->lineno, 0, t);
      break;
    }
    default:
      break;
    }
    break;
  default:
    break;
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
  switch (t->nodekind)
  {
  case ExpK:
    switch (t->kind.exp)
    {
    case OpK:
      t->type = Integer;
      if (t->child[0] != NULL && t->child[1] != NULL && ((t->child[0]->type != Integer) || (t->child[1]->type != Integer)))
      {
        semanticError(InvalOper, "", t->lineno);
        break;
      }
      break;
    case IdK:
    case ArrIdK:
    {
      BucketList l = st_lookup(currScope->name, t->attr.name);
      if (l == NULL)
      {
        break;
      }
      TreeNode *symbolNode = l->treeNode;
      if (t->kind.exp == ArrIdK)
      {
        if ((symbolNode->nodekind == DeclK && l->type != INTARR) || (symbolNode->nodekind == ParamK && l->type != INTARR))
        {
          semanticError(InvalAssign, "", lineno);
          break;
        }
      }
      if (t->child[0] != NULL && t->child[0]->type != Integer)
      {
        semanticError(NoIntIdx, t->attr.name, t->lineno);
      }
      t->type = symbolNode->type;
      break;
    }
    case AssignK:
      if (t->child[0] == NULL && t->child[1] == NULL && t->child[0]->type != t->child[1]->type)
      {
        semanticError(InvalAssign, "", t->lineno);
      }
      break;
    case CallK:
      break;
    default:
      break;
    }
    break;
  case StmtK:
    switch (t->kind.stmt)
    {
    case IfK:
    case IfEK:
    case IterK:
      if (t->child[0] == NULL || t->child[0]->type != Integer)
        semanticError(InvalCond, "", t->lineno);
      break;
    default:
      break;
    }
    break;
  case DeclK:
    switch (t->kind.decl)
    {
    case VarK:
    case VarArrK:
      if (t->type == Void)
      {
        semanticError(VoidVar, t->attr.name, t->lineno);
      }
    default:
      break;
    }
    break;
  default:
    break;
  }
}

static void beforeCheckNode(TreeNode *t)
{
  switch (t->nodekind)
  {
  case DeclK:
    if (t->kind.decl == FuncK)
    {
      funcName = t->attr.name;
    }
    break;
  case StmtK:
    if (t->kind.stmt == CompK)
      currScope = findScope(t->attr.scopeName);
    break;
  default:
    break;
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

  TreeNode *func;
  TreeNode *typeSpec;
  TreeNode *param;
  TreeNode *compStmt;

  typeSpec = newTypeNode(FuncK);
  typeSpec->attr.type = Integer;

  compStmt = newStmtNode(CompK);
  compStmt->child[0] = NULL;
  compStmt->child[1] = NULL;

  func = newDeclNode(FuncK);
  func->type = Integer;
  func->lineno = 0;
  func->attr.name = "input";
  func->child[0] = typeSpec;
  func->child[1] = NULL;
  func->child[2] = compStmt;

  st_insert(currScope->name, "input", Integer, 0, currScope->location++, func);
}

void addOutput()
{

  TreeNode *func;
  TreeNode *typeSpec;
  TreeNode *param;
  TreeNode *compStmt;
  typeSpec = newTypeNode(FuncK);
  typeSpec->attr.type = Void;

  param = newParamNode(SingleParamK);
  param->attr.name = "arg";
  param->type = Integer;
  param->child[0] = newTypeNode(FuncK);
  param->child[0]->attr.type = Integer;

  compStmt = newStmtNode(CompK);
  compStmt->child[0] = NULL;
  compStmt->child[1] = NULL;

  func = newDeclNode(FuncK);
  func->type = Void;
  func->lineno = 0;
  func->attr.name = "output";
  func->child[0] = typeSpec;
  func->child[1] = param;
  func->child[2] = compStmt;

  st_insert(currScope->name, "output", Void, 0, currScope->location++, func);
}