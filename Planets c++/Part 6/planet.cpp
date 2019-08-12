#include "planet.h"

planet* planet::randplanet()
{
	planet* p = new planet();
	p->speed = vec3(RANDFP - .5, RANDFP - .5, RANDFP - .5).normalized() * .1;//length .1
	p->pos = vec3(rand() % 0x100, rand() % 0x100, rand() % 0x100);
	fp weights[5] = { RANDFP,RANDFP ,RANDFP ,RANDFP ,RANDFP };
	p->minradius = RANDFP * 10 + 1;
	p->maxradius = p->minradius + RANDFP * 10;
	p->noise = new LayerNoiseSimplex(rand(), weights, 5, p->minradius, p->maxradius, RANDFP);
	for (int i = 0; i < 6; i++) 
	{
		terrainface* face = new terrainface(p, zup::directions[i]);
		p->terrainfaces[i] = face;
	}
	p->biomecount = (rand() % ((int)p->maxradius + 1)) + 1;
	p->biomes = new biome[p->biomecount];
	fp lastheight = p->minradius;
	for (int i = 0; i < p->biomecount; i++) 
	{
		fp weight = RANDFP;
		weight *= weight;
		fp height = lerp(lastheight, p->maxradius, weight);
		p->biomes[i] = biome{ height,RANDCOLOR };
		lastheight = height;
	}
	return p;
}

void planet::Draw(GraphicsObject* g, mat4x4 transform, int maxlevel, int pixelprecision, vec3 Camera, vec3 direction, int scrmaxsize)
{
	fp precision = scrmaxsize / pixelprecision * noise->OutputPlus;
	for (int i = 0; i < 6; i++)
	{
		terrainfaces[i]->m->Draw(g, transform, 10, precision, Camera, direction);
	}
}

void planet::Remove()
{
	delete noise;
	delete[] biomes;
	for (terrainface* t : terrainfaces) 
	{
		if (t->m->childs)t->m->DeleteChilds();
		t->m->DeleteArrays();
	}
}

terrainface::terrainface(planet* parent, vec3 axisup)
{
	this->parent = parent;
	this->axisup = axisup;
	this->axisa = vec3(axisup.y, axisup.z, axisup.x);
	this->axisb = vec3::cross(axisup, axisa);
	this->m = new meshholder(this, 0, 0, 1);
}

meshholder::meshholder()
{
}

meshholder::meshholder(terrainface* upperparent, fp minx, fp miny, fp size)
{
	this->upperparent = upperparent;
	this->minx = minx;
	this->miny = miny;
	this->size = size;
	this->childs = NULL;
	this->lastresolution = 0;
	vec3 MiddlepointOnUnitCube = upperparent->axisup * .5f + (minx + size * .5f - .5f) * upperparent->axisa + (miny + size * .5f - .5f) * upperparent->axisb;
	this->middlepointonunitsphere = MiddlepointOnUnitCube.normalized();
}

void meshholder::Draw(GraphicsObject* graphics, mat4x4 transform, int maxlevel, fp precision, vec3 Camera, vec3 direction)
{
	maxlevel--;
	const planet* parentplanet = upperparent->parent;
	const vec3 LocalUp = upperparent->axisup, AxisA = upperparent->axisa, AxisB = upperparent->axisb;
	//vec3 pos = transform.position - parentplanet->pos;
	fp radius = parentplanet->noise->OutputPlus;
	fp Middleelev = parentplanet->noise->Evaluate3d(middlepointonunitsphere);
	vec3 MiddlePoint = middlepointonunitsphere * Middleelev + parentplanet->pos;
	fp distance = (Camera - MiddlePoint).length();
	if (distance == 0)
		return;
	if (minx > 0)
	{

	}
	int resolution = FloorToBase((int)(precision / distance * size), 2);
	//resolution will be >0
	//#if false
	if (maxlevel > 0 && resolution > 16)//max 16x16=256
	{
		DrawChildMeshes(resolution, transform, Camera, graphics, precision, maxlevel,direction);
		return;
	}
	//#endif
	else if (maxlevel == 0)
	{

	}
	std::vector<vec3> v = {
		(LocalUp * .5f + (minx - .5f) * AxisA + (miny - .5f) * AxisB).normalized() * parentplanet->maxradius + parentplanet->pos,//00
		(LocalUp * .5f + (minx - .5f) * AxisA + ((miny + size) - .5f) * AxisB).normalized() * parentplanet->maxradius + parentplanet->pos,//01
		(LocalUp * .5f + ((minx + size) - .5f) * AxisA + (miny - .5f) * AxisB).normalized() * parentplanet->maxradius + parentplanet->pos,//10
		(LocalUp * .5f + ((minx + size) - .5f) * AxisA + ((miny + size) - .5f) * AxisB).normalized() * parentplanet->maxradius + parentplanet->pos//11
	};
	vec3 nearest = vec3::nearest(Camera, v);
	//int intersections = vec3.FindIntersections(parentplanet->pos, parentplanet->maxradius - Mathf.FloatEpsilon, nearest, Camera - nearest, out vec3 nearestpoint, out vec3 farthestpoint);
	//if (intersections > 0) return;//intersects with sphere
	int resolution1 = resolution + 1;
	if (lastresolution == resolution)
	{
		graphics->DrawTrianglesPlain((fp*)vertices, this->indices, this->colors, resolution * resolution * 2, resolution1 * resolution1, Camera, transform, direction);
		return;
	}
	// initialisation
	int cnt = resolution * resolution * 6;
	vec3* vertices = new vec3[resolution1 * resolution1];
	uint* indices = new uint[cnt];
	color* colors = new color[cnt / 3];
	int triIndex = 0;
	int i = 0;
	const fp wh = parentplanet->biomes[0].maxheight;
	const color watercolor = parentplanet->biomes[0].color;
	const biome* biomes = parentplanet->biomes;
	const int elevcount = parentplanet->biomecount;
	fp Scale = size / resolution;

	for (int y = 0; y < resolution1; y++)
	{
		for (int x = 0; x < resolution1; x++)
		{
			// Calculate Vertices
			vec2 percent = vec2(x * Scale + minx, y * Scale + miny);
			vec3 pointOnUnitCube = LocalUp * .5f + (percent.x - .5f) * AxisA + (percent.y - .5f) * AxisB;
			vec3 PointOnUnitSphere = pointOnUnitCube.normalized();
			fp elev = parentplanet->noise->Evaluate3d(PointOnUnitSphere);
			vertices[i] = PointOnUnitSphere * elev + parentplanet->pos;

			if (x != resolution && y != resolution)
			{
				color color = watercolor;
				if (elev > wh + floatepsilon)
				{
					for (int j = 0; j < elevcount; j++)
					{
						if (elev < biomes[j].maxheight + floatepsilon)
						{
							color = biomes[j].color;
							break;
						}
					}
				}
				// Calculate Colors
				colors[triIndex / 3] = color;
				colors[triIndex / 3 + 1] = color;

				// Calculate Triangles
				indices[triIndex] = i;
				indices[triIndex + 1] = i + resolution1 + 1;
				indices[triIndex + 2] = i + resolution1;
				indices[triIndex + 3] = i;
				indices[triIndex + 4] = i + 1;
				indices[triIndex + 5] = i + resolution1 + 1;

				triIndex += 6;
			}
			i++;
		}
	}
	//mesh.Clear();
	if (vertices) 
	{
		DeleteArrays();
		if (childs)//too far away to see precision
		{
			DeleteChilds();
		}
	}
	graphics->DrawTrianglesPlain((fp*)vertices, indices, colors, resolution * resolution * 2, resolution1 * resolution1, Camera, transform, direction);
	this->vertices = vertices;
	this->indices = indices;
	this->colors = colors;
	this->lastresolution = resolution;
}

void meshholder::DrawChildMeshes(int res, mat4x4 transform, vec3 Camera, GraphicsObject* graphics, fp precision, int maxlevel, vec3 direction)
{
	
	if (!childs)
	{
		fp step = size / Spread;
		childs = new meshholder * [Spread2];
		int k = 0;
		for (int j = 0; j < Spread; j++)
		{
			fp mny = miny + step * j;
			for (int i = 0; i < Spread; i++)
			{
				fp mnx = minx + step * i;
				childs[k] = new meshholder(upperparent, mnx, mny, step);
				k++;
			}
		}
	}
	for (int i = 0; i < Spread2; i++)
	{
		childs[i]->Draw(graphics,transform, maxlevel,precision, Camera, direction);
	}
}
