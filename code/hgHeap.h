#ifndef __HEAP_H__
#define __HEAP_H__

#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>


namespace HGR
{
    struct HeapNode
    {
        int id;
        int depth;
        int backtrace;
        double est;
        double act;

        HeapNode() :
            id(INT_MAX), depth(-1), backtrace(-1), act(DBL_MAX), est(DBL_MAX) {}

        HeapNode(const HeapNode& _node) :
            id(_node.id), depth(_node.depth), backtrace(_node.backtrace), est(_node.est), act(_node.act) {}

       
        void init(int _id, bool _isSource=false);
        void update(int _depth, int _backtrace, double _act, double _est);

        double cost() const { return (depth == -1) ? DBL_MAX : act + est; } //(depth == INT_MAX) ? DBL_MAX : wl + ns + og + of + bp + np + pc; }
        double act_cost() const { return act; } //(depth == INT_MAX) ? DBL_MAX : wl + ns + og + of + bp + np + pc; }
        double est_cost() const { return est; } //(depth == INT_MAX) ? DBL_MAX : est; } 
        double get_act_cost(int _prevNode, double _prevActCost, set<int> &_rGuide);
        double get_est_cost(int _targetNode);
        double get_est_cost(vector<int> &_targetNodes);
        
        bool operator == (const HeapNode& n) const { return id == n.id; }
        bool operator < (const HeapNode& n) const { return act_cost() < n.act_cost(); }
        bool operator > (const HeapNode& n) const { return act_cost() > n.act_cost(); }
    };

    template <class A>
    struct Heap
    {
      private:
        template <class B>
        struct Comp
        {
            bool operator () (B& n1, B& n2){ return n1.act_cost() + n1.est_cost() > n2.act_cost() + n2.est_cost(); }
        };
        
        priority_queue<A, vector<A>, Comp<A>> PQ;
        vector<A> container;
        dense_hash_map<int, A> nodes;

      public:
        /* helper function */
        //void push(vector<A> &next){ for(auto& elem : next) push(elem); }
        
        void push(A elem)
        { 
            PQ.push(elem);
            //typename vector<A>::iterator it = find(container.begin(), container.end(), elem);
            //if(it == container.end()) container.push_back(elem);
            //else container[it - container.begin()] = elem;
        }
        
        A pop()
        {
            A elem = PQ.top();
            PQ.pop();
            return elem;
            //PQ.top();
            /*
             * auto cmp = [](const A& v1, const A& v2){
                return v1.act_cost() + v1.est_cost() > v2.act_cost() + v2.est_cost();
            };
            make_heap(container.begin(), container.end(), cmp); 
            pop_heap(container.begin(), container.end(), cmp);
            A elem = container.back();
            container.pop_back();
            return elem;
            */
        }

        /*
        bool remove(A elem)
        {
            typename vector<A>::iterator it = find(container.begin(), container.end(), elem);
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

        bool exist(A elem)
        {
            typename vector<A>::iterator it = find(container.begin(), container.end(), elem);
            return (it != container.end()) ? true : false;
        }
        */
        bool empty(){ return PQ.empty(); }
        size_t size(){ return PQ.size(); } 
    
    };
};

#endif
