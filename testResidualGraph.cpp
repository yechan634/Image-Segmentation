#include <gtest/gtest.h>
#include "utilities.h"

TEST(GraphTest, ModifyingWeights)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 3);
    g->addEdgeWeightBy(1, 2, -3);
    assert(g->getWeight(1, 2) == 0);
    delete g;
}

TEST(GraphTest, ModifyingWeights2)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 3);
    g->addEdgeWeightBy(1, 2, -3);
    g->addEdgeWeightBy(1, 2, 2);
    assert(g->getWeight(1, 2) == 2);
    delete g;
}

TEST(GraphTest, NodeExists)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 3);
    assert(g->nodeExists(1));
    assert(g->nodeExists(2));
    delete g;
}

TEST(ResidualGraph, PushFlow)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 2);
    auto r = g->createResidualGraph();
    r->print();
    childType path;
    path[2] = {1, 2, true};
    r->pushFlow(path, 2, 2);
    assert(r->getWeight(1, 2, true) == 0);
    assert(r->getWeight(2, 1, false) == 2);
    delete g;
    delete r;
}

TEST(ResidualGraph, PushFlowWithCycle)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 3);
    g->addNewConnection(2, 1, 3);
    g->addNewConnection(2, 3, 1);
    auto r = g->createResidualGraph();
    childType path;
    path[3] = {2, 1, true};
    path[2] = {1, 3, true};
    r->pushFlow(path, 3, 1);
    assert(r->getWeight(1, 2, true) == 2);
    assert(r->getWeight(2, 1, false) == 1);
    assert(r->getWeight(2, 1, true) == 3);
    assert(r->getWeight(1, 2, false) == 0);
    assert(r->getWeight(2, 3, true) == 0);
    assert(r->getWeight(3, 2, false) == 1);
    delete g;
    delete r;
}

TEST(ResidualGraph, allReachable)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 3);
    g->addNewConnection(2, 3, 3);
    g->addNewConnection(4, 1, 1);
    auto r = g->createResidualGraph();
    auto actualSet = r->allReachable(1);
    auto expectedSet = std::set<nodeType>{1, 2, 3};
    assert(sameSet(actualSet, expectedSet));
    delete g;
    delete r;
}

TEST(ResidualGraph, MinCut)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 5);
    g->addNewConnection(2, 1, 5);
    g->addNewConnection(2, 3, 2);
    auto r = g->createResidualGraph();
    auto actualSet = r->getMinCut(1, 3);
    auto expectedSet = std::set<nodeType>{1, 2}; // Min cut separates nodes {1, 2, 3} from 4
    assert(sameSet(actualSet, expectedSet));
    delete g;
    delete r;
}

TEST(ResidualGraph, MinCut_CircularGraph)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 3);
    g->addNewConnection(2, 3, 4);
    g->addNewConnection(3, 1, 2); // Circular connection
    g->addNewConnection(3, 4, 1);
    auto r = g->createResidualGraph();
    auto actualSet = r->getMinCut(1, 4);
    auto expectedSet = std::set<nodeType>{1, 2, 3};
    assert(sameSet(actualSet, expectedSet));
    delete g;
    delete r;
}

TEST(ResidualGraph, MinCut_DisconnectedGraph)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 5);
    g->addNewConnection(3, 4, 3);
    auto r = g->createResidualGraph();
    auto actualSet = r->getMinCut(1, 4);
    auto expectedSet = std::set<nodeType>{1, 2};
    assert(sameSet(actualSet, expectedSet));
    delete g;
    delete r;
}

TEST(ResidualGraph, MinCut_Complex)
{
    Graph *g = new Graph();
    g->addNewConnection(1, 2, 8);
    g->addNewConnection(1, 3, 3);
    g->addNewConnection(2, 4, 2);
    g->addNewConnection(2, 3, 5);
    g->addNewConnection(3, 5, 4);
    g->addNewConnection(4, 5, 6);
    g->addNewConnection(4, 6, 9);
    g->addNewConnection(5, 6, 7);
    auto r = g->createResidualGraph();
    auto actualSet = r->getMinCut(1, 6);
    auto expectedSet = std::set<nodeType>{1, 2, 3};
    assert(sameSet(actualSet, expectedSet));
    delete g;
    delete r;
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
