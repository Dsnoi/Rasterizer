#include "SectionScanLine.h"


// 根据窗口来设定buffer大小
void SectionScanLine::setSize(int width, int height)
{
	if (width == this->width&&height == this->height) return;

	release();

	this->width = width;
	this->height = height;
	needUpdate = true;

	idBuffer = new int*[height];
	for (int i = 0; i < height; ++i)
	{
		idBuffer[i] = new int[width];
	}
}

void SectionScanLine::getSize(int& width, int& height)
{
	width = this->width;
	height = this->height;
}

void SectionScanLine::release()
{
	if (idBuffer != NULL)
	{
		for (int i = 0; i < height; ++i)
		{
			delete[] idBuffer[i];
			idBuffer[i] = NULL;
		}
	}
	delete[] idBuffer;
	idBuffer = NULL;
}
// 构建活化多边形表和活化边表
void SectionScanLine::build(std::vector<Model::MeshParameter>& meshes)
{
	polygonIDTable.clear();
	edgeTable.clear();
	edgeTable.resize(height);

	int faces_size = meshes.size();

	for (int i = 0; i < faces_size; ++i)
	{
		float min_y = 0xfffffff, max_y = -0xfffffff;

		const auto& face = meshes[i];
		SectionPolygon polygon;
		polygon.id = i;
		
		//构建分类边表
		auto& vertexIdx = face.Triangles;
		for (int j = 0, vsize = vertexIdx.size(); j < vsize; ++j)
		{
			//比较得到，边的上端点pt1
			Vector3 pt1 = { vertexIdx[j].Vertex[0].Position[0], vertexIdx[j].Vertex[0].Position[1], vertexIdx[j].Vertex[0].Position[2] };
			Vector3 pt2 = { vertexIdx[j].Vertex[1].Position[0], vertexIdx[j].Vertex[1].Position[1], vertexIdx[j].Vertex[1].Position[2] };
			if (pt1.y < pt2.y)std::swap(pt1, pt2);

			Edge edge;
			edge.dy = round(pt1.y) - round(pt2.y);
			//避免不必要的计算
			if (round(pt1.y) < 0)continue;
			//无法计算 - 1 / k
			if (edge.dy <= 0)continue;

			edge.x = pt1.x; 
			edge.id = polygon.id;
			edge.deltaX = -(pt1.x - pt2.x) / (pt1.y - pt2.y);
			
			edgeTable[round(pt1.y)].push_back(edge);

			min_y = min(pt2.y, min_y);
			max_y = max(pt1.y, max_y);

		}

		//构建分类多边形表
		polygon.dy = round(max_y) - round(min_y);
		Vector3 v = { vertexIdx[0].Vertex[0].Position[0], vertexIdx[0].Vertex[0].Position[1], vertexIdx[0].Vertex[0].Position[2] };
		polygon.a = vertexIdx[0].Vertex[0].Normal[0];
		polygon.b = vertexIdx[0].Vertex[0].Normal[0];
		polygon.c = vertexIdx[0].Vertex[0].Normal[0];
		polygon.d = -(polygon.a*v.x + polygon.b*v.y + polygon.c*v.z);
		polygon.inflag = false;
		polygonIDTable.push_back(polygon);

	}

}

//对活化多边形表排序的比较函数，递增
static bool edgeSortCmp(const ActiveEdge& lEdge, const ActiveEdge& rEdge)
{
	if (round(lEdge.x) < round(rEdge.x))return true;
	else if (round(lEdge.x) == round(rEdge.x))
	{
		if (round(lEdge.z) < round(rEdge.z))
			return true;
	}
	return false;
}

//把该多边形在oxy平面上的投影和扫描线相交的边加入到活化边表中
void SectionScanLine::addEdge(int y)
{
	// 三角形面片，一条扫描线只会与其中两条边相交，包括极点也算是两个交点
	if (edgeTable[y].size() == 0)
		return;
	// edgeTable是根据上端点的y来存放数据的，一定只有两条边
	for (list<Edge>::iterator it = edgeTable[y].begin(), end = edgeTable[y].end();
		it != end;it++)
	{
		SectionPolygon *active_polygon = &polygonIDTable[it->id];
		ActiveEdge active_edge;
		active_edge.x = it->x;
		active_edge.deltaX = it->deltaX;
		active_edge.dy = it->dy;
		active_edge.id = it->id;
		// Z轴方向的法向为0
		if (isEqualf(active_polygon->c,0))
		{
			active_edge.z = 0;
			active_edge.dzx = 0;
			active_edge.dzy = 0;
		}
		else
		{
			active_edge.z = -(active_polygon->d + 
				active_polygon->a*it->x + 
				active_polygon->b*y) / active_polygon->c;
			active_edge.dzx = -(active_polygon->a / active_polygon->c);
			active_edge.dzy = active_polygon->b / active_polygon->c;
		}

		ActiveEdgeTable.push_back(active_edge);
	}
}

// 区间扫描线算法本体
void SectionScanLine::start(std::vector<Model::MeshParameter>& meshes)
{
	clock_t t = clock();
	//if (!needUpdate)return idBuffer;
	// 如果图片位置变化了，那么就需要更新
	if (!needUpdate)return;

	build(meshes);
	
	// 活化多边形表
	ActiveEdgeTable.clear();

	//扫描线从上往下进行扫描
	for (int y = height - 1; y >= 0; --y)
	{
		
		// 各种数据结构初始化
		inPolygonList.clear();
		// buffer置空
		memset(idBuffer[y], -1, sizeof(int)*width);

		// 检查分类的多边形表，如果有新的多边形涉及该扫描线，则把它放入活化的多边形表中
		// 注意这里加的绝对都是新的
		addEdge(y);

		if (ActiveEdgeTable.size() % 2 == 1)
		{
			cout << "error num in the active edge table" << endl;
			//assert(1);
		}

		if (ActiveEdgeTable.size() == 0) {
			continue;
		}
		//cout << "y:" << y << " ";
		// 给活化表排序
		sort(ActiveEdgeTable.begin(), ActiveEdgeTable.end(), edgeSortCmp);
		
		// 遍历活化边表
		for (int ae = 0; ae < ActiveEdgeTable.size(); ++ae)
		{
			//取出活化边对
			ActiveEdge& edge1 = ActiveEdgeTable[ae];
			// 最后一个区间
			if (ae == ActiveEdgeTable.size() - 1) {
				polygonIDTable[edge1.id].inflag = false;
				inPolygonList.remove(edge1.id);
				break;
			}

			ActiveEdge& edge2 = ActiveEdgeTable[ae + 1];

			// 扫描线刚进入该多边形
			if (!polygonIDTable[edge1.id].inflag) {
				polygonIDTable[edge1.id].inflag = true;
				inPolygonList.push_back(edge1.id);
			}
			else {
				// 之前通过了该polygon，这次是出去了
				polygonIDTable[edge1.id].inflag = false;
				inPolygonList.remove(edge1.id);
			}
			// 两条边之间的交点容不下一个像素，跳过
			if (round(edge1.x) == round(edge2.x)) {
				// edge1过去了，
				continue;
			}

			int mid_x = round((edge1.x + edge2.x) / 2);

			// 遍历该扫描线的活化边表
			int polygon_id = -1;
			float Zvalue = -0xfffffff;
			float temZvalue;
			
			// 没有扫过一个多边形，跳过
			if (inPolygonList.size() == 0) 
				continue;
			
			// 当前只有一个polygon
			if (inPolygonList.size() == 1) {
				polygon_id = inPolygonList.front();
			}
			else if(inPolygonList.size() >= 2){
				list<int>::iterator it_beg = inPolygonList.begin();
				list<int>::iterator it_end = inPolygonList.end();

				for (; it_beg != it_end; ++it_beg) {
					SectionPolygon& active_polygon = polygonIDTable[*it_beg];
					if (isEqualf(active_polygon.c, 0)) {
						temZvalue = 0;
					}
					else {
						temZvalue = -(active_polygon.d + active_polygon.a*(mid_x)+
							active_polygon.b*y) / active_polygon.c;
					}
					// z = -(ax+by+d)/c
					if (Zvalue < temZvalue) {
						Zvalue = temZvalue;
						polygon_id = active_polygon.id;
					}
				}
			}

			// 记录该点要保留的颜色
			if (polygon_id >= 0) {
				//cout << polygon_id << " " << edge1.x << "," << edge2.x << endl;
				//cout << "y:" << y << " id:" << ActiveEdgeTable[ActiveEdgeTable.size() - 1].id <<
				//	" dy:" << ActiveEdgeTable[ActiveEdgeTable.size() - 1].dy << " x:"
				//	<< ActiveEdgeTable[ActiveEdgeTable.size() - 1].x << endl;
				for (int x = round(edge1.x), end = round(edge2.x); x < end; ++x)
				{
					idBuffer[y][x] = polygon_id;
				}
			}
		}

		// 更新(包括移除)活化边表的值
		vector<ActiveEdge> temActiveEdgeTable;
		for (int ae = 0; ae < ActiveEdgeTable.size(); ++ae) {
			ActiveEdge& edge = ActiveEdgeTable[ae];
			--edge.dy;
			if (edge.dy <= 0) {
				//cout << "edge out:" << ActiveEdgeTable[ae].id << endl;
				continue;
			}
			edge.x += edge.deltaX;
			edge.z += edge.dzx * edge.deltaX + edge.dzy;
			temActiveEdgeTable.push_back(edge);
		}
		ActiveEdgeTable.clear();
		ActiveEdgeTable.assign(temActiveEdgeTable.begin(), temActiveEdgeTable.end());	
		if (!inPolygonList.empty()) {
			cout << "inPolygonList error,check it" << endl;
		}
		inPolygonList.clear();

		//cout << "ActiveEdgeTable:" << ActiveEdgeTable.size() << endl;
	}

	needUpdate = false;
	cout << "SectionScanLine耗时:" << float((clock() - t)) << "ms" << endl;
}


