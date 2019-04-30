/*
 * Интерфейс к базе данных SQLite3, функции передают SQL команды работы с таблицами, записями.
 */

#ifdef DATABASE

#pragma once

#include <string>
#include <typeinfo>
#include <ctime>
#include <assert.h>
#include <vector>

#include "sqlite3.h"

#define CREATE_TABLES           "BEGIN TRANSACTION;                     \
        CREATE TABLE IF NOT EXISTS `ROOTTABLE` (                     \
        `ROOTID`        INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,     \
        `NAME`          TEXT NOT NULL,                                  \
        `TABLENAME`     TEXT NOT NULL                                   \
        );"

#define INSERT_ROOT_TABLE_FORMAT     "CREATE TABLE IF NOT EXISTS `%s` (  \
        `ENTRYID`         INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,     \
        `ID`              INTEGER NOT NULL ,     \
        `NAME`            TEXT NOT NULL,                                  \
        `DATEOFBIRTH`     TEXT NOT NULL,                               \
        `ISALIVE`         TEXT NOT NULL,                                  \
        `DATEOFDEATH`     TEXT NOT NULL,                               \
        `INFO`            TEXT NOT NULL,                                  \
        `BIRTHPLACE`      TEXT NOT NULL,                                  \
        `PHOTO`           LONG TEXT NOT NULL,                             \
        `SEX`             TEXT NOT NULL,                                  \
        `FATHERID`        INTEGER NOT NULL,                               \
        `MOTHERID`        INTEGER NOT NULL,                               \
        `CHILDRENCNT`     INTEGER NOT NULL,                               \
        `CHILDRENID`      TEXT NOT NULL                               \
        );"

//SELECT * FROM LOGLIST WHERE Tablename LIKE 'adminlog%'
//SELECT * FROM LOGLIST WHERE Tablename LIKE '%21122018%'

#define DB_PATH                         "family.db"

#ifndef F_OK
# define F_OK 0
#endif
#ifndef R_OK
# define R_OK 4
#endif
#ifndef W_OK
# define W_OK 2
#endif

#ifndef SQLITE_VFS_BUFFERSZ
#define SQLITE_VFS_BUFFERSZ             8192
#endif


#pragma pack(push, 1)

#pragma pack(pop)


class DB
{
public:
    DB(const char *dbpath = DB_PATH);
    virtual ~DB();

    void setDBPath(const char *dbpath);
    int openDB();
    int closeDB();
    int checkDB();

    static int dbCallback(void *count, int argc, char **argv, char **azColName);
    int createTables();
    void databaseError();

    int beginTransaction(sqlite3 *db);
    int endTransaction(sqlite3 *db);

    int createRoot(std::string rootName, std::string tableName);
    int addPerson(std::string tableName, uint32_t id, std::string name, std::string birthDate, std::string isAlive, std::string deathDate, std::string info, std::string birthPlace, std::string photo, std::string sex, uint32_t fatherId, uint32_t motherId, uint32_t childrenCnt, std::string childrenID);
    int getListOfRoots(std::vector<std::string> &rootList, std::vector<std::string> &tableList, std::string format = "'%'");

    int finalizeSTMT(sqlite3_stmt *_pStmt)
    {
        int ret = 0;

        if (_pStmt)
        {
            ret = sqlite3_finalize(_pStmt);
            _pStmt = nullptr;
        }
        return ret;
    }

    sqlite3 *_db;
private:
    std::string _dbPath;
    bool _bOpened;
//    sqlite3_stmt *_pStmt;
};


class dbTransactor
{
   DB * _db;
   sqlite3_stmt *_pStmt;
public:
   dbTransactor(DB * db, sqlite3_stmt * pStmt) : _db(db), _pStmt(pStmt)
   {
      if (_db)
         _db->beginTransaction(_db->_db);
   }
   ~dbTransactor()
   {
      if ((_db) && (_pStmt))
      {
         _db->finalizeSTMT(_pStmt);
         _db->endTransaction(_db->_db);
      }
   }
};

#endif
