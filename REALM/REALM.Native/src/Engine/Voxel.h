#pragma once

namespace REALM::Engine
{
    enum class CellMaterial
    {
        Soil,
        Sand,
        Rock,
        Dirt,
        Water
    };

    struct Voxel
    {
        CellMaterial Material = CellMaterial::Soil;
        bool IsVisible = false;
        bool IsSolid = false;
        bool HasVolume = false;
        float Density = 0.0f;
        float Moisture = 0.0f;
        float Temperature = 0.0f;
        float Porosity = 0.0f;
        float Compressibility = 0.0f;
        float VelocityX = 0.0f;
        float VelocityY = 0.0f;
        float VelocityZ = 0.0f;
    };
}