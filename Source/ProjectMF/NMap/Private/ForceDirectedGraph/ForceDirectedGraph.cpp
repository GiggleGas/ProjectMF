#include "ForceDirectedGraph/ForceDirectedGraph.h"

FForceDirectedGraph::FForceDirectedGraph()
{}

void FForceDirectedGraph::Initialize(const TArray<int>& InNodeIds, const TArray<FForceDirectedEdge>& InEdges)
{
    NodeIds = InNodeIds;
    Edges = InEdges;
}