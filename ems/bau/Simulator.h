#include <stdint.h>
#include <vector>
#include <map>

namespace ems
{
namespace bau
{
using namespace std;

class Simulator
{
public:
    Simulator();
    vector<uint8_t> getRespondFrame(const vector<uint8_t>& request);
private:
    void create_bingjiStatus();
    void create_bauStatus();
    void create_bauPowerOff();
    void create_bauQuickStartup();

    void create_bcuStatus(const int bcuIndex);
    void create_bcuSetRelayGridOff(const int bcuIndex);
    void create_bcuSetRelayGridOn(const int bcuIndex);
    void create_bmuCellvolt(const int bcuIndex, const int bmuIndex);
    void create_bmuCelltem(const int bcuIndex, const int bmuIndex);

    void insertCommunicateInstance(const vector<uint8_t>& request, const vector<uint8_t>& respond);
    map<vector<uint8_t>, vector<uint8_t>> requestRespondMap_;
};

}//namespace pcs
}//namespace ems
