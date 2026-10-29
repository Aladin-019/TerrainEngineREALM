#pragma once

#include <vector>
#include "Voxel.h"

namespace REALM::Engine
{
    class VoxelGrid
    {
    public:
        static constexpr int MinBrushRadius = 1;
        static constexpr int MaxBrushRadius = 16;

        VoxelGrid(int width, int height, int depth);

        Voxel& GetVoxel(int x, int y, int z);
        const Voxel& GetVoxel(int x, int y, int z) const;
        bool HasVoxel(int x, int y, int z) const;
        bool IsVisible(int x, int y, int z) const;
        bool IsSolid(int x, int y, int z) const;
        bool TryGetSurfaceHeight(int x, int z, float& height) const;
        bool TryEraseSphere(float centerX, float centerZ, int radius);
        bool TryDepositSphere(float centerX, float centerZ, int radius, CellMaterial material);
        bool IsTerrainDirty() const { return terrainDirty_; }
        void MarkTerrainDirty() { terrainDirty_ = true; }
        void MarkTerrainClean() { terrainDirty_ = false; }
        void GenerateTerrain();
        void UpdateSimulation(float deltaTime);
        void UpdateSoil(int x, int y, int z);
        void UpdateRock(int x, int y, int z);
        void UpdateWater(int x, int y, int z);

        int Width() const { return width_; }
        int Height() const { return height_; }
        int Depth() const { return depth_; }

    private:
        bool TryApplySphere(float centerX, float centerZ, int radius, const Voxel* deposit);
        bool InBounds(int x, int y, int z) const;
        bool CanMoveTo(int x, int y, int z) const;
        void MoveVoxel(int fromX, int fromY, int fromZ, int toX, int toY, int toZ);

        int width_;
        int height_;
        int depth_;
        bool terrainDirty_ = true;
        std::vector<Voxel> voxels_;
    };
}