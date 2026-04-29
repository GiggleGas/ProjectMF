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
}


void FForceDirectedSolver::CalculateRepulsionForces(IForceDirectedGraph* GraphA, IForceDirectedGraph* GraphB, const TArray<int>& IgnoreNodeIdsB)
{

    GraphA->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* NodeA, int) 
    {
        float Fx = 0;
        float Fy = 0;
        GraphB->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* NodeB, int NodeIndexB) 
        {
            if (IgnoreNodeIdsB.Contains(NodeIndexB)) return; // Skip nodes in the ignore list

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
    Graph->ForEachNode([this](IForceDirectedGraph*, FForceDirectedNode* Node, int) {
        // Apply damping to velocity
        Node->Velocity *= Params.Damping;
    });
}

void FForceDirectedSolver::CalculateCenterGravity(IForceDirectedGraph* Graph, const FVector2D& GraphPosition)
{
    // Calculate center gravity for nodes
    Graph->ForEachNode([this, &GraphPosition](IForceDirectedGraph*, FForceDirectedNode* Node, int) {
        // Additional gravity toward the graph's position (stronger pull)
        float Fx = (GraphPosition.X - Node->Position.X) * (Params.CenterPull * 2.0f);
        float Fy = (GraphPosition.Y - Node->Position.Y) * (Params.CenterPull * 2.0f);

        // Apply forces to velocity
        Node->Velocity.X += Fx;
        Node->Velocity.Y += Fy;
    });
}

void FForceDirectedSolver::CalculateNodeForces(IForceDirectedGraph* Graph)
{
    Graph->ForEachNode([&](IForceDirectedGraph*, FForceDirectedNode* Node, int) 
    {
        float Fx = 0;
        float Fy = 0;

        Graph->ForEachNode([ &](IForceDirectedGraph*, FForceDirectedNode* OtherNode, int) {
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
    Graph->ForEachEdge([this](IForceDirectedGraph*, FForceDirectedNode* Source, FForceDirectedNode* Target, int) {
        float Dx = Target->Position.X - Source->Position.X;
        float Dy = Target->Position.Y - Source->Position.Y;
        float Dist = FMath::Sqrt(Dx * Dx + Dy * Dy);

        if (Dist == 0)
        {
            return; // Avoid division by zero
        }

        // Hooke's Law: F = k * (current length - rest length)
        float Force = (Dist - Params.SpringLength) * Params.SpringStrength;

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
    
    Graph->ForEachEdge([&](IForceDirectedGraph*, FForceDirectedNode* Source, FForceDirectedNode* Target, int) 
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
    Graph->ForEachNode([DeltaTime](IForceDirectedGraph*, FForceDirectedNode* Node, int) {
        // Update position based on velocity
        Node->Position += Node->Velocity * DeltaTime;
    });
}