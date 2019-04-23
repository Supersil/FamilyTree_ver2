#include <cstdio>
#include <sys/types.h>
#include <cassert>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>
#include <cstdlib>

#include <sqlite3.h>
#include <db.h>

DB::DB(const char *dbpath)
   : _dbPath(dbpath),
   _db(nullptr),
   _pStmt(nullptr)
{

}

DB::~DB()
{
   finalizeSTMT();

   if (_db)
   {
      int ret = sqlite3_close_v2(_db);

      if(ret == SQLITE_OK)
      {
         printlog(DBG_INFO, "Database closed");
      }
      else
      {
         printlog(DBG_INFO, "Could not close database: %s", sqlite3_errmsg(_db));
      }
   }
}

int DB::dbCallback(void *count, int argc, char **argv, char **azColName)
{
   (void)argc;
   (void)azColName;

   int *c = static_cast<int*>(count);
   *c = atoi(argv[0]);
   return 0;
}

int DB::createTables()
{
   printlog(DBG_INFO,"Create tables");

   int ret;

   ret = sqlite3_exec(_db, CREATE_TABLES, nullptr, nullptr, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::createTables Exec failed");
      databaseError();
      return -ret;
   }

   return ret;
}

int DB::showTable(const char *tableName)
{
   printlog(DBG_INFO, "Show table");
   int ret;

   std::string zSql = "SELECT * FROM ";
   zSql += tableName;

   ret = sqlite3_prepare(_db, zSql.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      printlog(DBG_INFO, "CT2DB::showTable Prepare failed");
      databaseError();
      return -ret;
   }

   // sqlite3_bind_text(pStmt, 1, tableName, -1, SQLITE_STATIC);

   while (1)
   {
      ret = sqlite3_step (_pStmt);

      if (ret == SQLITE_ROW)
      {
         int colCnt = sqlite3_column_count(_pStmt);
         printlog(DBG_DEBUG, "Column count: %i", colCnt);

         for (int i = 0; i < colCnt;i++)
         {
            const unsigned char *text;
            text = sqlite3_column_text(_pStmt, i);
            printlog(DBG_DEBUG, "Column: %i, value %s", i, text);
         }
      }
      else if (ret == SQLITE_DONE)
      {
         printlog(DBG_INFO, "Done");
         break;
      }
      else
      {
         printlog(DBG_ERROR, "CT2DB::showTable Failed");
				ret = -1;
         break;
      }
   }

   finalizeSTMT();

   return ret;
}

void DB::databaseError()
{
   int errcode = sqlite3_errcode(_db);
   const char *errmsg = sqlite3_errmsg(_db);
   printlog(DBG_ERROR, "Database error %d: %s", errcode, errmsg);
}

int DB::beginTransaction(sqlite3 *db)
{
   return sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
}

int DB::endTransaction(sqlite3 *db)
{
   return sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr); // == COMMIT
}

void DB::setDBPath(const char *dbpath)
{
   _dbPath = dbpath;
}

int DB::openDB()
{
   int ret;

   if (_dbPath.empty())
   {
      printlog(DBG_WARN, "CT2DB::openDB Container is not set.");
      return -1;
   }

//   ret = sqlite3_open(_dbPath.c_str(), &_db);
   ret = sqlite3_open_v2(_dbPath.c_str(), &_db,
                    SQLITE_OPEN_READWRITE |
                    SQLITE_OPEN_CREATE |
                    SQLITE_OPEN_MAIN_DB,
                    nullptr );

   if(ret == SQLITE_OK)
   {
      printlog(DBG_INFO, "CT2DB::openDB Database is opened\n");
   }
   else
   {
//      fprintf(stderr, "Could not open database: %s\n", sqlite3_errmsg(_db));
      printlog(DBG_ERR, "CT2DB::openDB Could not open database: %s\n", sqlite3_errmsg(_db));
      return -1;
   }

   return 0;
}

int DB::closeDB()
{
   int ret = sqlite3_close_v2(_db);

   if(ret == SQLITE_OK)
   {
      printlog(DBG_INFO, "CT2DB::closeDB Database closed");
      _db = nullptr;
   }
   else
   {
      printlog(DBG_INFO, "CT2DB::closeDB Could not close database: %s", sqlite3_errmsg(_db));
   }

   return ret;
}

int DB::checkDB()
{
   if (openDB())
   {
      return -1;
   }

   int ret;

   ret = sqlite3_exec(_db, "pragma integrity_check;", nullptr, nullptr, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::checkDB Exec failed");
      databaseError();
   }

   return ret;
}

int DB::getKeyDataList(std::vector<std::string> &itemList)
{
//   printlog(DBG_DEBUG, "getMKIDList");

   int ret = 0;
   int row = 0;
   std::string request = "SELECT * FROM ";
   request += TABLE_MKID_NAME;

   itemList.clear();

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::getKeyDataList Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   while (1)
   {
      int s;

      s = sqlite3_step (_pStmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *value;
         value  = sqlite3_column_text(_pStmt, COLUMN_IDX_MKID);
//         printlog(DBG_INFO, "Item %d: %s", row, value);
         itemList.push_back((char*)value);
         row++;
      }
      else if (s == SQLITE_DONE)
      {
//         printlog(DBG_INFO, "DONE");
         break;
      }
      else
      {
         printlog(DBG_INFO, "CT2DB::getKeyDataList Failed");
				ret = -1;
         break;
      }
   }

   finalizeSTMT();
   endTransaction(_db);
   
   return ret;
}

int DB::insertKeyDataItem(std::string mkid, std::string seed1, std::string seed2, std::string &keydataid)
{
   printlog(DBG_INFO,"Insert keyData");

   if (mkid.empty() || seed1.empty() || seed2.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertKeyDataItem Invalid data");
      return -1;
   }
   keydataid.clear();
   int ret;

   std::vector<std::string> itemList;
   std::string tmp_mkid, tmp_seed1, tmp_seed2;
   if (getKeyDataList(itemList))
   {
      printlog(DBG_ERROR, "CT2DB::insertKeyDataItem Failed to get MKID list");
      return -1;
   }

   for(int id = itemList.size() - 1; id >= 0; id--)
   {
      if (getKeyDataByID(itemList[id],tmp_mkid,tmp_seed1,tmp_seed2))
      {
         printlog(DBG_ERROR, "CT2DB::insertKeyDataItem Failed to get MKID data");
         return -1;
      }
      if ((tmp_mkid.compare(mkid) == 0) && (tmp_seed1.compare(seed1) == 0) && (tmp_seed2.compare(seed2) == 0))
      {
        keydataid = itemList[id];
        printlog(DBG_ERROR, "CT2DB::insertKeyDataItem KeyData already presents in the table");
        return 0;
      }
   }

   std::string request = "INSERT INTO ";
   request += TABLE_MKID_NAME;
   request += " (MKID, SEED1, SEED2) VALUES(?, ?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

	 // Put arguments to sql command (instead of ? in request)
   sqlite3_bind_text(_pStmt, 1, mkid.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, seed1.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 3, seed2.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

	 if (ret == SQLITE_DONE)
		 ret = 0;

   finalizeSTMT();
   endTransaction(_db);
   
   if (getKeyDataList(itemList))
   {
      printlog(DBG_ERROR, "CT2DB::insertKeyDataItem Failed to get MKID list");
      return -1;
   }

   for(int id = itemList.size() - 1; id >= 0; id--)
   {
      if (getKeyDataByID(itemList[id],tmp_mkid,tmp_seed1,tmp_seed2))
      {
         printlog(DBG_ERROR, "CT2DB::insertKeyDataItem Failed to get MKID data");
         return -1;
      }
      if ((tmp_mkid.compare(mkid) == 0) && (tmp_seed1.compare(seed1) == 0) && (tmp_seed2.compare(seed2) == 0))
      {
        keydataid = itemList[id];
      }
   }

   if (keydataid.size() == 0)
   {
      printlog(DBG_ERROR, "CT2DB::insertKeyDataItem MKID not found in the list");
      return -1;
   }

   return ret;
}

int DB::getKeyDataByID(std::string &keyDataId, std::string &mkid, std::string &seed1, std::string &seed2)
{
   int ret;

   if (keyDataId.empty())
   {
      printlog(DBG_ERROR, "CT2DB::getKeyDataDataByID Invalid data");
      return -1;
   }

   std::string request = "SELECT * FROM ";
   request += TABLE_MKID_NAME;
   request += " WHERE ";
   request += COLUMN_MKID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, keyDataId.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);
   if (ret == SQLITE_ROW)
   {
      mkid = (char *)sqlite3_column_text(_pStmt, 1);
      seed1 = (char *)sqlite3_column_text(_pStmt, 2);
      seed2 = (char *)sqlite3_column_text(_pStmt, 3);
   }
   else if (ret == SQLITE_DONE)
   {
      printlog(DBG_ERROR, "CT2DB::getKeyDataDataByID No data");
      finalizeSTMT();
      return -1;
   }
   else
   {
      printlog(DBG_WARN, "CT2DB::getKeyDataDataByID Failed");
      finalizeSTMT();
      return -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::deleteKeyDataItemByID(std::string &keydataid)
{
   if (keydataid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::deleteKeyDataItemByID Invalid id");
      return -1;
   }

   int ret = 0;

   printlog(DBG_DEBUG, "Delete keyData %s", keydataid.c_str());

   std::string request = "DELETE FROM ";
   request += TABLE_MKID_NAME;
   request += " WHERE ";
   request += COLUMN_MKID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   sqlite3_bind_text(_pStmt, 1, keydataid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteKeyDataItemByID Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::createLogTable(std::string logtable, std::string utc, std::string keydataid)
{
   printlog(DBG_INFO,"Create logTable: %s", logtable.c_str());
   int ret;
   char request[1024] = { 0 };

   snprintf(request, 1024, INSERT_LOG_TABLE_FORMAT, logtable.c_str());

   ret = sqlite3_prepare(_db, request, -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   beginTransaction(_db);

   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   finalizeSTMT();
   endTransaction(_db);

   memset(request, 0, 1024);

   snprintf(request, 1024, "INSERT INTO LOGLIST (Tablename, UTC, KEYDATAID) VALUES(?, ?, ?)");

   ret = sqlite3_prepare(_db, request, -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   ret = sqlite3_bind_text(_pStmt, 1, logtable.c_str(),  -1, SQLITE_STATIC);
	 ret = sqlite3_bind_text(_pStmt, 2, utc.c_str(), -1, SQLITE_STATIC);
   ret = sqlite3_bind_text(_pStmt, 3, keydataid.c_str(), -1, SQLITE_STATIC);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   beginTransaction(_db);

   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);


	 if (ret == SQLITE_DONE)
		 ret = 0;


   finalizeSTMT();
   endTransaction(_db);

   return ret;
}

int DB::getLogIDListByLogTable(std::string logtable,
                        std::vector<std::string> &itemList)
{
//   printlog(DBG_DEBUG, "Table: %s", logtable.c_str());

   int ret = 0;
   int row = 0;
   std::string request = "SELECT * FROM ";
   request += logtable;

   itemList.clear();

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::getLogIDListByLogTable Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   while (1)
   {
      int s;

      s = sqlite3_step (_pStmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *value;
         value  = sqlite3_column_text(_pStmt, 0);
//         printlog(DBG_INFO, "Item %d: %s", row, value);
         itemList.push_back((char*)value);
         row++;
      }
      else if (s == SQLITE_DONE)
      {
//         printlog(DBG_INFO, "DONE");
         break;
      }
      else
      {
         printlog(DBG_INFO, "CT2DB::getLogIDListByLogTable Failed");
				ret = -1;
         break;
      }
   }

   finalizeSTMT();
   endTransaction(_db);

   return ret;
}

int DB::getLogTableList(std::vector<std::string> &itemList, std::vector<std::string> &utcList, std::string format)
{
//   printlog(DBG_DEBUG, "getLogTableList");

   int ret = 0;
   int row = 0;
   std::string request = "SELECT * FROM ";
   request += TABLE_LOGLIST_NAME;
   request += " WHERE Tablename LIKE ";
   request += format;

   itemList.clear();

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::getLogTableList Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   while (1)
   {
      int s;

      s = sqlite3_step (_pStmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *value;
         value  = sqlite3_column_text(_pStmt, 1);
//         printlog(DBG_INFO, "Item %d: %s", row, value);
         itemList.push_back((char*)value);
         value  = sqlite3_column_text(_pStmt, 2);
         utcList.push_back((char*)value);
         row++;
      }
      else if (s == SQLITE_DONE)
      {
//         printlog(DBG_INFO, "DONE");
         break;
      }
      else
      {
         printlog(DBG_INFO, "CT2DB::getLogTableList Failed");
				ret = -1;
         break;
      }
   }

   finalizeSTMT();
   endTransaction(_db);

   return ret;
}


int DB::getKeyDataIDbyLogName(std::string logName, std::string &keyDataID)
{
   
   int ret = 0;
   std::string request = "SELECT * FROM ";
   request += TABLE_LOGLIST_NAME;
   request += " WHERE Tablename LIKE ";
   request += "'" + logName + "'";
   
   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::getKeyDataIDbyLogName Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   int s;

   s = sqlite3_step (_pStmt);
   if (s == SQLITE_ROW)
   {
      const unsigned char *value;
      keyDataID  = (char *)sqlite3_column_text(_pStmt, 3);
   }
   else if (s == SQLITE_DONE)
   {
//         printlog(DBG_INFO, "DONE");
   }
   else
   {
      printlog(DBG_INFO, "CT2DB::getKeyDataIDbyLogName Failed");
      ret = -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return ret;  
}

int DB::insertLogItem(std::string logtable, std::string iv, std::string utc, std::string data, std::string mac)
{
   printlog(DBG_INFO,"Insert logItem in %s",logtable.c_str());

   if (logtable.empty() || iv.empty() ||
      utc.empty() || data.empty() || mac.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertLogItem Invalid data");
      return -1;
   }

   int ret;

   std::string request = "INSERT INTO ";
   request += logtable;
   request += " (IV, UTC, DATA, MAC) VALUES(?, ?, ?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, iv.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, utc.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 3, data.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 4, mac.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   finalizeSTMT();
   endTransaction(_db);


	 if (ret == SQLITE_DONE)
		 ret = 0;


   return ret;
}


int DB::insertLogItem(std::string logtable, std::string logid, std::string iv,
                  std::string utc, std::string data, std::string mac)
{
   printlog(DBG_INFO,"Insert logItem in %s",logtable.c_str());

   if (logtable.empty() || iv.empty() ||
      utc.empty() || data.empty() || mac.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertLogItem Invalid data");
      return -1;
   }

   int ret;

   std::string request = "INSERT INTO ";
   request += logtable;
   request += " (LOGID, IV, UTC, DATA, MAC) VALUES(?, ?, ?, ?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, logid.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, iv.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 3, utc.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 4, data.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 5, mac.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   finalizeSTMT();
   endTransaction(_db);


	 if (ret == SQLITE_DONE)
		 ret = 0;


   return ret;
}

int DB::getLogDataByID(std::string logtable, std::string logid, std::string &iv, std::string &utc, std::string &data, std::string &mac)
{
//   printlog(DBG_INFO,"");
   int ret;

   if (logtable.empty() || logid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::getLogDataByID Invalid data");
      return -1;
   }

   iv.clear();
   utc.clear();
   data.clear();
   mac.clear();

   std::string request = "SELECT * FROM ";
   request += logtable;
   request += " WHERE ";
   request += COLUMN_LOGID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, logid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);
   if (ret == SQLITE_ROW)
   {
      iv = (char *)sqlite3_column_text(_pStmt, 1);
      utc = (char *)sqlite3_column_text(_pStmt, 2);
      data = (char *)sqlite3_column_text(_pStmt, 3);
      mac = (char *)sqlite3_column_text(_pStmt, 4);
   }
   else if (ret == SQLITE_DONE)
   {
      printlog(DBG_ERROR, "CT2DB::getLogDataByID No data");
      finalizeSTMT();
      return -1;
   }
   else
   {
      printlog(DBG_WARN, "CT2DB::getLogDataByID Failed");
      finalizeSTMT();
      return -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::deleteLogTable(std::string logtable)
{
   printlog(DBG_DEBUG, "Delete %s", logtable.c_str());

   if (logtable.empty())
   {
      printlog(DBG_ERROR, "CT2DB::deleteLogTable Invalid data");
      return -1;
   }

   int ret;

   std::string request = "DELETE FROM ";
   request += TABLE_LOGLIST_NAME;
   request += " WHERE Tablename = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   sqlite3_bind_text(_pStmt, 1, logtable.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteLogTable Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   request = "DROP TABLE IF EXISTS ";
   request += logtable;

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteLogTable Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::deleteLogItemByID(std::string logtable, std::string logid)
{
   printlog(DBG_DEBUG, "Delete %s:%s", logtable.c_str(), logid.c_str());

   if (logtable.empty() || logid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::deleteLogItemByID Invalid data");
      return -1;
   }

   int ret;

   std::string request = "DELETE FROM ";
   request += logtable;
   request += " WHERE ";
   request += COLUMN_LOGID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   sqlite3_bind_text(_pStmt, 1, logid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteLogItemByID Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::getINIIDList(std::vector<std::string> &itemList)
{
//   printlog(DBG_DEBUG, "getINIIDList");

   int ret = 0;
   int row = 0;
   std::string request = "SELECT * FROM ";
   request += TABLE_INI_NAME;

   itemList.clear();

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::getINIIDList Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   while (1)
   {
      int s;

      s = sqlite3_step (_pStmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *value;
         value  = sqlite3_column_text(_pStmt, 0);
//         printlog(DBG_INFO, "Item %d: %s", row, value);
         itemList.push_back((char*)value);
         row++;
      }
      else if (s == SQLITE_DONE)
      {
//         printlog(DBG_INFO, "DONE");
         break;
      }
      else
      {
         printlog(DBG_INFO, "CT2DB::getINIIDList Failed");
				ret = -1;
         break;
      }
   }

   finalizeSTMT();
   endTransaction(_db);

   return ret;
}

int DB::insertINIItem(std::string iniid, std::string iv, std::string data, std::string mac, std::string utc, std::string keydataid)
{
   printlog(DBG_INFO,"Insert INI %s", iniid.c_str());

   if (iniid.empty() || data.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertINIItem Invalid data");
      return -1;
   }

   int ret;

   std::string request = "INSERT INTO ";
   request += TABLE_INI_NAME;
   request += " (IniID, IV, Data, MAC, UTC, KEYDATAID) VALUES(?, ?, ?, ?, ?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, iniid.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, iv.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 3, data.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 4, mac.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 5, utc.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 6, keydataid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   finalizeSTMT();
   endTransaction(_db);

	 if (ret == SQLITE_DONE)
		 ret = 0;

   return ret;
}

int DB::getINIDataByID(std::string iniid, std::string &iv, std::string &data, std::string &mac, std::string &utc, std::string &keydataid)
{
//   printlog(DBG_INFO,"");
   int ret;

   if (iniid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::getINIDataByID Invalid data");
      return -1;
   }

   std::string request = "SELECT * FROM ";
   request += TABLE_INI_NAME;
   request += " WHERE ";
   request += COLUMN_INIID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, iniid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);
   if (ret == SQLITE_ROW)
   {
      iv = (char *)sqlite3_column_text(_pStmt, 1);
      data = (char *)sqlite3_column_text(_pStmt, 2);
      mac = (char *)sqlite3_column_text(_pStmt, 3);
      utc = (char *)sqlite3_column_text(_pStmt, 4);
      keydataid = (char *)sqlite3_column_text(_pStmt, 5);
   }
   else if (ret == SQLITE_DONE)
   {
      printlog(DBG_ERROR, "CT2DB::getINIDataByID No data");
      finalizeSTMT();
      return -1;
   }
   else
   {
      printlog(DBG_WARN, "CT2DB::getINIDataByID Failed");
      finalizeSTMT();
      return -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::deleteINIItemByID(std::string iniid)
{
   if (iniid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::deleteINIItemByID Invalid id");
      return -1;
   }

   int ret = 0;

   printlog(DBG_DEBUG, "Delete INI %s", iniid.c_str());

   std::string request = "DELETE FROM ";
   request += TABLE_INI_NAME;
   request += " WHERE ";
   request += COLUMN_INIID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   sqlite3_bind_text(_pStmt, 1, iniid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteINIItemByID Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::getFileIDList(std::vector<std::string> &itemList)
{
//   printlog(DBG_DEBUG, "getFileIDList");

   int ret = 0;
   int row = 0;
   std::string request = "SELECT * FROM ";
   request += TABLE_FILE_NAME;

   itemList.clear();

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
      printlog(DBG_ERROR, "CT2DB::getFileIDList Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   while (1)
   {
      int s;

      s = sqlite3_step (_pStmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *value;
         value  = sqlite3_column_text(_pStmt, COLUMN_IDX_MKID);
//         printlog(DBG_INFO, "Item %d: %s", row, value);
         itemList.push_back((char*)value);
         row++;
      }
      else if (s == SQLITE_DONE)
      {
//         printlog(DBG_INFO, "DONE");
         break;
      }
      else
      {
         printlog(DBG_INFO, "CT2DB::getFileIDList Failed");
				ret = -1;
         break;
      }
   }

   finalizeSTMT();
   endTransaction(_db);

   return ret;
}

int DB::insertFileItem(std::string fileid, std::string iv, std::string data, std::string mac, std::string utc, std::string keydataid)
{
   printlog(DBG_INFO,"Insert file %s", fileid.c_str());

   if (fileid.empty() || data.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertFileItem Invalid id");
      return -1;
   }

   int ret;

   std::string request = "INSERT INTO ";
   request += TABLE_FILE_NAME;
   request += " (FileID, IV, Data, MAC, UTC, KEYDATAID) VALUES(?, ?, ?, ?, ?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, fileid.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, iv.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 3, data.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 4, mac.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 5, utc.c_str(), -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 6, keydataid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   finalizeSTMT();
   endTransaction(_db);


	 if (ret == SQLITE_DONE)
		 ret = 0;

   return ret;
}

int DB::getFileDataByID(std::string fileid, std::string &iv, std::string &data, std::string &mac, std::string &utc, std::string &keydataid)
{
//   printlog(DBG_INFO,"");
   int ret;

   if (fileid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::getFileDataByID Invalid data");
      return -1;
   }

   std::string request = "SELECT * FROM ";
   request += TABLE_FILE_NAME;
   request += " WHERE ";
   request += COLUMN_FILEID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   sqlite3_bind_text(_pStmt, 1, fileid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);
   if (ret == SQLITE_ROW)
   {
      iv = (char *)sqlite3_column_text(_pStmt, 1);
      data = (char *)sqlite3_column_text(_pStmt, 2);
      mac = (char *)sqlite3_column_text(_pStmt, 3);
      utc = (char *)sqlite3_column_text(_pStmt, 4);
      keydataid = (char *)sqlite3_column_text(_pStmt, 5);
   }
   else if (ret == SQLITE_DONE)
   {
      printlog(DBG_ERROR, "CT2DB::getFileDataByID No data");
      finalizeSTMT();
      return -1;
   }
   else
   {
      printlog(DBG_WARN, "CT2DB::getFileDataByID Failed");
      finalizeSTMT();
      return -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::deleteFileItemByID(std::string fileid)
{
   if (fileid.empty())
   {
      printlog(DBG_ERROR, "CT2DB::deleteFileItemByID Invalid id");
      return -1;
   }

   int ret = 0;

   printlog(DBG_DEBUG, "Delete File %s", fileid.c_str());

   std::string request = "DELETE FROM ";
   request += TABLE_FILE_NAME;
   request += " WHERE ";
   request += COLUMN_FILEID_NAME;
   request += " = ?";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   sqlite3_bind_text(_pStmt, 1, fileid.c_str(), -1, SQLITE_STATIC);

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteFileItemByID Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}


int DB::getDefaultMKID(std::string &mkid, std::string &utc_expireTime)
{
   int ret;

   std::string request = "SELECT * FROM ";
   request += TABLE_DEFAULTMKID_NAME;

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }
   
   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);
   if (ret == SQLITE_ROW)
   {
      mkid = (char *)sqlite3_column_text(_pStmt, 0);
      utc_expireTime = (char *)sqlite3_column_text(_pStmt, 1);
   }
   else if (ret == SQLITE_DONE)
   {
      printlog(DBG_ERROR, "CT2DB::getDefaultMKID No data");
      finalizeSTMT();
      return 1;
   }
   else
   {
      printlog(DBG_WARN, "CT2DB::getDefaultMKID Failed");
      finalizeSTMT();
      return -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0; 
}


int DB::insertDefaultMKID(std::string mkid, std::string utc_expireTime)
{
   if (mkid.empty() || utc_expireTime.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertDefaultMKID Invalid data");
      return -1;
   }
   int ret;
   std::string tmp_mkid, tmp_utc;
   if (getDefaultMKID(tmp_mkid, tmp_utc) != 1)
   {
      printlog(DBG_WRN, "CT2DB::insertDefaultMKID At first delete previous default mkid");
      return 1;
   }
   
   std::string request = "INSERT INTO ";
   request += TABLE_DEFAULTMKID_NAME;
   request += " (MKID, UTC) VALUES(?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   // Put arguments to sql command (instead of ? in request)
   sqlite3_bind_text(_pStmt, 1, mkid.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, utc_expireTime.c_str(), -1, SQLITE_STATIC);
  
   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   if (ret == SQLITE_DONE)
     ret = 0;

   finalizeSTMT();
   endTransaction(_db);
  
   return ret;
}


int DB::deleteDefaultMKID()
{
   int ret = 0;

   printlog(DBG_DEBUG, "Delete default MKID");

   std::string request = "DELETE FROM ";
   request += TABLE_DEFAULTMKID_NAME;
   
   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteDefaultMKID Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}

int DB::getLastPINChangeItem(std::string SerialNum, std::string &utc)
{
   int ret;

   std::string request = "SELECT * FROM ";
   request += TABLE_LASTPINCHANGE_NAME;
   request += " WHERE ";
   request += COLUMN_SERIALNUM_NAME;
   request += " = ?";
   
   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }

   sqlite3_bind_text(_pStmt, 1, SerialNum.c_str(), -1, SQLITE_STATIC);
   
   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);
   if (ret == SQLITE_ROW)
   {
      utc = (char *)sqlite3_column_text(_pStmt, 1);
   }
   else if (ret == SQLITE_DONE)
   {
      printlog(DBG_ERROR, "CT2DB::getLastPINChangeItem No data");
      finalizeSTMT();
      return 1;
   }
   else
   {
      printlog(DBG_WARN, "CT2DB::getLastPINChangeItem Failed");
      finalizeSTMT();
      return -1;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0; 
}

int DB::insertLastPINChangeItem(std::string SerialNum, std::string utc)
{
   if (SerialNum.empty() || utc.empty())
   {
      printlog(DBG_ERROR, "CT2DB::insertLastPINChangeItem Invalid data");
      return -1;
   }
   int ret;
   std::string tmp_utc;
   ret = getLastPINChangeItem(SerialNum, tmp_utc);
   if ( ret == -1)
   {
      printlog(DBG_WRN, "CT2DB::insertLastPINChangeItem Failed to get utc for input SN");
      return -1;
   }
   
   if (ret == 0)
     if (deleteLastPINChangeItem(SerialNum))
     {
       printlog(DBG_WRN, "CT2DB::insertLastPINChangeItem Failed to delete previous entry with such SN");
       return -1;
     }
   
   
   std::string request = "INSERT INTO ";
   request += TABLE_LASTPINCHANGE_NAME;
   request += " (SerialNum, UTC) VALUES(?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   // Put arguments to sql command (instead of ? in request)
   sqlite3_bind_text(_pStmt, 1, SerialNum.c_str(),  -1, SQLITE_STATIC);
   sqlite3_bind_text(_pStmt, 2, utc.c_str(), -1, SQLITE_STATIC);
  
   beginTransaction(_db);
   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   if (ret == SQLITE_DONE)
     ret = 0;

   finalizeSTMT();
   endTransaction(_db);
  
   return ret;
}

int DB::deleteLastPINChangeItem(std::string SerialNum)
{
   int ret = 0;

   printlog(DBG_DEBUG, "Delete LASTPINCHANGE entry");

   std::string request = "DELETE FROM ";
   request += TABLE_LASTPINCHANGE_NAME;
   request += " WHERE ";
   request += COLUMN_SERIALNUM_NAME;
   request += " = ?";
   
   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return -1;
   }
   
   sqlite3_bind_text(_pStmt, 1, SerialNum.c_str(), -1, SQLITE_STATIC);
   
   beginTransaction(_db);

   ret = sqlite3_step (_pStmt);

   if(ret != SQLITE_DONE)
   {
      printlog(DBG_INFO, "CT2DB::deleteDefaultMKID Delete failed");
      databaseError();
      return -ret;
   }

   finalizeSTMT();
   endTransaction(_db);

   return 0;
}



int DB::createRoot(std::string rootName, std::string &tableName)
{
//   printlog(DBG_INFO,"Create logTable: %s", logtable.c_str());
   int ret;
   char request[1024] = { 0 };
   std::string surname = rootName.substr(0,rootName.find(" "));

   std::vector<std::string> rootList;
   std::vector<std::string> tableList;

   ret = getListOfRoots(rootList,tableList);
   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }


   snprintf(request, 1024, "INSERT INTO ROOTTABLE (NAME,TABLENAME) VALUES(?, ?)");

   ret = sqlite3_prepare(_db, request, -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   ret = sqlite3_bind_text(_pStmt, 1, logtable.c_str(),  -1, SQLITE_STATIC);
    ret = sqlite3_bind_text(_pStmt, 2, utc.c_str(), -1, SQLITE_STATIC);
   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   beginTransaction(_db);

   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);


    if (ret == SQLITE_DONE)
       ret = 0;


   finalizeSTMT();
   endTransaction(_db);

   memset(request, 0, 1024);

   snprintf(request, 1024, INSERT_LOG_TABLE_FORMAT, logtable.c_str());

   ret = sqlite3_prepare(_db, request, -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   beginTransaction(_db);

   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   finalizeSTMT();
   endTransaction(_db);


   return ret;
}

int DB::selectRoot(std::string rootName, std::string &tableName)
{

}

int DB::addPerson(std::string tableName, Person *person, int *id)
{

}

int DB::getListOfRoots(std::vector<std::string> & rootList, std::vector<std::string> &tableList, std::string format )
{
   int ret = 0;
   int row = 0;
   std::string request = "SELECT * FROM ";
   request += "ROOTTABLE";
   request += " WHERE NAME LIKE ";
   request += format;

   tableList.clear();
   rootList.clear();

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
//      printlog(DBG_ERROR, "CT2DB::getLogTableList Prepare failed");
      databaseError();
      return -1;
   }

   beginTransaction(_db);

   while (1)
   {
      int s;

      s = sqlite3_step (_pStmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *value;
         value  = sqlite3_column_text(_pStmt, 1);
//         printlog(DBG_INFO, "Item %d: %s", row, value);
         rootList.push_back((char*)value);
         value  = sqlite3_column_text(_pStmt, 2);
         tableList.push_back((char*)value);
         row++;
      }
      else if (s == SQLITE_DONE)
      {
//         printlog(DBG_INFO, "DONE");
         break;
      }
      else
      {
//         printlog(DBG_INFO, "CT2DB::getLogTableList Failed");
         ret = -1;
         break;
      }
   }

   finalizeSTMT();
   endTransaction(_db);

   return ret;
}
