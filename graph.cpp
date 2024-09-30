#include "graph.h"
#include "utilities.h"
#include <vector>
#include <algorithm>
#include <stack>

// used to interpret small floating point values as 0 from floating point errors
const double EPSILON = 1e-9;

bool Graph::nodeExists(nodeType u)
{
    return (nodesMap.find(u) != nodesMap.end());
}

// isForward is true by default
bool Graph::connectionExists(nodeType u, nodeType v, bool isForward)
{
    if (!nodeExists(u))
    {
        return false;
    }
    return std::find_if(nodesMap[u]->begin(), nodesMap[u]->end(),
                        [v, isForward](adjListMember m)
                        {
                            return (m.node == v && m.isForward == isForward);
                        }) != nodesMap[u]->end();
}

weightType Graph::getWeight(nodeType u, nodeType v, bool isForward)
{
    if (!connectionExists(u, v, isForward))
    {
        throw std::invalid_argument("no such edge exists");
    }
    auto it = std::find_if(nodesMap[u]->begin(), nodesMap[u]->end(),
                           [v, isForward](adjListMember m)
                           {
                               return (m.node == v && m.isForward == isForward);
                           });
    return it->weight;
}

bool Graph::addNewConnection(nodeType u, nodeType v, weightType weight, bool isForward)
{
    // a connection already exists
    if (connectionExists(u, v, isForward))
    {
        return false;
    }
    if (!nodeExists(u))
    {
        nodesMap[u] = new std::vector<struct adjListMember>;
    }
    nodesMap[u]->push_back({v, weight, isForward});
    if (!nodeExists(v))
    {
        nodesMap[v] = new (std::nothrow) std::vector<struct adjListMember>;
        if (nodesMap[v] == nullptr) {
            delete nodesMap[u];
            return false;
        }
    }
    return true;
}

bool Graph::addEdgeWeightBy(nodeType u, nodeType v, weightType newWeight, bool isForward)
{
    if (!connectionExists(u, v))
    {
        return false;
    }
    auto it = std::find_if(nodesMap[u]->begin(), nodesMap[u]->end(),
                           [v, isForward](adjListMember m)
                           {
                               return (m.node == v && m.isForward == isForward);
                           });
    it->weight += newWeight;
    return true;
}

// uses all edges including reversed edges
std::set<nodeType> Graph::allReachable(nodeType start)
{
    std::queue<nodeType> q;
    std::map<nodeType, nodeType> prev;
    std::set<nodeType> visited;

    q.push(start);
    while (!q.empty())
    {
        nodeType current = q.front();
        q.pop();
        visited.insert(current);
        for (auto neighbour : *nodesMap[current])
        {
            if (visited.find(neighbour.node) == visited.end() && std::abs(neighbour.weight) > EPSILON)
            {
                q.push(neighbour.node);
            }
        }
    }
    return visited;
}

void Graph::print()
{
    for (const auto &el : nodesMap)
    {
        std::cout << "\n" + std::to_string(el.first) + ", [";
        int numConnections = el.second->size(); // size() of vector will be small (at most 6)
        for (int i = 0; i < numConnections - 1; i++)
        {
            std::cout << "(" +
                             std::to_string((*el.second)[i].node) + ", " +
                             std::to_string((*el.second)[i].weight) +
                             (((*el.second)[i].isForward) ? "f" : "r") +
                             "), ";
        }
        if (numConnections > 0)
        {
            std::cout
                << "(" +
                       std::to_string((*el.second)[numConnections - 1].node) + ", " +
                       std::to_string((*el.second)[numConnections - 1].weight) +
                       (((*el.second)[numConnections - 1].isForward) ? "f" : "r") +
                       ")]";
        }
        else
        {
            std::cout << " ]";
        }
    }
    std::cout << "\n";
}

Graph::~Graph()
{
    for (auto m : nodesMap)
    {
        delete m.second;
    }
    nodesMap.clear();
}

ResidualGraph::ResidualGraph() {};

void ResidualGraph::pushFlow(childType augmentedPath, nodeType finalNode, weightType minResidualCapacity)
{
    for (nodeType current = finalNode; augmentedPath.find(current) != augmentedPath.end(); current = augmentedPath[current].node)
    {
        auto edge = augmentedPath[current];
        auto before_weight = edge.weight;
        auto fromNode = edge.node;
        auto toNode = current;
        ResidualGraph::addEdgeWeightBy(
            fromNode,
            toNode,
            -minResidualCapacity,
            edge.isForward);
        ResidualGraph::addEdgeWeightBy(
            toNode,
            fromNode,
            minResidualCapacity,
            !edge.isForward);
    }
}

std::set<nodeType> ResidualGraph::getMinCut(nodeType s, nodeType t)
{
    while (true)
    {
        childType child = getPath(s, t);
        if (child.empty())
        {
            return allReachable(s);
        }
        printPathWithWeights(child, t, this);
        // finding min residual capacity
        weightType minResidualCapacity = MAX_RESIDUAL_CAPACITY;
        for (nodeType current = t; child.find(current) != child.end(); current = child[current].node)
        {
            auto w = child[current].weight;
            if (w < minResidualCapacity)
            {
                minResidualCapacity = w;
            }
        }
        // pushing/pulling flow across augmented path
        pushFlow(child, t, minResidualCapacity);
    }
}

// returns empty set if no path is found
childType ResidualGraph::getPath(nodeType start, nodeType end)
{
    std::queue<nodeType> q;
    childType child;
    std::set<nodeType> visited;
    if (nodesMap.find(start) == nodesMap.end() || nodesMap.find(end) == nodesMap.end())
    {
        return child;
    }
    q.push(start);
    while (!q.empty())
    {
        nodeType current = q.front();
        if (current == end)
        {
            return child;
        }
        q.pop();
        visited.insert(current);

        // each forward edge has corresponding reverse edge
        for (auto neighbour : *nodesMap[current])
        {
            if (visited.find(neighbour.node) == visited.end() && std::abs(neighbour.weight) > EPSILON)
            {
                child[neighbour.node] = {current, neighbour.weight, neighbour.isForward};
                q.push(neighbour.node);
            }
        }
    }
    child.clear();
    return child;
}

ResidualGraph *Graph::createResidualGraph()
{
    auto r = new ResidualGraph(); // throws error if memory allocatino fails
    for (auto i : nodesMap)
    {
        nodeType nodeFrom = i.first;
        for (auto m : *i.second)
        {
            nodeType nodeTo = m.node;
            weightType weight = m.weight;
            r->ResidualGraph::addNewConnection(nodeFrom, nodeTo, weight, true);
            // adding corresponding reverse edge
            r->ResidualGraph::addNewConnection(nodeTo, nodeFrom, 0, false);
        }
    }
    return r;
}
