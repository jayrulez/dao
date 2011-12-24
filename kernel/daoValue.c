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

#include"stdlib.h"
#include"string.h"

#include"daoGC.h"
#include"daoType.h"
#include"daoStdtype.h"
#include"daoStream.h"
#include"daoRoutine.h"
#include"daoClass.h"
#include"daoObject.h"
#include"daoNumtype.h"
#include"daoNamespace.h"
#include"daoVmspace.h"
#include"daoParser.h"
#include"daoProcess.h"
#include"daoValue.h"

#ifdef DAO_WITH_NUMARRAY
int DaoArray_Compare( DaoArray *x, DaoArray *y );
#endif

#define float_compare( x, y ) (x==y ? 0 : (x<y ? -1 : 1))

int DaoEnum_Compare( DaoEnum *L, DaoEnum *R )
{
	DaoEnum E;
	DNode *N = NULL;
	DMap *ML = L->etype->mapNames;
	DMap *MR = R->etype->mapNames;
	uchar_t FL = L->etype->flagtype;
	uchar_t FR = R->etype->flagtype;
	char SL = L->etype->name->mbs[0];
	char SR = R->etype->name->mbs[0];
	if( L->etype == R->etype ){
		return L->value == R->value ? 0 : ( L->value < R->value ? -1 : 1 );
	}else if( SL == '$' && SR == '$' && FL == 0 && FR == 0 ){
		return DString_Compare( L->etype->name, R->etype->name );
	}else if( SL == '$' && SR == '$' ){
		if( L->etype->mapNames->size != R->etype->mapNames->size ){
			return (int)L->etype->mapNames->size - (int)R->etype->mapNames->size;
		}else{
			for(N=DMap_First(ML);N;N=DMap_Next(ML,N)){
				if( DMap_Find( MR, N->key.pVoid ) ==0 ) return -1;
			}
			return 0;
		}
	}else if( SL == '$' ){
		E.etype = R->etype;
		E.value = R->value;
		DaoEnum_SetValue( & E, L, NULL );
		return E.value == R->value ? 0 : ( E.value < R->value ? -1 : 1 );
	}else if( SR == '$' ){
		E.etype = L->etype;
		E.value = L->value;
		DaoEnum_SetValue( & E, R, NULL );
		return L->value == E.value ? 0 : ( L->value < E.value ? -1 : 1 );
	}else{
		return L->value == R->value ? 0 : ( L->value < R->value ? -1 : 1 );
	}
}
int DaoTuple_Compare( DaoTuple *lt, DaoTuple *rt )
{
	int i, lb, rb, res;
	if( lt->size < rt->size ) return -1;
	if( lt->size > rt->size ) return 1;

	for(i=0; i<lt->size; i++){
		DaoValue *lv = lt->items[i];
		DaoValue *rv = rt->items[i];
		int lb = lv ? lv->type : 0;
		int rb = rv ? rv->type : 0;
		if( lb == rb && lb == DAO_TUPLE ){
			res = DaoTuple_Compare( (DaoTuple*) lv, (DaoTuple*) rv );
			if( res != 0 ) return res;
		}else if( lb != rb || lb ==0 || lb >= DAO_ARRAY || lb == DAO_COMPLEX ){
			if( lv < rv ){
				return -1;
			}else if( lv > rv ){
				return 1;
			}
		}else{
			res = DaoValue_Compare( lv, rv );
			if( res != 0 ) return res;
		}
	}
	return 0;
}
int DaoList_Compare( DaoList *list1, DaoList *list2 )
{
	DaoValue **d1 = list1->items.items.pValue;
	DaoValue **d2 = list2->items.items.pValue;
	int size1 = list1->items.size;
	int size2 = list2->items.size;
	int min = size1 < size2 ? size1 : size2;
	int i = 0, cmp = 0;
	/* find the first unequal items */
	while( i < min && (cmp = DaoValue_Compare(*d1, *d2)) ==0 ) i++, d1++, d2++;
	if( i < min ) return cmp;
	if( size1 == size2  ) return 0;
	return size1 > size2 ? 1 : -1;
}
int DaoValue_Compare( DaoValue *left, DaoValue *right )
{
	double L, R;
	int res = 0;
	if( left == right ) return 0;
	if( left == NULL || right == NULL ) return left < right ? -1 : 1;
	if( left->type != right->type ){
		res = left->type < right->type ? -1 : 1;
		if( right->type == DAO_TUPLE && right->xTuple.size == 2 ){
			res = DaoValue_Compare( left, right->xTuple.items[0] );
			if( res <= 0 ) return res;
			res = DaoValue_Compare( left, right->xTuple.items[1] );
			if( res >= 0 ) return res;
			return 0;
		}
		if( left->type < DAO_INTEGER || left->type > DAO_DOUBLE ) return res;
		if( right->type < DAO_INTEGER || right->type > DAO_DOUBLE ) return res;
		L = DaoValue_GetDouble( left );
		R = DaoValue_GetDouble( right );
		return L==R ? 0 : ( L<R ? -1 : 1 );
	}
	switch( left->type ){
	case DAO_NONE : return 0;
	case DAO_INTEGER : return left->xInteger.value - right->xInteger.value;
	case DAO_FLOAT   : return float_compare( left->xFloat.value, right->xFloat.value );
	case DAO_DOUBLE  : return float_compare( left->xDouble.value, right->xDouble.value );
	case DAO_LONG    : return DLong_Compare( left->xLong.value, right->xLong.value );
	case DAO_STRING  : return DString_Compare( left->xString.data, right->xString.data );
	case DAO_ENUM    : return DaoEnum_Compare( & left->xEnum, & right->xEnum );
	case DAO_TUPLE   : return DaoTuple_Compare( & left->xTuple, & right->xTuple );
	case DAO_LIST    : return DaoList_Compare( & left->xList, & right->xList );
	case DAO_CTYPE :
	case DAO_CDATA : return (int)( (size_t)left->xCdata.data - (size_t)right->xCdata.data ); 
#ifdef DAO_WITH_NUMARRAY
	case DAO_ARRAY   : return DaoArray_Compare( & left->xArray, & right->xArray );
#endif
	}
	return left < right ? -1 : (left > right); /* needed for map */
}
int DaoValue_IsZero( DaoValue *self )
{
	if( self == NULL ) return 1;
	switch( self->type ){
	case DAO_NONE : return 1;
	case DAO_INTEGER : return self->xInteger.value == 0;
	case DAO_FLOAT  : return self->xFloat.value == 0.0;
	case DAO_DOUBLE : return self->xDouble.value == 0.0;
	case DAO_COMPLEX : return self->xComplex.value.real == 0.0 && self->xComplex.value.imag == 0.0;
	case DAO_LONG : return self->xLong.value->size == 0 && self->xLong.value->data[0] ==0;
	case DAO_ENUM : return self->xEnum.value == 0;
	}
	return 0;
}
long_t DString_ToInteger( DString *self )
{
	if( self->mbs ) return strtoll( self->mbs, NULL, 0 );
	return wcstoll( self->wcs, NULL, 0 );
}
double DString_ToDouble( DString *self )
{
	if( self->mbs ) return strtod( self->mbs, 0 );
	return wcstod( self->wcs, 0 );
}
long_t DaoValue_GetInteger( DaoValue *self )
{
	switch( self->type ){
	case DAO_INTEGER : return self->xInteger.value;
	case DAO_FLOAT   : return self->xFloat.value;
	case DAO_DOUBLE  : return self->xDouble.value;
	case DAO_COMPLEX : return self->xComplex.value.real;
	case DAO_LONG    : return DLong_ToInteger( self->xLong.value );
	case DAO_STRING  : return DString_ToInteger( self->xString.data );
	case DAO_ENUM    : return self->xEnum.value;
	default : break;
	}
	return 0;
}
float DaoValue_GetFloat( DaoValue *self )
{
	return (float) DaoValue_GetDouble( self );
}
double DaoValue_GetDouble( DaoValue *self )
{
	DString *str;
	switch( self->type ){
	case DAO_NONE    : return 0;
	case DAO_INTEGER : return self->xInteger.value;
	case DAO_FLOAT   : return self->xFloat.value;
	case DAO_DOUBLE  : return self->xDouble.value;
	case DAO_COMPLEX : return self->xComplex.value.real;
	case DAO_LONG    : return DLong_ToDouble( self->xLong.value );
	case DAO_STRING  : return DString_ToDouble( self->xString.data );
	case DAO_ENUM    : return self->xEnum.value;
	default : break;
	}
	return 0.0;
}
int DaoValue_IsNumber( DaoValue *self )
{
	if( self->type >= DAO_INTEGER && self->type <= DAO_DOUBLE ) return 1;
	return 0;
}
static void DaoValue_BasicPrint( DaoValue *self, DaoProcess *proc, DaoStream *stream, DMap *cycData )
{
	DaoType *type = DaoNamespace_GetType( proc->activeNamespace, self );
	if( self->type <= DAO_STREAM )
		DaoStream_WriteMBS( stream, coreTypeNames[ self->type ] );
	else
		DaoStream_WriteMBS( stream, DaoValue_GetTyper( self )->name );
	if( self->type == DAO_NONE ) return;
	if( self->type == DAO_TYPE ){
		DaoStream_WriteMBS( stream, "<" );
		DaoStream_WriteMBS( stream, self->xType.name->mbs );
		DaoStream_WriteMBS( stream, ">" );
	}
	DaoStream_WriteMBS( stream, "_" );
	DaoStream_WriteInt( stream, self->type );
	DaoStream_WriteMBS( stream, "_" );
	DaoStream_WritePointer( stream, self );
	if( self->type < DAO_STREAM ) return;
	if( type && self == type->value ) DaoStream_WriteMBS( stream, "[default]" );
}
void DaoValue_Print( DaoValue *self, DaoProcess *proc, DaoStream *stream, DMap *cycData )
{
	DString *name;
	DaoTypeBase *typer;
	DMap *cd = cycData;
	if( self == NULL ){
		DaoStream_WriteMBS( stream, "none[0x0]" );
		return;
	}
	if( cycData == NULL ) cycData = DMap_New(0,0);
	switch( self->type ){
	case DAO_INTEGER :
		DaoStream_WriteInt( stream, self->xInteger.value ); break;
	case DAO_FLOAT   :
		DaoStream_WriteFloat( stream, self->xFloat.value ); break;
	case DAO_DOUBLE  :
		DaoStream_WriteFloat( stream, self->xDouble.value ); break;
	case DAO_COMPLEX :
		DaoStream_WriteFloat( stream, self->xComplex.value.real );
		if( self->xComplex.value.imag >= -0.0 ) DaoStream_WriteMBS( stream, "+" );
		DaoStream_WriteFloat( stream, self->xComplex.value.imag );
		DaoStream_WriteMBS( stream, "$" );
		break;
	case DAO_LONG :
		name = DString_New(1);
		DLong_Print( self->xLong.value, name );
		DaoStream_WriteString( stream, name );
		DString_Delete( name );
		break;
	case DAO_ENUM  :
		name = DString_New(1);
		DaoEnum_MakeName( & self->xEnum, name );
		DaoStream_WriteMBS( stream, name->mbs );
		DaoStream_WriteMBS( stream, "=" );
		DaoStream_WriteInt( stream, self->xEnum.value );
		DString_Delete( name );
		break;
	case DAO_STRING  :
		DaoStream_WriteString( stream, self->xString.data ); break;
	default :
		typer = DaoVmSpace_GetTyper( self->type );
		if( typer->core->Print == DaoValue_Print ){
			DaoValue_BasicPrint( self, proc, stream, cycData );
			break;
		}
		typer->core->Print( self, proc, stream, cycData );
		break;
	}
	if( cycData != cd ) DMap_Delete( cycData );
}
complex16 DaoValue_GetComplex( DaoValue *self )
{
	complex16 com = { 0.0, 0.0 };
	switch( self->type ){
	case DAO_INTEGER : com.real = self->xInteger.value; break;
	case DAO_FLOAT   : com.real = self->xFloat.value; break;
	case DAO_DOUBLE  : com.real = self->xDouble.value; break;
	case DAO_COMPLEX : com = self->xComplex.value; break;
	default : break;
	}
	return com;
}
DLong* DaoValue_GetLong( DaoValue *self, DLong *lng )
{
	switch( self->type ){
	case DAO_INTEGER : DLong_FromInteger( lng, DaoValue_GetInteger( self ) ); break;
	case DAO_FLOAT   : 
	case DAO_DOUBLE  : DLong_FromDouble( lng, DaoValue_GetDouble( self ) ); break;
	case DAO_COMPLEX : DLong_FromDouble( lng, self->xComplex.value.real ); break;
	case DAO_LONG    : DLong_Move( lng, self->xLong.value ); break;
	case DAO_STRING  : DLong_FromString( lng, self->xString.data ); break;
	default : break; /* TODO list array? */
	}
	return lng;
}
DString* DaoValue_GetString( DaoValue *self, DString *str )
{
	char chs[100] = {0};
	DString_Clear( str );
	switch( self->type ){
	case DAO_INTEGER : sprintf( chs, ( sizeof(dint) == 4 )? "%li" : "%lli", self->xInteger.value ); break;
	case DAO_FLOAT   : sprintf( chs, "%g", self->xFloat.value ); break;
	case DAO_DOUBLE  : sprintf( chs, "%g", self->xDouble.value ); break;
	case DAO_COMPLEX : sprintf( chs, ( self->xComplex.value.imag < 0) ? "%g%g$" : "%g+%g$", self->xComplex.value.real, self->xComplex.value.imag ); break;
	case DAO_LONG  : DLong_Print( self->xLong.value, str ); break;
	case DAO_ENUM : DaoEnum_MakeName( & self->xEnum, str ); break;
	case DAO_STRING : DString_Assign( str, self->xString.data ); break;
	default : break;
	}
	if( self->type <= DAO_COMPLEX ) DString_SetMBS( str, chs );
	return str;
}
void DaoValue_MarkConst( DaoValue *self )
{
	DMap *map;
	DNode *it;
	int i, n;
	if( self == NULL || (self->xNone.trait & DAO_DATA_CONST) ) return;
	self->xNone.trait |= DAO_DATA_CONST;
	switch( self->type ){
	case DAO_LIST :
		for(i=0; i<self->xList.items.size; i++)
			DaoValue_MarkConst( self->xList.items.items.pValue[i] );
		break;
	case DAO_TUPLE :
		for(i=0; i<self->xTuple.size; i++)
			DaoValue_MarkConst( self->xTuple.items[i] );
		break;
	case DAO_MAP :
		map = self->xMap.items;
		for(it=DMap_First( map ); it != NULL; it = DMap_Next(map, it) ){
			DaoValue_MarkConst( it->key.pValue );
			DaoValue_MarkConst( it->value.pValue );
		}
		break;
	case DAO_OBJECT :
		n = self->xObject.defClass->objDataDefault->size;
		for(i=1; i<n; i++) DaoValue_MarkConst( self->xObject.objValues[i] );
		for(i=0; i<self->xObject.baseCount; i++){
			DaoValue *obj = (DaoValue*) self->xObject.parents[i];
			if( obj == NULL || obj->type != DAO_OBJECT ) continue;
			DaoValue_MarkConst( obj );
		}
		break;
	default : break;
	}
}

void DaoValue_Clear( DaoValue **self )
{
	DaoValue *value = *self;
	*self = NULL;
	GC_DecRC( value );
}

void DaoObject_CopyData( DaoObject *self, DaoObject *from, DaoProcess *ctx, DMap *cyc );

DaoValue* DaoValue_SimpleCopyWithType( DaoValue *self, DaoType *tp )
{
	DaoEnum *e;
	size_t i;

	if( self == NULL ) return dao_none_value;
#ifdef DAO_WITH_NUMARRAY
	if( self->type == DAO_ARRAY && self->xArray.original ){
		DaoArray_Sliced( (DaoArray*)self );
		return self;
	}
#endif
	if( self->xNone.trait & DAO_DATA_NOCOPY ) return self;
	if( tp && tp->tid >= DAO_INTEGER && tp->tid <= DAO_DOUBLE ){
		double value = DaoValue_GetDouble( self );
		switch( tp->tid ){
		case DAO_INTEGER : return (DaoValue*) DaoInteger_New( (dint) value );
		case DAO_FLOAT   : return (DaoValue*) DaoFloat_New( value );
		case DAO_DOUBLE  : return (DaoValue*) DaoDouble_New( value );
		}
	}
	switch( self->type ){
	case DAO_NONE : return self;
	case DAO_INTEGER : return (DaoValue*) DaoInteger_New( self->xInteger.value );
	case DAO_FLOAT   : return (DaoValue*) DaoFloat_New( self->xFloat.value );
	case DAO_DOUBLE  : return (DaoValue*) DaoDouble_New( self->xDouble.value );
	case DAO_COMPLEX : return (DaoValue*) DaoComplex_New( self->xComplex.value );
	case DAO_LONG    : return (DaoValue*) DaoLong_Copy( & self->xLong );
	case DAO_STRING  : return (DaoValue*) DaoString_Copy( & self->xString );
	case DAO_ENUM    : return (DaoValue*) DaoEnum_Copy( & self->xEnum, tp );
	}
	if( (self->xNone.trait & DAO_DATA_CONST) == 0 ) return self;
	switch( self->type ){
	case DAO_LIST :
		{
			DaoList *list = (DaoList*) self;
			DaoList *copy = DaoList_New();
			/* no detailed checking of type matching, must be ensured by caller */
			copy->unitype = (tp && tp->tid == DAO_LIST) ? tp : list->unitype;
			GC_IncRC( copy->unitype );
			DArray_Resize( & copy->items, list->items.size, NULL );
			for(i=0; i<list->items.size; i++)
				DaoList_SetItem( copy, list->items.items.pValue[i], i );
			return (DaoValue*)copy;
		}
	case DAO_MAP :
		{
			DaoMap *map = (DaoMap*) self;
			DaoMap *copy = DaoMap_New( map->items->hashing );
			DNode *node = DMap_First( map->items );
			copy->unitype = (tp && tp->tid == DAO_MAP) ? tp : map->unitype;
			GC_IncRC( copy->unitype );
			for( ; node !=NULL; node = DMap_Next(map->items, node ))
				DaoMap_Insert( copy, node->key.pValue, node->value.pValue );
			return (DaoValue*)copy;
		}
	case DAO_TUPLE :
		{
			DaoTuple *tuple = (DaoTuple*) self;
			DaoTuple *copy = DaoTuple_New( tuple->size );
			copy->subtype = tuple->subtype;
			copy->unitype = (tp && tp->tid == DAO_TUPLE) ? tp : tuple->unitype;
			GC_IncRC( copy->unitype );
			for(i=0; i<tuple->size; i++) DaoTuple_SetItem( copy, tuple->items[i], i );
			return (DaoValue*) copy;
		}
#ifdef DAO_WITH_NUMARRAY
	case DAO_ARRAY :
		{
			DaoArray *array = (DaoArray*) self;
			DaoArray *copy = DaoArray_New( array->etype );
			copy->unitype = array->unitype;
			if( tp && tp->tid == DAO_ARRAY && tp->nested->size ){
				int nt = tp->nested->items.pType[0]->tid;
				if( nt >= DAO_INTEGER && nt <= DAO_COMPLEX ){
					copy->unitype = tp;
					copy->etype = nt;
				}
			}
			GC_IncRC( copy->unitype );
			DaoArray_ResizeArray( copy, array->dims, array->ndim );
			DaoArray_CopyArray( copy, array );
			return (DaoValue*) copy;
		}
#endif
	default : break;
	}
	if( self->type == DAO_OBJECT ){
		DaoObject *s = (DaoObject*) self;
		DaoObject *t = DaoObject_New( s->defClass );
		DMap *cyc = DHash_New(0,0);
		DaoObject_CopyData( t, s, NULL, cyc );
		DMap_Delete( cyc );
		return (DaoValue*) t;
	}
	return self;
}
DaoValue* DaoValue_SimpleCopy( DaoValue *self )
{
	return DaoValue_SimpleCopyWithType( self, NULL );
}

static void DaoValue_CopyExt( DaoValue *src, DaoValue **dest, int copy )
{
	DaoValue *dest2 = *dest;
	if( src == dest2 ) return;
	if( dest2 && dest2->xNone.refCount >1 ){
		GC_DecRC( dest2 );
		*dest = dest2 = NULL;
	}
	if( dest2 == NULL ){
		if( copy ) src = DaoValue_SimpleCopyWithType( src, NULL );
		GC_IncRC( src );
		*dest = src;
		return;
	}
	if( src->type != dest2->type || src->type > DAO_ENUM ){
		if( copy ) src = DaoValue_SimpleCopyWithType( src, NULL );
		GC_ShiftRC( src, dest2 );
		*dest = src;
		return;
	}
	switch( src->type ){
	case DAO_ENUM    : 
		DaoEnum_SetType( & dest2->xEnum, src->xEnum.etype );
		DaoEnum_SetValue( & dest2->xEnum, & src->xEnum, NULL );
		break;
	case DAO_INTEGER : dest2->xInteger.value = src->xInteger.value; break;
	case DAO_FLOAT   : dest2->xFloat.value = src->xFloat.value; break;
	case DAO_DOUBLE  : dest2->xDouble.value = src->xDouble.value; break;
	case DAO_COMPLEX : dest2->xComplex.value = src->xComplex.value; break;
	case DAO_LONG    : DLong_Move( dest2->xLong.value, src->xLong.value ); break;
	case DAO_STRING  : DString_Assign( dest2->xString.data, src->xString.data ); break;
	}
}
void DaoValue_Copy( DaoValue *src, DaoValue **dest )
{
	DaoValue_CopyExt( src, dest, 1 );
}
void DaoValue_SetType( DaoValue *to, DaoType *tp )
{
	DaoType *tp2;
	DNode *it;
	if( to->type != tp->tid && tp->tid != DAO_ANY ) return;
	/* XXX compatible types? list<int> list<float> */
	switch( to->type ){
	case DAO_LIST :
		/* v : any = {}, v->unitype should be list<any> */
		if( tp->tid == DAO_ANY ) tp = dao_list_any;
		tp2 = to->xList.unitype;
		if( tp2 && !(tp2->attrib & DAO_TYPE_EMPTY) ) break;
		GC_ShiftRC( tp, to->xList.unitype );
		to->xList.unitype = tp;
		break;
	case DAO_MAP :
		if( tp->tid == DAO_ANY ) tp = dao_map_any;
		tp2 = to->xMap.unitype;
		if( tp2 && !(tp2->attrib & DAO_TYPE_EMPTY) ) break;
		GC_ShiftRC( tp, to->xMap.unitype );
		to->xMap.unitype = tp;
		break;
	case DAO_TUPLE :
		tp2 = to->xTuple.unitype;
		if( tp->tid == DAO_ANY ) break;
		if( tp->nested->size ==0 ) break; /* not to the generic tuple type */
		if( tp2 == NULL || tp2->mapNames == NULL || tp2->mapNames->size ==0 ){
			GC_ShiftRC( tp, to->xTuple.unitype );
			to->xTuple.unitype = tp;
			break;
		}
		if( tp->mapNames == NULL || tp->mapNames->size ) break;
		for(it=DMap_First(tp2->mapNames); it!=NULL; it=DMap_Next(tp2->mapNames, it)){
			if( DMap_Find( tp->mapNames, it->key.pVoid ) == NULL ) break;
		}
		if( it ) break;
		GC_ShiftRC( tp, to->xTuple.unitype );
		to->xTuple.unitype = tp;
		break;
#ifdef DAO_WITH_NUMARRAY
	case DAO_ARRAY :
		/*XXX array<int> array<float>*/
		if( tp->tid == DAO_ANY ) tp = dao_array_any;
		tp2 = to->xArray.unitype;
		if( tp2 && !(tp2->attrib & DAO_TYPE_EMPTY) ) break;
		GC_ShiftRC( tp, to->xArray.unitype );
		to->xArray.unitype = tp;
		break;
#endif
	default : break;
	}
}
static int DaoValue_MoveVariant( DaoValue *src, DaoValue **dest, DaoType *tp )
{
	DaoType *itp = NULL;
	int j, k, mt = 0;
	for(j=0; j<tp->nested->size; j++){
		k = DaoType_MatchValue( tp->nested->items.pType[j], src, NULL );
		if( k > mt ){
			itp = tp->nested->items.pType[j];
			mt = k;
			if( mt == DAO_MT_EQ ) break;
		}
	}
	if( itp == NULL ) return 0;
	return DaoValue_Move( src, dest, itp );
}
int DaoValue_Move4( DaoValue *src, DaoValue **dest, DaoType *tp )
{
	int tm = 1;
	if( (tp->tid == DAO_OBJECT || tp->tid == DAO_CDATA) && src->type == DAO_OBJECT){
		if( src->xObject.defClass != & tp->aux->xClass ){
			src = DaoObject_MapThisObject( src->xObject.rootObject, tp );
			tm = (src != NULL);
		}
	}else if( src->type == DAO_CLASS && tp->tid == DAO_CLASS && src->xClass.typeHolders ){
		if( DMap_Find( src->xClass.instanceClasses, tp->aux->xClass.className ) ){
			src = tp->aux;
			tm = DAO_MT_SUB;
		}
	}else{
		tm = DaoType_MatchValue( tp, src, NULL );
	}
#if 0
	if( tm ==0 ){
		printf( "tp = %p; src = %p, type = %i\n", tp, src, src->type );
		printf( "tp: %s %i %i\n", tp->name->mbs, tp->tid, tm );
		if( src->type == DAO_LIST ) printf( "%s\n", src->xList.unitype->name->mbs );
		if( src->type == DAO_TUPLE ) printf( "%p\n", src->xTuple.unitype );
	}
	printf( "src->type = %p %s %i\n", src, tp->name->mbs, tm );
#endif
	if( tm == 0 ) return 0;
	/* composite known types must match exactly. example,
	 * where it will not work if composite types are allowed to match loosely.
	 * d : list<list<int>> = {};
	 * e : list<float> = { 1.0 };
	 * d.append( e );
	 *
	 * but if d is of type list<list<any>>,
	 * the matching do not necessary to be exact.
	 */
	src = DaoValue_SimpleCopyWithType( src, tp );
	GC_ShiftRC( src, *dest );
	*dest = src;
	if( src->type == DAO_TUPLE && src->xTuple.unitype != tp && tm >= DAO_MT_SIM ){
		DaoTuple *tuple;
		DaoType **item_types = tp->nested->items.pType;
		DaoType *totype = src->xTuple.unitype;
		DaoValue **data = src->xTuple.items;
		DMap *names = totype ? totype->mapNames : NULL;
		DNode *node, *search;
		int i, T = tp->nested->size;
		/* auto-cast tuple type, on the following conditions:
		 * (1) the item values of "dest" must match exactly to the item types of "tp";
		 * (2) "tp->mapNames" must contain "(*dest)->xTuple.unitype->mapNames"; */
		if( src->xTuple.unitype == NULL ){
			GC_IncRC( tp );
			src->xTuple.unitype = tp;
			return 1;
		}
		for(i=0; i<T; i++){
			DaoType *it = item_types[i];
			if( it->tid == DAO_PAR_NAMED ) it = & it->aux->xType;
			tm = DaoType_MatchValue( it, data[i], NULL );
			if( tm < DAO_MT_SIM ) return 1;
		}
		/* casting is not necessary if the tuple's field names are a superset of the
		 * field names of the target type: */
		if( tp->mapNames == NULL || tp->mapNames->size ==0 ) return 1;
		if( names ){
			int count = 0;
			for(node=DMap_First(names); node; node=DMap_Next(names,node)){
				search = DMap_Find( tp->mapNames, node->key.pVoid );
				if( search && node->value.pInt != search->value.pInt ) return 0;
				count += search != NULL;
			}
			/* be superset of the field names of the target type: */
			if( count == tp->mapNames->size ) return 1;
		}
		tuple = DaoTuple_New( T );
		for(i=0; i<T; i++){
			DaoType *it = item_types[i];
			if( it->tid == DAO_PAR_NAMED ) it = & it->aux->xType;
			DaoValue_Move( data[i], tuple->items+i, it );
		}
		GC_IncRC( tp );
		tuple->unitype = tp;
		GC_ShiftRC( tuple, *dest );
		*dest = (DaoValue*) tuple;
	}else if( tp && ! ( tp->attrib & DAO_TYPE_EMPTY ) && tp->tid == src->type ){
#if 0
		//int defed = DString_FindChar( tp->name, '@', 0 ) == MAXSIZE;
		//defed &= DString_FindChar( tp->name, '?', 0 ) == MAXSIZE;
#endif
		DaoValue_SetType( src, tp );
	}
	return 1;
}
int DaoValue_Move( DaoValue *S, DaoValue **D, DaoType *T )
{
	DaoValue *D2 = *D;
	if( S == D2 ) return 1;
	if( S == NULL ){
		GC_DecRC( *D );
		*D = NULL;
		return 0;
	}
	if( T == NULL ){
		DaoValue_Copy( S, D );
		return 1;
	}
	switch( T->tid ){
	case DAO_NONE :
	case DAO_INITYPE :
		DaoValue_Copy( S, D );
		return 1;
	case DAO_ANY :
		DaoValue_Copy( S, D );
		DaoValue_SetType( *D, T );
		return 1;
	case DAO_VALTYPE :
		if( DaoValue_Compare( S, T->aux ) !=0 ) return 0;
		DaoValue_Copy( S, D );
		return 1;
	case DAO_VARIANT :
		return DaoValue_MoveVariant( S, D, T );
	default : break;
	}
	if( D2 && D2->xNone.refCount >1 ){
		GC_DecRC( D2 );
		*D = D2 = NULL;
	}
	if( D2 == NULL || D2->type != T->tid ) return DaoValue_Move4( S, D, T );

	switch( (S->type << 8) | T->tid ){
	case (DAO_STRING<<8)|DAO_STRING :
		DString_Assign( D2->xString.data, S->xString.data );
		break;
	case (DAO_ENUM<<8)|DAO_ENUM :
		DaoEnum_SetType( & D2->xEnum, T );
		DaoEnum_SetValue( & D2->xEnum, & S->xEnum, NULL );
		break;
	case (DAO_INTEGER<<8)|DAO_INTEGER : D2->xInteger.value = S->xInteger.value; break;
	case (DAO_INTEGER<<8)|DAO_FLOAT   : D2->xFloat.value   = S->xInteger.value; break;
	case (DAO_INTEGER<<8)|DAO_DOUBLE  : D2->xDouble.value  = S->xInteger.value; break;
	case (DAO_FLOAT  <<8)|DAO_INTEGER : D2->xInteger.value = S->xFloat.value; break;
	case (DAO_FLOAT  <<8)|DAO_FLOAT   : D2->xFloat.value   = S->xFloat.value; break;
	case (DAO_FLOAT  <<8)|DAO_DOUBLE  : D2->xDouble.value  = S->xFloat.value; break;
	case (DAO_DOUBLE <<8)|DAO_INTEGER : D2->xInteger.value = S->xDouble.value; break;
	case (DAO_DOUBLE <<8)|DAO_FLOAT   : D2->xFloat.value   = S->xDouble.value; break;
	case (DAO_DOUBLE <<8)|DAO_DOUBLE  : D2->xDouble.value  = S->xDouble.value; break;
	case (DAO_COMPLEX<<8)|DAO_COMPLEX : D2->xComplex.value = S->xComplex.value; break;
	case (DAO_LONG<<8)|DAO_LONG : DLong_Move( D2->xLong.value, S->xLong.value ); break;
	default : return DaoValue_Move4( S, D, T );
	}
	return 1;
}
int DaoValue_Move2( DaoValue *S, DaoValue **D, DaoType *T )
{
	int rc = DaoValue_Move( S, D, T );
	if( rc == 0 || T == NULL ) return rc;
	if( S->type <= DAO_STREAM || S->type != T->tid ) return rc;
	if( S->type == DAO_CDATA && S->xCdata.subtype != DAO_CDATA_DAO ){
		if( S->xCdata.data == NULL ) rc = 0;
	}else{
		if( S == T->value ) rc = 0;
	}
	return rc;
}


int DaoValue_Type( DaoValue *self )
{
	return self->type;
}
DaoValue* DaoValue_NewNone()
{
	DaoNone *res = DaoNone_New();
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewInteger( dint v )
{
	DaoInteger *res = DaoInteger_New( v );
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewFloat( float v )
{
	DaoFloat *res = DaoFloat_New( v );
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewDouble( double v )
{
	DaoDouble *res = DaoDouble_New( v );
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewComplex( complex16 v )
{
	DaoComplex *res = DaoComplex_New( v );
	GC_IncRC( res );
	return (DaoValue*) res;
}

DaoValue* DaoValue_NewMBString( const char *s, size_t n )
{
	DaoString *res = DaoString_New(1);
	if( n ){
		DString_SetDataMBS( res->data, s, n );
	}else{
		DString_SetMBS( res->data, s );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewWCString( const wchar_t *s, size_t n )
{
	DaoString *res = DaoString_New(0);
	if( n ){
		DString_Resize( res->data, n );
		memcpy( res->data->wcs, s, n * sizeof(wchar_t) );
	}else{
		DString_SetWCS( res->data, s );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewList()
{
	DaoList *res = DaoList_New();
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMap( int hashing )
{
	DaoMap *res = DaoMap_New( hashing );
	GC_IncRC( res );
	return (DaoValue*) res;
}
#ifdef DAO_WITH_NUMARRAY
DaoValue* DaoValue_NewArray( int type )
{
	DaoArray *res = DaoArray_New( type );
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorSB( signed char *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorSB( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorUB( unsigned char *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorUB( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorSS( signed short *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorSS( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorUS( unsigned short *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorUS( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorSI( signed int *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorSI( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorI( dint *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorI( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorF( float *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_FLOAT );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorF( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewVectorD( double *s, size_t n )
{
	DaoArray *res = DaoArray_New( DAO_DOUBLE );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetVectorD( res, s, n );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
extern void DaoArray_SetMatrixUB( DaoArray *self, unsigned char **mat, int N, int M );
extern void DaoArray_SetMatrixUS( DaoArray *self, unsigned short **mat, int N, int M );
extern void DaoArray_SetMatrixUI( DaoArray *self, unsigned int **mat, int N, int M );
DaoValue* DaoValue_NewMatrixSB( signed char **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixSB( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixUB( unsigned char **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixUB( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixSS( signed short **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixSS( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixUS( unsigned short **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixUS( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixSI( signed int **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixSI( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixUI( unsigned int **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixUI( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixI( dint **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_INTEGER );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixI( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixF( float **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_FLOAT );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixF( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewMatrixD( double **s, size_t n, size_t m )
{
	DaoArray *res = DaoArray_New( DAO_DOUBLE );
	if( s && n == 0 ){
		DaoArray_UseData( res, s );
	}else if( s ){
		DaoArray_SetMatrixD( res, s, n, m );
	}
	GC_IncRC( res );
	return (DaoValue*) res;
}
DaoValue* DaoValue_NewBuffer( void *p, size_t n )
{
	DaoArray *res = DaoArray_New(0);
	DaoArray_SetBuffer( res, p, n );
	GC_IncRC( res );
	return (DaoValue*) res;
}
#else
DaoValue* DaoValue_NewArray( int type )
{
	printf( "Error: numeric array is disabled!\n" );
	return NULL;
}
DaoValue* DaoValue_NewVectorB( char *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorUB( unsigned char *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorS( short *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorUS( unsigned short *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorI( dint *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorUI( unsigned int *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorF( float *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewVectorD( double *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixB( signed char **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixUB( unsigned char **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixS( short **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixUS( unsigned short **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixI( dint **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixUI( unsigned int **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixF( float **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewMatrixD( double **s, size_t n, size_t m )
{
	return DaoValue_NewArray( 0 );
}
DaoValue* DaoValue_NewBuffer( void *s, size_t n )
{
	return DaoValue_NewArray( 0 );
}
#endif
DaoValue* DaoValue_NewStream( FILE *f )
{
	DaoStream *self = DaoStream_New();
	DaoStream_SetFile( self, f );
	return (DaoValue*) self;
}
DaoValue* DaoValue_NewCdata( DaoTypeBase *typer, void *data )
{
	return (DaoValue*) DaoCdata_New( typer, data );
}
DaoValue* DaoValue_WrapCdata( DaoTypeBase *typer, void *data )
{
	return (DaoValue*) DaoCdata_Wrap( typer, data );
}

DaoInteger* DaoValue_CastInteger( DaoValue *self )
{
	if( self == NULL || self->type != DAO_INTEGER ) return NULL;
	return (DaoInteger*) self;
}
DaoFloat* DaoValue_CastFloat( DaoValue *self )
{
	if( self == NULL || self->type != DAO_FLOAT ) return NULL;
	return (DaoFloat*) self;
}
DaoDouble* DaoValue_CastDouble( DaoValue *self )
{
	if( self == NULL || self->type != DAO_DOUBLE ) return NULL;
	return (DaoDouble*) self;
}
DaoComplex* DaoValue_CastComplex( DaoValue *self )
{
	if( self == NULL || self->type != DAO_COMPLEX ) return NULL;
	return (DaoComplex*) self;
}
DaoLong* DaoValue_CastLong( DaoValue *self )
{
	if( self == NULL || self->type != DAO_LONG ) return NULL;
	return (DaoLong*) self;
}
DaoString* DaoValue_CastString( DaoValue *self )
{
	if( self == NULL || self->type != DAO_STRING ) return NULL;
	return (DaoString*) self;
}
DaoEnum* DaoValue_CastEnum( DaoValue *self )
{
	if( self == NULL || self->type != DAO_ENUM ) return NULL;
	return (DaoEnum*) self;
}
DaoArray* DaoValue_CastArray( DaoValue *self )
{
	if( self == NULL || self->type != DAO_ARRAY ) return NULL;
	return (DaoArray*) self;
}
DaoList* DaoValue_CastList( DaoValue *self )
{
	if( self == NULL || self->type != DAO_LIST ) return NULL;
	return (DaoList*) self;
}
DaoMap* DaoValue_CastMap( DaoValue *self )
{
	if( self == NULL || self->type != DAO_MAP ) return NULL;
	return (DaoMap*) self;
}
DaoTuple* DaoValue_CastTuple( DaoValue *self )
{
	if( self == NULL || self->type != DAO_TUPLE ) return NULL;
	return (DaoTuple*) self;
}
DaoStream* DaoValue_CastStream( DaoValue *self )
{
	if( self == NULL || self->type != DAO_STREAM ) return NULL;
	return (DaoStream*) self;
}
DaoObject* DaoValue_CastObject( DaoValue *self )
{
	if( self == NULL || self->type != DAO_OBJECT ) return NULL;
	return (DaoObject*) self;
}
DaoCdata* DaoValue_CastCdata( DaoValue *self )
{
	if( self == NULL || self->type != DAO_CDATA ) return NULL;
	return (DaoCdata*) self;
}
DaoClass* DaoValue_CastClass( DaoValue *self )
{
	if( self == NULL || self->type != DAO_CLASS ) return NULL;
	return (DaoClass*) self;
}
DaoInterface* DaoValue_CastInterface( DaoValue *self )
{
	if( self == NULL || self->type != DAO_INTERFACE ) return NULL;
	return (DaoInterface*) self;
}
DaoRoutine* DaoValue_CastRoutine( DaoValue *self )
{
	if( self == NULL || self->type != DAO_ROUTINE ) return NULL;
	return (DaoRoutine*) self;
}
DaoProcess* DaoValue_CastProcess( DaoValue *self )
{
	if( self == NULL || self->type != DAO_PROCESS ) return NULL;
	return (DaoProcess*) self;
}
DaoNamespace* DaoValue_CastNamespace( DaoValue *self )
{
	if( self == NULL || self->type != DAO_NAMESPACE ) return NULL;
	return (DaoNamespace*) self;
}
DaoType* DaoValue_CastType( DaoValue *self )
{
	if( self == NULL || self->type != DAO_TYPE ) return NULL;
	return (DaoType*) self;
}

dint DaoValue_TryGetInteger( DaoValue *self )
{
	if( self->type != DAO_INTEGER ) return 0;
	return self->xInteger.value;
}
float DaoValue_TryGetFloat( DaoValue *self )
{
	if( self->type != DAO_FLOAT ) return 0.0;
	return self->xFloat.value;
}
double DaoValue_TryGetDouble( DaoValue *self )
{
	if( self->type != DAO_DOUBLE ) return 0.0;
	return self->xDouble.value;
}
dint DaoValue_TryGetEnum(DaoValue *self)
{
	if( self->type != DAO_ENUM ) return 0;
	return self->xEnum.value;
}
complex16 DaoValue_TryGetComplex( DaoValue *self )
{
	complex16 com = {0.0,0.0};
	if( self->type != DAO_COMPLEX ) return com;
	return self->xComplex.value;
}
char* DaoValue_TryGetMBString( DaoValue *self )
{
	if( self->type != DAO_STRING ) return NULL;
	return DString_GetMBS( self->xString.data );
}
wchar_t* DaoValue_TryGetWCString( DaoValue *self )
{
	if( self->type != DAO_STRING ) return NULL;
	return DString_GetWCS( self->xString.data );
}
void* DaoValue_TryCastCdata( DaoValue *self, DaoTypeBase *typer )
{
	if( self->type != DAO_CDATA ) return NULL;
	return DaoCdata_CastData( & self->xCdata, typer );
}
void* DaoValue_TryGetCdata( DaoValue *self )
{
	if( self->type != DAO_CDATA ) return NULL;
	return self->xCdata.data;
}
void** DaoValue_TryGetCdata2( DaoValue *self )
{
	if( self->type != DAO_CDATA ) return NULL;
	return & self->xCdata.data;
}
void DaoValue_ClearAll( DaoValue *v[], int n )
{
	int i;
	for(i=0; i<n; i++) DaoValue_Clear( v + i );
}

