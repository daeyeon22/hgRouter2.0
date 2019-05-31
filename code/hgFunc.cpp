#include "hgRouter.h"
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_base_map.hpp>
#include <boost/format.hpp>

using namespace HGR;

int HGR::nthreads()
{
    return atoi(ckt->threads);
}

int HGR::distance(Point<int> _pt, Rect<int> _line)
{
    return (int)(bg::distance(convert(_pt), convert(_line)));
}

inline double det(double a, double b, double c, double d)
{
    return (double)(a*d - b*c);
}

SegRtree* HGR::get_track_db(int _layer, bool _isPref)
{
    return _isPref ? &db->preferred[_layer] : &db->nonPreferred[_layer];
}

BoxRtree* HGR::get_metal_db(int _layer)
{
    return &db->metals[_layer];
}

BoxRtree* HGR::get_pin_db(int _layer)
{
    return &db->pins[_layer];
}

BoxRtree* HGR::get_cut_db(int _cut)
{
    return &db->cuts[_cut];
}

bool HGR::get_tracks_on_line(int lNum, int dir, Rect<int> _line, vector<int> &_tracks)
{
    SegRtree* _tree = get_track_db(lNum, is_preferred(lNum, dir));
    for(SegRtree::const_query_iterator it = _tree->qbegin(bgi::intersects(convert(_line))); it != _tree->qend(); it++)
    {
        _tracks.push_back(it->second);
    }

    return _tracks.size() > 0;
}

bool HGR::pin_to_line_overlapping_boundary(int pinid, int lNum, Rect<int> _line, Point<int> &ll, Point<int> &ur)
{
    Pin* _pin = get_pin(pinid);
    ll = Point<int>(INT_MAX, INT_MAX), 
    ur = Point<int>(INT_MIN, INT_MIN);
    int dir = direction(_line.ll, _line.ur);
    bool found = false;
    for(size_t i=0; i < _pin->rects.size(); i++)
    {
        int _layer = _pin->rects[i].first;
        Rect<int> _rect = _pin->rects[i].second;

        if(_layer != lNum)
            continue;

        if(!bg::intersects(convert(_line), convert(_rect)))
            continue;

        found = true;
        ll.x = (dir == VERTICAL) ? _line.ll.x : min(ll.x, _rect.ll.x);
        ur.x = (dir == VERTICAL) ? _line.ur.x : max(ur.x, _rect.ur.x);
        ll.y = (dir == VERTICAL) ? min(ll.y, _rect.ll.y) : _line.ll.y;
        ur.y = (dir == VERTICAL) ? max(ur.y, _rect.ur.y) : _line.ur.y;
    }

    return found;
}


bool HGR::pin_to_line_overlapping_intervals(int pinid, int lNum, Rect<int> _line, vector<Rect<int>> &_overlaps)
{
    using namespace boost::icl;

    Pin* _pin = get_pin(pinid);
    interval_set<int> _interval;

    int dir = direction(_line.ll, _line.ur);

    for(size_t i=0; i < _pin->rects.size(); i++)
    {
        int _layer = _pin->rects[i].first;
        Rect<int> _rect = _pin->rects[i].second;

        if(_layer != lNum)
            continue;

        if(!bg::intersects(convert(_line), convert(_rect)))
            continue;

        int lower = (dir == VERTICAL) ? _rect.ll.y : _rect.ll.x;
        int upper = (dir == VERTICAL) ? _rect.ur.y : _rect.ur.x;

        _interval += interval<int>::open(lower, upper);
    }

    for(auto& it : _interval)
    {
        Rect<int> _overlap;
        _overlap.ll.x = (dir != VERTICAL) ? it.lower() : _line.ll.x;
        _overlap.ur.x = (dir != VERTICAL) ? it.upper() : _line.ur.x;
        _overlap.ll.y = (dir != VERTICAL) ? _line.ll.y : it.lower();
        _overlap.ur.y = (dir != VERTICAL) ? _line.ur.y : it.upper();
        _overlaps.push_back(_overlap);
    }

    return _overlaps.size() > 0;
}



///Calculate intersection of two lines.
///\return true if found, false if not found or error
bool HGR::track_to_track_intersection(int t1, int t2, Point<int> &_pt)
{
    Track* _t1 = get_track(t1);
    Track* _t2 = get_track(t2);
    if(abs(_t1->layer - _t2->layer) > 1)
        return false;

    return line_to_line_intersection(_t1->ll, _t1->ur, _t2->ll, _t2->ur, _pt);
}

bool HGR::intersects(Point<int> _pt, Rect<int> _line)
{
    bool x_axis = (_pt.x <= _line.ur.x && _pt.x >= _line.ll.x) ? true : false;
    bool y_axis = (_pt.y <= _line.ur.y && _pt.y >= _line.ll.y) ? true : false;
    return x_axis && y_axis;
}


bool HGR::line_overlap(Rect<int> _line1, Rect<int> _line2)
{
    int x1 = max(_line1.ll.x, _line2.ll.x);
    int x2 = min(_line1.ur.x, _line2.ur.x);
    int y1 = max(_line1.ll.y, _line2.ll.y);
    int y2 = min(_line1.ur.y, _line2.ur.y);

    if( x1 > x2 || y1 > y2 )
        return false;
    
    if( x1 != x2 && y1 != y2 )
        return false;

    if(manhattan_distance(x1, y1, x2, y2) == 0)
        return false;

    return true;
}

int HGR::line_overlap_length(Rect<int> _line1, Rect<int> _line2)
{
    int x1 = max(_line1.ll.x, _line2.ll.x);
    int x2 = min(_line1.ur.x, _line2.ur.x);
    int y1 = max(_line1.ll.y, _line2.ll.y);
    int y2 = min(_line1.ur.y, _line2.ur.y);

    if( x1 > x2 || y1 > y2 )
        return 0;
    
    if( x1 != x2 && y1 != y2 )
        return 0;

    return manhattan_distance(x1, y1, x2, y2);
}


bool HGR::is_covered_by_pin(int pinid, Point<int> _pt)
{
    for(auto it : get_pin(pinid)->rects)
    {
        Rect<int> _bbox = it.second;
        if(intersects(_pt, _bbox))
            return true;
    }   
    return false;
}

Point<int> HGR::line_to_line_intersection(Rect<int> _line1, Rect<int> _line2)
{
    Point<int> iPt;
    line_to_line_intersection(_line1.ll, _line1.ur, _line2.ll, _line2.ur, iPt);
    return iPt;
}


bool HGR::line_to_line_intersection(Point<int> ll1, Point<int> ur1, Point<int> ll2, Point<int> ur2, Point<int> &ptOut)
{
    //http://mathworld.wolfram.com/Line-LineIntersection.html
    // Line(1)
    double x1 = ll1.x;
    double x2 = ur1.x;
    double y1 = ll1.y;
    double y2 = ur1.y;
    
    // Line(2)
    double x3 = ll2.x;
    double x4 = ur2.x;
    double y3 = ll2.y;
    double y4 = ur2.y;
    
    double detL1 = (double)det(x1, y1, x2, y2);
    double detL2 = (double)det(x3, y3, x4, y4);
    double x1mx2 = x1 - x2;
    double x3mx4 = x3 - x4;
    double y1my2 = y1 - y2;
    double y3my4 = y3 - y4;

    double xnom = (double)det(detL1, x1mx2, detL2, x3mx4);
    double ynom = (double)det(detL1, y1my2, detL2, y3my4);
    double denom = (double)det(x1mx2, y1my2, x3mx4, y3my4);

    /*
    int detL1 = det(x1, y1, x2, y2);
    int detL2 = det(x3, y3, x4, y4);
    int x1mx2 = x1 - x2;
    int x3mx4 = x3 - x4;
    int y1my2 = y1 - y2;
    int y3my4 = y3 - y4;

    int xnom = det(detL1, x1mx2, detL2, x3mx4);
    int ynom = det(detL1, y1my2, detL2, y3my4);
    int denom = det(x1mx2, y1my2, x3mx4, y3my4);
    */
    if(denom == 0.0)//Lines don't seem to cross
    {
        //ixOut = NAN;
        //iyOut = NAN;
        return false;
    }

    int ixOut = (int) ( 1.0* xnom / denom + 0.5);   
    int iyOut = (int) ( 1.0* ynom / denom + 0.5);
    
    //cout << "Line(1)        : (" << x1 << " " << y1 << ") (" << x2 << " " << y2 << ")" << endl;
    //cout << "Line(2)        : (" << x3 << " " << y3 << ") (" << x4 << " " << y4 << ")" << endl;
    //cout << "Intersection   : (" << ixOut << " " << iyOut << ")" << endl;
    ptOut.x = ixOut;
    ptOut.y = iyOut; //= Point<int>(ixOut, iyOut);
    if(!isfinite(ixOut) || !isfinite(iyOut)) //Probably a numerical issue
        return false;

    return true; //All OK
}

Net* HGR::get_net(int id)
{
    return &ckt->nets[id];
}

Pin* HGR::get_pin(int id)
{
    return &ckt->pins[id];
}

Cell* HGR::get_cell(int id)
{
    return &ckt->cells[id];
}

Layer* HGR::get_metal(int lNum)
{
    return &ckt->metals[lNum];
}

Layer* HGR::get_cut(string _layer)
{
    return &ckt->cuts[ckt->layer2id[_layer]];
}

Layer* HGR::get_metal(string _layer)
{
    return &ckt->cuts[ckt->layer2id[_layer]];
}


Layer* HGR::get_cut(int lNum)
{
    return &ckt->cuts[lNum];
}

Track* HGR::get_track(int id)
{
    return &ckt->tracks[id];
}

MacroVia* HGR::get_via(string type)
{
    return &ckt->macroVias[ckt->macroVia2id[type]];
}

Topology* HGR::get_topology(int id)
{
    return &rou->topologies[id];
    
}

Segment* HGR::get_segment(int netid, int segid)
{
    return &rou->topologies[netid].segments[segid];
}

Rect<int> HGR::get_pin_to_pin_interval(int p1, int p2, int trackid)
{
    Pin* _pin1 = get_pin(p1);
    Pin* _pin2 = get_pin(p2);
    Track* _track = get_track(trackid);
    seg _seg(convert(_track->ll), convert(_track->ur));
    Point<int> ll1(INT_MAX, INT_MAX), ur1(INT_MIN, INT_MIN);
    Point<int> ll2(INT_MAX, INT_MAX), ur2(INT_MIN, INT_MIN);
    
    for(size_t i=0; i < _pin1->rects.size(); i++)
    {
        int _layer = _pin1->rects[i].first;
        Rect<int> _rect = _pin1->rects[i].second;
        if(_track->layer != _layer)
            continue;

        if(!bg::intersects(convert(_rect), _seg))
            continue;

        ll1.x = (_track->direction == VERTICAL) ? _track->ll.x : min(_rect.ll.x, ll1.x);
        ur1.x = (_track->direction == VERTICAL) ? _track->ur.x : max(_rect.ur.x, ur1.x);
        ll1.y = (_track->direction == VERTICAL) ? min(_rect.ll.y, ll1.y) : _track->ll.y;
        ur1.y = (_track->direction == VERTICAL) ? max(_rect.ur.y, ur1.y) : _track->ur.y;
    }

    for(size_t i=0; i < _pin2->rects.size(); i++)
    {
        int _layer = _pin2->rects[i].first;
        Rect<int> _rect = _pin2->rects[i].second;
        if(_track->layer != _layer)
            continue;

        if(!bg::intersects(convert(_rect), _seg))
            continue;

        ll2.x = (_track->direction == VERTICAL) ? _track->ll.x : min(_rect.ll.x, ll2.x);
        ur2.x = (_track->direction == VERTICAL) ? _track->ur.x : max(_rect.ur.x, ur2.x);
        ll2.y = (_track->direction == VERTICAL) ? min(_rect.ll.y, ll2.y) : _track->ll.y;
        ur2.y = (_track->direction == VERTICAL) ? max(_rect.ur.y, ur2.y) : _track->ur.y;
    }

    if(manhattan_distance(ll1, ur2) < manhattan_distance(ll2, ur1))
    {
        Rect<int> _line;
        _line.ll.x = min(ll1.x, ur2.x);
        _line.ll.y = min(ll1.y, ur2.y);
        _line.ur.x = max(ll1.x, ur2.x);
        _line.ur.y = max(ll1.y, ur2.y);
        return _line; 
    }
    else
    {
        Rect<int> _line;
        _line.ll.x = min(ll2.x, ur1.x);
        _line.ll.y = min(ll2.y, ur1.y);
        _line.ur.x = max(ll2.x, ur1.x);
        _line.ur.y = max(ll2.y, ur1.y);
        return _line; 
    }
}


bool HGR::is_valid_via_type(string type)
{
    return ckt->macroVia2id.find(type) == ckt->macroVia2id.end() ? false : true;
}

bool HGR::is_cut_layer(string layerName)
{
    if(get_cut(ckt->layer2id[layerName])->name == layerName)
        return true;
    else    
        return false;
}

bool HGR::is_metal_layer(string layerName)
{
    if(get_metal(ckt->layer2id[layerName])->name == layerName)
        return true;
    else    
        return false;
}

bool HGR::get_track_index(int lNum, int dir, Point<int> _pt, int &trackid)
{
    SegRtree* rt = get_track_db(lNum, is_preferred(lNum, dir));

    vector<pair<seg, int>> queries;
    rt->query(bgi::intersects(convert(_pt)), back_inserter(queries));

    if(queries.size() == 0)
        return false;

    trackid = queries[0].second;

    return true;
}

int HGR::get_track_index(int _layer, int _dir, Point<int> _pt)
{
    SegRtree* rt = get_track_db(_layer, is_preferred(_layer, _dir));

    vector<pair<seg, int>> queries;
    rt->query(bgi::intersects(convert(_pt)), back_inserter(queries));

    if(queries.size() == 0)
        return OFF_TRACK;

    return queries[0].second;
}



int HGR::get_track_index(int lNum, Rect<int> line)
{
    SegRtree* rt = get_track_db(lNum, is_preferred(lNum, direction(line.ll, line.ur)));

    vector<pair<seg, int>> queries;
    rt->query(bgi::intersects(convert(line)), back_inserter(queries));

    if(queries.size() == 0)
        return OFF_TRACK;

    return queries[0].second;
}

bool HGR::is_vertical(int lNum)
{
    return (ckt->metals[lNum].direction == VERTICAL) ? true : false;
}

bool HGR::is_horizontal(int lNum)
{
    return (ckt->metals[lNum].direction == HORIZONTAL) ? true : false;
}

bool HGR::is_preferred(int lNum, int direction)
{
    return preferred(lNum) == direction ? true : false;
}

int HGR::preferred(int lNum)
{
    return ckt->metals[lNum].direction;
}

int HGR::non_preferred(int lNum)
{
    return preferred(lNum) == VERTICAL ? HORIZONTAL : VERTICAL;
}

int HGR::direction(Point<int> pt1, Point<int> pt2)
{
    int dx = abs(pt1.x - pt2.x);
    int dy = abs(pt1.y - pt2.y);
    if(dx > 0 && dy == 0)
    {
        return HORIZONTAL;
    }
    else if(dx == 0 && dy > 0)
    {
        return VERTICAL;
    }
    else
    {
        
        return VERTICAL;
    }
}

int HGR::direction(int x1, int y1, int x2, int y2)
{
    return direction(Point<int>(x1,y1), Point<int>(x2, y2));
}

int HGR::die_width()
{
    return ckt->dieArea.ur.x - ckt->dieArea.ll.x; 
}

int HGR::die_height()
{
    return ckt->dieArea.ur.y - ckt->dieArea.ll.y;
}

int HGR::num_layers()
{
    return (int)ckt->metals.size();
}

int HGR::num_nets()
{
    return (int)ckt->nets.size();
}

int HGR::num_pins()
{
    return (int)ckt->pins.size();
}

int HGR::num_tracks()
{
    return (int)ckt->tracks.size();
}

int HGR::num_cells()
{
    return (int)ckt->cells.size();
}

int HGR::wire_width(int lNum)
{
    return (int)lef_unit_microns() * get_metal(lNum)->width;

}

int HGR::min_spacing(int lNum)
{
    return parallel_run_length_spacing(lNum, 0, 0); //(int)lef_unit_microns() * get_metal(lNum)->spacings[0].minSpacing;
}

int HGR::min_width(int _layer)
{
    return (int)(lef_unit_microns() * get_metal(_layer)->minWidth);
}


int HGR::cut_spacing(int lNum)
{
    return (int)(get_cut(lNum)->spacings[0].minSpacing * lef_unit_microns());
}


int HGR::min_area(int lNum)
{
    return (int)(pow(lef_unit_microns(),2) * get_metal(lNum)->area);
}

int HGR::min_length(int lNum)
{
    return (int)(lef_unit_microns() * get_metal(lNum)->area / get_metal(lNum)->width);
}

bool HGR::end_of_line_rule_parallel_edge(int lNum, int& eolSpacing, int& eolWidth, int& eolWithin, int &parSpacing, int &parWithin)
{
    Layer* _layer = get_metal(lNum);
    for(size_t i=0; i < _layer->spacings.size(); i++)
    {
        Spacing* _spacing = &_layer->spacings[i];
        if(_spacing->type == "PARALLELEDGE")
        {
            eolSpacing = lef_unit_microns() * _spacing->minSpacing;
            eolWidth = lef_unit_microns() * _spacing->eolWidth;
            eolWithin = lef_unit_microns() * _spacing->eolWithin;
            parSpacing = lef_unit_microns() * _spacing->parSpacing;
            parWithin = lef_unit_microns() * _spacing->parWithin;
            return true;
        }
    }

    return false;

}


bool HGR::end_of_line_rule(int lNum, int& eolSpacing, int& eolWidth, int& eolWithin)
{
    Layer* _layer = get_metal(lNum);
    for(size_t i=0; i < _layer->spacings.size(); i++)
    {
        Spacing* _spacing = &_layer->spacings[i];
        if(_spacing->type == "ENDOFLINE")
        {
            eolSpacing = lef_unit_microns() * _spacing->minSpacing;
            eolWidth = lef_unit_microns() * _spacing->eolWidth;
            eolWithin = lef_unit_microns() * _spacing->eolWithin;
            return true;
        }
    }

    return false;
}

int HGR::get_spacing(Rect<int> _shape1, Rect<int> _shape2, bool vertical)
{
    if(bg::intersects(convert(_shape1), convert(_shape2)))
        return 0;
    
    int _spacing = (vertical) ? 
        min(abs(_shape1.ll.y - _shape2.ur.y), abs(_shape1.ur.y - _shape2.ll.y)) :
        min(abs(_shape1.ll.x - _shape2.ur.x), abs(_shape1.ur.x - _shape2.ll.x));
    return _spacing;
}


int HGR::run_length(int dir, Rect<int> _line1, Rect<int> _line2)
{
    int lower = (dir == VERTICAL) ? max(_line1.ll.y, _line2.ll.y) : max(_line1.ll.x, _line2.ll.x);
    int upper = (dir == VERTICAL) ? min(_line1.ur.y, _line2.ur.y) : min(_line1.ur.x, _line2.ur.x);

    return max( 0, upper - lower);
}



/* 수정 필요 */
int HGR::parallel_run_length_spacing(int lNum, int length, int width)
{
    Layer* _layer = get_metal(lNum);
    
    int i = lb(_layer->table.length, length);
    int j = lb(_layer->table.width, width);
    if(i > 0 && !exist(_layer->table.length, length))
        i--;

    if(j > 0 && !exist(_layer->table.width, width))
        j--;
    
    return _layer->table.spacing[i][j];
}

int HGR::manhattan_distance(Point<int> pt1, Point<int> pt2)
{
    return abs(pt1.x - pt2.x) + abs(pt1.y - pt2.y);
}

int HGR::manhattan_distance(int x1, int y1, int x2, int y2)
{
    return abs(x2 - x1) + abs(y2 - y1);
}


int HGR::lef_unit_microns()
{
    return ckt->LEFdist2Microns;
}

int HGR::def_unit_microns()
{
    return ckt->DEFdist2Microns;
}

bool HGR::remove(vector<int> &container, int elem)
{
    vector<int>::iterator it = find(container.begin(), container.end(), elem);
    if(it != container.end())
    {
        container.erase(it);
        return true;
    }
    else
    {
        return false;
    }
}

bool HGR::remove(vector<pair<int,int>> &container, pair<int,int> elem)
{
    vector<pair<int,int>>::iterator it = find(container.begin(), container.end(), elem);
    if(it != container.end())
    {
        container.erase(it);
        return true;
    }
    else
    {
        return false;
    }
}


bool HGR::exist(vector<pair<int,int>> &container, pair<int,int> elem)
{
    return find(container.begin(), container.end(), elem) != container.end() ? true : false;
}

bool HGR::exist(vector<int> &container, int elem)
{
    return find(container.begin(), container.end(), elem) != container.end() ? true : false;
}

bool HGR::exist(set<int> &_set, int elem)
{
    return _set.find(elem) == _set.end() ? false : true;
}


int HGR::lb(vector<int> &v, int elem)
{
    vector<int>::iterator lower = lower_bound(v.begin(), v.end(), elem);
    return min((int)v.size()-1, (int)(lower - v.begin()));
}

int HGR::ub(vector<int> &v, int elem)
{
    vector<int>::iterator upper = upper_bound(v.begin(), v.end(), elem);
    return min((int)v.size()-1, (int)(upper - v.begin()));
}

int HGR::area(Rect<int> rect)
{
    return (rect.ur.y - rect.ll.y) * (rect.ur.x - rect.ll.x);
}


int HGR::area(Rect<int> rect1, Rect<int> rect2)
{
    Point<int> ll(INT_MIN, INT_MIN), ur(INT_MAX, INT_MAX);
    ll.x = max(rect1.ll.x, rect2.ll.x);
    ll.y = max(rect1.ll.y, rect2.ll.y);
    ur.x = min(rect1.ur.x, rect2.ur.x);
    ur.y = min(rect1.ur.y, rect2.ur.y);
    return max(ur.y - ll.y, 0) * max(ur.x - ll.x, 0);
}

Rect<int> HGR::overlaps(Rect<int> rect1, Rect<int> rect2)
{
    Point<int> ll(INT_MIN, INT_MIN), ur(INT_MAX, INT_MAX);
    ll.x = max(rect1.ll.x, rect2.ll.x);
    ll.y = max(rect1.ll.y, rect2.ll.y);
    ur.x = min(rect1.ur.x, rect2.ur.x);
    ur.y = min(rect1.ur.y, rect2.ur.y);
    return Rect<int>(ll, ur); 
}

bool HGR::inside(Point<int> _pt, Rect<int> _rect)
{
    bool xInside = (_rect.ll.x <= _pt.x && _pt.x <= _rect.ur.x) ? true:false;
    bool yInside = (_rect.ll.y <= _pt.y && _pt.y <= _rect.ur.y) ? true:false;
    return xInside && yInside;
}


double HGR::pin_to_pin_distance(int p1, int p2)
{
    double minDist = DBL_MAX;
    for(auto& it1 : get_pin(p1)->rects)
    {
        for(auto& it2 : get_pin(p2)->rects)
        {
            minDist = min( bg::distance(convert(it1.second), convert(it2.second)), minDist );
        }
    }


    return minDist;
}

Point<int> HGR::center(Rect<int> _rect)
{
    return Point<int>((_rect.ll.x + _rect.ur.x)/2, (_rect.ll.y + _rect.ur.y)/2);
}


box HGR::convert(Rect<int> rect)
{
    return box(convert(rect.ll), convert(rect.ur));
}

pt HGR::convert(Point<int> point)
{
    return pt(point.x, point.y);
}

Rect<int> HGR::buffer(Rect<int> rect, int buffer_distance)
{
    Rect<int> buffered(rect);
    buffered.ll.x -= buffer_distance;
    buffered.ll.y -= buffer_distance;
    buffered.ur.x += buffer_distance;
    buffered.ur.y += buffer_distance;
    return buffered;
}

Rect<int> HGR::buffer(Rect<int> rect, int buffer_horizontal, int buffer_vertical)
{
    Rect<int> buffered(rect);
    buffered.ll.x -= buffer_horizontal;
    buffered.ll.y -= buffer_vertical;
    buffered.ur.x += buffer_horizontal;
    buffered.ur.y += buffer_vertical;
    return buffered;
}

Rect<int> HGR::convert(seg s)
{
    return Rect<int>((int)(bg::get<0,0>(s)+0.5), (int)(bg::get<0,1>(s)+0.5), (int)(bg::get<1,0>(s)+0.5), (int)(bg::get<1,1>(s)+0.5));
}


Rect<int> HGR::convert(box b)
{
    return Rect<int>((int)(bg::get<0,0>(b)+0.5), (int)(bg::get<0,1>(b)+0.5), (int)(bg::get<1,0>(b)+0.5), (int)(bg::get<1,1>(b)+0.5));
}

Point<int> HGR::convert(pt p)
{
    return Point<int>((int)(bg::get<0>(p)+0.5), (int)(bg::get<1>(p)+0.5));
}


string HGR::get_macro_via(int l1 , int d1, int l2, int d2) /* 1 : below | 2 : upper */
{
    if(l1 > l2)
    {
        swap(l1, l2);
        swap(d1, d2);
    }
    
    string call_type = "";
    via_call_type key(l1, d1, l2, d2);
    if(ckt->via_type.find(key) != ckt->via_type.end())
    {
        call_type = ckt->via_type[key];
    }
    else
    {
        via_call_type key2(l1, d2, l2, d1);
        if(ckt->via_type.find(key2) != ckt->via_type.end())
        {
            call_type = ckt->via_type[key2];
        }
    }

    return call_type;
}



/* MacroVia */
Rect<int> HGR::MacroVia::upper_metal_shape(Point<int> _pt)
{
    int lefUnit = lef_unit_microns();
    Rect<double> relative = metalShape[upper];
    Rect<int> shape;
    shape.ll.x = _pt.x + relative.ll.x * lefUnit;
    shape.ll.y = _pt.y + relative.ll.y * lefUnit;
    shape.ur.x = _pt.x + relative.ur.x * lefUnit;
    shape.ur.y = _pt.y + relative.ur.y * lefUnit;
    return shape;
}

Rect<int> HGR::MacroVia::lower_metal_shape(Point<int> _pt)
{
    int lefUnit = lef_unit_microns();
    Rect<double> relative = metalShape[lower];
    Rect<int> shape;
    shape.ll.x = _pt.x + relative.ll.x * lefUnit;
    shape.ll.y = _pt.y + relative.ll.y * lefUnit;
    shape.ur.x = _pt.x + relative.ur.x * lefUnit;
    shape.ur.y = _pt.y + relative.ur.y * lefUnit;
    return shape;
}


Rect<int> HGR::MacroVia::cut_shape(Point<int> _pt)
{
    int lefUnit = lef_unit_microns();
    Rect<double> relative = cutShape[cut];
    Rect<int> shape;
    shape.ll.x = _pt.x + relative.ll.x * lefUnit;
    shape.ll.y = _pt.y + relative.ll.y * lefUnit;
    shape.ur.x = _pt.x + relative.ur.x * lefUnit;
    shape.ur.y = _pt.y + relative.ur.y * lefUnit;
    return shape;
}


Rect<int> HGR::wire_shape(int _layer, Rect<int> _line)
{
    int halfWidth = wire_width(_layer) / 2;
    return buffer(_line, halfWidth);
}

Rect<int> HGR::wire_shape(int _layer, Point<int> _pt1, Point<int> _pt2)
{
    return wire_shape(_layer, get_line(_pt1, _pt2));
}

Rect<int> HGR::get_line(Point<int> _pt1, Point<int> _pt2)
{
    Rect<int> line;
    line.ll.x = min(_pt1.x, _pt2.x);
    line.ll.y = min(_pt1.y, _pt2.y);
    line.ur.x = max(_pt1.x, _pt2.x);
    line.ur.y = max(_pt1.y, _pt2.y);   
    return line;
}





