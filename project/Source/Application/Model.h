#pragma once

//======================================================================================================
//
//======================================================================================================
class Model
{
	struct VertexData;
	struct PolygonData;
	struct MaterialData;
	struct SkinInfo;
	struct ControlPoint32WeightInfo;
	struct MeshData;
	struct NodeData;
	struct AnimationData;
	struct RootData;

	struct VertexData
	{
		fp32	Position[3];
		fp32	Normal[3];
		fp32	Color[4];
		fp32	TexCoord[2];
	};

	struct PolygonData
	{
		int32			MaterialId;
		VertexData		Vertex[3];
	};

	struct MaterialData
	{
		std::wstring	TextureName;
		fp32			Diffuse[3];
		fp32			Opacity;
	};

	struct MeshData
	{
		std::vector<PolygonData>	Polygons;
		std::vector<MaterialData>	Materials;
	};

public:
	struct MeshParameter
	{
		struct Triangle
		{
			VertexData Vertex[3];
		};
		std::vector<Triangle>	Triangles;
		std::wstring			Texture;
	};

private:
	FbxManager*					_pManager;
	FbxIOSettings*				_pIOSettings;
	FbxScene*					_pScene;
	fp32						_ScaleFactor;
	std::vector<MeshData>		_MeshArray;

private:
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFileName);
	void LoadHierarchy(FbxNode* pNode);
	void LoadMeshData(MeshData& Mesh, FbxMesh* pMesh, const FbxAMatrix& mLocal);
	void LoadGeometry(MeshData& Mesh, FbxMesh* pMesh, const FbxAMatrix& mLocal);
	void LoadMaterialData(MeshData& Mesh, FbxMesh* pMesh);
	void LoadMaterialConnection(MeshData& Mesh, FbxMesh* pMesh);

	void CreateDrawObject(MeshData& Mesh);

public:
	Model(const wchar_t* pFileName);
	~Model();

	void GetMeshData(std::vector<MeshParameter>& Output);
};
