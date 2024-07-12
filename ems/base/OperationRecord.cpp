#include "OperationRecord.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "utils/Miscellaneous.h"
#include "utils/datetime.h"

using namespace ems;
using namespace ems::base;

void OperationRecord::insertRecord(const string& status, const string& content, const string& type, const string& username)
{
try
{
    const string nowTimestamp = datetime::getCurrentTimestamp();
    
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/OperationRecord.sqlite" };
    sqlite::database OperationRecordDb(filename);

    OperationRecordDb << "INSERT INTO OPERATION_RECORD ("
                            "STATUS, "
                            "CONTENT, TYPE, "
                            "CREATE_TIME, USERNAME) "
                            "VALUES (?, ?, ?, ?, ?);"
                        << status
                        << content << type
                        << nowTimestamp << username;
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}
