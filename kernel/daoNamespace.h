/*=========================================================================================
  This file is a part of a virtual machine for the Dao programming language.
  Copyright (C) 2006-2011, Fu Limin. Email: fu@daovm.net, limin.fu@yahoo.com

  This software is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License as published by the Free Software Foundation;
  either version 2.1 of the License, or (at your option) any later version.

  This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  =========================================================================================*/

#ifndef DAO_NAMESPACE_H
#define DAO_NAMESPACE_H

#include"daoBase.h"
#include"daoConst.h"
#include"daoString.h"
#include"daoArray.h"
#include"daoMap.h"
#include"daoType.h"

struct DaoCModule
{
	DAO_DATA_COMMON;

	void    *libHandle;

	DArray  *ctypers;

	/* The methods of C types loaded from a C module,
	 * used for the purpose of GC. */
	DArray  *cmethods; /* <DaoFunction*> */
};
DaoCModule* DaoCModule_New();
void DaoCModule_Delete( DaoCModule *self );

struct DaoNameSpace
{
	DAO_DATA_COMMON;

	DaoVmSpace *vmSpace;

	/* Namespaces that should be used to resolve un-resolved symbols: */
	/* 1. vmSpace.nsInternal; */
	/* 2. loaded namespaces by: load name, without "import" or "as" etc. */
	/* No GC, these namespaces are also referenced by ::cstData. */
	DArray *parents; /* DArray<DaoNameSpace*> */

	DMap   *lookupTable; /* <DString*,size_t> */
	DArray *cstDataTable;
	DArray *varDataTable;
	DArray *varTypeTable;
	DArray *nsTable;

	int cstUser;
	int options;

	DArray  *cstData; /* <DaoValue*>, global constants; */
	DArray  *varData; /* <DaoValue*>, global variables; */
	DArray  *varType; /* <DaoType*>, types of global variables */

	DaoRoutine *mainRoutine;
    DaoList    *argParams;

	DArray *mainRoutines; /* stdlib.eval() */
	DArray *definedRoutines; /* for DaoStudio IDE */

	DArray *nsLoaded; /* loaded modules as namespaces */

	DaoCModule *cmodule;

	DMap   *localMacros; /* <DString*,DaoMacro*> */
	DMap   *globalMacros; /* <DString*,DaoMacro*> */
	DMap   *abstypes; /* <DString*,DaoType*> */

	DaoType *udfType1;
	DaoType *udfType2;

	DString *file;
	DString *path;
	DString *name; /* path + file */
	DString *inputs; /* interactive inputs (load statements and some definitions) */
	DArray  *sources;
	DMap    *tokens; /* hash<DString,int> */
	ullong_t time;

	DaoVmProcess *vmpEvalConst;
	DaoRoutine   *routEvalConst;
	DaoType     **tempTypes;
};

DaoNameSpace* DaoNameSpace_New( DaoVmSpace *vms, const char *name );
void DaoNameSpace_Delete( DaoNameSpace *self );

void DaoNameSpace_SetName( DaoNameSpace *self, const char *name );

int DaoNameSpace_FindConst( DaoNameSpace *self, DString *name );
int DaoNameSpace_AddConst( DaoNameSpace *self, DString *name, DaoValue *value, int pm );
void DaoNameSpace_SetConst( DaoNameSpace *self, int index, DaoValue *value );
DaoValue* DaoNameSpace_GetConst( DaoNameSpace *self, int i );

int DaoNameSpace_FindVariable( DaoNameSpace *self, DString *name );
int DaoNameSpace_AddVariable( DaoNameSpace *self, DString *name, DaoValue *var, DaoType *tp, int pm );
int DaoNameSpace_SetVariable( DaoNameSpace *self, int index, DaoValue *var );
DaoValue* DaoNameSpace_GetVariable( DaoNameSpace *self, int i );
DaoType* DaoNameSpace_GetVariableType( DaoNameSpace *self, int i );

void DaoNameSpace_SetData( DaoNameSpace *self, DString *name, DaoValue *value );
DaoValue* DaoNameSpace_GetData( DaoNameSpace *self, DString *name );

DaoClass* DaoNameSpace_FindClass( DaoNameSpace *self, DString *name );
DaoNameSpace* DaoNameSpace_FindNameSpace( DaoNameSpace *self, DString *name );

int DaoNameSpace_AddParent( DaoNameSpace *self, DaoNameSpace *parent );
void DaoNameSpace_Import( DaoNameSpace *self, DaoNameSpace *ns, DArray *varImport );

void DaoNameSpace_AddConstNumbers( DaoNameSpace *self, DaoNumItem *items );

void DaoNameSpace_AddMacro( DaoNameSpace *self, DString *name, DaoMacro *macro, int local );
DaoMacro* DaoNameSpace_FindMacro( DaoNameSpace *self, DString *name );

DaoType* DaoNameSpace_FindType( DaoNameSpace *self, DString *name );
void DaoNameSpace_AddType( DaoNameSpace *self, DString *name, DaoType *tp );
void DaoNameSpace_AddTypeConstant( DaoNameSpace *self, DString *name, DaoType *tp );

DaoType* DaoNameSpace_GetType( DaoNameSpace *self, DaoValue *p );
DaoType* DaoNameSpace_MakeType( DaoNameSpace *self, const char *name,
		uchar_t basic, DaoValue *pb, DaoType *nest[], int N );
DaoType* DaoNameSpace_MakeRoutType( DaoNameSpace *self, DaoType *routype,
		DaoValue *vals[], DaoType *types[], DaoType *retp );

DaoType* DaoNameSpace_MakeValueType( DaoNameSpace *self, DaoValue *value );
DaoType* DaoNameSpace_MakePairType( DaoNameSpace *self, DaoType *first, DaoType *second );
DaoType* DaoNameSpace_MakePairValueType( DaoNameSpace *self, DaoValue *first, DaoValue *second );
DaoTuple* DaoNameSpace_MakePair( DaoNameSpace *self, DaoValue *first, DaoValue *second );

DaoType* DaoNameSpace_MakeEnumType( DaoNameSpace *self, const char *symbols );
DaoType* DaoNameSpace_SymbolTypeAdd( DaoNameSpace *self, DaoType *t1, DaoType *t2, dint *value );
DaoType* DaoNameSpace_SymbolTypeSub( DaoNameSpace *self, DaoType *t1, DaoType *t2, dint *value );

int DaoNameSpace_SetupValues( DaoNameSpace *self, DaoTypeBase *typer );
void DaoMethods_Insert( DMap *methods, DRoutine *rout, DaoType *host );
int DaoNameSpace_SetupMethods( DaoNameSpace *self, DaoTypeBase *typer );
DaoType* DaoNameSpace_SetupType( DaoNameSpace *self, DaoTypeBase *typer );

DaoFunction* DaoNameSpace_ParsePrototype( DaoNameSpace *self, const char *proto, DaoParser *parser );
DaoFunction* DaoNameSpace_MakeFunction( DaoNameSpace *self, const char *proto, DaoParser *parser );

void DaoNameSpace_Backup( DaoNameSpace *self, DaoVmProcess *proc, FILE *fout, int limit );
void DaoNameSpace_Restore( DaoNameSpace *self, DaoVmProcess *proc, FILE *fin );

#endif
