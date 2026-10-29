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
    void TestEraseShapeAndRadius()
    {
        const int smallCount = CheckSphere(1, 8.5f, 8.5f);
        const int largeCount = CheckSphere(3, 8.5f, 8.5f);
        Check(smallCount == 1, "Radius one at a column center should remove one surface voxel");
        Check(largeCount > smallCount, "Increasing radius must increase the erased volume");
    }

    void TestErasingAtGridBoundaries()
    {
        CheckSphere(4, 0.0f, 0.0f);
        CheckSphere(VoxelGrid::MaxBrushRadius, 15.9f, 15.9f);
    }

    void TestRepeatedScoopsFollowLoweredSurface()
    {
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
    }

    void TestInvalidCoordinatesAndRadiiAreRejected()
    {
        struct InvalidSphere
        {
            const char* name;
            float x;
            float z;
            int radius;
        };
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const InvalidSphere cases[] = {
            {"Negative X", -0.1f, 8.5f, 3},
            {"X outside grid", 16.0f, 8.5f, 3},
            {"Negative Z", 8.5f, -0.1f, 3},
            {"Z outside grid", 8.5f, 16.0f, 3},
            {"NaN coordinate", nan, 8.5f, 3},
            {"Infinite coordinate", 8.5f, std::numeric_limits<float>::infinity(), 3},
            {"Zero radius", 8.5f, 8.5f, 0},
            {"Radius above maximum", 8.5f, 8.5f, VoxelGrid::MaxBrushRadius + 1}
        };
        for (const auto& testCase : cases)
        {
            VoxelGrid world(16, 16, 16);
            FillFlatTerrain(world);

            Check(!world.TryEraseSphere(testCase.x, testCase.z, testCase.radius), testCase.name);
            Check(!world.IsTerrainDirty(), "Rejected operations must not invalidate the mesh");
        }
    }

    void TestEmptyColumnHasNoEraseTarget()
    {
        VoxelGrid world(16, 16, 16);
        FillFlatTerrain(world);
        for (int y = 0; y < world.Height(); ++y)
            world.GetVoxel(8, y, 8) = Voxel{};

        Check(!world.TryEraseSphere(8.5f, 8.5f, 3),
            "An empty center column must not erase surrounding terrain");
        float height = 0.0f;
        Check(!world.TryGetSurfaceHeight(8, 8, height) &&
            !world.TryGetSurfaceHeight(-1, 8, height), "Missing surfaces must have no midpoint");
    }

    void TestWaterAtGridCeilingCanBeErasedAndReplaced()
    {
        VoxelGrid world(16, 16, 16);
        FillFlatTerrain(world);
        for (int y = 9; y < world.Height(); ++y)
            Check(world.TryDepositSphere(8.5f, 8.5f, 1, CellMaterial::Water),
                "Deposit water above rock using the current brush");

        float height = 0.0f;
        Check(world.TryGetSurfaceHeight(8, 8, height) && height == 16.0f,
            "The midpoint must respect the top of water as well as rock");
        Check(world.TryEraseSphere(8.5f, 8.5f, 2) && !world.HasVoxel(8, 15, 8),
            "The sphere must erase water at the grid ceiling");
        Check(world.TryDepositSphere(8.5f, 8.5f, 1, CellMaterial::Water),
            "Replace the lower erased water cell");
        Check(world.TryDepositSphere(8.5f, 8.5f, 1, CellMaterial::Water) &&
            world.GetVoxel(8, 15, 8).Material == CellMaterial::Water,
            "Erased cells must be reusable by the material brush");
    }

    void TestErasingResetsEveryMaterialAndAllowsReuse()
    {
        const CellMaterial materials[] = {CellMaterial::Soil, CellMaterial::Sand,
            CellMaterial::Rock, CellMaterial::Water, CellMaterial::Vegetation};
        for (CellMaterial material : materials)
        {
            VoxelGrid world(16, 16, 16);
            Check(world.TryDepositSphere(8.5f, 8.5f, 1, material), "Deposit material before erasing");
            Voxel& deposited = world.GetVoxel(8, 3, 8);
            deposited.VelocityX = 2.0f;
            deposited.VelocityY = 3.0f;
            deposited.VelocityZ = 4.0f;
            world.MarkTerrainClean();

            Check(world.TryEraseSphere(8.5f, 8.5f, 1), "Erase each supported material");

            const Voxel& empty = world.GetVoxel(8, 3, 8);
            Check(!empty.IsVisible && !empty.IsSolid && !empty.HasVolume &&
                empty.Density == 0.0f && empty.Moisture == 0.0f &&
                empty.Temperature == 0.0f && empty.Porosity == 0.0f &&
                empty.Compressibility == 0.0f && empty.VelocityX == 0.0f &&
                empty.VelocityY == 0.0f && empty.VelocityZ == 0.0f,
                "Spherical erase must reset all voxel properties");
            Check(world.IsTerrainDirty(), "Erasing deposited material must invalidate the mesh");
            Check(world.IsSolid(8, 2, 8) && !world.HasVoxel(7, 3, 8),
                "Radius one must preserve the lower layer and neighboring columns");

            world.UpdateSimulation(1.0f / 60.0f);

            Check(!world.HasVoxel(8, 3, 8), "Erased material must not reappear during simulation");
            Check(world.TryDepositSphere(8.5f, 8.5f, 1, material),
                "Each erased material cell must be reusable");
        }
    }

    void RunTest(const char* name, void (*test)())
    {
        std::cout << "[ RUN  ] " << name << std::endl;
        test();
        std::cout << "[ PASS ] " << name << '\n';
    }
}

int main()
{
    RunTest("Erase shape and radius", TestEraseShapeAndRadius);
    RunTest("Erasing at grid boundaries", TestErasingAtGridBoundaries);
    RunTest("Repeated scoops follow the lowered surface", TestRepeatedScoopsFollowLoweredSurface);
    RunTest("Invalid coordinates and radii are rejected", TestInvalidCoordinatesAndRadiiAreRejected);
    RunTest("Empty column has no erase target", TestEmptyColumnHasNoEraseTarget);
    RunTest("Water at the grid ceiling can be erased and replaced", TestWaterAtGridCeilingCanBeErasedAndReplaced);
    RunTest("Erasing resets every material and allows reuse", TestErasingResetsEveryMaterialAndAllowsReuse);
    std::cout << "Spherical erase brush tests passed.\n";
}