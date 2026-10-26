#include "Engine/VoxelGrid.h"

#include <cstdlib>
#include <iostream>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    using namespace REALM::Engine;
    VoxelGrid world(8, 8, 8);
    for (int x = 0; x < world.Width(); ++x)
        for (int y = 0; y < world.Height(); ++y)
            for (int z = 0; z < world.Depth(); ++z)
                world.GetVoxel(x, y, z) = Voxel{};

    Voxel rock;
    rock.Material = CellMaterial::Rock;
    rock.IsVisible = true;
    rock.IsSolid = true;
    rock.HasVolume = true;
    world.GetVoxel(3, 2, 3) = rock;
    world.MarkTerrainClean();

    Check(world.TryAddSoilAboveSurface(3, 3), "Soil should be added above the surface");
    const Voxel& soil = world.GetVoxel(3, 3, 3);
    Check(soil.Material == CellMaterial::Soil && soil.IsVisible && soil.IsSolid &&
        soil.HasVolume && soil.Density == 1.0f && soil.VelocityX == 0.0f &&
        soil.VelocityY == 0.0f && soil.VelocityZ == 0.0f,
        "Added soil should have initialized simulation properties");
    Check(world.IsTerrainDirty(), "Adding soil must invalidate the terrain mesh");
    Check(world.GetVoxel(3, 2, 3).Material == CellMaterial::Rock,
        "The existing surface material must not be replaced");
    Check(!world.HasVoxel(3, 1, 3), "The brush must not fill cavities below the surface");
    Check(world.TryAddSoilAboveSurface(3, 3) && world.HasVoxel(3, 4, 3),
        "Repeated applications should build upward");
    Check(!world.HasVoxel(2, 3, 3), "Neighboring columns must be unchanged");

    world.GetVoxel(0, 0, 0) = rock;
    Check(world.TryAddSoilAboveSurface(0, 0), "Boundary columns should be brushable");
    world.GetVoxel(3, 7, 3) = rock;
    world.MarkTerrainClean();
    Check(!world.TryAddSoilAboveSurface(3, 3), "Full columns must reject deposition");
    Check(!world.TryAddSoilAboveSurface(-1, 3) &&
        !world.TryAddSoilAboveSurface(8, 3) &&
        !world.TryAddSoilAboveSurface(3, -1) &&
        !world.TryAddSoilAboveSurface(3, 8), "Out-of-bounds columns must be rejected");
    Check(!world.TryAddSoilAboveSurface(7, 7), "Empty columns have no surface to brush");
    Check(!world.IsTerrainDirty(), "Rejected deposition must not invalidate the mesh");

    Check(world.TrySpawnWater(5, 2, 5), "Existing water spawning should still work");
    Check(world.TryAddSoilAboveSurface(5, 5) && world.IsSolid(5, 3, 5),
        "Soil should be added above occupied water, not replace it");
    Check(world.GetVoxel(5, 2, 5).Material == CellMaterial::Water,
        "Brushing must preserve existing water");

    std::cout << "Soil brush engine tests passed.\n";
}