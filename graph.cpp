#include "graph.h"
#include "utilities.h"
#include <vector>
#include <algorithm>
#include <stack>

// used to interpret small floating point values as 0 from floating point errors
const double EPSILON = 1e-9;

Graph::Graph(int initialNodesNum)
{
    std::vector<std::array<matrixElem, 2>>
        row(initialNodesNum, std::array<matrixElem, 2>{{{0.0, false}, {0.0, false}}});
    matrixType _matrix(initialNodesNum, row);
    matrix = _matrix;
}

bool Graph::nodeExists(nodeType u)
{
    if (u < 0)
    {
        return false;
    }
    return (u < matrix.size());
}

// isForward is true by default
bool Graph::connectionExists(nodeType u, nodeType v, bool isForward)
{
    if (!nodeExists(u) || !nodeExists(v))
    {
        return false;
    }
    return matrix.at(u).at(v)[isForward].exists;
}

weightType Graph::getWeight(nodeType u, nodeType v, bool isForward)
{
    if (!connectionExists(u, v, isForward))
    {
        throw std::invalid_argument("no such edge exists");
    }
    return matrix.at(u).at(v)[isForward].weight;
}

bool Graph::addNewConnection(nodeType u, nodeType v, weightType weight, bool isForward)
{
    // a connection already exists
    if (connectionExists(u, v, isForward))
    {
        return false;
    }
    nodeType largerNode = std::max(u, v);

    if (!nodeExists(largerNode))
    {
        for (auto &row : matrix)
        {
            row.resize(largerNode + 1, std::array<matrixElem, 2>{{{0.0, false}, {0.0, false}}});
        }
        // Ensure the graph has enough space for the new node
        while (matrix.size() <= largerNode)
        {
            // Add a new row for the new node
            std::vector<std::array<matrixElem, 2>> newRow(largerNode + 1, std::array<matrixElem, 2>{{{0.0, false}, {0.0, false}}});
            matrix.push_back(newRow);
        }
    }

    matrix[u][v][isForward].weight = weight;
    matrix[u][v][isForward].exists = true;

    return true; // Successfully added the connection
}

bool Graph::addEdgeWeightBy(nodeType u, nodeType v, weightType newWeight, bool isForward)
{
    if (!connectionExists(u, v, isForward))
    {
        return false;
    }
    matrix[u][v][isForward].weight += newWeight;
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
        // for (auto neighbour : matrix.at(current))
        for (int otherNode = 0; otherNode < matrix.size(); otherNode++)
        {
            if (
                connectionExists(current, otherNode, true) &&
                    visited.find(otherNode) == visited.end() &&
                    getWeight(current, otherNode, true) > EPSILON ||
                connectionExists(current, otherNode, false) &&
                    visited.find(otherNode) == visited.end() &&
                    getWeight(current, otherNode, false) > EPSILON)
            {
                q.push(otherNode);
            }
        }
    }
    return visited;
}

void Graph::print()
{

    for (int u = 0; u < matrix.size(); u++)
    {
        std::cout << "\n" + std::to_string(u) + ", [";
        /*
        for (int i = 0; i < numConnections - 1; i++)
        {
            std::cout << "(" +
                             std::to_string((*el.second)[i].node) + ", " +
                             std::to_string((*el.second)[i].weight) +
                             (((*el.second)[i].isForward) ? "f" : "r") +
                             "), ";
        }
        */
        for (int v = 0; v < matrix.size(); v++)
        {
            if (connectionExists(u, v, true))
            {
                std::cout << "(" +
                                 std::to_string(v) + ", " +
                                 std::to_string(getWeight(u, v, true)) + ", f), ";
            }
            if (connectionExists(u, v, false))
            {
                std::cout << "(" +
                                 std::to_string(v) + ", " +
                                 std::to_string(getWeight(u, v, false)) + ", r), ";
            }
        }
        std::cout << " ]";
    }
    std::cout << "\n";
}

Graph::~Graph()
{
}

ResidualGraph::ResidualGraph() {};

void ResidualGraph::pushFlow(childType augmentedPath, nodeType finalNode, weightType minResidualCapacity)
{
    for (nodeType current = finalNode; augmentedPath.find(current) != augmentedPath.end(); current = augmentedPath[current].node)
    {
        auto edge = augmentedPath[current];
        // auto before_weight = edge.weight;
        auto fromNode = edge.node;
        auto toNode = current;
        addEdgeWeightBy(
            fromNode,
            toNode,
            -minResidualCapacity,
            edge.isForward);
        addEdgeWeightBy(
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
        // printPathWithWeights(child, t, this);

        if (child.empty())
        {
            return allReachable(s);
        }
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
    if (!nodeExists(start) || !nodeExists(end))
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

        for (int otherNode = 0; otherNode < matrix.size(); otherNode++)
        {
            if (
                connectionExists(current, otherNode, true) &&
                visited.find(otherNode) == visited.end() &&
                getWeight(current, otherNode, true) > EPSILON)
            {
                child[otherNode] = {current, getWeight(current, otherNode, true), true};
                q.push(otherNode);
            }

            if (
                connectionExists(current, otherNode, false) &&
                visited.find(otherNode) == visited.end() &&
                getWeight(current, otherNode, false) > EPSILON)
            {
                child[otherNode] = {current, getWeight(current, otherNode, false), false};
                q.push(otherNode);
            }
        }
        // each forward edge has corresponding reverse edge
        /*
        for (auto neighbour : *nodesMap[current])
        {
            if (visited.find(neighbour.node) == visited.end() && std::abs(neighbour.weight) > EPSILON)
            {
                child[neighbour.node] = {current, neighbour.weight, neighbour.isForward};
                q.push(neighbour.node);
            }
        }
        */
    }
    child.clear();
    return child;
}

ResidualGraph *Graph::createResidualGraph()
{
    auto r = new ResidualGraph(); // throws error if memory allocatino fails
    for (int nodeFrom = 0; nodeFrom < matrix.size(); nodeFrom++)
    {
        for (int nodeTo = 0; nodeTo < matrix.size(); nodeTo++)
        {
            if (connectionExists(nodeFrom, nodeTo))
            {
                r->addNewConnection(nodeFrom, nodeTo, getWeight(nodeFrom, nodeTo), true);
                // adding corresponding reverse edge
                r->addNewConnection(nodeTo, nodeFrom, 0, false);
            }
        }
    }
    return r;
}
