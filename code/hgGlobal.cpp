#include "hgCircuit.h"
#include "hgHeap.h"
#include "hgFunc.h"
#include "mymeasure.h"

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

//#define CHECK_RUNTIME

using namespace HGR;

bool Router::route_all()
{

    //cout << " ---- Route All ---- " << endl;
    //vector<vector<int>> _nets(nthreads(), vector<int>());
    

    vector<int> _nets;
    for(int i=0; i < num_nets(); i++)
    {
        int thr = i % nthreads();
        topologies[i].id = i;
        topologies[i].trees = vector<SegRtree>(num_layers());
        
        if(get_topology(i)->routed)
            continue;
        
        //_nets[thr].push_back(i);

        _nets.push_back(i);
    }

  
    // Previous Result
    int prevCongested = INT_MAX;

    for(int iterCount = 0; iterCount < 5; iterCount++)
    {
        int routingCount = 0;
        //vector<Topology> _backup = topologies;
        dense_hash_map<int, Topology> backup;
        backup.set_empty_key(INT_MAX);


        //#pragma omp parallel num_threads(nthreads()) shared(routingCount)
        //{
            //int thr_num = omp_get_thread_num();
            //for(auto i : _nets[thr_num])
        for(auto i : _nets)    
        {
            if(iterCount != 0)
            { 
                if(check_congested_region(i))
                {
                    //#pragma omp critical(BACKUP)
                    backup[i] = topologies[i];
                    topologies[i].clear();
                }
                else
                {
                    continue;
                }
            }

            if(!specified_global_routing(i, iterCount))
            {
                if(get_net(i)->terminals.size() > 1)
                    exit(0);
                continue;
            }
            topology_generation2(i);


            //if(routingCount++ % 1000 == 0)
            //    cout << "Routing Progress " << 100.0 * routingCount / num_nets() << "%" << endl;
        }
        //}

        cout << "Global Routing Done (" << iterCount << "-th iteration)" << endl;


        /////////////////////////////////////////
        //cout << "Grid Congestion Map (" << iterCount << "-th iteration)" << endl;
        int totalCongested = 0;
        int levelCongested = 0;
        
        //#pragma omp parallel num_threads(nthreads()) shared(totalCongested, levelCongested)
        for(int i=0; i < grid.gcells.size(); i++)

        {
            Gcell* _g = grid[i];
            int edgeCap = _g->edgeCap;
            int edgeAss = grid.assign[i];
            if(edgeCap < edgeAss)
            {
                //cout << "Gcell (" << _g->x << " " << _g->y << " " << _g->z << ") -> edgeCap " << _g->edgeCap << " Assigned " << grid.assign[i] 
                //    << " [Congested " << edgeAss - edgeCap << "]" << endl;
                totalCongested++;
                levelCongested += edgeAss - edgeCap;
            }
        }

        cout << "#Total Congested Region -> " << totalCongested << "/" << levelCongested<< "  (" << iterCount << "-th iteration)" << endl << endl;
        
        if(totalCongested == 0)
            break;

        // If not improve, go back
        if(prevCongested < levelCongested)
        {
            for(auto it : backup)
            {
                topologies[it.first] = it.second; //_backup;
            }
            break;
        }

        prevCongested = levelCongested;
    }

    int total_routed = 0;
    for(int i=0; i < num_nets(); i++)
    {
        Net* _net = get_net(i);
        Topology* _tp = get_topology(i);
        
        // Useless data pooling
        _tp->gcells.clear();
        _net->guides.clear();


        if(_tp->routed)
            continue;

        if(_net->terminals.size() == 2)
            //&& _tp->segments.size() == 2)
        {
            int p1 = _net->terminals[0];
            int p2 = _net->terminals[1];
            int numsegs = _tp->segments.size();

            if(numsegs == 2)
            {
                if(route_local_net_twoseg(i, p1, p2))
                    total_routed++;
            }

            if(numsegs == 3)
            {
                if(route_local_net_thrseg(i, p1, p2))
                    total_routed++;
            }
        }
    }

    cout << "Local Net Routing Done ( #routed : " << total_routed << " )" << endl;
}

bool Router::check_congested_region(int netid)
{
    Topology* _tp = get_topology(netid);
    
    if(_tp->segments.size() == 0)
        return true;

    bool removeAll = false;
    vector<int> gcells;
    
    //
    for(int i=0; i < (int)_tp->segments.size(); i++)
    {
        Segment* _s = get_segment(netid, i);

        int x1  = min(grid[_s->g1]->x, grid[_s->g2]->x);
        int x2  = max(grid[_s->g1]->x, grid[_s->g2]->x);
        int y1  = min(grid[_s->g1]->y, grid[_s->g2]->y);
        int y2  = max(grid[_s->g1]->y, grid[_s->g2]->y);
        int z   = _s->layer;

        for(int x = x1; x <= x2; x++)
        {
            for(int y = y1; y <= y2; y++)
            {
                int g = grid.index(x,y,z);
                gcells.push_back(g);
                if(grid.is_congested(g))
                {
                    removeAll = true;
                    grid.add_history(g);
                }
            }
        }
    }

    /*
    for(auto gcellid : _tp->gcells)
    {
        if(grid.is_congested(gcellid))
        {
            removeAll = true;
            grid.add_history(gcellid);
        }
    }
    */

    if(removeAll)
    {
        grid.gc_drain(gcells);
        //_tp->clear();
        return true;
    }

    return false;
}


bool Router::specified_global_routing(int netid, int iterCount)
{
    

    Net* curNet = get_net(netid);                   // Target Net
    vector<int> terminals = curNet->terminals;      // Unrouted pin list
    set<int> routing_guide, routing_space;

    int buffer_distance = (1 + 3*iterCount) * min(grid.GCheight, grid.GCwidth);
    get_routing_space(netid, buffer_distance, routing_guide, routing_space);


    //cout << "Route " << get_net(netid)->name << "(" << terminals.size() << ")" <<  endl;
    //cout << "terminals size : " << terminals.size() << endl;
    //////////////////////////////
    if(terminals.size() < 2) 
        return false;
    //////////////////////////////


    vector<pair<pair<int,int>,double>> comb;
    for(size_t i=0; i < terminals.size() - 1; i++)
    {
        for(size_t j=i+1; j < terminals.size(); j++)
        {
            comb.push_back( { { terminals[i], terminals[j] }, pin_to_pin_distance(terminals[i], terminals[j]) } );
        }
    }
   
    sort(comb.begin(), comb.end(), [](const pair<pair<int,int>, double> &left, const pair<pair<int,int>, double> &right){
            return left.second < right.second;
            });



    while(comb.size() > 0)
    {
        // Get source and target pins
        int source = comb[0].first.first;
        int target = comb[0].first.second;
        comb.erase(comb.begin());

        ////////////////////////////////////////////////
        //route_twopin_net_dijkstra(netid, source, target);
        ////////////////////////////////////////////////


        // Route twopin net p1 -> p2
        if(route_twopin_net2(netid, source, target, routing_guide, routing_space))
        //if(route_twopin_net_dijkstra(netid, source, target))
        {
            remove(terminals, source);
            remove(terminals, target);
            break;
        }else{
            cout << "Route Two-pin net Failed" << endl;

        }
    }



    for(size_t i=0; i < terminals.size(); i++)
    {
        if(!route_pin_to_tp2(netid, terminals[i], routing_guide, routing_space))
        {
            cout << "Route Pin-to-Topology Failed" << endl;
            return false;
        }
    }

    return true;
}


void Router::init()
{
    /*
    for(int i=0; i < num_layers(); i++)
    {
        cout << get_metal(i)->name << " minSpacing : " << min_spacing(i) << " width : " << wire_width(i) << endl; 
    }
    */
#ifdef CHECK_RUNTIME
    double we1, we2;
    we1 = measure.elapse_time(); 
#endif

    cout << "Start Initialize" << endl;

    topologies = vector<Topology>(num_nets());
    rtree.pref = vector<SegRtree>(num_layers());
    rtree.npref = vector<SegRtree>(num_layers());
    rtree.wires = vector<BoxRtree>(num_layers());
    rtree.pins = vector<BoxRtree>(num_layers());
    rtree.vias = vector<BoxRtree>(num_layers());

    //cout << "Track Initialize" << endl;
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_tracks(); i++)
    {
        Track* curT = &ckt->tracks[i];
        Layer* curL = &ckt->metals[curT->layer];
        int id = curT->id;
        int lNum = curT->layer;
        bool isPref = (curL->direction == curT->direction) ? true : false;

        seg ts(pt(curT->ll.x, curT->ll.y), pt(curT->ur.x, curT->ur.y));
        
        if(isPref)
        {
            #pragma omp critical(PREF)
            {
            rtree.pref[lNum].insert({ts, id});
            }
        }
        else
        {
            //#pragma omp critical(NPREF)
            #pragma omp critical(NPREF)
            {
            rtree.npref[lNum].insert({ts, id});
            }
        }
    }


    Point<int> ll(INT_MAX, INT_MAX), ur(INT_MIN, INT_MIN);
    dense_hash_map<int,int> vSize, hSize;
    vSize.set_empty_key(INT_MAX);
    hSize.set_empty_key(INT_MAX);

    for(int i=0; i < num_nets(); i++)
    {
        Net* curN = &ckt->nets[i];

        for(int j=0; j < (int)curN->guides.size(); j++)
        {
            int lNum = curN->guides[j].first;
            Rect<int> rect = curN->guides[j].second;
            int gcSize;

            ll.x = min(ll.x, rect.ll.x);
            ll.y = min(ll.y, rect.ll.y);


            if(ckt->metals[lNum].direction == VERTICAL)
            {
                gcSize = rect.ur.x - rect.ll.x;
                if(vSize.find(gcSize) == vSize.end())
                    vSize[gcSize] = 1;
                else
                    vSize[gcSize]++;
            }
            else
            {
                gcSize = rect.ur.y - rect.ll.y;
                if(hSize.find(gcSize) == hSize.end())
                    hSize[gcSize] = 1;
                else
                    hSize[gcSize]++;
            }
        }
    }

    int width = INT_MAX, height = INT_MAX, vMax= INT_MIN, hMax = INT_MIN;
    
    
    //cout << " - - - - < VERTICAL LAYER > - - - - \n";
    for(auto& it : vSize)
    {
        if(it.second > vMax)
        {
            vMax = it.second;
            width = it.first;
        }
        //cout << it.first << " " << it.second << endl;
    }

    //printf(" - - -  < HORIZONTAL LAYER >  - - - \n");
    for(auto& it : hSize)
    {
        if(it.second > hMax)
        {
            hMax = it.second;
            height = it.first;
        }
        //cout << it.first << " " << it.second << endl;
    }
    //cout << endl << endl;



    int dx = ll.x - ckt->dieArea.ll.x;
    int dy = ll.y - ckt->dieArea.ll.y;
    
    if(dx > 0 && dx > width)
    {
        ll.x -= round(1.0*dx/width) * width;
    }

    if(dy > 0 && dy > height)
    {
        ll.y -= round(1.0*dy/height) * height;
    }

    dx = ckt->dieArea.ur.x - ll.x;
    dy = ckt->dieArea.ur.y - ll.y;

    ur.x = ll.x + ceil(1.0*dx/width) * width;
    ur.y = ll.y + ceil(1.0*dy/height) * height;

    //cout << "Gcell Size (" << width << " " << height << ")" << endl;
    //cout << "Grid Boundary (" << ll.x << " " << ll.y << ") (" << ur.x << " " << ur.y << ")" << endl;
    //cout << "Design Bdounary (" << ckt->dieArea.ll.x << " " << ckt->dieArea.ll.y << ") (" << ckt->dieArea.ur.x << " " << ckt->dieArea.ur.y << ")" << endl;

    // Check
    int vCnt1 = 0, hCnt1 = 0;
    int vCnt2 = 0, hCnt2 = 0;
    for(int i=0; i < num_nets(); i++)
    {
        Net* curN = &ckt->nets[i];

        for(int j=0; j < (int)curN->guides.size(); j++)
        {
            int lNum = curN->guides[j].first;
            Rect<int> rect = curN->guides[j].second;

            dx = rect.ll.x - ll.x;//ckt->dieArea.ll.x;
            dy = rect.ll.y - ll.y;//ckt->dieArea.ll.y;

            if(dx % width == 0)
                vCnt1++;
            else
                vCnt2++;

            if(dy % height == 0)
                hCnt1++;
            else
                hCnt2++;
        }
    }

    //printf("Dividable { W : %d H : %d } \nUndividable { W : %d H : %d }\n",
    //        vCnt1, hCnt1, vCnt2, hCnt2);

    

    int GCwidth = width/3, GCheight = height/3;
    //int GCwidth = width, GCheight = height;
    int numCols = (ur.x - ll.x)/GCwidth;
    int numRows = (ur.y - ll.y)/GCheight;
    int numLayers = ckt->metals.size();


    /*
    if(numCols > 1000)
    {
        GCwidth = width/2;
        numCols = (ur.x - ll.x) / GCwidth;
    }


    if(numRows > 1000)
    {
        GCheight = height/2;
        numRows = (ur.y - ll.y) / GCheight;
    }
    */

    int num_gcells = numLayers * numRows * numCols;

    //cout << "Grid3D Initialize (#col : " << numCols << " #row : " << numRows << " #layer : " << numLayers << ")" << endl;

    grid.area = Rect<int>(ll, ur);
    grid.GCwidth = GCwidth;
    grid.GCheight = GCheight;
    grid.numCols = numCols;
    grid.numRows = numRows;
    grid.numLayers = numLayers;
    //grid.trees = vector<BoxRtree>(numLayers);
    grid.gcells = vector<Gcell>(num_gcells);
    grid.assign = vector<int>(num_gcells, 0);
    grid.history = vector<int>(num_gcells, 0);
    grid.xOffsets = vector<int>(numCols+1, 0);
    grid.yOffsets = vector<int>(numRows+1, 0);
    grid.row = vector<vector<Panel>>(num_layers(), vector<Panel>(numRows));
    grid.col = vector<vector<Panel>>(num_layers(), vector<Panel>(numCols));
    
    //#pragma omp parallel for num_threads(nthreads())
    for(int i=0; i <= numRows; i++)
    {
        grid.yOffsets[i] = GCheight * i + ll.y;
    }
    
    //cout << "xOffset" << endl;
    //#pragma omp parallel for num_threads(nthreads())
    for(int i=0; i <= numCols; i++)
    {
        grid.xOffsets[i] = GCwidth * i + ll.x;
        //cout << "(" << i << ")" << grid.xOffsets[i] << endl;
    }

    #pragma omp parallel for num_threads(nthreads())
    for(int lNum = 0 ; lNum < num_layers(); lNum++)
    {
        for(int col=0; col < numCols; col++)
        {
            bool isPref = is_preferred(lNum, VERTICAL);
            int x1 = grid.xOffsets[col];
            int x2 = grid.xOffsets[col+1];
            int y1 = grid.yOffsets[0];
            int y2 = grid.yOffsets[numRows];
            Rect<int> rect(x1, y1, x2, y2);
            grid.col[lNum][col] = Panel(col, 0, lNum, isPref, rect);
            rtree.get_tracks(lNum, rect, isPref, grid.col[lNum][col].tracks);
        }

        for(int row=0; row < numRows; row++)
        {
            bool isPref = is_preferred(lNum, HORIZONTAL);
            int x1 = grid.xOffsets[0];
            int x2 = grid.xOffsets[numCols];
            int y1 = grid.yOffsets[row];
            int y2 = grid.yOffsets[row+1];
            Rect<int> rect(x1, y1, x2, y2);
            grid.row[lNum][row] = Panel(0, row, lNum, isPref, rect);
            rtree.get_tracks(lNum, rect, isPref, grid.row[lNum][row].tracks);
        }
    }


    //cout << "Gcell Initialize" << endl;
    //cout << "START GCELL INITIALIZE" << endl;
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < numLayers * numRows * numCols; i++)
    {
        
        //for(int j=0; j < numRows*numCols; j++)
        //{
            //int gcellid = j + i*numRows*numCols;
        int x, y, z;
        grid.coord(i, x, y, z);
        Gcell* g = grid[i];

        int x1 = GCwidth * x + ll.x;
        int x2 = GCwidth * (x+1) + ll.x;
        int y1 = GCheight * y + ll.y;
        int y2 = GCheight * (y+1) + ll.y;
        int index = is_vertical(z) ? x : y;

        g->id = i;
        g->x = x, g->y = y, g->z = z;
        g->direction = preferred(z);
        g->rect = Rect<int>(x1, y1, x2, y2);
        g->edgeCap = grid.get_panel(z, preferred(z), index)->tracks.size();
        g->weight = 0;
    }


    //for(int i=0; i < num_gcells; i++)
    //cout << i << "th -> " << grid[i]->weight << endl;


    //cout << "yOffset" << endl;
    //cout << "Add Special Nets" << endl;
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_nets(); i++)
    {
        Net* _net = get_net(i);

        /*
        if(_net->wires.size() > 0)
        {
            cout << "SPECIAL NET " << _net->name << " -> " << _net->wires.size() <<  endl;
        
        }
        */

        for(size_t j=0; j < _net->wires.size(); j++)
        {
            Wire* _wire = &_net->wires[j];
            int wireid = _wire->id * num_nets() + _net->id;
            if(_wire->isVia)
            {
                MacroVia* _via = &ckt->macroVias[_wire->via_type];
                
                for(size_t k=0; k < _via->cuts.size(); k++)
                {
                    string layerName = _via->cuts[k].first;

                    for(size_t n=0; n < _via->cuts[k].second.size(); n++)
                    {
                        Rect<double> rect = _via->cuts[k].second[n];
                        int x1 = _wire->ll.x + rect.ll.x * lef_unit_microns();
                        int x2 = _wire->ll.x + rect.ur.x * lef_unit_microns();
                        int y1 = _wire->ll.y + rect.ll.y * lef_unit_microns();
                        int y2 = _wire->ll.y + rect.ur.y * lef_unit_microns();
                        int lNum = ckt->layer2id[layerName];                        
                        if(is_metal_layer(layerName))
                        {
                            //cout << "Power VIA " << get_metal(lNum)->name << " " << Rect<int>(x1, y1, x2, y2) << endl;
                            #pragma omp critical(RTREE_WIRE)
                            {
                                rtree.wires[lNum].insert( { box(pt(x1, y1), pt(x2, y2)), wireid } );
                            }

                            ///////////////////////////////////////////
                            Rect<int> rect2(x1, y1, x2, y2);
                            int g_ll = grid.get_index(lNum, rect2.ll);
                            int g_ur = grid.get_index(lNum, rect2.ur);

                            for(int x=grid[g_ll]->x; x <= grid[g_ur]->x; x++)
                            {
                                for(int y=grid[g_ll]->y; y <= grid[g_ur]->y; y++)
                                {
                                    int g = grid.index(x, y, lNum);
                                    int orig_area = area(grid[g]->rect);
                                    int over_area = area(grid[g]->rect, rect2);

                                    #pragma omp critical(GCELL)
                                    {
                                        grid[g]->weight += 100.0 * over_area/orig_area;
                                    }
                                }
                            }
                            ///////////////////////////////////////////

                        }

                        if(is_cut_layer(layerName))
                        {
                            //cout << "Power VIA " << get_metal(lNum)->name << " " << Rect<int>(x1, y1, x2, y2) << endl;
                            //#pragma omp critical(RTREE_VIA)
                            #pragma omp critical(RTREE_VIA)
                            {
                                rtree.vias[lNum].insert( { box(pt(x1, y1), pt(x2, y2)), wireid } );
                            }
                        }
                    }
                }
                //
            }
            else
            {
                int x1 = (direction(_wire->ll, _wire->ur) == VERTICAL) ? _wire->ll.x - _wire->width/2 : _wire->ll.x;
                int x2 = (direction(_wire->ll, _wire->ur) == VERTICAL) ? _wire->ur.x + _wire->width/2 : _wire->ur.x;
                int y1 = (direction(_wire->ll, _wire->ur) == VERTICAL) ? _wire->ll.y : _wire->ll.y - _wire->width/2;
                int y2 = (direction(_wire->ll, _wire->ur) == VERTICAL) ? _wire->ur.y : _wire->ur.y + _wire->width/2;
                int lNum = _wire->layer;

                #pragma omp critical(RTREE_WIRE)
                {
                    rtree.wires[lNum].insert( { box(pt(x1, y1), pt(x2, y2)), wireid } );
                }

                ///////////////////////////////////////////
                Rect<int> rect2(x1, y1, x2, y2);
                int g_ll = grid.get_index(lNum, rect2.ll);
                int g_ur = grid.get_index(lNum, rect2.ur);

                for(int x=grid[g_ll]->x; x <= grid[g_ur]->x; x++)
                {
                    for(int y=grid[g_ll]->y; y <= grid[g_ur]->y; y++)
                    {
                        int g = grid.index(x, y, lNum);
                        int orig_area = area(grid[g]->rect);
                        int over_area = area(grid[g]->rect, rect2);

                        #pragma omp critical(GCELL)
                        {
                            grid[g]->weight += 100.0 * over_area/orig_area;
                        }
                    }
                }
                ///////////////////////////////////////////

            
            }
        }
    }


    //cout << "Pin Initialize (Macro Pin)" << endl;
    /* Pin and Dummy Pin */
    set<int> added;
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_cells(); i++)
    {
        Cell* c = get_cell(i);
        Macro* m = &ckt->macros[c->type];
        
        // Get Orient Params
        int rotate;
        bool flip;
        get_orient(c->cellOrient, rotate, flip);
        Point<int> pin_orig = c->origin;

        //cout << "MACRO " << m->name << endl;

        for(auto& it : m->pins)
        {
            string portName = it.first;
            MacroPin* mp = &it.second;

            for(int j=0;  j < (int)mp->ports.size(); j++)
            {
                string layerName =  mp->ports[j].first;
                int lNum = ckt->layer2id[mp->ports[j].first];
                for(int k=0; k < (int)mp->ports[j].second.size(); k++)
                {
                    Rect<double> temp = mp->ports[j].second[k];
                    //cout << "PORT " << get_metal(lNum)->name << " " << temp << endl;
                    Point<int> pt1 = Point<int>( (int)( pin_orig.x + temp.ll.x * lef_unit_microns() ),
                                                 (int)( pin_orig.y + temp.ll.y * lef_unit_microns() ) );
                    Point<int> pt2 = Point<int>( (int)( pin_orig.x + temp.ur.x * lef_unit_microns() ),
                                                 (int)( pin_orig.y + temp.ur.y * lef_unit_microns() ) );

                    Rect<int> rect1, rect2;
                    rect1 = Rect<int>(pt1, pt2);

                    flip_or_rotate(Point<int>(0,0), rect1, rect2, rotate, flip);

                    rect2.ll.x += c->delta.x + c->ll.x;
                    rect2.ll.y += c->delta.y + c->ll.y;
                    rect2.ur.x += c->delta.x + c->ll.x;
                    rect2.ur.y += c->delta.y + c->ll.y;

                    string pinName = c->name + "_" + portName;

                    //cout << pinName << " -> " << rect2 << endl;
                    bool isDummy = (ckt->pin2id.find(pinName) == ckt->pin2id.end()) ? true : false;
                    int pinid = isDummy ? INT_MAX : ckt->pin2id[pinName];
                    
                    // Add into rtree
                    
                    #pragma omp critical(RTREE_PIN) 
                    {
                        rtree.pins[lNum].insert( { convert(rect2), pinid } );
                        added.emplace(pinid);
                    }

                    if(is_cut_layer(layerName)) continue;
                    int g_ll = grid.get_index(lNum, rect2.ll);
                    int g_ur = grid.get_index(lNum, rect2.ur);

                    for(int x=grid[g_ll]->x; x <= grid[g_ur]->x; x++)
                    {
                        for(int y=grid[g_ll]->y; y <= grid[g_ur]->y; y++)
                        {
                            int g = grid.index(x, y, lNum);
                            int orig_area = area(grid[g]->rect);
                            int over_area = area(grid[g]->rect, rect2);

                            #pragma omp critical(GCELL)
                            {
                                grid[g]->weight += 100.0 * over_area/orig_area;
                            }
                        }
                    }
                
                }
            }
        }
        //cout << endl;
    }


    //cout << "Pin Initialize (Macro OBS)" << endl;
    // Multi-threading unable
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_cells(); i++)
    {
        Cell* c = get_cell(i);
        //Macro* m = &ckt->macros[c->type];
        vector<pair<string,Rect<double>>> obstacles = ckt->macros[c->type].obstacles;

        // Get Orient Params
        int rotate;
        bool flip;
        get_orient(c->cellOrient, rotate, flip);
        Point<int> pin_orig = c->origin;
        for(int j=0; j < (int)obstacles.size(); j++)
        {
            string layerName = obstacles[j].first;
            int lNum = ckt->layer2id[obstacles[j].first];

            Rect<double> temp = obstacles[j].second;
            Point<int> pt1 = Point<int>( (int)( pin_orig.x + temp.ll.x * lef_unit_microns() ),
                    (int)( pin_orig.y + temp.ll.y * lef_unit_microns() ) );
            Point<int> pt2 = Point<int>( (int)( pin_orig.x + temp.ur.x * lef_unit_microns() ),
                    (int)( pin_orig.y + temp.ur.y * lef_unit_microns() ) );

            Rect<int> rect1, rect2;
            rect1 = Rect<int>(pt1, pt2);

            flip_or_rotate(Point<int>(0,0), rect1, rect2, rotate, flip);

            rect2.ll.x += c->delta.x + c->ll.x;
            rect2.ll.y += c->delta.y + c->ll.y;
            rect2.ur.x += c->delta.x + c->ll.x;
            rect2.ur.y += c->delta.y + c->ll.y;
            // Add into rtree
            

            #pragma omp critical(PINS_OBS)
            {
                if(is_metal_layer(layerName))
                {
                    rtree.pins[lNum].insert( { convert(rect2), INT_MAX } );
                }
                else
                {
                    rtree.vias[lNum].insert( { convert(rect2), INT_MAX } );
                }
            }
            
            if(is_cut_layer(layerName)) continue;

            int g_ll = grid.get_index(lNum, rect2.ll);
            int g_ur = grid.get_index(lNum, rect2.ur);
            
            for(int x=grid[g_ll]->x; x <= grid[g_ur]->x; x++)
            {
                for(int y=grid[g_ll]->y; y <= grid[g_ur]->y; y++)
                {
                    int g = grid.index(x, y, lNum);
                    int orig_area = area(grid[g]->rect);
                    int over_area = area(grid[g]->rect, rect2);

                    #pragma omp critical(GCELL)
                    {
                        grid[g]->weight += 100.0 * over_area/orig_area;
                    }
                }
            }
        }
    }



    //cout << "I/O Pin Initialize" << endl;
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_pins(); i++)
    {
        Pin* _p = get_pin(i);

        vector<Point<int>> ll(num_layers(), Point<int>(INT_MAX, INT_MAX));
        vector<Point<int>> ur(num_layers(), Point<int>(INT_MIN, INT_MIN));
        Rect<int> env(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
        
        // Insert Rect into Rtree 
        
        for(int j=0; j < (int)_p->rects.size(); j++)
        {
            int lNum = _p->rects[j].first;
            Rect<int> rect = _p->rects[j].second;


            ll[lNum].x = min(ll[lNum].x, rect.ll.x);
            ll[lNum].y = min(ll[lNum].y, rect.ll.y);
            ur[lNum].x = max(ur[lNum].x, rect.ur.x);
            ur[lNum].y = max(ur[lNum].y, rect.ur.y);
            env.ll.x = min(rect.ll.x, env.ll.x);
            env.ll.y = min(rect.ll.y, env.ll.y);
            env.ur.x = max(rect.ur.x, env.ur.x);
            env.ur.y = max(rect.ur.y, env.ur.y);
            
            if(added.find(i) == added.end())
            {
                // Add into rtree
                #pragma omp critical(RTREE_PIN)
                {
                    rtree.pins[lNum].insert( { convert(rect), _p->id } );
                }
            }

            vector<pair<seg,int>> queries;
            rtree.pref[lNum].query(bgi::intersects(convert(rect)), back_inserter(queries));
            for(auto it : queries)
            {
                if(!exist(_p->pref, it.second))
                    _p->pref.push_back(it.second);
            }

            queries.clear();
            rtree.npref[lNum].query(bgi::intersects(convert(rect)), back_inserter(queries));
            for(auto it : queries)
            {
                if(!exist(_p->npref, it.second))
                    _p->npref.push_back(it.second);
            }
        }


        for(int lNum = 0; lNum < num_layers(); lNum++)
        {
            if(ll[lNum].x == INT_MAX) continue;
            _p->envelope[lNum] = Rect<int>(ll[lNum], ur[lNum]);
        }

        // On-Track | Off-Track
        _p->onTrack = (_p->pref.size() + _p->npref.size() > 0) ? true : false;
    }

    cout << "Initialize Done" << endl;

#ifdef DEBUG_GRID 
    cout << "Grid Weight Report" << endl;
    for(auto _g : grid.gcells)
    {
        if(_g.weight > 0)
        {
            cout << "Gcell (" << _g.x << " " << _g.y << " " << _g.z << ") -> weight " << _g.weight << endl;
        }
    }
#endif
    /*
    for(int k =0; k < num_layers(); k++)
    {
        Layer* _layer = get_metal(k);
        cout << _layer->name << endl;
        cout << "PARALLEL RUN LENGTH : ";
        for(size_t i = 0; i < _layer->table.length.size(); i++)
        {
            cout << _layer->table.length[i] << " ";
        }
        cout << endl;
        cout << "PARALLEL RUN WIDTH : ";
        for(size_t i = 0; i < _layer->table.width.size(); i++)
        {
            cout << _layer->table.width[i] << " ";
        }

        cout << "SPACING  ";
        for(size_t i = 0; i < _layer->table.length.size(); i++)
        {

            for(size_t j = 0; j < _layer->table.width.size(); j++)
            {
                cout << _layer->table.spacing[i][j] << " ";
            }
            cout << endl;
        }
    }
    */
}

int Rtree::num_tracks(int lNum, Rect<int> rect, bool isPref)
{
    vector<pair<seg,int>> queries;
    SegRtree* tree = (isPref) ? &pref[lNum] : &npref[lNum];
    tree->query(bgi::intersects(box(pt(rect.ll.x, rect.ll.y), pt(rect.ur.x, rect.ur.y))), back_inserter(queries));
    return queries.size();
}

int Rtree::num_wires(int lNum, Rect<int> rect)
{
    vector<pair<box,int>> queries;
    wires[lNum].query(bgi::intersects(convert(rect)), back_inserter(queries));
    return queries.size();
}


void Rtree::get_tracks(int lNum, Rect<int> rect, bool isPref, vector<int>& tracks)
{
    vector<pair<seg,int>> queries;
    SegRtree* tree = (isPref) ? &pref[lNum] : &npref[lNum];
    tree->query(bgi::intersects(box(pt(rect.ll.x, rect.ll.y), pt(rect.ur.x, rect.ur.y))), back_inserter(queries));

    for(auto& it : queries)
    {
        tracks.push_back(it.second);
    }
}



        /*
        Topology* _tp = get_topology(_p->net);
        #pragma omp critical(ADDPIN)
        {
            _tp->pintree.insert( { convert(env), _p->id } );
        }

        for(auto& it : _p->pinShape)
        {
            int lNum = it.first;
            multi_polygon shape = it.second;
            vector<pair<seg,int>> queries;
            
            // preferred
            rtree.pref[lNum].query(bgi::intersects(shape), back_inserter(queries));

            for(int k=0; k < (int)queries.size(); k++)
                _p->pref.push_back(queries[k].second);

            queries.clear();
            
            // non-preferred
            rtree.npref[lNum].query(bgi::intersects(shape), back_inserter(queries));
            
            for(int k=0; k < (int)queries.size(); k++)
                _p->npref.push_back(queries[k].second);
        
            //cout << p->name << " pref(" << p->pref.size() << ") npref(" << p->npref.size() << ")" << endl;
        
        }
        */
    /*
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_pins(); i++)
    {
        Pin* _p = get_pin(i);
        vector<Point<int>> ll(num_layers(), Point<int>(INT_MAX, INT_MAX));
        vector<Point<int>> ur(num_layers(), Point<int>(INT_MIN, INT_MIN));
        Rect<int> env(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
        for(size_t j=0; j < _p->rects.size(); j++)
        {
            int lNum = _p->rects[j].first;
            Rect<int> rect = _p->rects[j].second;

            ll[lNum].x = min(ll[lNum].x, rect.ll.x);
            ll[lNum].y = min(ll[lNum].y, rect.ll.y);
            ur[lNum].x = max(ur[lNum].x, rect.ur.x);
            ur[lNum].y = max(ur[lNum].y, rect.ur.y);
            env.ll.x = min(rect.ll.x, env.ll.x);
            env.ll.y = min(rect.ll.y, env.ll.y);
            env.ur.x = max(rect.ur.x, env.ur.x);
            env.ur.y = max(rect.ur.y, env.ur.y);
        }


        for(int lNum = 0; lNum < num_layers(); lNum++)
        {
            if(ll[lNum].x == INT_MAX) continue;
            _p->envelope[lNum] = Rect<int>(ll[lNum], ur[lNum]);
        }

        Topology* _tp = get_topology(_p->net);
        #pragma omp critical(ADDPIN)
        {
            _tp->pintree.insert( { convert(env), _p->id } );
        }

    }
    */


/*
void Topology::get_pins(Rect<int> rect, vector<int>& pins)
{
    vector<pair<box,int>> queries;
    pintree.query(bgi::intersects(convert(rect)), back_inserter(queries));
    for(auto& it : queries)
    {
        pins.push_back(it.second);
    }
}
*/






