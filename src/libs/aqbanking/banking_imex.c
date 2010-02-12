/***************************************************************************
 begin       : Mon Mar 01 2004
 copyright   : (C) 2009 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

/* This file is included by banking.c */



void AB_Banking__fillTransactionFromAccount(AB_TRANSACTION *t, const AB_ACCOUNT *a) {
  const char *s;

  s=AB_Transaction_GetLocalName(t);
  if (!s)
    AB_Transaction_SetLocalName(t, AB_Account_GetOwnerName(a));
  s=AB_Transaction_GetLocalBankCode(t);
  if (!s)
    AB_Transaction_SetLocalBankCode(t, AB_Account_GetBankCode(a));
  s=AB_Transaction_GetLocalAccountNumber(t);
  if (!s)
    AB_Transaction_SetLocalAccountNumber(t, AB_Account_GetAccountNumber(a));
  s=AB_Transaction_GetLocalIban(t);
  if (!s)
    AB_Transaction_SetLocalIban(t, AB_Account_GetIBAN(a));
  s=AB_Transaction_GetLocalBic(t);
  if (!s)
    AB_Transaction_SetLocalBic(t, AB_Account_GetBIC(a));
}



void AB_Banking__fillTransactionRemoteInfo(AB_TRANSACTION *t) {
  const GWEN_STRINGLIST *sl;

  sl=AB_Transaction_GetPurpose(t);
  if (sl) {
    GWEN_STRINGLISTENTRY *se;

    se=GWEN_StringList_FirstEntry(sl);
    while(se) {
      const char *s;

      s=GWEN_StringListEntry_Data(se);
      if (-1!=GWEN_Text_ComparePattern(s, "KTO* BLZ*", 0)) {
	char *cpy;
	char *p;
	char *kto;
	char *blz;

	cpy=strdup(s);
	p=cpy;

	/* skip "KTO", position to account number */
	while(*p && !isdigit(*p))
	  p++;
	kto=p;

	/* skip account number */
	while(*p && isdigit(*p))
	  p++;
	/* terminate account number */
	*(p++)=0;

	/* skip "BLZ", position to account number */
	while(*p && !isdigit(*p))
	  p++;
	blz=p;

	/* skip bank code */
	while(*p && isdigit(*p))
	  p++;
	/* terminate bank code */
	*p=0;

	if (*kto && *blz) {
	  AB_Transaction_SetRemoteAccountNumber(t, kto);
	  AB_Transaction_SetRemoteBankCode(t, blz);
	  free(cpy);
	  break;
	}
	else
	  free(cpy);
      }

      se=GWEN_StringListEntry_Next(se);
    }
  }
}




int AB_Banking_FillGapsInImExporterContext(AB_BANKING *ab, AB_IMEXPORTER_CONTEXT *iec) {
  AB_IMEXPORTER_ACCOUNTINFO *iea;
  int notFounds=0;

  assert(iec);
  iea=AB_ImExporterContext_GetFirstAccountInfo(iec);
  while(iea) {
    AB_ACCOUNT *a;

    a=AB_Banking_GetAccountByCodeAndNumber(ab,
					   AB_ImExporterAccountInfo_GetBankCode(iea),
					   AB_ImExporterAccountInfo_GetAccountNumber(iea));
    if (!a)
      a=AB_Banking_GetAccountByIban(ab, AB_ImExporterAccountInfo_GetIban(iea));
    if (a) {
      AB_TRANSACTION *t;

      AB_ImExporterAccountInfo_FillFromAccount(iea, a);

      /* fill transactions */
      t=AB_ImExporterAccountInfo_GetFirstTransaction(iea);
      while(t) {
	AB_Banking__fillTransactionFromAccount(t, a);
	if (AB_Transaction_GetRemoteBankCode(t)==NULL &&
	    AB_Transaction_GetRemoteAccountNumber(t)==NULL)
	  AB_Banking__fillTransactionRemoteInfo(t);
	t=AB_ImExporterAccountInfo_GetNextTransaction(iea);
      }

      /* fill standing orders */
      t=AB_ImExporterAccountInfo_GetFirstStandingOrder(iea);
      while(t) {
	AB_Banking__fillTransactionFromAccount(t, a);
	t=AB_ImExporterAccountInfo_GetNextStandingOrder(iea);
      }

      /* fill transfers */
      t=AB_ImExporterAccountInfo_GetFirstTransfer(iea);
      while(t) {
	AB_Banking__fillTransactionFromAccount(t, a);
	t=AB_ImExporterAccountInfo_GetNextTransfer(iea);
      }

      /* fill dated transfers */
      t=AB_ImExporterAccountInfo_GetFirstDatedTransfer(iea);
      while(t) {
	AB_Banking__fillTransactionFromAccount(t, a);
	t=AB_ImExporterAccountInfo_GetNextDatedTransfer(iea);
      }

      /* fill noted transactions */
      t=AB_ImExporterAccountInfo_GetFirstNotedTransaction(iea);
      while(t) {
	AB_Banking__fillTransactionFromAccount(t, a);
	t=AB_ImExporterAccountInfo_GetNextNotedTransaction(iea);
      }
    }
    else
      notFounds++;

    iea=AB_ImExporterContext_GetNextAccountInfo(iec);
  }

  return (notFounds==0)?0:1;
}



int AB_Banking_ExportToBuffer(AB_BANKING *ab,
			      AB_IMEXPORTER_CONTEXT *ctx,
			      const char *exporterName,
                              const char *profileName,
			      GWEN_BUFFER *buf,
			      uint32_t guiid) {
  AB_IMEXPORTER *ie;
  GWEN_DB_NODE *dbProfile;
  int rv;

  ie=AB_Banking_GetImExporter(ab, exporterName);
  if (ie==NULL) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here");
    return GWEN_ERROR_NO_DATA;
  }

  if (profileName && *profileName)
    dbProfile=AB_Banking_GetImExporterProfiles(ab, profileName);
  else
    dbProfile=GWEN_DB_Group_new("profile");
  if (dbProfile==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Profile [%s] not found",
	      profileName?profileName:"(null)");
    return GWEN_ERROR_NO_DATA;
  }

  rv=AB_ImExporter_ExportToBuffer(ie, ctx, buf, dbProfile, guiid);
  GWEN_DB_Group_free(dbProfile);

  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  return 0;
}



GWEN_PLUGIN_DESCRIPTION_LIST2 *AB_Banking_GetImExporterDescrs(AB_BANKING *ab){
  GWEN_PLUGIN_DESCRIPTION_LIST2 *l;

  l=GWEN_LoadPluginDescrs(AQBANKING_PLUGINS DIRSEP AB_IMEXPORTER_FOLDER);
  return l;
}



int AB_Banking__ReadImExporterProfiles(AB_BANKING *ab,
                                       const char *path,
				       GWEN_DB_NODE *db,
				       int isGlobal) {
  GWEN_DIRECTORY *d;
  GWEN_BUFFER *nbuf;
  char nbuffer[64];
  unsigned int pathLen;

  if (!path)
    path="";

  /* create path */
  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(nbuf, path);
  pathLen=GWEN_Buffer_GetUsedBytes(nbuf);

  d=GWEN_Directory_new();
  if (GWEN_Directory_Open(d, GWEN_Buffer_GetStart(nbuf))) {
    DBG_INFO(AQBANKING_LOGDOMAIN,
             "Path \"%s\" is not available",
	     GWEN_Buffer_GetStart(nbuf));
    GWEN_Buffer_free(nbuf);
    GWEN_Directory_free(d);
    return GWEN_ERROR_NOT_FOUND;
  }

  while(!GWEN_Directory_Read(d,
                             nbuffer,
                             sizeof(nbuffer))) {
    if (strcmp(nbuffer, ".") &&
        strcmp(nbuffer, "..")) {
      int nlen;

      nlen=strlen(nbuffer);
      if (nlen>4) {
        if (strcasecmp(nbuffer+nlen-5, ".conf")==0) {
          struct stat st;

	  GWEN_Buffer_Crop(nbuf, 0, pathLen);
	  GWEN_Buffer_SetPos(nbuf, pathLen);
	  GWEN_Buffer_AppendString(nbuf, DIRSEP);
	  GWEN_Buffer_AppendString(nbuf, nbuffer);

	  if (stat(GWEN_Buffer_GetStart(nbuf), &st)) {
	    DBG_ERROR(AQBANKING_LOGDOMAIN, "stat(%s): %s",
		      GWEN_Buffer_GetStart(nbuf),
		      strerror(errno));
	  }
	  else {
            if (!S_ISDIR(st.st_mode)) {
              GWEN_DB_NODE *dbT;

              dbT=GWEN_DB_Group_new("profile");
              if (GWEN_DB_ReadFile(dbT,
                                   GWEN_Buffer_GetStart(nbuf),
                                   GWEN_DB_FLAGS_DEFAULT |
				   GWEN_PATH_FLAGS_CREATE_GROUP, 0, 2000)) {
                DBG_ERROR(AQBANKING_LOGDOMAIN,
                          "Could not read file \"%s\"",
                          GWEN_Buffer_GetStart(nbuf));
              }
              else {
                const char *s;

                s=GWEN_DB_GetCharValue(dbT, "name", 0, 0);
                if (!s) {
                  DBG_ERROR(AQBANKING_LOGDOMAIN,
                            "Bad file \"%s\" (no name)",
                            GWEN_Buffer_GetStart(nbuf));
                }
                else {
                  GWEN_DB_NODE *dbTarget;
                  DBG_INFO(AQBANKING_LOGDOMAIN,
                           "File \"%s\" contains profile \"%s\"",
                           GWEN_Buffer_GetStart(nbuf), s);

                  dbTarget=GWEN_DB_GetGroup(db,
                                            GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                                            s);
                  assert(dbTarget);
		  GWEN_DB_AddGroupChildren(dbTarget, dbT);
		  GWEN_DB_SetIntValue(dbTarget, GWEN_DB_FLAGS_OVERWRITE_VARS, "isGlobal", isGlobal);
		  GWEN_DB_SetCharValue(dbTarget, GWEN_DB_FLAGS_OVERWRITE_VARS, "fileName", nbuffer);
                } /* if name */
              } /* if file successfully read */
              GWEN_DB_Group_free(dbT);
            } /* if !dir */
          } /* if stat was ok */
        } /* if conf */
      } /* if name has more than 4 chars */
    } /* if not "." and not ".." */
  } /* while */
  GWEN_Directory_Close(d);
  GWEN_Directory_free(d);
  GWEN_Buffer_free(nbuf);

  return 0;
}



GWEN_DB_NODE *AB_Banking_GetImExporterProfiles(AB_BANKING *ab,
                                               const char *name){
  GWEN_BUFFER *buf;
  GWEN_DB_NODE *db;
  int rv;
  GWEN_STRINGLIST *sl;
  GWEN_STRINGLISTENTRY *sentry;

  buf=GWEN_Buffer_new(0, 256, 0, 1);
  db=GWEN_DB_Group_new("profiles");

  sl=AB_Banking_GetGlobalDataDirs(ab);
  assert(sl);

  sentry=GWEN_StringList_FirstEntry(sl);
  assert(sentry);

  while(sentry) {
    const char *pkgdatadir;

    pkgdatadir = GWEN_StringListEntry_Data(sentry);
    assert(pkgdatadir);

    /* read global profiles */
    GWEN_Buffer_AppendString(buf, pkgdatadir);
    GWEN_Buffer_AppendString(buf,
			     DIRSEP
			     "aqbanking"
			     DIRSEP
			     AB_IMEXPORTER_FOLDER
			     DIRSEP);
    if (GWEN_Text_EscapeToBufferTolerant(name, buf)) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
                "Bad name for importer/exporter");
      GWEN_StringList_free(sl);
      GWEN_DB_Group_free(db);
      GWEN_Buffer_free(buf);
      return 0;
    }
    GWEN_Buffer_AppendString(buf, DIRSEP "profiles");
    rv=AB_Banking__ReadImExporterProfiles(ab,
                                          GWEN_Buffer_GetStart(buf),
					  db,
					  1);
    if (rv && rv!=GWEN_ERROR_NOT_FOUND) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
                "Error reading global profiles");
      GWEN_StringList_free(sl);
      GWEN_DB_Group_free(db);
      GWEN_Buffer_free(buf);
      return 0;
    }
    GWEN_Buffer_Reset(buf);
    sentry=GWEN_StringListEntry_Next(sentry);
  }
  GWEN_StringList_free(sl);

  /* read local user profiles */
  if (AB_Banking_GetUserDataDir(ab, buf)) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
              "Could not get user data dir");
    GWEN_DB_Group_free(db);
    GWEN_Buffer_free(buf);
    return 0;
  }
  GWEN_Buffer_AppendString(buf, DIRSEP AB_IMEXPORTER_FOLDER DIRSEP);
  if (GWEN_Text_EscapeToBufferTolerant(name, buf)) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
              "Bad name for importer/exporter");
    GWEN_DB_Group_free(db);
    GWEN_Buffer_free(buf);
    return 0;
  }
  GWEN_Buffer_AppendString(buf, DIRSEP "profiles");

  rv=AB_Banking__ReadImExporterProfiles(ab,
                                        GWEN_Buffer_GetStart(buf),
					db,
					0);
  if (rv && rv!=GWEN_ERROR_NOT_FOUND) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
              "Error reading users profiles");
    GWEN_DB_Group_free(db);
    GWEN_Buffer_free(buf);
    return 0;
  }
  GWEN_Buffer_free(buf);

  return db;
}



int AB_Banking_SaveLocalImExporterProfile(AB_BANKING *ab,
                                          const char *imexporterName,
					  GWEN_DB_NODE *dbProfile,
					  const char *fname){
  GWEN_BUFFER *buf;
  int rv;

  buf=GWEN_Buffer_new(0, 256, 0, 1);

  /* get folder for local user profiles */
  rv=AB_Banking_GetUserDataDir(ab, buf);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Could not get user data dir");
    GWEN_Buffer_free(buf);
    return rv;
  }
  GWEN_Buffer_AppendString(buf, DIRSEP AB_IMEXPORTER_FOLDER DIRSEP);
  rv=GWEN_Text_EscapeToBufferTolerant(imexporterName, buf);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Bad name for importer/exporter (%d)", rv);
    GWEN_Buffer_free(buf);
    return rv;
  }
  GWEN_Buffer_AppendString(buf, DIRSEP "profiles" DIRSEP);
  GWEN_Buffer_AppendString(buf, fname);

  rv=GWEN_DB_WriteFile(dbProfile,
		       GWEN_Buffer_GetStart(buf),
		       GWEN_DB_FLAGS_DEFAULT,
		       0,
		       2000);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Error writing users profile (%d)", rv);
    GWEN_Buffer_free(buf);
    return 0;
  }
  GWEN_Buffer_free(buf);

  return 0;
}



AB_IMEXPORTER *AB_Banking_FindImExporter(AB_BANKING *ab, const char *name) {
  AB_IMEXPORTER *ie;

  assert(ab);
  assert(name);
  ie=AB_ImExporter_List_First(ab_imexporters);
  while(ie) {
    if (strcasecmp(AB_ImExporter_GetName(ie), name)==0)
      break;
    ie=AB_ImExporter_List_Next(ie);
  } /* while */

  return ie;
}



AB_IMEXPORTER *AB_Banking_GetImExporter(AB_BANKING *ab, const char *name){
  AB_IMEXPORTER *ie;

  assert(ab);
  assert(name);

  ie=AB_Banking_FindImExporter(ab, name);
  if (ie)
    return ie;
  ie=AB_Banking__LoadImExporterPlugin(ab, name);
  if (ie) {
    AB_ImExporter_List_Add(ie, ab_imexporters);
  }

  return ie;
}



AB_IMEXPORTER *AB_Banking__LoadImExporterPlugin(AB_BANKING *ab,
                                                const char *modname){
  GWEN_PLUGIN *pl;

  pl=GWEN_PluginManager_GetPlugin(ab_pluginManagerImExporter, modname);
  if (pl) {
    AB_IMEXPORTER *ie;

    ie=AB_Plugin_ImExporter_Factory(pl, ab);
    if (!ie) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Error in plugin [%s]: No im/exporter created",
		modname);
      return NULL;
    }
    return ie;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Plugin [%s] not found", modname);
    return NULL;
  }
}



int AB_Banking_ExportWithProfile(AB_BANKING *ab,
				 const char *exporterName,
				 AB_IMEXPORTER_CONTEXT *ctx,
				 const char *profileName,
				 const char *profileFile,
				 GWEN_IO_LAYER *io,
				 uint32_t guiid) {
  AB_IMEXPORTER *exporter;
  GWEN_DB_NODE *dbProfiles;
  GWEN_DB_NODE *dbProfile;
  int rv;

  exporter=AB_Banking_GetImExporter(ab, exporterName);
  if (!exporter) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Export module \"%s\" not found",
	      exporterName);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* get profiles */
  if (profileFile) {
    dbProfiles=GWEN_DB_Group_new("profiles");
    if (GWEN_DB_ReadFile(dbProfiles, profileFile,
                         GWEN_DB_FLAGS_DEFAULT |
			 GWEN_PATH_FLAGS_CREATE_GROUP,
			 0,
			 2000)) {
      DBG_ERROR(0, "Error reading profiles from \"%s\"",
                profileFile);
      return GWEN_ERROR_GENERIC;
    }
  }
  else {
    dbProfiles=AB_Banking_GetImExporterProfiles(ab, exporterName);
  }

  /* select profile */
  dbProfile=GWEN_DB_GetFirstGroup(dbProfiles);
  while(dbProfile) {
    const char *name;

    name=GWEN_DB_GetCharValue(dbProfile, "name", 0, 0);
    assert(name);
    if (strcasecmp(name, profileName)==0)
      break;
    dbProfile=GWEN_DB_GetNextGroup(dbProfile);
  }
  if (!dbProfile) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Profile \"%s\" for exporter \"%s\" not found",
	      profileName, exporterName);
    GWEN_DB_Group_free(dbProfiles);
    return GWEN_ERROR_NOT_FOUND;
  }

  rv=AB_ImExporter_Export(exporter,
			  ctx,
			  io,
			  dbProfile,
			  guiid);
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    GWEN_DB_Group_free(dbProfiles);
    return rv;
  }

  GWEN_DB_Group_free(dbProfiles);
  return 0;
}



int AB_Banking_ImportWithProfile(AB_BANKING *ab,
				 const char *importerName,
				 AB_IMEXPORTER_CONTEXT *ctx,
				 const char *profileName,
				 const char *profileFile,
				 GWEN_IO_LAYER *io,
				 uint32_t guiid) {
  AB_IMEXPORTER *importer;
  GWEN_DB_NODE *dbProfiles;
  GWEN_DB_NODE *dbProfile;
  int rv;

  importer=AB_Banking_GetImExporter(ab, importerName);
  if (!importer) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Import module \"%s\" not found",
	      importerName);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* get profiles */
  if (profileFile) {
    dbProfiles=GWEN_DB_Group_new("profiles");
    if (GWEN_DB_ReadFile(dbProfiles, profileFile,
                         GWEN_DB_FLAGS_DEFAULT |
			 GWEN_PATH_FLAGS_CREATE_GROUP,
			 0,
			 2000)) {
      DBG_ERROR(0, "Error reading profiles from \"%s\"",
                profileFile);
      return GWEN_ERROR_GENERIC;
    }
  }
  else {
    dbProfiles=AB_Banking_GetImExporterProfiles(ab, importerName);
  }

  /* select profile */
  dbProfile=GWEN_DB_GetFirstGroup(dbProfiles);
  while(dbProfile) {
    const char *name;

    name=GWEN_DB_GetCharValue(dbProfile, "name", 0, 0);
    assert(name);
    if (strcasecmp(name, profileName)==0)
      break;
    dbProfile=GWEN_DB_GetNextGroup(dbProfile);
  }
  if (!dbProfile) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Profile \"%s\" for importer \"%s\" not found",
	      profileName, importerName);
    GWEN_DB_Group_free(dbProfiles);
    return GWEN_ERROR_NOT_FOUND;
  }

  rv=AB_ImExporter_Import(importer,
			  ctx,
			  io,
			  dbProfile,
			  guiid);
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    GWEN_DB_Group_free(dbProfiles);
    return rv;
  }

  GWEN_DB_Group_free(dbProfiles);
  return 0;
}



int AB_Banking_ImportFileWithProfile(AB_BANKING *ab,
				     const char *importerName,
				     AB_IMEXPORTER_CONTEXT *ctx,
				     const char *profileName,
				     const char *profileFile,
                                     const char *inputFileName,
				     uint32_t guiid) {
  int fd;

  fd=open(inputFileName, O_RDONLY);
  if (fd<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Error opening input file [%s]: %s",
	      inputFileName, strerror(errno));
    return GWEN_ERROR_IO;
  }
  else {
    GWEN_IO_LAYER *io;
    int rv;
  
    io=GWEN_Io_LayerFile_new(fd, -1);
    GWEN_Io_Layer_AddFlags(io, GWEN_IO_LAYER_FLAGS_DONTCLOSE);
    rv=GWEN_Io_Manager_RegisterLayer(io);
    if (rv<0) {
      DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
      GWEN_Io_Layer_free(io);
      close(fd);
      return rv;
    }
    rv=AB_Banking_ImportWithProfile(ab,
				    importerName,
				    ctx,
				    profileName,
				    profileFile,
				    io,
				    guiid);
    if (rv<0) {
      DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
      GWEN_Io_Layer_free(io);
      close(fd);
      return rv;
    }

    rv=GWEN_Io_Layer_DisconnectRecursively(io, NULL, 0, 0, 2000);
    if (rv<0) {
      DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
      GWEN_Io_Layer_free(io);
      close(fd);
      return rv;
    }

    return 0;
  }
}


