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

#include "gs-mipmapper.hpp"
#include <algorithm>
#include <stdexcept>
#include "obs/gs/gs-helper.hpp"
#include "plugin.hpp"

// OBS
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <graphics/graphics.h>
#include <obs-module.h>
#include <obs.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

gs::mipmapper::~mipmapper()
{
	_vb.reset();
	_rt.reset();
	_effect.reset();
}

gs::mipmapper::mipmapper()
{
	auto gctx = gs::context();
	_vb       = std::make_shared<gs::vertex_buffer>(uint32_t(3u), uint8_t(1u));

	{
		auto vtx        = _vb->at(0);
		vtx.position->x = vtx.position->y = vtx.uv[0]->x = vtx.uv[0]->y = 0.0;
	}

	{
		auto vtx        = _vb->at(1);
		vtx.position->x = 1.;
		vtx.uv[0]->x    = 2.;
		vtx.position->y = vtx.uv[0]->y = 0.0;
	}

	{
		auto vtx        = _vb->at(2);
		vtx.position->y = 1.;
		vtx.uv[0]->y    = 2.;
		vtx.position->x = vtx.uv[0]->x = 0.0;
	}

	_vb->update();

	char* effect_file = obs_module_file("effects/mipgen.effect");
	_effect           = std::make_shared<gs::effect>(effect_file);
	bfree(effect_file);

#ifdef WIN32
	if (gs_get_device_type() == GS_DEVICE_DIRECT3D_11) {
		_d3d_device = ATL::CComPtr<ID3D11Device>(reinterpret_cast<ID3D11Device*>(gs_get_device_obj()));
		_d3d_device->GetImmediateContext(&_d3d_context);
	}
#endif
	if (gs_get_device_type() == GS_DEVICE_OPENGL) {
		// Not Implemented Yet.
	}
}

void gs::mipmapper::rebuild(std::shared_ptr<gs::texture> source, std::shared_ptr<gs::texture> target,
							gs::mipmapper::generator generator = gs::mipmapper::generator::Linear,
							float_t                  strength  = 1.0)
{
	// Enter Graphics Context
	auto gctx = gs::context();

	// Validate some things to make sure we can actually work.
	if (!source || !target) { // Neither source or target exists, skip.
		throw std::invalid_argument("Missing source or target, skipping.");
	}

	// Ensure Source and Target match.
	if ((source->get_width() != target->get_width()) || (source->get_height() != target->get_height())
		|| (source->get_type() != target->get_type()) || (source->get_color_format() != target->get_color_format())) {
		throw std::invalid_argument("Source and Target textures must be the same size, type and format");
	}

	auto   gdbg       = gs::debug_marker(gs::debug_color_cache, "gs::mipmapper");
	size_t mip_levels = 0;

#ifdef WIN32
	if (gs_get_device_type() == GS_DEVICE_DIRECT3D_11) {
		D3D11_TEXTURE2D_DESC src_desc;
		D3D11_TEXTURE2D_DESC tgt_desc;

		ATL::CComPtr<ID3D11Texture2D> source_texture =
			reinterpret_cast<ID3D11Texture2D*>(gs_texture_get_obj(source->get_object()));
		ATL::CComPtr<ID3D11Texture2D> target_texture =
			reinterpret_cast<ID3D11Texture2D*>(gs_texture_get_obj(target->get_object()));

		source_texture->GetDesc(&src_desc);
		target_texture->GetDesc(&tgt_desc);

		mip_levels = tgt_desc.MipLevels;

		if ((!_d3d_rtt) || (!_d3d_rtv) || (_width != source->get_width()) || (_height != source->get_height())) {
			auto gdbg = gs::debug_marker(gs::debug_color_cache, "Recreate RenderTarget");
			// Recreate the Render Target due to the source changing size.
			D3D11_TEXTURE2D_DESC rt_desc = {};
			rt_desc.Width                = tgt_desc.Width;
			rt_desc.Height               = tgt_desc.Height;
			rt_desc.MipLevels            = 1;
			rt_desc.ArraySize            = 1;
			rt_desc.Format               = tgt_desc.Format;
			rt_desc.SampleDesc.Count     = 1;
			rt_desc.SampleDesc.Quality   = 0;
			rt_desc.Usage                = D3D11_USAGE_DEFAULT;
			rt_desc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			rt_desc.CPUAccessFlags       = 0;
			rt_desc.MiscFlags            = 0;
			_d3d_device->CreateTexture2D(&rt_desc, nullptr, &_d3d_rtt);

			D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
			rtv_desc.Format                        = tgt_desc.Format;
			rtv_desc.ViewDimension                 = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtv_desc.Texture2D.MipSlice            = 0;

			_d3d_device->CreateRenderTargetView(_d3d_rtt, &rtv_desc, &_d3d_rtv);

			_width  = source->get_width();
			_height = source->get_height();
		}
		if (!_d3d_dss) {
			D3D11_DEPTH_STENCIL_DESC dss_desc;
			dss_desc.DepthEnable      = false;
			dss_desc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ZERO;
			dss_desc.DepthFunc        = D3D11_COMPARISON_ALWAYS;
			dss_desc.StencilEnable    = false;
			dss_desc.StencilReadMask  = 0;
			dss_desc.StencilWriteMask = 0;
			dss_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dss_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			dss_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dss_desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
			dss_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dss_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			dss_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dss_desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
			_d3d_device->CreateDepthStencilState(&dss_desc, &_d3d_dss);
		}

		// Copy Mip 0.
		_d3d_context->CopySubresourceRegion(target_texture, 0, 0, 0, 0, source_texture, 0, nullptr);

		// Load Vertex and Index buffer.
		auto prev_rt = gs_get_render_target();
		auto prev_zs = gs_get_zstencil_target();
		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();

		// Render each layer using the previous layer.
		uint32_t width  = source->get_width();
		uint32_t height = source->get_height();
		for (size_t lvl = 1; lvl < mip_levels; lvl++) {
			auto gdbg2 = gs::debug_marker(gs::debug_color_convert, "Layer %llu", lvl);
			width      = std::max(1ul, width / 2ul);
			height     = std::max(1ul, height / 2ul);

			{
				// Set Render Target
				ID3D11RenderTargetView* view = _d3d_rtv.p;
				_d3d_context->OMSetRenderTargets(1, &view, nullptr);

				const FLOAT col[] = {rand() / 65535.0, rand() / 65535.0, rand() / 65535.0, 1.};
				_d3d_context->ClearRenderTargetView(view, col);

				// Set State
				FLOAT blend[] = {1., 1., 1., 1.};
				_d3d_context->OMSetBlendState(NULL, blend, 0xffffffff);
				_d3d_context->OMSetDepthStencilState(NULL, 0);
				_d3d_context->OMSetDepthStencilState(_d3d_dss.p, 0);

				// Set Viewport
				D3D11_VIEWPORT vp = {};
				vp.TopLeftX = 0.;
				vp.TopLeftY = 0.;
				vp.Width = width;
				vp.Height = height;
				vp.MinDepth = 0.;
				vp.MaxDepth = 1.;
				_d3d_context->RSSetViewports(1, &vp);

				// 

				gs_load_vertexbuffer(_vb->update());
				gs_load_indexbuffer(nullptr);

				gs_ortho(0, 1, 0, 1, -1, 1);

				_effect->get_parameter("image_size")
					->set_float2(static_cast<float_t>(width), static_cast<float_t>(height));
				_effect->get_parameter("image_texel")->set_float2(1.0f / width, 1.0f / height);
				_effect->get_parameter("image_level")->set_int(int32_t(lvl - 1));
				_effect->get_parameter("image")->set_texture(target);

				while (gs_effect_loop(_effect->get_object(), "Draw")) {
					gs_draw(gs_draw_mode::GS_TRIS, 0, _vb->size());
				}

				// Clear Render Target
				_d3d_context->OMSetRenderTargets(0, nullptr, nullptr);
			}

			{ // Copy subregion
				auto      gdbg2 = gs::debug_marker(gs::debug_color_cache_render, "Copy");
				D3D11_BOX box;
				box.left   = 0;
				box.right  = width;
				box.top    = 0;
				box.bottom = height;
				box.front  = 0;
				box.back   = 1;

				_d3d_context->CopySubresourceRegion(target_texture, UINT(lvl), 0, 0, 0, _d3d_rtt, 0, &box);
			}

			_d3d_context->Flush();
		}

		gs_matrix_pop();
		gs_projection_pop();
		gs_viewport_pop();
		gs_load_indexbuffer(nullptr);
		gs_load_vertexbuffer(nullptr);
		gs_set_render_target(prev_rt, prev_zs);
	}
#endif
	if (gs_get_device_type() == GS_DEVICE_OPENGL) {
		// Not Implemented Yet.
	}
}
