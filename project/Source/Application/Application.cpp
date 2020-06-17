#include <Application/Application.h>
#include <Application/Model.h>
#include <Renderer/Texture.h>
#include <Math/Math.h>
#include <Math/Geometry.h>

Mat ModelView;
Mat Viewport;
Mat Projection;

//======================================================================================================
// 初期化するぞ
//======================================================================================================
bool Application::OnInitialize()
{
	std::wstring Dir = L"resources\\sponza\\";
	Model Loader((Dir + L"sponza.obj").c_str());


	Loader.GetMeshData(_MeshParams);
	
	// メッシュ毎のテクスチャを読む
	for (auto&& Mesh : _MeshParams)
	{
		Texture texture;
		texture.Load((Dir + Mesh.Texture + L".dds").c_str());
	}
	

	_CameraDistance = 45.0f;
	Vector_Set(_CameraAngle, -0.2f, 0.3f, 0.0f, 0.0f);
	Vector_Set(_CameraTarget, 0.0f, 3.0f, 0.0f, 1.0f);

	return true;
}

//======================================================================================================
// 解放するぞ
//======================================================================================================
void Application::OnFinalize()
{
}

//======================================================================================================
// 毎フレームの更新
//======================================================================================================
void Application::OnUpdate(fp32 FrameTime)
{
	//--------------------------------------------------------------------
	// カメラの行列を作る
	//--------------------------------------------------------------------
	// ビュー行列
	Matrix mRotateX, mRotateY;
	Matrix_RotationX(mRotateX, _CameraAngle.x);
	Matrix_RotationY(mRotateY, _CameraAngle.y);

	Vector4 Eye = { 0.0f, 0.0f, _CameraDistance, 1.0f };
	Matrix_Transform3x3(Eye, Eye, mRotateX);
	Matrix_Transform3x3(Eye, Eye, mRotateY);
	Vector4 At = _CameraTarget;
	Vector_Add(Eye, Eye, At);
	Matrix_CreateLookAtView(_mView, Eye, At, Vector4::Y);
	// プロジェクション行列
	Matrix_CreateProjection(_mProj, 0.1f, 200.0f, ToRadian(45.0f), SCREEN_WIDTH_F / SCREEN_HEIGHT_F);
	// ビュープロジェクション行列
	Matrix_Multiply4x4(_mViewProj, _mView, _mProj);
}

void Application::DrawPolygon(Vector3 t0, Vector3 t1, Vector3 t2, FrameBuffer<uint32>& ColorBuffer, FrameBuffer<fp32>& DepthBuffer) {
	if (t0.y == t1.y && t0.y == t2.y) return;
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);
	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
		Vector3 A = t0 + (t2 - t0) * alpha;
		Vector3 B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
		if (A.x > B.x) std::swap(A, B);
		for (int j = A.x < 0 ? 0 : A.x; j < (B.x < SCREEN_WIDTH_F ? B.x : SCREEN_WIDTH_F); j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vector3 P = A + (B - A) * phi;
			P.x = j; P.y = t0.y + i;
			if (P.y < 0 || P.y >= SCREEN_HEIGHT_F)
				continue;
			if (DepthBuffer.GetPixel(P.x, P.y) < P.z) {
				DepthBuffer.SetPixel(P.x, P.y, P.z);
				if(j == (int)A.x || j == (int)B.x)
					ColorBuffer.SetPixel(P.x, P.y, 0xFFFF00FF);
				else
					ColorBuffer.SetPixel(P.x, P.y, 0xFFFFFFFF);
			}
			
		}
	}
}

Vector3 barycentric(Vector3 A, Vector3 B, Vector3 C, Vector3 P) {
	Vector2 ab = (B - A).RefVec2();
	Vector2 ac = (C - A).RefVec2();
	Vector2 ap = (P - A).RefVec2();
	float factor = 1 / (ab.x * ac.y - ab.y * ac.x);
	float s = (ac.y * ap.x - ac.x * ap.y) * factor;
	float t = (ab.x * ap.y - ab.y * ap.x) * factor;
	return { 1 - s - t, s, t };
}

void triangle(Vector4 *pts, FrameBuffer<uint32>& ColorBuffer, FrameBuffer<fp32>& DepthBuffer, uint32 color) {
	Vector3 back_facing;
	Vector_CrossProduct(back_facing, pts[1].RefVec3() - pts[0].RefVec3(), pts[2].RefVec3() - pts[0].RefVec3());
	if (back_facing.z < 0) {
		return;
	}
	Vector2 bboxmin = { std::numeric_limits<fp32>::max(), std::numeric_limits<fp32>::max() };
	Vector2 bboxmax = { -std::numeric_limits<fp32>::max(), -std::numeric_limits<fp32>::max() };
	int32 bboxminX = std::numeric_limits<int32>::max();
	int32 bboxminY = std::numeric_limits<int32>::max();
	int32 bboxmaxX = -std::numeric_limits<int32>::max();
	int32 bboxmaxY = -std::numeric_limits<int32>::max();
	for (int i = 0; i < 3; i++) {
		bboxmin.x = std::max(0.f, std::min(bboxmin.x, pts[i].x));
		bboxmax.x = std::min(SCREEN_WIDTH_F - 1, std::max(bboxmax.x, pts[i].x));
		bboxmin.y = std::max(0.f, std::min(bboxmin.y, pts[i].y));
		bboxmax.y = std::min(SCREEN_HEIGHT_F - 1, std::max(bboxmax.y, pts[i].y));
	}
	bboxminX = std::max((int32)ceil(bboxmin.x), 0);
	bboxminY = std::max((int32)ceil(bboxmin.y), 0);
	bboxmaxX = std::min((int32)floor(bboxmax.x), SCREEN_WIDTH - 1);
	bboxmaxY = std::min((int32)floor(bboxmax.y), SCREEN_HEIGHT - 1);
	
	Vector3 P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vector3 bc_screen = barycentric(pts[0].RefVec3(), pts[1].RefVec3(), pts[2].RefVec3(), P);
			bool weight0_okay = bc_screen.x > -EPSILON;
			bool weight1_okay = bc_screen.y > -EPSILON;
			bool weight2_okay = bc_screen.z > -EPSILON;
			if (weight0_okay && weight1_okay && weight2_okay) {
				P.z = 0;
				for (int i = 0; i < 3; i++) P.z += pts[i][2] * bc_screen[i];
				if (DepthBuffer.GetPixel(P.x, P.y) < P.z) {
					DepthBuffer.SetPixel(P.x, P.y, P.z);
					if (bc_screen.x < EPSILON * 150 || bc_screen.y < EPSILON * 150 || bc_screen.z < EPSILON * 150)
						ColorBuffer.SetPixel(P.x, P.y, 0xFFFF00FF);
					else
						ColorBuffer.SetPixel(P.x, P.y, color);
				}
			}
		}
	}
}

//======================================================================================================
// 毎フレームの描画
//======================================================================================================
void Application::OnRendering(FrameBuffer<uint32>& ColorBuffer, FrameBuffer<fp32>& DepthBuffer)
{
	srand((unsigned)time(NULL));
	// メッシュ毎
	for (auto&& Mesh : _MeshParams)
	{
		// 三角形（ポリゴン）毎
		for (auto&& Triangle : Mesh.Triangles)
		{
			// 各頂点事
			Vector4 positions[3];
			for (int32 i = 0; i < 3; ++i)
			{
				const auto& Vertex = Triangle.Vertex[i];
				Vector3 source = { Vertex.Position[0], Vertex.Position[1], Vertex.Position[2] };
				//std::cout << "source before: " << source.x << " " << source.y << " " << source.z;
				
				Matrix_Transform4x4Projection(positions[i], source, _mViewProj);
				positions[i].x = (positions[i].x * 0.5f + 0.5f) * SCREEN_WIDTH_F;
				positions[i].y = (positions[i].y * -0.5f + 0.5f) * SCREEN_HEIGHT_F;
				positions[i].z = 1 - positions[i].z;
				//std::cout << "source after: " << positions[i].x << " " << positions[i].y << " " << positions[i].z << std::endl;
			}
			
			triangle(positions, ColorBuffer, DepthBuffer, ((rand() % 256) << 24) + ((rand() % 256) << 16) + ((rand() % 255) << 8) + 0xFF);
		}
	}
}

//======================================================================================================
// マウスドラッグ
//======================================================================================================
void Application::OnLeftMouseDrag(int32 x, int32 y)
{
	_CameraAngle.x -= fp32(y) * 0.005f;
	_CameraAngle.y += fp32(x) * 0.005f;

	if (_CameraAngle.x < ToRadian(-88.0f)) _CameraAngle.x = ToRadian(-88.0f);
	if (_CameraAngle.x > ToRadian(+88.0f)) _CameraAngle.x = ToRadian(+88.0f);
	while (_CameraAngle.y < ToRadian(-180.0f)) _CameraAngle.y += ToRadian(360.0f);
	while (_CameraAngle.y > ToRadian(+180.0f)) _CameraAngle.y -= ToRadian(360.0f);
}

//======================================================================================================
// マウスドラッグ
//======================================================================================================
void Application::OnRightMouseDrag(int32 x, int32 y)
{
	_CameraDistance += fp32(y - x) * 0.001f * _CameraDistance;
	if (_CameraDistance < 0.001f) _CameraDistance = 0.001f;
}

//======================================================================================================
// マウスドラッグ
//======================================================================================================
void Application::OnWheelMouseDrag(int32 x, int32 y)
{
	_CameraTarget.x -= _mView.x.x * fp32(x) * 0.003f;
	_CameraTarget.y -= _mView.y.x * fp32(x) * 0.003f;
	_CameraTarget.z -= _mView.z.x * fp32(x) * 0.003f;
	_CameraTarget.x += _mView.x.y * fp32(y) * 0.003f;
	_CameraTarget.y += _mView.y.y * fp32(y) * 0.003f;
	_CameraTarget.z += _mView.z.y * fp32(y) * 0.003f;
}
