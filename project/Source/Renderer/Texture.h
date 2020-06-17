#pragma once

//======================================================================================================
//
//======================================================================================================
class Texture
{
public:
	union Texel {
		struct {
			uint8 b, g, r, a;
		};
		uint32 data;
	};

private:
	Texel*		_pTexel;
	int32		_Width;
	int32		_Height;
	fp32		_WidthF;
	fp32		_HeightF;
	int32		_UMask;
	int32		_VMask;
	int32		_UBit;
	int32		_VBit;

public:
	Texture();
	~Texture();

public:
	bool Create(uint32 w, uint32 h);
	bool Load(const wchar_t* pFileName);
	void Release();

public:
	Texel* GetTexelPtr()
	{
		return _pTexel;
	}

	const Texel* GetTexelPtr() const
	{
		return _pTexel;
	}

	uint32 GetWidth() const
	{
		return _Width;
	}

	uint32 GetHeight() const
	{
		return _Height;
	}

	Texel Sample(fp32 u, fp32 v) const;
};
