
//======================================================================================================
//
//======================================================================================================
#include <Application/Application.h>
#include <Renderer/FrameBuffer.h>

//======================================================================================================
//
//======================================================================================================
class DIBBuffer
{
private:
	uint32* _pBackBuffer;
	int32 _Width;
	int32 _Height;
	HBITMAP _hBitmap;
	HWND _hWnd;
	HDC _hWindowDC;
	HDC _hBackBufferDC;

public:
	uint32* BackBuffer()
	{
		return _pBackBuffer;
	}
	HDC BackBufferDC()
	{
		return _hBackBufferDC;
	}
	int32 Width()
	{
		return _Width;
	}
	int32 Height()
	{
		return _Height;
	}
	void Create(HWND hWnd, HDC hWindowDC, int32 w, int32 h)
	{
		_Width = w;
		_Height = h;

		BITMAPINFOHEADER BmpInfoHeader = { sizeof(BITMAPINFOHEADER) };
		BmpInfoHeader.biWidth = +_Width;
		BmpInfoHeader.biHeight = -_Height;
		BmpInfoHeader.biPlanes = 1;
		BmpInfoHeader.biBitCount = 32;
		BmpInfoHeader.biCompression = BI_RGB;

		_hWnd = hWnd;
		_hWindowDC = hWindowDC;
		_hBitmap = ::CreateDIBSection(_hWindowDC, (BITMAPINFO*)&BmpInfoHeader, DIB_RGB_COLORS, (void**)&_pBackBuffer, nullptr, 0);
		_hBackBufferDC = ::CreateCompatibleDC(_hWindowDC);
		::SelectObject(_hBackBufferDC, _hBitmap);
	}
	void Release()
	{
		::ReleaseDC(_hWnd, _hBackBufferDC);
		::DeleteObject(_hBitmap);
		::ReleaseDC(_hWnd, _hWindowDC);
	}
};

//======================================================================================================
//
//======================================================================================================
static Application _App;
static bool _RequireSaveScene;

//======================================================================================================
//
//======================================================================================================
static void SaveToBMP(const wchar_t* pFileName, const FrameBuffer<uint32>& Buffer)
{
	BITMAPINFOHEADER BmpIH = { sizeof(BITMAPINFOHEADER) };
	BmpIH.biWidth = Buffer.GetWidth();
	BmpIH.biHeight = Buffer.GetHeight();
	BmpIH.biPlanes = 1;
	BmpIH.biBitCount = 32;
	BmpIH.biCompression = BI_RGB;

	BITMAPFILEHEADER BmpH;
	BmpH.bfType = 'MB';
	BmpH.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(uint32) * BmpIH.biWidth * BmpIH.biHeight;
	BmpH.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	HANDLE hFile = ::CreateFile(pFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD WriteSize;
		::WriteFile(hFile, &BmpH, sizeof(BITMAPFILEHEADER), &WriteSize, nullptr);
		::WriteFile(hFile, &BmpIH, sizeof(BITMAPINFOHEADER), &WriteSize, nullptr);
		const uint32* pPixel = Buffer.GetPixelPointer() + (BmpIH.biWidth * BmpIH.biHeight);
		for (int32 y = 0; y < BmpIH.biHeight; ++y)
		{
			pPixel -= BmpIH.biWidth;
			::WriteFile(hFile, pPixel, sizeof(uint32) * BmpIH.biWidth, &WriteSize, nullptr);
		}
		::CloseHandle(hFile);
	}
}

//======================================================================================================
//
//======================================================================================================
LRESULT CALLBACK MessageProc(HWND hWnd, uint32 Msg, WPARAM wParam, LPARAM lParam)
{
	static POINT CursorPosition;

	switch (Msg)
	{
		//---------------------------------------------
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&CursorPosition);
		break;
		//---------------------------------------------
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		::ReleaseCapture();
		break;
		//---------------------------------------------
	case WM_MOUSEMOVE:
		{
			POINT NewCursorPosition;
			::GetCursorPos(&NewCursorPosition);
			const int32 mx = NewCursorPosition.x - CursorPosition.x;
			const int32 my = NewCursorPosition.y - CursorPosition.y;
			CursorPosition = NewCursorPosition;
			if (wParam & MK_LBUTTON)
			{
				_App.OnLeftMouseDrag(mx, my);
			}
			if (wParam & MK_RBUTTON)
			{
				_App.OnRightMouseDrag(mx, my);
			}
			if (wParam & MK_MBUTTON)
			{
				_App.OnWheelMouseDrag(mx, my);
			}
		}
		break;
		//---------------------------------------------
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			::SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case VK_RETURN:
			_RequireSaveScene = true;
			break;
		}
		break;
		//---------------------------------------------
	case WM_CREATE:
		::SetCursor(LoadCursor(nullptr, IDC_ARROW));
		break;
		//---------------------------------------------
	case WM_CLOSE:
		::PostMessage(hWnd, WM_DESTROY, 0, 0);
		break;
		//---------------------------------------------
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

//======================================================================================================
//
//======================================================================================================
int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int32)
{
	wchar_t ModuleFileName[MAX_PATH];
	std::chrono::high_resolution_clock::time_point TimePoint;
	WNDCLASS WindowClass;
	HWND hWnd;
	MSG Msg;

	//---------------------------------------------------------
	// カレントディレクトリ設定
	//---------------------------------------------------------
	GetModuleFileName(hInstance, ModuleFileName, MAX_PATH);
	int32 Length = (int32)wcslen(ModuleFileName);
	while (ModuleFileName[Length] != L'\\')
	{
		if (--Length < 0)
		{
			break;
		}
	}
	ModuleFileName[Length + 1] = L'\0';
	SetCurrentDirectory(ModuleFileName);

	//---------------------------------------------------------
	// ウィンドウクラス登録
	//---------------------------------------------------------
	WindowClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MessageProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = hInstance;
	WindowClass.hIcon = nullptr;
	WindowClass.hCursor = nullptr;
	WindowClass.hbrBackground = nullptr;
	WindowClass.lpszMenuName = nullptr;
	WindowClass.lpszClassName = APPLICATION_TITLE;
	if (!::RegisterClass(&WindowClass))
	{
		return 1;
	}

	//---------------------------------------------------------
	// ウィンドウ生成
	//---------------------------------------------------------
	int32 x = 0;
	int32 y = 0;
	int32 w = SCREEN_WIDTH;
	int32 h = SCREEN_HEIGHT;
	int32 Style = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	RECT Rect = { 0, 0, w, h };
	::AdjustWindowRect(&Rect, Style, false);

	w = Rect.right - Rect.left;
	h = Rect.bottom - Rect.top;

	x = ::GetSystemMetrics(SM_CXSCREEN) / 2 - w / 2;
	y = ::GetSystemMetrics(SM_CYSCREEN) / 2 - h / 2;

	hWnd = ::CreateWindowEx(
			0, APPLICATION_TITLE, APPLICATION_TITLE, Style,
			x, y, w, h,
			nullptr, nullptr, hInstance, nullptr);
	if (hWnd == nullptr)
	{
		return 1;
	}

	//--------------------------------------------------------------------------
	// 初期化処理
	//--------------------------------------------------------------------------
	if (!_App.OnInitialize())
	{
		return 1;
	}

	//--------------------------------------------------------------------------
	// バッファ生成
	//--------------------------------------------------------------------------
	auto hWindowDC = ::GetDC(hWnd);

	DIBBuffer Buffer;
	fp32 *zbuffer = new fp32[SCREEN_WIDTH * SCREEN_HEIGHT];
	Buffer.Create(hWnd, hWindowDC, SCREEN_WIDTH, SCREEN_HEIGHT);
	FrameBuffer<uint32> BackBuffer(Buffer.BackBuffer(), Buffer.Width(), Buffer.Height());
	FrameBuffer<fp32> DepthBuffer(zbuffer, Buffer.Width(), Buffer.Height());
	DepthBuffer.Clear(std::numeric_limits<fp32>::lowest());

	//--------------------------------------------------------------------------
	// メッセージループ
	//--------------------------------------------------------------------------
	bool IsRunning = true;

	TimePoint = std::chrono::high_resolution_clock::now();
	while (IsRunning)
	{
		auto TimeNow = std::chrono::high_resolution_clock::now();

		//------------------------------------------------------
		// FPS計算
		//------------------------------------------------------
		{
			static int32 FPS = 0U;
			static int64 PreTime = std::chrono::duration_cast<std::chrono::microseconds>(TimeNow - TimePoint).count();
			auto NowTime = std::chrono::duration_cast<std::chrono::microseconds>(TimeNow - TimePoint).count();
			if (NowTime - PreTime >= 1000000 / 4)
			{
				wchar_t Text[64];
				swprintf_s(Text, 64, L"%s  (FPS:%.1lf)",
					APPLICATION_TITLE,
					(fp32)FPS * 1000000.0f / (fp32)(NowTime - PreTime));
				::SetWindowText(hWnd, Text);
				FPS = 0;
				PreTime = NowTime;
			}
			FPS++;
		}

		//------------------------------------------------------
		// レンダリング
		//------------------------------------------------------
		{
			auto TimeNow = std::chrono::high_resolution_clock::now();
			static int64 BeforeTime = std::chrono::duration_cast<std::chrono::microseconds>(TimeNow - TimePoint).count();
			auto NowTime = std::chrono::duration_cast<std::chrono::microseconds>(TimeNow - TimePoint).count();
			auto FrameTime = (fp32)(NowTime - BeforeTime) / 1000000.0f;
			BeforeTime = NowTime;

			_App.OnUpdate(FrameTime);
			_App.OnRendering(BackBuffer, DepthBuffer);
		}

		//------------------------------------------------------
		// 画面更新
		//------------------------------------------------------
		::BitBlt(
			hWindowDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
			Buffer.BackBufferDC(), 0, 0, SRCCOPY);

		if (_RequireSaveScene)
		{
			_RequireSaveScene = false;
			SaveToBMP(L"ScreenShot.bmp", BackBuffer);
			uint32 *z = new uint32[SCREEN_WIDTH * SCREEN_HEIGHT];
			FrameBuffer<uint32> Depth(z, Buffer.Width(), Buffer.Height());
			fp32 max = std::numeric_limits<fp32>::min(), min = std::numeric_limits<fp32>::max();
			for (int j = 0; j < Depth.GetHeight(); j++)
			{
				for (int i = 0; i < Depth.GetWidth(); i++)
				{
					fp32 depth = DepthBuffer.GetPixel(i, j);
					max = depth > max ? depth : max;
					min = min > depth ? depth : min;
					Depth.SetPixel(i, j, 0xFFFFFF00 * ((depth + 1.) / 2.) + 0xFF);
				}
			}
			std::cout << "Max:" << max << "Min:" << min << std::endl;
			SaveToBMP(L"ScreenShot_Depth.bmp", Depth);
		}

		BackBuffer.Clear(0xFF000000);
		DepthBuffer.Clear(std::numeric_limits<fp32>::lowest());

		//------------------------------------------------------
		// メッセージ処理
		//------------------------------------------------------
		while (::PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT)
			{
				IsRunning = false;
				break;
			}
			::TranslateMessage(&Msg);
			::DispatchMessage(&Msg);
		}
	}

	//--------------------------------------------------------------------------
	// バッファ解放
	//--------------------------------------------------------------------------
	Buffer.Release();

	//--------------------------------------------------------------------------
	// 解放
	//--------------------------------------------------------------------------
	_App.OnFinalize();

	return 0;
}

//======================================================================================================
//
//======================================================================================================
int32 main()
{
	return WinMain(::GetModuleHandle(nullptr), nullptr, nullptr, 0);
}
