#include "hgGeometry.h"
#include "hgRouter.h"

using namespace HGR;

Segment* HGR::create_segment(int _net)
{
    
    Topology* tp = get_topology(_net);
    
    Segment s;
    s.id = tp->segments.size();
    s.net = _net;
    tp->segments.push_back(s);

    return &tp->segments[s.id];
}

void HGR::Topology::add_edge(int _s1, int _s2, int _type)
{
    edge_t key(min(_s1, _s2), max(_s1, _s2));

    if(edge.find(key) == edge.end())
    {
        Edge e;
        e.s1 = key.first;
        e.s2 = key.second;
        e.type = _type;
        edge[key] = e;
        edges.push_back(key);
    }
}

void HGR::Topology::remove_edge(int _s1, int _s2)
{
    edge_t key(min(_s1, _s2), max(_s1, _s2));

    if(edge.find(key) != edge.end())
    {
        edge.erase(key);
        remove(edges, key);
    }
}

void HGR::Topology::set_edge_type(int _s1, int _s2, int _type)
{
    edge_t key(min(_s1, _s2), max(_s1, _s2));

    if(edge.find(key) != edge.end())
    {
        edge[key].type = _type;
    }
}

void HGR::Topology::set_edge_via(int _s1, int _s2, Via _via)
{
    edge_t key(min(_s1, _s2), max(_s1, _s2));

    if(edge.find(key) != edge.end())
    {
        edge[key].via = _via;
        edge[key].isVia = true;
    }
}

int HGR::Topology::get_edge_type(int _s1, int _s2)
{
    edge_t key(min(_s1, _s2), max(_s1, _s2));
    int type = NODATA;
    if(edge.find(key) != edge.end())
    {
        type = edge[key].type;
    }

    return type;
}

Via HGR::Topology::get_edge_via(int _s1, int _s2)
{
    edge_t key(min(_s1, _s2), max(_s1, _s2));

    Via via;
    if(edge.find(key) != edge.end())
    {
        via = edge[key].via;
    }
    
    return via;
}

bool HGR::Topology::parallel_edge_exist()
{
    for(auto& it : edge)
    {
        Edge &e = it.second;

        if(e.type == PRL)
            return true;
    }

    return false;
}

void HGR::Topology::clear()
{
    routed = false;
    segments.clear();
    edge.clear();
}


void HGR::Segment::update_database()
{
    if(assign)
    {
        //wire.id = objid;
        wire.update_database();
    }

    for(size_t i=0; i < exts.size(); i++)
    {
        exts[i].update_database();         
    }
}

void HGR::Segment::remove_database()
{
    if(assign)
    {
        //wire.id = objid;
        wire.remove_database();
    }

    for(size_t i=0; i < exts.size(); i++)
    {
        exts[i].remove_database();         
    }
}

void HGR::Ext::set_id(int _id)
{
    for(size_t i=0; i < vias.size(); i++)
    {
        vias[i].id = _id; 
    }

    for(size_t i=0; i < wires.size(); i++)
    {
        wires[i].id = _id; 
    }
}


void HGR::Ext::update_database()
{
    for(size_t i=0; i < vias.size(); i++)
    {
        vias[i].update_database();
    }

    for(size_t i=0; i < wires.size(); i++)
    {
        wires[i].update_database();
    }
}

void HGR::Ext::remove_database()
{
    for(size_t i=0; i < vias.size(); i++)
    {
        vias[i].remove_database();
    }

    for(size_t i=0; i < wires.size(); i++)
    {
        wires[i].remove_database();
    }
}



void HGR::Via::update_database()
{
    db->add_metal(id, lower, lowerMetalShape);
    db->add_metal(id, upper, upperMetalShape);
    db->add_cut(id, cut, cutShape);
}

void HGR::Via::remove_database()
{
    db->remove_metal(id, lower, lowerMetalShape);
    db->remove_metal(id, upper, upperMetalShape);
    db->remove_cut(id, cut, cutShape);
}

void HGR::Wire::update_database()
{
    db->add_metal(id, layer, metalShape);
}

void HGR::Wire::remove_database()
{
    db->remove_metal(id, layer, metalShape);
}

vector<int> HGR::Router::get_accessible_tracks(int _pin)
{
    set<int> tracks;

    for(auto ext : pinAccessPool[_pin])
    {
        tracks.insert(ext.track);
    }

    return vector<int>(tracks.begin(), tracks.end());
}




/*
void Router::topology_generation(int netid)
{
    //vector<pair<int,int>> edges;
    //vector<int> weight;
    using edge_weight_property = boost::property<boost::edge_weight_t, int>;
    using graph_type = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, edge_weight_property>;
    using predecessor_container = vector<boost::graph_traits<graph_type>::vertex_descriptor>;
    
    
    if(topologies[netid].segments.size() == 0) return;
    Topology* tp = get_topology(netid);
    graph_type G((int)topologies[netid].segments.size());

    int dz[] = { -1, 0, 1 };
    for(size_t i=0; i < topologies[netid].segments.size(); i++)
    {
        Segment* s1 = &topologies[netid].segments[i];
        Gcell* g1 = grid[s1->g1];
        Gcell* g2 = grid[s1->g2];

        for(int j=0; j < 3; j++)
        {
            int lNum = grid[s1->g1]->z + dz[j];

            if(lNum < 0 || lNum >= num_layers())
                continue;

            int x1 = min(g1->x, g2->x);
            int x2 = max(g1->x, g2->x);
            int y1 = min(g1->y, g2->y);
            int y2 = max(g1->y, g2->y);
            vector<pair<seg,int>> queries;
            topologies[netid].trees[lNum].query(bgi::intersects(seg(pt(x1,y1), pt(x2, y2))), 
                    back_inserter(queries));

            for(auto& it : queries)
            {
                Segment* s2 = &topologies[netid].segments[it.second];
                
                if(s1->id == s2->id)
                    continue;
           
                //if(s2->access) continue;

                int n1 = min(s1->id, s2->id);
                int n2 = max(s1->id, s2->id);
                int w = (s2->access || s1->access) ? 100 : 0;

                if(dz[j] == 0) w += (s1->direction == s2->direction) ? 30 : 5;
                else           w += (s1->direction == s2->direction) ? 50 : 1;

                //boost::add_edge(n1, n2, w, G);
                boost::add_edge(n1, n2, edge_weight_property(w), G);
            }
        }
    }



    vector<boost::graph_traits<graph_type>::edge_descriptor> edges;
    boost::kruskal_minimum_spanning_tree(G, back_inserter(edges));
    topologies[netid].p = vector<int>(topologies[netid].segments.size(),-1);    // -1 : Root

    for(vector<boost::graph_traits<graph_type>::edge_descriptor>::iterator it = edges.begin(); 
            it != edges.end(); ++it)
    {

        int n1 = boost::source(*it, G);
        int n2 = boost::target(*it, G);
        int c1, c2;
        // Parent
        topologies[netid].p[n1] = n2;

        // Add Edge
        Segment* s1 = &topologies[netid].segments[n1];
        Segment* s2 = &topologies[netid].segments[n2];

        if(s1->direction == s2->direction)
        {
            int n3 = topologies[netid].segments.size();
            int type13=0, type23=0;

            Segment s3;
            s3.id = n3;
            s3.leaf = false;
            s3.access = false; 
            //

            if(s1->layer != s2->layer)
            {
                if(!is_preferred(s1->layer, s1->direction))
                {
                    s3.layer = s1->layer;
                    s3.direction = preferred(s1->layer);
                    type13 = CROSS;
                    type23 = VIA_CROSS;
                    c1 = max(s1->layer, s3.layer);
                    c2 = max(s2->layer, s3.layer);

                }
                else if(!is_preferred(s2->layer, s2->direction))
                {
                    s3.layer = s2->layer;
                    s3.direction = preferred(s2->layer);
                    type13 = VIA_CROSS;
                    type23 = CROSS;
                    c1 = max(s1->layer, s3.layer);
                    c2 = max(s2->layer, s3.layer);
                }

                Gcell* g11 = grid[s1->g1];
                Gcell* g12 = grid[s1->g2];
                Gcell* g21 = grid[s1->g1];
                Gcell* g22 = grid[s1->g2];

                if((g11->x == g21->x && g11->y == g21->y) || (g11->x == g22->x && g11->y == g22->y))
                {
                    s3.g1 = grid.index(g11->x, g11->y, s3.layer);
                    s3.g2 = grid.index(g11->x, g11->y, s3.layer);
                }
                else if((g12->x == g21->x && g12->y == g21->y) || (g12->x == g22->x && g12->y == g22->y))
                {
                    s3.g1 = grid.index(g12->x, g12->y, s3.layer);
                    s3.g2 = grid.index(g12->x, g12->y, s3.layer);
                }
                else
                {
                    cout << "?? - 1" << endl;
                    exit(0);
                }
            }else{
              
                int x1 = max( min(grid[s1->g1]->x, grid[s1->g2]->x), min(grid[s2->g1]->x, grid[s2->g2]->x) );
                int x2 = min( max(grid[s1->g1]->x, grid[s1->g2]->x), max(grid[s2->g1]->x, grid[s2->g2]->x) );
                int y1 = max( min(grid[s1->g1]->y, grid[s1->g2]->y), min(grid[s2->g1]->y, grid[s2->g2]->y) );
                int y2 = min( max(grid[s1->g1]->y, grid[s1->g2]->y), max(grid[s2->g1]->y, grid[s2->g2]->y) );
                int z1 = s1->layer + 1;
                
                if(x1 == x2 && y1 == y2)
                {
                    s3.g1 = grid.index(x1, y1, z1);
                    s3.g2 = grid.index(x1, y1, z1);
                }
                else
                {
                    cout << "invalid ... - 1" << endl;
                    exit(0);
                }
                s3.layer = z1;
                s3.direction = (s1->direction == VERTICAL) ? HORIZONTAL : VERTICAL;
                type13 = VIA_CROSS;
                type23 = VIA_CROSS;
                c1 = max(s1->layer, s3.layer);
                c2 = max(s2->layer, s3.layer);
            }
            
            //cout << " g1 : " << s3.g1 << " g2 : " << s3.g2 <<  endl;
            s3.ll.x = min(grid[s3.g1]->rect.ll.x, grid[s3.g2]->rect.ll.x);
            s3.ll.y = min(grid[s3.g1]->rect.ll.y, grid[s3.g2]->rect.ll.y);
            s3.ur.x = max(grid[s3.g1]->rect.ur.x, grid[s3.g2]->rect.ur.x);
            s3.ur.y = max(grid[s3.g1]->rect.ur.y, grid[s3.g2]->rect.ur.y);

            _tp->segments.push_back(s3);
            _tp->add_edge(n1, n3, type13);
            _tp->add_edge(n2, n3, type23);

            //cout << "New Seg Added : " << s3 << endl;
            //cout << "New Edge : " << *s1 << " <--> " << s3 << endl;
            //cout << "New Edge : " << *s2 << " <--> " << s3 << endl;
        }


        else
        {
            int type = (s1->layer == s2->layer) ? CROSS : VIA_CROSS;
            
            _tp->add_edge(n1, n2, type);
            //s1->adjs.push_back(n2);
            //s2->adjs.push_back(n1);
            //s1->type[n2] = type;
            //s2->type[n1] = type;
        }

    }

#ifdef DEBUG_TOPOLOGY
    #pragma omp critical(PRINT)
    {
        cout << "Topology Generation <" << get_net(netid)->name << ">" <<  endl;

        for(size_t i=0; i < topologies[netid].segments.size(); i++)
        {
            cout << topologies[netid].segments[i] << endl;

        }

        cout << endl;
        for(size_t i=0; i < topologies[netid].edges.size(); i++)
        {
            int n1 = topologies[netid].edges[i].first;
            int n2 = topologies[netid].edges[i].second;
            int type = topologies[netid].get_type(n1, n2);
            cout << "Edge : " << topologies[netid].segments[n1] << " <--> " << topologies[netid].segments[n2] << " ";

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
    }
#endif
}


void Router::subnet_segmentation(int netid, int p1, int p2, vector<int> gcells)
{    
    Topology* tp = &topologies[netid];
    int g1 = gcells[0];

    //
    int cnt =0;
    int num_segs = 0;

    //cout << "< Subnet Segmentation #Gcells : " << gcells.size() << " >" << endl;

    if(gcells.size() == 1)
    {
        if(find(tp->gcells.begin(), tp->gcells.end(), gcells[0]) == tp->gcells.end())
        {
            tp->gcells.push_back(gcells[0]);
        }
        Segment s;
        s.id = tp->segments.size();
        s.g1 = g1;
        s.g2 = g1;
        s.layer = grid[g1]->z;
        s.direction = grid.direction(g1,g1);
        s.access = true;
        s.leaf = true; //leafEnd;
        s.pins.push_back(p1);
        s.ll.x = min(grid[g1]->rect.ll.x, grid[g1]->rect.ll.x);
        s.ll.y = min(grid[g1]->rect.ll.y, grid[g1]->rect.ll.y);
        s.ur.x = max(grid[g1]->rect.ur.x, grid[g1]->rect.ur.x);
        s.ur.y = max(grid[g1]->rect.ur.y, grid[g1]->rect.ur.y);
        tp->pin2seg[p1] = s.id;
        tp->segments.push_back(s);
        tp->trees[grid[g1]->z].insert({ seg(pt(grid[g1]->x, grid[g1]->y), pt(grid[g1]->x, grid[g1]->y)), s.id });


        cnt += s.pins.size();

        if(p2 != INT_MAX)
        {
            int g2 = grid.index(grid[g1]->x, grid[g1]->y, grid[g1]->z + 1);
            if(find(tp->gcells.begin(), tp->gcells.end(), g2) == tp->gcells.end())
            {
                tp->gcells.push_back(g2);
            }

            Segment s2;
            s2.id = tp->segments.size();
            s2.g1 = g2; //grid.index(grid[g1]->x, grid[g1]->y, grid[g1]->z + 1);
            s2.g2 = g2; //grid.index(grid[g1]->x, grid[g1]->y, grid[g1]->z + 1);
            s2.layer = grid[g2]->z;
            s2.direction = grid.direction(g2, g2);///(s.direction == VERTICAL) ? HORIZONTAL : VERTICAL;
            s2.access = false;
            s2.leaf = false; //leafEnd;
            s2.ll.x = min(grid[g2]->rect.ll.x, grid[g2]->rect.ll.x);
            s2.ll.y = min(grid[g2]->rect.ll.y, grid[g2]->rect.ll.y);
            s2.ur.x = max(grid[g2]->rect.ur.x, grid[g2]->rect.ur.x);
            s2.ur.y = max(grid[g2]->rect.ur.y, grid[g2]->rect.ur.y);
            tp->segments.push_back(s2);
            tp->trees[s2.layer].insert({ seg(pt(grid[g2]->x, grid[g2]->y), pt(grid[g2]->x, grid[g2]->y)), s2.id });           


            Segment s3;
            s3.id = tp->segments.size();
            s3.g1 = g1;
            s3.g2 = g1;
            s3.layer = grid[g1]->z;
            s3.direction = grid.direction(g1,g1);///(s.direction == VERTICAL) ? HORIZONTAL : VERTICAL;
            s3.access = true;
            s3.leaf = true; //leafEnd;
            s3.pins.push_back(p2);
            s3.ll.x = min(grid[g1]->rect.ll.x, grid[g1]->rect.ll.x);
            s3.ll.y = min(grid[g1]->rect.ll.y, grid[g1]->rect.ll.y);
            s3.ur.x = max(grid[g1]->rect.ur.x, grid[g1]->rect.ur.x);
            s3.ur.y = max(grid[g1]->rect.ur.y, grid[g1]->rect.ur.y);
            tp->pin2seg[p2] = s3.id;
            tp->segments.push_back(s3);
            tp->trees[grid[g1]->z].insert({ seg(pt(grid[g1]->x, grid[g1]->y), pt(grid[g1]->x, grid[g1]->y)), s3.id });
            cnt += s.pins.size();
        }

    
    }
    else
    {
        int num_segs = 0;
        vector<pair<int,int>> _segs;
        for(size_t i=0; i < gcells.size(); i++)
        {
            if(find(tp->gcells.begin(), tp->gcells.end(), gcells[i]) == tp->gcells.end())
            {
                tp->gcells.push_back(gcells[i]);
            }

            if(i!=0)
            {
                int g2 = gcells[i-1];
                int g3 = gcells[i];

                bool diffZ = (grid[g1]->z != grid[g3]->z) ? true : false;
                bool diffXY = (grid[g1]->x != grid[g3]->x && grid[g1]->y != grid[g3]->y) ? true : false;

                if(diffZ || diffXY)
                {
                    _segs.push_back( { g1, g2 } );
                    g1 = (diffXY) ? g2 : g3;

                }

                if((i+1) == gcells.size())
                {
                    _segs.push_back( { g1, g3 } );
                }
            }
        }

        if(_segs.size() == 1 && p2 != INT_MAX)
        {
            int g1 = _segs[0].first;
            int g2 = _segs[0].second;

/
            Pin* _pin1 = get_pin(p1);
            Pin* _pin2 = get_pin(p2);
            
            int g11, g12, g21, g22;

            if(_pin1->num_access < _pin2->num_access)
            {
                g11 = g1;
                g12 = g1;
                g21 = g1;
                g22 = g2;
            }
            else
            {
                g11 = g1;
                g12 = g2;
                g21 = g2;
                g22 = g2;
            }


            Segment s1;
            s1.id = tp->segments.size();
            s1.g1 = g11;
            s1.g2 = g12;
            s1.layer = grid[g2]->z;
            s1.direction = grid.direction(g11, g12);
            s1.access = true;
            s1.pins.push_back(p1);
            s1.ll.x = min(grid[g11]->rect.ll.x, grid[g12]->rect.ll.x);
            s1.ll.y = min(grid[g11]->rect.ll.y, grid[g12]->rect.ll.y);
            s1.ur.x = max(grid[g11]->rect.ur.x, grid[g12]->rect.ur.x);
            s1.ur.y = max(grid[g11]->rect.ur.y, grid[g12]->rect.ur.y);
            tp->pin2seg[p1] = s1.id;
            tp->segments.push_back(s1);
            tp->trees[s1.layer].insert({ seg(pt(grid[g11]->x, grid[g11]->y), pt(grid[g12]->x, grid[g12]->y)), s1.id });
           

            Segment s3;
            s3.id = tp->segments.size();
            s3.g1 = g21;
            s3.g2 = g22;
            s3.layer = grid[g2]->z;
            s3.direction = grid.direction(g21, g22);
            s3.access = true;
            s3.pins.push_back(p2);
            s3.ll.x = min(grid[g21]->rect.ll.x, grid[g22]->rect.ll.x);
            s3.ll.y = min(grid[g21]->rect.ll.y, grid[g22]->rect.ll.y);
            s3.ur.x = max(grid[g21]->rect.ur.x, grid[g22]->rect.ur.x);
            s3.ur.y = max(grid[g21]->rect.ur.y, grid[g22]->rect.ur.y);
            tp->pin2seg[p2] = s3.id;
            tp->segments.push_back(s3);
            tp->trees[s3.layer].insert({ seg(pt(grid[g21]->x, grid[g21]->y), pt(grid[g22]->x, grid[g22]->y)), s3.id });
        }
        else
        {

            for(size_t i=0; i < _segs.size(); i++)
            {
                int g1 = _segs[i].first;
                int g2 = _segs[i].second;



                Segment s;
                s.id = tp->segments.size();
                s.g1 = g1;
                s.g2 = g2;
                s.layer = grid[g1]->z;
                s.direction = grid.direction(g1, g2);
                s.access = false;
                s.leaf = false;

                if(i==0 && p2 != INT_MAX)
                {
                    s.leaf = true;
                    s.access = true;
                    s.pins.push_back(p2);
                    tp->pin2seg[p2] = s.id;
                }

                if(i == _segs.size() - 1)
                {
                    s.leaf = true;
                    s.access = true;
                    s.pins.push_back(p1);
                    tp->pin2seg[p1] = s.id;
                }

                s.leaf = ((g1 == gcells[0]) && (p2 != INT_MAX)) ? true : false;
                s.ll.x = min(grid[g1]->rect.ll.x, grid[g2]->rect.ll.x);
                s.ll.y = min(grid[g1]->rect.ll.y, grid[g2]->rect.ll.y);
                s.ur.x = max(grid[g1]->rect.ur.x, grid[g2]->rect.ur.x);
                s.ur.y = max(grid[g1]->rect.ur.y, grid[g2]->rect.ur.y);

                tp->segments.push_back(s);
                tp->trees[grid[g1]->z].insert({ seg(pt(grid[g1]->x, grid[g1]->y), pt(grid[g2]->x, grid[g2]->y)), s.id });
            }

        }

    }


}

bool HGR::Router::check_parallel_edge_connectivity(int netid, int lNum)
{
    bool broken_edge = false;
    Topology* _tp = get_topology(netid);
   
    vector<pair<int,int>> _edges = _tp->edges;

    for(size_t i=0; i < _edges.size(); i++)
    {
        int s1 = _edges[i].first;
        int s2 = _edges[i].second;
        
        if(_tp->get_type(s1, s2) != PRL)
            continue;

        Segment* _s1 = get_segment(netid, s1);
        Segment* _s2 = get_segment(netid, s2);
        if(_s1->layer != lNum)
            continue;

        if(!_s1->assign || !_s2->assign)
        {
            cout << "Not Assigned Segment Exist!!" << endl;
        }


        if(_s1->track != _s2->track)
        {
            
            
            int xl = max( min(grid[_s1->g1]->x, grid[_s1->g2]->x), min(grid[_s2->g1]->x, grid[_s2->g2]->x) );
            int xh = min( max(grid[_s1->g1]->x, grid[_s1->g2]->x), max(grid[_s2->g1]->x, grid[_s2->g2]->x) );
            int yl = max( min(grid[_s1->g1]->y, grid[_s1->g2]->y), min(grid[_s2->g1]->y, grid[_s2->g2]->y) );
            int yh = min( max(grid[_s1->g1]->y, grid[_s1->g2]->y), max(grid[_s2->g1]->y, grid[_s2->g2]->y) );
            int lh = _s1->layer + 1;

            // Remove edge and Add new Segment, Edge
            _tp->remove_edge(s1, s2);
            
            vector<pair<seg,int>> queries;
            _tp->trees[lh].query(bgi::intersects(seg(pt(xl, yl), pt(xh, yh))), back_inserter(queries));

            if(queries.size() > 0)
            {
                int s3 = queries[0].second;
                _tp->add_edge(s1, s3, VIA_CROSS);
                _tp->add_edge(s2, s3, VIA_CROSS);
#ifdef REPORT_TOPOLOGY
                cout << get_net(netid)->name  
                    << " Edge (" << s1 << " " << s2 << ") removed -> Edge (" << s1 << " " << s3 << ") and Edge (" << s2 << " " << s3 << ") added" << endl;
#endif
            }
            else
            {
                int g1 = grid.index(xl, yl, lh);
                int g2 = grid.index(xl, yl, lh);

                Segment _s3;
                _s3.id = _tp->segments.size();
                _s3.g1 = g1;
                _s3.g2 = g2;
                _s3.direction = grid.direction(g1, g2);
                _s3.layer = lh; //_s1->layer + 1;
                _tp->segments.push_back(_s3);
                
                _tp->trees[lh].insert( { seg( pt(xl, yl), pt(xh, yh) ), _s3.id } );

                int s3 = _s3.id;
                _tp->add_edge(s1, s3, VIA_CROSS);
                _tp->add_edge(s2, s3, VIA_CROSS);
#ifdef REPORT_TOPOLOGY
                cout << get_net(netid)->name  
                    << " Edge (" << s1 << " " << s2 << ") removed -> Edge (" << s1 << " " << s3 << ") and Edge (" << s2 << " " << s3 << ") added" << endl;
#endif
            }

            broken_edge = true;
            //cout << _s1 << endl;
            //cout << _s2 << endl;
            //cout << "Edge (" << s1 << " " << s2 << ") removed -> Edge (" << s1 << " " << s3 << ") and Edge (" << s2 << " " << s3 << ") added" << endl;
        }
    }

#ifdef DEBUG_TOPOLOGY
    if(get_net(netid)->name == "net99")
    {
        cout << "Topology Update -> " << get_net(netid)->name << endl;
        for(size_t i=0; i < topologies[netid].edges.size(); i++)
        {
            int n1 = topologies[netid].edges[i].first;
            int n2 = topologies[netid].edges[i].second;
            int type = topologies[netid].get_type(n1, n2);
            cout << "Edge : Segment[" << n1 << "] <--> Segment[" << n2 << "] ";
            //topologies[netid].segments[n1] << " <--> " << topologies[netid].segments[n2] << " ";

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
    }
#endif
    return broken_edge;
}


bool HGR::Router::check_topology_connectivity(int netid)
{
    bool broken_edge = false;
    Topology* _tp = get_topology(netid);
    for(size_t i=0; i < _tp->edges.size(); i++)
    {
        
        int s1 = _tp->edges[i].first;
        int s2 = _tp->edges[i].second;
        if(_tp->get_type(s1, s2) != PRL)
            continue;
        Segment* _s1 = get_segment(netid, s1);
        Segment* _s2 = get_segment(netid, s2);

        if(_s1->assign != _s2->assign)
        {
            if(!_s1->assign && !_s2->assign)
                continue;

            if(_s1->track != _s2->track)
            {
                int xl = max( min(grid[_s1->g1]->x, grid[_s1->g2]->x), min(grid[_s2->g1]->x, grid[_s2->g2]->x) );
                int xh = min( max(grid[_s1->g1]->x, grid[_s1->g2]->x), max(grid[_s2->g1]->x, grid[_s2->g2]->x) );
                int yl = max( min(grid[_s1->g1]->y, grid[_s1->g2]->y), min(grid[_s2->g1]->y, grid[_s2->g2]->y) );
                int yh = min( max(grid[_s1->g1]->y, grid[_s1->g2]->y), max(grid[_s2->g1]->y, grid[_s2->g2]->y) );
                
                int g1 = grid.index(xl, yl, _s1->layer+1);
                int g2 = grid.index(xl, yl, _s1->layer+1);

                Segment _s3;
                _s3.id = _tp->segments.size();
                _s3.g1 = g1;
                _s3.g2 = g2;
                _s3.direction = grid.direction(g1, g2);
                _tp->segments.push_back(_s3);

                int s3 = _s3.id;
                
                // Remove edge and Add new Segment, Edge
                _tp->remove_edge(s1, s2);
                _tp->add_edge(s1, s3, VIA_CROSS);
                _tp->add_edge(s2, s3, VIA_CROSS);

                broken_edge = true;

                cout << get_net(netid)->name  << " Edge (" << s1 << " " << s2 << ") removed -> Edge (" << s1 << " " << s3 << ") and Edge (" << s2 << " " << s3 << ") added" << endl;
            }
        }
        else
        {
            cout << *_s1 << endl;
            cout << *_s2 << endl;
            cout << "Not Assigned!" << endl;
            exit(0);
        }
    }

    return broken_edge;
}





*/
