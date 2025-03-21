/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017-2018 Michael Fabian Dirks
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

#include "filter-sdf-effects.hpp"
#include "strings.hpp"
#include <stdexcept>
#include "obs/gs/gs-helper.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<filter::sdf_effects> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

// Translation Strings
#define ST_I18N "Filter.SDFEffects"

#define ST_I18N_SHADOW_INNER "Filter.SDFEffects.Shadow.Inner"
#define ST_KEY_SHADOW_INNER "Filter.SDFEffects.Shadow.Inner"
#define ST_I18N_SHADOW_INNER_RANGE_MINIMUM "Filter.SDFEffects.Shadow.Inner.Range.Minimum"
#define ST_KEY_SHADOW_INNER_RANGE_MINIMUM "Filter.SDFEffects.Shadow.Inner.Range.Minimum"
#define ST_I18N_SHADOW_INNER_RANGE_MAXIMUM "Filter.SDFEffects.Shadow.Inner.Range.Maximum"
#define ST_KEY_SHADOW_INNER_RANGE_MAXIMUM "Filter.SDFEffects.Shadow.Inner.Range.Maximum"
#define ST_I18N_SHADOW_INNER_OFFSET_X "Filter.SDFEffects.Shadow.Inner.Offset.X"
#define ST_KEY_SHADOW_INNER_OFFSET_X "Filter.SDFEffects.Shadow.Inner.Offset.X"
#define ST_I18N_SHADOW_INNER_OFFSET_Y "Filter.SDFEffects.Shadow.Inner.Offset.Y"
#define ST_KEY_SHADOW_INNER_OFFSET_Y "Filter.SDFEffects.Shadow.Inner.Offset.Y"
#define ST_I18N_SHADOW_INNER_COLOR "Filter.SDFEffects.Shadow.Inner.Color"
#define ST_KEY_SHADOW_INNER_COLOR "Filter.SDFEffects.Shadow.Inner.Color"
#define ST_I18N_SHADOW_INNER_ALPHA "Filter.SDFEffects.Shadow.Inner.Alpha"
#define ST_KEY_SHADOW_INNER_ALPHA "Filter.SDFEffects.Shadow.Inner.Alpha"

#define ST_I18N_SHADOW_OUTER "Filter.SDFEffects.Shadow.Outer"
#define ST_KEY_SHADOW_OUTER "Filter.SDFEffects.Shadow.Outer"
#define ST_I18N_SHADOW_OUTER_RANGE_MINIMUM "Filter.SDFEffects.Shadow.Outer.Range.Minimum"
#define ST_KEY_SHADOW_OUTER_RANGE_MINIMUM "Filter.SDFEffects.Shadow.Outer.Range.Minimum"
#define ST_I18N_SHADOW_OUTER_RANGE_MAXIMUM "Filter.SDFEffects.Shadow.Outer.Range.Maximum"
#define ST_KEY_SHADOW_OUTER_RANGE_MAXIMUM "Filter.SDFEffects.Shadow.Outer.Range.Maximum"
#define ST_I18N_SHADOW_OUTER_OFFSET_X "Filter.SDFEffects.Shadow.Outer.Offset.X"
#define ST_KEY_SHADOW_OUTER_OFFSET_X "Filter.SDFEffects.Shadow.Outer.Offset.X"
#define ST_I18N_SHADOW_OUTER_OFFSET_Y "Filter.SDFEffects.Shadow.Outer.Offset.Y"
#define ST_KEY_SHADOW_OUTER_OFFSET_Y "Filter.SDFEffects.Shadow.Outer.Offset.Y"
#define ST_I18N_SHADOW_OUTER_COLOR "Filter.SDFEffects.Shadow.Outer.Color"
#define ST_KEY_SHADOW_OUTER_COLOR "Filter.SDFEffects.Shadow.Outer.Color"
#define ST_I18N_SHADOW_OUTER_ALPHA "Filter.SDFEffects.Shadow.Outer.Alpha"
#define ST_KEY_SHADOW_OUTER_ALPHA "Filter.SDFEffects.Shadow.Outer.Alpha"

#define ST_I18N_GLOW_INNER "Filter.SDFEffects.Glow.Inner"
#define ST_KEY_GLOW_INNER "Filter.SDFEffects.Glow.Inner"
#define ST_I18N_GLOW_INNER_COLOR "Filter.SDFEffects.Glow.Inner.Color"
#define ST_KEY_GLOW_INNER_COLOR "Filter.SDFEffects.Glow.Inner.Color"
#define ST_I18N_GLOW_INNER_ALPHA "Filter.SDFEffects.Glow.Inner.Alpha"
#define ST_KEY_GLOW_INNER_ALPHA "Filter.SDFEffects.Glow.Inner.Alpha"
#define ST_I18N_GLOW_INNER_WIDTH "Filter.SDFEffects.Glow.Inner.Width"
#define ST_KEY_GLOW_INNER_WIDTH "Filter.SDFEffects.Glow.Inner.Width"
#define ST_I18N_GLOW_INNER_SHARPNESS "Filter.SDFEffects.Glow.Inner.Sharpness"
#define ST_KEY_GLOW_INNER_SHARPNESS "Filter.SDFEffects.Glow.Inner.Sharpness"

#define ST_I18N_GLOW_OUTER "Filter.SDFEffects.Glow.Outer"
#define ST_KEY_GLOW_OUTER "Filter.SDFEffects.Glow.Outer"
#define ST_I18N_GLOW_OUTER_COLOR "Filter.SDFEffects.Glow.Outer.Color"
#define ST_KEY_GLOW_OUTER_COLOR "Filter.SDFEffects.Glow.Outer.Color"
#define ST_I18N_GLOW_OUTER_ALPHA "Filter.SDFEffects.Glow.Outer.Alpha"
#define ST_KEY_GLOW_OUTER_ALPHA "Filter.SDFEffects.Glow.Outer.Alpha"
#define ST_I18N_GLOW_OUTER_WIDTH "Filter.SDFEffects.Glow.Outer.Width"
#define ST_KEY_GLOW_OUTER_WIDTH "Filter.SDFEffects.Glow.Outer.Width"
#define ST_I18N_GLOW_OUTER_SHARPNESS "Filter.SDFEffects.Glow.Outer.Sharpness"
#define ST_KEY_GLOW_OUTER_SHARPNESS "Filter.SDFEffects.Glow.Outer.Sharpness"

#define ST_I18N_OUTLINE "Filter.SDFEffects.Outline"
#define ST_KEY_OUTLINE "Filter.SDFEffects.Outline"
#define ST_I18N_OUTLINE_COLOR "Filter.SDFEffects.Outline.Color"
#define ST_KEY_OUTLINE_COLOR "Filter.SDFEffects.Outline.Color"
#define ST_I18N_OUTLINE_ALPHA "Filter.SDFEffects.Outline.Alpha"
#define ST_KEY_OUTLINE_ALPHA "Filter.SDFEffects.Outline.Alpha"
#define ST_I18N_OUTLINE_WIDTH "Filter.SDFEffects.Outline.Width"
#define ST_KEY_OUTLINE_WIDTH "Filter.SDFEffects.Outline.Width"
#define ST_I18N_OUTLINE_OFFSET "Filter.SDFEffects.Outline.Offset"
#define ST_KEY_OUTLINE_OFFSET "Filter.SDFEffects.Outline.Offset"
#define ST_I18N_OUTLINE_SHARPNESS "Filter.SDFEffects.Outline.Sharpness"
#define ST_KEY_OUTLINE_SHARPNESS "Filter.SDFEffects.Outline.Sharpness"

#define ST_I18N_SDF_SCALE "Filter.SDFEffects.SDF.Scale"
#define ST_KEY_SDF_SCALE "Filter.SDFEffects.SDF.Scale"
#define ST_I18N_SDF_THRESHOLD "Filter.SDFEffects.SDF.Threshold"
#define ST_KEY_SDF_THRESHOLD "Filter.SDFEffects.SDF.Threshold"

using namespace streamfx::filter::sdf_effects;

static constexpr std::string_view HELP_URL = "https://github.com/Xaymar/obs-StreamFX/wiki/Filter-SDF-Effects";

sdf_effects_instance::sdf_effects_instance(obs_data_t* settings, obs_source_t* self)
	: obs::source_instance(settings, self), _source_rendered(false), _sdf_scale(1.0), _sdf_threshold(),
	  _output_rendered(false), _inner_shadow(false), _inner_shadow_color(), _inner_shadow_range_min(),
	  _inner_shadow_range_max(), _inner_shadow_offset_x(), _inner_shadow_offset_y(), _outer_shadow(false),
	  _outer_shadow_color(), _outer_shadow_range_min(), _outer_shadow_range_max(), _outer_shadow_offset_x(),
	  _outer_shadow_offset_y(), _inner_glow(false), _inner_glow_color(), _inner_glow_width(), _inner_glow_sharpness(),
	  _inner_glow_sharpness_inv(), _outer_glow(false), _outer_glow_color(), _outer_glow_width(),
	  _outer_glow_sharpness(), _outer_glow_sharpness_inv(), _outline(false), _outline_color(), _outline_width(),
	  _outline_offset(), _outline_sharpness(), _outline_sharpness_inv()
{
	{
		auto gctx        = streamfx::obs::gs::context();
		vec4 transparent = {0, 0, 0, 0};

		_source_rt = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
		_sdf_write = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA32F, GS_ZS_NONE);
		_sdf_read  = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA32F, GS_ZS_NONE);
		_output_rt = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);

		std::shared_ptr<streamfx::obs::gs::rendertarget> initialize_rts[] = {_source_rt, _sdf_write, _sdf_read,
																			 _output_rt};
		for (auto rt : initialize_rts) {
			auto op = rt->render(1, 1);
			gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &transparent, 0, 0);
		}

		std::pair<const char*, streamfx::obs::gs::effect&> load_arr[] = {
			{"effects/sdf/sdf-producer.effect", _sdf_producer_effect},
			{"effects/sdf/sdf-consumer.effect", _sdf_consumer_effect},
		};
		for (auto& kv : load_arr) {
			auto file = streamfx::data_file_path(kv.first);
			try {
				kv.second = streamfx::obs::gs::effect::create(file);
			} catch (std::exception& ex) {
				D_LOG_ERROR("Error loading '%s': %s", file.u8string().c_str(), ex.what());
				throw;
			}
		}
	}

	update(settings);
}

sdf_effects_instance::~sdf_effects_instance() {}

void sdf_effects_instance::load(obs_data_t* settings)
{
	update(settings);
}

void sdf_effects_instance::migrate(obs_data_t* data, uint64_t version) {}

void sdf_effects_instance::update(obs_data_t* data)
{
	{
		_outer_shadow =
			obs_data_get_bool(data, ST_KEY_SHADOW_OUTER)
			&& (obs_data_get_double(data, ST_KEY_SHADOW_OUTER_ALPHA) >= std::numeric_limits<double_t>::epsilon());
		{
			struct cs {
				uint8_t r, g, b, a;
			};
			union {
				uint32_t color;
				uint8_t  channel[4];
				cs       c;
			};
			color                 = uint32_t(obs_data_get_int(data, ST_KEY_SHADOW_OUTER_COLOR));
			_outer_shadow_color.x = float_t(c.r / 255.0);
			_outer_shadow_color.y = float_t(c.g / 255.0);
			_outer_shadow_color.z = float_t(c.b / 255.0);
			_outer_shadow_color.w = float_t(obs_data_get_double(data, ST_KEY_SHADOW_OUTER_ALPHA) / 100.0);
		}
		_outer_shadow_range_min = float_t(obs_data_get_double(data, ST_KEY_SHADOW_OUTER_RANGE_MINIMUM));
		_outer_shadow_range_max = float_t(obs_data_get_double(data, ST_KEY_SHADOW_OUTER_RANGE_MAXIMUM));
		_outer_shadow_offset_x  = float_t(obs_data_get_double(data, ST_KEY_SHADOW_OUTER_OFFSET_X));
		_outer_shadow_offset_y  = float_t(obs_data_get_double(data, ST_KEY_SHADOW_OUTER_OFFSET_Y));
	}

	{
		_inner_shadow =
			obs_data_get_bool(data, ST_KEY_SHADOW_INNER)
			&& (obs_data_get_double(data, ST_KEY_SHADOW_INNER_ALPHA) >= std::numeric_limits<double_t>::epsilon());
		{
			struct cs {
				uint8_t r, g, b, a;
			};
			union {
				uint32_t color;
				uint8_t  channel[4];
				cs       c;
			};
			color                 = uint32_t(obs_data_get_int(data, ST_KEY_SHADOW_INNER_COLOR));
			_inner_shadow_color.x = float_t(c.r / 255.0);
			_inner_shadow_color.y = float_t(c.g / 255.0);
			_inner_shadow_color.z = float_t(c.b / 255.0);
			_inner_shadow_color.w = float_t(obs_data_get_double(data, ST_KEY_SHADOW_INNER_ALPHA) / 100.0);
		}
		_inner_shadow_range_min = float_t(obs_data_get_double(data, ST_KEY_SHADOW_INNER_RANGE_MINIMUM));
		_inner_shadow_range_max = float_t(obs_data_get_double(data, ST_KEY_SHADOW_INNER_RANGE_MAXIMUM));
		_inner_shadow_offset_x  = float_t(obs_data_get_double(data, ST_KEY_SHADOW_INNER_OFFSET_X));
		_inner_shadow_offset_y  = float_t(obs_data_get_double(data, ST_KEY_SHADOW_INNER_OFFSET_Y));
	}

	{
		_outer_glow =
			obs_data_get_bool(data, ST_KEY_GLOW_OUTER)
			&& (obs_data_get_double(data, ST_KEY_GLOW_OUTER_ALPHA) >= std::numeric_limits<double_t>::epsilon());
		{
			struct cs {
				uint8_t r, g, b, a;
			};
			union {
				uint32_t color;
				uint8_t  channel[4];
				cs       c;
			};
			color               = uint32_t(obs_data_get_int(data, ST_KEY_GLOW_OUTER_COLOR));
			_outer_glow_color.x = float_t(c.r / 255.0);
			_outer_glow_color.y = float_t(c.g / 255.0);
			_outer_glow_color.z = float_t(c.b / 255.0);
			_outer_glow_color.w = float_t(obs_data_get_double(data, ST_KEY_GLOW_OUTER_ALPHA) / 100.0);
		}
		_outer_glow_width         = float_t(obs_data_get_double(data, ST_KEY_GLOW_OUTER_WIDTH));
		_outer_glow_sharpness     = float_t(obs_data_get_double(data, ST_KEY_GLOW_OUTER_SHARPNESS) / 100.0);
		_outer_glow_sharpness_inv = float_t(1.0f / (1.0f - _outer_glow_sharpness));
		if (_outer_glow_sharpness >= (1.0f - std::numeric_limits<float_t>::epsilon())) {
			_outer_glow_sharpness = 1.0f - std::numeric_limits<float_t>::epsilon();
		}
	}

	{
		_inner_glow =
			obs_data_get_bool(data, ST_KEY_GLOW_INNER)
			&& (obs_data_get_double(data, ST_KEY_GLOW_INNER_ALPHA) >= std::numeric_limits<double_t>::epsilon());
		{
			struct cs {
				uint8_t r, g, b, a;
			};
			union {
				uint32_t color;
				uint8_t  channel[4];
				cs       c;
			};
			color               = uint32_t(obs_data_get_int(data, ST_KEY_GLOW_INNER_COLOR));
			_inner_glow_color.x = float_t(c.r / 255.0);
			_inner_glow_color.y = float_t(c.g / 255.0);
			_inner_glow_color.z = float_t(c.b / 255.0);
			_inner_glow_color.w = float_t(obs_data_get_double(data, ST_KEY_GLOW_INNER_ALPHA) / 100.0);
		}
		_inner_glow_width         = float_t(obs_data_get_double(data, ST_KEY_GLOW_INNER_WIDTH));
		_inner_glow_sharpness     = float_t(obs_data_get_double(data, ST_KEY_GLOW_INNER_SHARPNESS) / 100.0);
		_inner_glow_sharpness_inv = float_t(1.0f / (1.0f - _inner_glow_sharpness));
		if (_inner_glow_sharpness >= (1.0f - std::numeric_limits<float_t>::epsilon())) {
			_inner_glow_sharpness = 1.0f - std::numeric_limits<float_t>::epsilon();
		}
	}

	{
		_outline = obs_data_get_bool(data, ST_KEY_OUTLINE)
				   && (obs_data_get_double(data, ST_KEY_OUTLINE_ALPHA) >= std::numeric_limits<double_t>::epsilon());
		{
			struct cs {
				uint8_t r, g, b, a;
			};
			union {
				uint32_t color;
				uint8_t  channel[4];
				cs       c;
			};
			color            = uint32_t(obs_data_get_int(data, ST_KEY_OUTLINE_COLOR));
			_outline_color.x = float_t(c.r / 255.0);
			_outline_color.y = float_t(c.g / 255.0);
			_outline_color.z = float_t(c.b / 255.0);
			_outline_color.w = float_t(obs_data_get_double(data, ST_KEY_OUTLINE_ALPHA) / 100.0);
		}
		_outline_width         = float_t(obs_data_get_double(data, ST_KEY_OUTLINE_WIDTH));
		_outline_offset        = float_t(obs_data_get_double(data, ST_KEY_OUTLINE_OFFSET));
		_outline_sharpness     = float_t(obs_data_get_double(data, ST_KEY_OUTLINE_SHARPNESS) / 100.0);
		_outline_sharpness_inv = float_t(1.0f / (1.0f - _outline_sharpness));
		if (_outline_sharpness >= (1.0f - std::numeric_limits<float_t>::epsilon())) {
			_outline_sharpness = 1.0f - std::numeric_limits<float_t>::epsilon();
		}
	}

	_sdf_scale     = double_t(obs_data_get_double(data, ST_KEY_SDF_SCALE) / 100.0);
	_sdf_threshold = float_t(obs_data_get_double(data, ST_KEY_SDF_THRESHOLD) / 100.0);
}

void sdf_effects_instance::video_tick(float_t)
{
	if (obs_source_t* target = obs_filter_get_target(_self); target != nullptr) {
		_source_rendered = false;
		_output_rendered = false;
	}
}

void sdf_effects_instance::video_render(gs_effect_t* effect)
{
	obs_source_t* parent         = obs_filter_get_parent(_self);
	obs_source_t* target         = obs_filter_get_target(_self);
	uint32_t      baseW          = obs_source_get_base_width(target);
	uint32_t      baseH          = obs_source_get_base_height(target);
	gs_effect_t*  final_effect   = effect ? effect : obs_get_base_effect(obs_base_effect::OBS_EFFECT_DEFAULT);
	gs_effect_t*  default_effect = obs_get_base_effect(obs_base_effect::OBS_EFFECT_DEFAULT);

	if (!_self || !parent || !target || !baseW || !baseH || !final_effect) {
		obs_source_skip_video_filter(_self);
		return;
	}

#ifdef ENABLE_PROFILING
	streamfx::obs::gs::debug_marker gdmp{streamfx::obs::gs::debug_color_source, "SDF Effects '%s' on '%s'",
										 obs_source_get_name(_self), obs_source_get_name(obs_filter_get_parent(_self))};
#endif

	auto gctx              = streamfx::obs::gs::context();
	vec4 color_transparent = {0, 0, 0, 0};

	try {
		gs_blend_state_push();
		gs_reset_blend_state();
		gs_enable_blending(false);
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		gs_set_cull_mode(GS_NEITHER);
		gs_enable_color(true, true, true, true);
		gs_enable_depth_test(false);
		gs_depth_function(GS_ALWAYS);
		gs_enable_stencil_test(false);
		gs_enable_stencil_write(false);
		gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
		gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

		if (!_source_rendered) {
			// Store input texture.
			{
#ifdef ENABLE_PROFILING
				streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_cache, "Cache"};
#endif

				auto op = _source_rt->render(baseW, baseH);
				gs_ortho(0, static_cast<float>(baseW), 0, static_cast<float>(baseH), -1, 1);
				gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &color_transparent, 0, 0);

				if (obs_source_process_filter_begin(_self, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
					obs_source_process_filter_end(_self, final_effect, baseW, baseH);
				} else {
					throw std::runtime_error("failed to process source");
				}
			}
			_source_rt->get_texture(_source_texture);
			if (!_source_texture) {
				throw std::runtime_error("failed to draw source");
			}

			// Generate SDF Buffers
			{
				_sdf_read->get_texture(_sdf_texture);
				if (!_sdf_texture) {
					throw std::runtime_error("SDF Backbuffer empty");
				}

				if (!_sdf_producer_effect) {
					throw std::runtime_error("SDF Effect no loaded");
				}

				// Scale SDF Size
				double_t sdfW, sdfH;
				sdfW = baseW * _sdf_scale;
				sdfH = baseH * _sdf_scale;
				if (sdfW <= 1) {
					sdfW = 1.0;
				}
				if (sdfH <= 1) {
					sdfH = 1.0;
				}

				{
#ifdef ENABLE_PROFILING
					streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_convert,
														"Update Distance Field"};
#endif

					auto op = _sdf_write->render(uint32_t(sdfW), uint32_t(sdfH));
					gs_ortho(0, 1, 0, 1, -1, 1);
					gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &color_transparent, 0, 0);

					_sdf_producer_effect.get_parameter("_image").set_texture(_source_texture);
					_sdf_producer_effect.get_parameter("_size").set_float2(float_t(sdfW), float_t(sdfH));
					_sdf_producer_effect.get_parameter("_sdf").set_texture(_sdf_texture);
					_sdf_producer_effect.get_parameter("_threshold").set_float(_sdf_threshold);

					while (gs_effect_loop(_sdf_producer_effect.get_object(), "Draw")) {
						streamfx::gs_draw_fullscreen_tri();
					}
				}
				std::swap(_sdf_read, _sdf_write);
				_sdf_read->get_texture(_sdf_texture);
				if (!_sdf_texture) {
					throw std::runtime_error("SDF Backbuffer empty");
				}
			}

			_source_rendered = true;
		}

		gs_blend_state_pop();
	} catch (...) {
		gs_blend_state_pop();
		obs_source_skip_video_filter(_self);
		return;
	}

	if (!_output_rendered) {
		_output_texture = _source_texture;

		if (!_sdf_consumer_effect) {
			obs_source_skip_video_filter(_self);
			return;
		}

		gs_blend_state_push();
		gs_reset_blend_state();
		gs_enable_color(true, true, true, true);
		gs_enable_depth_test(false);
		gs_set_cull_mode(GS_NEITHER);

		// SDF Effects Stack:
		//   Normal Source
		//   Outer Shadow
		//   Inner Shadow
		//   Outer Glow
		//   Inner Glow
		//   Outline

		// Optimized Render path.
		try {
#ifdef ENABLE_PROFILING
			streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_convert, "Calculate"};
#endif

			auto op = _output_rt->render(baseW, baseH);
			gs_ortho(0, 1, 0, 1, 0, 1);

			gs_enable_blending(false);
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
			auto param = gs_effect_get_param_by_name(default_effect, "image");
			if (param) {
				gs_effect_set_texture(param, _output_texture->get_object());
			}
			while (gs_effect_loop(default_effect, "Draw")) {
				streamfx::gs_draw_fullscreen_tri();
			}

			gs_enable_blending(true);
			gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_ONE);
			if (_outer_shadow) {
				_sdf_consumer_effect.get_parameter("pSDFTexture").set_texture(_sdf_texture);
				_sdf_consumer_effect.get_parameter("pSDFThreshold").set_float(_sdf_threshold);
				_sdf_consumer_effect.get_parameter("pImageTexture").set_texture(_source_texture->get_object());
				_sdf_consumer_effect.get_parameter("pShadowColor").set_float4(_outer_shadow_color);
				_sdf_consumer_effect.get_parameter("pShadowMin").set_float(_outer_shadow_range_min);
				_sdf_consumer_effect.get_parameter("pShadowMax").set_float(_outer_shadow_range_max);
				_sdf_consumer_effect.get_parameter("pShadowOffset")
					.set_float2(_outer_shadow_offset_x / float_t(baseW), _outer_shadow_offset_y / float_t(baseH));
				while (gs_effect_loop(_sdf_consumer_effect.get_object(), "ShadowOuter")) {
					streamfx::gs_draw_fullscreen_tri();
				}
			}
			if (_inner_shadow) {
				_sdf_consumer_effect.get_parameter("pSDFTexture").set_texture(_sdf_texture);
				_sdf_consumer_effect.get_parameter("pSDFThreshold").set_float(_sdf_threshold);
				_sdf_consumer_effect.get_parameter("pImageTexture").set_texture(_source_texture->get_object());
				_sdf_consumer_effect.get_parameter("pShadowColor").set_float4(_inner_shadow_color);
				_sdf_consumer_effect.get_parameter("pShadowMin").set_float(_inner_shadow_range_min);
				_sdf_consumer_effect.get_parameter("pShadowMax").set_float(_inner_shadow_range_max);
				_sdf_consumer_effect.get_parameter("pShadowOffset")
					.set_float2(_inner_shadow_offset_x / float_t(baseW), _inner_shadow_offset_y / float_t(baseH));
				while (gs_effect_loop(_sdf_consumer_effect.get_object(), "ShadowInner")) {
					streamfx::gs_draw_fullscreen_tri();
				}
			}
			if (_outer_glow) {
				_sdf_consumer_effect.get_parameter("pSDFTexture").set_texture(_sdf_texture);
				_sdf_consumer_effect.get_parameter("pSDFThreshold").set_float(_sdf_threshold);
				_sdf_consumer_effect.get_parameter("pImageTexture").set_texture(_source_texture->get_object());
				_sdf_consumer_effect.get_parameter("pGlowColor").set_float4(_outer_glow_color);
				_sdf_consumer_effect.get_parameter("pGlowWidth").set_float(_outer_glow_width);
				_sdf_consumer_effect.get_parameter("pGlowSharpness").set_float(_outer_glow_sharpness);
				_sdf_consumer_effect.get_parameter("pGlowSharpnessInverse").set_float(_outer_glow_sharpness_inv);
				while (gs_effect_loop(_sdf_consumer_effect.get_object(), "GlowOuter")) {
					streamfx::gs_draw_fullscreen_tri();
				}
			}
			if (_inner_glow) {
				_sdf_consumer_effect.get_parameter("pSDFTexture").set_texture(_sdf_texture);
				_sdf_consumer_effect.get_parameter("pSDFThreshold").set_float(_sdf_threshold);
				_sdf_consumer_effect.get_parameter("pImageTexture").set_texture(_source_texture->get_object());
				_sdf_consumer_effect.get_parameter("pGlowColor").set_float4(_inner_glow_color);
				_sdf_consumer_effect.get_parameter("pGlowWidth").set_float(_inner_glow_width);
				_sdf_consumer_effect.get_parameter("pGlowSharpness").set_float(_inner_glow_sharpness);
				_sdf_consumer_effect.get_parameter("pGlowSharpnessInverse").set_float(_inner_glow_sharpness_inv);
				while (gs_effect_loop(_sdf_consumer_effect.get_object(), "GlowInner")) {
					streamfx::gs_draw_fullscreen_tri();
				}
			}
			if (_outline) {
				_sdf_consumer_effect.get_parameter("pSDFTexture").set_texture(_sdf_texture);
				_sdf_consumer_effect.get_parameter("pSDFThreshold").set_float(_sdf_threshold);
				_sdf_consumer_effect.get_parameter("pImageTexture").set_texture(_source_texture->get_object());
				_sdf_consumer_effect.get_parameter("pOutlineColor").set_float4(_outline_color);
				_sdf_consumer_effect.get_parameter("pOutlineWidth").set_float(_outline_width);
				_sdf_consumer_effect.get_parameter("pOutlineOffset").set_float(_outline_offset);
				_sdf_consumer_effect.get_parameter("pOutlineSharpness").set_float(_outline_sharpness);
				_sdf_consumer_effect.get_parameter("pOutlineSharpnessInverse").set_float(_outline_sharpness_inv);
				while (gs_effect_loop(_sdf_consumer_effect.get_object(), "Outline")) {
					streamfx::gs_draw_fullscreen_tri();
				}
			}
		} catch (...) {
		}

		_output_rt->get_texture(_output_texture);

		gs_blend_state_pop();
		_output_rendered = true;
	}

	if (!_output_texture) {
		obs_source_skip_video_filter(_self);
		return;
	}

	{
#ifdef ENABLE_PROFILING
		streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_render, "Render"};
#endif

		gs_eparam_t* ep = gs_effect_get_param_by_name(final_effect, "image");
		if (ep) {
			gs_effect_set_texture(ep, _output_texture->get_object());
		}
		while (gs_effect_loop(final_effect, "Draw")) {
			gs_draw_sprite(0, 0, baseW, baseH);
		}
	}
}

sdf_effects_factory::sdf_effects_factory()
{
	_info.id           = S_PREFIX "filter-sdf-effects";
	_info.type         = OBS_SOURCE_TYPE_FILTER;
	_info.output_flags = OBS_SOURCE_VIDEO;

	set_resolution_enabled(false);
	finish_setup();
	register_proxy("obs-stream-effects-filter-sdf-effects");
}

sdf_effects_factory::~sdf_effects_factory() {}

const char* sdf_effects_factory::get_name()
{
	return D_TRANSLATE(ST_I18N);
}

void sdf_effects_factory::get_defaults2(obs_data_t* data)
{
	obs_data_set_default_bool(data, ST_KEY_SHADOW_OUTER, false);
	obs_data_set_default_int(data, ST_KEY_SHADOW_OUTER_COLOR, 0x00000000);
	obs_data_set_default_double(data, ST_KEY_SHADOW_OUTER_ALPHA, 100.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_OUTER_RANGE_MINIMUM, 0.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_OUTER_RANGE_MAXIMUM, 4.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_OUTER_OFFSET_X, 0.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_OUTER_OFFSET_Y, 0.0);

	obs_data_set_default_bool(data, ST_KEY_SHADOW_INNER, false);
	obs_data_set_default_int(data, ST_KEY_SHADOW_INNER_COLOR, 0x00000000);
	obs_data_set_default_double(data, ST_KEY_SHADOW_INNER_ALPHA, 100.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_INNER_RANGE_MINIMUM, 0.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_INNER_RANGE_MAXIMUM, 4.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_INNER_OFFSET_X, 0.0);
	obs_data_set_default_double(data, ST_KEY_SHADOW_INNER_OFFSET_Y, 0.0);

	obs_data_set_default_bool(data, ST_KEY_GLOW_OUTER, false);
	obs_data_set_default_int(data, ST_KEY_GLOW_OUTER_COLOR, 0xFFFFFFFF);
	obs_data_set_default_double(data, ST_KEY_GLOW_OUTER_ALPHA, 100.0);
	obs_data_set_default_double(data, ST_KEY_GLOW_OUTER_WIDTH, 4.0);
	obs_data_set_default_double(data, ST_KEY_GLOW_OUTER_SHARPNESS, 50.0);

	obs_data_set_default_bool(data, ST_KEY_GLOW_INNER, false);
	obs_data_set_default_int(data, ST_KEY_GLOW_INNER_COLOR, 0xFFFFFFFF);
	obs_data_set_default_double(data, ST_KEY_GLOW_INNER_ALPHA, 100.0);
	obs_data_set_default_double(data, ST_KEY_GLOW_INNER_WIDTH, 4.0);
	obs_data_set_default_double(data, ST_KEY_GLOW_INNER_SHARPNESS, 50.0);

	obs_data_set_default_bool(data, ST_KEY_OUTLINE, false);
	obs_data_set_default_int(data, ST_KEY_OUTLINE_COLOR, 0x00000000);
	obs_data_set_default_double(data, ST_KEY_OUTLINE_ALPHA, 100.0);
	obs_data_set_default_double(data, ST_KEY_OUTLINE_WIDTH, 4.0);
	obs_data_set_default_double(data, ST_KEY_OUTLINE_OFFSET, 0.0);
	obs_data_set_default_double(data, ST_KEY_OUTLINE_SHARPNESS, 50.0);

	obs_data_set_default_double(data, ST_KEY_SDF_SCALE, 100.0);
	obs_data_set_default_double(data, ST_KEY_SDF_THRESHOLD, 50.0);
}

obs_properties_t* sdf_effects_factory::get_properties2(sdf_effects_instance* data)
{
	obs_properties_t* prs = obs_properties_create();
	obs_property_t*   p   = nullptr;

#ifdef ENABLE_FRONTEND
	{
		obs_properties_add_button2(prs, S_MANUAL_OPEN, D_TRANSLATE(S_MANUAL_OPEN),
								   streamfx::filter::sdf_effects::sdf_effects_factory::on_manual_open, nullptr);
	}
#endif

	{ // Shadow Outer
		auto pr = obs_properties_create();
		obs_properties_add_group(prs, ST_KEY_SHADOW_OUTER, D_TRANSLATE(ST_I18N_SHADOW_OUTER), OBS_GROUP_CHECKABLE, pr);

		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_OUTER_RANGE_MINIMUM,
										D_TRANSLATE(ST_I18N_SHADOW_OUTER_RANGE_MINIMUM), -16.0, 16.0, 0.01);
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_OUTER_RANGE_MAXIMUM,
										D_TRANSLATE(ST_I18N_SHADOW_OUTER_RANGE_MAXIMUM), -16.0, 16.0, 0.01);
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_OUTER_OFFSET_X, D_TRANSLATE(ST_I18N_SHADOW_OUTER_OFFSET_X),
										-100.0, 100.0, 0.01);
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_OUTER_OFFSET_Y, D_TRANSLATE(ST_I18N_SHADOW_OUTER_OFFSET_Y),
										-100.0, 100.0, 0.01);
		obs_properties_add_color(pr, ST_KEY_SHADOW_OUTER_COLOR, D_TRANSLATE(ST_I18N_SHADOW_OUTER_COLOR));
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_OUTER_ALPHA, D_TRANSLATE(ST_I18N_SHADOW_OUTER_ALPHA), 0.0,
										100.0, 0.1);
	}

	{ // Shadow Inner
		auto pr = obs_properties_create();
		obs_properties_add_group(prs, ST_KEY_SHADOW_INNER, D_TRANSLATE(ST_I18N_SHADOW_INNER), OBS_GROUP_CHECKABLE, pr);

		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_INNER_RANGE_MINIMUM,
										D_TRANSLATE(ST_I18N_SHADOW_INNER_RANGE_MINIMUM), -16.0, 16.0, 0.01);
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_INNER_RANGE_MAXIMUM,
										D_TRANSLATE(ST_I18N_SHADOW_INNER_RANGE_MAXIMUM), -16.0, 16.0, 0.01);
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_INNER_OFFSET_X, D_TRANSLATE(ST_I18N_SHADOW_INNER_OFFSET_X),
										-100.0, 100.0, 0.01);
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_INNER_OFFSET_Y, D_TRANSLATE(ST_I18N_SHADOW_INNER_OFFSET_Y),
										-100.0, 100.0, 0.01);
		obs_properties_add_color(pr, ST_KEY_SHADOW_INNER_COLOR, D_TRANSLATE(ST_I18N_SHADOW_INNER_COLOR));
		obs_properties_add_float_slider(pr, ST_KEY_SHADOW_INNER_ALPHA, D_TRANSLATE(ST_I18N_SHADOW_INNER_ALPHA), 0.0,
										100.0, 0.1);
	}

	{ // Glow Outer
		auto pr = obs_properties_create();
		obs_properties_add_group(prs, ST_KEY_GLOW_OUTER, D_TRANSLATE(ST_I18N_GLOW_OUTER), OBS_GROUP_CHECKABLE, pr);

		obs_properties_add_color(pr, ST_KEY_GLOW_OUTER_COLOR, D_TRANSLATE(ST_I18N_GLOW_OUTER_COLOR));
		obs_properties_add_float_slider(pr, ST_KEY_GLOW_OUTER_ALPHA, D_TRANSLATE(ST_I18N_GLOW_OUTER_ALPHA), 0.0, 100.0,
										0.1);
		obs_properties_add_float_slider(pr, ST_KEY_GLOW_OUTER_WIDTH, D_TRANSLATE(ST_I18N_GLOW_OUTER_WIDTH), 0.0, 16.0,
										0.01);
		obs_properties_add_float_slider(pr, ST_KEY_GLOW_OUTER_SHARPNESS, D_TRANSLATE(ST_I18N_GLOW_OUTER_SHARPNESS),
										0.00, 100.0, 0.01);
	}

	{ // Glow Inner
		auto pr = obs_properties_create();
		obs_properties_add_group(prs, ST_KEY_GLOW_INNER, D_TRANSLATE(ST_I18N_GLOW_INNER), OBS_GROUP_CHECKABLE, pr);

		obs_properties_add_color(pr, ST_KEY_GLOW_INNER_COLOR, D_TRANSLATE(ST_I18N_GLOW_INNER_COLOR));
		obs_properties_add_float_slider(pr, ST_KEY_GLOW_INNER_ALPHA, D_TRANSLATE(ST_I18N_GLOW_INNER_ALPHA), 0.0, 100.0,
										0.1);
		obs_properties_add_float_slider(pr, ST_KEY_GLOW_INNER_WIDTH, D_TRANSLATE(ST_I18N_GLOW_INNER_WIDTH), 0.0, 16.0,
										0.01);
		obs_properties_add_float_slider(pr, ST_KEY_GLOW_INNER_SHARPNESS, D_TRANSLATE(ST_I18N_GLOW_INNER_SHARPNESS),
										0.00, 100.0, 0.01);
	}

	{ // Outline
		auto pr = obs_properties_create();
		obs_properties_add_group(prs, ST_KEY_OUTLINE, D_TRANSLATE(ST_I18N_OUTLINE), OBS_GROUP_CHECKABLE, pr);

		obs_properties_add_color(pr, ST_KEY_OUTLINE_COLOR, D_TRANSLATE(ST_I18N_OUTLINE_COLOR));
		obs_properties_add_float_slider(pr, ST_KEY_OUTLINE_ALPHA, D_TRANSLATE(ST_I18N_OUTLINE_ALPHA), 0.0, 100.0, 0.1);

		obs_properties_add_float_slider(pr, ST_KEY_OUTLINE_WIDTH, D_TRANSLATE(ST_I18N_OUTLINE_WIDTH), 0.0, 16.0, 0.01);

		obs_properties_add_float_slider(pr, ST_KEY_OUTLINE_OFFSET, D_TRANSLATE(ST_I18N_OUTLINE_OFFSET), -16.0, 16.0,
										0.01);

		obs_properties_add_float_slider(pr, ST_KEY_OUTLINE_SHARPNESS, D_TRANSLATE(ST_I18N_OUTLINE_SHARPNESS), 0.00,
										100.0, 0.01);
	}

	{ // Advanced Options
		auto pr = obs_properties_create();
		obs_properties_add_group(prs, S_ADVANCED, D_TRANSLATE(S_ADVANCED), OBS_GROUP_NORMAL, pr);

		obs_properties_add_float_slider(pr, ST_KEY_SDF_SCALE, D_TRANSLATE(ST_I18N_SDF_SCALE), 0.1, 500.0, 0.1);
		obs_properties_add_float_slider(pr, ST_KEY_SDF_THRESHOLD, D_TRANSLATE(ST_I18N_SDF_THRESHOLD), 0.0, 100.0, 0.01);
	}

	return prs;
}

#ifdef ENABLE_FRONTEND
bool sdf_effects_factory::on_manual_open(obs_properties_t* props, obs_property_t* property, void* data)
try {
	streamfx::open_url(HELP_URL);
	return false;
} catch (const std::exception& ex) {
	D_LOG_ERROR("Failed to open manual due to error: %s", ex.what());
	return false;
} catch (...) {
	D_LOG_ERROR("Failed to open manual due to unknown error.", "");
	return false;
}
#endif

std::shared_ptr<sdf_effects_factory> _filter_sdf_effects_factory_instance = nullptr;

void streamfx::filter::sdf_effects::sdf_effects_factory::initialize()
try {
	if (!_filter_sdf_effects_factory_instance)
		_filter_sdf_effects_factory_instance = std::make_shared<sdf_effects_factory>();
} catch (const std::exception& ex) {
	D_LOG_ERROR("Failed to initialize due to error: %s", ex.what());
} catch (...) {
	D_LOG_ERROR("Failed to initialize due to unknown error.", "");
}

void streamfx::filter::sdf_effects::sdf_effects_factory::finalize()
{
	_filter_sdf_effects_factory_instance.reset();
}

std::shared_ptr<sdf_effects_factory> streamfx::filter::sdf_effects::sdf_effects_factory::get()
{
	return _filter_sdf_effects_factory_instance;
}
