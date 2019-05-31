#ifndef __TYPEDEF__
#define __TYPEDEF__

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <cmath>
#include <climits>
#include <cfloat>
#include <algorithm>
#include <limits>
#include <assert.h>
#include <sparsehash/dense_hash_map>
#include <omp.h>
#include <tuple>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/format.hpp>

#define INITSTR "SANGGIDO!@#!@#"

// Direction types
#define STACK 123
#define VERTICAL 111
#define HORIZONTAL 222

// Pin types
#define PI_PIN 1
#define PO_PIN 2
#define NONPIO_PIN 3
#define DUMMY_PIN -1

// Layer types
#define CUT 1212
#define ROUTING 31222

// Junction types
#define VIA_CROSS 3332
#define VIA_PRL 3112
#define CROSS 12111
#define PRL 8423
#define OFF_TRACK 8112856
//
#define METAL_TO_METAL 6767
#define PIN_TO_METAL 7676

#define DRC_METAL -122
#define DRC_CUT -688


#define CONN_PIN 125
#define CONN_WIRE 126
#define CONN_VIA 127

#define STRIPE 128
#define RECT 129

#define NODATA 1333567

//#define DEBUG
//#define CHECK_RUNTIME
#define REPORT_LOCALNET
//#define REPORT_VIA
//#define REPORT_TOPOLOGY
//#define REPORT_CONN
//#define DEBUG_SHIFT
//#define DEBUG_RIPUP
//#define REPORT_SHIFT
//#define DEBUG_EXT
#define ckt HGR::Circuit::inst()
#define grid HGR::Grid3D::inst()
#define drc HGR::DRC::inst()
#define db HGR::Rtree::inst()
#define rou HGR::Router::inst()

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using google::dense_hash_map;
using namespace std;

typedef int DRCMode;
#define DRC_VIA_DEFAULT                 30
#define DRC_VIA_CUT_DISABLE             31
#define DRC_VIA_METAL_DISABLE           32
#define DRC_VIA_METAL_BELOW_DISABLE     33
#define DRC_VIA_METAL_UPPER_DISABLE     34
#define DRC_VIA_CUT_ONLY                35
#define DRC_VIA_METAL_BELOW_ONLY        36
#define DRC_VIA_METAL_UPPER_ONLY        37

#define DRC_WIRE_DEFAULT                35
typedef tuple<int,int,int,int> via_call_type;
typedef pair<int,int> edge_t;
typedef bg::model::point<float, 2, bg::cs::cartesian> pt;
typedef bg::model::segment<pt> seg;
typedef bg::model::box<pt> box;
typedef bg::model::polygon<pt> polygon;

typedef bgi::rtree<pair<pt, int>, bgi::quadratic<16>> PointRtree;
typedef bgi::rtree<pair<seg, int>, bgi::quadratic<16>> SegRtree;
typedef bgi::rtree<pair<box, int>, bgi::quadratic<16>> BoxRtree;





#endif
