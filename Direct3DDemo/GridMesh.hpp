#pragma once
#include "BaseMesh.hpp"
#include "Perlin.h"
#include "Simplex.h"
#include <iostream>
#include "string"

namespace GridMeshUtil
{
    enum DisplacementType
    {
        DiamondSquare,
        Perlin,
        Simplex
    };
    static std::string DisplacementTypeString(DisplacementType type)
    {
        switch (type)
        {
        case DiamondSquare:
            return "Diamond Square";
        case Perlin:
            return "Perlin";
        case Simplex:
            return "Simplex";
        default:
            return "";
        }
    }
}

using namespace DirectX;
using namespace GridMeshUtil;

template <typename VertexType>
class GridMesh : public BaseMesh<VertexType>
{
public:
	GridMesh(ID3D11Device* device, UINT width = 33, UINT height = 33);
	virtual ~GridMesh() = default;

	void InitBuffers(ID3D11Device* device);
    void Draw(ID3D11DeviceContext* deviceContext);


    DisplacementType GetDisplacementType() const { return m_displacementType; };
    float GetAmplitude() const { return m_amplitude; };
    float GetFrequency() const { return m_frequency; };
    float GetGain() const { return m_gain; };
    float GetLacunarity() const { return m_lacunarity; };
    UINT GetOctaves() const { return m_octaves; };

    void SetDisplacementType(DisplacementType type) { m_displacementType = type; };
    void SetAmplitude(float amplitude) { m_amplitude = amplitude; };
    void SetFrequency(float frequency) { m_frequency = frequency; };
    void SetGain(float gain) { m_gain = gain; };
    void SetLacunarity(float lacunarity) { m_lacunarity = lacunarity; };
    void SetOctaves(UINT octaves) { m_octaves = octaves; };

protected:
	void CalculateNormals(std::vector<VertexType>& vertices, std::vector<UINT>& indices);

private:
	DisplacementType m_displacementType;
	std::vector<std::vector<float>> m_heightMap;

	UINT m_width, m_height;

	// displacement props
	float m_amplitude, m_frequency, m_gain, m_lacunarity;
	UINT m_octaves;


	void ApplyDiamondSquare();
	void ApplyPerlin();
	void ApplySimplex();
};





template <typename T>
GridMesh<T>::GridMesh(ID3D11Device* device, UINT width, UINT height) :
    BaseMesh<T>(),
    m_heightMap(width, std::vector<float>(height, 0.0f)),
    m_displacementType(DisplacementType::Perlin),
    m_width(width),
    m_height(height),
    m_amplitude(5.f),
    m_frequency(0.07f),
    m_octaves(8),
    m_gain(0.8f),
    m_lacunarity(1.2f)
{
    InitBuffers(device);
}


template <typename VertexType>
void GridMesh<VertexType>::InitBuffers(ID3D11Device* device)
{
    /*switch (m_displacementType)
    {
    case DisplacementType::DiamondSquare:
    {
        ApplyDiamondSquare();
        break;
    }
    case DisplacementType::Perlin:
        ApplyPerlin();
        break;
    case DisplacementType::Simplex:
    default:
        ApplySimplex();
        break;
    }*/


    /*
    UINT vertexCount = m_width * m_height;
    UINT indexCount = (m_width - 1) * (m_height - 1) * 6;

    std::vector<VertexType> vertices(vertexCount);
    std::vector<UINT> indices(indexCount);

    int i = 0;
    for (UINT z = 0; z < m_height; z++)
    {
        for (UINT x = 0; x < m_width; x++)
        {
            //float displacement = m_heightMap[x][z];
            float displacement = 0;

            vertices[i].position = XMFLOAT3(static_cast<float>(x), displacement, static_cast<float>(z));

            //vertices[i].normal = XMFLOAT3(0, 1, 0);

            vertices[i].texcoord = XMFLOAT2(static_cast<float>(x) / (m_width - 1), static_cast<float>(z) / (m_height - 1));

            i++;
        }
    }

    int vert = 0, tris = 0;
    for (UINT z = 0; z < m_height - 1; z++)
    {
        for (UINT x = 0; x < m_width - 1; x++)
        {
            indices[tris + 0] = vert + 0;
            indices[tris + 1] = vert + m_width;
            indices[tris + 2] = vert + 1;
            indices[tris + 3] = vert + 1;
            indices[tris + 4] = vert + m_width;
            indices[tris + 5] = vert + m_width + 1;

            vert++;
            tris += 6;
        }
        vert++; // Prevents connecting last vertex of a row to the first vertex of the next row
    }*/


    UINT vertexCount = m_width * m_height;
    UINT indexCount = (m_width - 1) * (m_height - 1) * 4; // 4 indices per quad

    std::vector<VertexType> vertices(vertexCount);
    std::vector<UINT> indices(indexCount);

    // Create vertex buffer
    int i = 0;
    for (UINT z = 0; z < m_height; z++)
    {
        for (UINT x = 0; x < m_width; x++)
        {
            float displacement = 0.0f; // or heightmap lookup

            // displacement should be normalized based on max y as well

            vertices[i].position = XMFLOAT3(static_cast<float>(x) / (m_width - 1), displacement, static_cast<float>(z) / (m_height - 1));
            vertices[i].texcoord = XMFLOAT2(
                static_cast<float>(x) / (m_width - 1),
                static_cast<float>(z) / (m_height - 1)
            );

            i++;
        }
    }

    // Create index buffer for quads (as 4-control-point patches)
    int quads = 0;
    for (UINT z = 0; z < m_height - 1; z++)
    {
        for (UINT x = 0; x < m_width - 1; x++)
        {
            int bottomLeft = x + z * m_width;
            int bottomRight = (x + 1) + z * m_width;
            int topRight = (x + 1) + (z + 1) * m_width;
            int topLeft = x + (z + 1) * m_width;

            // clockwise winding order
            indices[quads + 0] = topLeft;
            indices[quads + 1] = topRight;
            indices[quads + 2] = bottomRight;
            indices[quads + 3] = bottomLeft;

            quads += 4;
        }
    }


    //CalculateNormals(vertices, indices);

    BaseMesh<VertexType>::CreateBuffers(device, vertices.data(), sizeof(VertexType), vertexCount, indices.data(), indexCount);
}

template<typename VertexType>
inline void GridMesh<VertexType>::Draw(ID3D11DeviceContext* deviceContext)
{
    BaseMesh<VertexType>::Draw(deviceContext, D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
}

template <typename VertexType>
void GridMesh<VertexType>::CalculateNormals(std::vector<VertexType>& vertices, std::vector<UINT>& indices)
{
    using namespace DirectX::SimpleMath;

    for (auto& vertex : vertices)
        vertex.normal = Vector3::Zero;

    // Compute face normals and accumulate them for each vertex
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        // Get the vertex indices for the triangle
        UINT i0 = indices[i];
        UINT i1 = indices[i + 1];
        UINT i2 = indices[i + 2];

        // Get the vertex positions
        Vector3 v0(vertices[i0].position.x, vertices[i0].position.y, vertices[i0].position.z);
        Vector3 v1(vertices[i1].position.x, vertices[i1].position.y, vertices[i1].position.z);
        Vector3 v2(vertices[i2].position.x, vertices[i2].position.y, vertices[i2].position.z);

        // Compute edge vectors
        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;

        // Compute face normal (cross product)
        Vector3 normal = edge1.Cross(edge2);
        normal.Normalize();

        // Accumulate normals for each vertex (smoothing)
        XMStoreFloat3(&vertices[i0].normal, XMVectorAdd(XMLoadFloat3(&vertices[i0].normal), XMLoadFloat3(&normal)));
        XMStoreFloat3(&vertices[i1].normal, XMVectorAdd(XMLoadFloat3(&vertices[i1].normal), XMLoadFloat3(&normal)));
        XMStoreFloat3(&vertices[i2].normal, XMVectorAdd(XMLoadFloat3(&vertices[i2].normal), XMLoadFloat3(&normal)));
    }

    // Normalize all accumulated normals
    for (auto& vertex : vertices)
        XMStoreFloat3(&vertex.normal, XMVector3Normalize(XMLoadFloat3(&vertex.normal)));
}

template <typename T>
void GridMesh<T>::ApplyDiamondSquare()
{
    UINT size = m_width;
    srand(static_cast<unsigned int>(time(nullptr)));

    //--- iteration 0
    m_heightMap[0][0] = (rand() % 1000) / 1000.0f;
    m_heightMap[0][size - 1] = (rand() % 1000) / 1000.0f;
    m_heightMap[size - 1][0] = (rand() % 1000) / 1000.0f;
    m_heightMap[size - 1][size - 1] = (rand() % 1000) / 1000.0f;

    int step = size - 1;
    float scale = m_amplitude * m_gain;
    //---

    while (step > 1)
    {
        int halfStep = step / 2;

        // diamond step
        for (int x = halfStep; x < size - 1; x += step)
        {
            for (int y = halfStep; y < size - 1; y += step)
            {
                float avg = (m_heightMap[x - halfStep][y - halfStep] +
                    m_heightMap[x - halfStep][y + halfStep] +
                    m_heightMap[x + halfStep][y - halfStep] +
                    m_heightMap[x + halfStep][y + halfStep]) / 4.f;

                m_heightMap[x][y] = avg + ((rand() % 1000) / 1000.0f - 0.5f) * scale;
            }
        }

        // square step
        for (int x = 0; x < size; x += halfStep)
        {
            for (int y = (x + halfStep) % step; y < size; y += step)
            {
                float sum = 0;
                int count = 0;

                if (x - halfStep >= 0)
                {
                    sum += m_heightMap[x - halfStep][y];
                    count++;
                }
                if (x + halfStep < size)
                {
                    sum += m_heightMap[x + halfStep][y];
                    count++;
                }
                if (y - halfStep >= 0)
                {
                    sum += m_heightMap[x][y - halfStep];
                    count++;
                }
                if (y + halfStep < size)
                {
                    sum += m_heightMap[x][y + halfStep];
                    count++;
                }

                m_heightMap[x][y] = (sum / count) + ((rand() % 1000) / 1000.0f - 0.5f) * scale;
            }
        }

        step /= 2.f;
        //scale /= 2.f;
        scale *= m_gain;
    }
}

template <typename T>
void GridMesh<T>::ApplyPerlin()
{
    float displacement, amplitude, frequency;

    for (int x = 0; x < m_width; x++)
    {
        for (int y = 0; y < m_height; y++)
        {
            displacement = 0;
            amplitude = m_amplitude;
            frequency = m_frequency;

            for (UINT i = 0; i < m_octaves; i++)
            {
                displacement += Perlin::GetInstance().Noise(x * frequency, y * frequency) * amplitude;

                amplitude *= m_gain;
                frequency *= m_lacunarity;
            }

            m_heightMap[x][y] = displacement;
        }
    }
}

template <typename T>
void GridMesh<T>::ApplySimplex()
{
    float displacement, amplitude, frequency;

    for (int x = 0; x < m_width; x++)
    {
        for (int y = 0; y < m_height; y++)
        {
            displacement = 0;
            amplitude = m_amplitude;
            frequency = m_frequency;

            for (UINT i = 0; i < m_octaves; i++)
            {
                displacement += Simplex::GetInstance().Noise(x * frequency, y * frequency) * amplitude;

                amplitude *= m_gain;
                frequency *= m_lacunarity;
            }

            m_heightMap[x][y] = displacement;
        }
    }
}