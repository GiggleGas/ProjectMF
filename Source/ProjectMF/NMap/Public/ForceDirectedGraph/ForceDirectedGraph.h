#pragma once

#include "CoreMinimal.h"
#include "ForceDirectedGraph/ForceDirectedSolver.h"
class FForceDirectedGraph : public TSharedFromThis<FForceDirectedGraph>
{
public:

    // Constructor
    FForceDirectedGraph();

    // Initialize with node IDs and edges
    void Initialize(const TArray<int>& InNodeIds, const TArray<FForceDirectedEdge>& InEdges);

    // Get node IDs and edges
    const TArray<int>& GetNodeIds() const { return NodeIds; }
    const TArray<FForceDirectedEdge>& GetEdges() const { return Edges; }

private:
    // Node IDs and edges (only topology information)
    TArray<int> NodeIds;
    TArray<FForceDirectedEdge> Edges;

};