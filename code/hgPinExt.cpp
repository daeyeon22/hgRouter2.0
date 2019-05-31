#include "hgCircuit.h"
#include "hgRouter.h"
#include <boost/format.hpp>

//#define DEBUG_EXT

using namespace HGR;
int extended = 0;

void HGR::pin_extension()
{
    int totalPinCnt     = num_pins();
    int onTrackAccess   = 0;

    // Pin Sorting
    vector<pair<int,int>> pinArea;
    for(int i=0; i < num_pins(); i++)
    {
        pinArea.push_back( { i, area(get_pin(i)->envelope[0]) });
    }

    sort(pinArea.begin(), pinArea.end(), [](const pair<int,int> &left, const pair<int,int> &right){
            return left.second < right.second;
            });



    for(int i=0; i < num_pins(); i++)
    {
        int pinid = pinArea[i].first;
        if(pin_extension(pinid))
        {
            onTrackAccess++;
        }
    }

    cout << "Total On-track Access (#total pin)  : " << onTrackAccess << " (" << num_pins() << ")" << endl;
}



bool HGR::pin_extension(int _pin)
{
    vector<Ext> candidates;

    // Target Pin
    Pin* pin = get_pin(_pin);

    // Pin bbox
    for(auto it_e : pin->envelope)
    {
        int l1 = it_e.first;
        int l2 = l1 + 1;

        Rect<int> envelope = it_e.second;

        if(area(envelope) == 0)
            continue;

        // Search Area
        bool offTrackSearch = true;
        int bufferDist = 2 * lef_unit_microns() * max( get_metal(l1)->xPitch, get_metal(l2)->yPitch );
        Rect<int> searchArea = buffer(envelope, bufferDist);

        if( l1 == 0 )
        {
            // Standard Cell Pin
            vector<Rect<int>> intervals;
            if(on_track_hit_intervals(_pin, l1, l2, intervals))
            {
                for(auto& interval : intervals)
                {

                    int accessTrack     = get_track_index(l2, interval);
                    int pitchSize       = get_metal(l1)->xPitch;
                    int intervalLength  = manhattan_distance(interval.ll, interval.ur);
                   

                    if(intervalLength > 2*pitchSize)
                    {
                        // OnTrack Grid Point
                        SegRtree* tree = &db->preferred[l1];
                        for(SegRtree::const_query_iterator it = tree->qbegin(bgi::intersects(convert(interval)));
                                it != tree->qend(); it++)
                        {


                            int d2 = preferred(l2);
                            Point<int> hp = line_to_line_intersection(convert(it->first), interval);

                            via_call_type callType1(l1, VERTICAL,   l2, preferred(l2));
                            via_call_type callType2(l1, HORIZONTAL, l2, preferred(l2));
                            string viaName1 = ckt->via_type[callType1];
                            string viaName2 = ckt->via_type[callType2];

                            Ext ext1, ext2;
                            if(make_extension(_pin, l1, l2, accessTrack, viaName1, hp, ext1))
                            {
                                candidates.push_back(ext1);
                            }
                            if(make_extension(_pin, l1, l2, accessTrack, viaName2, hp, ext2))
                            {
                                candidates.push_back(ext2);
                            }
                        }
                    }
                    else
                    {
                        // OnTrack Centor of Interval
                        int d2 = preferred(l2);
                        Point<int> hp;
                        hp.x = (interval.ll.x + interval.ur.x) / 2;
                        hp.y = (interval.ll.y + interval.ur.y) / 2;

                        via_call_type callType1(l1, VERTICAL,   l2, preferred(l2));
                        via_call_type callType2(l1, HORIZONTAL, l2, preferred(l2));
                        string viaName1 = ckt->via_type[callType1];
                        string viaName2 = ckt->via_type[callType2];

                        Ext ext1, ext2;
                        if(make_extension(_pin, l1, l2, accessTrack, viaName1, hp, ext1))
                        {
                            candidates.push_back(ext1);
                            offTrackSearch = ext1.penalty > 0 ? true : false; 
                        }
                        if(make_extension(_pin, l1, l2, accessTrack, viaName2, hp, ext2))
                        {
                            candidates.push_back(ext2);
                            offTrackSearch = ext2.penalty > 0 ? true : false; 
                        }
                    }

                    
                }

            }


            //////////////////////////////////////////
            if(offTrackSearch)
            {
                for(size_t i=0; i < pin->rects.size(); i++)
                {
                    if(pin->rects[i].first != l1)
                        continue;

                    Rect<int> pinShape = pin->rects[i].second;
                    Point<int> hp = center(pinShape);;
                    int accessTrack = get_track_index(l2, preferred(l2), hp);

                    via_call_type callType1(l1, VERTICAL,   l2, preferred(l2));
                    via_call_type callType2(l1, HORIZONTAL, l2, preferred(l2));
                    string viaName1 = ckt->via_type[callType1];
                    string viaName2 = ckt->via_type[callType2];

                    Ext ext1, ext2;
                    if(make_extension(_pin, l1, l2, accessTrack, viaName1, hp, ext1))
                    {
                        candidates.push_back(ext1);
                        offTrackSearch = ext1.penalty > 0 ? true : false; 
                    }
                    if(make_extension(_pin, l1, l2, accessTrack, viaName2, hp, ext2))
                    {
                        candidates.push_back(ext2);
                        offTrackSearch = ext2.penalty > 0 ? true : false; 
                    }
                }
            }
            // End For
            if(candidates.size() > 1)
            {
                sort(candidates.begin(), candidates.end(), [](const Ext& left, const Ext& right){
                        return left.penalty < right.penalty;
                        });
            }

            //cout << "#candiates : " << candidates.size() << endl;
            if(candidates.size() != 0)
            {
                Ext* minExt = &candidates[0];
                //get_topology(pin->net)->routed = true;


                if(minExt->penalty > 0)
                {
                    cout << boost::format("%s --> (%d %d) %s min penalty %f\n") % pin->name % minExt->access.x % minExt->access.y % minExt->vias[0].viaType % minExt->penalty;
                    drc->report_violation(minExt);
                    cout << endl;
                }


                rou->set_extension(pin->id, minExt);
                
                for(int j=0; j < auto it : candidates)
                {
                    rou->add_extension(pin->id, &it);
                }
                /*
                Segment* seg = create_segment(pin->net);
                //
                minExt->set_id(seg->id * num_nets() + pin->net);

                seg->g1 = grid->index(l2, minExt->vias[0].origin);
                seg->g2 = grid->index(l2, minExt->vias[0].origin);
                seg->layer = l2;
                seg->direction = preferred(l2);
                seg->track = minExt->track;
                seg->assign = false;
                seg->access = true;
                seg->pins.push_back(pin->id);
                seg->exts.push_back(*minExt);
                //
                seg->update_database();
                */
                return true;
            }
        }
        else
        {
            // I/O Pin or Blk Cell

        }
    }

    return false;
}

bool HGR::make_extension(int _pin, int _l1, int _l2, int _track, string _viaType, Point<int> _hp, Ext &_outExt)
{

    Pin* pin = get_pin(_pin);
    if(is_valid_via_type(_viaType))
    {
        if(pin->isHit(_l1, _hp))
        {

            Via via;
            via.id = INT_MAX;
            via.net = pin->net;
            via.cut = _l1;
            via.viaType = _viaType;
            via.origin = _hp;
            via.lower = _l1;
            via.upper = _l2;
            via.cutShape = get_via(_viaType)->cut_shape(_hp);
            via.upperMetalShape = get_via(_viaType)->upper_metal_shape(_hp);
            via.lowerMetalShape = get_via(_viaType)->lower_metal_shape(_hp);


            Ext pExt;
            pExt.pin = pin->id;
            pExt.layer = _l2;
            pExt.track = _track;
            pExt.access = _hp;
            pExt.via = _hp;
            pExt.useVia = true;
            pExt.penalty = drc->check_violation_via(pin->net, _viaType, _hp, DRC_VIA_DEFAULT);                            
            pExt.vias.push_back(via);
            _outExt = pExt;
            return true;
        }
        else
        {
            Point<int> _pt1, _pt2;

            if(pin_access_point(_pin, _track, _hp, _pt1, _pt2))
            {
                
                Rect<int> line = get_line(_hp, _pt1);
                Rect<int> shape = wire_shape(_l1, line);

                Wire wire;
                wire.id = INT_MAX;
                wire.type = STRIPE;
                wire.net = pin->net;
                wire.layer = _l1;
                wire.track = get_track_index(_l1, line);
                //wire.conn1 = 
                //wire.conn2 =
                wire.width = shape.ur.x - shape.ll.x;
                wire.height = shape.ur.y - shape.ll.y;
                wire.line = line;
                wire.metalShape = shape;
                
                Via via;
                via.id = INT_MAX;
                via.net = pin->net;
                via.cut = _l1;
                via.viaType = _viaType;
                via.origin = _hp;
                via.lower = _l1;
                via.upper = _l2;
                via.cutShape = get_via(_viaType)->cut_shape(_hp);
                via.upperMetalShape = get_via(_viaType)->upper_metal_shape(_hp);
                via.lowerMetalShape = get_via(_viaType)->lower_metal_shape(_hp);

                Ext pExt;
                pExt.pin = pin->id;
                pExt.layer = _l2;
                pExt.track = _track;
                pExt.access = _hp;
                pExt.via = _hp;
                pExt.useVia = true;
                pExt.penalty = 
                    drc->check_violation_via(pin->net, _viaType, _hp, DRC_VIA_DEFAULT) + 
                    drc->check_violation_metal(pin->net, _l1, shape);

                pExt.vias.push_back(via);
                pExt.viaConn.push_back(false);

                pExt.wires.push_back(wire);
                pExt.wireConn.push_back(true);

                _outExt = pExt;
                return true;
            }
            else
            {
                Rect<int> line1 = get_line(_hp, _pt1);
                Rect<int> line2 = get_line(_pt1, _pt2);
                Rect<int> shape1 = wire_shape(_l1, line1);
                Rect<int> shape2 = wire_shape(_l1, line2);

                Wire wire1;
                wire1.id = INT_MAX;
                wire1.type = STRIPE;
                wire1.net = pin->net;
                wire1.layer = _l1;
                wire1.track = get_track_index(_l1, line1);
                //wire1.conn1 = 
                //wire1.conn2 =
                wire1.width = shape1.ur.x - shape1.ll.x;
                wire1.height = shape1.ur.y - shape1.ll.y;
                wire1.line = line1;
                wire1.metalShape = shape1;

                
                Wire wire2;
                wire2.id = INT_MAX;
                wire2.type = STRIPE;
                wire2.net = pin->net;
                wire2.layer = _l1;
                wire2.track = get_track_index(_l1, line2);
                //wire1.conn1 = 
                //wire1.conn2 =
                wire2.width = shape2.ur.x - shape2.ll.x;
                wire2.height = shape2.ur.y - shape2.ll.y;
                wire2.line = line2;
                wire2.metalShape = shape2;
                
                Via via;
                via.id = INT_MAX;
                via.net = pin->net;
                via.cut = _l1;
                via.viaType = _viaType;
                via.origin = _hp;
                via.lower = _l1;
                via.upper = _l2;
                via.cutShape = get_via(_viaType)->cut_shape(_hp);
                via.upperMetalShape = get_via(_viaType)->upper_metal_shape(_hp);
                via.lowerMetalShape = get_via(_viaType)->lower_metal_shape(_hp);

                Ext pExt;
                pExt.pin = pin->id;
                pExt.layer = _l2;
                pExt.track = _track;
                pExt.access = _hp;
                pExt.via = _hp;
                pExt.useVia = true;
                pExt.penalty = 
                    drc->check_violation_via(pin->net, _viaType, _hp, DRC_VIA_DEFAULT) + 
                    drc->check_violation_metal(pin->net, _l1, shape1) +
                    drc->check_violation_metal(pin->net, _l1, shape2);

                pExt.vias.push_back(via);
                pExt.viaConn.push_back(false);
                pExt.wires.push_back(wire1);
                pExt.wireConn.push_back(false);
                pExt.wires.push_back(wire2);
                pExt.wireConn.push_back(true);

                _outExt = pExt;
                return true;
            }
        }
    }

    return false;
}




/*
bool HGR::on_track_hit_points(int _pin, int _l1, int _l2, Rect<int> _interval, vector<Point<int>> &_hitpoints)
{



}
*/


bool HGR::on_track_hit_intervals(int _pin, int _l1, int _l2, vector<Rect<int>> & _intervals)
{
    Pin* pin = get_pin(_pin);

    Rect<int> env = pin->envelope[_l1];
    SegRtree* tree = &db->preferred[_l2];

    for(SegRtree::const_query_iterator it = tree->qbegin(bgi::intersects(convert(env))); it != tree->qend(); it++)
    {
        pin_to_line_overlapping_intervals(_pin, _l1, convert(it->first), _intervals);        
    }

    return _intervals.size() > 0 ? true : false;
}

Ext* HGR::get_extension(int _pin)
{
    return &rou->pin2ext[_pin];
}

bool HGR::is_extended(int _pin)
{
    return (rou->pin2ext.find(_pin) == rou->pin2ext.end()) ? false : true;
}



/*
double HGR::Ext::update_penalty(int netid)
{
    double total_PS = 0;
    double total_WL = 0;
    double total_NS = 0;
    for(size_t i=0; i < lines.size(); i++)
    {
        int _layer = layers[i];
        Point<int> _pt1 = lines[i].ll;
        Point<int> _pt2 = lines[i].ur;

        bool eol1 = _layer == layer ? false : true;
        //(_pt1 == via && _layer == layer) ? false : true;
        bool eol2 = _layer == layer ? false : true;//(_pt2 == via && _layer == layer) ? false : true;

        total_WL += rou->weighted_wirelength(layers[i], lines[i]);
        total_PS += rou->check_violation_for_wire(netid, _layer, _pt1, _pt2, eol1, eol2); //direction(lines[i].ll, lines[i].ur), lines[i].ll, lines[i].ur);
       
        if(connect_line[i])
        {
            total_PS += rou->check_violation_for_pin(pin, layers[i], lines[i]);
        }
    }

    for(size_t i=0; i < vias.size(); i++)
    {
        total_PS += rou->check_violation_for_via(netid, via_types[i], vias[i]);
    }

    double ps = total_PS + total_WL;

    if(floor(ps) < floor(penalty))
        //ceil(total_PS + total_WL) < ceil(penalty))
    {
        //penalty = total_PS + total_WL;
        //cout << "why penalty reduced????" << endl;
        //cout << "PS : " << total_PS << " WL : " << total_WL << " previous Penalty : " << penalty << endl;
        //exit(0);
    }


    penalty = total_PS + total_WL;

    return penalty;
}
*/

