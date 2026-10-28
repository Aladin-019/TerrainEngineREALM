#include "Engine/VoxelGrid.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
    using namespace REALM::Engine;

    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    void FillFlatTerrain(VoxelGrid& world)
    {
        for (int x = 0; x < world.Width(); ++x)
            for (int y = 0; y < world.Height(); ++y)
                for (int z = 0; z < world.Depth(); ++z)
                {
                    Voxel voxel;
                    if (y < 9)
                    {
                        voxel.Material = CellMaterial::Rock;
                        voxel.IsVisible = true;
                        voxel.IsSolid = true;
                        voxel.HasVolume = true;
                        voxel.Density = 1.0f;
                    }
                    world.GetVoxel(x, y, z) = voxel;
                }
        world.MarkTerrainClean();
    }

    int CheckSphere(int radius, float centerX, float centerZ)
    {
        VoxelGrid world(16, 16, 16);
        FillFlatTerrain(world);
        float height = 0.0f;
        Check(world.TryGetSurfaceHeight(static_cast<int>(centerX),
            static_cast<int>(centerZ), height) && height == 9.0f,
            "The midpoint must be on top of the surface, not inside its voxel");
        Check(world.TryEraseSphere(centerX, centerZ, radius), "The sphere must remove terrain");
        Check(world.IsTerrainDirty(), "Sphere erasing must invalidate the mesh");
        int removed = 0;
        for (int x = 0; x < world.Width(); ++x)
            for (int y = 0; y < world.Height(); ++y)
                for (int z = 0; z < world.Depth(); ++z)
                {
                    const float dx = x + 0.5f - centerX;
                    const float dy = y + 0.5f - height;
                    const float dz = z + 0.5f - centerZ;
                    const bool inside = dx * dx + dy * dy + dz * dz <= radius * radius;
                    Check(world.HasVoxel(x, y, z) == (y < 9 && !inside),
                        "Only voxel centers within the sphere may be erased");
                    if (y < 9 && inside)
                        ++removed;
                }
        return removed;
    }
}

int main()
{
    using namespace REALM::Engine;
    const int smallCount = CheckSphere(1, 8.5f, 8.5f);
    const int largeCount = CheckSphere(3, 8.5f, 8.5f);
    Check(smallCount == 1, "Radius one at a column center should remove one surface voxel");
    Check(largeCount > smallCount, "Increasing radius must increase the erased volume");
    CheckSphere(4, 0.0f, 0.0f);
    CheckSphere(VoxelGrid::MaxEraseRadius, 15.9f, 15.9f);

    VoxelGrid world(16, 16, 16);
    FillFlatTerrain(world);
    Check(world.TryEraseSphere(8.5f, 8.5f, 3), "First scoop should succeed");
    float lowered = 0.0f;
    Check(world.TryGetSurfaceHeight(8, 8, lowered) && lowered == 6.0f,
        "The next midpoint must follow the newly lowered surface");
    Check(world.HasVoxel(8, 5, 8), "Terrain below the first sphere must remain");
    Check(world.TryEraseSphere(8.5f, 8.5f, 3) && !world.HasVoxel(8, 5, 8),
        "Holding for another application should dig further");
    Check(world.TryGetSurfaceHeight(8, 8, lowered) && lowered == 3.0f,
        "Repeated scoops should track the exposed surface");

    world.MarkTerrainClean();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Check(!world.TryEraseSphere(-0.1f, 8.5f, 3) &&
        !world.TryEraseSphere(16.0f, 8.5f, 3) &&
        !world.TryEraseSphere(8.5f, -0.1f, 3) &&
        !world.TryEraseSphere(8.5f, 16.0f, 3) &&
        !world.TryEraseSphere(nan, 8.5f, 3) &&
        !world.TryEraseSphere(8.5f, std::numeric_limits<float>::infinity(), 3) &&
        !world.TryEraseSphere(8.5f, 8.5f, 0) &&
        !world.TryEraseSphere(8.5f, 8.5f, VoxelGrid::MaxEraseRadius + 1),
        "Invalid targets and radii must be rejected safely");
    Check(!world.IsTerrainDirty(), "Rejected operations must not invalidate the mesh");

    for (int y = 0; y < world.Height(); ++y)
        world.GetVoxel(8, y, 8) = Voxel{};
    Check(!world.TryEraseSphere(8.5f, 8.5f, 3),
        "An empty center column must not erase surrounding terrain");
    Check(!world.TryGetSurfaceHeight(8, 8, lowered) &&
        !world.TryGetSurfaceHeight(-1, 8, lowered), "Missing surfaces must have no midpoint");

    FillFlatTerrain(world);
    for (int y = 9; y < world.Height(); ++y)
        Check(world.TrySpawnWater(8, y, 8), "Set up water above rock");
    Check(world.TryGetSurfaceHeight(8, 8, lowered) && lowered == 16.0f,
        "The midpoint must respect the top of water as well as rock");
    Check(world.TryEraseSphere(8.5f, 8.5f, 2) && !world.HasVoxel(8, 15, 8),
        "The sphere must erase water at the grid ceiling");
    Check(world.TrySpawnWater(8, 15, 8), "Erased cells must be reusable");
    std::cout << "Spherical erase brush tests passed.\n";
}