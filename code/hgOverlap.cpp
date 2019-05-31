#include "hgRouter.h"
#include "hgTypeDef.h"
#include <utility>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_base_map.hpp>
#include <boost/format.hpp>

using namespace HGR;

void merge_wire(Rect<int> &wire1, Rect<int> wire2)
{
    int direct = 0;
    if ((wire1.ur.x - wire1.ll.x) == 0 && (wire1.ur.y - wire1.ll.y) == 0) //point
        cout << "point" << endl;
    else if ((wire1.ur.x - wire1.ll.x) == 0)
        direct = VERTICAL;
    else if ((wire1.ur.y - wire1.ll.y) == 0)
        direct = HORIZONTAL;
    else {
        exit(0);
    }

    if(direct == HORIZONTAL)
    {
        wire1.ll.x = min(wire1.ll.x, wire2.ll.x);
        wire1.ur.x = max(wire1.ur.x, wire2.ur.x);
    }
    else if(direct == VERTICAL)
    {
        wire1.ll.y = min(wire1.ll.y, wire2.ll.y);
        wire1.ur.y = max(wire1.ur.y, wire2.ur.y);
    }
    
}

void HGR::convert_to_output_format(int _net, vector<string> &_outputs)
{
    namespace bi = boost::icl;

    Net* net        = get_net(_net);
    Topology* tp    = get_topology(_net);

    if(!tp->routed)
        return;

    vector<pair<int,Rect<int>>> off_track_wires;
    dense_hash_map<int, bi::interval_set<int>> intervals;
    intervals.set_empty_key(INT_MAX);

//cout << get_net(_net)->name << endl;
    for(size_t i=0; i < tp->segments.size(); i++)
    {
        Segment* s  = get_segment(_net, i);
        Wire* w     = &s->wire;
   
        if(s->assign)
        {
            if(w->track == OFF_TRACK)
            {
                off_track_wires.push_back({w->layer, w->line});
            }
            else
            {
                intervals[w->track] += (get_track(w->track)->direction == VERTICAL) ?
                    bi::interval<int>::closed(w->line.ll.y, w->line.ur.y) :
                    bi::interval<int>::closed(w->line.ll.x, w->line.ur.x);
            }
        }

        // Pin Extension
        for(size_t j=0; j < s->exts.size(); j++)
        {
            Ext* pExt = &s->exts[j];

            for(size_t k=0; k < pExt->wires.size(); k++)
            {
                Wire* w = &pExt->wires[k];    
                if(w->track == OFF_TRACK)
                {
                    off_track_wires.push_back({w->layer, w->line});
                }
                else
                {
                    
                    intervals[w->track] += (get_track(w->track)->direction == VERTICAL) ?
                        bi::interval<int>::closed(w->line.ll.y, w->line.ur.y) :
                        bi::interval<int>::closed(w->line.ll.x, w->line.ur.x);
                }
            }
        
            for(size_t k=0; k < pExt->vias.size(); k++)
            {

                Via* via = &pExt->vias[k];
                string out_format = get_metal(via->cut+1)->name + " ( " + to_string(via->origin.x) + " " + to_string(via->origin.y) + " ) " + via->viaType;
                _outputs.push_back(out_format);
            }

        }



    }


    for(auto& it : tp->edge)
    {
        Edge* e = &it.second;
        if(e->isVia)
        {
            Via* via = &e->via;
            string out_format = get_metal(via->cut+1)->name + " ( " + to_string(via->origin.x) + " " + to_string(via->origin.y) + " ) " + via->viaType;
            _outputs.push_back(out_format);
        }
    }

    for(size_t i=0; i < net->terminals.size(); i++)
    {
        Pin* pin = get_pin(net->terminals[i]);
        
        for(size_t j=0; j < pin->rects.size(); j++)
        {
            int layer = pin->rects[j].first;
            Rect<int> rect = pin->rects[j].second;
            
            vector<int> tracks;

            db->get_tracks(layer, rect, true, tracks);
            db->get_tracks(layer, rect, false, tracks);

            for(size_t k=0; k < tracks.size(); k++)
            {
                Track* track = get_track(tracks[k]);
                if(intervals.find(track->id) == intervals.end())
                    continue;
                
                int lower = (track->direction == VERTICAL) ? rect.ll.y : rect.ll.x;
                int upper = (track->direction == VERTICAL) ? rect.ur.y : rect.ur.x;

                intervals[track->id] -= 
                    bi::interval<int>::open(lower, upper);
            }
        }
    }

    for(auto& it : intervals)
    {
        Track* track = get_track(it.first);
        bi::interval_set<int> &iset = it.second;

        bi::interval_set<int>::iterator it_cur = iset.begin();
        bi::interval_set<int>::iterator it_end = iset.end();

        while(it_cur != it_end)
        {
            int x1 = (track->direction == VERTICAL) ? track->ll.x : it_cur->lower();
            int x2 = (track->direction == VERTICAL) ? track->ur.x : it_cur->upper();
            int y1 = (track->direction == VERTICAL) ? it_cur->lower() : track->ll.y;
            int y2 = (track->direction == VERTICAL) ? it_cur->upper() : track->ur.y;
            int layer = track->layer;

            string out_format = 
                get_metal(layer)->name + 
                " ( " + to_string(x1) + " " + to_string(y1) + " )" +
                " ( " + to_string(x2) + " " + to_string(y2) + " )";
            _outputs.push_back(out_format);
            it_cur++;
        }
    }


    size_t i, j =0;

    for (i = 0 ; i < off_track_wires.size() ; i++)
    {
        int lNum = off_track_wires[i].first;

        for (j = i+1 ; j < off_track_wires.size() ; j++)
        {
            if (lNum != off_track_wires[j].first)
                continue;

            Rect<int> _line1 = off_track_wires[i].second;
            Rect<int> _line2 = off_track_wires[j].second;

            if(line_overlap(_line1, _line2))
            {
#ifdef DEBUG_OVERLAP
                cout << "merged wire -------------" << endl
                     << off_track_wires[i].second << endl;
#endif
                merge_wire(off_track_wires[i].second, off_track_wires[j].second);
#ifdef DEBUG_OVERLAP
                cout << off_track_wires[j].second << endl
                     << ">>>>>>" << off_track_wires[i].second << endl;
#endif
                off_track_wires.erase(off_track_wires.begin()+j); // added
            }
        }

        int x1 = off_track_wires[i].second.ll.x;
        int y1 = off_track_wires[i].second.ll.y;
        int x2 = off_track_wires[i].second.ur.x;
        int y2 = off_track_wires[i].second.ur.y;

        string out_format = 
            get_metal(lNum)->name + 
            " ( " + to_string(x1) + " " + to_string(y1) + " )" +
            " ( " + to_string(x2) + " " + to_string(y2) + " )";

        _outputs.push_back(out_format);
    }
}
   
