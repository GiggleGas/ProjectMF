#include "NMGraph.h"
#include "IslandShape.h"
#include "VNGraph.h"
#include <Biome.h>

FNMGraph::FNMGraph(TSharedPtr<FVNGraph> Graph, int width, int height, float lakeThreshold)
{
    Init(FIslandShape::MakePerlin(), Graph,  width, height, lakeThreshold);
}

FNMGraph::FNMGraph(TFunction<bool(FVector2D)> checkIsland, TSharedPtr<FVNGraph> Graph,  int width, int height, float lakeThreshold)
{
    Init(checkIsland, Graph,  width, height, lakeThreshold);
}

void FNMGraph::Init(TFunction<bool(FVector2D)> checkIsland, TSharedPtr<FVNGraph> Graph,  int width, int height, float lakeThreshold)
{
    Width = width;
    Height = height;
    inside = checkIsland;
    VNGraph = Graph;
    if (!Graph) return;

    BuildGraph();
    AssignCornerElevations();
    AssignOceanCoastAndLand(lakeThreshold);
    RedistributeElevations();

    AssignPolygonElevations();

    // Determine downslope paths.
    CalculateDownslopes();

    // Determine watersheds: for every corner, where does it flow
    // out into the ocean? 
    CalculateWatersheds();

    // Create rivers.
    CreateRivers();

    // Determine moisture at corners, starting at rivers
    // and lakes, but not oceans. Then redistribute
    // moisture to cover the entire range evenly from 0.0
    // to 1.0. Then assign polygon moisture as the average
    // of the corner moisture.
    AssignCornerMoisture();
    RedistributeMoisture();
    AssignPolygonMoisture();

    for (auto& p : centers) p.biome = GetBiome(p);
}

const FNMCenter* FNMGraph::GetCenterAtLocation(FVector Location) const
{
    if (!VNGraph) return nullptr;
    for (int i = 0; i < centers.Num(); ++i)
    {
        if (VNGraph->PointInside(i, Location.X, Location.Y))
            return &centers[i];
    }
    return nullptr;
}



int FNMGraph::lookupEdgeFromCenter(int pi, int ri)
{
    if (pi==ri||!centers.IsValidIndex(pi) || !centers.IsValidIndex(ri)) return -1;
    for (auto& ei : centers[pi].pvn->Edges)
    {
        if (edges[ei].pvn->CellId1==ri || edges[ei].pvn->CellId2 == ri) return ei;
    }
    return -1;
}

int FNMGraph::lookupEdgeFromCorner(int qi, int si)
{
    if (qi == si || !corners.IsValidIndex(qi) || !corners.IsValidIndex(si)) return -1;

    for (auto& ei : corners[qi].pvn->Edges)
    {
        if (edges[ei].pvn->StartCornerId == si || edges[ei].pvn->EndCornerId == si) return ei;
    }
    return -1;
}


void FNMGraph::BuildGraph()
{
    // Build graph data structure in 'edges', 'centers', 'corners',

    centers.SetNumZeroed(VNGraph->Cells.Num());
    corners.SetNumZeroed(VNGraph->Corners.Num());
    edges.SetNumZeroed(VNGraph->Edges.Num());

    
    for (int i = 0; i < centers.Num(); ++i)
    {
        centers[i].index = i;
        centers[i].pvn = &VNGraph->Cells[i];
        //VNGraph->Cells[i].ExData = &centers[i];
    }
    for (int i = 0; i < corners.Num(); ++i)
    {
        corners[i].index = i;
        corners[i].pvn = &VNGraph->Corners[i];
        //VNGraph->Corners[i].ExData = &corners[i];
    }
    for (int i = 0; i < edges.Num(); ++i)
    {
        edges[i].index = i;
        edges[i].pvn = &VNGraph->Edges[i];
        //VNGraph->Edges[i].ExData = &edges[i];
    }

    //


    /*
    double tl = double(Width+Height);
    double  br = 0;
    double bl = double(Width+Height);
    double tr = -bl;

    int itl = 0;
    int ibr = 0;

    int itr = 0;
    int ibl = 0;

    for (int i = 0; i < points.Num(); ++i)
    {
        const FVector& point = points[i];
        double vtlbr = point.X + point.Y; // tl->br
        double vtrbl = point.X - point.Y; // bl->tr

        if (vtlbr < tl)
        {
            tl = vtlbr;
            itl = i;
        }
        if (vtlbr > br)
        {
            br = vtlbr;
            ibr = i;
        }
        if (vtrbl < bl)
        {
            bl = vtrbl;
            ibl = i;
        }
        if (vtrbl > tr)
        {
            tr = vtrbl;
            itr = i;
        }
    }

    //VNCells[itl].Corners.Add(-1);
    //VNCells[ibr].Corners.Add(-2);
    //VNCells[ibl].Corners.Add(-3);
    //VNCells[ibr].Corners.Add(-4);

    // TODO: use edges to determine these
    //FNMCenter topLeft = centers.OrderBy(p = > p.point.X + p.point.Y).First();
    //AddCorner(topLeft, 0, 0);

    //FNMCenter bottomRight = centers.OrderByDescending(p = > p.point.X + p.point.Y).First();
    //AddCorner(bottomRight, Width, Height);

    //FNMCenter topRight = centers.OrderByDescending(p = > Width - p.point.X + p.point.Y).First();
    //AddCorner(topRight, 0, Height);

    //FNMCenter bottomLeft = centers.OrderByDescending(p = > p.point.X + Height - p.point.Y).First();
    //AddCorner(bottomLeft, Width, 0);

    // required for polygon fill
    // for ( auto &  center : centers) { center.corners.Sort(ClockwiseComparison(center)); }

    */
}



/*
void FNMGraph::AddCorner(FNMCenter& topLeft, int x, int y)
{
    if (topLeft.point.X != x || topLeft.point.Y != y)
        topLeft.corners.Add({ .ocean = true, .point =FVector2D(x, y) });
}
*/

/*
FNMCorner* FNMGraph::MakeCorner(FVector2D nullablePoint)
{
    // The Voronoi library generates multiple Point objects for
    // corners, and we need to canonicalize to one Corner object.
    // To make lookup fast, we keep an array of Points, bucketed by
    // x value, and then we only have to look at other Points in
    // nearby buckets. When we fail to find one, we'll create a new
    // Corner object.

    if (nullablePoint == null) return nullptr;

    FVector2D point = nullablePoint.Value;

    for (int i = (int)(point.X - 1); i <= (int)(point.X + 1); i++)
    {
        for (auto& kvp : _cornerMap)
        {
            if (kvp.Key == i)
            {
                float dx = point.X - kvp.Value.point.X;
                float dy = point.Y - kvp.Value.point.Y;
                if (dx * dx + dy * dy < 1e-6) return kvp.Value;
            }
        }
    }

    FNMCorner corner = { .index = corners.Num(), .point = point};
    corners.Add(corner);

    corner.border = point.X == 0 || point.X == Width || point.Y == 0 || point.Y == Height;

    _cornerMap.Emplace((int)(point.X), corner);

    return corner;
}
*/

void FNMGraph::AssignCornerElevations()
{
    // Determine elevations and water at Voronoi corners. By
    // construction, we have no local minima. This is important for
    // the downslope vectors later, which are used : the river
    // construction algorithm. Also by construction, inlets/bays
    // push low elevation areas inland, which means many rivers end
    // up flowing out through them. Also by construction, lakes
    // often end up on river paths because they don't raise the
    // elevation as much as other terrain does.

    //var q:Corner, s:Corner;

    for(auto& cor: corners) cor.water = !inside(cor.pvn->Position);

    // 
    TQueue<FNMCorner*> queue;

    for (auto& cor : corners)
    {
        // The edges of the map are elevation 0
        if (cor.border)
        {
            cor.elevation = 0;
            queue.Enqueue(&cor);
        }
        else
        {
            cor.elevation = 10000000000;// float.PositiveInfinity;
        }
    }

    // Traverse the graph and assign elevations to each point. As we
    // move away from the map border, increase the elevations. This
    // guarantees that rivers always have a way down to the coast by
    // going downhill (no local minima).
    while (!queue.IsEmpty())
    {
        FNMCorner* corp;
        queue.Dequeue(corp);
        FNMCorner& cor = *corp;

        for (auto &corsi : cor.pvn->Corners)
        {
            // Every step up is epsilon over water or 1 over land. The
            // number doesn't matter because we'll rescale the
            // elevations later.
            FNMCorner& cors = corners[corsi];

            float newElevation = 0.01f + cor.elevation;
            if (!cor.water && !cors.water)
            {
                newElevation += 1;
                if (_needsMoreRandomness)
                {
                    // HACK: the map looks nice because of randomness of
                    // points, randomness of rivers, and randomness of
                    // edges. Without random point selection, I needed to
                    // inject some more randomness to make maps look
                    // nicer. I'm doing it here, with elevations, but I
                    // think there must be a better way. This hack is only
                    // used with square/hexagon grids.
                    // 
                    newElevation += FMath::Rand();
                }
            }
            // If this point changed, we'll add it to the queue so
            // that we can process its neighbors too.
            if (newElevation < cors.elevation)
            {
                cors.elevation = newElevation;
                queue.Enqueue(&cors);
            }
        }
    }
}

void FNMGraph::AssignOceanCoastAndLand(float lakeThreshold)
{
    // Compute polygon attributes 'ocean' and 'water' based on the
    // corner attributes. Count the water corners per
    // polygon. Oceans are all polygons connected to the edge of the
    // map. : the first pass, mark the edges of the map as ocean;
    // : the second pass, mark any water-containing polygon
    // connected an ocean as ocean.
    TQueue<FNMCenter*> queue;
    //var p:Center, q:Corner, r:Center, numWater:int;

    for (auto& cen : centers)
    {
        int numWater = 0;
        for (auto& cori : cen.pvn->Corners)
        {
            FNMCorner& cor = corners[cori];
            if (cor.border)
            {
                cen.border = true;
                cen.ocean = true;
                cor.water = true;
                queue.Enqueue(&cen);
            }

            if (cor.water) numWater += 1;
        }

        cen.water = (cen.ocean || numWater >= cen.pvn->Corners.Num() * lakeThreshold);
    }

    while (!queue.IsEmpty())
    {
        FNMCenter* cenp;
        queue.Dequeue(cenp);


        auto& cen = *cenp;

        for (auto cenri : cen.pvn->Cells)
        {
            auto& cenr = centers[cenri];

            if (cenr.water && !cenr.ocean)
            {
                cenr.ocean = true;
                queue.Enqueue(&cenr);
            }
        }
    }

    // Set the polygon attribute 'coast' based on its neighbors. If
    // it has at least one ocean and at least one land neighbor,
    // then this is a coastal polygon.
    for (auto& cen: centers)
    {
        int numOcean = 0;
        int numLand = 0;
        for (auto cenri : cen.pvn->Cells)
        {
            auto& cenr = centers[cenri];
            numOcean += cenr.ocean ? 1 : 0;
            numLand += !cenr.water ? 1 : 0;
        }
        cen.coast = (numOcean > 0) && (numLand > 0);
    }

    // Set the corner attributes based on the computed polygon
    // attributes. If all polygons connected to this corner are
    // ocean, then it's ocean; if all are land, then it's land;
    // otherwise it's coast.
    for (FNMCorner& cor:corners)
    {
        int numOcean = 0;
        int numLand = 0;
        for (auto ceni : cor.pvn->Cells)
        {
            auto& cen = centers[ceni];
            numOcean += cen.ocean ? 1 : 0;
            numLand += !cen.water ? 1 : 0;
        }
        cor.ocean = (numOcean == cor.pvn->Cells.Num());
        cor.coast = (numOcean > 0) && (numLand > 0);
        cor.water = cor.border || ((numLand != cor.pvn->Cells.Num()) && !cor.coast);
    }
}

void FNMGraph::RedistributeElevations()
{
    // Change the overall distribution of elevations so that lower
    // elevations are more common than higher
    // elevations. Specifically, we want elevation X to have frequency
    // (1-X).  To do this we will sort the corners, then set each
    // corner to its desired elevation.

    TArray<int> locations = LandCorners();
    // SCALE_FACTOR increases the mountain area. At 1.0 the maximum
    // elevation barely shows up on the map, so we set it to 1.1.
    float SCALE_FACTOR = 1.1f;
    float sq_sf = FMath::Sqrt(SCALE_FACTOR);

    locations.Sort([&](int a, int b) {return corners[a].elevation >corners[b].elevation; });

    for (int i = 0; i < locations.Num(); i++)
    {
        // Let y(x) be the total area that we want at elevation <= x.
        // We want the higher elevations to occur less than lower
        // ones, and set the area to be y(x) = 1 - (1-x)^2.
        float y = (float)i / (locations.Num() - 1);
        // Now we have to solve for x, given the known y.
        //  *  y = 1 - (1-x)^2
        //  *  y = 1 - (1 - 2x + x^2)
        //  *  y = 2x - x^2
        //  *  x^2 - 2x + y = 0
        // From this we can use the quadratic equation to get:
        float x = sq_sf - sq_sf*FMath::Sqrt( (1 - y));
        if (x > 1.0) x = 1.0f;  // TODO: does this break downslopes?
        corners[locations[i]].elevation = x;
    }

    // Assign elevations to non-land corners
    for (auto& p : corners) if (p.ocean || p.coast) p.elevation = 0;
}

void FNMGraph::AssignPolygonElevations()
{
    // Polygon elevations are the average of their corners
    for(FNMCenter& cen:centers)
    {
        float sumElevation = 0.0f;
        for (auto& cori : cen.pvn->Corners)
        {
            sumElevation += corners[cori].elevation;
        }
        cen.elevation = sumElevation / cen.pvn->Corners.Num();
    }
}

void FNMGraph::CalculateDownslopes()
{
    // Calculate downslope pointers.  At every point, we point to the
    // point downstream from it, or to itself.  This is used for
    // generating rivers and watersheds.

    for(int qi=0;qi<corners.Num();++qi)
    {
        int ri = qi;
        for (auto& si : corners[qi].pvn->Corners)
        {
            if (corners[ si].elevation <= corners[ri].elevation) ri = si;
        }
        corners[qi].downslope = &corners[ri];
    }
}

void FNMGraph::CalculateWatersheds()
{
    // Calculate the watershed of every land point. The watershed is
    // the last downstream land point : the downslope graph. TODO:
    // watersheds are currently calculated on corners, but it'd be
    // more useful to compute them on polygon centers so that every
    // polygon can be marked as being : one watershed.

    // Initially the watershed pointer points downslope one step.    
    for (auto& q : corners)
    {
        q.watershed = &q;
        if (!q.ocean && !q.coast) q.watershed = q.downslope;
    }

    // Follow the downslope pointers to the coast. Limit to 100
    // iterations although most of the time with numPoints==2000 it
    // only takes 20 iterations because most points are not far from
    // a coast.  TODO: can run faster by looking at
    // p.watershed.watershed instead of p.downslope.watershed.
    for (int i = 0; i < 100; i++)
    {
        bool changed = false;
        for (auto& q : corners)
        {
            if (!q.ocean && !q.coast && !q.watershed->coast)
            {
                FNMCorner* r = q.downslope->watershed;
                if (!r->ocean) q.watershed = r;
                changed = true;
            }
        }
        if (!changed) break;
    }

    // How big is each watershed?
    for (auto& q : corners)
    {
        FNMCorner* r = q.watershed;
        r->watershed_size = 1 + r->watershed_size;
    }
}

void FNMGraph::CreateRivers()
{
    // Create rivers along edges. Pick a random corner point, then
    // move downslope. Mark the edges and corners as rivers.
    for (int i = 0; i < (Width + Height) / 4; i++)
    {
        FNMCorner* corp = &corners[FMath::RandRange(0, corners.Num() - 1)];
        if (corp->ocean || corp->elevation < 0.3 || corp->elevation > 0.9) continue;
        // Bias rivers to go west: if (corp.downslope.X > corp.X) continue;
        while (!corp->coast)
        {
            if (corp == corp->downslope) break;

            int ei = lookupEdgeFromCorner(corp->index, corp->downslope->index);
            edges[ei].river = edges[ei].river + 1;

            corp->river++;
            corp->downslope->river++;  // TODO: fix double count
            corp = corp->downslope;
        }
    }
}

void FNMGraph::AssignCornerMoisture()
{
    // Calculate moisture. Freshwater sources spread moisture: rivers
    // and lakes (not oceans). Saltwater sources have moisture but do
    // not spread it (we set it at the end, after propagation).

    TQueue<int> queue;
    // Fresh water
    for(int qi=0;qi<corners.Num();++qi)
    {
        FNMCorner& q = corners[qi];
        if ((q.water || q.river > 0) && !q.ocean)
        {
            q.moisture = q.river > 0 ? FMath::Min(3.0f, (0.2f * q.river)) : 1.0f;
            queue.Enqueue(qi);
        }
        else
        {
            q.moisture = 0;
        }
    }
    while (!queue.IsEmpty())
    {
        int qi;
        queue.Dequeue(qi);
        FNMCorner& q = corners[qi];

        for (auto ri : q.pvn->Corners)
        {
            FNMCorner& r = corners[ri];

            float newMoisture = q.moisture * 0.9f;
            if (newMoisture > r.moisture)
            {
                r.moisture = newMoisture;
                queue.Enqueue(ri);
            }
        }
    }
    // Salt water
    for (auto q : corners)
    {
        if (q.ocean || q.coast) q.moisture = 1.0f;
    }
}

void FNMGraph::AssignPolygonMoisture()
{
    // Polygon moisture is the average of the moisture at corners
    for (int pi=0;pi<centers.Num();++pi)
    {
        FNMCenter& p = centers[pi];
        float sumMoisture = 0.0f;
        for (auto& qi : p.pvn->Corners)
        {
            FNMCorner& q = corners[qi];
            if (q.moisture > 1.0) q.moisture = 1.0f;

            sumMoisture += q.moisture;
        }
        p.moisture = sumMoisture / p.pvn->Corners.Num();
    }
}

void FNMGraph::RedistributeMoisture()
{
    // Change the overall distribution of moisture to be evenly distributed.
    TArray<int> locations = LandCorners();
    locations.Sort([&](auto& a, auto& b) {return corners[a].moisture > corners[b].moisture; });

    for (int i = 0; i < locations.Num(); i++)
    {
        corners[locations[i]].moisture = (float)i / (locations.Num() - 1);
    }
}

ENMBiome FNMGraph::GetBiome(const FNMCenter& p)
{
    if (p.ocean)
    {
        return ENMBiome::Ocean;
    }
    else if (p.water)
    {
        if (p.elevation < 0.1) return ENMBiome::Marsh;
        if (p.elevation > 0.8) return ENMBiome::Ice;
        return ENMBiome::Lake;
    }
    else if (p.coast)
    {
        return ENMBiome::Beach;
    }
    else if (p.elevation > 0.8)
    {
        if (p.moisture > 0.50) return ENMBiome::Snow;
        else if (p.moisture > 0.33) return ENMBiome::Tundra;
        else if (p.moisture > 0.16) return ENMBiome::Bare;
        else return ENMBiome::Scorched;
    }
    else if (p.elevation > 0.6)
    {
        if (p.moisture > 0.66) return ENMBiome::Taiga;
        else if (p.moisture > 0.33) return ENMBiome::Shrubland;
        else return ENMBiome::TemperateDesert;
    }
    else if (p.elevation > 0.3)
    {
        if (p.moisture > 0.83) return ENMBiome::TemperateRainForest;
        else if (p.moisture > 0.50) return ENMBiome::TemperateDeciduousForest;
        else if (p.moisture > 0.16) return ENMBiome::Grassland;
        else return ENMBiome::TemperateDesert;
    }
    else
    {
        if (p.moisture > 0.66) return ENMBiome::TropicalRainForest;
        else if (p.moisture > 0.33) return ENMBiome::TropicalSeasonalForest;
        else if (p.moisture > 0.16) return ENMBiome::Grassland;
        else return ENMBiome::SubtropicalDesert;
    }
}

TArray<int> FNMGraph::LandCorners()
{
    TArray<int> ret;
    for(int pi=0;pi<corners.Num();++pi)
    {
        FNMCorner& p = corners[pi];
        if (!p.ocean && !p.coast)  ret.Add(pi);
    }
    return ret;
}

