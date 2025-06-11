#include "pch.h"
#include "Light.h"

Light::Light() :
	m_position(0, 0, 0),
	m_direction(0, -1, 0),
	m_color(1, 1, 1),
	m_intensity(20)
{
}
