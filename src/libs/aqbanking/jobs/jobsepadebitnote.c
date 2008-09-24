/***************************************************************************
 begin       : Sun Sep 21 2008
 copyright   : (C) 2008 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "jobtransferbase_l.h"
#include "jobsepadebitnote.h"
#include "jobsepadebitnote_be.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>




AB_JOB *AB_JobSepaDebitNote_new(AB_ACCOUNT *a){
  return AB_JobTransferBase_new(AB_Job_TypeSepaDebitNote, a);
}



void AB_JobSepaDebitNote_SetFieldLimits(AB_JOB *j,
					 AB_TRANSACTION_LIMITS *limits){
  AB_JobTransferBase_SetFieldLimits(j, limits);
}



const AB_TRANSACTION_LIMITS *AB_JobSepaDebitNote_GetFieldLimits(AB_JOB *j) {
  return AB_JobTransferBase_GetFieldLimits(j);
}



int AB_JobSepaDebitNote_SetTransaction(AB_JOB *j, const AB_TRANSACTION *t){
  return AB_JobTransferBase_SetTransaction(j, t);
}



const AB_TRANSACTION *AB_JobSepaDebitNote_GetTransaction(const AB_JOB *j){
  return AB_JobTransferBase_GetTransaction(j);
}


