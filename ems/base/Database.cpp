#include "Database.h"
#include "boost/assert.hpp"
#include <filesystem>

using namespace std;
using namespace ems;
using namespace ems::base;

sqlite::database Database::open(const string& filepath)
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + filepath };
    sqlite::database UserDb(filename);
    return UserDb;
}
