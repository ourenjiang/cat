#include <stdint.h>
#include <vector>
#include <map>

namespace ems
{
namespace pcs
{
using namespace std;

class Simulator
{
public:
    Simulator();
    vector<uint8_t> getRespondFrame(const vector<uint8_t>& request);
private:
    void create_0406_0460();
    void create_0474_04D0();

    void insertCommunicateInstance(const vector<uint8_t>& request, const vector<uint8_t>& respond);
    map<vector<uint8_t>, vector<uint8_t>> requestRespondMap_;
};

}//namespace pcs
}//namespace ems
