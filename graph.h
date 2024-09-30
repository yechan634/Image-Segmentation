#ifndef MAIN_H
#define MAIN_H

#include <iostream>

#include <map>
#include <tuple>
#include <queue>
#include <set>
#include <vector>

#define MAX_RESIDUAL_CAPACITY 999

typedef int nodeType;
typedef double weightType;
struct adjListMember
{
    nodeType node;
    weightType weight;
    bool isForward;
};
typedef std::map<nodeType, std::vector<struct adjListMember> *> nodesMapType;

typedef std::map<nodeType, struct adjListMember> childType;
// first is node, second is weight

class ResidualGraph;

// starts empty - directed - uses adjacency List
class Graph
{
private:
    friend class ResidualGraph;
    // automaticlaly creates empty map
protected:
    nodesMapType nodesMap;

public:
    bool addNewConnection(nodeType u, nodeType v, weightType weight, bool isForward = true);
    bool connectionExists(nodeType u, nodeType v, bool isForward = true);
    bool nodeExists(nodeType u);
    weightType getWeight(nodeType u, nodeType v, bool isForward = true);
    bool addEdgeWeightBy(nodeType u, nodeType v, weightType newWeight, bool isForward = true);
    std::set<nodeType> allReachable(nodeType start);
    ResidualGraph *createResidualGraph(); // FIX THIS
    void print();
    ~Graph(); // same dereference can be used for ResidualGraph
};

// ResidualGraph class declaration
class ResidualGraph : public Graph
{
private:
    ResidualGraph();
    friend Graph;

public:
    void pushFlow(childType augmentedPath, nodeType finalNode, weightType minResidualCapacity); // MAKE PRIVATE LATER
    childType getPath(nodeType start, nodeType end);
    std::set<nodeType> getMinCut(nodeType s, nodeType t);
};

#endif