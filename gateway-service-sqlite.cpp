#include <sstream>
#include <iostream>
#include "gateway-service-sqlite.h"
#include "lorawan-error.h"
#include "lorawan-string.h"
#include "ip-address.h"
#include "file-helper.h"

#ifdef ESP_PLATFORM
#include <iostream>
#include "platform-defs.h"
#endif

static int tableCallback(
    void *env,
    int columns,
    char **value,
    char **column
)
{
    if (!env)
        return 0;
    std::vector<std::vector<std::string>> *table = (std::vector<std::vector<std::string>> *) env;

    std::vector<std::string> line;
    for (int i = 0; i < columns; i++) {
        line.push_back(value[i] ? value[i] : "");
    }
    table->push_back(line);
    return 0;
}

static int rowCallback(
    void *env,
    int columns,
    char **value,
    char **column
)
{
    if (!env)
        return 0;
    std::vector<std::string> *row = (std::vector<std::string> *) env;
    if (!row->empty())
        return 0;   // just first row
    for (int i = 0; i < columns; i++) {
        row->push_back(value[i] ? value[i] : "");
    }
    return 0;
}

SqliteGatewayService::SqliteGatewayService()
    : db(nullptr)
{

}

SqliteGatewayService::~SqliteGatewayService() = default;

/**
 * request device identifier by network address. Return 0 if success, retval = EUI and keys
 * @param retval device identifier
 * @param devaddr network address
 * @return CODE_OK- success
 */
int SqliteGatewayService::get(
    GatewayIdentity &retVal,
    const GatewayIdentity &request
)
{
    if (!db)
        return ERR_CODE_DB_DATABASE_NOT_FOUND;
    char *zErrMsg = nullptr;
    std::stringstream statement;
    statement << "SELECT id, addr FROM gateway WHERE ";
    if (request.gatewayId)
        statement << "id = '" << gatewayId2str(request.gatewayId) << "'";
    else
        statement << "addr = '" << sockaddr2string(&request.sockaddr) << "'";
    std::vector<std::string> row;
    int r = sqlite3_exec(db, statement.str().c_str(), rowCallback, &row, &zErrMsg);
    if (r != SQLITE_OK) {
        if (zErrMsg) {
            sqlite3_free(zErrMsg);
        }
        return ERR_CODE_DB_SELECT;
    } else {
        if (row.size() < 2)
            return ERR_CODE_BEST_GATEWAY_NOT_FOUND;
    }
    retVal.gatewayId = string2gatewayId(row[0]);
    string2sockaddr(&retVal.sockaddr, row[1]);
    return CODE_OK;
}

// List entries
int SqliteGatewayService::list(
    std::vector<GatewayIdentity> &retVal,
    size_t offset,
    size_t size
) {
    if (!db)
        return ERR_CODE_DB_DATABASE_NOT_FOUND;
    char *zErrMsg = nullptr;
    std::stringstream statement;
    statement << "SELECT id, addr FROM gateway LIMIT " << size << " OFFSET " << offset;
    std::vector<std::vector<std::string>> table;
    int r = sqlite3_exec(db, statement.str().c_str(), tableCallback, &table, &zErrMsg);
    if (r != SQLITE_OK) {
        if (zErrMsg) {
            sqlite3_free(zErrMsg);
        }
        return ERR_CODE_DB_SELECT;
    }
    for (auto row : table) {
        if (row.size() < 2)
            continue;
        GatewayIdentity gi;
        gi.gatewayId = string2gatewayId(row[0]);
        string2sockaddr(&gi.sockaddr, row[1]);
        retVal.push_back(gi);
    }
    return CODE_OK;
}

// Entries count
size_t SqliteGatewayService::size()
{
    if (!db)
        return 0;
    char *zErrMsg = nullptr;
    std::string statement = "SELECT count(id) FROM gateway";
    std::vector<std::string> row;
    int r = sqlite3_exec(db, statement.c_str(), rowCallback, &row, &zErrMsg);
    if (r != SQLITE_OK) {
        if (zErrMsg) {
            sqlite3_free(zErrMsg);
        }
        return 0;
    } else {
        if (row.empty())
            return 0;
    }
    return strtoull(row[0].c_str(), nullptr, 10);
}

/**
 * UPSERT SQLite >= 3.24.0
 * @param request gateway identifier or address
 * @return 0- success
 */
int SqliteGatewayService::put(
    const GatewayIdentity &request
)
{
    if (!db)
        return ERR_CODE_DB_DATABASE_NOT_FOUND;
    char *zErrMsg = nullptr;
    std::stringstream statement;
    statement << "INSERT INTO gateway (id, addr) VALUES ('"
        << gatewayId2str(request.gatewayId) << "', '"
        << sockaddr2string(&request.sockaddr)
        << "') ON CONFLICT(id) DO UPDATE SET addr=excluded.addr";
    int r = sqlite3_exec(db, statement.str().c_str(), nullptr, nullptr, &zErrMsg);
    if (r != SQLITE_OK) {
        if (zErrMsg) {
            sqlite3_free(zErrMsg);
        }
        return ERR_CODE_DB_INSERT;
    }
    return CODE_OK;
}

int SqliteGatewayService::rm(
    const GatewayIdentity &request
)
{
    if (!db)
        return ERR_CODE_DB_DATABASE_NOT_FOUND;
    char *zErrMsg = nullptr;
    std::stringstream statement;
    statement << "DELETE FROM gateway WHERE ";
    if (request.gatewayId)
        statement << "id = '" << gatewayId2str(request.gatewayId) << "'";
    else
        statement << "addr = '" <<  sockaddr2string(&request.sockaddr) << "'";
    int r = sqlite3_exec(db, statement.str().c_str(), nullptr, nullptr, &zErrMsg);
    std::cerr << statement.str() << std::endl;
    if (r != SQLITE_OK) {
        if (zErrMsg) {
            sqlite3_free(zErrMsg);
        }
        return ERR_CODE_DB_EXEC;
    }
    return CODE_OK;
}

/**
 * "CREATE DATABASE IF NOT EXISTS \"gateway\" USE \"db_name\"",
 */
static std::string SCHEMA_STATEMENT[] {
        "CREATE TABLE \"gateway\" (\"id\" TEXT NOT NULL PRIMARY KEY, \"addr\" TEXT NOT NULL)",
        "CREATE INDEX \"gateway_key_addr\" ON \"gateway\" (\"addr\")"
};

static int createDatabaseFile(
    const std::string &fileName
)
{
    sqlite3 *db;
    int r = sqlite3_open_v2(fileName.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (r)
        return r;
    char *zErrMsg = nullptr;
    for (auto s : SCHEMA_STATEMENT) {
        r = sqlite3_exec(db, s.c_str(), nullptr, nullptr, &zErrMsg);
        if (r != SQLITE_OK) {
            if (zErrMsg) {
                sqlite3_free(zErrMsg);
            }
            break;
        }
    }
    sqlite3_close(db);
    return r;
}

int SqliteGatewayService::init(
    const std::string &databaseName,
    void *database
)
{
    dbName = databaseName;
    if (database) {
        // use external db
        db = (sqlite3 *) database;
        return CODE_OK;
    }
    if (!file::fileExists(dbName)) {
        int r = createDatabaseFile(dbName);
        if (r)
            return r;
    }
    int r = sqlite3_open(dbName.c_str(), &db);
    if (r) {
        db = nullptr;
        return ERR_CODE_DB_DATABASE_OPEN;
    }
    return CODE_OK;
}

void SqliteGatewayService::flush()
{
    // re-open database file
    // external db closed
    if (db)
        sqlite3_close(db);
    sqlite3_open(dbName.c_str(), &db);
}

void SqliteGatewayService::done()
{
    int r = sqlite3_close(db);
    db = nullptr;
}
