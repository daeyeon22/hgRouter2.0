#include "hgRtree.h"
#include "hgRouter.h"

using namespace HGR;

/*
void Rtree::add_object((void*)_obj)
{
    
}
*/

void Rtree::add_metal(int _id, int _layer, Rect<int> _shape)
{
    #pragma omp critical(RTREE_METAL)
    metals[_layer].insert( { convert(_shape), _id } ); 
}

void Rtree::add_cut(int _id, int _cut, Rect<int> _shape)
{
    #pragma omp critical(RTREE_CUT)
    cuts[_cut].insert( { convert(_shape), _id } );
    
}

void Rtree::remove_metal(int _id, int _layer, Rect<int> _shape)
{
    #pragma omp critical(RTREE_METAL)
    metals[_layer].remove( { convert(_shape), _id } );

}

void Rtree::remove_cut(int _id, int _layer, Rect<int> _shape)
{
    #pragma omp critical(RTREE_CUT)
    cuts[_layer].remove( { convert(_shape), _id } );
}


void Rtree::get_tracks(int _layer, Rect<int> _area, bool _isPref, vector<int>& _tracks)
{
    SegRtree* tree = (_isPref) ? &preferred[_layer] : &nonPreferred[_layer];

    for(SegRtree::const_query_iterator it = tree->qbegin(bgi::intersects(convert(_area))); it != tree->qend(); it++)
    {
        _tracks.push_back(it->second);
    }
}


