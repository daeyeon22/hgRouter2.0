#include "hgRouter.h"
#include "hgGrid.h"

using namespace HGR;


int HGR::Grid3D::index(int _g, int _dx, int _dy, int _dz)
{
    Gcell* c = &gcells[_g];
    int x = c->x + _dx;
    int y = c->y + _dy;
    int z = c->z + _dz;
    return index(x, y, z);
}



int HGR::Grid3D::index(int _x, int _y, int _z)
{
    return _x + numCols * _y + numCols * numRows * _z;
}

int HGR::Grid3D::index(int _layer, Point<int> _pt, bool _bound)
{
    int x = min(lb(xOffsets, _pt.x), numCols-1);
    int y = min(lb(yOffsets, _pt.y), numRows-1);

    if(x > 0 && xOffsets[x] > _pt.x)
        x--;

    if(y > 0 && yOffsets[y] > _pt.y)
        y--;

    if(_bound)
    {
        if(yOffsets[y] == _pt.y)
            y--;
        if(xOffsets[x] == _pt.x)
            x--;
    }

    return index(x, y, _layer);
}


int HGR::Grid3D::index(int _layer, Point<int> _pt)
{
    int x = min(lb(xOffsets, _pt.x), numCols-1);
    int y = min(lb(yOffsets, _pt.y), numRows-1);

    if(x > 0 && xOffsets[x] > _pt.x)
        x--;

    if(y > 0 && yOffsets[y] > _pt.y)
        y--;

    return index(x, y, _layer);
}

int HGR::Grid3D::cap(int gcellid)
{
    return gcells[gcellid].edgeCap;
}

int HGR::Grid3D::cap(int x, int y, int z)
{
    return gcells[index(x, y, z)].edgeCap;
}

int HGR::Grid3D::direction(int g1, int g2)
{
    Gcell* gc1 = gcell(g1);
    Gcell* gc2 = gcell(g2);
    return direction(gc1->x, gc1->y, gc1->z, gc2->x, gc2->y, gc2->z);
}

int HGR::Grid3D::direction(int x1, int y1, int z1, int x2, int y2, int z2)
{
    if(abs(z1 - z2) > 0)
        return STACK;
    else
    {
        if(abs(x1 - x2) > 0)
            return HORIZONTAL;
        if(abs(y1 - y2) > 0)
            return VERTICAL;
        else
            return preferred(z1);
    }
}



double HGR::Grid3D::wirelength(int g1, int g2)
{
    double wl = 0.0;
    if(direction(g1,g2) != STACK)
    {
        if(gcells[g1].z == 0 && gcells[g2].z == 0)
        {
            wl += 2.0;
        }
        else if(gcells[g1].z == 1 && gcells[g2].z == 1)
        {
            wl += 1.5;
        }
        else
        {
            wl += 1.0;        
        }
    }

    return wl;
}

double HGR::Grid3D::segment(int g1, int g2)
{
    return 0;
    //return (direction(g1, g2) == STACK) ? 1.0 : 0.0;
}

double HGR::Grid3D::bending(int g1, int g2)
{
    return 0.0;
}

void HGR::Grid3D::gc_assign(int gcellid)
{
    #pragma omp critical(HISTORY)
    {
        assign[gcellid]++;
    }
}

void HGR::Grid3D::gc_drain(int gcellid)
{
    #pragma omp critical(HISTORY)
    {
        assign[gcellid]--;
    }   
}
void HGR::Grid3D::gc_drain(vector<int> &_gcells)
{
    #pragma omp critical(HISTORY)
    {
        for(auto gcellid : _gcells)
            assign[gcellid]--;
    }
}


void HGR::Grid3D::add_history(int gcellid)
{
    #pragma omp critical(HISTORY)
    {
        history[gcellid]++;
    } 
}

bool HGR::Grid3D::is_congested(int gcellid)
{
    bool isCongested = false;
    #pragma omp critical(HISTORY)
    {
        int edgeCap = gcells[gcellid].edgeCap;
        int edgeAss = assign[gcellid];
        isCongested = edgeCap - edgeAss < 0;
    }
    return isCongested;
}



double HGR::Grid3D::overflow(int gcellid)
{
    double cost = gcells[gcellid].weight;
    #pragma omp critical(HISTORY)
    {
        int edgeCap = gcells[gcellid].edgeCap;
        int edgeAss = assign[gcellid];
        cost += 10.0 * max(0.0, 1.0*edgeAss - 0.5*edgeCap) + 100.0 * max(0, edgeAss - edgeCap) + 100.0 * history[gcellid];
    }
    return cost;
}


double HGR::Grid3D::offguide(int gcellid, set<int>& routing_guide)
{
    return (routing_guide.find(gcellid) == routing_guide.end()) ? 0.5 : 0.0;
}

double HGR::Grid3D::npref(int g1, int g2)
{
    int dir = direction(g1, g2);
    if(dir == STACK)
        return 0.0;
    else
    {
        return (preferred(gcells[g1].z) == dir)? 0.0 : 10000.0;
    }
}

double HGR::Grid3D::distance(int _g1, int _g2)
{
    Gcell* c1 = &gcells[_g1];
    Gcell* c2 = &gcells[_g2];

    int dx = abs(c1->x - c2->x);
    int dy = abs(c1->y - c2->y);
    int dz = abs(c1->z - c2->z);

    return 1.0 * ( dx + dy + dz );
}



double HGR::Grid3D::estimation(int g1, vector<int> &multi_target)
{
    return 0.0;
}


bool HGR::Grid3D::intersects(int _layer, Rect<int> _rect, set<int> &_results)
{

    if(_layer >= numLayers || _layer < 0)
        return false;

    int g1 = index(_layer, _rect.ll, false);
    int g2 = index(_layer, _rect.ur, true);
    int x1, y1, x2, y2, z1, z2;
    coord(g1, x1, y1, z1);
    coord(g2, x2, y2, z2);

    for(int x = x1; x <= x2; x++)
    {
        for(int y = y1; y <= y2; y++)
        {
            _results.emplace(index(x, y, _layer));
        }
    }

    return _results.size() > 0 ? true : false;
}

/*
bool HGR::Grid3D::intersects(int _lower, int _upper, Rect<int> _rect, set<int> &_results)
{

    for(int layer = _lower; layer <= _upper; layer++)
    {
        if(layer >= numLayers || layer < 0)
            return false;

        int g1 = index(layer, _rect.ll, false);
        int g2 = index(layer, _rect.ur, true);
        int x1, y1, x2, y2, z1, z2;
        coord(g1, x1, y1, z1);
        coord(g2, x2, y2, z2);

        for(int x = x1; x <= x2; x++)
        {
            for(int y = y1; y <= y2; y++)
            {
                _results.emplace(index(x, y, layer));
            }
        }
    }

    return _results.size() > 0 ? true : false;
}
*/

void HGR::Grid3D::coord(int gcellid, int& x, int& y, int& z)
{
    x = gcellid % numCols;
    y = (gcellid / numCols) % numRows;
    z = gcellid / (numCols * numRows);
}

Panel* HGR::Grid3D::panel(int lNum, int dir, int index)
{
    return (dir == VERTICAL) ? &cols[lNum][index] : &rows[lNum][index]; 
}

Gcell* HGR::Grid3D::gcell(int _id)
{
    return &gcells[_id];
}

Gcell* HGR::Grid3D::gcell(int _x, int _y, int _z)
{
    return &gcells[index(_x,_y,_z)];
}

Gcell* HGR::Grid3D::gcell(int _layer, Point<int> _pt)
{
    return &gcells[index(_layer, _pt)];
}


bool HGR::Grid3D::out_of_index(int _x, int _y, int _z)
{
    if(_x < 0 || _x >= numCols)
        return true;
    if(_y < 0 || _y >= numRows)
        return true;
    if(_z < 0 || _z >= numLayers)
        return true;

    return false;

}

bool HGR::Grid3D::out_of_index(int _g)
{
    int x,y,z;
    coord(_g,x,y,z);
    return out_of_index(x,y,z);
}

