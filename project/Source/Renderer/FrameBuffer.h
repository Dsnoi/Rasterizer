#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
//======================================================================================================
//
//======================================================================================================
template <typename T>
class FrameBuffer
{
private:
	T*		_pPixel;
	int32	_Width;
	int32	_Height;
	bool	_IsInternal;
	static const int INSIDE = 0;
	static const int LEFT = 1;
	static const int RIGHT = 2;
	static const int BOTTOM = 4;
	static const int TOP = 8;
	double xMin;
	double yMin;
	double xMax;
	double yMax;

public:
	FrameBuffer(T* pPixel, int32 Width, int32 Height)
		: _pPixel(pPixel)
		, _Width(Width)
		, _Height(Height)
		, _IsInternal(pPixel == nullptr)
	{
		if (_IsInternal)
		{
			_pPixel = new T[Width * Height];
		}
	}
	~FrameBuffer()
	{
		if (_IsInternal)
		{
			delete[] _pPixel;
		}
	}

public:
	void Clear(T pixel)
	{
		int32 count = int32(_Width * _Height);
		while (count-- > 0)
		{
			_pPixel[count] = pixel;
		}
	}

	bool SetPixel(int32 x, int32 y, T color)
	{
		ASSERT(x >= 0);
		ASSERT(y >= 0);
		ASSERT(x < _Width);
		ASSERT(y < _Height);
		if (x < 0 || x >= _Width || y < 0 || y >= _Height)
			return false;
		_pPixel[x + y * _Width] = color;
		return true;
	}

	void DrawBresenhamline(int32 x0, int32 y0, int32 x1, int32 y1, T color)
	{
		int32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int32 dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int32 err = (dx > dy ? dx : -dy) / 2, e2;
		for (;;) {
			SetPixel(x0, y0, color);
			if (x0 == x1 && y0 == y1) break;
			e2 = err;
			if (e2 > -dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}

	T& GetPixelRef(int32 x, int32 y)
	{
		ASSERT(x >= 0);
		ASSERT(y >= 0);
		ASSERT(x < _Width);
		ASSERT(y < _Height);
		return _pPixel[x + y * _Width];
	}

	T GetPixel(int32 x, int32 y) const
	{
		ASSERT(x >= 0);
		ASSERT(y >= 0);
		ASSERT(x < _Width);
		ASSERT(y < _Height);
		return _pPixel[x + y * _Width];
	}

	T* GetPixelPointer()
	{
		return _pPixel;
	}

	const T* GetPixelPointer() const
	{
		return _pPixel;
	}

	uint32 GetWidth() const
	{
		return _Width;
	}

	uint32 GetHeight() const
	{
		return _Height;
	}
};
