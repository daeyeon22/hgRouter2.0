#include "hgTypeDef.h"
#include "hgRouter.h"
#include <omp.h>

using namespace HGR;

void HGR::parsing()
{
    cout << "lef : " << ckt->lef << endl;
    cout << "def : " << ckt->def << endl;
    cout << "gud : " << ckt->guide << endl;
    cout << "out : " << ckt->outfile << endl;

    ckt->read_ispd2019_lef(ckt->lef);
    ckt->read_ispd2019_def(ckt->def);
    ckt->read_ispd2019_guide(ckt->guide);
}

void HGR::Router::init()
{
    topologies = vector<Topology>(num_nets());

    for(int i=0; i < num_nets(); i++)
    {
        topologies[i].net = i;
        topologies[i].trees = vector<SegRtree>(num_layers());
    }
}



void HGR::init()
{
#ifdef CHECK_RUNTIME
    double we1, we2;
    we1 = measure.elapse_time(); 
#endif

    cout << "Start Initialize" << endl;
    ckt->init();


    rou->topologies = vector<Topology>(num_nets());
    rou->init();
    db->init(num_layers());
    
    

    cout << "Track Initialize" << endl;
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_tracks(); i++)
    {
        Track* curT = get_track(i);
        Layer* curL = get_metal(curT->layer);
        int id = curT->id;
        int lNum = curT->layer;
        bool isPref = (curL->direction == curT->direction) ? true : false;

        seg ts(pt(curT->ll.x, curT->ll.y), pt(curT->ur.x, curT->ur.y));
        
        if(isPref)
        {
            #pragma omp critical(PREF)
            {
                db->preferred[lNum].insert({ts, id});
            }
        }
        else
        {
            //#pragma omp critical(NPREF)
            #pragma omp critical(NPREF)
            {
                db->nonPreferred[lNum].insert({ts, id});
            }
        }
    }



    // Decide Gcell Size
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
    
    for(auto& it : vSize)
    {
        if(it.second > vMax)
        {
            vMax = it.second;
            width = it.first;
        }
    }

    for(auto& it : hSize)
    {
        if(it.second > hMax)
        {
            hMax = it.second;
            height = it.first;
        }
    }

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


    int GCwidth = width/3, GCheight = height/3;
    int numCols = (ur.x - ll.x)/GCwidth;
    int numRows = (ur.y - ll.y)/GCheight;
    int numLayers = ckt->metals.size();

    // Grid initialize
    grid->init(Rect<int>(ll, ur), numCols, numRows, numLayers, GCwidth, GCheight);


    cout << "Add Special Nets" << endl;
    
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < num_nets(); i++)
    {
        Net* net = get_net(i);

        for(size_t j=0; j < net->wires.size(); j++)
        {
            Wire* wire      = &net->wires[j];
            int wireid      = wire->id * num_nets() + net->id;
            int layer       = wire->layer;
            Rect<int> rect  = wire->metalShape;


            cout << net->name << " " << get_metal(layer)->name << " " << rect << endl;

            // Add metal shape into DB
            db->add_metal(wireid, layer, rect);
        
            // Gcell Weighting
            int g1 = grid->index(layer, rect.ll);
            int g2 = grid->index(layer, rect.ur);

            for(int x = grid->gcell(g1)->x; x <= grid->gcell(g2)->x; x++)
            {
                for(int y = grid->gcell(g1)->y; y <= grid->gcell(g2)->y; y++)
                {
                    Gcell* gcell = grid->gcell(grid->index(x, y, layer));
                    int orig_area = area(gcell->rect);
                    int over_area = area(gcell->rect, rect);

                    #pragma omp critical(GCELL)
                    gcell->weight += 100.0 * over_area/orig_area;
                }
            }
        }

        for(size_t j=0; j < net->vias.size(); j++)
        {
            Via* via = &net->vias[j];
            MacroVia* macVia = get_via(via->viaType);

            int wireid = via->id * num_nets() + net->id;

            for(size_t k=0; k < macVia->cuts.size(); k++)
            {
                string layerName = macVia->cuts[k].first;

                for(size_t n=0; n < macVia->cuts[k].second.size(); n++)
                {
                    Rect<double> temp = macVia->cuts[k].second[n];
                    int x1 = via->origin.x + temp.ll.x * lef_unit_microns();
                    int x2 = via->origin.x + temp.ur.x * lef_unit_microns();
                    int y1 = via->origin.y + temp.ll.y * lef_unit_microns();
                    int y2 = via->origin.y + temp.ur.y * lef_unit_microns();
                    int layer = ckt->layer2id[layerName];                        
                    Rect<int> rect(x1, y1, x2, y2);


                    if(is_metal_layer(layerName))
                    {
                        // Add metal shape into DB
                        db->add_metal(wireid, layer, rect);
                        
                        // Gcell Weighting
                        int g1 = grid->index(layer, rect.ll);
                        int g2 = grid->index(layer, rect.ur);

                        for(int x = grid->gcell(g1)->x; x <= grid->gcell(g2)->x; x++)
                        {
                            for(int y = grid->gcell(g1)->y; y <= grid->gcell(g2)->y; y++)
                            {
                                Gcell* gcell = grid->gcell(grid->index(x, y, layer));
                                int orig_area = area(gcell->rect);
                                int over_area = area(gcell->rect, rect);

                                #pragma omp critical(GCELL)
                                gcell->weight += 100.0 * over_area/orig_area;
                            }
                        }
                    }

                    if(is_cut_layer(layerName))
                    {
                        // Add cut shape into DB
                        db->add_cut(wireid, layer, rect);
                    }
                }
            }
            //
        }
    }



    cout << "Pin Initialize (Macro Pin)" << endl;
    /* Pin and Dummy Pin */
    set<int> added;
    #pragma omp parallel for num_threads(1) //nthreads())
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
                    int pinid = isDummy ? DUMMY_PIN : ckt->pin2id[pinName];
                    
                    // Add into db
                    
                    #pragma omp critical(RTREE_PIN) 
                    {
                        db->pins[lNum].insert( { convert(rect2), pinid } );
                        added.emplace(pinid);
                    }

                    if(is_cut_layer(layerName)) continue;
                    
                    int g_ll = grid->index(lNum, rect2.ll);
                    int g_ur = grid->index(lNum, rect2.ur);

                    for(int x=grid->gcell(g_ll)->x; x <= grid->gcell(g_ur)->x; x++)
                    {
                        for(int y=grid->gcell(g_ll)->y; y <= grid->gcell(g_ur)->y; y++)
                        {
                            Gcell* gcell = grid->gcell(grid->index(x, y, lNum));
                            int orig_area = area(gcell->rect);
                            int over_area = area(gcell->rect, rect2);

                            #pragma omp critical(GCELL)
                            gcell->weight += 100.0 * over_area/orig_area;
                        }
                    }
                }
            }
        }

        vector<pair<string,Rect<double>>> obstacles = ckt->macros[c->type].obstacles;
        //cout << endl;
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

            #pragma omp critical(PINS_OBS)
            {
                if(is_metal_layer(layerName))
                    db->pins[lNum].insert( { convert(rect2), DUMMY_PIN } );
                
                if(is_cut_layer(layerName))
                    db->cuts[lNum].insert( { convert(rect2), DUMMY_PIN } );
            }
            
            if(is_cut_layer(layerName)) continue;

            int g_ll = grid->index(lNum, rect2.ll);
            int g_ur = grid->index(lNum, rect2.ur);
            
            for(int x=grid->gcell(g_ll)->x; x <= grid->gcell(g_ur)->x; x++)
            {
                for(int y=grid->gcell(g_ll)->y; y <= grid->gcell(g_ur)->y; y++)
                {
                    Gcell* gcell = grid->gcell(grid->index(x, y, lNum));
                    int orig_area = area(gcell->rect);
                    int over_area = area(gcell->rect, rect2);

                    #pragma omp critical(GCELL)
                    gcell->weight += 100.0 * over_area/orig_area;
                }
            }
        }
    }


    cout << "I/O Pin Initialize" << endl;
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


            ll[lNum].x  = min(ll[lNum].x, rect.ll.x);
            ll[lNum].y  = min(ll[lNum].y, rect.ll.y);
            ur[lNum].x  = max(ur[lNum].x, rect.ur.x);
            ur[lNum].y  = max(ur[lNum].y, rect.ur.y);
            env.ll.x    = min(rect.ll.x, env.ll.x);
            env.ll.y    = min(rect.ll.y, env.ll.y);
            env.ur.x    = max(rect.ur.x, env.ur.x);
            env.ur.y    = max(rect.ur.y, env.ur.y);
            
            if(added.find(i) == added.end())
            {
                // Add into db
                #pragma omp critical(RTREE_PIN)
                db->pins[lNum].insert( { convert(rect), _p->id } );
            }

            SegRtree* pref = get_track_db(lNum, true);
            SegRtree* npref = get_track_db(lNum, false);

            // Preferred Track Resources
            for(SegRtree::const_query_iterator it = pref->qbegin(bgi::intersects(convert(rect))); it != pref->qend(); it++)
            {
                if(!exist(_p->pref, it->second))
                    _p->pref.push_back(it->second);
            }
            
            // Nonpreferred Track Resources
            for(SegRtree::const_query_iterator it = npref->qbegin(bgi::intersects(convert(rect))); it != npref->qend(); it++)
            {
                if(!exist(_p->npref, it->second))
                    _p->npref.push_back(it->second);
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
}

void HGR::Grid3D::init(Rect<int> _area, int _numCols, int _numRows, int _numLayers, int _GCwidth, int _GCheight)
{

    area = _area;
    GCwidth = _GCwidth;
    GCheight = _GCheight;
    numCols = _numCols;
    numRows = _numRows;
    numLayers = _numLayers;
    
    int numGcells = numCols * numRows * numLayers;

    gcells = vector<Gcell>(numGcells);
    assign = vector<int>(numGcells, 0);
    history = vector<int>(numGcells, 0);
    xOffsets = vector<int>(numCols+1, 0);
    yOffsets = vector<int>(numRows+1, 0);
    rows = vector<vector<Panel>>(numLayers, vector<Panel>(numRows));
    cols = vector<vector<Panel>>(numLayers, vector<Panel>(numCols));
   
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i <= numRows; i++)
    {
        yOffsets[i] = GCheight * i + _area.ll.y;
    }
    
    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i <= numCols; i++)
    {
        xOffsets[i] = GCwidth * i + _area.ll.x;
    }

    #pragma omp parallel for num_threads(nthreads())
    for(int lNum = 0 ; lNum < numLayers; lNum++)
    {
        for(int col=0; col < numCols; col++)
        {
            bool isPref = is_preferred(lNum, VERTICAL);
            int x1 = xOffsets[col];
            int x2 = xOffsets[col+1];
            int y1 = yOffsets[0];
            int y2 = yOffsets[numRows];
            Rect<int> rect(x1, y1, x2, y2);
            cols[lNum][col] = Panel(col, 0, lNum, isPref, rect);
            
            db->get_tracks(lNum, rect, isPref, cols[lNum][col].tracks);
        }

        for(int row=0; row < numRows; row++)
        {
            bool isPref = is_preferred(lNum, HORIZONTAL);
            int x1 = xOffsets[0];
            int x2 = xOffsets[numCols];
            int y1 = yOffsets[row];
            int y2 = yOffsets[row+1];
            Rect<int> rect(x1, y1, x2, y2);
            rows[lNum][row] = Panel(0, row, lNum, isPref, rect);
            
            db->get_tracks(lNum, rect, isPref, rows[lNum][row].tracks);
        }
    }


    #pragma omp parallel for num_threads(nthreads())
    for(int i=0; i < numLayers * numRows * numCols; i++)
    {
        
        int x, y, z;
        coord(i, x, y, z);
        Gcell* g = &gcells[i];

        int x1 = xOffsets[x];
        int x2 = xOffsets[x] + GCwidth;
        int y1 = yOffsets[y];
        int y2 = yOffsets[y] + GCheight;
        int index = is_vertical(z) ? x : y;

        g->id = i;
        g->x = x, g->y = y, g->z = z;
        g->direction = preferred(z);
        g->rect = Rect<int>(x1, y1, x2, y2);
        g->edgeCap = panel(z, preferred(z), index)->tracks.size();
        g->weight = 0;
    }
}


void HGR::Circuit::init()
{
    for(int i=0; i < (int)macroVias.size(); i++)
    {
        MacroVia* _via = &macroVias[i];

        if(_via->cuts.size() != 3)
            continue;

        int _layer1     = layer2id[_via->cuts[0].first];
        int _cut        = layer2id[_via->cuts[1].first];
        int _layer2     = layer2id[_via->cuts[2].first];

        int num_rect1   = _via->cuts[0].second.size();
        int num_cut     = _via->cuts[1].second.size();
        int num_rect2   = _via->cuts[2].second.size();

        _via->lower = min(_layer1, _layer2);
        _via->upper = max(_layer1, _layer2);
        _via->cut = _cut;

        if(num_rect1 == 1 && num_cut == 1 && num_rect2 == 1)
        {
            Rect<double> _rect1 = _via->cuts[0].second[0];
            Rect<double> _rect2 = _via->cuts[2].second[0];
            Rect<double> _cutrect = _via->cuts[1].second[0];
            _via->metalShape[_layer1] = _rect1;
            _via->cutShape[_cut] = _cutrect;
            _via->metalShape[_layer2] = _rect2;
            double width1  = _rect1.ur.x - _rect1.ll.x;
            double width2  = _rect2.ur.x - _rect2.ll.x;
            double height1 = _rect1.ur.y - _rect1.ll.y;
            double height2 = _rect2.ur.y - _rect2.ll.y;

            double min_width1 = metals[_layer1].width;
            double min_width2 = metals[_layer2].width;
            int dir1 = (width1 < height1) ? VERTICAL : HORIZONTAL;
            int dir2 = (width2 < height2) ? VERTICAL : HORIZONTAL;
            
            bool isCenter1 = ( (_rect1.ur.x + _rect1.ll.x) == 0 && (_rect1.ur.y + _rect1.ll.y) == 0 ) ? true : false;
            bool isCenter2 = ( (_rect2.ur.x + _rect2.ll.x) == 0 && (_rect2.ur.y + _rect2.ll.y) == 0 ) ? true : false;


            if(isCenter1 && isCenter2)
            {
                via_call_type calltype(_layer1, dir1, _layer2, dir2);
                if(via_type.find(calltype) == via_type.end())
                {
                    via_type.insert( { calltype, _via->name } );
                    _via->isDefault = true;

#ifdef REPORT_VIA
                    cout << "Via call type added --> " << _via->name << " " << _layer1 << " " << dir1 << " " << _layer2 << " " << dir2 << endl;
#endif
                }
            }
        }
    }
}


