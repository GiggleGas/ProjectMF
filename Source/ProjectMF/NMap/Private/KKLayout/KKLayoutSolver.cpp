#include "KKLayout/KKLayoutSolver.h"

FKKLayoutSolver::FKKLayoutSolver()
    : bInitialized(false)
{}

void FKKLayoutSolver::SetParams(const FKKParams& NewParams)
{
    Params = NewParams;
}

void FKKLayoutSolver::Initialize(IKKGraph* Graph)
{
    if (!Graph) return;
    
    const int NodeNum = Graph->GetNodeNum();
    
    // Initialize distance matrix
    DistanceMatrix.SetNum(NodeNum);
    SpringConstants.SetNum(NodeNum);
    for (int i = 0; i < NodeNum; ++i)
    {
        DistanceMatrix[i].SetNum(NodeNum);
        SpringConstants[i].SetNum(NodeNum);
        for (int j = 0; j < NodeNum; ++j)
        {
            DistanceMatrix[i][j] = (i == j) ? 0.0f : TNumericLimits<float>::Max();
            SpringConstants[i][j] = 0.0f;
        }
    }
    
    // Compute all-pairs shortest paths
    ComputeAllPairsShortestPaths(Graph);
    
    // Precompute spring constants: k_ij = K / d_ij^2
    for (int i = 0; i < NodeNum; ++i)
    {
        for (int j = 0; j < NodeNum; ++j)
        {
            if (i != j && DistanceMatrix[i][j] > 0 && DistanceMatrix[i][j] != TNumericLimits<float>::Max())
            {
                SpringConstants[i][j] = Params.K / (DistanceMatrix[i][j] * DistanceMatrix[i][j]);
            }
        }
    }
    
    // Calculate initial energy for all nodes and find max gradient node
    float MaxGradient = -TNumericLimits<float>::Max();
    int MaxGradientNodeIndex = -1;
    
    for (int i = 0; i < NodeNum; ++i)
    {
        FKKNodeData* Node = Graph->GetNodeAt(i);
        if (!Node) continue;
        
        // Calculate gradient magnitude for this node
        float GradientX = 0.0f, GradientY = 0.0f;
        CalculateGradient(Graph, i, GradientX, GradientY);
        float GradientMag = FMath::Sqrt(GradientX * GradientX + GradientY * GradientY);
        
        Node->Energy = GradientMag; // Store gradient magnitude as "energy" for selection
        
        if (GradientMag > MaxGradient)
        {
            MaxGradient = GradientMag;
            MaxGradientNodeIndex = i;
        }
    }
    
    // Cache the max gradient node index
    Graph->SetMaxEnergyNodeIndex(MaxGradientNodeIndex);
    
    bInitialized = true;
}

void FKKLayoutSolver::ComputeAllPairsShortestPaths(IKKGraph* Graph)
{
    const int NodeNum = Graph->GetNodeNum();
    
    // Initialize with direct edges (using unit distance for graph-theoretic shortest path)
    Graph->ForEachEdge([this](IKKGraph*, FKKNodeData* Source, FKKNodeData* Target, float) {
        int SourceIndex = Source->Index;
        int TargetIndex = Target->Index;
        
        if (SourceIndex >= 0 && SourceIndex < DistanceMatrix.Num() &&
            TargetIndex >= 0 && TargetIndex < DistanceMatrix.Num())
        {
            // Use unit distance for graph-theoretic shortest path count
            DistanceMatrix[SourceIndex][TargetIndex] = 1.0f;
            DistanceMatrix[TargetIndex][SourceIndex] = 1.0f;
        }
    });
    
    // Floyd-Warshall algorithm to compute shortest paths
    for (int k = 0; k < NodeNum; ++k)
    {
        for (int i = 0; i < NodeNum; ++i)
        {
            for (int j = 0; j < NodeNum; ++j)
            {
                if (DistanceMatrix[i][k] != TNumericLimits<float>::Max() &&
                    DistanceMatrix[k][j] != TNumericLimits<float>::Max())
                {
                    float NewDistance = DistanceMatrix[i][k] + DistanceMatrix[k][j];
                    if (NewDistance < DistanceMatrix[i][j])
                    {
                        DistanceMatrix[i][j] = NewDistance;
                    }
                }
            }
        }
    }
    
    // For disconnected nodes, set a large distance
    for (int i = 0; i < NodeNum; ++i)
    {
        for (int j = 0; j < NodeNum; ++j)
        {
            if (i != j && DistanceMatrix[i][j] == TNumericLimits<float>::Max())
            {
                DistanceMatrix[i][j] = NodeNum * 2.0f; // Use a large distance for disconnected nodes
            }
        }
    }
}

float FKKLayoutSolver::GetDistance(FKKNodeData* NodeA, FKKNodeData* NodeB) const
{
    return FVector2D::Distance(NodeA->Position, NodeB->Position);
}

void FKKLayoutSolver::CalculateGradient(IKKGraph* Graph, int NodeIndex, float& OutGradientX, float& OutGradientY)
{
    OutGradientX = 0.0f;
    OutGradientY = 0.0f;
    
    FKKNodeData* TargetNode = Graph->GetNodeAt(NodeIndex);
    if (!TargetNode) return;
    
    const int NodeNum = Graph->GetNodeNum();
    
    for (int i = 0; i < NodeNum; ++i)
    {
        if (i == NodeIndex) continue;
        
        FKKNodeData* OtherNode = Graph->GetNodeAt(i);
        if (!OtherNode) continue;
        
        float dx = TargetNode->Position.X - OtherNode->Position.X;
        float dy = TargetNode->Position.Y - OtherNode->Position.Y;
        float d = FMath::Sqrt(dx * dx + dy * dy);
        
        if (d < 0.0001f)
        {
            dx = FMath::FRandRange(-0.01f, 0.01f);
            dy = FMath::FRandRange(-0.01f, 0.01f);
            d = FMath::Sqrt(dx * dx + dy * dy);
        }
        
        float L_mi = GetIdealLength(NodeIndex, i);
        float k_mi = GetSpringConstant(NodeIndex, i);
        
        // dE/dx_m = sum_{i≠m} k_mi * ((x_m - x_i) - L_mi * (x_m - x_i) / d)
        OutGradientX += k_mi * (dx - L_mi * dx / d);
        OutGradientY += k_mi * (dy - L_mi * dy / d);
    }
}

void FKKLayoutSolver::CalculateHessian(IKKGraph* Graph, int NodeIndex, float& Hxx, float& Hxy, float& Hyy)
{
    Hxx = 0.0f;
    Hxy = 0.0f;
    Hyy = 0.0f;
    
    FKKNodeData* TargetNode = Graph->GetNodeAt(NodeIndex);
    if (!TargetNode) return;
    
    const int NodeNum = Graph->GetNodeNum();
    
    for (int i = 0; i < NodeNum; ++i)
    {
        if (i == NodeIndex) continue;
        
        FKKNodeData* OtherNode = Graph->GetNodeAt(i);
        if (!OtherNode) continue;
        
        float dx = TargetNode->Position.X - OtherNode->Position.X;
        float dy = TargetNode->Position.Y - OtherNode->Position.Y;
        float d_sq = dx * dx + dy * dy;
        float d = FMath::Sqrt(d_sq);
        
        if (d < 0.0001f)
        {
            dx = FMath::FRandRange(-0.01f, 0.01f);
            dy = FMath::FRandRange(-0.01f, 0.01f);
            d_sq = dx * dx + dy * dy;
            d = FMath::Sqrt(d_sq);
        }
        
        float L_mi = GetIdealLength(NodeIndex, i);
        float k_mi = GetSpringConstant(NodeIndex, i);
        
        float d_cubed = d_sq * d;
        
        // d^2E/dx_m^2 = sum_{i≠m} k_mi * (1 - L_mi * dy^2 / d^3)
        Hxx += k_mi * (1.0f - L_mi * dy * dy / d_cubed);
        
        // d^2E/dx_mdy_m = sum_{i≠m} k_mi * (L_mi * dx * dy / d^3)
        Hxy += k_mi * (L_mi * dx * dy / d_cubed);
        
        // d^2E/dy_m^2 = sum_{i≠m} k_mi * (1 - L_mi * dx^2 / d^3)
        Hyy += k_mi * (1.0f - L_mi * dx * dx / d_cubed);
    }
}

float FKKLayoutSolver::CalculateTotalEnergy(IKKGraph* Graph)
{
    float TotalEnergy = 0.0f;
    const int NodeNum = Graph->GetNodeNum();
    
    for (int i = 0; i < NodeNum; ++i)
    {
        for (int j = i + 1; j < NodeNum; ++j)
        {
            FKKNodeData* NodeA = Graph->GetNodeAt(i);
            FKKNodeData* NodeB = Graph->GetNodeAt(j);
            
            if (!NodeA || !NodeB) continue;
            
            float dx = NodeA->Position.X - NodeB->Position.X;
            float dy = NodeA->Position.Y - NodeB->Position.Y;
            float d = FMath::Sqrt(dx * dx + dy * dy);
            float L_ij = GetIdealLength(i, j);
            float k_ij = GetSpringConstant(i, j);
            
            // E = 0.5 * k_ij * ((dx^2 + dy^2) + L_ij^2 - 2 * L_ij * d)
            float Energy = 0.5f * k_ij * (dx * dx + dy * dy + L_ij * L_ij - 2.0f * L_ij * d);
            TotalEnergy += Energy;
        }
    }
    
    return TotalEnergy;
}

bool FKKLayoutSolver::Iterate(IKKGraph* Graph)
{
    if (!Graph || !bInitialized) return false;
    
    const int NodeNum = Graph->GetNodeNum();
    
    // Find node with maximum gradient
    float MaxGradient = -TNumericLimits<float>::Max();
    int MaxGradientNodeIndex = -1;
    
    for (int i = 0; i < NodeNum; ++i)
    {
        FKKNodeData* Node = Graph->GetNodeAt(i);
        if (!Node) continue;
        
        float GradientX = 0.0f, GradientY = 0.0f;
        CalculateGradient(Graph, i, GradientX, GradientY);
        float GradientMag = FMath::Sqrt(GradientX * GradientX + GradientY * GradientY);
        
        Node->Energy = GradientMag;
        
        if (GradientMag > MaxGradient)
        {
            MaxGradient = GradientMag;
            MaxGradientNodeIndex = i;
        }
    }
    
    // Check global convergence
    if (MaxGradient < Params.Tolerance)
    {
        return true; // Converged
    }
    
    Graph->SetMaxEnergyNodeIndex(MaxGradientNodeIndex);
    
    // Perform Newton-Raphson local optimization on the selected node
    FKKNodeData* TargetNode = Graph->GetNodeAt(MaxGradientNodeIndex);
    if (!TargetNode) return false;
    
    for (int i = 0; i < Params.MaxIterations; ++i)
    {
        float GradientX = 0.0f, GradientY = 0.0f;
        CalculateGradient(Graph, MaxGradientNodeIndex, GradientX, GradientY);
        
        float GradientMag = FMath::Sqrt(GradientX * GradientX + GradientY * GradientY);
        
        // Check local convergence
        if (GradientMag < Params.Tolerance)
        {
            break;
        }
        
        // Calculate Hessian matrix
        float Hxx = 0.0f, Hxy = 0.0f, Hyy = 0.0f;
        CalculateHessian(Graph, MaxGradientNodeIndex, Hxx, Hxy, Hyy);
        
        // Solve linear system: H * delta = -gradient
        // Hxx * dx + Hxy * dy = -GradientX
        // Hxy * dx + Hyy * dy = -GradientY
        
        float Det = Hxx * Hyy - Hxy * Hxy;
        
        // Add small epsilon to avoid division by zero
        if (FMath::Abs(Det) < 0.0001f)
        {
            Det = 0.0001f;
        }
        
        float dx = (Hyy * (-GradientX) - Hxy * (-GradientY)) / Det;
        float dy = (Hxx * (-GradientY) - Hxy * (-GradientX)) / Det;
        
        // Update position
        TargetNode->Position.X += dx;
        TargetNode->Position.Y += dy;
    }
    
    return false; // Not converged yet
}

void FKKLayoutSolver::Solve(IKKGraph* Graph)
{
    if (!Graph || !bInitialized) return;
    
    float PreviousEnergy = CalculateTotalEnergy(Graph);
    
    for (int Iteration = 0; Iteration < Params.MaxGlobalIterations; ++Iteration)
    {
        if (Iterate(Graph))
        {
            // Converged based on gradient
            break;
        }
        
        // Check energy convergence
        float CurrentEnergy = CalculateTotalEnergy(Graph);
        float EnergyChange = FMath::Abs(CurrentEnergy - PreviousEnergy);
        
        if (EnergyChange < Params.EnergyTolerance)
        {
            // Converged based on energy change
            break;
        }
        
        PreviousEnergy = CurrentEnergy;
    }
}

void FKKLayoutSolver::Reset()
{
    DistanceMatrix.Empty();
    SpringConstants.Empty();
    bInitialized = false;
}