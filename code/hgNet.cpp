
#include "hgCircuit.h"
#include "hgRouter.h"
#include "hgTypeDef.h"

bool HGR::Net::is_routed()
{
    cout << "net id : " << id << endl;
    return get_topology(id)->routed;
}


bool HGR::Net::is_single_pin()
{
    return terminals.size() == 1 ? true : false;
}

bool HGR::Net::is_double_pin()
{
    return terminals.size() == 2 ? true : false;
}

bool HGR::Net::is_multi_pin()
{
    return terminals.size() > 2 ? true : false;
}


void HGR::Net::get_routing_space(int _bufferDistance, set<int> &_rGuide, set<int> &_rSpace)
{
    for(size_t i=0; i < guides.size(); i++)
    {
        int layer = guides[i].first;
        Rect<int> guideRect     = guides[i].second;
        Rect<int> bufferRect    = buffer(guideRect, _bufferDistance);

        grid->intersects(layer-1, bufferRect, _rSpace);
        grid->intersects(layer,   bufferRect, _rSpace);
        grid->intersects(layer+1, bufferRect, _rSpace);
        grid->intersects(layer,   guideRect,  _rGuide);
    }
}






