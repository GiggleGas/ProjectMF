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

void FForceDirectedSolver::CalculateForces(IForceDirectedGraph* Graph, const FVector2D& GraphPosition)
{
    // Calculate center gravity
    CalculateCenterGravity(Graph, GraphPosition);

    // Calculate node repulsion
    CalculateNodeForces(Graph);
    
    // Calculate edge forces (spring forces)
    CalculateEdgeForces(Graph);
    
    // Calculate edge midpoint repulsion
    CalculateEdgeMidpointRepulsion(Graph);

    // Calculate edge repulsion forces
    CalculateEdgeRepulsion(Graph);
}


void FForceDirectedSolver::CalculateRepulsionForces(IForceDirectedGraph* GraphA, IForceDirectedGraph* GraphB, const TArray<int>& IgnoreNodeIdsB)
{

    GraphA->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* NodeA, const FForceDirectedNodeInfo&) 
    {
        float Fx = 0;
        float Fy = 0;
        GraphB->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* NodeB, const FForceDirectedNodeInfo& NodeInfoB) 
        {
            if (IgnoreNodeIdsB.Contains(NodeInfoB.NodeIndex)) return; // Skip nodes in the ignore list

            // Calculate distance between nodes
            float Dx = NodeA->Position.X - NodeB->Position.X;
            float Dy = NodeA->Position.Y - NodeB->Position.Y;
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
            float Force = Params.Repulsion * (NodeA->Weight + NodeB->Weight) * 0.5f / DistSq;
            
            // Add repulsion force component
            Fx += (Dx / Dist) * Force;
            Fy += (Dy / Dist) * Force;
        });
                // Apply forces to velocity of node A
        NodeA->Velocity.X += Fx;
        NodeA->Velocity.Y += Fy;
    });
}

void FForceDirectedSolver::ResetNodeVelocities(IForceDirectedGraph* Graph)
{
    // Reset velocities (since we're directly modifying velocity in this implementation)
    Graph->ForEachNode([this](IForceDirectedGraph*, FForceDirectedNode* Node, const FForceDirectedNodeInfo&) {
        // Apply damping to velocity
        Node->Velocity *= Params.Damping;
    });
}

void FForceDirectedSolver::CalculateCenterGravity(IForceDirectedGraph* Graph, const FVector2D& GraphPosition)
{
    // Calculate center gravity for nodes
    Graph->ForEachNode([this, &GraphPosition](IForceDirectedGraph*, FForceDirectedNode* Node, const FForceDirectedNodeInfo& NodeInfo) {
        // Additional gravity toward the graph's position (stronger pull based on CenterGravityScale)
        float Fx = (GraphPosition.X - Node->Position.X) * (Params.CenterPull * 2.0f) * NodeInfo.CenterGravityScale;
        float Fy = (GraphPosition.Y - Node->Position.Y) * (Params.CenterPull * 2.0f) * NodeInfo.CenterGravityScale;

        // Apply forces to velocity
        Node->Velocity.X += Fx;
        Node->Velocity.Y += Fy;
    });
}

void FForceDirectedSolver::CalculateNodeForces(IForceDirectedGraph* Graph)
{
    Graph->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* Node, const FForceDirectedNodeInfo&) 
    {
        float Fx = 0;
        float Fy = 0;

        Graph->ForEachNode([ &](IForceDirectedGraph*, FForceDirectedNode* OtherNode, const FForceDirectedNodeInfo&) {
            if (Node == OtherNode ) return;// Skip self-edges
            
            float Dx = Node->Position.X - OtherNode->Position.X;
            float Dy = Node->Position.Y - OtherNode->Position.Y;
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
            float Force = Params.Repulsion * (Node->Weight + OtherNode->Weight) * 0.5f / DistSq;

            // Add repulsion force component
            Fx += (Dx / Dist) * Force;
            Fy += (Dy / Dist) * Force;
            
        });

        Node->Velocity.X += Fx;
        Node->Velocity.Y += Fy;
    });
}

void FForceDirectedSolver::CalculateEdgeForces(IForceDirectedGraph* Graph)
{
    // Calculate spring forces for edges
    Graph->ForEachEdge([this](IForceDirectedGraph*, FForceDirectedNode* Source, FForceDirectedNode* Target, const FForceDirectedEdgeInfo& EdgeInfo) {
        float Dx = Target->Position.X - Source->Position.X;
        float Dy = Target->Position.Y - Source->Position.Y;
        float Dist = FMath::Sqrt(Dx * Dx + Dy * Dy);

        if (Dist == 0)
        {
            return; // Avoid division by zero
        }

        // Hooke's Law: F = k * (current length - rest length)
        // Use EdgeLengthScale to scale the spring length for this edge
        float ScaledSpringLength = Params.SpringLength * EdgeInfo.EdgeLengthScale;
        float Force = (Dist - ScaledSpringLength) * Params.SpringStrength;

        // Calculate force components
        float Fx1 = (Dx / Dist) * Force;
        float Fy1 = (Dy / Dist) * Force;

        // Apply spring forces
        Source->Velocity.X += Fx1;
        Source->Velocity.Y += Fy1;
        Target->Velocity.X -= Fx1;
        Target->Velocity.Y -= Fy1;
    });
}

void FForceDirectedSolver::CalculateEdgeMidpointRepulsion(IForceDirectedGraph* Graph)
{
    const int32 EdgeNum = Graph->GetEdgeNum();
    if (EdgeNum < 2)
    {
        return; // No need to calculate repulsion with less than 2 edges
    }

    // First, collect all edge midpoints and their endpoints
    TArray<FVector2D> Midpoints;
    TArray<TTuple<FForceDirectedNode*, FForceDirectedNode*>> EdgeEndpoints;
    
    Midpoints.Reserve(EdgeNum);
    EdgeEndpoints.Reserve(EdgeNum);
    
    Graph->ForEachEdge([&](IForceDirectedGraph*, FForceDirectedNode* Source, FForceDirectedNode* Target, const FForceDirectedEdgeInfo&) 
    {
        // Calculate midpoint
        FVector2D Midpoint(
            (Source->Position.X + Target->Position.X) * 0.5f,
            (Source->Position.Y + Target->Position.Y) * 0.5f
        );
        
        Midpoints.Add(Midpoint);
        EdgeEndpoints.Emplace(Source, Target);
    });

    // Now calculate repulsion between midpoints
    const int32 ValidEdgeNum = Midpoints.Num();
    for (int32 i = 0; i < ValidEdgeNum; i++)
    {
        const FVector2D& MidpointA = Midpoints[i];
        FForceDirectedNode* SourceA = EdgeEndpoints[i].Get<0>();
        FForceDirectedNode* TargetA = EdgeEndpoints[i].Get<1>();
        
        for (int32 j = i + 1; j < ValidEdgeNum; j++)
        {
            const FVector2D& MidpointB = Midpoints[j];
            
            float Dx = MidpointA.X - MidpointB.X;
            float Dy = MidpointA.Y - MidpointB.Y;
            float DistSq = Dx * Dx + Dy * Dy;
            
            // Avoid division by zero
            if (DistSq < 0.01f)
            {
                DistSq = 0.01f;
            }
            
            float Dist = FMath::Sqrt(DistSq);
            
            // Repulsion force: inversely proportional to distance squared
            float Force = Params.Repulsion * 0.5f / DistSq;
            
            // Calculate force components
            float Fx = (Dx / Dist) * Force;
            float Fy = (Dy / Dist) * Force;
            
            // Apply half of the force to each endpoint of edge A
            SourceA->Velocity.X += Fx * 0.5f;
            SourceA->Velocity.Y += Fy * 0.5f;
            TargetA->Velocity.X += Fx * 0.5f;
            TargetA->Velocity.Y += Fy * 0.5f;
            
            // Apply opposite force to endpoints of edge B
            FForceDirectedNode* SourceB = EdgeEndpoints[j].Get<0>();
            FForceDirectedNode* TargetB = EdgeEndpoints[j].Get<1>();
            
            SourceB->Velocity.X -= Fx * 0.5f;
            SourceB->Velocity.Y -= Fy * 0.5f;
            TargetB->Velocity.X -= Fx * 0.5f;
            TargetB->Velocity.Y -= Fy * 0.5f;
        }
    }
}

void FForceDirectedSolver::UpdatePositions(IForceDirectedGraph* Graph, float DeltaTime)
{
    Graph->ForEachNode([DeltaTime](IForceDirectedGraph*, FForceDirectedNode* Node, const FForceDirectedNodeInfo&) {
        // Update position based on velocity
        Node->Position += Node->Velocity * DeltaTime;
    });
}

void FForceDirectedSolver::ApplyNodeOffset(IForceDirectedGraph* Graph, const FVector2D& PositionOffset, const FVector2D& VelocityOffset)
{
    Graph->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* Node, const FForceDirectedNodeInfo&) {
        // Apply offset to position
        Node->Position += PositionOffset;
        // Apply offset to velocity
        Node->Velocity += VelocityOffset;
    });
}

void FForceDirectedSolver::CalculateEdgeRepulsion(IForceDirectedGraph* Graph)
{
    // For each node, calculate forces to make edges tend towards 180 degrees apart
    const int NodeNum = Graph->GetNodeNum();
    
    for (int NodeIndex = 0; NodeIndex < NodeNum; ++NodeIndex)
    {
        FForceDirectedNode* CenterNode = Graph->GetNodeAt(NodeIndex);
        if (!CenterNode) continue;
        
        // Collect all edges connected to this node
        TArray<TTuple<FForceDirectedNode*, FVector2D>> EdgeList;
        
        Graph->ForEachEdgeOfNode(NodeIndex, [&](IForceDirectedGraph*, FForceDirectedNode* Source, FForceDirectedNode* Target, const FForceDirectedEdgeInfo&) {
            // Determine the direction from center node to the other endpoint
            FVector2D Direction;
            FForceDirectedNode* OtherNode;
            
            if (Source == CenterNode)
            {
                Direction = Target->Position - Source->Position;
                OtherNode = Target;
            }
            else
            {
                Direction = Source->Position - Target->Position;
                OtherNode = Source;
            }
            
            // Normalize direction
            float Length = Direction.Size();
            if (Length > 0.0001f)
            {
                Direction /= Length;
            }
            
            EdgeList.Add(MakeTuple(OtherNode, Direction));
        });
        
        // Calculate forces between all pairs of edges to make them tend towards 180 degrees
        const int EdgeCount = EdgeList.Num();
        for (int i = 0; i < EdgeCount; ++i)
        {
            for (int j = i + 1; j < EdgeCount; ++j)
            {
                FForceDirectedNode* NodeA = EdgeList[i].Get<0>();
                FVector2D DirA = EdgeList[i].Get<1>();
                
                FForceDirectedNode* NodeB = EdgeList[j].Get<0>();
                FVector2D DirB = EdgeList[j].Get<1>();
                
                // Calculate dot product: DotProduct = cos(theta)
                // When theta = 180 degrees, DotProduct = -1
                // When theta = 0 degrees, DotProduct = 1
                float DotProduct = FVector2D::DotProduct(DirA, DirB);
               
                // Calculate cross product to determine the relative orientation
                float CrossProduct = FVector2D::CrossProduct(DirA, DirB);
                
                // We want edges to be 180 degrees apart (DotProduct = -1)
                // Force is proportional to how far we are from the target (180 degrees)
                // Target: DotProduct = -1, so error = DotProduct - (-1) = DotProduct + 1
                // When edges are already 180 degrees apart, error = 0, no force
                // When edges are parallel (0 degrees), error = 2, maximum force
                float Error = DotProduct + 1.0f; // Range: 0 (at 180) to 2 (at 0)
                
                if (Error > 0.0001f)
                {
                    // Calculate the direction to rotate edges towards 180 degrees
                    // The goal is to rotate edge A clockwise and edge B counter-clockwise (or vice versa)
                    // so they become opposite directions
                    
                    // Determine rotation direction based on cross product
                    float RotationDir = CrossProduct > 0 ? -1.0f : 1.0f;
                    
                    // Apply force perpendicular to each edge direction
                    // This will rotate the edges around the center node
                    FVector2D PerpA(-DirA.Y, DirA.X);  // Perpendicular to edge A
                    FVector2D PerpB(-DirB.Y, DirB.X);  // Perpendicular to edge B
                    
                    // Rotate edge A and edge B in opposite directions to increase the angle between them
                    // Edge A rotates clockwise, Edge B rotates counter-clockwise (or vice versa)

                    // 固定大小为 EdgeRepulsion
                    FVector2D ForceA = PerpA * Params.EdgeRepulsion * RotationDir;
                    FVector2D ForceB = PerpB * Params.EdgeRepulsion * (-RotationDir);
                    
                    // Apply forces
                    NodeA->Velocity.X += ForceA.X;
                    NodeA->Velocity.Y += ForceA.Y;
                    NodeB->Velocity.X += ForceB.X;
                    NodeB->Velocity.Y += ForceB.Y;
                }
            }
        }
    }
}