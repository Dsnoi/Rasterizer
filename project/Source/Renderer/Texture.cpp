#include <Renderer/Texture.h>

//======================================================================================================
//
//======================================================================================================
Texture::Texture()
	: _pTexel(nullptr)
	, _Width(0)
	, _Height(0)
{
}

//======================================================================================================
//
//======================================================================================================
Texture::~Texture()
{
	Release();
}

//======================================================================================================
//
//======================================================================================================
bool Texture::Create(uint32 w, uint32 h)
{
	Release();

	_Width = w;
	_Height = h;
	_pTexel = (Texel*)_aligned_malloc(sizeof(Texel) * w * h, 16);
	_UMask = _Width - 1;
	_VMask = _Height - 1;

	return _pTexel != nullptr;
}

//======================================================================================================
//
//======================================================================================================
bool Texture::Load(const wchar_t* pFileName)
{
	struct DDPIXELFORMAT
	{
		uint32	dwSize;
		uint32	dwFlags;
		uint32	dwFourCC;
		uint32	dwRGBBitCount;
		uint32	dwRBitMask;
		uint32	dwGBitMask;
		uint32	dwBBitMask;
		uint32	dwRGBAlphaBitMask;
	};
	struct DDCAPS2
	{
		uint32	dwCaps1;
		uint32	dwCaps2;
		uint32	Reserved[2];
	};
	struct DDSURFACEDESC2
	{
		uint32			dwSize;
		uint32			dwFlags;
		uint32			dwHeight;
		uint32			dwWidth;
		uint32			dwPitchOrLinearSize;
		uint32			dwDepth;
		uint32			dwMipMapCount;
		uint32			dwReserved1[11];
		DDPIXELFORMAT	ddpfTexelFormat;
		DDCAPS2			ddsCaps;
		uint32			dwReserved2;
	};

	DWORD ReadedBytes;
	uint32 MagicNumber;
	DDSURFACEDESC2 DDSHeader;

	Release();

	HANDLE hFile = ::CreateFile(pFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) goto EXIT;

	// マジックナンバー
	::ReadFile(hFile, &MagicNumber, sizeof(uint32), &ReadedBytes, nullptr);
	if (MagicNumber != ' SDD') goto EXIT;

	// ヘッダ
	::ReadFile(hFile, &DDSHeader, sizeof(DDSURFACEDESC2), &ReadedBytes, nullptr);

	if (DDSHeader.ddpfTexelFormat.dwRGBBitCount != 32) goto EXIT;
	if (DDSHeader.ddpfTexelFormat.dwRGBAlphaBitMask != 0xFF000000) goto EXIT;
	if (DDSHeader.ddpfTexelFormat.dwRBitMask != 0x00FF0000) goto EXIT;
	if (DDSHeader.ddpfTexelFormat.dwGBitMask != 0x0000FF00) goto EXIT;
	if (DDSHeader.ddpfTexelFormat.dwBBitMask != 0x000000FF) goto EXIT;

	// サーフェイス生成
	Create(DDSHeader.dwWidth, DDSHeader.dwHeight);

	_WidthF  = fp32(_Width);
	_HeightF = fp32(_Height);

	_UBit = 0;
	while ((1 << _UBit) != _Width)
	{
		_UBit++;
	}

	_VBit = 0;
	while ((1 << _VBit) != _Height)
	{
		_VBit++;
	}

	// ピクセルデータ読み込み
	::ReadFile(hFile, _pTexel, sizeof(Texel) * DDSHeader.dwWidth * DDSHeader.dwHeight, &ReadedBytes, nullptr);

	for (uint32 i = 0; i < DDSHeader.dwWidth * DDSHeader.dwHeight; ++i)
	{
		auto& p = _pTexel[i];
		p.a = uint8((uint32(p.a) * 128) / 255);
		p.r = uint8((uint32(p.r) * 128) / 255);
		p.g = uint8((uint32(p.g) * 128) / 255);
		p.b = uint8((uint32(p.b) * 128) / 255);
	}

EXIT:
	// ファイル閉じる
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	return _pTexel != nullptr;
}

//======================================================================================================
//
//======================================================================================================
void Texture::Release()
{
	if (_pTexel != nullptr)
	{
		_aligned_free(_pTexel);
		_pTexel = nullptr;
	}
}

//======================================================================================================
//
//======================================================================================================
Texture::Texel Texture::Sample(fp32 u, fp32 v) const
{
	const int32 ui = int32(u * _WidthF ) & _UMask;
	const int32 vi = int32(v * _HeightF) & _VMask;
	return _pTexel[ui + (vi << _UBit)];
}
