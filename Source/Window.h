/*
Copyright (c) 2010 - Wii Banner Player Project

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Picture.h"

#include "WrapGx.h"

#ifndef WII_BNR_WINDOW_H_
#define WII_BNR_WINDOW_H_

namespace WiiBanner
{

class Window : public Quad
{
public:
	typedef Quad Base;
	
	static const u32 BINARY_MAGIC = 'wnd1';

	void Load(std::istream& file);

private:
	void Draw(const Resources& resources, u8 render_alpha) const;

	struct
	{
		float l, r, t, b;

	} inflation;

	struct Frame
	{
	    u16 material_index;
		u8 texture_flip;
	};
	std::vector<Frame> frames;
};

}

#endif
