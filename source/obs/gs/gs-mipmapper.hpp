/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
#include "gs-effect.hpp"
#include "gs-rendertarget.hpp"
#include "gs-texture.hpp"
#include "gs-vertexbuffer.hpp"

// OBS
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <graphics/graphics.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef WIN32
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191)
#pragma warning(disable : 4201)
#pragma warning(disable : 4365)
#pragma warning(disable : 4777)
#pragma warning(disable : 4986)
#pragma warning(disable : 5039)
#endif
#include <atlutil.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

namespace gs {
	class mipmapper {
#ifdef WIN32
		// Direct3D 11
		ATL::CComPtr<ID3D11Device>            _d3d_device;
		ATL::CComPtr<ID3D11DeviceContext>     _d3d_context;
		ATL::CComPtr<ID3D11Texture2D>         _d3d_rtt;
		ATL::CComPtr<ID3D11RenderTargetView>  _d3d_rtv;
		ATL::CComPtr<ID3D11DepthStencilState> _d3d_dss;
#endif

		std::shared_ptr<gs::vertex_buffer> _vb;
		std::shared_ptr<gs::rendertarget>  _rt;
		std::shared_ptr<gs::effect>        _effect;

		uint32_t _width;
		uint32_t _height;

		public:
		enum class generator : uint8_t {
			Point,
			Linear,
			Sharpen,
			Smoothen,
			Bicubic,
			Lanczos,
		};

		public:
		~mipmapper();
		mipmapper();

		void rebuild(std::shared_ptr<gs::texture> source, std::shared_ptr<gs::texture> target,
					 gs::mipmapper::generator generator, float_t strength);
	};
} // namespace gs
