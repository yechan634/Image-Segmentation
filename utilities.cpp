#include "graph.h"
#include "utilities.h"

#include <sstream>
#include <algorithm>

void printPathWithWeights(childType child, nodeType finalNode, ResidualGraph *r)
{
    if (child.empty())
    {
        std::cout << "no path found\n";
        return;
    }
    std::string output;
    output = std::to_string(finalNode) + "\n";
    for (nodeType current = finalNode; child.find(current) != child.end(); current = child[current].node)
    {
        auto edge = child[current];
        output = std::to_string(edge.node) +
                 " -> (" + std::to_string(edge.isForward ? r->getWeight(edge.node, current, true) : r->getWeight(edge.node, current, false)) + ") -> " + output;
    }
    std::cout << output;
}

