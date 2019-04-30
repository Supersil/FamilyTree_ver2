#ifdef DATABASE

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

#include "writelog.h"

DB::DB(const char *dbpath)
   : _dbPath(dbpath),
   _db(nullptr),
   _bOpened(false)
{

}

DB::~DB()
{
   _bOpened = false;
   if (_db)
   {
        int ret = sqlite3_close_v2(_db);

        if(ret == SQLITE_OK)
        {
             writeDebugLog("Database closed");
        }
        else
        {
             writeDebugLog(QString("Could not close database: ") + sqlite3_errmsg(_db));
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
   if (!_bOpened)
        return -1;

   writeDebugLog("Create tables");

   int ret;

   ret = sqlite3_exec(_db, CREATE_TABLES, nullptr, nullptr, nullptr);

   if(ret != SQLITE_OK)
   {
        writeDebugLog("CT2DB::createTables Exec failed");
        databaseError();
        return -ret;
   }

   return ret;
}

void DB::databaseError()
{
   int errcode = sqlite3_errcode(_db);
   const char *errmsg = sqlite3_errmsg(_db);
   writeDebugLog(QString("Database error: ") + QString::number(errcode) + ", " + errmsg);
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
        writeDebugLog("CT2DB::openDB Container is not set.");
        return -1;
   }

   ret = sqlite3_open_v2(_dbPath.c_str(), &_db,
                              SQLITE_OPEN_READWRITE |
                              SQLITE_OPEN_CREATE |
                              SQLITE_OPEN_MAIN_DB,
                              nullptr );

   if(ret == SQLITE_OK)
   {
        writeDebugLog("CT2DB::openDB Database is opened\n");
   }
   else
   {
        writeErrorLog(QString("CT2DB::openDB Could not open database: ") + sqlite3_errmsg(_db));
        return -1;
   }
   _bOpened = true;
   return 0;
}

int DB::closeDB()
{
   _bOpened = false;
   int ret = sqlite3_close_v2(_db);

   if(ret == SQLITE_OK)
   {
        writeDebugLog("CT2DB::closeDB Database closed");
        _db = nullptr;
   }
   else
   {
        writeDebugLog(QString("CT2DB::closeDB Could not close database: ") + sqlite3_errmsg(_db));
   }

   return ret;
}

int DB::checkDB()
{
   if (!_bOpened)
        return -1;

   int ret;

   ret = sqlite3_exec(_db, "pragma integrity_check;", nullptr, nullptr, nullptr);

   if(ret != SQLITE_OK)
   {
        writeDebugLog("CT2DB::checkDB Exec failed");
        databaseError();
   }

   return ret;
}

int DB::createRoot(std::string rootName, std::string tableName)
{
   writeDebugLog(QString("Create logTable: ") + tableName.c_str());
   int ret;
   char request[1224] = { 0 };
   sqlite3_stmt *_pStmt;

   snprintf(request, 1224, "INSERT INTO ROOTTABLE (NAME,TABLENAME) VALUES(?, ?)");

   ret = sqlite3_prepare(_db, request, -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
        databaseError();
        return ret;
   }

   ret = sqlite3_bind_text(_pStmt, 1, rootName.c_str(),  -1, SQLITE_STATIC);
   ret = sqlite3_bind_text(_pStmt, 2, tableName.c_str(), -1, SQLITE_STATIC);

   if( ret != SQLITE_OK )
   {
        databaseError();
        return ret;
   }
{
   dbTransactor trans(this,_pStmt);

   do {
        ret = sqlite3_step(_pStmt);
        assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);


      if (ret == SQLITE_DONE)
         ret = 0;
}
   _pStmt = 0;
   memset(request, 0, 1224);

   snprintf(request, 1224, INSERT_ROOT_TABLE_FORMAT, tableName.c_str());

   ret = sqlite3_prepare(_db, request, -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
        databaseError();
        return ret;
   }

   dbTransactor trans(this,_pStmt);
   do {
        ret = sqlite3_step(_pStmt);
        assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

   if (ret == SQLITE_DONE)
      ret = 0;

   if( ret != SQLITE_OK )
   {
        databaseError();
        return ret;
   }

   return ret;
}

int DB::addPerson(std::string tableName, uint32_t id, std::string name, std::string birthDate,
                  std::string isAlive, std::string deathDate, std::string info,
                  std::string birthPlace, std::string photo, std::string sex,
                  uint32_t fatherId, uint32_t motherId, uint32_t childrenCnt,
                  std::string childrenID)
{
   writeDebugLog(QString("Insert logItem in ") + tableName.c_str());

   if (tableName.empty() || (name.empty()))
   {
      //printlog(DBG_ERROR, "CT2DB::insertLogItem Invalid data");
      return -1;
   }

   int ret;
   sqlite3_stmt *_pStmt;

   std::string request = "INSERT INTO ";
   request += tableName;
   request += " (ID, NAME, DATEOFBIRTH, ISALIVE, DATEOFDEATH, INFO, BIRTHPLACE, PHOTO, SEX, FATHERID,\
 MOTHERID, CHILDRENCNT, CHILDRENID) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if( ret != SQLITE_OK )
   {
      databaseError();
      return ret;
   }

   ret |= sqlite3_bind_int(_pStmt, 1, id);
   ret |= sqlite3_bind_text(_pStmt, 2, name.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 3, birthDate.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 4, isAlive.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 5, deathDate.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 6, info.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 7, birthPlace.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 8, photo.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_text(_pStmt, 9, sex.c_str(),  -1, SQLITE_STATIC);
   ret |= sqlite3_bind_int(_pStmt, 10, fatherId);
   ret |= sqlite3_bind_int(_pStmt, 11, motherId);
   ret |= sqlite3_bind_int(_pStmt, 12, childrenCnt);
   ret |= sqlite3_bind_text(_pStmt, 13, childrenID.c_str(),  -1, SQLITE_STATIC);


   if( ret != SQLITE_OK )
   {
        databaseError();
        return ret;
   }

   dbTransactor trans(this,_pStmt);

   do {
      ret = sqlite3_step(_pStmt);
      assert( ret != SQLITE_ROW );
   } while(ret == SQLITE_SCHEMA);

    if (ret == SQLITE_DONE)
         ret = 0;

    if( ret != SQLITE_OK )
    {
         databaseError();
         return ret;
    }

   return ret;
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
   sqlite3_stmt *_pStmt;

   ret = sqlite3_prepare(_db, request.c_str(), -1, &_pStmt, nullptr);

   if(ret != SQLITE_OK)
   {
//        writeDebugLog("CT2DB::getLogTableList Prepare failed");
        databaseError();
        return -1;
   }

   dbTransactor trans(this,_pStmt);

   while (1)
   {
        int s;

        s = sqlite3_step (_pStmt);
        if (s == SQLITE_ROW)
        {
             const unsigned char *value;
             value  = sqlite3_column_text(_pStmt, 1);
//             writeDebugLog("Item %d: %s", row, value);
             rootList.push_back((char*)value);
             value  = sqlite3_column_text(_pStmt, 2);
             tableList.push_back((char*)value);
             row++;
        }
        else if (s == SQLITE_DONE)
        {
//             writeDebugLog("DONE");
             break;
        }
        else
        {
//             writeDebugLog("CT2DB::getLogTableList Failed");
             ret = -1;
             break;
        }
   }

   return ret;
}

#endif
