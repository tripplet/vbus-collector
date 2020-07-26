#include <sqlite3.h>
#include <stdio.h>

#include "sqlite.h"

static sqlite3 *db = NULL;

void sqlite_print_error(char* error_msg)
{
  printf("SQL error: %s\n", error_msg);
  sqlite3_free(error_msg);
}

bool sqlite_open(const char *path)
{
  if (sqlite3_open(path, &db) != 0)
  {
    printf("Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite_close();
    return false;
  }

  return true;
}


void sqlite_close()
{
  if (db != NULL)
  {
    sqlite3_close(db);
  }
}


bool sqlite_exec(char* sql)
{
  char *error_msg;

  if (sqlite3_exec(db, sql, NULL, 0, &error_msg) != SQLITE_OK) {
    sqlite_print_error(error_msg);
    return false;
  }

  return true;
}


bool sqlite_insert_data(Data_Packet* packet)
{
  char *error_msg;
  char sql_buffer[256];

  sprintf(sql_buffer, "INSERT INTO data "
    "(system_time, temp1, temp2, temp3, temp4, pump1, pump2, hours1, hours2) VALUES "
    "(\"%02d:%02d\", %.1f, %.1f, %.1f, %.1f, %d, %d, %d, %d);",
    packet->bsPlusPkt.SystemTime / 60,
    packet->bsPlusPkt.SystemTime % 60,
    packet->bsPlusPkt.TempSensor1 * 0.1,
    packet->bsPlusPkt.TempSensor2 * 0.1,
    packet->bsPlusPkt.TempSensor3 * 0.1,
    packet->bsPlusPkt.TempSensor4 * 0.1,
    packet->bsPlusPkt.PumpSpeed1,
    packet->bsPlusPkt.PumpSpeed2,
    packet->bsPlusPkt.OperatingHoursRelay1,
    packet->bsPlusPkt.OperatingHoursRelay2);

  if (sqlite3_exec(db, sql_buffer, NULL, 0, &error_msg) != 0)
  {
    sqlite_print_error(error_msg);
    return false;
  }

  return true;
}


bool sqlite_create_table()
{
  char *error_msg;
  char sql_create_table[] = "CREATE TABLE IF NOT EXISTS data ("
    "\"id\"          INTEGER PRIMARY KEY AUTOINCREMENT,"
    "\"time\"        DEFAULT CURRENT_TIMESTAMP NOT NULL,"
    "\"system_time\" TEXT NOT NULL,"
    "\"temp1\"       DOUBLE NOT NULL,"
    "\"temp2\"       DOUBLE NOT NULL,"
    "\"temp3\"       DOUBLE NOT NULL,"
    "\"temp4\"       DOUBLE NOT NULL,"
    "\"pump1\"       INTEGER NOT NULL,"
    "\"pump2\"       INTEGER NOT NULL,"
    "\"hours1\"      INTEGER NOT NULL,"
    "\"hours2\"      INTEGER NOT NULL);";

  if (sqlite3_exec(db, sql_create_table, NULL, 0, &error_msg) != 0)
  {
    sqlite_print_error(error_msg);
    return false;
  }

  return true;
}
