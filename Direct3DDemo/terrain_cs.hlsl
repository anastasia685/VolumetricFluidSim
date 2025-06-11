RWStructuredBuffer<float> gDisplacement : register(u0);
RWStructuredBuffer<float3> gNormal : register(u1);

cbuffer DisplacementParams : register(b0)
{
    float frequency;
    float amplitude;
    float lacunarity;
    float gain;
    float3 offset;
    int octaves;
}

// Permutation table (256 values repeated for seamless lookup)
static const int perm[512] =
{
    151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
    // Repeat the same 256 values to avoid needing modulo operations
    151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

float2 Gradient(int hash)
{
    hash = hash & 3;
    float2 grad = float2(
        (hash & 1) == 0 ? 1.0 : -1.0,
        (hash & 2) == 0 ? 1.0 : -1.0
    );
    return normalize(grad);
}

// Fade function: Smoothstep interpolation
float Fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}
// Derivative of Fade function
float dFade(float t)
{
    return 30.0 * t * t * (t * (t - 2.0) + 1.0);
}
void Perlin2D(float2 P, out float displacement, out float2 gradient)
{
    // Get grid cell coordinates
    int X = (int) floor(P.x) & 255;
    int Y = (int) floor(P.y) & 255;

    float2 Pf = P - floor(P);

    // Compute fade curves
    float2 f = float2(Fade(Pf.x), Fade(Pf.y));
    float2 df = float2(dFade(Pf.x), dFade(Pf.y));
    
    int A = perm[X] + Y;
    int B = perm[X + 1] + Y;
    
    float2 g00 = Gradient(perm[A]);
    float2 g10 = Gradient(perm[B]);
    float2 g01 = Gradient(perm[A + 1]);
    float2 g11 = Gradient(perm[B + 1]);
    
    float2 d00 = Pf - float2(0, 0);
    float2 d10 = Pf - float2(1, 0);
    float2 d01 = Pf - float2(0, 1);
    float2 d11 = Pf - float2(1, 1);
    
    float v00 = dot(g00, d00);
    float v10 = dot(g10, d10);
    float v01 = dot(g01, d01);
    float v11 = dot(g11, d11);
    
    // Interpolate noise value
    float nx0 = lerp(v00, v10, f.x);
    float nx1 = lerp(v01, v11, f.x);
    float nxy = lerp(nx0, nx1, f.y);

    displacement = nxy;
    
    // Compute partial derivatives using chain rule
    float2 dv00 = g00;
    float2 dv10 = g10;
    float2 dv01 = g01;
    float2 dv11 = g11;
    
    float2 dnx0 = lerp(dv00, dv10, f.x) + (v10 - v00) * df.x * float2(1, 0);
    float2 dnx1 = lerp(dv01, dv11, f.x) + (v11 - v01) * df.x * float2(1, 0);
    gradient = lerp(dnx0, dnx1, f.y) + (nx1 - nx0) * df.y * float2(0, 1);
}
void Perlin2DFBm(float2 P, out float displacement, out float2 gradient)
{
    float _frequency = frequency;
    float _amplitude = amplitude;
    
    displacement = 0;
    gradient = float2(0, 0);
    
    float weight = 1.0f;

    for (int i = 0; i < octaves; i++)
    {
        float _displacement;
        float2 _gradient;
        Perlin2D(P * _frequency + offset.xy, _displacement, _gradient); // actually offset is for xz coords, xy is just more conveniet to set from imgui
        
        displacement += _displacement * _amplitude * weight;
        gradient += _gradient * _amplitude * _frequency * weight;
        
        weight = 1 / (1 + 2 * length(gradient));
        
        _frequency *= lacunarity;
        _amplitude *= gain;
    }
}

[numthreads(8, 8, 1)] // subdivision factor
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 8;
    int dimension = 17;
    
    // this is for 2D
    int outputIndex = ((groupID.y * subdivision + groupThreadID.y) * dimension * subdivision) + (groupID.x * subdivision + groupThreadID.x);
    float2 positionCoords = (float2(groupID.xy) + float2(groupThreadID.xy) / subdivision) / dimension; // thread coordinate to 0-1 range
    
    float displacement;
    float2 gradient;
    Perlin2DFBm(positionCoords.xy, displacement, gradient);
    
    gDisplacement[outputIndex] = displacement;
    gNormal[outputIndex] = normalize(float3(-gradient.x, 1.0, -gradient.y));
}