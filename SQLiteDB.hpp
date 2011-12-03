/*
Little C++ SQLite wrapper.
Author: Will Timoney
http://wrmsr.com

C++ifies the most common db ops but hides nothing from you.

Public domain. Enjoy.
*/

#ifndef __SQLITEDB_H__
#define __SQLITEDB_H__

#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include <sqlite3.h>

class SQLiteDB {
public:
  class Exception : std::exception {
  public:
    Exception(int _rc) :
      rc(_rc) {
      std::stringstream cvt;
      cvt << rc;
      msg = cvt.str();
    }

    Exception(const char* _msg) :
      msg(_msg) {
    }

    ~Exception() throw() {
    }

    int getCode() const {
      return rc;
    }

    const char* getMessage() const {
      return msg.c_str();
    }

  protected:
    int rc;
    std::string msg;
  };
  
  typedef int (*Callback)(void*, int, char**, char**);
  
  SQLiteDB(const char* path, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, const char* zVfs = NULL) :
    conn(NULL) {
    int rc = sqlite3_open_v2(path, &conn, flags, zVfs);
    if(rc)
      throw Exception(rc);
  }

  ~SQLiteDB() {
    if(conn)
      sqlite3_close(conn);
  }

  sqlite3* getConn() const {
    return conn;
  }

  void exec(const char* sql, Callback cb = NULL, void* arg = NULL) {
    char* error = NULL;
    int rc = sqlite3_exec(conn, sql, cb, arg, &error);
    if(error) {
      Exception ex(error);
      sqlite3_free(error);
      throw ex;
    }
    if(rc)
      throw Exception(rc);
  }

  std::string execStr(const char* sql) {
    std::string ret;
    
    Cursor c(*this, sql);
    
    if(c.read() && c.getNumCols() > 0)
      ret = c[0];
    
    return ret;
  }

  long long execInt(const char* sql) {
    Cursor c(*this, sql);
    
    if(!c.read() || c.getNumCols() < 1)
      return -1;
    
    return strtoll(c[0], NULL, 10);
  }

  unsigned long long execUInt(const char* sql) {
     Cursor c(*this, sql);
    
    if(!c.read() || c.getNumCols() < 1)
      return -1;
    
    return strtoull(c[0], NULL, 10);
  }   
  
  typedef std::map<std::string, std::string> ValueMap;

  class Cursor {
  public:
    Cursor(SQLiteDB& _db, const char* sql) :
      db(_db), stmt(NULL), pos(0) {
      int rc = sqlite3_prepare_v2(db.getConn(), sql, -1, &stmt, NULL);
      if(rc)
        throw Exception(rc);
    }

    ~Cursor() {
      if(stmt)
        sqlite3_finalize(stmt);
    }

    SQLiteDB& getDB() const {
      return db;
    }

    size_t getPos() const {
      return pos;
    }

    size_t getNumCols() const {
      return cols.size();
    }

    bool read() {
      int rc = sqlite3_step(stmt);
      if(rc != SQLITE_ROW && rc != SQLITE_DONE)
        throw Exception(rc);

      if(pos++ < 1) {
        size_t numCols = sqlite3_column_count(stmt);

        cols.reserve(numCols);
        vals.reserve(numCols);

        for(size_t i = 0; i < numCols; i++) {
          const char* col = sqlite3_column_name(stmt, i);

          cols.push_back(col);
          colIdxs[std::string(col)] = i;
        }
      }

      if(rc == SQLITE_DONE)
        return false;

      vals.clear();

      for(size_t i = 0; i < getNumCols(); i++) {
        const char* val = (const char*)sqlite3_column_text(stmt, i);

        vals.push_back(val);
      }

      return true;
    }

    int getColIdx(const char* col) const {
      std::map<std::string, int>::const_iterator i = colIdxs.find(std::string(col));
      
      if(i != colIdxs.end())
        return (*i).second;
      
      return -1;
    }

    const char* getVal(size_t idx) const {
      if(idx < 0 || idx > getNumCols())
        return NULL;

      return vals[idx];
    }

    const char* getVal(const char* col) const {
      return getVal(getColIdx(col));
    }

    const char* operator[](int idx) const {
      return getVal(idx);
    }

    const char* operator[](const char* col) const {
      return getVal(col);
    }

    ValueMap getVals() const {
      ValueMap ret;

      for(size_t i = 0; i < getNumCols(); i++)
        ret[std::string(cols[i])] = getVal(i);

      return ret;
    }

  private:
    Cursor(const Cursor& src);
    
    SQLiteDB& db;
    sqlite3_stmt* stmt;

    size_t pos;

    std::vector<const char*> cols;
    std::map<std::string, int> colIdxs;

    std::vector<const char*> vals;
  };

  std::auto_ptr<Cursor> execCursor(const char* sql) {
    return std::auto_ptr<Cursor>(new Cursor(*this, sql));
  }

  ValueMap execOne(const char* sql) {
    Cursor c(*this, sql);
    
    if(!c.read()) {
      ValueMap ret;
      return ret;
    }
    
    return c.getVals();
  }

private:
  sqlite3* conn;
};

#endif
