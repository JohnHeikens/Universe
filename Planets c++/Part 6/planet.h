#include "include.h"
#pragma once
struct terrainface;//else meshholder wont know about terrainface
static const int maxres = 0x10;
//static const uint* const planeindices = getplaneindices(maxres * maxres);
struct meshholder
{
	const int maxres = 0x10;//the maximum resolution before there will be childs
	meshholder();
	meshholder(terrainface* upperparent, fp minx, fp miny, fp size);
	vec3* vertices = NULL;
	uint* indices = NULL;
	color* colors = NULL;
	terrainface* upperparent;
	meshholder** childs = NULL;
	fp minx, miny, size;
	vec3 middlepointonunitsphere;
	const int Spread = 4;//divide over 4x4 grid
	const int Spread2 = Spread * Spread;//divide over 16 cells
	int lastresolution;
	void Draw(GraphicsObject* g, mat4x4 transform, int maxlevel, fp precision, vec3 Camera, vec3 direction);
	void DrawChildMeshes(int res, mat4x4 transform, vec3 Camera, GraphicsObject* graphics, fp precision, int maxlevel, vec3 direction);
	inline void DeleteChilds() 
	{
		for (int i = 0; i < Spread2; i++) 
		{
			childs[i]->DeleteArrays();
			if (childs[i]->childs) 
			{
				childs[i]->DeleteChilds();
			}
		}
		delete[] childs;
		childs = NULL;
	}
	inline void DeleteArrays()
	{
		delete[] this->vertices;
		delete[] this->indices;
		delete[] this->colors;
		vertices = NULL;
		indices = NULL;
		colors = NULL;
	}
};
struct planet;//else terrainface wont know about planet
struct terrainface
{
	terrainface(planet* parent, vec3 axisup);
	meshholder* m;
	planet* parent;
	vec3 axisup, axisa, axisb;
};
struct biome 
{
	fp maxheight;
	color color;
};
struct planet
{
	LayerNoiseSimplex* noise;
	vec3 pos, speed;
	fp minradius, maxradius; 
	//will always be between maxradius and minradius
	int biomecount;
	biome* biomes;
	terrainface* terrainfaces[6];
	static planet* randplanet();
	void Draw(GraphicsObject* g, mat4x4 transform, int maxlevel, int pixelprecision, vec3 Camera, vec3 direction, int scrmaxsize);
	void Remove();
};
