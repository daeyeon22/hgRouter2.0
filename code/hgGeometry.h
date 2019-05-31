#ifndef __GEOMETRY__
#define __GEOMETRY__
#include <iostream>
#include <ostream>
#include <vector>
#include <string>
#include <map>
#include "hgTypeDef.h"

using namespace std;

namespace HGR
{
    template <typename A> 
    struct Point
    {
        A x, y;
     
        Point(){}
        Point(const Point<A> & pt) : x(pt.x), y(pt.y) {}
        Point(A x, A y) : x(x), y(y) {}
    };
    
    template <typename A>
    struct Rect 
    {
        Point<A> ll, ur;

        Rect()
        {
            ll = Point<A>();
            ur = Point<A>();
        }
        Rect(const Rect<A> &rt) : ll(rt.ll), ur(rt.ur) {}
        Rect(const Point<A> &ll, const Point<A> &ur) : ll(ll), ur(ur) {}
        Rect(int x1, int y1, int x2, int y2)
        {
            ll = Point<int>(x1, y1), ur = Point<int>(x2, y2);
        }
    };

    struct Wire
    {
        int id;             // Wire id
        int type;           // STRIPE | RECT | 
        int net;
        int layer;          
        int track;          // Track or Off-track
        int conn1, conn2;   // Connection Type (1 : lowerLeft, 2 : upperRight)
        int width;
        int height;
        Rect<int> line;
        Rect<int> metalShape;
        //rect;
    
        void update_database();
        void remove_database();
    };

    struct Via
    {
        int id;
        int net;
        int cut;
        int lower, upper;
        string      viaType;
        Point<int>  origin;
        Rect<int>   cutShape;
        Rect<int>   upperMetalShape;
        Rect<int>   lowerMetalShape;
        
        void update_database();
        void remove_database();
    };
    
    struct Patch
    {
        Point<int> origin;  
        Rect<int> rect;     // relative
    };

    struct Ext
    {
        int         pin;        // Pin
        int         layer;      // Accessible Layer
        int         track;      // Accessible Track
        Point<int>  access;     // Access Point
        Point<int>  via;        // Via Position
        bool        useVia;     // If Via used, true
        
        // Design Rule Violation Penalty
        double      penalty;

        // Required Paths
        vector<Wire>        wires;
        vector<Via>         vias;
        vector<bool>        wireConn;
        vector<bool>        viaConn;

        Ext() : pin(0), layer(0), track(0), penalty(0.0), useVia(true) {}
        Ext(const Ext& _ext) :
            pin(_ext.pin), layer(_ext.layer), track(_ext.track), access(_ext.access), via(_ext.via), useVia(_ext.useVia),
            penalty(_ext.penalty), wires(_ext.wires), vias(_ext.vias), wireConn(_ext.wireConn), viaConn(_ext.viaConn) {}

        double update_penalty(int _net);
        void update_database();
        void remove_database();
        void set_id(int _id);
    };


    struct Segment
    {
        int id;
        int g1, g2;
        int track;
        int layer;
        int direction;
        int net;

        bool assign;
        bool access;
        
        //Via via;
        Wire wire;
        vector<Patch>   patches;
        vector<int>     adjs;

        // Pin Extension
        vector<int>     pins;
        vector<Ext>     exts;

        Segment() : id(-1), g1(-1), g2(-1), track(-1), layer(-1), assign(false), access(false) {}
        Segment(const Segment& _s) :
            id(_s.id), g1(_s.g1), g2(_s.g2), track(_s.track), layer(_s.layer), direction(_s.direction), 
            net(_s.net), assign(_s.assign), access(_s.access), wire(_s.wire), patches(_s.patches), adjs(_s.adjs),
            pins(_s.pins), exts(_s.exts) {}

        void update_database();
        void remove_database();
            
    };
    
    struct Edge
    {
        int s1;
        int s2;
        int type;           // CROSS | VIA_CROSS | PRL | VIA_PRL
        int layer;
        bool isVia;

        Via via;

        Edge() : s1(0), s2(0), type(0), layer(0), isVia(false) {}
        Edge(const Edge& _e) : s1(_e.s1), s2(_e.s2), type(_e.type), layer(_e.layer), isVia(_e.isVia), via(_e.via) {} 
        void update_database();
        void remove_database();
    };

    struct Topology
    {
        
        int net;
        bool routed;
        
        //
        vector<int>         gcells; 
        vector<Segment>     segments;
        vector<SegRtree>    trees;

        
        // Indexing
        
        // Edges
        vector<edge_t> edges;
        map<edge_t, Edge> edge;



        Topology()
        {
            net = INT_MAX;
            routed = false;
            gcells.clear();
            segments.clear();
            trees.clear();
        }

        Topology(const Topology& _tp)
        {
            net = _tp.net;
            routed = _tp.routed;
            gcells = _tp.gcells;
            segments = _tp.segments;
            trees = _tp.trees;
            edge = _tp.edge;
        }


        void add_edge(int _s1, int _s2, int _type = VIA_CROSS);
        void remove_edge(int _s1, int _s2);
        //
        void set_edge_type(int _s1, int _s2, int _type);
        void set_edge_via(int _s1, int _s2, Via _via);
        //
        int get_edge_type(int _s1, int _s2);
        Via get_edge_via(int _s1, int _s2);
        bool parallel_edge_exist();
        bool check_parallel_edge_connectivity(int _layer);
        void clear();
    };



    class Router
    {
      private:
        static Router* instance;

      public:
        static Router* inst();

        vector<Topology>            topologies;
        
        dense_hash_map<int, vector<Ext>>    pinAccessPool;
        dense_hash_map<int, Ext>            pinAccessExt;
   

        Router()
        {
            pinAccessExt.set_empty_key(INT_MAX);
            pinAccessPool.set_empty_key(INT_MAX);
        }

        void init();

        void set_extension(int _pin, Ext *_ext){ 
            pinAccessExt = *_ext;
        }

        void add_extension(int _pin, Ext *_ext)
        {
            pinAccessPool[_pin].push_back(*_ext);
        }

        void get_accessible_tracks(int _pin, vector<int> &_tracks);

        //void build_graph_from_guide(int _net);
        //Path route(vector<int> _multiSource, vector<int> _multiTarget);
    };
};

inline ostream& operator << (ostream& _os, HGR::Rect<int> _rect)
{
    return _os << "(" << _rect.ll.x << " " << _rect.ll.y << ") (" << _rect.ur.x << " " << _rect.ur.y << ")";
}




#endif
