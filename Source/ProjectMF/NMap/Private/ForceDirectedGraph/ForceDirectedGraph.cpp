#include "ForceDirectedGraph/ForceDirectedGraph.h"

FForceDirectedGraph::FForceDirectedGraph():Position(FVector2D::ZeroVector)
    , Velocity(FVector2D::ZeroVector)
{}

void FForceDirectedGraph::Initialize(const TArray<FNode>& InNodes, const TArray<FEdge>& InEdges)
{
    Nodes = InNodes;
    Edges = InEdges;

    // Initialize velocities to zero
    for (FNode& Node : Nodes)
    {
        Node.Velocity = FVector2D::ZeroVector;
    }

    // Calculate initial position as the centroid of all nodes
    if (Nodes.Num() > 0)
    {
        FVector2D Centroid = FVector2D::ZeroVector;
        for (const FNode& Node : Nodes)
        {
            Centroid += Node.Position;
        }
        Centroid /= Nodes.Num();
        Position = Centroid;
    }
}

void FForceDirectedGraph::SetCanvasSize(float Width, float Height)
{
    CanvasWidth = Width;
    CanvasHeight = Height;
}

float FForceDirectedGraph::GetTotalWeight() const
{
    float TotalWeight = 0.0f;

    // Add weight of all nodes
    for (const FNode& Node : Nodes)
    {
        TotalWeight += Node.Weight;
    }

    // Add weight of all child graphs
    for (const TSharedPtr<FForceDirectedGraph>& ChildGraph : ChildrenGraphs)
    {
        if (ChildGraph.IsValid())
        {
            TotalWeight += ChildGraph->GetTotalWeight();
        }
    }

    return TotalWeight;
}

void FForceDirectedGraph::CalculateChildGraphForces()
{
    // Calculate canvas center
    // float CenterX = CanvasWidth / 2.0f;
    // float CenterY = CanvasHeight / 2.0f;

    // Apply damping to velocity
    // Velocity *= Params.Damping;

    // Center gravity force
 
    for (int32 i = 0; i < ChildrenGraphs.Num(); i++)
    {
        TSharedPtr<FForceDirectedGraph> ChildA = ChildrenGraphs[i];
        if (!ChildA.IsValid()) continue;
        ChildA->Velocity *= Params.Damping; // child's param?
        ChildA->Velocity += (Position - ChildA->Position) * Params.CenterPull;
        
        // Velocity.X -= F; // ?
    }

    //float Fx = 0;
    //float Fy = 0;

    // Child graph repulsion forces
    for (int32 i = 0; i < ChildrenGraphs.Num(); i++)
    {
        TSharedPtr<FForceDirectedGraph> ChildA = ChildrenGraphs[i];
        if (!ChildA.IsValid()) continue;

        for (int32 j = i + 1; j < ChildrenGraphs.Num(); j++)
        {
            TSharedPtr<FForceDirectedGraph> ChildB = ChildrenGraphs[j];
            if (!ChildB.IsValid()) continue;

            float Dx = ChildA->Position.X - ChildB->Position.X;
            float Dy = ChildA->Position.Y - ChildB->Position.Y;
            float DistSq = Dx * Dx + Dy * Dy;

            // Avoid division by zero
            if (DistSq == 0)
            {
                DistSq = 0.1f;
                //Fx += FMath::FRand();
                //Fy += FMath::FRand();
            }

            float Dist = FMath::Sqrt(DistSq);
            // Repulsion force: proportional to total weights, inversely proportional to distance squared
            float Force = Params.Repulsion * (ChildA->GetTotalWeight() + ChildB->GetTotalWeight()) * 0.5f / DistSq;

            // Add repulsion force component
            float ForceX = (Dx / Dist) * Force;
            float ForceY = (Dy / Dist) * Force;

            ChildA->Velocity.X += ForceX;
            ChildA->Velocity.Y += ForceY;
            ChildB->Velocity.X -= ForceX;
            ChildB->Velocity.Y -= ForceY;
        }
    }
}

void FForceDirectedGraph::UpdateChildGraphPositions(float DeltaTime)
{
    // Update child graph positions
    for (TSharedPtr<FForceDirectedGraph> ChildGraph : ChildrenGraphs)
    {
        if (ChildGraph.IsValid())
        {
            ChildGraph->Position += ChildGraph->Velocity * DeltaTime;
        }
    }
}

void FForceDirectedGraph::UpdateNodesRelativeToGraphPosition()
{
    // Calculate centroid of nodes
    FVector2D Centroid = FVector2D::ZeroVector;
    int NodeCount = 0;
    
    for (const FNode& Node : Nodes)
    {
        Centroid += Node.Position;
        NodeCount++;
    }
    
    if (NodeCount > 0)
    {
        Centroid /= NodeCount;
    }

    // Calculate offset from centroid to graph position
    FVector2D Offset = Position - Centroid;

    // Update node positions to be relative to the graph's position
    for (FNode& Node : Nodes)
    {
        Node.Position += Offset;
    }
}

void FForceDirectedGraph::Update(float DeltaTime)
{
    // Top-down approach: update parent first, then children
    
    // nodea no side effect
    // 1. Calculate forces for this graph's nodes
    CalculateForces();
    
    // 2. Update this graph's node positions
    UpdatePositions(DeltaTime);
    


    // Update child graphs
    for (TSharedPtr<FForceDirectedGraph> ChildGraph : ChildrenGraphs)
    {
        if (ChildGraph.IsValid())
        {
            ChildGraph->Update(DeltaTime);
        }
    }

    // 3. Calculate centroid of nodes and update graph position
    /*
    if (Nodes.Num() > 0)
    {
        FVector2D Centroid = FVector2D::ZeroVector;
        for (const FNode& Node : Nodes)
        {
            Centroid += Node.Position;
        }
        Centroid /= Nodes.Num();
        Position = Centroid;
    }
    */
    // 4. Calculate forces for child graphs (treating each as a single node)
    CalculateChildGraphForces();

    // 5. Update child graph positions
    UpdateChildGraphPositions(DeltaTime);
    
    // 6. Update child graphs recursively
    /*
    for (TSharedPtr<FForceDirectedGraph> ChildGraph : ChildrenGraphs)
    {
        if (ChildGraph.IsValid())
        {
            ChildGraph->Update(DeltaTime);
        }
    }
    */
    // 7. Update nodes to be relative to the graph's position
    // UpdateNodesRelativeToGraphPosition();
}

void FForceDirectedGraph::CalculateForces()
{

    // Reset velocities (since we're directly modifying velocity in this implementation)
    for (FNode& Node : Nodes)
    {
        // Apply damping to velocity
        Node.Velocity *= Params.Damping;
    }

    float Fx = 0;
    float Fy = 0;

    // Calculate center gravity and node repulsion
    for (int32 i = 0; i < Nodes.Num(); i++)
    {
        FNode& Node = Nodes[i];

        // Additional gravity toward the graph's position (stronger pull)
        Fx += (Position.X - Node.Position.X) * (Params.CenterPull * 2.0f);
        Fy += (Position.Y - Node.Position.Y) * (Params.CenterPull * 2.0f);

        // Node repulsion forces
        for (int32 j = 0; j < Nodes.Num(); j++)
        {
            if (i == j)
            {
                continue; // Skip self
            }

            FNode& OtherNode = Nodes[j];
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

    // Calculate spring forces for edges
    for (const FEdge& Edge : Edges)
    {
        // Find source and target nodes
        FNode* Source = nullptr;
        FNode* Target = nullptr;

        for (FNode& Node : Nodes)
        {
            if (Node.Id == Edge.SourceId)
            {
                Source = &Node;
            }
            else if (Node.Id == Edge.TargetId)
            {
                Target = &Node;
            }

            if (Source && Target)
            {
                break;
            }
        }

        if (!Source || !Target)
        {
            continue; // Skip if nodes not found
        }


        float Dx = Target->Position.X - Source->Position.X;
        float Dy = Target->Position.Y - Source->Position.Y;
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
        Source->Velocity.X += Fx1;
        Source->Velocity.Y += Fy1;
        Target->Velocity.X -= Fx1;
        Target->Velocity.Y -= Fy1;
    }
}

void FForceDirectedGraph::UpdatePositions(float DeltaTime)
{
    for (FNode& Node : Nodes)
    {
        // Update position based on velocity
        Node.Position += Node.Velocity * DeltaTime;
    }

}

void FForceDirectedGraph::SetParent(TSharedPtr<FForceDirectedGraph> InParent)
{
    ParentGraph = InParent;
}

void FForceDirectedGraph::AddChild(TSharedPtr<FForceDirectedGraph> InChild)
{
    if (InChild.IsValid())
    {
        ChildrenGraphs.Add(InChild);
        InChild->SetParent(SharedThis(this));
    }
}

void FForceDirectedGraph::RemoveChild(TSharedPtr<FForceDirectedGraph> InChild)
{
    if (InChild.IsValid())
    {
        ChildrenGraphs.Remove(InChild);
        InChild->SetParent(nullptr);
    }
}

TSharedPtr<FForceDirectedGraph> FForceDirectedGraph::GetParent() const
{
    return ParentGraph.Pin();
}

const TArray<TSharedPtr<FForceDirectedGraph>>& FForceDirectedGraph::GetChildren() const
{
    return ChildrenGraphs;
}