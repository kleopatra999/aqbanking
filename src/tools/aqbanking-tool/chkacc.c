/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Tue May 03 2005
 copyright   : (C) 2005 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "globals.h"
#include <gwenhywfar/text.h>
#include <aqbanking/jobsingletransfer.h>



int chkAcc(AB_BANKING *ab,
           GWEN_DB_NODE *dbArgs,
           int argc,
           char **argv) {
  GWEN_DB_NODE *db;
  int rv;
  AB_BANKINFO_CHECKRESULT res;
  const char *country;
  const char *bankId;
  const char *accountId;
  const GWEN_ARGS args[]={
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT,  /* flags */
    GWEN_ArgsTypeChar,             /* type */
    "remoteCountry",               /* name */
    0,                             /* minnum */
    1,                             /* maxnum */
    0,                             /* short option */
    "rcountry",                    /* long option */
    "Specify the remote country code",/* short description */
    "Specify the remote country code" /* long description */
  },
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT,  /* flags */
    GWEN_ArgsTypeChar,             /* type */
    "remoteBankId",                /* name */
    1,                             /* minnum */
    1,                             /* maxnum */
    0,                             /* short option */
    "rbank",                       /* long option */
    "Specify the remote bank code",/* short description */
    "Specify the remote bank code" /* long description */
  },
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
    GWEN_ArgsTypeChar,            /* type */
    "remoteAccountId",                  /* name */
    1,                            /* minnum */
    1,                            /* maxnum */
    0,                            /* short option */
    "raccount",                    /* long option */
    "Specify the remote account number",     /* short description */
    "Specify the remote account number"      /* long description */
  },
  {
    GWEN_ARGS_FLAGS_HELP | GWEN_ARGS_FLAGS_LAST, /* flags */
    GWEN_ArgsTypeInt,             /* type */
    "help",                       /* name */
    0,                            /* minnum */
    0,                            /* maxnum */
    "h",                          /* short option */
    "help",                       /* long option */
    "Show this help screen",      /* short description */
    "Show this help screen"       /* long description */
  }
  };

  db=GWEN_DB_GetGroup(dbArgs, GWEN_DB_FLAGS_DEFAULT, "local");
  rv=GWEN_Args_Check(argc, argv, 1,
                     0 /*GWEN_ARGS_MODE_ALLOW_FREEPARAM*/,
                     args,
                     db);
  if (rv==GWEN_ARGS_RESULT_ERROR) {
    fprintf(stderr, "ERROR: Could not parse arguments\n");
    return 1;
  }
  else if (rv==GWEN_ARGS_RESULT_HELP) {
    GWEN_BUFFER *ubuf;

    ubuf=GWEN_Buffer_new(0, 1024, 0, 1);
    if (GWEN_Args_Usage(args, ubuf, GWEN_ArgsOutTypeTXT)) {
      fprintf(stderr, "ERROR: Could not create help string\n");
      return 1;
    }
    fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }

  country=GWEN_DB_GetCharValue(db, "remoteCountry", 0, "de");
  assert(country);
  bankId=GWEN_DB_GetCharValue(db, "remoteBankId", 0, 0);
  assert(bankId);
  accountId=GWEN_DB_GetCharValue(db, "remoteAccountId", 0, 0);
  assert(bankId);

  rv=AB_Banking_Init(ab);
  if (rv) {
    DBG_ERROR(AQT_LOGDOMAIN, "Error on init (%d)", rv);
    return 2;
  }

  res=AB_Banking_CheckAccount(ab,
                              country,
                              0,
                              bankId,
                              accountId);
  switch(res) {
  case AB_BankInfoCheckResult_NotOk:
    DBG_ERROR(AQT_LOGDOMAIN,
              "Invalid combination of bank code and account number "
              "for remote account");
    return 3;

  case AB_BankInfoCheckResult_UnknownBank:
    DBG_ERROR(AQT_LOGDOMAIN, "Remote bank code is unknown");
    return 4;

  case AB_BankInfoCheckResult_UnknownResult:
    DBG_ERROR(AQT_LOGDOMAIN,
              "Indifferent result for remote account check");
    return 4;

  case AB_BankInfoCheckResult_Ok:
    break;
  default:
    DBG_ERROR(AQT_LOGDOMAIN, "Unknown check result %d", res);
    return 4;
  }

  rv=AB_Banking_Fini(ab);
  if (rv) {
    fprintf(stderr, "ERROR: Error on deinit (%d)\n", rv);
    return 5;
  }

  return 0;
}





