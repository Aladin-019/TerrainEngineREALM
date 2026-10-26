#include "VoxelGrid.h"

#include <cmath>
#include <algorithm>

namespace
{
    template <typename T>
    T Clamp(T value, T minValue, T maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }

    REALM::Engine::Voxel CreateWaterVoxel()
    {
        REALM::Engine::Voxel water;
        water.Material = REALM::Engine::CellMaterial::Water;
        water.IsVisible = true;
        water.IsSolid = false;
        water.HasVolume = true;
        water.Density = 1.0f;
        water.Moisture = 1.0f;
        water.Temperature = 12.0f;
        water.Porosity = 0.0f;
        water.Compressibility = 0.8f;
        return water;
    }
}

namespace REALM::Engine
{
    VoxelGrid::VoxelGrid(int width, int height, int depth)
        : width_(width), height_(height), depth_(depth), voxels_(static_cast<size_t>(width * height * depth))
    {
        GenerateTerrain();
    }

    Voxel& VoxelGrid::GetVoxel(int x, int y, int z)
    {
        return voxels_[static_cast<size_t>((z * height_ + y) * width_ + x)];
    }

    const Voxel& VoxelGrid::GetVoxel(int x, int y, int z) const
    {
        return voxels_[static_cast<size_t>((z * height_ + y) * width_ + x)];
    }

    bool VoxelGrid::HasVoxel(int x, int y, int z) const
    {
        if (!InBounds(x, y, z))
            return false;

        return GetVoxel(x, y, z).IsVisible;
    }

    bool VoxelGrid::IsVisible(int x, int y, int z) const
    {
        if (!InBounds(x, y, z))
            return false;

        return GetVoxel(x, y, z).IsVisible;
    }

    bool VoxelGrid::IsSolid(int x, int y, int z) const
    {
        if (!InBounds(x, y, z))
            return false;

        return GetVoxel(x, y, z).IsSolid;
    }

    bool VoxelGrid::TrySpawnWater(int x, int y, int z)
    {
        if (!CanMoveTo(x, y, z))
            return false;

        GetVoxel(x, y, z) = CreateWaterVoxel();
        MarkTerrainDirty();
        return true;
    }

    bool VoxelGrid::TryAddSoilAboveSurface(int x, int z)
    {
        if (!InBounds(x, 0, z))
            return false;

        for (int y = height_ - 1; y >= 0; --y)
        {
            const Voxel& surface = GetVoxel(x, y, z);
            if (!surface.IsVisible && !surface.IsSolid && !surface.HasVolume)
                continue;

            if (!CanMoveTo(x, y + 1, z))
                return false;

            Voxel soil;
            soil.Material = CellMaterial::Soil;
            soil.IsVisible = true;
            soil.IsSolid = true;
            soil.HasVolume = true;
            soil.Density = 1.0f;
            soil.Moisture = 0.15f;
            soil.Temperature = 10.0f;
            soil.Porosity = 0.2f;
            soil.Compressibility = 0.15f;
            GetVoxel(x, y + 1, z) = soil;
            MarkTerrainDirty();
            return true;
        }

        return false;
    }

    bool VoxelGrid::TryEraseSurface(int x, int z)
    {
        if (!InBounds(x, 0, z))
            return false;

        for (int y = height_ - 1; y >= 0; --y)
        {
            Voxel& surface = GetVoxel(x, y, z);
            if (!surface.IsVisible && !surface.IsSolid && !surface.HasVolume)
                continue;

            surface = Voxel{};
            MarkTerrainDirty();
            return true;
        }

        return false;
    }

    void VoxelGrid::GenerateTerrain()
    {
        terrainDirty_ = true;

        const int baseDepth = std::max(1, height_ / 3);
        const int centerX = width_ / 2;
        const int centerZ = depth_ / 2;

        for (int x = 0; x < width_; ++x)
        {
            for (int z = 0; z < depth_; ++z)
            {
                const float dx = static_cast<float>(x - centerX) / static_cast<float>(std::max(1, width_ / 2));
                const float dz = static_cast<float>(z - centerZ) / static_cast<float>(std::max(1, depth_ / 2));
                const float hill = 18.0f * std::exp(-(((x - (width_ * 0.28f)) * (x - (width_ * 0.28f))) + ((z - (depth_ * 0.5f)) * (z - (depth_ * 0.5f)))) / 180.0f);
                const float trough = -22.0f * std::exp(-(((x - (width_ * 0.72f)) * (x - (width_ * 0.72f))) + ((z - (depth_ * 0.5f)) * (z - (depth_ * 0.5f)))) / 90.0f);
                const float ridge = std::sin((x * 0.3f) + 1.0f) * 1.8f + std::cos((z * 0.4f) - 0.6f) * 1.8f;
                const int topY = Clamp(static_cast<int>(baseDepth + hill + trough + ridge), 1, height_ - 1);

                for (int y = 0; y <= topY; ++y)
                {
                    Voxel voxel;
                    voxel.Material = (y < baseDepth) ? CellMaterial::Rock : CellMaterial::Soil;
                    voxel.IsVisible = true;
                    voxel.IsSolid = true;
                    voxel.HasVolume = true;
                    voxel.Density = 1.0f;
                    voxel.Moisture = 0.15f;
                    voxel.Temperature = 10.0f;
                    voxel.Porosity = 0.2f;
                    voxel.Compressibility = 0.15f;
                    GetVoxel(x, y, z) = voxel;
                }
            }
        }

    }

    void VoxelGrid::UpdateSimulation(float deltaTime)
    {
        (void)deltaTime;

        for (int x = 0; x < width_; ++x)
        {
            for (int z = 0; z < depth_; ++z)
            {
                for (int y = 1; y < height_; ++y)
                {
                    Voxel& voxel = GetVoxel(x, y, z);

                    switch (voxel.Material)
                    {
                    case CellMaterial::Soil:
                        UpdateSoil(x, y, z);
                        break;

                    case CellMaterial::Rock:
                        UpdateRock(x, y, z);
                        break;

                    case CellMaterial::Water:
                        UpdateWater(x, y, z);
                        break;

                    default:
                        break;
                    }
                }
            }
        }
    }

    void VoxelGrid::UpdateSoil(int x, int y, int z)
    {
        Voxel& current = GetVoxel(x, y, z);
        if (!current.HasVolume)
            return;

        Voxel& lower = GetVoxel(x, y - 1, z);
        if (!lower.HasVolume)
        {
            MoveVoxel(x, y, z, x, y - 1, z);
        }
    }

    void VoxelGrid::UpdateRock(int x, int y, int z)
    {
        (void)x;
        (void)y;
        (void)z;
    }

    void VoxelGrid::UpdateWater(int x, int y, int z)
    {
        Voxel& current = GetVoxel(x, y, z);
        if (!current.HasVolume)
            return;

        if (CanMoveTo(x, y - 1, z))
        {
            MoveVoxel(x, y, z, x, y - 1, z);
            return;
        }

        if (CanMoveTo(x - 1, y, z))
        {
            MoveVoxel(x, y, z, x - 1, y, z);
            return;
        }

        if (CanMoveTo(x + 1, y, z))
        {
            MoveVoxel(x, y, z, x + 1, y, z);
            return;
        }

        if (CanMoveTo(x, y, z - 1))
        {
            MoveVoxel(x, y, z, x, y, z - 1);
            return;
        }

        if (CanMoveTo(x, y, z + 1))
        {
            MoveVoxel(x, y, z, x, y, z + 1);
        }
    }

    bool VoxelGrid::CanMoveTo(int x, int y, int z) const
    {
        if (!InBounds(x, y, z))
            return false;

        const Voxel& target = GetVoxel(x, y, z);
        return !target.IsVisible && !target.IsSolid && !target.HasVolume;
    }

    void VoxelGrid::MoveVoxel(int fromX, int fromY, int fromZ, int toX, int toY, int toZ)
    {
        if (!InBounds(fromX, fromY, fromZ) || !InBounds(toX, toY, toZ))
            return;

        if (!CanMoveTo(toX, toY, toZ))
            return;

        Voxel moved = GetVoxel(fromX, fromY, fromZ);
        Voxel empty;
        empty.Material = CellMaterial::Soil;
        empty.IsVisible = false;
        empty.IsSolid = false;
        empty.HasVolume = false;
        empty.Density = 0.0f;
        empty.Moisture = 0.0f;
        empty.Temperature = 0.0f;
        empty.Porosity = 0.0f;
        empty.Compressibility = 0.0f;

        GetVoxel(toX, toY, toZ) = moved;
        GetVoxel(fromX, fromY, fromZ) = empty;
        MarkTerrainDirty();
    }

    bool VoxelGrid::InBounds(int x, int y, int z) const
    {
        return x >= 0 && x < width_ && y >= 0 && y < height_ && z >= 0 && z < depth_;
    }
}