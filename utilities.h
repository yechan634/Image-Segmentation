#ifndef UTILITIES_H
#define UTILITIES_H

#include "graph.h"

void printPathWithWeights(childType child, nodeType finalNode, ResidualGraph *r);

template <typename T>
void printNodeSet(std::set<T> nodes)
{
    for (const nodeType &n : nodes)
    {
        std::cout << ", " + std::to_string(n);
    }
    std::cout << "\n";
}

/*

parentType createPath(const std::vector<nodeType> nodes);

bool samePath(parentType parent1, parentType parent2);



*/
template<typename T>
bool sameSet(const std::set<T>& set1, const std::set<T>& set2) {
    // Check if the sizes are different
    if (set1.size() != set2.size()) {
        return false;
    }

    // Check if all elements in set1 are in set2
    for (const auto& element : set1) {
        if (set2.find(element) == set2.end()) {
            return false; // Element not found in set2
        }
    }
    return true;
}

#endif