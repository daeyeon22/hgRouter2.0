#ifndef __ROUTER__
#define __ROUTER__
#include "hgTypeDef.h"
#include "hgCircuit.h"
#include "hgGrid.h"
#include "hgDRC.h"
#include "hgRtree.h"
#include "hgGeometry.h"
namespace HGR
{
    


    /* Parsing */
    void parsing();

    void write_def(); 

    /* Initialize */
    void init();

    /*   Getter functions   */
    SegRtree* get_track_db(int _layer, bool _isPref);
    BoxRtree* get_pin_db(int _layer);
    BoxRtree* get_metal_db(int _layer);
    BoxRtree* get_cut_db(int _layer);
    Net* get_net(int id);
    Pin* get_pin(int id);
    Cell* get_cell(int id);
    Layer* get_metal(int lNum);
    Layer* get_cut(int lNum);
    Layer* get_metal(string _layer);
    Layer* get_cut(string _layer);
    MacroVia* get_via(string type);
    Track* get_track(int id);
    Topology* get_topology(int id);
    Segment* get_segment(int netid, int segid);
    Segment* create_segment(int _net);
    Ext* get_extension(int _pin);

    /* Pin Extension */
    void pin_extension();
    bool pin_extension(int _pin);
    bool make_extension(int _pin, int _l1, int _l2, int _track, string _viaType, Point<int> _hp, Ext& _outExt);
    bool is_extended(int _pin);

    /* Preroute */
    void preroute();
    bool preroute_localnet_m1(int _net, int _p1, int _p2);

    bool is_local_net(int netid);


    /* Pin Access Point */
    bool pin_access_point(int _pin, int _track, Point<int> _pt1, Point<int> &_pt2, Point<int> &_pt3);
    bool pin_to_track_intersects(int _pin, int _track);
    bool pin_to_line_overlapping_intervals(int _pin, int _layer, Rect<int> _line, vector<Rect<int>> &_overlaps);
    bool pin_to_line_overlapping_boundary(int _pin, int _layer, Rect<int> _line, Point<int> &_ll, Point<int> &_ur);
    bool on_track_hit_intervals(int _pin, int _l1, int _l2, vector<Rect<int>> &_intervals);    

    /* Overlap */
    void convert_to_output_format(int _net, vector<string> &_outputs);


    /*  */
    bool fine_grained_routing();  
    bool route(int _net, int _trial=1);
    bool route_twopin_net(int _net, int _p1, int _p2, set<int> &_rGuide, set<int> &_rSpace);
    bool route_pin_to_tp(int _net, int _p, set<int> &_rGuide, set<int> &_rSpace);
    
    /* Topology Gen */
    void topology_generation(int _net);
    void subnet_segmentation(int _net, int _p1, int _p2, vector<int> &_gcells);


    int get_track_index(int _layer, Rect<int> _line);
    int get_track_index(int _layer, int _dir, Point<int> _pt);
    bool is_valid_via_type(string type);
    bool is_cut_layer(string layerName);
    bool is_metal_layer(string layerName);
    bool get_track_index(int lNum, Rect<int> line, int &trackid);
    bool get_track_index(int lNum, int dir, Point<int> _pt, int &trackid);
    bool get_tracks_on_line(int lNum, int dir, Rect<int> _line, vector<int> &_tracks);
    bool is_vertical(int lNum);
    bool is_horizontal(int lNum);
    bool is_preferred(int lNum, int direction);
    bool intersects(Point<int> _pt, Rect<int> _line);
    bool is_covered_by_pin(int pinid, Point<int> _pt);
    bool line_overlap(Rect<int> _line1, Rect<int> _line2);
    
    int line_overlap_length(Rect<int> _line1, Rect<int> _line2);    
    int nthreads();
    int direction(Point<int> pt1, Point<int> pt2);
    int direction(int x1, int y1, int x2, int y2);
    int preferred(int lNum);
    int non_preferred(int lNum);
    int die_width();
    int die_height();
    int num_layers();
    int num_tracks();
    int num_nets();
    int num_pins();
    int num_cells();
    int lef_unit_microns();
    int def_unit_microns();
    int wire_width(int lNum);
    int min_spacing(int lNum);
    int min_width(int _layer);
    int cut_spacing(int lNum);
    int min_area(int lNum);
    int min_length(int lNum);
    int run_length(int dir, Rect<int> _line1, Rect<int> _line2);
    int parallel_run_length_spacing(int lNum, int length, int width);
    int get_spacing(Rect<int> _shape1, Rect<int> _shape2, bool vertical);
    bool end_of_line_rule(int lNum, int &eolSpacing, int &eolWidth, int &eolWithin);
    bool end_of_line_rule_parallel_edge(int lNum, int &eolSpacing, int &eolWidth, int &eolWithin, int &parSpacing, int &parWithin);

    int end_of_line_spacing(int lNum);
    int end_of_line_within(int lNum);
    int distance(Point<int> _pt, Rect<int> _line);
    int manhattan_distance(Point<int> pt1, Point<int> pt2);
    int manhattan_distance(int x1, int y1, int x2, int y2);
    string benchName(); 
    bool remove(vector<int> &container, int elem);
    bool remove(vector<pair<int,int>> &container, pair<int,int> elem);
    bool exist(vector<int> &container, int elem);
    bool exist(vector<pair<int,int>> &container, pair<int,int> elem);
    bool exist(set<int> &_set, int _elem);
    int lb(vector<int> &list, int elem);
    int ub(vector<int> &list, int elem);
    Point<int> line_to_line_intersection(Rect<int> _line1, Rect<int> _line2);
    bool line_to_line_intersection(Point<int> ll1, Point<int> ur1, Point<int> ll2, Point<int> ur2, Point<int> &ptOut);
    bool track_to_track_intersection(int t1, int t2, Point<int> &_pt);
    double pin_to_pin_distance(int p1, int p2);
    
    // For parsing & initialize
    int delta_x(Point<int> orig, Rect<int> rect);
    int delta_y(Point<int> orig, Rect<int> rect);
    void get_orient(string orient, int &rotate, bool &flip);
    void shift_to_origin(int dx, int dy, Rect<int> rect1, Rect<int> &rect2);
    void flip_or_rotate(Point<int> orig, Rect<int> rect1, Rect<int> &rect2, int rotate, bool flip);

    // For boost geometry
    Point<int> center(Rect<int> _rect);
    box convert(Rect<int> rect);
    pt convert(Point<int> point);
   
    // shape.cpp
    Rect<int> get_line(Point<int> _pt1, Point<int> _pt2);
    Rect<int> wire_shape(int _layer, Rect<int> _line);
    Rect<int> wire_shape(int _layer, Point<int> _pt1, Point<int> _pt2);
    Rect<int> buffer(Rect<int> rect, int buffer_distance);
    Rect<int> buffer(Rect<int> rect, int buffer_horizontal, int buffer_vertical);
    Rect<int> convert(seg s);
    Rect<int> convert(box b);
    Point<int> convert(pt p);
    Rect<int> overlaps(Rect<int> rect1, Rect<int> rect2);
    int area(Rect<int> rect1, Rect<int> rect2);
    int area(Rect<int> rect);
    bool inside(Point<int> _pt, Rect<int> _rect);
    Rect<int> get_pin_to_pin_interval(int p1, int p2, int trackid);
    
    // Util functions
    void create_plot(int netid);
    void create_congestion_map(int lNum);

    string get_macro_via(int l1, int d1, int l2, int d2);






};



#endif

