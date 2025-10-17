#include "graph.h"
#include "cassert"

#include <queue>
#include <math.h>

bool Graph::applyCheckForEachEdgeInfo(std::function<bool(struct EdgeInfo &)> func)
{
    for (auto &edges : m_neighbours)
    {
        for (auto &edge : edges)
        {
            if (!func(edge))
            {
                return false;
            }
        }
    }
    return true;
}

int Graph::getNextId()
{
    return m_idToNode.size();
}

void Graph::setHeight(int id, int newHeight)
{
    m_idToNode[id].height = newHeight;
}

double Graph::getExcessFlow(int id)
{
    return getNode(id)->excessFlow;
}

struct Node *Graph::getNode(int id)
{
    return &m_idToNode[id];
}

// void Graph::addConnection(int id1, int id2, double capacity)
// {
//     if (!isNode(id1)) {
//         addNodeWithVal(capacity);
//     }   
//     if (!isNode(id2)) {
//         addNodeWithVal(0);
//     }
//     addEdgeFromTo(id1, id2, capacity);
// }

int Graph::addNodeWithVal(double val)
{
    int id = getNextId();
    m_idToNode.emplace_back(id, val, 0);
    m_neighbours.emplace_back();
    return id;
}

bool Graph::isNode(int id) {
    return id < m_idToNode.size();
}

/* Assumes nodes have already been created */
void Graph::addEdgeFromTo(int id1, int id2, double capacity)
{
    assert(std::max(id1, id2) < m_idToNode.size());
    assert(!isEdge(id1, id2, true));
    m_neighbours[id1].emplace_back(id2, capacity, true);

    // creating residual
    m_neighbours[id2].emplace_back(id1, 0, false);
}

bool Graph::isEdge(int id1, int id2, bool isNormal)
{
    return (
        std::max(id1, id2) < m_idToNode.size() &&
        getEdgeInfo(id1, id2, isNormal) != nullptr);
}

int Graph::getSourceId()
{
    return m_sourceId;
}

int Graph::getSinkId()
{
    return m_sinkId;
}

struct EdgeInfo *Graph::getEdgeInfo(int id1, int id2, bool isNormal)
{
    for (EdgeInfo &edge : m_neighbours[id1])
    {
        if (edge.neighbourId == id2 && edge.isNormalEdge == isNormal)
        {
            // std::cout << "Found edge from " << id1 << " to " << edge.neighbourId << " isNormal: " << edge.isNormalEdge << " with capacity " << edge.capacity << std::endl;
            // std::cout << "Looking for edge from " << id1 << " to " << id2 << " isNormal: " << isNormal << std::endl;
            return &edge;
        }
    }
    return nullptr;
}

void Graph::pushFromTo(int n1, int n2, bool isNormalEdge, std::unordered_set<int> &nodesWithExcessFlow)
{
    assert(getExcessFlow(n1) > 0);
    assert(isEdge(n1, n2, isNormalEdge));

    double flowToPush = std::min(getExcessFlow(n1), getEdgeInfo(n1, n2, isNormalEdge)->capacity);

    getEdgeInfo(n1, n2, isNormalEdge)->capacity -= flowToPush;
    getEdgeInfo(n2, n1, !isNormalEdge)->capacity += flowToPush;

    getNode(n1)->excessFlow -= flowToPush;
    getNode(n2)->excessFlow += flowToPush;

    assert(getExcessFlow(n1) >= 0);
    if (getExcessFlow(n1) == 0)
    {
        nodesWithExcessFlow.erase(n1);
    }

    if (getExcessFlow(n2) > 0 && n2 != m_sinkId && n2 != m_sourceId)
    {
        nodesWithExcessFlow.insert(n2);
    }
}

void Graph::doPushOrRelabel(int curId, std::unordered_set<int> &nodesWithExcessFlow)
{
    assert(curId != m_sourceId && curId != m_sinkId);
    bool pushed = false;
    int minHeightOfNeighbours = constants::inftyInt;

    for (EdgeInfo &edge : m_neighbours[curId])
    {
        int neighbourId = edge.neighbourId;
        if (getNode(curId)->height > getNode(neighbourId)->height &&
            edge.capacity > 0)
        {
            // Push the neighbour into the nodesWithExcessFlow set
            pushFromTo(curId, neighbourId, edge.isNormalEdge, nodesWithExcessFlow);
            pushed = true;
            break;
        }
        if (edge.capacity > 0)
        {
            assert(getNode(neighbourId)->height != constants::inftyInt);
            minHeightOfNeighbours = std::min(minHeightOfNeighbours, getNode(neighbourId)->height);
        }
    }
    if (!pushed)
    {
        // If no push happened, relabel current node
        assert(minHeightOfNeighbours != constants::inftyInt);
        int newHeight = minHeightOfNeighbours + 1;
        setHeight(curId, newHeight);
        // Try again
        doPushOrRelabel(curId, nodesWithExcessFlow);
    }
}

/* Pushes flow from source to all its neighbours, and adds them to nodesWithExcessFlow */
void Graph::propogateFromSource(std::unordered_set<int> &nodesWithExcessFlow)
{
    for (auto &edge : m_neighbours[getSourceId()])
    {
        double pushingFlow = edge.capacity;
        if (edge.neighbourId == m_sinkId || edge.capacity <= 0)
        {
            continue;
        }
        edge.capacity -= pushingFlow;
        getEdgeInfo(edge.neighbourId, m_sourceId, !edge.isNormalEdge)->capacity += pushingFlow;

        getNode(m_sourceId)->excessFlow -= pushingFlow;
        getNode(edge.neighbourId)->excessFlow += pushingFlow;

        nodesWithExcessFlow.insert(edge.neighbourId);
    }
}

/* Network flow algorithm, O(V^2E) */
double Graph::performRelabel(int sourceId, int sinkId)
{

    assert(getSourceId() == -1);
    assert(getSinkId() == -1);

    std::unordered_set<int> nodesWithExcessFlow;

    this->m_sourceId = sourceId;
    this->m_sinkId = sinkId;

    // assume excess of all are 0 at this point

    propogateFromSource(nodesWithExcessFlow);

    setHeight(getSourceId(), m_idToNode.size());

    while (!nodesWithExcessFlow.empty())
    {

        int curId = *nodesWithExcessFlow.begin();
        assert(getExcessFlow(curId) > 0);

        doPushOrRelabel(curId, nodesWithExcessFlow);
    }

    return getNode(this->m_sinkId)->excessFlow;
}
/* Returns all nodes that are reachable from the source using BFS,
   excludes source node */
const std::unordered_set<int> Graph::getNodesOfSource()
{
    std::queue<int> toVisit;
    std::unordered_set<int> visited;

    toVisit.push(m_sourceId);

    while (!toVisit.empty())
    {
        auto curId = toVisit.front();
        toVisit.pop();

        if (visited.count(curId) == 0)
        {
            visited.insert(curId);

            for (const auto &edge : m_neighbours[curId])
            {
                auto neighbourId = edge.neighbourId;
                if (edge.capacity > 0 && visited.count(neighbourId) == 0)
                {
                    toVisit.push(neighbourId);
                }
            }
        }
    }

    visited.erase(m_sourceId);

    return visited;
}

/* Returns all nodes not reachable from the source, excludes sink node. Not
   equivalent to finding all nodes reachable from the sink */
const std::unordered_set<int> Graph::getNodesOfSink()
{
    std::unordered_set<int> sourceNodes = getNodesOfSource();

    std::unordered_set<int> sinkNodes;
    for (int i = 0; i < m_idToNode.size(); i++)
    {
        if (i != m_sourceId && i != m_sinkId && (sourceNodes.count(i) == 0))
        {
            sinkNodes.insert(i);
        }
    }

    sinkNodes.erase(m_sinkId);
    return sinkNodes;
}