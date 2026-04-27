#include "ForceDirectedGraph/ForceDirectedSolver.h"

FForceDirectedSolver::FForceDirectedSolver()
    : CanvasWidth(0.0f)
    , CanvasHeight(0.0f)
{}

void FForceDirectedSolver::SetParams(const FForceDirectedParams& NewParams)
{
    Params = NewParams;
}

void FForceDirectedSolver::SetCanvasSize(float Width, float Height)
{
    CanvasWidth = Width;
    CanvasHeight = Height;
}

void FForceDirectedSolver::CalculateForces(const TArray<int>& NodeIds, TArray<FForceDirectedNode>& Nodes, const TArray<FForceDirectedEdge>& Edges, const FVector2D& GraphPosition)
{

    // Calculate node forces (center gravity and repulsion)
    CalculateNodeForces(NodeIds, Nodes, GraphPosition);

    // Calculate edge forces (spring forces)
    CalculateEdgeForces(Edges, Nodes);
}

void FForceDirectedSolver::CalculateForcesForAllNodes(TArray<FForceDirectedNode>& Nodes, const TArray<FForceDirectedEdge>& Edges, const FVector2D& GraphPosition)
{
    // Create an array of all node indices
    TArray<int> AllNodeIds;
    for (int i = 0; i < Nodes.Num(); i++)
    {
        AllNodeIds.Add(i);
    }
    
    // Calculate node forces (center gravity and repulsion) for all nodes
    CalculateNodeForces(AllNodeIds, Nodes, GraphPosition);

    // Calculate edge forces (spring forces) for all edges
    CalculateEdgeForces(Edges, Nodes);
}

void FForceDirectedSolver::CalculateAllNodeForces(TArray<FForceDirectedNode>& Nodes, const FVector2D& GraphPosition)
{
    // Create an array of all node indices
    TArray<int> AllNodeIds;
    for (int i = 0; i < Nodes.Num(); i++)
    {
        AllNodeIds.Add(i);
    }
    
    // Calculate node forces (center gravity and repulsion) for all nodes
    CalculateNodeForces(AllNodeIds, Nodes, GraphPosition);
    
    // Note: No edge forces calculation since this method is intended for graph nodes
    // which don't have edges between them
}

void FForceDirectedSolver::CalculateRepulsionForces(const TArray<int>& NodeIdsA, TArray<FForceDirectedNode>& NodesA, const TArray<int>& IgnoreNodeIdsB, const TArray<FForceDirectedNode>& NodesB)
{
    // Calculate repulsion forces for nodes in A, ignoring nodes in IgnoreNodeIdsB
    for (int32 i = 0; i < NodeIdsA.Num(); i++)
    {
        // Get node A
        int NodeIndexA = NodeIdsA[i];
        if (NodeIndexA < 0 || NodeIndexA >= NodesA.Num())
        {
            continue; // Skip if index is out of bounds
        }
        
        FForceDirectedNode& NodeA = NodesA[NodeIndexA];
        float Fx = 0;
        float Fy = 0;
        
        // Calculate repulsion from all nodes in B except those in IgnoreNodeIdsB
        for (int32 j = 0; j < NodesB.Num(); j++)
        {
            // Check if this node is in the ignore list
            if (IgnoreNodeIdsB.Contains(j))
            {
                continue; // Skip nodes in the ignore list
            }
            
            const FForceDirectedNode& NodeB = NodesB[j];
            
            // Calculate distance between nodes
            float Dx = NodeA.Position.X - NodeB.Position.X;
            float Dy = NodeA.Position.Y - NodeB.Position.Y;
            float DistSq = Dx * Dx + Dy * Dy;
            
            // Avoid division by zero
            if (DistSq == 0)
            {
                DistSq = 0.1f;
                Fx += FMath::FRand();
                Fy += FMath::FRand();
            }
            
            float Dist = FMath::Sqrt(DistSq);
            // Repulsion force: proportional to average weight, inversely proportional to distance squared
            float Force = Params.Repulsion * (NodeA.Weight + NodeB.Weight) * 0.5f / DistSq;
            
            // Add repulsion force component
            Fx += (Dx / Dist) * Force;
            Fy += (Dy / Dist) * Force;
        }
        
        // Apply forces to velocity of node A
        NodeA.Velocity.X += Fx;
        NodeA.Velocity.Y += Fy;
    }
}

void FForceDirectedSolver::ResetNodeVelocities(TArray<FForceDirectedNode>& Nodes)
{
    // Reset velocities (since we're directly modifying velocity in this implementation)
    for (FForceDirectedNode& Node : Nodes)
    {
        // Apply damping to velocity
        Node.Velocity *= Params.Damping;
    }
}

void FForceDirectedSolver::CalculateNodeForces(const TArray<int>& NodeIds, TArray<FForceDirectedNode>& Nodes, const FVector2D& GraphPosition)
{
    float Fx = 0;
    float Fy = 0;

    // Calculate center gravity and node repulsion
    for (int32 i = 0; i < NodeIds.Num(); i++)
    {
        // NodeIds 就是数组索引
        int NodeIndex = NodeIds[i];
        if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
        {
            continue; // Skip if index is out of bounds
        }

        FForceDirectedNode& Node = Nodes[NodeIndex];

        // Additional gravity toward the graph's position (stronger pull)
        Fx = (GraphPosition.X - Node.Position.X) * (Params.CenterPull * 2.0f);
        Fy = (GraphPosition.Y - Node.Position.Y) * (Params.CenterPull * 2.0f);

        // Node repulsion forces
        for (int32 j = 0; j < NodeIds.Num(); j++)
        {
            if (i == j)
            {
                continue; // Skip self
            }

            // NodeIds 就是数组索引
            int OtherNodeIndex = NodeIds[j];
            if (OtherNodeIndex < 0 || OtherNodeIndex >= Nodes.Num())
            {
                continue; // Skip if index is out of bounds
            }

            FForceDirectedNode& OtherNode = Nodes[OtherNodeIndex];

            float Dx = Node.Position.X - OtherNode.Position.X;
            float Dy = Node.Position.Y - OtherNode.Position.Y;
            float DistSq = Dx * Dx + Dy * Dy;

            // Avoid division by zero
            if (DistSq == 0)
            {
                DistSq = 0.1f;
                Fx += FMath::FRand();
                Fy += FMath::FRand();
            }

            float Dist = FMath::Sqrt(DistSq);
            // Repulsion force: proportional to average weight, inversely proportional to distance squared
            float Force = Params.Repulsion * (Node.Weight + OtherNode.Weight) * 0.5f / DistSq;

            // Add repulsion force component
            Fx += (Dx / Dist) * Force;
            Fy += (Dy / Dist) * Force;
        }

        // Apply forces to velocity
        Node.Velocity.X += Fx;
        Node.Velocity.Y += Fy;
    }
}

void FForceDirectedSolver::CalculateEdgeForces(const TArray<FForceDirectedEdge>& Edges, TArray<FForceDirectedNode>& Nodes)
{
    // Calculate spring forces for edges
    for (const FForceDirectedEdge& Edge : Edges)
    {
        // Edge.SourceId 和 Edge.TargetId 是 NodeIds 数组中的索引
        int SourceIndex = Edge.SourceId;
        int TargetIndex = Edge.TargetId;

        if (SourceIndex < 0 || SourceIndex >= Nodes.Num() || TargetIndex < 0 || TargetIndex >= Nodes.Num())
        {
            continue; // Skip if indices are out of bounds
        }

        FForceDirectedNode& Source = Nodes[SourceIndex];
        FForceDirectedNode& Target = Nodes[TargetIndex];

        float Dx = Target.Position.X - Source.Position.X;
        float Dy = Target.Position.Y - Source.Position.Y;
        float Dist = FMath::Sqrt(Dx * Dx + Dy * Dy);

        if (Dist == 0)
        {
            continue; // Avoid division by zero
        }

        // Hooke's Law: F = k * (current length - rest length)
        float Force = (Dist - Params.SpringLength) * Params.SpringStrength;

        // Calculate force components
        float Fx1 = (Dx / Dist) * Force;
        float Fy1 = (Dy / Dist) * Force;

        // Apply spring forces
        Source.Velocity.X += Fx1;
        Source.Velocity.Y += Fy1;
        Target.Velocity.X -= Fx1;
        Target.Velocity.Y -= Fy1;
    }
}

void FForceDirectedSolver::UpdatePositions(TArray<FForceDirectedNode>& Nodes, float DeltaTime)
{
    for (FForceDirectedNode& Node : Nodes)
    {
        // Update position based on velocity
        Node.Position += Node.Velocity * DeltaTime;
    }
}