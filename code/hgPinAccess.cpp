#include "hgRouter.h"
#include "hgDRC.h"
using namespace HGR;

bool HGR::pin_to_track_intersects(int _pin, int _track)
{
    
    Pin* p = get_pin(_pin);
    Track* t = get_track(_track);
    return exist(p->pref, _track) || exist(p->npref, _track);
}


bool HGR::pin_access_point(int _pin, int _track, Point<int> _pt1, Point<int> &_pt2, Point<int> &_pt3)
{
    Pin* p      = get_pin(_pin);
    Track* t    = get_track(_track);

    if(pin_to_track_intersects(_pin, _track))
    {
        // 
        Point<int> ll(INT_MAX, INT_MAX);
        Point<int> ur(INT_MIN, INT_MIN);
        for(size_t i=0; i < p->rects.size(); i++)
        {
            if(p->rects[i].first != t->layer) continue;

            if(bg::intersects(convert(p->rects[i].second), convert(Rect<int>(t->ll, t->ur))))
            {
                if(bg::intersects(convert(_pt1), convert(p->rects[i].second)))
                {
                    // Case1 : Only VIA (STACK)
                    _pt2 = _pt1;
                    return true;
                }
                else
                {
                    // Case2 : Wire + VIA (TOUCH)
                    if(t->direction == VERTICAL)
                    {
                        ll.x = t->ll.x;
                        ur.x = t->ur.x;
                        ll.y = min(p->rects[i].second.ll.y, ll.y);
                        ur.y = max(p->rects[i].second.ur.y, ur.y);
                    }
                    else
                    {
                        ll.x = min(p->rects[i].second.ll.x, ll.x);
                        ur.x = max(p->rects[i].second.ur.x, ur.x);
                        ll.y = t->ll.y;
                        ur.y = t->ur.y;
                    }
                }
            }
        }

        _pt2 = manhattan_distance(_pt1, ll) < manhattan_distance(_pt1, ur) ? ll : ur;
        
        return true;
    }
    else
    {
        // Case3    : Wire + Wire + VIA
        // Case3-1  : On-Track
        vector<int> &tracks = is_preferred(t->layer, t->direction) ? p->npref : p->pref;

        double      min_ps = DBL_MAX;
        Point<int>   min_pt2, min_pt3;

        for(size_t i=0; i < tracks.size(); i++)
        {
            Track* t1 = t;
            Track* t2 = get_track(tracks[i]);

            int lNum1 = t1->layer, lNum2 = t2->layer;

            Point<int> pt1, pt2, pt3, temp;
            pt1 = _pt1;

            if(lNum1 != lNum2) continue;

            if(!line_to_line_intersection(t1->ll, t1->ur, t2->ll, t2->ur, pt2))
            {
                cout << "??1" << endl;
                exit(0);
            }

            if(!pin_access_point(_pin, t2->id, pt2, pt3, temp))
            {
                cout << "??2" << endl;
                exit(0);
            }

            Rect<int> shape1 = wire_shape(t1->layer, pt1, pt2);
            Rect<int> shape2 = wire_shape(t1->layer, pt2, pt3);

            double PS   = drc->check_violation_metal(p->net, t1->layer, shape1) 
                        + drc->check_violation_metal(p->net, t2->layer, shape2) 
                        + drc->non_sufficient_overlap(_pin, t2->layer, shape2);

            if(min_ps > PS)
            {
                min_ps = PS;
                min_pt2 = pt2;
                min_pt3 = pt3;
            }
        }

        // Case3-2 : Off-Track
        for(size_t i=0; i < p->rects.size(); i++)
        {

            int layer = p->rects[i].first;
            Rect<int> rect = p->rects[i].second;

            if(layer != t->layer)
                continue;
           
            int dir1, dir2;
            Point<int> pt1, pt2, pt3;
            pt1 = _pt1;

            if(t->direction == VERTICAL)
            {
                dir1 = VERTICAL;
                dir2 = HORIZONTAL;
                int x2  = t->ll.x;
                int y2  = (rect.ll.y + rect.ur.y) / 2;
                int x31 = rect.ll.x;
                int x32 = rect.ur.x;
                int y3  = (rect.ll.y + rect.ur.y) / 2;

                if(manhattan_distance(x2, y2, x31, y3) < manhattan_distance(x2, y2, x32, y3))
                {
                    pt2 = Point<int>(x2, y2);
                    pt3 = Point<int>(x31, y3);
                }
                else
                {
                    pt2 = Point<int>(x2, y2);
                    pt3 = Point<int>(x32, y3);
                }
            }
            else
            {
                dir2 = VERTICAL;
                dir1 = HORIZONTAL;
                int x2  = (rect.ll.x + rect.ur.x) / 2;
                int y2  = t->ll.y;
                int x3  = (rect.ll.x + rect.ur.x) / 2;
                int y31 = rect.ll.y;
                int y32 = rect.ur.y;

                if(manhattan_distance(x2, y2, x3, y31) < manhattan_distance(x2, y2, x3, y32))
                {
                    pt2 = Point<int>(x2, y2);
                    pt3 = Point<int>(x3, y31);
                }
                else
                {
                    pt2 = Point<int>(x2, y2);
                    pt3 = Point<int>(x3, y32);
                }
            }

        

            Rect<int> shape1 = wire_shape(t->layer, pt1, pt2);
            Rect<int> shape2 = wire_shape(t->layer, pt2, pt3);

            double PS   = drc->check_violation_metal(p->net, layer, shape1) 
                        + drc->check_violation_metal(p->net, layer, shape2) 
                        + drc->non_sufficient_overlap(p->id, layer, shape2);

            if(min_ps > PS)
            {
                min_ps  = PS;
                min_pt2 = pt2;
                min_pt3 = pt3;
            }
        }

        _pt2 = min_pt2;
        _pt3 = min_pt3;
          
        return false;
    }
}

bool HGR::Pin::isHit(int _layer, Point<int> _pt)
{
    for(size_t i=0; i < rects.size(); i++)
    {
        if(_layer != rects[i].first)
            continue;

        
        if(inside(_pt, rects[i].second))
            return true;
    }

    return false;
}


