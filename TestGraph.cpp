#include "gtest/gtest.h"
#include "graph.h"
#include <memory>

// Test that residual edges are created and updated correctly
TEST(Graph, ResidualEdgeUpdate) {
    Graph g;
    int n0 = g.addNodeWithVal(0);
    int n1 = g.addNodeWithVal(0);
    int n2 = g.addNodeWithVal(0);
    g.addEdgeFromTo(n0, n1, 5);
    g.addEdgeFromTo(n1, n2, 5);
    double maxflow = g.performRelabel(n0, n2);
    EXPECT_DOUBLE_EQ(maxflow, 5);
}

// Test push-relabel with bidirectional edges
TEST(Graph, PushRelabelBidirectional) {
    Graph g;
    int s = g.addNodeWithVal(0);
    int a = g.addNodeWithVal(0);
    int b = g.addNodeWithVal(0);
    int t = g.addNodeWithVal(0);
    g.addEdgeFromTo(s, a, 10); // s->a
    g.addEdgeFromTo(a, b, 5);  // a->b
    g.addEdgeFromTo(b, a, 7);  // b->a (reverse)
    g.addEdgeFromTo(b, t, 10); // b->t
    double maxflow = g.performRelabel(s, t);
    EXPECT_DOUBLE_EQ(maxflow, 5);
}

// Test push-relabel with a cycle
TEST(Graph, PushRelabelCycle) {
    Graph g;
    int s = g.addNodeWithVal(0);
    int a = g.addNodeWithVal(0);
    int b = g.addNodeWithVal(0);
    int t = g.addNodeWithVal(0);
    g.addEdgeFromTo(s, a, 8); // s->a
    g.addEdgeFromTo(a, b, 4); // a->b
    g.addEdgeFromTo(b, a, 2); // b->a (cycle)
    g.addEdgeFromTo(b, t, 6); // b->t
    double maxflow = g.performRelabel(s, t);
    EXPECT_DOUBLE_EQ(maxflow, 4);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}