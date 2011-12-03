#include <iostream>

#include <SQLiteDB.hpp>

int main() {
  try {
    unlink("test.db");
    SQLiteDB db("test.db");

    db.exec("create table test(id int primary key, value int);");

    for(int i = 0; i < 100; i++) {
      std::stringstream sql;
      sql << "insert into test values(" << i << ", " << i * 2 << ");";
      db.exec(sql.str().c_str());
    }

    std::cout << "Max: " << db.execInt("select max(value) from test;") << std::endl;

    SQLiteDB::Cursor cursor(db, "select * from test order by value desc;");
    while(cursor.read())
      std::cout << cursor["id"] << " = " << cursor["value"] << std::endl;

    //some arbitrary op to show that you can easily get the sqlite3 object and do whatever to it directly
    sqlite3_interrupt(db.getConn());
  }
  catch(SQLiteDB::Exception ex) {
    std::cout << "SQLite exception: " << ex.getMessage() << std::endl;
  }
  
  return 0;
}
