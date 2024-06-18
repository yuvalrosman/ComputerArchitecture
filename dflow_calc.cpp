/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include "iostream"
using std::cerr;
using std::endl;

class graphNode //describes each node in the graph, including the entry node -  each one has its info of the command and pointers to his dependencies
{
public:
    graphNode();
    graphNode(graphNode* next1, graphNode* next2);
    ~graphNode();
    InstInfo* info;
    graphNode* next1;
    graphNode* next2;
    unsigned int latency;
    unsigned int index;
    unsigned int depth;

};


graphNode::graphNode(graphNode* next1, graphNode* next2):info(new InstInfo),next1(next1),next2(next2),latency(0),index(0),depth(0) {}
graphNode::graphNode():info(new InstInfo),next1(nullptr),next2(nullptr),latency(0),index(0), depth(0) {

}

graphNode::~graphNode() {

    delete info;
}




class Graph{ // our datastructure , holds all nodes,  and entry nodes as well
public:
    explicit Graph(unsigned int size);
    ~Graph();
    unsigned int size; // the amount o nodes in the DS == amount of commands in the trace
    graphNode* entry;
    graphNode** all_nodes;
    int longest_path;

};

Graph::Graph(unsigned int size):size(size) {
    entry = new graphNode();
    all_nodes = new graphNode*[size];
    for(unsigned int i=0;i<size;i++)
    {
        all_nodes[i] = new graphNode(this->entry,this->entry); // as a default a node depends on entry
    }
    longest_path=0;
}

Graph::~Graph() {
    delete entry;
    for(unsigned int  i=0;i<size;i++)
    {
        delete all_nodes[i];
    }
    delete[] all_nodes;
}
int maximum(int a , int b);
int maximum(int a , int b)
{
    if(a>=b ) return a;
    return b;
}

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    Graph* dep_graph = new Graph(numOfInsts);
    for(unsigned int i=0;i<numOfInsts;i++)// iterating the command, and assigning the relevant data to each node
    {
        dep_graph->all_nodes[i]->latency = opsLatency[progTrace[i].opcode];
        dep_graph->all_nodes[i]->index =i;
        *(dep_graph->all_nodes[i]->info) = progTrace[i];
        if(i==0)
        {
            continue;
        }
        for(int j=i-1;j>=0;j--) // for each commands, iterate backwards on previous commands and assign dependencies if needed
        {
            if(dep_graph->all_nodes[i]->next1 == dep_graph->entry)
            {

                if(unsigned(progTrace[j].dstIdx) == progTrace[i].src1Idx)
                {
                    dep_graph->all_nodes[i]->next1 = dep_graph->all_nodes[j];

                }
            }
            if(dep_graph->all_nodes[i]->next2 == dep_graph->entry)
            {
                    if(unsigned(progTrace[j].dstIdx) == progTrace[i].src2Idx)
                    {
                        dep_graph->all_nodes[i]->next2 = dep_graph->all_nodes[j];

                    }
            }
        }
        int max1=0,max2=0;
        if(dep_graph->all_nodes[i]->next1 != dep_graph->entry)
        {
            max1= dep_graph->all_nodes[i]->next1->depth + dep_graph->all_nodes[i]->next1->latency;
        }
        if(dep_graph->all_nodes[i]->next2 != dep_graph->entry)
        {
            max2= dep_graph->all_nodes[i]->next2->depth + dep_graph->all_nodes[i]->next2->latency;
        }
        dep_graph->all_nodes[i]->depth = maximum(max1,max2);
        dep_graph->longest_path = maximum(dep_graph->longest_path, dep_graph->all_nodes[i]->depth + dep_graph->all_nodes[i]->latency);
    }
    return dep_graph;

}

void freeProgCtx(ProgCtx ctx) { // casting to our DS and calling its D'Tor
    Graph* our_graph = (Graph*)ctx;
    delete our_graph;

}



int getInstDepthAux(Graph* graph, graphNode* node) { // recursive function to calculate the depth of a given node.
    if(node == graph->entry || node == nullptr || node->next1 == nullptr || node->next2 == nullptr) return 0;
    int max = maximum(getInstDepthAux(graph,node->next1)+node->next1->latency,getInstDepthAux(graph,node->next2)+node->next2->latency);
    return  max;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) { //using the aux recursive function on the watned command
    Graph* our_graph = (Graph*)ctx;
    if(theInst<0 || theInst>=our_graph->size) return -1; // check validity of theInst
    return our_graph->all_nodes[theInst]->depth;
    //return getInstDepthAux(our_graph,our_graph->all_nodes[theInst]);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) { //given a wanted command, assign to the pointers given, the command dependencies index in the trace file
    Graph* our_graph = (Graph*)ctx;
    if(theInst<0 || theInst>=our_graph->size) return -1;
    graphNode* node = our_graph->all_nodes[theInst];
    if(node->next1 != our_graph->entry)
    {
        *src1DepInst = node->next1->index;
    }
    else
    {
        *src1DepInst = -1;
    }
    if(node->next2 != our_graph->entry)
    {
        *src2DepInst = node->next2->index;
    }
    else
    {
        *src2DepInst = -1;
    }
    return 0;
}

int getProgDepth(ProgCtx ctx) { // using our recursive function, find the max between all of the pointers in exits array, the max progDepth will always be from one of exits pointers
    Graph* our_graph = (Graph*)ctx;
    return our_graph->longest_path;
}


