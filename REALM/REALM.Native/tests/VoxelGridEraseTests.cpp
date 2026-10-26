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
    world.GetVoxel(2, 2, 3) = rock;
    Check(world.TryAddSoilAboveSurface(3, 3), "Set up soil above rock");
    world.GetVoxel(3, 3, 3).VelocityX = 2.0f;
    world.GetVoxel(3, 3, 3).VelocityY = 3.0f;
    world.GetVoxel(3, 3, 3).VelocityZ = 4.0f;
    world.MarkTerrainClean();

    Check(world.TryEraseSurface(3, 3), "Erase should remove the top soil voxel");
    const Voxel& empty = world.GetVoxel(3, 3, 3);
    Check(!empty.IsVisible && !empty.IsSolid && !empty.HasVolume &&
        empty.Density == 0.0f && empty.Moisture == 0.0f &&
        empty.Temperature == 0.0f && empty.Porosity == 0.0f &&
        empty.Compressibility == 0.0f && empty.VelocityX == 0.0f &&
        empty.VelocityY == 0.0f && empty.VelocityZ == 0.0f,
        "Erasing must reset all voxel state");
    Check(world.IsTerrainDirty(), "Erasing must invalidate the terrain mesh");
    Check(world.IsSolid(3, 2, 3), "Only one surface voxel should be removed");
    Check(world.IsSolid(2, 2, 3), "Neighboring columns must be unchanged");
    Check(world.TryEraseSurface(3, 3) && !world.HasVoxel(3, 2, 3),
        "Repeated erasing should remove the exposed rock");
    world.MarkTerrainClean();
    Check(!world.TryEraseSurface(3, 3), "An empty column must reject erasing");
    Check(!world.TryEraseSurface(-1, 3) && !world.TryEraseSurface(8, 3) &&
        !world.TryEraseSurface(3, -1) && !world.TryEraseSurface(3, 8),
        "Out-of-bounds columns must reject erasing");
    Check(!world.IsTerrainDirty(), "Rejected erasing must not invalidate the mesh");

    world.GetVoxel(0, 0, 0) = rock;
    world.GetVoxel(7, 7, 7) = rock;
    Check(world.TryEraseSurface(0, 0) && !world.IsSolid(0, 0, 0),
        "The bottom voxel at a boundary must be erasable");
    Check(world.TryEraseSurface(7, 7) && !world.IsSolid(7, 7, 7),
        "The highest voxel at a boundary must be erasable");
    Check(world.TrySpawnWater(4, 4, 4), "Set up water for erasing");
    Check(world.TryEraseSurface(4, 4) && !world.HasVoxel(4, 4, 4),
        "Erase must remove water as well as solid materials");
    Check(world.TrySpawnWater(4, 4, 4), "The erased cell must be reusable");

    world.GetVoxel(5, 0, 5) = rock;
    Check(world.TryAddSoilAboveSurface(5, 5) && world.TryEraseSurface(5, 5),
        "Soil brushing and erasing should work together");
    world.UpdateSimulation(1.0f / 60.0f);
    Check(!world.HasVoxel(5, 1, 5), "Erased soil must not reappear during simulation");
    Check(world.TryAddSoilAboveSurface(5, 5), "Soil should be addable after erasing");
    std::cout << "Erase brush engine tests passed.\n";
}
