#ifndef __SectionScanLineZBuffer_H
#define __SectionScanLineZBuffer_H

#include <vector>
#include <list>
#include <algorithm>
#include <ctime>
#include <Application/Model.h>
#include <Math/Math.h>
#include "utils.h"


struct Edge
{
	float x;
	float deltaX;
	int dy;
	int id;
};

//活化边表
struct ActiveEdge
{
	float x;
	float deltaX;
	int dy;
	float z;
	float dzx;
	float dzy;
	int id;
};

//这里的多边形边表元素，不记录多边形颜色，通过id查询shader.model中face的color得到
struct SectionPolygon
{
	float a, b, c, d;
	int id;
	int dy;
	bool inflag;
};

class SectionScanLine
{
public:
	SectionScanLine() {};
	~SectionScanLine() 
	{ 
		release(); 
	};
	void setSize(int width, int height);
	void getSize(int & width, int & height);
	void start(std::vector<Model::MeshParameter>& meshes);
	
	int** idBuffer;
	bool needUpdate;


private:
	int width, height;

	std::vector<SectionPolygon> polygonIDTable;
	std::vector<std::list<Edge>> edgeTable;
	std::vector<ActiveEdge> ActiveEdgeTable;
	std::list<int> inPolygonList;

	void release();
	void build(std::vector<Model::MeshParameter>& meshes);
	void addEdge(int y);

};
#endif