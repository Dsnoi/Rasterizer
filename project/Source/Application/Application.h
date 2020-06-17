#pragma once

//======================================================================================================
//
//======================================================================================================
#include <Renderer/FrameBuffer.h>
#include <Application/Model.h>
#include <Math/Math.h>

//======================================================================================================
//
//======================================================================================================
class Application
{
	std::vector<Model::MeshParameter>	_MeshParams;
	Matrix								_mView;
	Matrix								_mProj;
	Matrix								_mViewProj;
	fp32								_CameraDistance;
	Vector4								_CameraAngle;
	Vector4								_CameraTarget;

public:
	Application() {}
	~Application() {}

	bool OnInitialize();
	void OnFinalize();

	void OnUpdate(fp32 FrameTime);
	void OnRendering(FrameBuffer<uint32>& ColorBuffer, FrameBuffer<fp32>& DepthBuffer);
	void DrawPolygon(Vector3 t0, Vector3 t1, Vector3 t2, FrameBuffer<uint32>& ColorBuffer, FrameBuffer<fp32>& DepthBuffer);

	void OnLeftMouseDrag(int32 x, int32 y);
	void OnRightMouseDrag(int32 x, int32 y);
	void OnWheelMouseDrag(int32 x, int32 y);
	
};
