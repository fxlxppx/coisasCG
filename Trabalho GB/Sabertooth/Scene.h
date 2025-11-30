#pragma once
#include <vector>
#include <string>
#include "Obj3D.h"
#include "Light.h"

struct Scene
{
    std::vector<Obj3D*> objects;
    std::vector<Light> lights;
};