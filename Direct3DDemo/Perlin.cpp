#include "Perlin.h"

Perlin::Perlin()
{
	for (int i = 0; i < 256; ++i) {
		perm[i] = i;
	}
	for (int i = 0; i < 256; ++i) {
		perm[256 + i] = perm[i];
	}
}

Perlin& Perlin::GetInstance()
{
	static Perlin instance;
	return instance;
}

float Perlin::Noise(float x, float y) const
{
	return noiseInternal(x, y, perm);
}
float Perlin::Noise(float x, float y, float z) const
{
	return noiseInternal(x, y, z, perm);
}
float Perlin::noiseInternal(float x, float y, const std::array<int, 512>& perm)
{
	int X = fastfloor(x);
	int Y = fastfloor(y);

	x -= X;
	y -= Y;

	X = X & 255;
	Y = Y & 255;

	int g00 = perm[X + perm[Y]] % 8;
	int g01 = perm[X + perm[Y + 1]] % 8;
	int g10 = perm[X + 1 + perm[Y]] % 8;
	int g11 = perm[X + 1 + perm[Y + 1]] % 8;

	double n00 = dot(grad2[g00], x, y);
	double n10 = dot(grad2[g10], x - 1, y);
	double n01 = dot(grad2[g01], x, y - 1);
	double n11 = dot(grad2[g11], x - 1, y - 1);

	double u = fade(x);
	double v = fade(y);

	double nx0 = mix(n00, n10, u);
	double nx1 = mix(n01, n11, u);

	return mix(nx0, nx1, v);
}

float Perlin::noiseInternal(float x, float y, float z, const std::array<int, 512>& perm)
{
	int X = fastfloor(x);
	int Y = fastfloor(y);
	int Z = fastfloor(z);

	x = x - X;
	y = y - Y;
	z = z - Z;

	X = X & 255;
	Y = Y & 255;
	Z = Z & 255;

	int gi000 = perm[X + perm[Y + perm[Z]]] % 12;
	int gi001 = perm[X + perm[Y + perm[Z + 1]]] % 12;
	int gi010 = perm[X + perm[Y + 1 + perm[Z]]] % 12;
	int gi011 = perm[X + perm[Y + 1 + perm[Z + 1]]] % 12;
	int gi100 = perm[X + 1 + perm[Y + perm[Z]]] % 12;
	int gi101 = perm[X + 1 + perm[Y + perm[Z + 1]]] % 12;
	int gi110 = perm[X + 1 + perm[Y + 1 + perm[Z]]] % 12;
	int gi111 = perm[X + 1 + perm[Y + 1 + perm[Z + 1]]] % 12;

	double n000 = dot(grad3[gi000], x, y, z);
	double n100 = dot(grad3[gi100], x - 1, y, z);
	double n010 = dot(grad3[gi010], x, y - 1, z);
	double n110 = dot(grad3[gi110], x - 1, y - 1, z);
	double n001 = dot(grad3[gi001], x, y, z - 1);
	double n101 = dot(grad3[gi101], x - 1, y, z - 1);
	double n011 = dot(grad3[gi011], x, y - 1, z - 1);
	double n111 = dot(grad3[gi111], x - 1, y - 1, z - 1);

	double u = fade(x);
	double v = fade(y);
	double w = fade(z);

	double nx00 = mix(n000, n100, u);
	double nx01 = mix(n001, n101, u);
	double nx10 = mix(n010, n110, u);
	double nx11 = mix(n011, n111, u);

	double nxy0 = mix(nx00, nx10, v);
	double nxy1 = mix(nx01, nx11, v);

	double nxyz = mix(nxy0, nxy1, w);

	return nxyz;
}

int Perlin::fastfloor(float x)
{
	return x > 0 ? (int)x : (int)x - 1;
}

float Perlin::dot(const int* g, float x, float y)
{
	return g[0] * x + g[1] * y;
}

float Perlin::dot(const int* g, float x, float y, float z)
{
	return g[0] * x + g[1] * y + g[2] * z;
}

float Perlin::mix(float a, float b, float t)
{
	return (1 - t) * a + t * b;
}

float Perlin::fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}