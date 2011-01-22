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

#include <iostream>
#include <fstream>
#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include <sstream>
#include <string>

// hax
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_

#include <gl/glew.h>

#include <SFML/Window.hpp>

#include "Banner.h"
#include "QueueThread.h"
#include "Types.h"

#define NO_CONSOLE 0

void DrawBannerBorder(/*float width*/)
{
	//width /= 2;

	glColor4f(0, 0, 0, 1.f);
	glUseProgram(0);

	glBegin(GL_QUADS);
	// bottom
	glVertex2f(-450.f, -112.f);
	glVertex2f(450.f, -112.f);
	glVertex2f(450.f, -400.f);
	glVertex2f(-450.f, -400.f);
	// top
	//glVertex2f(-450.f, 400.f);
	//glVertex2f(450.f, 400.f);
	//glVertex2f(450.f, 222.f);
	//glVertex2f(-450.f, 222.f);
	//// left
	//glVertex2f(-450.f, 400.f);
	//glVertex2f(-(width - 6), 400.f);
	//glVertex2f(-(width - 6), -400.f);
	//glVertex2f(-450.f, -400.f);
	//// right
	//glVertex2f(width - 6, 400.f);
	//glVertex2f(450.f, 400.f);
	//glVertex2f(450.f, -400.f);
	//glVertex2f(width - 6, -400.f);
	glEnd();
}

Vec2i g_tile_size;

void DrawIconBorder()
{
	glUseProgram(0);

	const float
		x = (float)(g_tile_size.x / 2),
		y = (float)(g_tile_size.y / 2);

	glBegin(GL_LINE_STRIP);
	glVertex2f(-x * 2, -y + 1);
	glVertex2f(x, -y + 1);
	glVertex2f(x, y);
	glVertex2f(-x + 1, y);
	glVertex2f(-x + 1, -y * 2);
	glEnd();
}

class Tile
{
public:
	Tile() : banner(nullptr)
	{
		position.x = position.y = 0;
	}

	~Tile()
	{
		delete banner;
	}

	void Draw() const
	{
		if (!(banner && banner->GetIcon()))
			return;

		// why doesn't this work
#if 0
		glClearColor(0.f, 0.f, 0.f, 1.f);
#else
		glLoadIdentity();
		glOrtho(-1, 1, -1, 1, -1, 1);

		glColor4f(0.f, 0.f, 0.f, 1.f);

		glBegin(GL_QUADS);
		glVertex2f(-2, -2);
		glVertex2f(2, -2);
		glVertex2f(2, 2);
		glVertex2f(-2, 2);
		glEnd();
#endif

		// TODO: don't need to push everything
		glPushAttrib(-1);
		banner->GetIcon()->Render((float)g_tile_size.x / g_tile_size.y);
		glPopAttrib();
	}

	Vec2f position;
	WiiBanner::Banner* banner;
};

template <typename F>
void ForEachFile(const std::string& search, F func)
{
#ifdef _WIN32
	WIN32_FIND_DATA find_data;
	HANDLE find_handle;

	find_handle = FindFirstFile(search.c_str(), &find_data);

	if (INVALID_HANDLE_VALUE != find_handle)
	{
		do
		{
			func(find_data.cFileName);
		} while (FindNextFile(find_handle, &find_data));

		FindClose(find_handle);
	}
#else
	// TODO
#endif
}

static std::vector<Tile*> g_tiles;
static Common::CriticalSection g_tiles_lock;
static int g_tile_columns;

void AddTile(Tile* tile)
{
	// TODO: make not so ugly

	g_tiles_lock.Enter();

	Vec2f v(0.f, 0.f);
	if (!g_tiles.empty())
	{
		v = g_tiles.back()->position;
		if (++v.x == g_tile_columns)
		{
			v.x = 0;
			++v.y;
		}
	}

	g_tiles.push_back(tile);
	tile->position  = v;

	g_tiles_lock.Leave();
}

//class CritSecLocker
//{
//	CritSecLocker(Common::CriticalSection& _crit) : crit(_crit) { crit.Enter(); };
//	~CritSecLocker() { crit.Leave(); };
//
//private:
//	Common::CriticalSection& crit;
//};

Tile* FindTile(const Vec2f& pos)
{
	Tile* ret = nullptr;

	g_tiles_lock.Enter();

	foreach (Tile* tile, g_tiles)
	{
		if (tile->position == pos)
		{
			ret = tile;
			break;
		}
	}

	g_tiles_lock.Leave();

	return ret;
}

// TODO: make some sort of thread pool instead of this
QueueThread g_worker;

enum
{
	PRIORITY_TILE = 1,
	PRIORITY_BANNER = 2,
};

void LoadTile(const std::string& fname)
{
	g_worker.Push(PRIORITY_TILE, [](const std::string& fname)
	{
		auto* const bnr = new WiiBanner::Banner(fname);

		bnr->LoadIcon();
		if (bnr->GetIcon())
		{
			bnr->GetIcon()->SetLanguage("ENG");

			Tile* const tile = new Tile;
			tile->banner = bnr;

			AddTile(tile);

			// this sleep just makes it look cool :p
			Common::SleepCurrentThread(50);
		}
		else
			delete bnr;

	}, fname);
}

#if defined(_WIN32) && !defined(DEBUG) && NO_CONSOLE
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#else
int main(int argc, char* argv[])
{
#endif

#if 1
	// 4:3 tiles
	g_tile_columns = 8;
	g_tile_size = Vec2i(128, 96);
#else
	// 16:9 tiles
	g_tile_columns = 6;
	g_tile_size = Vec2i(170, 96);
#endif

	auto const load_tiles = []
	{
		auto const add_extension = [](const char ext[])
		{
			ForEachFile(ext, [](const std::string& fname)
			{
				LoadTile(fname);
			});
		};

		add_extension("*.bnr");
		add_extension("*.app");
	};

	load_tiles();

	static const float
		min_aspect = 4.f / 3,
		max_aspect = 16.f / 9;

	float render_aspect = min_aspect;

	sf::Window window(sf::VideoMode(g_tile_columns * g_tile_size.x, 576, 32), "Wii Banner Player");

	Tile* tile_selected = nullptr;

	sf::Vector2i mouse_position, mouse_down_position;
	mouse_position.x = mouse_position.y = 0;

	mouse_down_position = mouse_position;

	bool mouse_click = false;

	Vec2f tile_adjust(0.f, 0.f);

	auto const draw_tile = [&](const Tile& tile)
	{
		static const int adjust_scale = 1000;

		Vec2f adjust(0.f, 0.f);

		if (&tile == tile_selected)
		{
			adjust.x = mouse_position.x / g_tile_size.x - mouse_down_position.x / g_tile_size.x;
			adjust.y = mouse_position.y / g_tile_size.y - mouse_down_position.y / g_tile_size.y;

			if (adjust != Vec2f(0.f, 0.f))
				mouse_click = false;

			static const float slide_speed = 0.2f;

			if (tile_adjust.x < adjust.x)
				tile_adjust.x = Clamp(tile_adjust.x + slide_speed, tile_adjust.x, adjust.x);
			else if (adjust.x < tile_adjust.x)
				tile_adjust.x = Clamp(tile_adjust.x - slide_speed, adjust.x, tile_adjust.x);

			if (tile_adjust.y < adjust.y)
				tile_adjust.y = Clamp(tile_adjust.y + slide_speed, tile_adjust.y, adjust.y);
			else if (adjust.y < tile_adjust.y)
				tile_adjust.y = Clamp(tile_adjust.y - slide_speed, adjust.y, tile_adjust.y);

			adjust = tile_adjust;
		}

		glViewport((tile.position.x + adjust.x) * g_tile_size.x,
			window.GetHeight() - (tile.position.y + adjust.y + 1) * g_tile_size.y,
			g_tile_size.x, g_tile_size.y);

		tile.Draw();

		if (&tile == tile_selected)
			glColor4ub(0xff, 0xff, 0xff, 0xff);
		else
			glColor4ub(0, 0, 0, 0xff);

		DrawIconBorder();
	};

	GX_Init(0, 0);

	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	WiiBanner::Banner* volatile full_banner = nullptr;

	auto const disable_full_banner = [&full_banner]
	{
		if (full_banner)
		{
			auto* const bnr = full_banner;

			g_tiles_lock.Enter();
			full_banner = nullptr;
			g_tiles_lock.Leave();

			bnr->UnloadSound();
			bnr->UnloadBanner();
		}
	};

	bool banner_play = true;

	while (window.IsOpened())
	{
		sf::Event ev;
		while (window.GetEvent(ev))
		{
			switch (ev.Type)
			{
			case sf::Event::MouseMoved:
				mouse_position.x = ev.MouseMove.X;
				mouse_position.y = ev.MouseMove.Y;
				break;

			case sf::Event::MouseButtonPressed:
				if (full_banner)
				{
					g_worker.Push(PRIORITY_BANNER, [&]
					{
						disable_full_banner();
					});
				}
				else
				{
					mouse_down_position = mouse_position;
					tile_selected = FindTile(Vec2f(mouse_position.x / g_tile_size.x, mouse_position.y / g_tile_size.y));
					tile_adjust = Vec2f(0.f, 0.f);

					mouse_click = true;
				}
				break;

			case sf::Event::MouseButtonReleased:
				if (tile_selected)
				{
					const Vec2f dest(mouse_position.x / g_tile_size.x, mouse_position.y / g_tile_size.y);

					g_tiles_lock.Enter();

					Tile* const tile_dest = FindTile(Vec2f(dest.x, dest.y));
					if (tile_dest)
					{
						if (mouse_click)
						{
							g_worker.Push(PRIORITY_BANNER, [&](Tile* tile)
							{
								if (full_banner != tile->banner)
								{
									tile->banner->LoadBanner();
									tile->banner->GetBanner()->SetLanguage("ENG");

									tile->banner->LoadSound();

									disable_full_banner();

									g_tiles_lock.Enter();
									full_banner = tile->banner;
									full_banner->GetSound()->Play();
									g_tiles_lock.Leave();
								}

							}, tile_selected);
						}
						else
							std::swap(tile_dest->position, tile_selected->position);
					}
					else
						tile_selected->position = dest;
							
					g_tiles_lock.Leave();

					tile_selected = nullptr;
				}
				break;

			case sf::Event::Closed:
				window.Close();
				break;

			case sf::Event::KeyPressed:
				switch (ev.Key.Code)
				{
					// reload all tiles
				case sf::Key::Back:
					g_worker.Clear();
					full_banner = nullptr;
					foreach (Tile* tile, g_tiles)
						delete tile;
					g_tiles.clear();
					load_tiles();
					break;

					// pause resume playback
				case sf::Key::Space:
					banner_play ^= true;
					break;

					// exit app
				case sf::Key::Escape:
					window.Close();
					break;
				}
				break;
			}
		}

		window.SetActive();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 1.f);

		g_tiles_lock.Enter();
		
		if (full_banner)
		{
			const float window_aspect = (float)window.GetWidth() / (float)window.GetHeight();
					
			render_aspect = Clamp(window_aspect, min_aspect, max_aspect);

			if (render_aspect > window_aspect)
			{
				const GLsizei h = GLsizei(window.GetWidth() / render_aspect);
				glViewport(0, (window.GetHeight() - h) / 2, window.GetWidth(), h);
			}
			else
			{
				const GLsizei w = GLsizei(window.GetHeight() * render_aspect);
				glViewport((window.GetWidth() - w) / 2, 0, w, window.GetHeight());
			}

			// TODO: don't need to push everything
			glPushAttrib(-1);
			full_banner->GetBanner()->Render(render_aspect, 608.f / (608 - 12));	// crop off a bit
			glPopAttrib();

			DrawBannerBorder(/*608.f * render_aspect / min_aspect*/);

			if (banner_play)
				full_banner->GetBanner()->AdvanceFrame();
		}
		else
		{
			// draw icons
			foreach (Tile* tile, g_tiles)
			{
				if (tile != tile_selected)
					if (tile->banner)
						draw_tile(*tile);
			}

			if (tile_selected)
				draw_tile(*tile_selected);

			if (banner_play)
			{
				foreach (Tile* tile, g_tiles)
					if (tile->banner)
						tile->banner->GetIcon()->AdvanceFrame();
			}
		}

		g_tiles_lock.Leave();

		window.Display();

		// TODO: could be improved
		// limit fps
		static const int desired_fps = 60;
		static int frame_count = 0;
		static sf::Clock clock;
		static float sleep_time = 1.f / desired_fps;

		if (10 == frame_count)
		{
			sleep_time = (1.f / desired_fps) - (clock.GetElapsedTime() / frame_count - sleep_time);
			
			//std::cout << "sleep_time: " << (1.f / sleep_time) << '\n';

			frame_count = 0;
			clock.Reset();
		}
		else
			++frame_count;

		sf::Sleep(sleep_time);
	}

	// cleanup
	foreach (Tile* tile, g_tiles)
		delete tile;
}
