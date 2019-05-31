#ifndef __GRID__
#define __GRID__
#include "hgGeometry.h"

namespace HGR
{


    struct Gcell
    {
        int id;
        int x, y, z;        // x : col, y : row, z : layer
        int direction;      // preferred direction
        int edgeCap;        // edge capacitance
        double weight;
        Rect<int> rect;     // obsolute coordinate
    };

    struct Panel
    {
        int col, row, layer;
        bool isPref;
        vector<int> tracks; // container of track id 
        Rect<int> rect;     // absolute coordinate

        Panel() : col(0), row(0), layer(0), isPref(true), rect(Rect<int>()) {}
        Panel(const Panel& _p) : col(_p.col), row(_p.row), layer(_p.layer), isPref(_p.isPref), rect(_p.rect), tracks(_p.tracks) {}
        Panel(int col, int row, int layer, bool isPref, Rect<int> rect) :
            col(col), row(row), layer(layer), isPref(isPref), rect(rect) {}

    };

    struct Grid3D
    {
      private:
        static Grid3D* instance;
        //= nullptr;
    
      public:
        static Grid3D* inst();

        Rect<int> area;
        int numCols, numRows, numLayers;
        int GCwidth, GCheight;

        vector<int> xOffsets;
        vector<int> yOffsets;

        vector<int> assign;
        vector<int> history;
        vector<Gcell> gcells;
        vector<vector<Panel>> cols;
        vector<vector<Panel>> rows;
        //vector<BoxRtree> trees;
        //Gcell* operator [] (int gcellid){ return &gcells[gcellid]; }
       
        void init(Rect<int> _area, int _numCols, int _numRows, int _numLayers, int _GCwidth, int _GCheight);

        int index(int _g, int _dx, int _dy, int _dz);
        int index(int _x, int _y, int _z);
        int index(int _layer, Point<int> _pt);
        int index(int _layer, Point<int> _pt, bool bounded);
        int cap(int gcellid);
        int cap(int x, int y, int z);
        int direction(int g1, int g2);
        int direction(int x1, int y1, int z1, int x2, int y2, int z2);
        
        double distance(int _g1, int _g2);
        double wirelength(int g1, int g2);
        double segment(int g1, int g2);
        double bending(int g1, int g2);
        double overflow(int gcellid);
        double offguide(int gcellid, set<int>& routing_guide);
        double npref(int g1, int g2);
        double estimation(int g1, vector<int> &multi_target);
        bool intersects(int lNum, Rect<int> rect, set<int> &results);
        bool is_congested(int gcellid);
        void coord(int gcellid, int& x, int& y, int& z);
        void gc_assign(int gcellid);
        void gc_drain(int gcellid);
        void gc_drain(vector<int> &_gcells);
        void add_history(int gcellid);
        bool is_overflow(int _g);
        bool out_of_index(int _x, int _y, int _z);
        bool out_of_index(int _g);

        Panel* panel(int _layer, int _dir, int _i);
        Gcell* gcell(int _i);
        Gcell* gcell(int _x, int _y, int _z);
        Gcell* gcell(int _layer, Point<int> _pt);
    
    };

    /*
    inline ostream& operator << (ostream& _os, Gcell* _c)
    {
        return _os << "Gcell " << _c->id << " (" << _c->x << " " << _c->y << " " << _c->z << ")";
    }
    */

};

#endif
