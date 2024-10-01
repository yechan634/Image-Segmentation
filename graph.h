#ifndef MAIN_H
#define MAIN_H

#include <iostream>

#include <map>
#include <tuple>
#include <queue>
#include <set>
#include <vector>
#include <array>

#define MAX_RESIDUAL_CAPACITY 999

typedef int nodeType;
typedef double weightType;
struct childInfo
{
    nodeType node;
    weightType weight;
    bool isForward;
};
// first entry is reverse edge while second is forward
struct matrixElem
{
    weightType weight;
    bool exists;
};

typedef std::vector<std::vector<std::array<matrixElem, 2>>> matrixType;

typedef std::map<nodeType, struct childInfo> childType;
// first is node, second is weight

class ResidualGraph;

// starts empty - directed - uses adjacency matrix
// assume node numbering starts from 0
class Graph
{
private:
    friend class ResidualGraph;
    // automaticlaly creates empty map
protected:
    matrixType matrix;

public:
    Graph(int initialNumNodes = 0);
    ~Graph(); // same dereference can be used for ResidualGraph
    bool addNewConnection(nodeType u, nodeType v, weightType weight, bool isForward = true);
    bool connectionExists(nodeType u, nodeType v, bool isForward = true);
    bool nodeExists(nodeType u);
    weightType getWeight(nodeType u, nodeType v, bool isForward = true);
    bool addEdgeWeightBy(nodeType u, nodeType v, weightType newWeight, bool isForward = true);
    std::set<nodeType> allReachable(nodeType start);
    ResidualGraph *createResidualGraph(); // FIX THIS
    void print();
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