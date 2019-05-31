#include "hgRouter.h"
#include "hgTypeDef.h"


using namespace HGR;


bool HGR::is_local_net(int netid)
{
    Net* _net = get_net(netid);
    
    if(_net->terminals.size() == 2)
    {
        Pin* _pin1 = get_pin(_net->terminals[0]);
        Pin* _pin2 = get_pin(_net->terminals[1]);
        for(size_t i=0; i < _net->guides.size(); i++)
        {
            int lNum = _net->guides[i].first;
            Rect<int> bbox = _net->guides[i].second;

            Rect<int> env1 = _pin1->envelope[lNum];
            Rect<int> env2 = _pin2->envelope[lNum];

            int area1 = area(bbox, env1);
            int area2 = area(bbox, env2);
            if(area1 > 0 && area2 > 0)
                return true;
        }
    }
    return false;
}

void HGR::preroute()
{
    int num_local_net = 0;
    cout << "Preroute start" << endl;
    for(int i=0; i < num_nets(); i++)
    {
        
        if(!is_local_net(i))
            continue;

        Net* _net = get_net(i);
#ifdef REPORT_LOCALNET
        //cout << _net->name << " is local net" << endl;
#endif
        
        
        if(preroute_localnet_m1(i, _net->terminals[0], _net->terminals[1]))
        {
            num_local_net++;
        }



    }

    cout << "Preroute #nets " << num_local_net << endl;
}



bool HGR::preroute_localnet_m1(int _net, int _p1, int _p2)
{
    Topology* tp = get_topology(_net);
    Pin* pin1 = get_pin(_p1);
    Pin* pin2 = get_pin(_p2);

   

    sort(pin1->pref.begin(), pin1->pref.end());
    sort(pin2->pref.begin(), pin2->pref.end());

    sort(pin1->npref.begin(), pin1->npref.end());
    sort(pin2->npref.begin(), pin2->npref.end());

    vector<int> share;
    set_intersection(pin1->npref.begin(), pin1->npref.end(), pin2->npref.begin(), pin2->npref.end(), back_inserter(share));
    set_intersection(pin1->pref.begin(), pin1->pref.end(), pin2->pref.begin(), pin2->pref.end(), back_inserter(share));

    if(tp->routed)
    {
        cout << "routed" << endl;
    }

    // No Bending
    if(share.size() > 0)
    {
        for(size_t i=0; i < share.size(); i++)
        {
            if(tp->routed)
                break;

            Track* t = get_track(share[i]);
            int layer = t->layer;
            Rect<int> line = get_pin_to_pin_interval(_p1, _p2, t->id);
            Rect<int> shape = wire_shape(layer, line);
            
            
            double penalty = 
                drc->check_violation_metal(_net, layer, shape) +
                drc->non_sufficient_overlap(_p1, layer, shape) + 
                drc->non_sufficient_overlap(_p2, layer, shape);

            // Metal 1
            if(penalty == 0)
            {
                // Segment Creation
                Segment* seg = create_segment(_net);
                seg->g1 = grid->index(t->layer, line.ll);
                seg->g2 = grid->index(t->layer, line.ur);
                seg->direction = t->direction;
                seg->layer = t->layer;
                seg->track = t->id;
                seg->assign = true;
                seg->access = true;

                // Wire Creation 
                Wire wire;
                wire.id = seg->id * num_nets() + _net;
                wire.type = STRIPE;
                wire.net = _net;
                wire.layer = t->layer;
                wire.track = t->id;
                wire.conn1 = CONN_PIN;
                wire.conn2 = CONN_PIN;
                wire.line = line;
                wire.metalShape = shape;
                wire.width = shape.ur.x - shape.ll.x;
                wire.height = shape.ur.y - shape.ll.y;

                seg->wire = wire;

                // Add Wire into DB
                wire.update_database();
                //db->add_metal(wire.id, wire.layer, wire.rect);

                tp->routed = true;

#ifdef REPORT_LOCALNET
                cout << get_net(_net)->name << " is prerouted (0-bending)" << endl;
#endif
                break;
            }
        }

    }
    else
    {

        if(!tp->routed)
        {
            vector<int> tracks1;
            tracks1.insert(tracks1.end(), pin1->pref.begin(), pin1->pref.end());
            tracks1.insert(tracks1.end(), pin1->npref.begin(), pin1->npref.end());

            // 1-Bending
            for(size_t i=0; i < tracks1.size(); i++)
            {
                int t1 = tracks1[i];
                
                if(pin_to_track_intersects(_p2, t1))
                    continue;

                Track* track1 = get_track(t1);
                int layer = track1->layer;
                
                Point<int> pt11, pt12, pt21, pt22, pt31, pt32;

                if(!pin_to_line_overlapping_boundary(_p1, track1->layer, Rect<int>(track1->ll, track1->ur), pt11, pt12))
                {
                    cout << "!!!" << endl;
                    exit(0);
                }

                if(pin_access_point(_p2, t1, pt11, pt21, pt31))
                {
                    continue;
                }

                if(pin_access_point(_p2, t1, pt12, pt22, pt32))
                {
                    continue;
                }
                
                int dist1 = manhattan_distance(pt11, pt21) + manhattan_distance(pt21, pt31);
                int dist2 = manhattan_distance(pt12, pt22) + manhattan_distance(pt22, pt32);

                Point<int> pt1, pt2, pt3;
                if(dist1 < dist2)
                {
                    pt1 = pt11;
                    pt2 = pt21;
                    pt3 = pt31;
                }
                else
                {
                    pt1 = pt12;
                    pt2 = pt22;
                    pt3 = pt32;
                }

                if(manhattan_distance(pt1, pt2) == 0)
                    continue;

                if(manhattan_distance(pt2, pt3) == 0)
                    continue;

                Rect<int> shape1 = wire_shape(track1->layer, pt1, pt2);
                Rect<int> shape2 = wire_shape(track1->layer, pt2, pt3);


                double penalty = 
                    drc->check_violation_metal(_net, track1->layer, shape1) +
                    drc->check_violation_metal(_net, track1->layer, shape2) +
                    drc->non_sufficient_overlap(_p1, track1->layer, shape1) +
                    drc->non_sufficient_overlap(_p2, track1->layer, shape2);

                if(penalty == 0)
                {

                    Rect<int> line1 = get_line(pt1, pt2);
                    Rect<int> line2 = get_line(pt2, pt3);
                    int t2 = get_track_index(layer, line2);
                    
                    // Segment Creation
                    Segment* seg1 = create_segment(_net);
                    seg1->g1 = grid->index(layer, line1.ll);
                    seg1->g2 = grid->index(layer, line1.ur);
                    seg1->direction = direction(line1.ll, line1.ur);
                    seg1->layer = layer;
                    seg1->track = t1;
                    seg1->assign = true;
                    seg1->access = true;

                    // Wire Creation 
                    Wire wire1;
                    wire1.id = seg1->id * num_nets() + _net;
                    wire1.type = STRIPE;
                    wire1.net = _net;
                    wire1.layer = layer;
                    wire1.track = t1;
                    wire1.conn1 = CONN_PIN;
                    wire1.conn2 = CONN_PIN;
                    wire1.line = line1;
                    wire1.metalShape = shape1;
                    wire1.width = shape1.ur.x - shape1.ll.x;
                    wire1.height = shape1.ur.y - shape1.ll.y;

                    seg1->wire = wire1;

                    // Add Wire into DB
                    wire1.update_database();
                    //db->add_metal(wire1.id, wire1.layer, wire1.rect);

                    // Segment Creation
                    Segment* seg2 = create_segment(_net);
                    seg2->g1 = grid->index(layer, line2.ll);
                    seg2->g2 = grid->index(layer, line2.ur);
                    seg2->direction = direction(line2.ll, line2.ur);
                    seg2->layer = layer;
                    seg2->track = t2;
                    seg2->assign = true;
                    seg2->access = true;

                    // Wire Creation 
                    Wire wire2;
                    wire2.id    = seg2->id * num_nets() + _net;
                    wire2.type  = STRIPE;
                    wire2.net   = _net;
                    wire2.layer = layer;
                    wire2.track = t2;
                    wire2.conn1 = CONN_PIN;
                    wire2.conn2 = CONN_PIN;
                    wire2.line  = line2;
                    wire2.metalShape  = shape2;
                    wire2.width = shape2.ur.x - shape2.ll.x;
                    wire2.height = shape2.ur.y - shape2.ll.y;

                    seg2->wire = wire2;

                    // Add Wire into DB
                    wire2.update_database();
                    //db->add_metal(wire2.id, wire2.layer, wire2.rect);

                    // Topology Update
                    tp->add_edge(seg1->id, seg2->id);
                    tp->set_edge_type(seg1->id, seg2->id, CROSS);
                    tp->routed = true;


#ifdef REPORT_LOCALNET
                    cout << get_net(_net)->name << " is prerouted (1-bending)" << endl;
#endif
                    break;

                }
            }
        }

        if(!tp->routed)
        {
            vector<int> tracks2;
            tracks2.insert(tracks2.end(), pin2->pref.begin(), pin2->pref.end());
            tracks2.insert(tracks2.end(), pin2->npref.begin(), pin2->npref.end());
            
            for(auto t1 : tracks2)
            {
                if(pin_to_track_intersects(_p1, t1))
                    continue;

                Track* track1 = get_track(t1);
                int layer = track1->layer;
                
                /* Get Points */
                Point<int> pt11, pt12, pt21, pt22, pt31, pt32;
                if(!pin_to_line_overlapping_boundary(_p2, track1->layer, Rect<int>(track1->ll, track1->ur), pt11, pt12))
                {
                    cout << "!!!" << endl;
                    exit(0);
                }

                if(pin_access_point(_p1, t1, pt11, pt21, pt31))
                    continue;

                if(pin_access_point(_p1, t1, pt12, pt22, pt32))
                    continue;

                /* Choose */
                int dist1 = manhattan_distance(pt11, pt21) + manhattan_distance(pt21, pt31);
                int dist2 = manhattan_distance(pt12, pt22) + manhattan_distance(pt22, pt32);


                Point<int> pt1, pt2, pt3;
                if(dist1 < dist2)
                {
                    pt1 = pt11;
                    pt2 = pt21;
                    pt3 = pt31;
                }
                else
                {
                    pt1 = pt12;
                    pt2 = pt22;
                    pt3 = pt32;
                }

                if(manhattan_distance(pt1, pt2) == 0)
                    continue;

                if(manhattan_distance(pt2, pt3) == 0)
                    continue;


                /* Check Violation */
                Rect<int> shape1 = wire_shape(track1->layer, pt1, pt2);
                Rect<int> shape2 = wire_shape(track1->layer, pt2, pt3);

                double penalty = 
                    drc->check_violation_metal(_net, track1->layer, shape1) +
                    drc->check_violation_metal(_net, track1->layer, shape2) +
                    drc->non_sufficient_overlap(_p2, track1->layer, shape1) +
                    drc->non_sufficient_overlap(_p1, track1->layer, shape2);


                if(penalty == 0)
                {
                    Rect<int> line1 = get_line(pt1, pt2);
                    Rect<int> line2 = get_line(pt2, pt3);
                    int t2 = get_track_index(layer, line2);
                    
                    // Segment Creation
                    Segment* seg1 = create_segment(_net);
                    seg1->g1 = grid->index(layer, line1.ll);
                    seg1->g2 = grid->index(layer, line1.ur);
                    seg1->direction = direction(line1.ll, line1.ur);
                    seg1->layer = layer;
                    seg1->track = t1;
                    seg1->assign = true;
                    seg1->access = true;

                    // Wire Creation 
                    Wire wire1;
                    wire1.id = seg1->id * num_nets() + _net;
                    wire1.type = STRIPE;
                    wire1.net = _net;
                    wire1.layer = layer;
                    wire1.track = t1;
                    wire1.conn1 = CONN_PIN;
                    wire1.conn2 = CONN_PIN;
                    wire1.line = line1;
                    wire1.metalShape = shape1;
                    wire1.width = shape1.ur.x - shape1.ll.x;
                    wire1.height = shape1.ur.y - shape1.ll.y;

                    seg1->wire = wire1;

                    // Add Wire into DB
                    wire1.update_database();
                    //db->add_metal(wire1.id, wire1.layer, wire1.rect);

                    // Segment Creation
                    Segment* seg2 = create_segment(_net);
                    seg2->g1 = grid->index(layer, line2.ll);
                    seg2->g2 = grid->index(layer, line2.ur);
                    seg2->direction = direction(line2.ll, line2.ur);
                    seg2->layer = layer;
                    seg2->track = t2;
                    seg2->assign = true;
                    seg2->access = true;

                    // Wire Creation 
                    Wire wire2;
                    wire2.id    = seg2->id * num_nets() + _net;
                    wire2.type  = STRIPE;
                    wire2.net   = _net;
                    wire2.layer = layer;
                    wire2.track = t2;
                    wire2.conn1 = CONN_PIN;
                    wire2.conn2 = CONN_PIN;
                    wire2.line  = line2;
                    wire2.metalShape  = shape2;
                    wire2.width = shape2.ur.x - shape2.ll.x;
                    wire2.height = shape2.ur.y - shape2.ll.y;

                    seg2->wire = wire2;

                    // Add Wire into DB
                    wire2.update_database();
                    //db->add_metal(wire2.id, wire2.layer, wire2.rect);

                    // Topology Update
                    tp->add_edge(seg1->id, seg2->id);
                    tp->set_edge_type(seg1->id, seg2->id, CROSS);
                    tp->routed = true;

#ifdef REPORT_LOCALNET
                    cout << get_net(_net)->name << " is prerouted (1-bending)" << endl;
#endif
                    break;
                }
            }
        }

    }
    return tp->routed;
}

