/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef AH_MEDIUM_P_H
#define AH_MEDIUM_P_H

#include "medium_l.h"
#include "mediumctx_l.h"

#include <gwenhywfar/crypttoken.h>


struct AH_MEDIUM {
  GWEN_LIST_ELEMENT(AH_MEDIUM);
  GWEN_INHERIT_ELEMENT(AH_MEDIUM);

  AH_HBCI *hbci;
  char *mediumName;
  char *typeName;
  char *subTypeName;
  char *descriptiveName;
  GWEN_TYPE_UINT32 flags;

  GWEN_TYPE_UINT32 mountCount;
  GWEN_TYPE_UINT32 usage;

  GWEN_CRYPTTOKEN *cryptToken;

  AH_MEDIUM_CTX_LIST *contextList;
  AH_MEDIUM_CTX *currentContext;
  int selected;

};


int AH_Medium__ReadContextsFromToken(AH_MEDIUM *m, GWEN_CRYPTTOKEN *ct);
int AH_Medium__MountCt(AH_MEDIUM *m);
int AH_Medium__ReadKeySpec(AH_MEDIUM *m,
                           GWEN_TYPE_UINT32 kid,
                           GWEN_KEYSPEC **ks);
int AH_Medium__ReadKeySpecs(AH_MEDIUM *m);

int AH_Medium__ResetKey(AH_MEDIUM *m, int kid);


#endif /* AH_MEDIUM_P_H */

