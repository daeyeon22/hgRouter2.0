#include "hgRouter.h"

using namespace HGR;


bool HGR::track_assignment(int _col, int _row, int _layer, int _dir, vector<pair<int,int>> &_segs)
{
    using namespace boost::icl;

    try
    {
        IloExv env;
        stringstream logfile;
        IloModel model(env);
        IloNumVarArray cplexVars(env);
        IloExprArray cplexExprs(env);
        IloRangeArray cplexConsts(env);
        IloExpr objective(env);
        IloCplex cplex(model);

        cplex.setParam(IloCplex::TiLim, 10);
        cplex.setParam(IloCplex::Threads, 8);
        cplex.setParam(IloCplex::EpGap, 0.05);
        cplex.setParam(IloCplex::EpAGap, 0.05);
        cplex.setParam(IloCplex::SolnPoolGap, 0.1);
        cplex.setParam(IloCplex::SolnPoolAGap, 0.1);
        cplex.setOut(logfile);
        
        Panel* panel = get_panel(_col, _row, _layer);
        
        
        interval_map<int, set<int>> intervalMap;
        int numVars     = 0;
        int numSegs     = (int)_segs.size();
        int numTracks   = (int)panel->tracks.size();
        int netID[numSegs];
        int segID[numSegs];
        int xl, xh, yl, yh;
        int size;
        float **pCoeff  = new float*[numSegs];



        for(int i=0; i < numSegs; i++)
        {
            netID[i] = _segs[i].first;
            segID[i] = _segs[i].second;

            Segment* s = get_segment(netID[i], segID[i]);

            if(s->access)
            {
                vector<int> tracks;
                for(int j=0; j < (int)s->pins.size(); j++)
                {
                    rou->get_accessible_tracks(s->pins[j], tracks);
                }
                
                numVars = (int)tracks.size();
                pCoeff[i] = new float[numVars];



            }
            else
            {

            }


        }

        //= [numTracks] = { {1.0} };


        









    }
}






