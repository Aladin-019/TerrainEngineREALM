#include "Engine/VoxelGrid.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    using namespace REALM::Engine;

    constexpr CellMaterial Materials[] = {CellMaterial::Soil, CellMaterial::Water,
        CellMaterial::Rock, CellMaterial::Sand, CellMaterial::Vegetation};

    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    int CheckDeposit(CellMaterial material, int radius, float centerX, float centerZ)
    {
        VoxelGrid world(16, 16, 16);
        // A cavity and an overhang must not be filled by the material brush.
        world.GetVoxel(8, 1, 8) = Voxel{};
        world.GetVoxel(9, 7, 8) = world.GetVoxel(0, 0, 0);
        const VoxelGrid before = world;
        float centerY = 0.0f;
        Check(world.TryGetSurfaceHeight(static_cast<int>(centerX),
            static_cast<int>(centerZ), centerY), "The brush must have a surface target");
        world.MarkTerrainClean();
        Check(world.TryDepositSphere(centerX, centerZ, radius, material),
            "Each supported material must be deposited");
        Check(world.IsTerrainDirty(), "Deposition must invalidate the terrain mesh");
        int added = 0;
        for (int x = 0; x < world.Width(); ++x)
            for (int z = 0; z < world.Depth(); ++z)
            {
                float columnHeight = 0.0f;
                before.TryGetSurfaceHeight(x, z, columnHeight);
                for (int y = 0; y < world.Height(); ++y)
                {
                    const float dx = x + 0.5f - centerX;
                    const float dy = y + 0.5f - centerY;
                    const float dz = z + 0.5f - centerZ;
                    const bool shouldAdd = !before.HasVoxel(x, y, z) &&
                        dy >= 0.0f && y >= columnHeight &&
                        dx * dx + dy * dy + dz * dz <= radius * radius;
                    const Voxel& voxel = world.GetVoxel(x, y, z);
                    Check(world.HasVoxel(x, y, z) == (before.HasVoxel(x, y, z) || shouldAdd),
                        "Deposit must match the empty, above-surface upper hemisphere exactly");
                    if (shouldAdd)
                    {
                        ++added;
                        const bool water = material == CellMaterial::Water;
                        Check(voxel.Material == material && voxel.HasVolume &&
                            voxel.IsSolid == !water && voxel.Density == 1.0f &&
                            voxel.Moisture == (water ? 1.0f : 0.15f) &&
                            voxel.Temperature == (water ? 12.0f : 10.0f) &&
                            voxel.Porosity == (water ? 0.0f : 0.2f) &&
                            voxel.Compressibility == (water ? 0.8f : 0.15f) &&
                            voxel.VelocityX == 0.0f && voxel.VelocityY == 0.0f &&
                            voxel.VelocityZ == 0.0f,
                            "Deposited material must have initialized simulation properties");
                    }
                    else if (before.HasVoxel(x, y, z))
                        Check(voxel.Material == before.GetVoxel(x, y, z).Material &&
                            voxel.IsSolid == before.GetVoxel(x, y, z).IsSolid,
                            "Existing terrain must never be replaced");
                }
            }
        return added;
    }
    void TestDepositionShapeAndMaterialProperties()
    {
        for (CellMaterial material : Materials)
            CheckDeposit(material, 3, 8.5f, 8.5f);
    }

    void TestRadiusOneDepositsOneVoxel()
    {
        for (CellMaterial material : Materials)
            Check(CheckDeposit(material, 1, 8.5f, 8.5f) == 1,
                "Radius one should add a single voxel above the surface");
    }

    void TestLargerRadiusDepositsMoreMaterial()
    {
        for (CellMaterial material : Materials)
        {
            const int small = CheckDeposit(material, 3, 8.5f, 8.5f);
            Check(CheckDeposit(material, 6, 8.5f, 8.5f) > small,
                "Increasing radius must increase deposition");
        }
    }

    void TestDepositionAtGridBoundaries()
    {
        for (CellMaterial material : Materials)
        {
            CheckDeposit(material, 3, 0.0f, 0.0f);
            CheckDeposit(material, VoxelGrid::MaxBrushRadius, 15.9f, 15.9f);
        }
    }

    void TestDepositedMaterialsSettle()
    {
        for (CellMaterial material : Materials)
        {
            VoxelGrid world(16, 16, 16);
            Check(world.TryDepositSphere(8.5f, 8.5f, 3, material), "Deposit before settling");

            for (int step = 0; step < 20; ++step)
                world.UpdateSimulation(1.0f / 60.0f);

            for (int x = 0; x < world.Width(); ++x)
                for (int z = 0; z < world.Depth(); ++z)
                    for (int y = 1; y < world.Height(); ++y)
                        if (world.GetVoxel(x, y, z).HasVolume)
                            Check(world.GetVoxel(x, y - 1, z).HasVolume,
                                "Deposited materials must settle rather than float above gaps");
        }
    }

    void TestInitialTerrainIsRockSlab()
    {
        VoxelGrid world(16, 16, 16);
        for (int x = 0; x < world.Width(); ++x)
            for (int y = 0; y < world.Height(); ++y)
                for (int z = 0; z < world.Depth(); ++z)
                    Check(world.HasVoxel(x, y, z) == (y < 3) &&
                        (y >= 3 || world.GetVoxel(x, y, z).Material == CellMaterial::Rock),
                        "Initial terrain must be exactly a three-layer rock slab");
    }

    void TestEmptyColumnRejectsDeposition()
    {
        VoxelGrid world(16, 16, 16);
        Check(world.TryEraseSphere(8.5f, 8.5f, 3) && !world.HasVoxel(8, 0, 8),
            "The starting rock slab must be erasable through the bottom");
        Check(!world.TryDepositSphere(8.5f, 8.5f, 3, CellMaterial::Soil),
            "An empty center column must not become a floating brush target");
    }

    void TestRepeatedDepositsFollowRaisedSurface()
    {
        VoxelGrid world(16, 16, 16);
        Check(world.TryDepositSphere(8.5f, 8.5f, 1, CellMaterial::Soil),
            "Deposit the first soil layer");
        Check(world.TryDepositSphere(8.5f, 8.5f, 1, CellMaterial::Sand),
            "Deposit sand above the soil layer");
        Check(world.GetVoxel(8, 3, 8).Material == CellMaterial::Soil &&
            world.GetVoxel(8, 4, 8).Material == CellMaterial::Sand,
            "Repeated strokes must follow the raised surface without replacing earlier deposits");
    }

    void TestRegenerationClearsEdits()
    {
        VoxelGrid world(16, 16, 16);
        Check(world.TryDepositSphere(8.5f, 8.5f, 1, CellMaterial::Soil),
            "Deposit material before regeneration");

        world.GenerateTerrain();

        Check(!world.HasVoxel(8, 3, 8), "Regeneration must clear previous edits");
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
            world.MarkTerrainClean();

            Check(!world.TryDepositSphere(testCase.x, testCase.z, testCase.radius, CellMaterial::Soil),
                testCase.name);
            Check(!world.IsTerrainDirty(), "Rejected deposits must leave the mesh clean");
        }
    }

    void TestUnsupportedMaterialThrows()
    {
        VoxelGrid world(16, 16, 16);
        bool threw = false;
        try
        {
            world.TryDepositSphere(8.5f, 8.5f, 3, static_cast<CellMaterial>(99));
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }
        Check(threw, "An unsupported material must report an explicit error");
    }

    void TestGridCeilingRejectsDeposition()
    {
        VoxelGrid world(16, 16, 16);
        world.GetVoxel(8, 15, 8) = world.GetVoxel(0, 0, 0);

        Check(!world.TryDepositSphere(8.5f, 8.5f, 3, CellMaterial::Water),
            "A target at the grid ceiling has no upper hemisphere space");
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
    RunTest("Deposition shape and material properties", TestDepositionShapeAndMaterialProperties);
    RunTest("Radius one deposits one voxel", TestRadiusOneDepositsOneVoxel);
    RunTest("Larger radius deposits more material", TestLargerRadiusDepositsMoreMaterial);
    RunTest("Deposition at grid boundaries", TestDepositionAtGridBoundaries);
    RunTest("Deposited materials settle", TestDepositedMaterialsSettle);
    RunTest("Initial terrain is a rock slab", TestInitialTerrainIsRockSlab);
    RunTest("Empty column rejects deposition", TestEmptyColumnRejectsDeposition);
    RunTest("Repeated deposits follow the raised surface", TestRepeatedDepositsFollowRaisedSurface);
    RunTest("Regeneration clears edits", TestRegenerationClearsEdits);
    RunTest("Invalid coordinates and radii are rejected", TestInvalidCoordinatesAndRadiiAreRejected);
    RunTest("Unsupported material throws", TestUnsupportedMaterialThrows);
    RunTest("Grid ceiling rejects deposition", TestGridCeilingRejectsDeposition);
    std::cout << "Spherical material deposit tests passed.\n";
}