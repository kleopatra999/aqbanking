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


#ifndef AQBANKING_JOBEUTRANSFER_P_H
#define AQBANKING_JOBEUTRANSFER_P_H


#include <aqbanking/job.h>
#include "jobeutransfer_be.h"


AB_JOB *AB_JobEuTransfer_fromDb(AB_ACCOUNT *a, GWEN_DB_NODE *db);
int AB_JobEuTransfer_toDb(const AB_JOB *j, GWEN_DB_NODE *db);


#endif
