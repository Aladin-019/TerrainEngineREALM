#include "VoxelGrid.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace
{
    REALM::Engine::Voxel CreateMaterialVoxel(REALM::Engine::CellMaterial material)
    {
        using REALM::Engine::CellMaterial;
        switch (material)
        {
        case CellMaterial::Soil:
        case CellMaterial::Sand:
        case CellMaterial::Rock:
        case CellMaterial::Water:
        case CellMaterial::Vegetation:
            break;
        default:
            throw std::invalid_argument("Unsupported brush material.");
        }

        const bool water = material == CellMaterial::Water;
        REALM::Engine::Voxel voxel;
        voxel.Material = material;
        voxel.IsVisible = true;
        voxel.IsSolid = !water;
        voxel.HasVolume = true;
        voxel.Density = 1.0f;
        voxel.Moisture = water ? 1.0f : 0.15f;
        voxel.Temperature = water ? 12.0f : 10.0f;
        voxel.Porosity = water ? 0.0f : 0.2f;
        voxel.Compressibility = water ? 0.8f : 0.15f;
        return voxel;
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

    bool VoxelGrid::TryGetSurfaceHeight(int x, int z, float& height) const
    {
        if (!InBounds(x, 0, z))
            return false;

        for (int y = height_ - 1; y >= 0; --y)
        {
            const Voxel& voxel = GetVoxel(x, y, z);
            if (voxel.IsVisible || voxel.IsSolid || voxel.HasVolume)
            {
                height = static_cast<float>(y + 1);
                return true;
            }
        }
        return false;
    }

    bool VoxelGrid::TryEraseSphere(float centerX, float centerZ, int radius)
    {
        return TryApplySphere(centerX, centerZ, radius, nullptr);
    }

    bool VoxelGrid::TryDepositSphere(float centerX, float centerZ, int radius, CellMaterial material)
    {
        const Voxel deposit = CreateMaterialVoxel(material);
        return TryApplySphere(centerX, centerZ, radius, &deposit);
    }

    bool VoxelGrid::TryApplySphere(float centerX, float centerZ, int radius, const Voxel* deposit)
    {
        if (!std::isfinite(centerX) || !std::isfinite(centerZ) ||
            centerX < 0.0f || centerX >= width_ || centerZ < 0.0f || centerZ >= depth_ ||
            radius < MinBrushRadius || radius > MaxBrushRadius)
            return false;

        float centerY = 0.0f;
        if (!TryGetSurfaceHeight(static_cast<int>(centerX), static_cast<int>(centerZ), centerY))
            return false;

        const int minX = std::max(0, static_cast<int>(std::floor(centerX - radius)));
        const int maxX = std::min(width_ - 1, static_cast<int>(std::floor(centerX + radius)));
        const int minY = std::max(0, static_cast<int>(std::floor(centerY - radius)));
        const int maxY = std::min(height_ - 1, static_cast<int>(std::floor(centerY + radius)));
        const int minZ = std::max(0, static_cast<int>(std::floor(centerZ - radius)));
        const int maxZ = std::min(depth_ - 1, static_cast<int>(std::floor(centerZ + radius)));
        const float radiusSquared = static_cast<float>(radius * radius);
        bool changed = false;
        for (int x = minX; x <= maxX; ++x)
            for (int z = minZ; z <= maxZ; ++z)
            {
                float columnHeight = 0.0f;
                if (deposit)
                    TryGetSurfaceHeight(x, z, columnHeight);
                for (int y = minY; y <= maxY; ++y)
                {
                    const float dx = x + 0.5f - centerX;
                    const float dy = y + 0.5f - centerY;
                    const float dz = z + 0.5f - centerZ;
                    if (dx * dx + dy * dy + dz * dz > radiusSquared)
                        continue;

                    Voxel& voxel = GetVoxel(x, y, z);
                    if (deposit)
                    {
                        // Only the upper hemisphere, above each column's pre-stroke surface.
                        if (dy < 0.0f || y < columnHeight || !CanMoveTo(x, y, z))
                            continue;
                        voxel = *deposit;
                    }
                    else
                    {
                        if (!voxel.IsVisible && !voxel.IsSolid && !voxel.HasVolume)
                            continue;
                        voxel = Voxel{};
                    }
                    changed = true;
                }
            }

        if (changed)
            MarkTerrainDirty();
        return changed;
    }

    void VoxelGrid::GenerateTerrain()
    {
        terrainDirty_ = true;

        std::fill(voxels_.begin(), voxels_.end(), Voxel{});
        const int slabThickness = std::min(3, height_);
        for (int x = 0; x < width_; ++x)
            for (int z = 0; z < depth_; ++z)
                for (int y = 0; y < slabThickness; ++y)
                    GetVoxel(x, y, z) = CreateMaterialVoxel(CellMaterial::Rock);
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
                    case CellMaterial::Sand:
                    case CellMaterial::Vegetation:
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
        UpdateSoil(x, y, z);
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