#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>

#include <cassert>
#include <vector>
#include <utility>
#include <tuple>
#include <unordered_set>
#include <limits>
#include <functional>
#include <algorithm>

struct Node
{
    struct
    { // attributes
        int height = 0;
        double excessFlow = 0;
    };
    int id;
    double val;
    Node(int id, double val, int height)
        : id(id), val(val), height(height) {}
};

struct EdgeInfo
{
    int neighbourId;
    double capacity;
    bool isNormalEdge = true; // true if normal edge, false if residual edge
    EdgeInfo(int nId, double cap, bool isNormalEdge)
        : neighbourId(nId), capacity(cap), isNormalEdge(isNormalEdge) {}
};

namespace constants
{
    constexpr double infty = std::numeric_limits<double>::max();
    constexpr int inftyInt = std::numeric_limits<int>::max();
}

class Graph
{
public:
    Graph() {};
    ~Graph() {};

    // For testing
    bool applyCheckForEachEdgeInfo(std::function<bool(struct EdgeInfo &)> func);

    // int addNodeWithVal(double val);

    struct Node *getNode(int id);

    int addNodeWithVal(double val = 0);

    void addEdgeFromTo(int id1, int id2, double capacity);

    int getSourceId();

    int getSinkId();

    double performRelabel(int sourceId, int sinkId);

    const std::unordered_set<int> getNodesOfSource();
    const std::unordered_set<int> getNodesOfSink();

    int getNodeCount() { return m_idToNode.size(); };

private:
    std::vector<struct Node> m_idToNode;

    // tuple of neighbourId, capacity, flow
    std::vector<std::vector<struct EdgeInfo>> m_neighbours;

    int m_sourceId = -1;
    int m_sinkId = -1;

    int getNextId();

    bool isNode(int id);

    bool isEdge(int id1, int id2, bool isNormal = true);

    struct EdgeInfo *getEdgeInfo(int id1, int id2, bool isNormal);

    void setHeight(int id, int newHeight);

    double getExcessFlow(int id);

    void pushFromTo(int n1, int n2, bool isNormalEdge, std::unordered_set<int> &nodesWithExcessFlow);

    void doPushOrRelabel(int curId, std::unordered_set<int> &nodesWithExcessFlow);

    void propogateFromSource(std::unordered_set<int> &nodesWithExcessFlow);
};

#endif