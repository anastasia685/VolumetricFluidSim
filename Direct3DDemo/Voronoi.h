#pragma once
#include <array>

class Voronoi
{
public:
	static Voronoi& GetInstance();

	Voronoi(const Voronoi&) = delete;
	Voronoi(Voronoi&&) = delete;
	Voronoi& operator=(const Voronoi&) = delete;
	Voronoi& operator=(Voronoi&&) = delete;

	float Noise(float x, float y) const;

private:
	static constexpr int Poisson_count[256] = { 
		4,3,1,1,1,2,4,2,2,2,5,1,0,2,1,2,2,0,4,3,2,1,2,1,3,2,2,4,2,2,5,1,2,3,
		2,2,2,2,2,3,2,4,2,5,3,2,2,2,5,3,3,5,2,1,3,3,4,4,2,3,0,4,2,2,2,1,3,2,
		2,2,3,3,3,1,2,0,2,1,1,2,2,2,2,5,3,2,3,2,3,2,2,1,0,2,1,1,2,1,2,2,1,3,
		4,2,2,2,5,4,2,4,2,2,5,4,3,2,2,5,4,3,3,3,5,2,2,2,2,2,3,1,1,4,2,1,3,3,
		4,3,2,4,3,3,3,4,5,1,4,2,4,3,1,2,3,5,3,2,1,3,1,3,3,3,2,3,1,5,5,4,2,2,
		4,1,3,4,1,5,3,3,5,3,4,3,2,2,1,1,1,1,1,2,4,5,4,5,4,2,1,5,1,1,2,3,3,3,
		2,5,2,3,3,2,0,2,1,1,4,2,1,3,2,1,2,2,3,2,5,5,3,4,5,5,2,4,4,5,3,2,2,2,
		1,4,2,3,3,4,2,5,4,2,4,2,2,2,4,5,3,2 };

	std::array<int, 512> perm;

	Voronoi();

	static float noiseInternal(float x, float y, const std::array<int, 512>& perm);
};
