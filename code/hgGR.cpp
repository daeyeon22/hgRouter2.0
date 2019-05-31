#include "hgRouter.h"
#include "hgTypeDef.h"
#include "hgHeap.h"
#include "hgGeometry.h"
#include <boost/format.hpp>
#include <tuple>
#include <queue>
#include <deque>

#include <boost/foreach.hpp>
#include <boost/config.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_base_map.hpp>

using namespace HGR;

ostream& operator << (ostream& _os, Segment* _seg)
{
    Gcell* c1 = grid->gcell(_seg->g1);
    Gcell* c2 = grid->gcell(_seg->g2);

    int lx = min(c1->x, c2->x);
    int ly = min(c1->y, c2->y);
    int ux = max(c1->x, c2->x);
    int uy = max(c1->y, c2->y);

    return _os << boost::format("Segment[%d] %s (%d %d) (%d %d)") % _seg->id % get_metal(_seg->layer)->name % lx % ly % ux % uy;
}

bool exist(set<int>& _set, int _elem)
{
    return _set.find(_elem) == _set.end() ? false : true;
}

bool HGR::fine_grained_routing()
{
    /*
    int prevCongestion = INT_MAX;

    for(int trial=0; trial < 5; trial++)
    {
        dense_hash_map<int, Topology> backup;
        backup.set_empty_key(INT_MAX);

        for(int i=0; i < num_nets(); i++)
        {
            if(trial!=0)
            {
                if(check_congest


            }


        }
    }
    */

    
    for(int i=0; i < num_nets(); i++)
    {
        Net* net = get_net(i);

        
        if(net->is_routed() || net->is_single_pin())
            continue;

        if(!route(i))
        {
            continue;
        }

        topology_generation(i);
    }
}



bool HGR::route(int _net, int _trial)
{
    Net* net = get_net(_net);

    if(net->is_single_pin())
        return false;
    
    
    int bufferDistance = (1 + 3*_trial) * min(grid->GCwidth, grid->GCheight);
    vector<int> terminals = net->terminals;
    set<int> rGuide, rSpace;


    //
    net->get_routing_space(bufferDistance, rGuide, rSpace);



    typedef pair<int,int> TwoPin;
    vector<pair<TwoPin, double>> twopins;

    for(size_t i=0; i < terminals.size(); i++)
    {
        for(size_t j=i+1; j < terminals.size(); j++)
        {
            double p2pDist = pin_to_pin_distance(terminals[i], terminals[j]);
            TwoPin pinPair = { terminals[i], terminals[j] };
            twopins.push_back( { pinPair, p2pDist } );
        }
    }


    sort(twopins.begin(), twopins.end(), [](const pair<TwoPin, double>& left, const pair<TwoPin, double>& right){
        return left.second < right.second;
            });


    // Route Twopin Net
    while(true)
    {
        if(twopins.size() == 0)
            return false;

        int p1 = twopins.begin()->first.first;
        int p2 = twopins.begin()->first.second;
        twopins.erase(twopins.begin());

        if(route_twopin_net(_net, p1, p2, rGuide, rSpace))
        {
            remove(terminals, p1);
            remove(terminals, p2);
            break;
        }
        else
        {
            cout << boost::format("Route Twopin net Failed... [%s] %s -> %s\n") %
                net->name % get_pin(p1)->name % get_pin(p2)->name;
        }
    }


    // Route Pin to Topology
    while(true)
    {
        if(terminals.size() == 0)
            break;

        int p = terminals[0];
        remove(terminals, p);
       
        if(!route_pin_to_tp(_net, p, rGuide, rSpace))
        {
            cout << boost::format("Route Pin to Topology Failed... [%s] %s\n") %
                net->name % get_pin(p)->name;
            return false;
        }
    }

    return true;
}



/*
void HGR::get_routing_space(int _net, int _bufferDistance, set<int> &_rGuide, set<int> &_rSpace)
{
    
    Net* net = get_net(_net);
    for(size_t i=0; i < net->guides.size(); i++)
    {
        int layer = net->guides[i].first;
        Rect<int> guideRect     = net->guides[i].second;
        Rect<int> bufferRect    = buffer(guideRect, _bufferDistance);

        grid->intersects(layer-1, bufferRect, _rSpace);
        grid->intersects(layer,   bufferRect, _rSpace);
        grid->intersects(layer+1, bufferRect, _rSpace);
        grid->intersects(layer,   guideRect,  _rGuide);
    }
}
*/
void HGR::HeapNode::init(int _id, bool _isSource)
{
    if(id == INT_MAX)
    {
        id = _id;
        depth = -1;
        backtrace = -1;
        est = DBL_MAX;
        act = DBL_MAX;
        if(_isSource)
        {
            depth = 0;
            backtrace = id; //-1;
            act = 0; //DBL_MAX;
            est = 0;
        }
    }
}
// Actual Cost
double HGR::HeapNode::get_act_cost(int _prevNode, double _prevActCost, set<int> &_rGuide)
{
    int g1 = _prevNode;
    int g2 = id;

    double _act = 
        _prevActCost;
        + grid->wirelength(g1, g2) 
        + grid->segment(g1, g2) 
        + grid->offguide(g2, _rGuide)
        + grid->npref(g1, g2)
        + grid->overflow(g2);
    return _act;
}

// Estimation Cost (Single Target)
double HGR::HeapNode::get_est_cost(int _targetNode)
{
    return 1.0 * grid->distance(id, _targetNode);
}

// Estimation Cost (Multi Target)
double HGR::HeapNode::get_est_cost(vector<int> &_targetNodes)
{
    double _est = DBL_MAX;
    for(auto target : _targetNodes)
    {
        _est = min(_est, grid->distance(id, target));
    }

    return _est;
}

void HGR::HeapNode::update(int _depth, int _backtrace, double _act, double _est)
{
    depth = _depth;
    backtrace = _backtrace;
    act = _act;
    est = _est;
}





/*
double HGR::HeapNode::get_cost(int _prevNode, double _prevActCost, vector<int> &_targetNodes, set<int> &_rGuide)
{
    int g1 = _prevNode;
    int g2 = id;
    int g3;

    double _act = 
        _prevActCost;
        + grid->wirelength(g1, g2) 
        + grid->segment(g1, g2) 
        + grid->offguide(g2, _rGuide)
        + grid->npref(g1, g2)
        + grid->overflow(g2);

    double _est = DBL_MAX;

    for(auto target : _targetNodes)
    {
        g3 = target;
        _est = min(_est, grid->distance(g2, g3));
    }

    return _act + _est;
}



void HGR::HeapNode::update(int _prevNode, int _prevDepth, double _prevActCost, vector<int> &_targetNodes, set<int> &_rGuide)
{
    int g1 = _parent->id;
    int g2 = id;
    int g3;

    act = _parent->act 
        + grid->wirelength(g1, g2) 
        + grid->segment(g1, g2) 
        + grid->offguide(g2, _rGuide)
        + grid->npref(g1, g2)
        + grid->overflow(g2);

    est = DBL_MAX; // grid->distance(g2, g3);

    for(auto target : _targets)
    {
        g3 = target->id;
        est = min(est, grid->distance(g2, g3));
    }
   
    depth = _parent->depth + 1;
    backtrace = _parent->id;
}

double HGR::HeapNode::get_cost(int _prevNode, double _prevActCost, int _targetNode, set<int> &_rGuide)
{
    int g1 = _prevNode;
    int g2 = id;
    int g3 = _targetNode;

    cout << g1 << " " << g2 << " " << g3 << endl;
    double _act = 
        _prevActCost
        + grid->wirelength(g1, g2) 
        + grid->segment(g1, g2) 
        + grid->offguide(g2, _rGuide)
        + grid->npref(g1, g2)
        + grid->overflow(g2);

    double _est = grid->distance(g2, g3);

    return _act + _est;
}

void HGR::HeapNode::update(int _prevNode, int _prevDepth, double _prevActCost, int _targetNode, set<int> &_rGuide)
{
    int g1 = _prevNode;
    int g2 = id;
    int g3 = _targetNode;

    act = _prevActCost
        + grid->wirelength(g1, g2) 
        + grid->segment(g1, g2) 
        + grid->offguide(g2, _rGuide)
        + grid->npref(g1, g2)
        + grid->overflow(g2);

    est =  grid->distance(g2, g3);
    
    depth = _prevDepth + 1;
    backtrace = _prevNode;
}
*/









bool HGR::route_pin_to_tp(int _net, int _pin, set<int> &_rGuide, set<int> &_rSpace)
{
    Net* net = get_net(_net);
    Pin* pin = get_pin(_pin);
    Topology* tp = get_topology(_net);

    dense_hash_map<int, HeapNode> nodes;
    nodes.set_empty_key(INT_MAX);
    Heap<HeapNode> heap;

    cout << boost::format("Route Pin to Topology [%s]\n") % net->name;

    vector<int> multiTarget = tp->gcells;
    //
    int minNode = INT_MAX;
    double minCost = DBL_MAX;
    bool hasSolution = false;

   
    if(!is_extended(_pin))
    {
        return false;
    }
    else
    {
        Ext *ext = get_extension(_pin);
        int n = grid->index(ext->layer, ext->access);

        //
        nodes[n].init(n, true);
        _rSpace.insert(n);

        //         
        if(exist(multiTarget,n) && minCost > nodes[n].cost())
        {
            hasSolution = true;
            minNode = n;
            minCost = nodes[n].cost();
        }

        heap.push(nodes[n]);
    }

    for(auto target : multiTarget)
    {
        _rSpace.insert(target);
    }



    typedef tuple<int,int,int> Coord3D;

    Coord3D adjV[] = { Coord3D(0,0,-1), Coord3D(0,0,1), Coord3D(0,1,0), Coord3D(0,-1,0) };
    Coord3D adjH[] = { Coord3D(0,0,-1), Coord3D(0,0,1), Coord3D(1,0,0), Coord3D(-1,0,0) };

    int n1, n2;
    int x1, y1, z1;
    int x2, y2, z2;
   
    ///////////////////
    while(!heap.empty())
    {
        HeapNode temp = heap.pop();
        n1 = temp.id;

        if(temp.cost() != nodes[n1].cost())
            continue;

        if(n1 == minNode)
            break;
        
        grid->coord(n1, x1, y1, z1);

        for(size_t i=0; i < 4; i++)
        {
            if(is_vertical(z1))
            {
                x2 = x1 + get<0>(adjV[i]);
                y2 = y1 + get<1>(adjV[i]);
                z2 = z1 + get<2>(adjV[i]);
            }
            else
            {
                x2 = x1 + get<0>(adjH[i]);
                y2 = y1 + get<1>(adjH[i]);
                z2 = z1 + get<2>(adjH[i]);
            }

            if(z2 == 0) 
                continue;

            if(grid->out_of_index(x2,y2,z2))
                continue;
            
            n2 = grid->index(x2, y2, z2);
            if(nodes.find(n2) == nodes.end())
            {
                nodes[n2].init(n2);
            }

            

            if(nodes[n1].backtrace == n2)
                continue;

            if(grid->cap(n2) == 0)
                continue;

            if(!exist(_rSpace, n2)) 
                continue;

            double act = nodes[n2].get_act_cost(n1, nodes[n1].act, _rGuide);
            double est = nodes[n2].get_est_cost(multiTarget);

            if(nodes[n2].cost() > act + est)
            {
                nodes[n2].update(nodes[n1].depth+1, n1, act, est);

                if(exist(multiTarget, n2) && minCost > nodes[n2].cost())
                {
                    hasSolution = true;
                    minNode = n2;
                    minCost = nodes[n2].cost();
                }

                heap.push(nodes[n2]);
            }
        }
    }



    if(hasSolution)
    {
        cout << "Found Solution!" << endl;

        int iterNode = minNode;

        Topology* tp = get_topology(_net);
        
        vector<int> gcells;
        while(true)
        {
            grid->gc_assign(iterNode);
            gcells.push_back(iterNode);

            if(iterNode == nodes[iterNode].backtrace)
                break;

            iterNode = nodes[iterNode].backtrace;
        }

        subnet_segmentation(_net, _pin, INT_MAX, gcells);
        return true;
    }
    else
    {
        cout << "No Soultion..." << endl;

        exit(0);
    }

    return false;

}




bool HGR::route_twopin_net(int _net, int _p1, int _p2, set<int> &_rGuide, set<int> &_rSpace)
{
    Net* net = get_net(_net);
    Pin* pin1 = get_pin(_p1);
    Pin* pin2 = get_pin(_p2);

    dense_hash_map<int, HeapNode> nodes;
    nodes.set_empty_key(INT_MAX);
    Heap<HeapNode> heap;
    //vector<int> multiSource;
    //vector<int> multiTarget;
    
    cout << boost::format("Route Twopin Net [%s] %s -> %s\n") %
        net->name % pin1->name % pin2->name;
    
    //
    int minNode = INT_MAX;
    int destNode = INT_MAX;
    double minCost = DBL_MAX;
    bool hasSolution = false;

    if(!is_extended(_p1) || !is_extended(_p2))
    {
        return false;
    }
    else
    {
        Ext *ext1 = get_extension(_p1);
        Ext *ext2 = get_extension(_p2);
        int n1 = grid->index(ext1->layer, ext1->access);
        int n2 = grid->index(ext2->layer, ext2->access);
        _rSpace.insert(n1);
        _rSpace.insert(n2);
        //cout << "Extension -> Gcell[n1] : " << n1 << endl;
        //cout << "Extension -> Gcell[n2] : " << n2 << endl;

        //
        destNode = n2;

        nodes[n1].init(n1, true);
        nodes[n2].init(n2); //, false);
        //         
        if(n1 == destNode)
        {
            if(minCost > nodes[n1].cost())
            {
                hasSolution = true;
                minNode = n1;
                minCost = nodes[n1].cost();
            }
        }

        heap.push(nodes[n1]);
    }

    typedef tuple<int,int,int> Coord3D;

    Coord3D adjV[] = { Coord3D(0,0,-1), Coord3D(0,0,1), Coord3D(0,1,0), Coord3D(0,-1,0) };
    Coord3D adjH[] = { Coord3D(0,0,-1), Coord3D(0,0,1), Coord3D(1,0,0), Coord3D(-1,0,0) };

    int n1, n2;
    int x1, y1, z1;
    int x2, y2, z2;
 
    ///////////////////
    while(!heap.empty())
    {
        HeapNode temp = heap.pop();
        
        n1 = temp.id;

        //cout << "Current Node : " << n1 << endl;

        if(temp.cost() != nodes[n1].cost())
            continue;

        if(n1 == minNode)
            break;
        
        grid->coord(n1, x1, y1, z1);

        for(size_t i=0; i < 4; i++)
        {
            if(is_vertical(z1))
            {
                x2 = x1 + get<0>(adjV[i]);
                y2 = y1 + get<1>(adjV[i]);
                z2 = z1 + get<2>(adjV[i]);
            }
            else
            {
                x2 = x1 + get<0>(adjH[i]);
                y2 = y1 + get<1>(adjH[i]);
                z2 = z1 + get<2>(adjH[i]);
            }
            
            if(z2 == 0)
                continue;

            if(grid->out_of_index(x2,y2,z2))
                continue;


            
            n2 = grid->index(x2, y2, z2);
            if(nodes.find(n2) == nodes.end())
            {
                nodes[n2].init(n2);
            }

            if(nodes[n1].backtrace == n2)
                continue;

            if(grid->cap(n2) == 0)
                continue;

            if(!exist(_rSpace, n2)) 
                continue;

            double act = nodes[n2].get_act_cost(n1, nodes[n1].act, _rGuide);
            double est = nodes[n2].get_est_cost(destNode);
            
            if(nodes[n2].cost() > act + est)
            {
                nodes[n2].update(nodes[n1].depth+1, n1, act, est);
                
                if(n2 == destNode && minCost > nodes[n2].cost())
                {
                    hasSolution = true;
                    minNode = n2; //nNode->id;
                    minCost = nodes[n2].cost(); //nNode->cost();
                }

                heap.push(nodes[n2]);
            }
        }
    }


    if(hasSolution)
    {
        cout << boost::format("Has Solution ! [%s]\n") % net->name;

        int iterNode = minNode;

        Topology* tp = get_topology(_net);
        
        vector<int> paths;
        while(true)
        {
            grid->gc_assign(iterNode);
            paths.push_back(iterNode);

            if(iterNode == nodes[iterNode].backtrace)
                break;

            iterNode = nodes[iterNode].backtrace;
        }

        subnet_segmentation(_net, _p1, _p2, paths);
        return true;
    }
    else
    {
       
        cout << "No Solution..." << endl;

        exit(0);
    }

    return false;
}


void HGR::subnet_segmentation(int _net, int _p1, int _p2, vector<int> &_gcells)
{

    cout << boost::format("Subnet Segmentation [%s]\n") % get_net(_net)->name;

    Topology* tp = get_topology(_net);

    int g1, g2, g3;
    int numSegs = 0;

    for(auto& it : _gcells)
    {
        //cout << "Gcell : " << it << endl;
        if(!exist(tp->gcells, it))
        {
            tp->gcells.push_back(it);
        }
    }


    if(_gcells.size() == 1)
    {
        g1 = _gcells[0];
        Segment* s1 = create_segment(_net);
        s1->g1 = g1;
        s1->g2 = g1;
        s1->layer = grid->gcell(g1)->z;
        s1->direction = grid->direction(g1, g1);
        //s1->ll = grid->gcell(g1)->rect.ll;
        //s1->ur = grid->gcell(g1)->rect.ur;
        s1->access = true;
        s1->pins.push_back(_p1);
        s1->exts.push_back(*get_extension(_p1));
        
        //seg sElem1(pt(grid->gcell(g1)->x, grid->gcell(g1)->y), pt(grid->gcell(g1)->x, grid->gcell(g1)->y));
        //tp->trees[s1->layer].insert({sElem1, s1->id});
        
        cout << s1 << endl;

        if(_p2 = INT_MAX)
        {
            /*
            g2 = grid->index(g1, 0, 0, 1);
            
            if(!exist(tp->gcells, g2))
            {
                tp->gcells.push_back(g2);
            }

            Segment *s2 = create_segment(_net);
            s2->g1 = g2;
            s2->g2 = g2;
            s2->layer = grid->gcell(g2)->z;
            s2->access = false;
            s2->direction = grid->direction(g2, g2);
            //s2->ll = grid->gcell(g2)->rect.ll;
            //s2->ur = grid->gcell(g2)->rect.ur;
            
            seg sElem2(pt(grid->gcell(g2)->x, grid->gcell(g2)->y), pt(grid->gcell(g2)->x, grid->gcell(g2)->y));
            tp->trees[s2->layer].insert({sElem2, s2->id});
            */
            
            Segment *s3 = create_segment(_net);
            s3->g1 = g1;
            s3->g2 = g1;
            s3->layer = grid->gcell(g1)->z;
            s3->direction = grid->direction(g1, g1);
            //s3->ll = grid->gcell(g1)->rect.ll;
            //s3->ur = grid->gcell(g1)->rect.ur;
            s3->access = true;
            s3->pins.push_back(_p2);
            s3->exts.push_back(*get_extension(_p2));

            //seg sElem3(pt(grid->gcell(g1)->x, grid->gcell(g1)->y), pt(grid->gcell(g1)->x, grid->gcell(g1)->y));
            //tp->trees[s3->layer].insert({sElem3, s3->id});
            //cout << s3 << endl;
        }
    }
    else
    {
        typedef pair<int,int> GcellPair;

        Gcell *c1, *c2, *c3;
        vector<GcellPair> gcellPairs;
        
        g1 = _gcells[0];
        for(size_t i=1; i < _gcells.size(); i++)
        {
            g2 = _gcells[i-1];
            g3 = _gcells[i];
            
            c1 = grid->gcell(g1);
            c2 = grid->gcell(g2);
            c3 = grid->gcell(g3);

            bool diffZ  = (c1->z != c3->z) ? true : false;
            bool diffXY = (c1->x != c3->x && c1->y != c3->y) ? true : false;

            if(diffZ | diffXY)
            {
                gcellPairs.push_back( {g1, g2} );
                g1 = (diffXY) ? g2 : g3;
            }

            if((i+1) == _gcells.size())
            {
                gcellPairs.push_back( {g1, g3} );
            }
        }


        if(gcellPairs.size() == 1 && _p2 != INT_MAX)
        {
            g1 = gcellPairs.begin()->first;
            g2 = gcellPairs.begin()->second;

            c1 = grid->gcell(g1);
            c2 = grid->gcell(g2);

            Segment* s1 = create_segment(_net);
            s1->g1 = g1;
            s1->g2 = g2;
            s1->layer = c1->z;
            s1->direction = grid->direction(g1, g2);
            s1->access = true;
            s1->pins.push_back(_p2);
            s1->exts.push_back(*get_extension(_p2));
            //s1->ll.x = min(c1->rect.ll.x, c2->rect.ll.x);
            //s1->ll.y = min(c1->rect.ll.y, c2->rect.ll.y);
            //s1->ur.x = max(c1->rect.ur.x, c2->rect.ur.x);
            //s1->ur.y = max(c1->rect.ur.y, c2->rect.ur.y);
            
            //int lx = min(c1->x, c2->x);
            //int ly = min(c1->y, c2->y);
            //int ux = max(c1->x, c2->x);
            //int uy = max(c1->y, c2->y);
            //seg sElem1(pt(lx, ly), pt(ux, uy));
            //tp->trees[s1->layer].insert({sElem1, s1->id});
            
            Segment* s2 = create_segment(_net);
            s2->g1 = g2;
            s2->g2 = g2;
            s2->layer = c2->z;
            s2->direction = grid->direction(g2, g2);
            s2->access = true;
            s2->pins.push_back(_p1);
            s2->exts.push_back(*get_extension(_p1));
            //s2->ll = c2->rect.ll;
            //s2->ur = c2->rect.ur;
 
            //seg sElem2(pt(c2->x, c2->y), pt(c2->x, c2->y));
            //tp->trees[s2->layer].insert({sElem2, s2->id});
            //cout << "g1 : " << g1 << " g2 : " << g2 << endl;
            //cout << s1->layer << " " << s2->layer << endl;
            //cout << "Gcell : " << grid->gcell(g1)->x << " " << grid->gcell(g1)->y << " " << grid->gcell(g1)->z << endl;
            //cout << "Gcell : " << grid->gcell(g2)->x << " " << grid->gcell(g2)->y << " " << grid->gcell(g2)->z << endl;
            //cout << "!" << endl; 
            //cout << s1 << endl;
            //cout << s2 << endl;
            //cout << "@" << endl;

        }
        else
        {
            for(size_t i=0; i < gcellPairs.size(); i++)
            {
                g1 = gcellPairs[i].first;
                g2 = gcellPairs[i].second;
                c1 = grid->gcell(g1);
                c2 = grid->gcell(g2);

                Segment* s = create_segment(_net);
                s->g1 = g1;
                s->g2 = g2;
                s->layer = c1->z;
                s->direction = grid->direction(g1, g2);

                if(i==0 && _p2 != INT_MAX)
                {
                    s->access = true;
                    s->pins.push_back(_p2);
                    s->exts.push_back(*get_extension(_p2));
                }

                if(i==gcellPairs.size()-1)
                {
                    s->access = true;
                    s->pins.push_back(_p1);
                    s->exts.push_back(*get_extension(_p1));
                }

                //s->ll.x = min(c1->rect.ll.x, c2->rect.ll.x);
                //s->ll.y = min(c1->rect.ll.y, c2->rect.ll.y);
                //s->ur.x = max(c1->rect.ur.x, c2->rect.ur.x);
                //s->ur.y = max(c1->rect.ur.y, c2->rect.ur.y);
            
                //int lx = min(c1->x, c2->x);
                //int ly = min(c1->y, c2->y);
                //int ux = max(c1->x, c2->x);
                //int uy = max(c1->y, c2->y);
                //seg sElem(pt(lx, ly), pt(ux, uy));
                //tp->trees[s->layer].insert({sElem, s->id});
                //cout << s << endl;
            }
        }
    }

}

void HGR::topology_generation(int _net)
{
    using EdgeWeightProperty = boost::property<boost::edge_weight_t, int>;
    using GraphType = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EdgeWeightProperty>;
    using PredecessorContainer = vector<boost::graph_traits<GraphType>::vertex_descriptor>;
    using EdgeDescriptor = boost::graph_traits<GraphType>::edge_descriptor;

    Topology* tp = get_topology(_net);

    if(tp->segments.size() == 0) return;

    GraphType G(tp->segments.size());

    int dz[] = { -1, 0, 1 };
    int lx, ly, ux, uy, layer;
    int edgeType12, edgeType13, edgeType23;
    int cutL13, cutL23;
    int n1, n2, n3, w;
    Gcell *c1, *c2;
    Segment *s, *s1, *s2, *s3;

    for(int i=0; i < (int)tp->segments.size(); i++)
    {
        s = &tp->segments[i];
        c1 = grid->gcell(s->g1);
        c2 = grid->gcell(s->g2);
        lx = min(c1->x, c2->x);
        ly = min(c1->y, c2->y);
        ux = max(c1->x, c2->x);
        uy = max(c1->y, c2->y);
        seg sElem(pt(lx, ly), pt(ux, uy));
        tp->trees[s->layer].insert({sElem, s->id});
        cout << s << endl;
    }


    for(int i=0; i < (int)tp->segments.size(); i++)
    {
        s1 = &tp->segments[i];
        
        //cout << s1->g1 << " " << s1->g2 << endl;
        c1 = grid->gcell(s1->g1);
        c2 = grid->gcell(s1->g2);

        for(int j=0; j < 3; j++)
        {
            layer = s1->layer + dz[j];
            
            if(layer < 0 || layer >= num_layers())
                continue;


            lx = min(c1->x, c2->x);
            ly = min(c1->y, c2->y);
            ux = max(c1->x, c2->x);
            uy = max(c1->y, c2->y);
           
            SegRtree* tree = &tp->trees[layer];
            for(SegRtree::const_query_iterator it = tree->qbegin(bgi::intersects(seg(pt(lx, ly), pt(ux, uy))));
                    it != tree->qend(); it++)
            {
           
                s2 = &tp->segments[it->second];

                if(s1->id == s2->id)
                    continue;

                n1 = min(s1->id, s2->id);
                n2 = max(s1->id, s2->id);
                w = (s1->access || s2->access) ? 100 : 0;

                if(dz[j] == 0) 
                    w += (s1->direction == s2->direction) ? 30 : 5;
                else
                    w += (s1->direction == s2->direction) ? 50 : 1;

                boost::add_edge(n1, n2, EdgeWeightProperty(w), G);
            }
        }
    }


    vector<EdgeDescriptor> edges;
    boost::kruskal_minimum_spanning_tree(G, back_inserter(edges));

    for(vector<EdgeDescriptor>::iterator it = edges.begin(); it != edges.end(); it++)
    {
        n1 = boost::source(*it, G);
        n2 = boost::target(*it, G);

        s1 = &tp->segments[n1];
        s2 = &tp->segments[n2];
      
        if(s1->direction == s2->direction)
            edgeType12 = (s1->layer == s2->layer) ? PRL : VIA_PRL;
        else
            edgeType12 = (s1->layer == s2->layer) ? CROSS : VIA_CROSS;

        tp->add_edge(n1, n2, edgeType12);

        /*
        if(s1->direction == s2->direction)
        {
            s3 = create_segment(_net);
            n3 = s3->id;
            
            //cout << "n1 : " << n1 << " n2 : " << n2 << " n3 : " << n3 << endl; 
            if(s1->layer != s2->layer)
            {
                if(!is_preferred(s1->layer, s1->direction))
                {
                    s3->layer = s1->layer;
                    s3->direction = preferred(s1->layer);
                    edgeType13 = CROSS;
                    edgeType23 = VIA_CROSS;
                }
                else if(!is_preferred(s2->layer, s2->direction))
                {
                    s3->layer = s2->layer;
                    s3->direction = preferred(s2->layer);
                    edgeType13 = VIA_CROSS;
                    edgeType23 = CROSS;
                }
            }
            else
            {
                s3->layer = s1->layer + 1;
                s3->direction = preferred(s3->layer);
                edgeType13 = VIA_CROSS;
                edgeType23 = VIA_CROSS;
            }
            
            
            //cutL13 = max(s1->layer, s3->layer);
            //cutL23 = max(s2->layer, s3->layer);
            int lx1 = min(grid->gcell(s1->g1)->x, grid->gcell(s1->g2)->x);
            int lx2 = min(grid->gcell(s2->g1)->x, grid->gcell(s2->g2)->x);
            int ly1 = min(grid->gcell(s1->g1)->y, grid->gcell(s1->g2)->y);
            int ly2 = min(grid->gcell(s2->g1)->y, grid->gcell(s2->g2)->y);
            s3->g1 = grid->index(max(lx1, lx2), max(ly1, ly2), s3->layer);
            s3->g2 = grid->index(max(lx1, lx2), max(ly1, ly2), s3->layer);
            s3->net = s1->net;
            s3->direction = preferred(s3->layer);
            s3->assign = false;
            s3->access = false;

            //s3->ll = grid->gcell(s3->g1)->rect.ll;
            //s3->ur = grid->gcell(s3->g2)->rect.ur;
            
            tp->add_edge(n1, n3);
            tp->add_edge(n2, n3);
            tp->set_edge_type(n1, n3, edgeType13);
            tp->set_edge_type(n2, n3, edgeType23);
        }
        else
        {
            //cout << "n1 : " << n1 << " n2 : " << n2<< endl;
            
            edgeType12 = (s1->layer == s2->layer) ? CROSS : VIA_CROSS;
            tp->add_edge(n1, n2, edgeType12);
            tp->set_edge_type(n1, n2, edgeType12);
        }
        */
    }

//#ifdef DEBUG_TOPOLOGY
    cout << "Topology Generation <" << get_net(_net)->name << ">" <<  endl;

    for(size_t i=0; i < tp->segments.size(); i++)
    {
        cout << &tp->segments[i] << endl;
    }

    //for(size_t i=0; i < topologies[netid].edges.size(); i++)
    for(auto it : tp->edge)
    {
        Edge e = it.second;
        int n1 = e.s1;
        int n2 = e.s2;
        int type = e.type;
        //tp->get_edge_type(n1, n2);
        cout << "Edge : " << &tp->segments[n1] << " <--> " << &tp->segments[n2] << " ";

        switch(type)
        {
            case VIA_PRL    : cout << "VIA_PRL";   break;
            case VIA_CROSS  : cout << "VIA_CROSS"; break;
            case PRL        : cout << "PRL"; break;
            case CROSS      : cout << "CROSS"; break;
            default: break;
        }
        cout << endl;
    }
    cout << endl;
//#endif



}


bool HGR::Topology::check_parallel_edge_connectivity(int _layer)
{
    bool brokenEdge = false;
    int xl, xh, yl, yh;
    int edgeType;
    int g1, g2;
    Segment *s1, *s2, *s3;

    vector<edge_t> temp = edges;

    for(auto it : temp)
    {
        Edge* e = &edge[it];
        s1 = get_segment(net, e->s1);
        s2 = get_segment(net, e->s2);
        edgeType = e->type;

        if(edgeType != PRL)
            continue;

        if(s1->layer != _layer)
            continue;

        if(!s1->assign || !s2->assign)
        {
            
            cout << s1 << endl;
            cout << s2 << endl;
            cout << "Not Assigned!" << endl;
            continue;
        }
        
        if(s1->track != s2->track)
        { 
            xl = max( min(grid->gcell(s1->g1)->x, grid->gcell(s1->g2)->x), min(grid->gcell(s2->g1)->x, grid->gcell(s2->g2)->x) );
            xh = min( max(grid->gcell(s1->g1)->x, grid->gcell(s1->g2)->x), max(grid->gcell(s2->g1)->x, grid->gcell(s2->g2)->x) );
            yl = max( min(grid->gcell(s1->g1)->y, grid->gcell(s1->g2)->y), min(grid->gcell(s2->g1)->y, grid->gcell(s2->g2)->y) );
            yh = min( max(grid->gcell(s1->g1)->y, grid->gcell(s1->g2)->y), max(grid->gcell(s2->g1)->y, grid->gcell(s2->g2)->y) );
            g1 = grid->index(xl, yl, _layer+1);
            g2 = grid->index(xl, yl, _layer+1);

            s3 = create_segment(net);
            s3->g1 = g1;
            s3->g2 = g2;
            s3->layer = _layer + 1;
            s3->direction = grid->direction(g1, g2);
            s3->net = net;
            s3->assign = false;
            s3->access = false;

            // Remove edge and Add new Segment, Edge
            remove_edge(s1->id, s2->id);
            add_edge(s1->id, s3->id, VIA_CROSS);
            add_edge(s2->id, s3->id, VIA_CROSS);

            brokenEdge = true;
            cout << boost::format("%s edge (%d %d) removed -> edge (%d %d) and (%d %d) added\n") 
                % get_net(net)->name % s1->id % s2->id % s1->id % s3->id % s2->id % s3->id;
        }
    }

    return brokenEdge;
}








