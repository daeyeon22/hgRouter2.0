    typedef vector<Vertex*>::iterator VertexIterator;
    typedef vector<int> Path;
    typedef pair<int,int> Edge_t;
    struct Vertex
    {
      public:
        int id;
        double weight;

        vector<Vertex*> adjacents; //Vertices;
        dense_hash_map<int, double> eWeight;

        Vertex()
        {
            eWeight.set_empty_key(INT_MAX);
        }

        bool operator == const (Vertex* _vtx)
        {
            return _vtx->id == id;
        }



        VertexIterator adjcent_vertex_iterator();
        double edge_weight(int _v);
        double vertex_weight();
        int num_adjacents();
    };

    // Weighted Graph
    class WGraph
    {
      private:
        dense_hash_map<int, Vertex> vertex;
        
        set<edge_t> edges;

      public:

        WGraph()
        {
            vertex.set_empty_key(INT_MAX);
        }


        Vertex* get_vertex(int _v);
        bool vertex_exist(int _v);
        bool edge_exist(int _v1, int _v2);
        void add_vertex(int _v, double _w);
        void remove_vertex(int _v);
        void add_edge(int _v1, int _v2, double _w);
        void remove_edge(int _v1, int _v2);

        Path find_shortest_path(vector<int> _multiSource, vector<int> _multiTarget);
    };


void HGR::Router::build_graph_from_guide(set<int> &_rGuide, set<int> &_rSpace)
{
    graph = WGraph();
    typedef tuple<int,int,int> Coord3D;
    Coord3D adjV[] = { Coord3D(0,0,-1), Coord3D(0,0,1), Coord3D(0,1,0), Coord3D(0,-1,0) };
    Coord3D adjH[] = { Coord3D(0,0,-1), Coord3D(0,0,1), Coord3D(1,0,0), Coord3D(-1,0,0) };

    int v1, v2;
    int x1, y1, z1;
    int x2, y2, z2;
    double vertexWeight;
    double edgeWeight;

    for(auto id : _rSpace)
    {
        // Add Vertex
        vertexWeight = grid->overflow(id) + grid->offguide(id, _rGuide);
        graph.add_vertex(id, vertexWeight);
        
        
        grid->coord(id, x1, y1, z1);
        for(int i=0; i < 4; i++)
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


            if(grid->out_of_index(x2, y2, z2))
                continue;

            // Vertex ID1
            v1 = id;
            v2 = grid->index(x2, y2, z2);

            if(graph.edge_exist(v1, v2))
                continue;
            
            if(!graph.vertex_exist(v2))
            {
                // Add Vertex if not exist
                vertexWeight = grid->overflow(v2) + grid->offguide(v2, _rGuide);
                graph.add_vertex(v2, vertexWeight);
            }

            // Add Edge
            edgeWeight = grid->wirelength(v1, v2) + grid->segment(v1, v2) + grid->npref(v1, v2);
            graph.add_edge(v1, v2, edgeWeight);
        }
    }
}


Path HGR::WGraph::find_shortest_path(vector<int> _multiSource, vector<int> _multiTarget)
{
    dense_hash_map<int, HeapNode> nodes;
    nodes.set_empty_key(INT_MAX);
    Heap<HeapNode> heap;
    //
    int minNode = INT_MAX;
    int destNode = INT_MAX;
    double minCost = DBL_MAX;
    bool hasSolution = false;


    for(auto it : vertex)
    {
        int n = it.first;
        nodes[n].init(n, false);
        nodes[n].est = grid->estimation(n, _multiTarget); // 
    }


    for(auto n : _multiSource)
    {
        nodes[n].init(n, true);

        if(exist(_multiTarget, n))
        {
            if(minCost > nodes[n].cost())
            {
                hasSolution = true;
                minNode = n;
                minCost = nodes[n].cost();
            }
        }

        heap.push(nodes[n]);
    }

    int n1, n2;
    HeapNode *cNode, *nNode;
    Vertex *v1, *v2;
    //, *tNode = &nodes[destNode]; 
   
    ///////////////////
    while(!heap.empty())
    {
        HeapNode temp = heap.pop();
        
        n1 = temp.id;
        cNode = &nodes[n1];

        if(temp.cost() != nodes[n1].cost())
            continue;

        if(n1 == minNode)
            break;
        
        v1 = &vertex[n1];

        for(int i=0; i < v1->num_adjacnets(); i++)
        {
            v2 = v1->adjacents[i];

            if(cNode->backtrace = v2->id)
                continue;

            double cost = cNode->cost() + v2->weight + v1->edge_weight(v2->id);

            if(nNode->cost() > cost)
            {
                nNode->act = cost;
                nNode->est = 0;//grid->estimation(cNode->id, _multiTarget);
                nNode->backtrace = cNode->id;
                nNode->depth = cNode->depth + 1;

                if(nNode->est == 0 && minCost > nNode->cost())
                {
                    hasSolution = true;
                    minNode = nNode->id;
                    minCost = nNode->cost();
                }

    
            }


        }


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

            n2 = grid->index(x2, y2, z2);
            nNode = &nodes[n2];

            if(cNode->backtrace == nNode->id)
                continue;

            if(grid->cap(n2) == 0)
                continue;

            if(!exist(_rSpace, n2)) 
                continue;

            double cost = nNode->get_cost(cNode, tNode, _rGuide);

            if(nNode->cost() > cost)
            {
                nNode->update(cNode, tNode, _rGuide);
                
                if(nNode->id == destNode && minCost > nNode->cost())
                {
                    hasSolution = true;
                    minNode = nNode->id;
                    minCost = nNode->cost();
                }

                heap.push(*nNode);
            }
        }
    }



    if(hasSolution)
    {
        int iterNode = minNode;

        Topology* tp = get_topology(_net);
        
        vector<int> paths;
        while(true)
        {
            grid->gc_assign(iterNode);
            paths.push_back(iterNode);

            if(iterNode = nodes[iterNode].backtrace)
                break;

            iterNode = nodes[iterNode].backtrace;
        }

        subnet_segmentation(_net, _p1, _p2, paths);
    }
    else
    {
        
        exit(0);
    }

    return true;

}



void HGR::WGraph::add_vertex(int _v, double _w)
{
    Vertex* vtx = &vertex[_v];
    vtx->id = _v;
    vtx->weight = _w;
}


void HGR::WGraph::add_edge(int _v1, int _v2, double _w)
{
    Vertex* vtx1 = &vertex[_v1];
    Vertex* vtx2 = &vertex[_v2];

    if(!edge_exist(_v1, _v2))
    {
        vtx1->adjcent.push_back(vtx2);
        vtx2->adjcent.push_back(vtx1);
        vtx1->eWeight[_v2] = _w;
        vtx2->eWeight[_v1] = _w;
        edges.insert( { min(_v1, _v2), max(_v1, _v2) } );
    }
}

void HGR::WGraph::remove_vertex(int _v)
{
    if(vertex_exist(_v))
    {
        Vertex* vtx1 = get_vertex(_v);


        for(auto vtx2 : vtx1->adjacent)
        {
            int v1 = vtx1->id;
            int v2 = vtx2->id;
            
            remove_edge(v1, v2);
        }
    }
    vertex.erase(_v);
}

void HGR::WGraph::remove_edge(int _v1, int _v2)
{
   
    Vertex* vtx1 = get_vertex(_v1);
    Vertex* vtx2 = get_vertex(_v2);

    VertexIterator it1 = find(vtx1->adjacents.begin(), vtx1->adjacents.end(), vtx2);
    VertexIterator it2 = find(vtx2->adjacents.begin(), vtx2->adjacents.end(), vtx1);

    vtx1->adjacents.erase(it1);
    vtx2->adjacents.erase(it2);
    vtx1->eWeight.erase(_v2);
    vtx2->eWeight.erase(_v1);

    Edge_t e(min(_v1, _v2), max(_v1, _v2));
    edges.erase(e);
}

bool HGR::WGraph::vertex_exist(int _v)
{
    return vertex.find(_v1) != vertex.end() ? true : false;
}

bool HGR::WGraph::edge_exist(int _v1, int _v2)
{
    return edges.find({ min(_v1, _v2), max(_v1, _v2) }) != edges.end() ? true : false ;
}



