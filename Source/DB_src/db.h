/*
 * Интерфейс к базе данных SQLite3, функции передают SQL команды работы с таблицами, записями.
 */

#pragma once

#include <string>
#include <typeinfo>
#include <ctime>
#include <assert.h>
#include <vector>

#include "sqlite3.h"
#include "person.h"

#define CREATE_TABLES           "BEGIN TRANSACTION;                     \
        CREATE TABLE IF NOT EXISTS `ROOTTABLE` (                     \
        `ROOTID`        INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,     \
        `NAME`          TEXT NOT NULL,                                  \
        `TABLENAME`     TEXT NOT NULL                                   \
        );"

#define INSERT_LOG_TABLE_FORMAT     "CREATE TABLE IF NOT EXISTS `%s` (  \
        `ENTRYID`         INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,     \
        `NAME`            TEXT NOT NULL,                                  \
        `DATEOFBIRTH`     INTEGER NOT NULL,                               \
        `ISALIVE`         TEXT NOT NULL,                                  \
        `DATEOFDEATH`     INTEGER NOT NULL,                               \
        `INFO`            TEXT NOT NULL,                                  \
        `BIRTHPLACE`      TEXT NOT NULL,                                  \
        `PHOTO`           LONG TEXT NOT NULL,                             \
        `SEX`             TEXT NOT NULL,                                  \
        `FATHERID`        INTEGER NOT NULL,                               \
        `MOTHERID`        INTEGER NOT NULL,                               \
        `CHILDRENCNT`     INTEGER NOT NULL,                               \
        `CHILDRENID`      TEXT NOT NULL,                               \
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
    int showTable(const char *tableName);
    void databaseError();

    int beginTransaction(sqlite3 *db);
    int endTransaction(sqlite3 *db);

    int createRoot(std::string rootName, std::string& tableName);
    int selectRoot(std::string rootName, std::string& tableName);
    int addPerson(std::string tableName, Person * person, int * id);
    int getListOfRoots(std::vector<std::string> &rootList, std::vector<std::string> &tableList, std::string format = "'%'");

    int createLogTable(std::string logtable, std::string utc, std::string keydataid);
    int getLogIDListByLogTable(std::string logtable, std::vector<std::string> &itemList);
    int getLogTableList(std::vector<std::string> &itemList, std::vector<std::string> &utcList, std::string format = "'%'");
    int getKeyDataIDbyLogName(std::string logName, std::string &keyDataID);
    int insertLogItem(std::string logtable, std::string iv,
                        std::string utc, std::string data, std::string mac);
    int insertLogItem(std::string  logtable, std::string logid, std::string iv,
                            std::string utc, std::string data, std::string mac);
    int getLogDataByID(std::string logtable, std::string logid, std::string &iv,
                        std::string &utc, std::string &data, std::string &mac);
    int deleteLogTable(std::string logtable);
    int deleteLogItemByID(std::string logtable, std::string logid);

    
    int getINIIDList(std::vector<std::string> &itemList);
    int insertINIItem(std::string iniid, std::string iv, std::string data, std::string mac, std::string utc, std::string keydataid);
    int getINIDataByID(std::string iniid, std::string &iv, std::string &data, std::string &mac, std::string &utc, std::string &keydataid);
    int deleteINIItemByID(std::string iniid);

    
    int getFileIDList(std::vector<std::string> &itemList);
    int insertFileItem(std::string fileid, std::string iv, std::string data, std::string mac, std::string utc, std::string keydataid);
    int getFileDataByID(std::string fileid, std::string &iv, std::string &data, std::string &mac, std::string &utc, std::string &keydataid);
    int deleteFileItemByID(std::string fileid);

    
    int getDefaultMKID(std::string &mkid, std::string &utc_expireTime);
    int insertDefaultMKID(std::string mkid, std::string utc_expireTime);
    int deleteDefaultMKID();
    
    
    int getLastPINChangeItem(std::string SerialNum, std::string &utc);
    int insertLastPINChangeItem(std::string SerialNum, std::string utc);
    int deleteLastPINChangeItem(std::string SerialNum);
    
private:
    int finalizeSTMT()
    {
        int ret = 0;

        if (_pStmt)
        {
            ret = sqlite3_finalize(_pStmt);
            _pStmt = nullptr;
        }
        return ret;
    }

private:
    std::string _dbPath;
    sqlite3 *_db;
    sqlite3_stmt *_pStmt;
};

