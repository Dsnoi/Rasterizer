#include <Application/Model.h>

//======================================================================================================
//
//======================================================================================================
Model::Model(const wchar_t* pFileName)
{
	_pManager = FbxManager::Create();
	if (_pManager == nullptr) return;

	_pIOSettings = FbxIOSettings::Create(_pManager, IOSROOT);
	_pManager->SetIOSettings(_pIOSettings);

	FbxString Path = FbxGetApplicationDirectory();
	_pManager->LoadPluginsDirectory(Path.Buffer());

	_pScene = FbxScene::Create(_pManager, "My Scene");
	if (_pScene == nullptr) return;

	FbxSystemUnit SysUnit = _pScene->GetGlobalSettings().GetSystemUnit();
	_ScaleFactor = (fp32)SysUnit.GetScaleFactor() / 100.0f;

	char FileNameUTF8[MAX_PATH] = "";
	WideCharToMultiByte(CP_UTF8, 0, pFileName, -1, FileNameUTF8, MAX_PATH, nullptr, nullptr);
	if (!LoadScene(_pManager, _pScene, FileNameUTF8)) return;

	wchar_t CurrentFolder[MAX_PATH] = L"";
	int32 Length = (int32)wcslen(pFileName);
	while ((pFileName[Length] != L'\\') && (pFileName[Length] != L'/'))
	{
		if (--Length < 0) break;
	}
	wcscpy_s(CurrentFolder, sizeof(CurrentFolder) / sizeof(wchar_t), pFileName);
	CurrentFolder[Length < 0 ? 0 : Length] = L'\0';

	FbxGeometryConverter Converter(_pManager);
	Converter.Triangulate(_pScene, true);
	Converter.SplitMeshesPerMaterial(_pScene, true);

	FbxAxisSystem SceneAxisSystem = _pScene->GetGlobalSettings().GetAxisSystem();
	if (SceneAxisSystem != FbxAxisSystem::MayaYUp)
	{
		FbxAxisSystem::MayaYUp.ConvertScene(_pScene);
	}

	LoadHierarchy(_pScene->GetRootNode());
}

//======================================================================================================
//
//======================================================================================================
Model::~Model()
{
	if (_pIOSettings)
	{
		_pIOSettings->Destroy();
		_pIOSettings = nullptr;
	}
	if (_pManager)
	{
		_pManager->Destroy();
		_pManager = nullptr;
	}
}

//======================================================================================================
//
//======================================================================================================
bool Model::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFileName)
{
	int32 FileMajor, FileMinor, FileRevision;
	int32 SDKMajor, SDKMinor, SDKRevision;
	bool bStatus;

	FbxManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);

	FbxImporter* pImporter = FbxImporter::Create(pManager, "");
	const bool bImportStatus = pImporter->Initialize(pFileName, -1, pManager->GetIOSettings());
	if (!bImportStatus)
	{
		return false;
	}

	pImporter->GetFileVersion(FileMajor, FileMinor, FileRevision);
	if (pImporter->IsFBX())
	{
		pManager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL, true);
		pManager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE, true);
		pManager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK, true);
		pManager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE, true);
		pManager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	bStatus = pImporter->Import(pScene);
	if (!bStatus && (pImporter->GetStatus().GetCode() == FbxStatus::ePasswordError))
	{
		return false;
	}

	pImporter->Destroy();

	return bStatus;
}

//======================================================================================
//
//======================================================================================
void Model::LoadHierarchy(FbxNode* pNode)
{
	auto pNodeAttribute = pNode->GetNodeAttribute();
	if (pNodeAttribute != nullptr)
	{
		FbxNodeAttribute::EType AttributeType = pNodeAttribute->GetAttributeType();
		switch (AttributeType)
		{
		case FbxNodeAttribute::eMesh:
			{
				FbxVector4 S = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
				FbxVector4 R = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
				FbxVector4 T = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
				FbxAMatrix mLocal = FbxAMatrix(T, R, S);

				auto* pMesh = (FbxMesh*)pNodeAttribute;
				_MeshArray.resize(_MeshArray.size() + 1);
				LoadMeshData(_MeshArray.back(), pMesh, mLocal);
			}
			break;
		}
	}

	const int32 ChildCount = pNode->GetChildCount();
	for (int32 i = 0; i < ChildCount; ++i)
	{
		LoadHierarchy(pNode->GetChild(i));
	}
}

//======================================================================================
//
//======================================================================================
void Model::LoadMeshData(MeshData& Mesh, FbxMesh* pMesh, const FbxAMatrix& mLocal)
{
	LoadGeometry(Mesh, pMesh, mLocal);
	LoadMaterialData(Mesh, pMesh);
	LoadMaterialConnection(Mesh, pMesh);
}

//======================================================================================
//
//======================================================================================
void Model::LoadGeometry(MeshData& Mesh, FbxMesh* pMesh, const FbxAMatrix& mLocal)
{
	const int32 PolygonCount = pMesh->GetPolygonCount();

	FbxGeometryElementVertexColor* pVertexColors = pMesh->GetElementVertexColorCount() > 0
		? pMesh->GetElementVertexColor(0)
		: nullptr;

	FbxArray<FbxVector4> Normals;
	FbxStringList UVSetName;
	FbxArray<FbxVector2> UVSets;
	pMesh->GenerateNormals();
	pMesh->GetPolygonVertexNormals(Normals);
	pMesh->GetUVSetNames(UVSetName);
	if (UVSetName.GetCount() > 0)
	{
		pMesh->GetPolygonVertexUVs(UVSetName.GetStringAt(0), UVSets);
	}

	Mesh.Polygons.resize(PolygonCount);
	int32 PolygonInVertexNo = 0;
	for (int32 iPolygon = 0; iPolygon < PolygonCount; ++iPolygon)
	{
		for (int32 iVertex = 0; iVertex < 3; ++iVertex)
		{
			const int32 PolygonVertexIndex = PolygonInVertexNo + iVertex;

			const int32 ControlPointIndex = pMesh->GetPolygonVertex(iPolygon, iVertex);
			FbxVector4 Position = pMesh->GetControlPointAt(ControlPointIndex);
			FbxVector4 Normal(0.0f, 0.0f, 0.0f, 0.0);
			FbxColor VertexColor(1.0f, 1.0f, 1.0f, 1.0f);
			FbxVector2 TexCoord(0.0f, 0.0f);

			if (pVertexColors != nullptr)
			{
				int32 VIndex = 0;
				switch (pVertexColors->GetMappingMode())
				{
				case FbxLayerElement::eByControlPoint:
					VIndex = ControlPointIndex;
					break;
				case FbxLayerElement::eByPolygonVertex:
					VIndex = PolygonVertexIndex;
					break;
				}

				if (pVertexColors->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					VIndex = pVertexColors->GetIndexArray().GetAt(VIndex);
				}

				VertexColor = pVertexColors->GetDirectArray().GetAt(VIndex);
			}

			if (Normals.GetCount() > 0)
			{
				Normal = Normals[PolygonVertexIndex];
				Normal.Normalize();
			}

			if (UVSets.GetCount() > 0)
			{
				TexCoord = UVSets[PolygonVertexIndex];
			}

			auto& Dst = Mesh.Polygons[iPolygon].Vertex[iVertex];
			Dst.Position[0] = (fp32)Position[0] * _ScaleFactor;
			Dst.Position[1] = (fp32)Position[1] * _ScaleFactor;
			Dst.Position[2] = (fp32)Position[2] * _ScaleFactor;
			Dst.Normal[0] = (fp32)Normal[0];
			Dst.Normal[1] = (fp32)Normal[1];
			Dst.Normal[2] = (fp32)Normal[2];
			Dst.Color[0] = (fp32)VertexColor[0];
			Dst.Color[1] = (fp32)VertexColor[1];
			Dst.Color[2] = (fp32)VertexColor[2];
			Dst.Color[3] = (fp32)VertexColor[3];
			Dst.TexCoord[0] = (fp32)(TexCoord[0]);
			Dst.TexCoord[1] = (fp32)(1.0 - TexCoord[1]);
		}

		PolygonInVertexNo += pMesh->GetPolygonSize(iPolygon);
	}
}

//======================================================================================
//
//======================================================================================
void Model::LoadMaterialConnection(MeshData& Mesh, FbxMesh* pMesh)
{
	const int32 ElementMaterialCount = pMesh->GetElementMaterialCount();

	const int32 PolygonCount = pMesh->GetPolygonCount();
	for (int32 i = 0; i < ElementMaterialCount; ++i)
	{
		FbxGeometryElementMaterial* pMaterialElement = pMesh->GetElementMaterial(i);
		for (int32 j = 0; j < PolygonCount; ++j)
		{
			const int32 MaterialId = pMaterialElement->GetIndexArray().GetAt(j);
			if (MaterialId >= 0)
			{
				Mesh.Polygons[j].MaterialId = MaterialId;
			}
		}
	}
}

//======================================================================================
//
//======================================================================================
void Model::LoadMaterialData(MeshData& Mesh, FbxMesh* pMesh)
{
	struct GetFileName
	{
		std::string operator () (const char* pSource)
		{
			char FileNameBuffer[260] = "";
			strcpy_s(FileNameBuffer, sizeof(FileNameBuffer), pSource);
			const int32 NameLength = (int32)strlen(FileNameBuffer);
			int32 NameOffset = NameLength;
			while (FileNameBuffer[NameOffset] != '.')
			{
				if (--NameOffset < 0)
				{
					break;
				}
			}
			if (NameOffset > 0)
			{
				FileNameBuffer[NameOffset] = '\0';
			}
			NameOffset = NameLength;
			while ((FileNameBuffer[NameOffset] != '\\') && (FileNameBuffer[NameOffset] != '/'))
			{
				if (--NameOffset < 0)
				{
					break;
				}
			}
			return FileNameBuffer + NameOffset + 1;
		}
	};

	const int32 MaterialCount = pMesh->GetNode()->GetMaterialCount();
	if (MaterialCount > 0)
	{
		FbxPropertyT<FbxDouble3> KFbxDouble3;
		FbxPropertyT<FbxDouble> KFbxDouble1;
		FbxColor Color;

		for (int32 i = 0; i < MaterialCount; ++i)
		{
			MaterialData Material = {
				L"", { 1.0f, 1.0f, 1.0f }, 1.0f,
			};

			FbxSurfaceMaterial* pMaterial = pMesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(i);
			if (pMaterial != nullptr)
			{
				for (int32 j = 0; j < FbxLayerElement::sTypeTextureCount; ++j)
				{
					FbxProperty Property = pMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[j]);
					if (Property.IsValid())
					{
						int32 TextureCount = Property.GetSrcObjectCount<FbxTexture>();
						for (int32 k = 0; k < TextureCount; ++k)
						{
							FbxLayeredTexture* pLayeredTexture = Property.GetSrcObject<FbxLayeredTexture>(k);
							if (pLayeredTexture != nullptr)
							{
								int32 NbTextures = pLayeredTexture->GetSrcObjectCount<FbxTexture>();
								for (int32 l = 0; l < NbTextures; ++l)
								{
									FbxTexture* pTexture = pLayeredTexture->GetSrcObject<FbxTexture>(l);
									if (pTexture->Is<FbxFileTexture>())
									{
										std::string FileName = reinterpret_cast<FbxFileTexture*>(pTexture)->GetFileName();
										FileName = GetFileName()(FileName.c_str());
										wchar_t FileNameUTF16[MAX_PATH];
										MultiByteToWideChar(CP_ACP, 0, FileName.c_str(), -1, FileNameUTF16, MAX_PATH);
										Material.TextureName = FileNameUTF16;
										goto FIND_MATERIAL;
									}
								}
							}
							else
							{
								FbxTexture* pTexture = Property.GetSrcObject<FbxTexture>(k);
								if (pTexture->Is<FbxFileTexture>())
								{
									std::string FileName = reinterpret_cast<FbxFileTexture*>(pTexture)->GetFileName();
									FileName = GetFileName()(FileName.c_str());
									wchar_t FileNameUTF16[MAX_PATH];
									MultiByteToWideChar(CP_ACP, 0, FileName.c_str(), -1, FileNameUTF16, MAX_PATH);
									Material.TextureName = FileNameUTF16;
									goto FIND_MATERIAL;
								}
							}
						}
					}
				}
			}

		FIND_MATERIAL:

			const FbxImplementation* pImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_HLSL);
			FbxString ImplemenationType = "HLSL";
			if (!pImplementation)
			{
				pImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_CGFX);
				ImplemenationType = "CGFX";
			}

			if (pImplementation)
			{
				const FbxBindingTable* lRootTable = pImplementation->GetRootTable();
				FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
				FbxString lTechniqueName = lRootTable->DescTAG.Get();
				const FbxBindingTable* lTable = pImplementation->GetRootTable();
				size_t EntryCount = lTable->GetEntryCount();
				for (int32 j = 0; j < (int32)EntryCount; ++j)
				{
					const FbxBindingTableEntry& Entry = lTable->GetEntry(j);
					const char* EntrySrcType = Entry.GetEntryType(true);
					FbxProperty FbxProp;
					FbxString Test = Entry.GetSource();
					if (strcmp(FbxPropertyEntryView::sEntryType, EntrySrcType) == 0)
					{
						FbxProp = pMaterial->FindPropertyHierarchical(Entry.GetSource());
						if (!FbxProp.IsValid())
						{
							FbxProp = pMaterial->RootProperty.FindHierarchical(Entry.GetSource());
						}
					}
					else
					if (strcmp(FbxConstantEntryView::sEntryType, EntrySrcType) == 0)
					{
						FbxProp = pImplementation->GetConstants().FindHierarchical(Entry.GetSource());
					}

					if (FbxProp.IsValid())
					{
						if (FbxProp.GetSrcObjectCount<FbxTexture>() > 0)
						{
							for (int32 k = 0; k < FbxProp.GetSrcObjectCount<FbxFileTexture>(); ++k)
							{
								FbxFileTexture* pTex = FbxProp.GetSrcObject<FbxFileTexture>(k);
								if (strstr(Test.Buffer(), "DiffuseTexture") != nullptr)
								{
									std::string FileName = GetFileName()(pTex->GetFileName());
									wchar_t FileNameUTF16[MAX_PATH];
									MultiByteToWideChar(CP_ACP, 0, FileName.c_str(), -1, FileNameUTF16, MAX_PATH);
									Material.TextureName = FileNameUTF16;
								}
							}
						}
					}
				}
			}
			else
			if (pMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
			{
				FbxSurfacePhong* pPhong = (FbxSurfacePhong*)pMaterial;

				KFbxDouble3 = pPhong->Diffuse;
				Material.Diffuse[0] = (fp32)KFbxDouble3.Get()[0];
				Material.Diffuse[1] = (fp32)KFbxDouble3.Get()[1];
				Material.Diffuse[2] = (fp32)KFbxDouble3.Get()[2];
			}
			else
			if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
			{
				FbxSurfaceLambert* pLambert = (FbxSurfaceLambert*)pMaterial;

				KFbxDouble3 = pLambert->Diffuse;
				Material.Diffuse[0] = (fp32)KFbxDouble3.Get()[0];
				Material.Diffuse[1] = (fp32)KFbxDouble3.Get()[1];
				Material.Diffuse[2] = (fp32)KFbxDouble3.Get()[2];
			}

			Mesh.Materials.push_back(Material);
		}
	}
}

//======================================================================================================
//
//======================================================================================================
void Model::GetMeshData(std::vector<MeshParameter>& Output)
{
	for (auto&& Mesh : _MeshArray)
	{
		if (Mesh.Materials.empty())
		{
			if (!Mesh.Polygons.empty())
			{
				MeshParameter Param;
				Param.Texture = L"";

				for (auto& Poly : Mesh.Polygons)
				{
					MeshParameter::Triangle Tri;
					Tri.Vertex[0] = Poly.Vertex[0];
					Tri.Vertex[1] = Poly.Vertex[1];
					Tri.Vertex[2] = Poly.Vertex[2];
					Param.Triangles.push_back(Tri);
				}

				Output.push_back(Param);
			}
		}
		else
		{
			if (!Mesh.Polygons.empty())
			{
				std::map<int32, std::vector<PolygonData>> MaterialPolygons;
				for (auto& Poly : Mesh.Polygons)
				{
					MaterialPolygons[Poly.MaterialId].push_back(Poly);
				}

				for (auto& Polys : MaterialPolygons)
				{
					auto& Material = Mesh.Materials[Polys.first];

					MeshParameter Param;
					Param.Texture = Material.TextureName;

					for (auto Poly : Polys.second)
					{
						MeshParameter::Triangle Tri;
						Tri.Vertex[0] = Poly.Vertex[0];
						Tri.Vertex[1] = Poly.Vertex[1];
						Tri.Vertex[2] = Poly.Vertex[2];
						Param.Triangles.push_back(Tri);
					}

					Output.push_back(Param);
				}
			}
		}
	}
}
