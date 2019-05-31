#ifndef __RTREE__
#define __RTREE__


#include "hgGeometry.h"
#include "hgTypeDef.h"

namespace HGR
{
    struct Rtree
    {
      private:
        static Rtree* instance;

      public:
        static Rtree* inst();
        
        vector<SegRtree> preferred;
        vector<SegRtree> nonPreferred;
        vector<BoxRtree> pins;
        vector<BoxRtree> metals;
        vector<BoxRtree> cuts;
    
        void init(int _numLayers)
        {
            preferred       = vector<SegRtree>(_numLayers);
            nonPreferred    = vector<SegRtree>(_numLayers);
            pins            = vector<BoxRtree>(_numLayers);
            metals          = vector<BoxRtree>(_numLayers);
            cuts            = vector<BoxRtree>(_numLayers);
        }

        void add_metal(int _id, int _layer, Rect<int> _shape);
        void add_cut(int _id, int _cut, Rect<int> _shape);
        void remove_metal(int _id, int _layer, Rect<int> _shape);
        void remove_cut(int _id, int _layer, Rect<int> _shape);
        void get_tracks(int _layer, Rect<int> _area, bool _isPref, vector<int> &_tracks);
    };



};
#endif
