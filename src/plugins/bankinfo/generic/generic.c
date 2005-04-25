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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "generic_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/waitcallback.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>



GWEN_INHERIT(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC)



AB_BANKINFO_PLUGIN *AB_BankInfoPluginGENERIC_new(AB_BANKING *ab,
                                                 GWEN_DB_NODE *db,
                                                 const char *country){
  AB_BANKINFO_PLUGIN *bip;
  AB_BANKINFO_PLUGIN_GENERIC *bde;

  assert(country);
  bip=AB_BankInfoPlugin_new(country);
  GWEN_NEW_OBJECT(AB_BANKINFO_PLUGIN_GENERIC, bde);
  GWEN_INHERIT_SETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                       bip, bde, AB_BankInfoPluginGENERIC_FreeData);

  bde->banking=ab;
  bde->dbData=db;
  bde->country=strdup(country);
  AB_BankInfoPlugin_SetGetBankInfoFn(bip, AB_BankInfoPluginGENERIC_GetBankInfo);
  AB_BankInfoPlugin_SetGetBankInfoByTemplateFn(bip,
                                               AB_BankInfoPluginGENERIC_SearchbyTemplate);

  return bip;
}



void AB_BankInfoPluginGENERIC_FreeData(void *bp, void *p){
  AB_BANKINFO_PLUGIN_GENERIC *bde;

  bde=(AB_BANKINFO_PLUGIN_GENERIC*)p;
  free(bde->country);
  GWEN_FREE_OBJECT(bde);
}



void AB_BankInfoPluginGENERIC__GetDataDir(AB_BANKINFO_PLUGIN *bip,
                                          GWEN_BUFFER *pbuf) {
  AB_BANKINFO_PLUGIN_GENERIC *bde;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  assert(pbuf);
  GWEN_Buffer_AppendString(pbuf, PLUGIN_DATADIR);
  GWEN_Buffer_AppendByte(pbuf, '/');
  GWEN_Buffer_AppendString(pbuf, bde->country);
}



AB_BANKINFO *AB_BankInfoPluginGENERIC__ReadBankInfo(AB_BANKINFO_PLUGIN *bip,
                                                    const char *num){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_BUFFER *pbuf;
  AB_BANKINFO *bi;
  GWEN_BUFFEREDIO *bio;
  int fd;
  GWEN_DB_NODE *dbT;
  GWEN_TYPE_UINT32 pos;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  /* get position */
  assert(strlen(num)==8);
  if (1!=sscanf(num, "%08x", &pos)) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Invalid index");
    return 0;
  }

  /* get path */
  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  AB_BankInfoPluginGENERIC__GetDataDir(bip, pbuf);
  GWEN_Buffer_AppendString(pbuf, "/banks.data");

  /* open file */
  fd=open(GWEN_Buffer_GetStart(pbuf), O_RDONLY | O_EXCL);
  if (fd==-1) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "open(%s): %s",
              GWEN_Buffer_GetStart(pbuf), strerror(errno));
    GWEN_Buffer_free(pbuf);
    return 0;
  }
  /* seek position */
  DBG_VERBOUS(0, "Seeking to %08x (%d)", pos, pos);
  if ((off_t)-1==lseek(fd, pos, SEEK_SET)) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "lseek(%s, "GWEN_TYPE_TMPL_UINT32"): %s",
              GWEN_Buffer_GetStart(pbuf),
              pos,
              strerror(errno));
    close(fd);
    GWEN_Buffer_free(pbuf);
    return 0;
  }

  bio=GWEN_BufferedIO_File_new(fd);
  GWEN_BufferedIO_SetReadBuffer(bio, 0, 512);

  /* read data */
  dbT=GWEN_DB_Group_new("bank");

  if (GWEN_DB_ReadFromStream(dbT, bio,
                             GWEN_DB_FLAGS_DEFAULT |
                             GWEN_DB_FLAGS_STOP_ON_EMPTY_LINE |
                             GWEN_PATH_FLAGS_CREATE_GROUP)) {
    DBG_ERROR(0, "Could not load file \"%s\"",
              GWEN_Buffer_GetStart(pbuf));
    GWEN_DB_Group_free(dbT);
    GWEN_BufferedIO_Abandon(bio);
    GWEN_BufferedIO_free(bio);
    GWEN_Buffer_free(pbuf);
    return 0;
  }

  bi=AB_BankInfo_fromDb(dbT);
  assert(bi);
  GWEN_DB_Group_free(dbT);
  GWEN_BufferedIO_Close(bio);
  GWEN_BufferedIO_free(bio);
  GWEN_Buffer_free(pbuf);

  return bi;
}



AB_BANKINFO *AB_BankInfoPluginGENERIC_GetBankInfo(AB_BANKINFO_PLUGIN *bip,
                                                  const char *branchId,
                                                  const char *bankId){
  return AB_BankInfoPluginGENERIC__SearchbyCode(bip, bankId);
}



AB_BANKINFO *AB_BankInfoPluginGENERIC__SearchbyCode(AB_BANKINFO_PLUGIN *bip,
                                                    const char *bankId){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_BUFFER *pbuf;
  FILE *f;
  char lbuf[512];

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  AB_BankInfoPluginGENERIC__GetDataDir(bip, pbuf);
  GWEN_Buffer_AppendString(pbuf, "/blz.idx");
  f=fopen(GWEN_Buffer_GetStart(pbuf), "r");
  if (!f) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "fopen(%s): %s",
             GWEN_Buffer_GetStart(pbuf),
             strerror(errno));
    GWEN_Buffer_free(pbuf);
    return 0;
  }

  while(!feof(f)) {
    unsigned char *p;

    lbuf[0]=0;
    p=(unsigned char*)fgets(lbuf, sizeof(lbuf), f);
    if (p) {
      char *blz=0;
      char *num=0;
      unsigned int i;

      i=strlen(lbuf);
      if (lbuf[i-1]==10)
        lbuf[i-1]=0;
      blz=(char*)p;
      while(*p && *p!='\t')
        p++;
      assert(*p=='\t');
      *p=0;
      p++;
      num=(char*)p;
      if (strcasecmp(blz, bankId)==0) {
        AB_BANKINFO *bi;

        bi=AB_BankInfoPluginGENERIC__ReadBankInfo(bip, num);
        fclose(f);
        GWEN_Buffer_free(pbuf);
        return bi;
      }
    }
  }
  fclose(f);
  DBG_INFO(AQBANKING_LOGDOMAIN, "Bank %s not found", bankId);
  return 0;
}



int AB_BankInfoPluginGENERIC__AddById(AB_BANKINFO_PLUGIN *bip,
                                      const char *bankId,
                                      AB_BANKINFO_LIST2 *bl){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_BUFFER *pbuf;
  FILE *f;
  char lbuf[512];
  GWEN_TYPE_UINT32 count=0;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  AB_BankInfoPluginGENERIC__GetDataDir(bip, pbuf);
  GWEN_Buffer_AppendString(pbuf, "/blz.idx");
  f=fopen(GWEN_Buffer_GetStart(pbuf), "r");
  if (!f) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "fopen(%s): %s",
             GWEN_Buffer_GetStart(pbuf),
             strerror(errno));
    GWEN_Buffer_free(pbuf);
    return AB_ERROR_NOT_AVAILABLE;
  }

  while(!feof(f)) {
    unsigned char *p;

    lbuf[0]=0;
    p=(unsigned char*)fgets(lbuf, sizeof(lbuf), f);
    if (p) {
      char *blz=0;
      char *num=0;
      unsigned int i;

      i=strlen(lbuf);
      if (lbuf[i-1]==10)
        lbuf[i-1]=0;
      blz=(char*)p;
      while(*p && *p!='\t')
        p++;
      assert(*p=='\t');
      *p=0;
      p++;
      num=(char*)p;
      i=strlen(lbuf);
      if (GWEN_Text_ComparePattern(blz, bankId, 0)!=-1) {
        AB_BANKINFO *bi;

        bi=AB_BankInfoPluginGENERIC__ReadBankInfo(bip, num);
        if (bi) {
          AB_BankInfo_List2_PushBack(bl, bi);
          count++;
        }
      }
    }
  } /* while ! feof */
  fclose(f);
  if (!count) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Bank %s not found", bankId);
    return AB_ERROR_NOT_FOUND;
  }
  return 0;
}



int AB_BankInfoPluginGENERIC__AddByBic(AB_BANKINFO_PLUGIN *bip,
                                       const char *bic,
                                       AB_BANKINFO_LIST2 *bl){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_BUFFER *pbuf;
  FILE *f;
  char lbuf[512];
  GWEN_TYPE_UINT32 count=0;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  AB_BankInfoPluginGENERIC__GetDataDir(bip, pbuf);
  GWEN_Buffer_AppendString(pbuf, "/bic.idx");
  f=fopen(GWEN_Buffer_GetStart(pbuf), "r");
  if (!f) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "fopen(%s): %s",
             GWEN_Buffer_GetStart(pbuf),
             strerror(errno));
    GWEN_Buffer_free(pbuf);
    return AB_ERROR_NOT_AVAILABLE;
  }

  while(!feof(f)) {
    unsigned char *p;

    lbuf[0]=0;
    p=(unsigned char*)fgets(lbuf, sizeof(lbuf), f);
    if (p) {
      char *key=0;
      char *num=0;
      unsigned int i;

      i=strlen(lbuf);
      if (lbuf[i-1]==10)
        lbuf[i-1]=0;
      key=(char*)p;
      while(*p && *p!='\t')
        p++;
      assert(*p=='\t');
      *p=0;
      p++;
      num=(char*)p;
      if (GWEN_Text_ComparePattern(key, bic, 0)!=-1) {
        AB_BANKINFO *bi;

        bi=AB_BankInfoPluginGENERIC__ReadBankInfo(bip, num);
        if (bi) {
          AB_BankInfo_List2_PushBack(bl, bi);
          count++;
        }
      }
    }
  } /* while ! feof */
  fclose(f);
  if (!count) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Bank %s not found", bic);
    return AB_ERROR_NOT_FOUND;
  }
  return 0;
}



int AB_BankInfoPluginGENERIC__AddByNameAndLoc(AB_BANKINFO_PLUGIN *bip,
                                              const char *name,
                                              const char *loc,
                                              AB_BANKINFO_LIST2 *bl){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_BUFFER *pbuf;
  FILE *f;
  char lbuf[512];
  GWEN_TYPE_UINT32 count=0;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  if (name==0)
    name="*";
  if (loc==0)
    loc="*";

  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  AB_BankInfoPluginGENERIC__GetDataDir(bip, pbuf);
  GWEN_Buffer_AppendString(pbuf, "/namloc.idx");
  f=fopen(GWEN_Buffer_GetStart(pbuf), "r");
  if (!f) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "fopen(%s): %s",
             GWEN_Buffer_GetStart(pbuf),
             strerror(errno));
    GWEN_Buffer_free(pbuf);
    DBG_ERROR(AQBANKING_LOGDOMAIN, "namloc index file not available");
    return AB_ERROR_NOT_AVAILABLE;
  }

  while(!feof(f)) {
    unsigned char *p;

    lbuf[0]=0;
    p=(unsigned char*)fgets(lbuf, sizeof(lbuf), f);
    if (p) {
      char *key1=0;
      char *key2=0;
      char *num=0;
      unsigned int i;

      i=strlen(lbuf);
      if (lbuf[i-1]==10)
        lbuf[i-1]=0;
      key1=(char*)p;
      while(*p && *p!='\t')
        p++;
      assert(*p=='\t');
      *p=0;
      p++;
      key2=/* GCC4 pointer-signedness fix: */ (char*) p;
      while(*p && *p!='\t')
        p++;
      assert(*p=='\t');
      *p=0;
      p++;
      num=(char*)p;
      if (GWEN_Text_ComparePattern(key1, name, 0)!=-1 &&
          GWEN_Text_ComparePattern(key2, loc, 0)!=-1) {
        AB_BANKINFO *bi;

        bi=AB_BankInfoPluginGENERIC__ReadBankInfo(bip, num);
	if (bi) {
          AB_BankInfo_List2_PushBack(bl, bi);
          count++;
        }
      }
    }
  } /* while ! feof */
  fclose(f);
  if (!count) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Bank %s/%s not found", name, loc);
    return AB_ERROR_NOT_FOUND;
  }
  return 0;
}



int AB_BankInfoPluginGENERIC__CmpTemplate(AB_BANKINFO *bi,
                                          const AB_BANKINFO *tbi,
                                          GWEN_TYPE_UINT32 flags) {
  const char *s;
  const char *t;
  
  if (flags & AB_BANKINFO_GENERIC__FLAGS_BRANCHID) {
    s=AB_BankInfo_GetBranchId(bi);
    t=AB_BankInfo_GetBranchId(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  
  if (flags & AB_BANKINFO_GENERIC__FLAGS_BANKID) {
    s=AB_BankInfo_GetBankId(bi);
    t=AB_BankInfo_GetBankId(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_BIC) {
    s=AB_BankInfo_GetBic(bi);
    t=AB_BankInfo_GetBic(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_BANKNAME) {
    s=AB_BankInfo_GetBankName(bi);
    t=AB_BankInfo_GetBankName(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_LOCATION) {
    s=AB_BankInfo_GetLocation(bi);
    t=AB_BankInfo_GetLocation(tbi);
    if (!t || !*t)
      t=AB_BankInfo_GetCity(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_ZIPCODE) {
    s=AB_BankInfo_GetZipcode(bi);
    t=AB_BankInfo_GetZipcode(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_REGION) {
    s=AB_BankInfo_GetRegion(bi);
    t=AB_BankInfo_GetRegion(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_PHONE) {
    s=AB_BankInfo_GetPhone(bi);
    t=AB_BankInfo_GetPhone(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_FAX) {
    s=AB_BankInfo_GetFax(bi);
    t=AB_BankInfo_GetFax(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_EMAIL) {
    s=AB_BankInfo_GetEmail(bi);
    t=AB_BankInfo_GetEmail(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }
  if (flags & AB_BANKINFO_GENERIC__FLAGS_WEBSITE) {
    s=AB_BankInfo_GetWebsite(bi);
    t=AB_BankInfo_GetWebsite(tbi);
    if (s && *s)
      if (GWEN_Text_ComparePattern(s, t, 0)==-1)
        return 0;
  }

  return 1;
}



int AB_BankInfoPluginGENERIC_AddByTemplate(AB_BANKINFO_PLUGIN *bip,
                                           AB_BANKINFO *tbi,
                                           AB_BANKINFO_LIST2 *bl,
                                           GWEN_TYPE_UINT32 flags){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_TYPE_UINT32 count=0;
  GWEN_TYPE_UINT32 i=0;
  GWEN_BUFFEREDIO *bio;
  GWEN_BUFFER *pbuf;
  int fd;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  /* get path */
  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  AB_BankInfoPluginGENERIC__GetDataDir(bip, pbuf);
  GWEN_Buffer_AppendString(pbuf, "/banks.data");
  /* open file */
  fd=open(GWEN_Buffer_GetStart(pbuf), O_RDONLY | O_EXCL);
  if (fd==-1) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
              "open(%s): %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    return AB_ERROR_NOT_FOUND;
  }
  bio=GWEN_BufferedIO_File_new(fd);
  GWEN_BufferedIO_SetReadBuffer(bio, 0, 1024);

  while(!GWEN_BufferedIO_CheckEOF(bio)) {
    GWEN_DB_NODE *dbT;
    AB_BANKINFO *bi;
    int pos;

    if (i & ~63) {
      if (GWEN_WaitCallbackProgress(AB_BANKING_PROGRESS_NONE)==
          GWEN_WaitCallbackResult_Abort) {
        DBG_ERROR(AQBANKING_LOGDOMAIN,
                  "Aborted by user");
        GWEN_BufferedIO_Abandon(bio);
        GWEN_BufferedIO_free(bio);
        GWEN_Buffer_free(pbuf);
        return AB_ERROR_USER_ABORT;
      }
    }

    dbT=GWEN_DB_Group_new("bank");
    pos=GWEN_BufferedIO_GetBytesRead(bio);
    if (GWEN_DB_ReadFromStream(dbT, bio,
                               GWEN_DB_FLAGS_DEFAULT |
                               GWEN_DB_FLAGS_STOP_ON_EMPTY_LINE |
                               GWEN_PATH_FLAGS_CREATE_GROUP)) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
                "Could not read from file \"%s\"",
                GWEN_Buffer_GetStart(pbuf));
      GWEN_DB_Group_free(dbT);
      GWEN_BufferedIO_Abandon(bio);
      GWEN_BufferedIO_free(bio);
      GWEN_Buffer_free(pbuf);
      return AB_ERROR_GENERIC;
    }

    bi=AB_BankInfo_fromDb(dbT);
    assert(bi);
    if (AB_BankInfoPluginGENERIC__CmpTemplate(bi, tbi, flags)==1) {
      count++;
      AB_BankInfo_List2_PushBack(bl, bi);
    }
    else {
      AB_BankInfo_free(bi);
    }
    GWEN_DB_Group_free(dbT);
    i++;
  } /* while */

  GWEN_BufferedIO_Close(bio);
  GWEN_BufferedIO_free(bio);
  GWEN_Buffer_free(pbuf);

  if (count==0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "No matching bank found");
    return AB_ERROR_NOT_FOUND;
  }

  return 0;
}



int AB_BankInfoPluginGENERIC_SearchbyTemplate(AB_BANKINFO_PLUGIN *bip,
                                              AB_BANKINFO *tbi,
                                              AB_BANKINFO_LIST2 *bl){
  AB_BANKINFO_PLUGIN_GENERIC *bde;
  GWEN_TYPE_UINT32 flags;
  int rv;
  const char *s;

  assert(bip);
  bde=GWEN_INHERIT_GETDATA(AB_BANKINFO_PLUGIN, AB_BANKINFO_PLUGIN_GENERIC,
                           bip);
  assert(bde);

  /* this is just to speed up often needed tests */
  flags=0;
  s=AB_BankInfo_GetBranchId(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_BRANCHID;
  s=AB_BankInfo_GetBankId(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_BANKID;
  s=AB_BankInfo_GetBic(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_BIC;
  s=AB_BankInfo_GetBankName(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_BANKNAME;
  s=AB_BankInfo_GetLocation(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_LOCATION;
  s=AB_BankInfo_GetStreet(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_STREET;
  s=AB_BankInfo_GetZipcode(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_ZIPCODE;
  s=AB_BankInfo_GetCity(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_CITY;
  s=AB_BankInfo_GetRegion(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_REGION;
  s=AB_BankInfo_GetPhone(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_PHONE;
  s=AB_BankInfo_GetFax(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_FAX;
  s=AB_BankInfo_GetEmail(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_EMAIL;
  s=AB_BankInfo_GetWebsite(tbi);
  if (s && *s)
    flags|=AB_BANKINFO_GENERIC__FLAGS_WEBSITE;

  if (flags==AB_BANKINFO_GENERIC__FLAGS_BIC)
    rv=AB_BankInfoPluginGENERIC__AddByBic(bip,
                                          AB_BankInfo_GetBic(tbi),
                                          bl);
  else if ((flags & ~AB_BANKINFO_GENERIC__FLAGS_BRANCHID)==
           AB_BANKINFO_GENERIC__FLAGS_BANKID)
    rv=AB_BankInfoPluginGENERIC__AddById(bip,
                                         AB_BankInfo_GetBankId(tbi),
                                         bl);
  else if (flags==(AB_BANKINFO_GENERIC__FLAGS_BANKNAME|
                   AB_BANKINFO_GENERIC__FLAGS_LOCATION) ||
           flags==AB_BANKINFO_GENERIC__FLAGS_BANKNAME ||
           flags==AB_BANKINFO_GENERIC__FLAGS_LOCATION) {
    rv=AB_BankInfoPluginGENERIC__AddByNameAndLoc(bip,
                                                 AB_BankInfo_GetBankName(tbi),
                                                 AB_BankInfo_GetLocation(tbi),
                                                 bl);
  }
  else {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "No quick search implemented for these flags (%08x)", flags);
    rv=AB_ERROR_NOT_AVAILABLE;
  }
  if (rv==AB_ERROR_NOT_AVAILABLE) {
    rv=AB_BankInfoPluginGENERIC_AddByTemplate(bip, tbi, bl, flags);
  }

  return rv;
}










