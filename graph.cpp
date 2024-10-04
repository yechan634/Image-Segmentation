#include "graph.h"
#include "utilities.h"
#include <vector>
#include <algorithm>
#include <stack>

// used to interpret small floating point values as 0 from floating point errors
const double EPSILON = 1e-9;

Graph::Graph(int initialNodesNum)
{
    std::vector<std::vector<struct matrixElem>> _matrix(initialNodesNum);
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

struct matrixElem *Graph::getMatrixElem(nodeType u, nodeType v, bool isForward)
{ // can use this in place of connectionExists
    auto it = std::find_if(matrix[u].begin(), matrix[u].end(), [v, isForward](struct matrixElem edge)
                           { return (edge.toNode == v && edge.isForward == isForward); });
    if (it == matrix[u].end())
    {
        return nullptr;
    }
    return &(*it);
}

// isForward is true by default
bool Graph::connectionExists(nodeType u, nodeType v, bool isForward)
{
    if (!nodeExists(u) || !nodeExists(v))
    {
        return false;
    }
    return getMatrixElem(u, v, isForward) != nullptr;
}

weightType Graph::getWeight(nodeType u, nodeType v, bool isForward)
{
    if (!connectionExists(u, v, isForward)) // can use getMatrixElem here for micro improvement
    {
        throw std::invalid_argument("no such edge exists");
    }
    return getMatrixElem(u, v, isForward)->weight;
}

bool Graph::addNewConnection(nodeType u, nodeType v, weightType weight, bool isForward)
{
    // a connection already exists
    if (connectionExists(u, v, isForward))
    {
        return false;
    }
    nodeType largerNode = std::max(u, v);
    // assume u >= 0 so not checked as well
    if (!nodeExists(largerNode))
    {
        // Ensure the graph has enough space for the new node
        while (matrix.size() <= largerNode)
        {
            // Add a new row for the new node
            std::vector<struct matrixElem> newRow(largerNode + 1);
            matrix.push_back(newRow);
        }
    }
    matrix[u].push_back(
        {v,
         weight,
         isForward});
    return true; // Successfully added the connection
}

bool Graph::addEdgeWeightBy(nodeType u, nodeType v, weightType newWeight, bool isForward)
{
    if (!connectionExists(u, v, isForward))
    {
        return false;
    }
    getMatrixElem(u, v, isForward)->weight += newWeight;
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
        printf("\n%d, [", u);
        for (int v = 0; v < matrix.size(); v++)
        {
            if (connectionExists(u, v, true))
            {
                // used std::cout to use to_string, since getWeight() return type may change
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
        printf("]");
    }
    printf("\n");
}

Graph::~Graph()
{
}

ResidualGraph::ResidualGraph(matrixType originalMatrix)
{
    matrix = originalMatrix;
};

void ResidualGraph::pushFlow(childType augmentedPath, nodeType finalNode, weightType minResidualCapacity)
{
    for (nodeType current = finalNode; augmentedPath.find(current) != augmentedPath.end(); current = augmentedPath[current].node)
    {
        auto edge = augmentedPath[current];
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
        printPathWithWeights(child, t, this);

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

// uses dfs - more efficient than bfs for image graph since source is connected to many nodes,
// so will take longer for bfs to pop next node
childType ResidualGraph::getPath(nodeType start, nodeType end)
{
    std::stack<nodeType> s;
    childType child;
    std::set<nodeType> visited;

    if (!nodeExists(start) || !nodeExists(end))
    {
        return child; // Return empty if start or end is invalid
    }

    s.push(start);

    while (!s.empty())
    {
        nodeType current = s.top();
        s.pop();

        if (current == end)
        {
            return child;
        }

        if (visited.find(current) == visited.end())
        {
            visited.insert(current);
            for (auto edge : matrix[current])
            {
                if (visited.find(edge.toNode) == visited.end() && std::abs(edge.weight) > EPSILON)
                {
                    child[edge.toNode] = {current, edge.weight, edge.isForward};
                    s.push(edge.toNode);
                }
            }
        }
    }

    child.clear(); // Return empty child if no path is found
    return child;
}

ResidualGraph *Graph::createResidualGraph()
{
    auto r = new ResidualGraph(matrix); // throws error if memory allocatino fails
    for (int fromNode = 0; fromNode < matrix.size(); fromNode++)
    {
        for (auto edge : matrix[fromNode])
        {
            auto toNode = edge.toNode;
            // adding corresponding reverse edge
            r->addNewConnection(toNode, fromNode, 0, false);
        }
    }
    return r;
}
