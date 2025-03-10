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

#include "filter-blur.hpp"
#include "strings.hpp"
#include <cfloat>
#include <cinttypes>
#include <cmath>
#include <map>
#include <stdexcept>
#include "gfx/blur/gfx-blur-box-linear.hpp"
#include "gfx/blur/gfx-blur-box.hpp"
#include "gfx/blur/gfx-blur-dual-filtering.hpp"
#include "gfx/blur/gfx-blur-gaussian-linear.hpp"
#include "gfx/blur/gfx-blur-gaussian.hpp"
#include "obs/gs/gs-helper.hpp"
#include "obs/obs-source-tracker.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<filter::blur> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

// OBS
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <callback/signal.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <util/platform.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Translation Strings
#define ST_I18N "Filter.Blur"

#define ST_I18N_TYPE "Filter.Blur.Type"
#define ST_KEY_TYPE "Filter.Blur.Type"
#define ST_I18N_SUBTYPE "Filter.Blur.SubType"
#define ST_KEY_SUBTYPE "Filter.Blur.SubType"
#define ST_I18N_SIZE "Filter.Blur.Size"
#define ST_KEY_SIZE "Filter.Blur.Size"
#define ST_I18N_ANGLE "Filter.Blur.Angle"
#define ST_KEY_ANGLE "Filter.Blur.Angle"
#define ST_CENTER "Filter.Blur.Center"
#define ST_I18N_CENTER_X "Filter.Blur.Center.X"
#define ST_KEY_CENTER_X "Filter.Blur.Center.X"
#define ST_I18N_CENTER_Y "Filter.Blur.Center.Y"
#define ST_KEY_CENTER_Y "Filter.Blur.Center.Y"
#define ST_I18N_STEPSCALE "Filter.Blur.StepScale"
#define ST_KEY_STEPSCALE "Filter.Blur.StepScale"
#define ST_I18N_STEPSCALE_X "Filter.Blur.StepScale.X"
#define ST_KEY_STEPSCALE_X "Filter.Blur.StepScale.X"
#define ST_I18N_STEPSCALE_Y "Filter.Blur.StepScale.Y"
#define ST_KEY_STEPSCALE_Y "Filter.Blur.StepScale.Y"
#define ST_I18N_MASK "Filter.Blur.Mask"
#define ST_KEY_MASK "Filter.Blur.Mask"
#define ST_I18N_MASK_TYPE "Filter.Blur.Mask.Type"
#define ST_KEY_MASK_TYPE "Filter.Blur.Mask.Type"
#define ST_I18N_MASK_TYPE_REGION "Filter.Blur.Mask.Type.Region"
#define ST_I18N_MASK_TYPE_IMAGE "Filter.Blur.Mask.Type.Image"
#define ST_I18N_MASK_TYPE_SOURCE "Filter.Blur.Mask.Type.Source"
#define ST_I18N_MASK_REGION_LEFT "Filter.Blur.Mask.Region.Left"
#define ST_KEY_MASK_REGION_LEFT "Filter.Blur.Mask.Region.Left"
#define ST_I18N_MASK_REGION_RIGHT "Filter.Blur.Mask.Region.Right"
#define ST_KEY_MASK_REGION_RIGHT "Filter.Blur.Mask.Region.Right"
#define ST_I18N_MASK_REGION_TOP "Filter.Blur.Mask.Region.Top"
#define ST_KEY_MASK_REGION_TOP "Filter.Blur.Mask.Region.Top"
#define ST_I18N_MASK_REGION_BOTTOM "Filter.Blur.Mask.Region.Bottom"
#define ST_KEY_MASK_REGION_BOTTOM "Filter.Blur.Mask.Region.Bottom"
#define ST_I18N_MASK_REGION_FEATHER "Filter.Blur.Mask.Region.Feather"
#define ST_KEY_MASK_REGION_FEATHER "Filter.Blur.Mask.Region.Feather"
#define ST_I18N_MASK_REGION_FEATHER_SHIFT "Filter.Blur.Mask.Region.Feather.Shift"
#define ST_KEY_MASK_REGION_FEATHER_SHIFT "Filter.Blur.Mask.Region.Feather.Shift"
#define ST_I18N_MASK_REGION_INVERT "Filter.Blur.Mask.Region.Invert"
#define ST_KEY_MASK_REGION_INVERT "Filter.Blur.Mask.Region.Invert"
#define ST_I18N_MASK_IMAGE "Filter.Blur.Mask.Image"
#define ST_KEY_MASK_IMAGE "Filter.Blur.Mask.Image"
#define ST_I18N_MASK_SOURCE "Filter.Blur.Mask.Source"
#define ST_KEY_MASK_SOURCE "Filter.Blur.Mask.Source"
#define ST_I18N_MASK_COLOR "Filter.Blur.Mask.Color"
#define ST_KEY_MASK_COLOR "Filter.Blur.Mask.Color"
#define ST_I18N_MASK_ALPHA "Filter.Blur.Mask.Alpha"
#define ST_KEY_MASK_ALPHA "Filter.Blur.Mask.Alpha"
#define ST_I18N_MASK_MULTIPLIER "Filter.Blur.Mask.Multiplier"
#define ST_KEY_MASK_MULTIPLIER "Filter.Blur.Mask.Multiplier"

using namespace streamfx::filter::blur;

static constexpr std::string_view HELP_URL = "https://github.com/Xaymar/obs-StreamFX/wiki/Filter-Blur";

struct local_blur_type_t {
	std::function<::streamfx::gfx::blur::ifactory&()> fn;
	const char*                                       name;
};
struct local_blur_subtype_t {
	::streamfx::gfx::blur::type type;
	const char*                 name;
};

static std::map<std::string, local_blur_type_t> list_of_types = {
	{"box", {&::streamfx::gfx::blur::box_factory::get, S_BLUR_TYPE_BOX}},
	{"box_linear", {&::streamfx::gfx::blur::box_linear_factory::get, S_BLUR_TYPE_BOX_LINEAR}},
	{"gaussian", {&::streamfx::gfx::blur::gaussian_factory::get, S_BLUR_TYPE_GAUSSIAN}},
	{"gaussian_linear", {&::streamfx::gfx::blur::gaussian_linear_factory::get, S_BLUR_TYPE_GAUSSIAN_LINEAR}},
	{"dual_filtering", {&::streamfx::gfx::blur::dual_filtering_factory::get, S_BLUR_TYPE_DUALFILTERING}},
};
static std::map<std::string, local_blur_subtype_t> list_of_subtypes = {
	{"area", {::streamfx::gfx::blur::type::Area, S_BLUR_SUBTYPE_AREA}},
	{"directional", {::streamfx::gfx::blur::type::Directional, S_BLUR_SUBTYPE_DIRECTIONAL}},
	{"rotational", {::streamfx::gfx::blur::type::Rotational, S_BLUR_SUBTYPE_ROTATIONAL}},
	{"zoom", {::streamfx::gfx::blur::type::Zoom, S_BLUR_SUBTYPE_ZOOM}},
};

blur_instance::blur_instance(obs_data_t* settings, obs_source_t* self)
	: obs::source_instance(settings, self), _source_rendered(false), _output_rendered(false)
{
	{
		auto gctx = streamfx::obs::gs::context();

		// Create RenderTargets
		this->_source_rt = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
		this->_output_rt = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);

		// Load Effects
		{
			auto file = streamfx::data_file_path("effects/mask.effect");
			try {
				_effect_mask = streamfx::obs::gs::effect::create(file);
			} catch (std::exception& ex) {
				DLOG_ERROR("Error loading '%s': %s", file.generic_u8string().c_str(), ex.what());
			}
		}
	}

	update(settings);
}

blur_instance::~blur_instance() {}

bool blur_instance::apply_mask_parameters(streamfx::obs::gs::effect effect, gs_texture_t* original_texture,
										  gs_texture_t* blurred_texture)
{
	if (effect.has_parameter("image_orig")) {
		effect.get_parameter("image_orig").set_texture(original_texture);
	}
	if (effect.has_parameter("image_blur")) {
		effect.get_parameter("image_blur").set_texture(blurred_texture);
	}

	// Region
	if (_mask.type == mask_type::Region) {
		if (effect.has_parameter("mask_region_left")) {
			effect.get_parameter("mask_region_left").set_float(_mask.region.left);
		}
		if (effect.has_parameter("mask_region_right")) {
			effect.get_parameter("mask_region_right").set_float(_mask.region.right);
		}
		if (effect.has_parameter("mask_region_top")) {
			effect.get_parameter("mask_region_top").set_float(_mask.region.top);
		}
		if (effect.has_parameter("mask_region_bottom")) {
			effect.get_parameter("mask_region_bottom").set_float(_mask.region.bottom);
		}
		if (effect.has_parameter("mask_region_feather")) {
			effect.get_parameter("mask_region_feather").set_float(_mask.region.feather);
		}
		if (effect.has_parameter("mask_region_feather_shift")) {
			effect.get_parameter("mask_region_feather_shift").set_float(_mask.region.feather_shift);
		}
	}

	// Image
	if (_mask.type == mask_type::Image) {
		if (effect.has_parameter("mask_image")) {
			if (_mask.image.texture) {
				effect.get_parameter("mask_image").set_texture(_mask.image.texture);
			} else {
				effect.get_parameter("mask_image").set_texture(nullptr);
			}
		}
	}

	// Source
	if (_mask.type == mask_type::Source) {
		if (effect.has_parameter("mask_image")) {
			if (_mask.source.texture) {
				effect.get_parameter("mask_image").set_texture(_mask.source.texture);
			} else {
				effect.get_parameter("mask_image").set_texture(nullptr);
			}
		}
	}

	// Shared
	if (effect.has_parameter("mask_color")) {
		effect.get_parameter("mask_color").set_float4(_mask.color.r, _mask.color.g, _mask.color.b, _mask.color.a);
	}
	if (effect.has_parameter("mask_multiplier")) {
		effect.get_parameter("mask_multiplier").set_float(_mask.multiplier);
	}

	return true;
}

void blur_instance::load(obs_data_t* settings)
{
	update(settings);
}

void blur_instance::migrate(obs_data_t* settings, uint64_t version)
{
	// Now we use a fall-through switch to gradually upgrade each known version change.
	switch (version) {
	case 0:
		/// Blur Type
		int64_t old_blur = obs_data_get_int(settings, "Filter.Blur.Type");
		if (old_blur == 0) { // Box
			obs_data_set_string(settings, ST_KEY_TYPE, "box");
		} else if (old_blur == 1) { // Gaussian
			obs_data_set_string(settings, ST_KEY_TYPE, "gaussian");
		} else if (old_blur == 2) { // Bilateral, no longer included.
			obs_data_set_string(settings, ST_KEY_TYPE, "box");
		} else if (old_blur == 3) { // Box Linear
			obs_data_set_string(settings, ST_KEY_TYPE, "box_linear");
		} else if (old_blur == 4) { // Gaussian Linear
			obs_data_set_string(settings, ST_KEY_TYPE, "gaussian_linear");
		} else {
			obs_data_set_string(settings, ST_KEY_TYPE, "box");
		}
		obs_data_unset_user_value(settings, "Filter.Blur.Type");

		/// Directional Blur
		bool directional = obs_data_get_bool(settings, "Filter.Blur.Directional");
		if (directional) {
			obs_data_set_string(settings, ST_KEY_SUBTYPE, "directional");
		} else {
			obs_data_set_string(settings, ST_KEY_SUBTYPE, "area");
		}
		obs_data_unset_user_value(settings, "Filter.Blur.Directional");

		/// Directional Blur Angle
		double_t angle = obs_data_get_double(settings, "Filter.Blur.Directional.Angle");
		obs_data_set_double(settings, ST_KEY_ANGLE, angle);
		obs_data_unset_user_value(settings, "Filter.Blur.Directional.Angle");
	}
}

void blur_instance::update(obs_data_t* settings)
{
	{ // Blur Type
		const char* blur_type      = obs_data_get_string(settings, ST_KEY_TYPE);
		const char* blur_subtype   = obs_data_get_string(settings, ST_KEY_SUBTYPE);
		const char* last_blur_type = obs_data_get_string(settings, ST_KEY_TYPE ".last");

		auto type_found = list_of_types.find(blur_type);
		if (type_found != list_of_types.end()) {
			auto subtype_found = list_of_subtypes.find(blur_subtype);
			if (subtype_found != list_of_subtypes.end()) {
				if ((strcmp(last_blur_type, blur_type) != 0) || (_blur->get_type() != subtype_found->second.type)) {
					if (type_found->second.fn().is_type_supported(subtype_found->second.type)) {
						_blur = type_found->second.fn().create(subtype_found->second.type);
					}
				}
			}
		}
	}

	{ // Blur Parameters
		this->_blur_size          = obs_data_get_double(settings, ST_KEY_SIZE);
		this->_blur_angle         = obs_data_get_double(settings, ST_KEY_ANGLE);
		this->_blur_center.first  = obs_data_get_double(settings, ST_KEY_CENTER_X) / 100.0;
		this->_blur_center.second = obs_data_get_double(settings, ST_KEY_CENTER_Y) / 100.0;

		// Scaling
		this->_blur_step_scaling      = obs_data_get_bool(settings, ST_KEY_STEPSCALE);
		this->_blur_step_scale.first  = obs_data_get_double(settings, ST_KEY_STEPSCALE_X) / 100.0;
		this->_blur_step_scale.second = obs_data_get_double(settings, ST_KEY_STEPSCALE_Y) / 100.0;
	}

	{ // Masking
		_mask.enabled = obs_data_get_bool(settings, ST_KEY_MASK);
		if (_mask.enabled) {
			_mask.type = static_cast<mask_type>(obs_data_get_int(settings, ST_KEY_MASK_TYPE));
			switch (_mask.type) {
			case mask_type::Region:
				_mask.region.left    = float_t(obs_data_get_double(settings, ST_KEY_MASK_REGION_LEFT) / 100.0);
				_mask.region.top     = float_t(obs_data_get_double(settings, ST_KEY_MASK_REGION_TOP) / 100.0);
				_mask.region.right   = 1.0f - float_t(obs_data_get_double(settings, ST_KEY_MASK_REGION_RIGHT) / 100.0);
				_mask.region.bottom  = 1.0f - float_t(obs_data_get_double(settings, ST_KEY_MASK_REGION_BOTTOM) / 100.0);
				_mask.region.feather = float_t(obs_data_get_double(settings, ST_KEY_MASK_REGION_FEATHER) / 100.0);
				_mask.region.feather_shift =
					float_t(obs_data_get_double(settings, ST_KEY_MASK_REGION_FEATHER_SHIFT) / 100.0);
				_mask.region.invert = obs_data_get_bool(settings, ST_KEY_MASK_REGION_INVERT);
				break;
			case mask_type::Image:
				_mask.image.path = obs_data_get_string(settings, ST_KEY_MASK_IMAGE);
				break;
			case mask_type::Source:
				_mask.source.name = obs_data_get_string(settings, ST_KEY_MASK_SOURCE);
				break;
			}
			if ((_mask.type == mask_type::Image) || (_mask.type == mask_type::Source)) {
				uint32_t color   = static_cast<uint32_t>(obs_data_get_int(settings, ST_KEY_MASK_COLOR));
				_mask.color.r    = ((color >> 0) & 0xFF) / 255.0f;
				_mask.color.g    = ((color >> 8) & 0xFF) / 255.0f;
				_mask.color.b    = ((color >> 16) & 0xFF) / 255.0f;
				_mask.color.a    = static_cast<float_t>(obs_data_get_double(settings, ST_KEY_MASK_ALPHA));
				_mask.multiplier = float_t(obs_data_get_double(settings, ST_KEY_MASK_MULTIPLIER));
			}
		}
	}
}

void blur_instance::video_tick(float)
{
	// Blur
	if (_blur) {
		_blur->set_size(_blur_size);
		if (_blur_step_scaling) {
			_blur->set_step_scale(_blur_step_scale.first, _blur_step_scale.second);
		} else {
			_blur->set_step_scale(1.0, 1.0);
		}
		if ((_blur->get_type() == ::streamfx::gfx::blur::type::Directional)
			|| (_blur->get_type() == ::streamfx::gfx::blur::type::Rotational)) {
			auto obj = std::dynamic_pointer_cast<::streamfx::gfx::blur::base_angle>(_blur);
			obj->set_angle(_blur_angle);
		}
		if ((_blur->get_type() == ::streamfx::gfx::blur::type::Zoom)
			|| (_blur->get_type() == ::streamfx::gfx::blur::type::Rotational)) {
			auto obj = std::dynamic_pointer_cast<::streamfx::gfx::blur::base_center>(_blur);
			obj->set_center(_blur_center.first, _blur_center.second);
		}
	}

	// Load Mask
	if (_mask.type == mask_type::Image) {
		if (_mask.image.path_old != _mask.image.path) {
			try {
				_mask.image.texture  = std::make_shared<streamfx::obs::gs::texture>(_mask.image.path);
				_mask.image.path_old = _mask.image.path;
			} catch (...) {
				DLOG_ERROR("<filter-blur> Instance '%s' failed to load image '%s'.", obs_source_get_name(_self),
						   _mask.image.path.c_str());
			}
		}
	} else if (_mask.type == mask_type::Source) {
		if (_mask.source.name_old != _mask.source.name) {
			try {
				_mask.source.source_texture = std::make_shared<streamfx::gfx::source_texture>(_mask.source.name, _self);
				_mask.source.is_scene = (obs_scene_from_source(_mask.source.source_texture->get_object()) != nullptr);
				_mask.source.name_old = _mask.source.name;
			} catch (...) {
				DLOG_ERROR("<filter-blur> Instance '%s' failed to grab source '%s'.", obs_source_get_name(_self),
						   _mask.source.name.c_str());
			}
		}
	}

	_source_rendered = false;
	_output_rendered = false;
}

void blur_instance::video_render(gs_effect_t* effect)
{
	obs_source_t* parent        = obs_filter_get_parent(this->_self);
	obs_source_t* target        = obs_filter_get_target(this->_self);
	gs_effect_t*  defaultEffect = obs_get_base_effect(obs_base_effect::OBS_EFFECT_DEFAULT);
	uint32_t      baseW         = obs_source_get_base_width(target);
	uint32_t      baseH         = obs_source_get_base_height(target);

	// Verify that we can actually run first.
	if (!target || !parent || !this->_self || !this->_blur || (baseW == 0) || (baseH == 0)) {
		obs_source_skip_video_filter(this->_self);
		return;
	}

#ifdef ENABLE_PROFILING
	streamfx::obs::gs::debug_marker gdmp{streamfx::obs::gs::debug_color_source, "Blur '%s'",
										 obs_source_get_name(_self)};
#endif

	if (!_source_rendered) {
		// Source To Texture
		{
#ifdef ENABLE_PROFILING
			streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_cache, "Cache"};
#endif

			if (obs_source_process_filter_begin(this->_self, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
				{
					auto op = this->_source_rt->render(baseW, baseH);

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
					gs_stencil_op(GS_STENCIL_BOTH, GS_KEEP, GS_KEEP, GS_KEEP);

					// Orthographic Camera and clear RenderTarget.
					gs_ortho(0, static_cast<float>(baseW), 0, static_cast<float>(baseH), -1., 1.);
					//gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &black, 0, 0);

					// Render
					obs_source_process_filter_end(this->_self, defaultEffect, baseW, baseH);

					gs_blend_state_pop();
				}

				_source_texture = this->_source_rt->get_texture();
				if (!_source_texture) {
					obs_source_skip_video_filter(this->_self);
					return;
				}
			} else {
				obs_source_skip_video_filter(this->_self);
				return;
			}
		}

		_source_rendered = true;
	}

	if (!_output_rendered) {
		{
#ifdef ENABLE_PROFILING
			streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_convert, "Blur"};
#endif

			_blur->set_input(_source_texture);
			_output_texture = _blur->render();
		}

		// Mask
		if (_mask.enabled) {
#ifdef ENABLE_PROFILING
			streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_convert, "Mask"};
#endif

			gs_blend_state_push();
			gs_reset_blend_state();
			gs_enable_color(true, true, true, true);
			gs_enable_blending(false);
			gs_enable_depth_test(false);
			gs_enable_stencil_test(false);
			gs_enable_stencil_write(false);
			gs_set_cull_mode(GS_NEITHER);
			gs_depth_function(GS_ALWAYS);
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
			gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
			gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

			std::string technique = "";
			switch (this->_mask.type) {
			case mask_type::Region:
				if (this->_mask.region.feather > std::numeric_limits<float_t>::epsilon()) {
					if (this->_mask.region.invert) {
						technique = "RegionFeatherInverted";
					} else {
						technique = "RegionFeather";
					}
				} else {
					if (this->_mask.region.invert) {
						technique = "RegionInverted";
					} else {
						technique = "Region";
					}
				}
				break;
			case mask_type::Image:
			case mask_type::Source:
				technique = "Image";
				break;
			}

			if (_mask.source.source_texture) {
				uint32_t source_width  = obs_source_get_width(this->_mask.source.source_texture->get_object());
				uint32_t source_height = obs_source_get_height(this->_mask.source.source_texture->get_object());

				if (source_width == 0) {
					source_width = baseW;
				}
				if (source_height == 0) {
					source_height = baseH;
				}
				if (this->_mask.source.is_scene) {
					obs_video_info ovi;
					if (obs_get_video_info(&ovi)) {
						source_width  = ovi.base_width;
						source_height = ovi.base_height;
					}
				}

#ifdef ENABLE_PROFILING
				streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_capture, "Capture '%s'",
													obs_source_get_name(_mask.source.source_texture->get_object())};
#endif

				this->_mask.source.texture = this->_mask.source.source_texture->render(source_width, source_height);
			}

			apply_mask_parameters(_effect_mask, _source_texture->get_object(), _output_texture->get_object());

			try {
				auto op = this->_output_rt->render(baseW, baseH);
				gs_ortho(0, 1, 0, 1, -1, 1);

				// Render
				while (gs_effect_loop(_effect_mask.get_object(), technique.c_str())) {
					streamfx::gs_draw_fullscreen_tri();
				}
			} catch (const std::exception&) {
				gs_blend_state_pop();
				obs_source_skip_video_filter(this->_self);
				return;
			}
			gs_blend_state_pop();

			if (!(_output_texture = this->_output_rt->get_texture())) {
				obs_source_skip_video_filter(this->_self);
				return;
			}
		}

		_output_rendered = true;
	}

	// Draw source
	{
#ifdef ENABLE_PROFILING
		streamfx::obs::gs::debug_marker gdm{streamfx::obs::gs::debug_color_render, "Render"};
#endif

		// It is important that we do not modify the blend state here, as it is set correctly by OBS
		gs_set_cull_mode(GS_NEITHER);
		gs_enable_color(true, true, true, true);
		gs_enable_depth_test(false);
		gs_depth_function(GS_ALWAYS);
		gs_enable_stencil_test(false);
		gs_enable_stencil_write(false);
		gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
		gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

		gs_effect_t* finalEffect = effect ? effect : defaultEffect;
		const char*  technique   = "Draw";

		gs_eparam_t* param = gs_effect_get_param_by_name(finalEffect, "image");
		if (!param) {
			DLOG_ERROR("<filter-blur:%s> Failed to set image param.", obs_source_get_name(this->_self));
			obs_source_skip_video_filter(_self);
			return;
		} else {
			gs_effect_set_texture(param, _output_texture->get_object());
		}
		while (gs_effect_loop(finalEffect, technique)) {
			gs_draw_sprite(_output_texture->get_object(), 0, baseW, baseH);
		}
	}
}

blur_factory::blur_factory()
{
	_info.id           = S_PREFIX "filter-blur";
	_info.type         = OBS_SOURCE_TYPE_FILTER;
	_info.output_flags = OBS_SOURCE_VIDEO;

	set_resolution_enabled(false);
	finish_setup();
	register_proxy("obs-stream-effects-filter-blur");
}

blur_factory::~blur_factory() {}

const char* blur_factory::get_name()
{
	return D_TRANSLATE(ST_I18N);
}

void blur_factory::get_defaults2(obs_data_t* settings)
{
	// Type, Subtype
	obs_data_set_default_string(settings, ST_KEY_TYPE, "box");
	obs_data_set_default_string(settings, ST_KEY_SUBTYPE, "area");

	// Parameters
	obs_data_set_default_int(settings, ST_KEY_SIZE, 5);
	obs_data_set_default_double(settings, ST_KEY_ANGLE, 0.);
	obs_data_set_default_double(settings, ST_KEY_CENTER_X, 50.);
	obs_data_set_default_double(settings, ST_KEY_CENTER_Y, 50.);
	obs_data_set_default_bool(settings, ST_KEY_STEPSCALE, false);
	obs_data_set_default_double(settings, ST_KEY_STEPSCALE_X, 1.);
	obs_data_set_default_double(settings, ST_KEY_STEPSCALE_Y, 1.);

	// Masking
	obs_data_set_default_bool(settings, ST_KEY_MASK, false);
	obs_data_set_default_int(settings, ST_KEY_MASK_TYPE, static_cast<int64_t>(mask_type::Region));
	obs_data_set_default_double(settings, ST_KEY_MASK_REGION_LEFT, 0.0);
	obs_data_set_default_double(settings, ST_KEY_MASK_REGION_RIGHT, 0.0);
	obs_data_set_default_double(settings, ST_KEY_MASK_REGION_TOP, 0.0);
	obs_data_set_default_double(settings, ST_KEY_MASK_REGION_BOTTOM, 0.0);
	obs_data_set_default_double(settings, ST_KEY_MASK_REGION_FEATHER, 0.0);
	obs_data_set_default_double(settings, ST_KEY_MASK_REGION_FEATHER_SHIFT, 0.0);
	obs_data_set_default_bool(settings, ST_KEY_MASK_REGION_INVERT, false);
	obs_data_set_default_string(settings, ST_KEY_MASK_IMAGE, streamfx::data_file_path("white.png").u8string().c_str());
	obs_data_set_default_string(settings, ST_KEY_MASK_SOURCE, "");
	obs_data_set_default_int(settings, ST_KEY_MASK_COLOR, 0xFFFFFFFFull);
	obs_data_set_default_double(settings, ST_KEY_MASK_MULTIPLIER, 1.0);
}

bool modified_properties(void*, obs_properties_t* props, obs_property* prop, obs_data_t* settings) noexcept
try {
	obs_property_t* p;
	const char*     propname = obs_property_name(prop);
	const char*     vtype    = obs_data_get_string(settings, ST_KEY_TYPE);
	const char*     vsubtype = obs_data_get_string(settings, ST_KEY_SUBTYPE);

	// Find new Type
	auto type_found = list_of_types.find(vtype);
	if (type_found == list_of_types.end()) {
		return false;
	}

	// Find new Subtype
	auto subtype_found = list_of_subtypes.find(vsubtype);
	if (subtype_found == list_of_subtypes.end()) {
		return false;
	}

	// Blur Type
	if (strcmp(propname, ST_KEY_TYPE) == 0) {
		obs_property_t* prop_subtype = obs_properties_get(props, ST_KEY_SUBTYPE);

		/// Disable unsupported items.
		std::size_t subvalue_idx = 0;
		for (std::size_t idx = 0, edx = obs_property_list_item_count(prop_subtype); idx < edx; idx++) {
			const char* subtype  = obs_property_list_item_string(prop_subtype, idx);
			bool        disabled = false;

			auto subtype_found_idx = list_of_subtypes.find(subtype);
			if (subtype_found_idx != list_of_subtypes.end()) {
				disabled = !type_found->second.fn().is_type_supported(subtype_found_idx->second.type);
			} else {
				disabled = true;
			}

			obs_property_list_item_disable(prop_subtype, idx, disabled);
			if (strcmp(subtype, vsubtype) == 0) {
				subvalue_idx = idx;
			}
		}

		/// Ensure that there is a valid item selected.
		if (obs_property_list_item_disabled(prop_subtype, subvalue_idx)) {
			for (std::size_t idx = 0, edx = obs_property_list_item_count(prop_subtype); idx < edx; idx++) {
				if (!obs_property_list_item_disabled(prop_subtype, idx)) {
					obs_data_set_string(settings, ST_KEY_SUBTYPE, obs_property_list_item_string(prop_subtype, idx));

					// Find new Subtype
					auto subtype_found2 = list_of_subtypes.find(vsubtype);
					if (subtype_found2 == list_of_subtypes.end()) {
						subtype_found = list_of_subtypes.end();
					} else {
						subtype_found = subtype_found2;
					}

					break;
				}
			}
		}
	}

	// Blur Sub-Type
	{
		bool has_angle_support = (subtype_found->second.type == ::streamfx::gfx::blur::type::Directional)
								 || (subtype_found->second.type == ::streamfx::gfx::blur::type::Rotational);
		bool has_center_support = (subtype_found->second.type == ::streamfx::gfx::blur::type::Rotational)
								  || (subtype_found->second.type == ::streamfx::gfx::blur::type::Zoom);
		bool has_stepscale_support = type_found->second.fn().is_step_scale_supported(subtype_found->second.type);
		bool show_scaling          = obs_data_get_bool(settings, ST_KEY_STEPSCALE) && has_stepscale_support;

		/// Size
		p = obs_properties_get(props, ST_KEY_SIZE);
		obs_property_float_set_limits(p, type_found->second.fn().get_min_size(subtype_found->second.type),
									  type_found->second.fn().get_max_size(subtype_found->second.type),
									  type_found->second.fn().get_step_size(subtype_found->second.type));

		/// Angle
		p = obs_properties_get(props, ST_KEY_ANGLE);
		obs_property_set_visible(p, has_angle_support);
		obs_property_float_set_limits(p, type_found->second.fn().get_min_angle(subtype_found->second.type),
									  type_found->second.fn().get_max_angle(subtype_found->second.type),
									  type_found->second.fn().get_step_angle(subtype_found->second.type));

		/// Center, Radius
		obs_property_set_visible(obs_properties_get(props, ST_KEY_CENTER_X), has_center_support);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_CENTER_Y), has_center_support);

		/// Step Scaling
		obs_property_set_visible(obs_properties_get(props, ST_KEY_STEPSCALE), has_stepscale_support);
		p = obs_properties_get(props, ST_KEY_STEPSCALE_X);
		obs_property_set_visible(p, show_scaling);
		obs_property_float_set_limits(p, type_found->second.fn().get_min_step_scale_x(subtype_found->second.type),
									  type_found->second.fn().get_max_step_scale_x(subtype_found->second.type),
									  type_found->second.fn().get_step_step_scale_x(subtype_found->second.type));
		p = obs_properties_get(props, ST_KEY_STEPSCALE_Y);
		obs_property_set_visible(p, show_scaling);
		obs_property_float_set_limits(p, type_found->second.fn().get_min_step_scale_x(subtype_found->second.type),
									  type_found->second.fn().get_max_step_scale_x(subtype_found->second.type),
									  type_found->second.fn().get_step_step_scale_x(subtype_found->second.type));
	}

	{ // Masking
		using namespace ::streamfx::gfx::blur;
		bool      show_mask   = obs_data_get_bool(settings, ST_KEY_MASK);
		mask_type mtype       = static_cast<mask_type>(obs_data_get_int(settings, ST_KEY_MASK_TYPE));
		bool      show_region = (mtype == mask_type::Region) && show_mask;
		bool      show_image  = (mtype == mask_type::Image) && show_mask;
		bool      show_source = (mtype == mask_type::Source) && show_mask;
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_TYPE), show_mask);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_LEFT), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_TOP), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_RIGHT), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_BOTTOM), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_FEATHER), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_FEATHER_SHIFT), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_REGION_INVERT), show_region);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_IMAGE), show_image);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_SOURCE), show_source);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_COLOR), show_image || show_source);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_ALPHA), show_image || show_source);
		obs_property_set_visible(obs_properties_get(props, ST_KEY_MASK_MULTIPLIER), show_image || show_source);
	}

	return true;
} catch (...) {
	DLOG_ERROR("Unexpected exception in modified_properties callback.");
	return false;
}

obs_properties_t* blur_factory::get_properties2(blur_instance* data)
{
	obs_properties_t* pr = obs_properties_create();
	obs_property_t*   p  = NULL;

#ifdef ENABLE_FRONTEND
	{
		obs_properties_add_button2(pr, S_MANUAL_OPEN, D_TRANSLATE(S_MANUAL_OPEN),
								   streamfx::filter::blur::blur_factory::on_manual_open, nullptr);
	}
#endif

	// Blur Type and Sub-Type
	{
		p = obs_properties_add_list(pr, ST_KEY_TYPE, D_TRANSLATE(ST_I18N_TYPE), OBS_COMBO_TYPE_LIST,
									OBS_COMBO_FORMAT_STRING);
		obs_property_set_modified_callback2(p, modified_properties, this);
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_TYPE_BOX), "box");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_TYPE_BOX_LINEAR), "box_linear");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_TYPE_GAUSSIAN), "gaussian");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_TYPE_GAUSSIAN_LINEAR), "gaussian_linear");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_TYPE_DUALFILTERING), "dual_filtering");

		p = obs_properties_add_list(pr, ST_KEY_SUBTYPE, D_TRANSLATE(ST_I18N_SUBTYPE), OBS_COMBO_TYPE_LIST,
									OBS_COMBO_FORMAT_STRING);
		obs_property_set_modified_callback2(p, modified_properties, this);
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_SUBTYPE_AREA), "area");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_SUBTYPE_DIRECTIONAL), "directional");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_SUBTYPE_ROTATIONAL), "rotational");
		obs_property_list_add_string(p, D_TRANSLATE(S_BLUR_SUBTYPE_ZOOM), "zoom");
	}

	// Blur Parameters
	{
		p = obs_properties_add_float_slider(pr, ST_KEY_SIZE, D_TRANSLATE(ST_I18N_SIZE), 1, 32767, 1);
		p = obs_properties_add_float_slider(pr, ST_KEY_ANGLE, D_TRANSLATE(ST_I18N_ANGLE), -180.0, 180.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_CENTER_X, D_TRANSLATE(ST_I18N_CENTER_X), 0.00, 100.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_CENTER_Y, D_TRANSLATE(ST_I18N_CENTER_Y), 0.00, 100.0, 0.01);

		p = obs_properties_add_bool(pr, ST_KEY_STEPSCALE, D_TRANSLATE(ST_I18N_STEPSCALE));
		obs_property_set_modified_callback2(p, modified_properties, this);
		p = obs_properties_add_float_slider(pr, ST_KEY_STEPSCALE_X, D_TRANSLATE(ST_I18N_STEPSCALE_X), 0.0, 1000.0,
											0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_STEPSCALE_Y, D_TRANSLATE(ST_I18N_STEPSCALE_Y), 0.0, 1000.0,
											0.01);
	}

	// Masking
	{
		p = obs_properties_add_bool(pr, ST_KEY_MASK, D_TRANSLATE(ST_I18N_MASK));
		obs_property_set_modified_callback2(p, modified_properties, this);
		p = obs_properties_add_list(pr, ST_KEY_MASK_TYPE, D_TRANSLATE(ST_I18N_MASK_TYPE), OBS_COMBO_TYPE_LIST,
									OBS_COMBO_FORMAT_INT);
		obs_property_set_modified_callback2(p, modified_properties, this);
		obs_property_list_add_int(p, D_TRANSLATE(ST_I18N_MASK_TYPE_REGION), static_cast<int64_t>(mask_type::Region));
		obs_property_list_add_int(p, D_TRANSLATE(ST_I18N_MASK_TYPE_IMAGE), static_cast<int64_t>(mask_type::Image));
		obs_property_list_add_int(p, D_TRANSLATE(ST_I18N_MASK_TYPE_SOURCE), static_cast<int64_t>(mask_type::Source));
		/// Region
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_REGION_LEFT, D_TRANSLATE(ST_I18N_MASK_REGION_LEFT), 0.0,
											100.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_REGION_TOP, D_TRANSLATE(ST_I18N_MASK_REGION_TOP), 0.0,
											100.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_REGION_RIGHT, D_TRANSLATE(ST_I18N_MASK_REGION_RIGHT), 0.0,
											100.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_REGION_BOTTOM, D_TRANSLATE(ST_I18N_MASK_REGION_BOTTOM), 0.0,
											100.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_REGION_FEATHER, D_TRANSLATE(ST_I18N_MASK_REGION_FEATHER),
											0.0, 50.0, 0.01);
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_REGION_FEATHER_SHIFT,
											D_TRANSLATE(ST_I18N_MASK_REGION_FEATHER_SHIFT), -100.0, 100.0, 0.01);
		p = obs_properties_add_bool(pr, ST_KEY_MASK_REGION_INVERT, D_TRANSLATE(ST_I18N_MASK_REGION_INVERT));
		/// Image
		{
			std::string filter =
				translate_string("%s (%s);;* (*.*)", D_TRANSLATE(S_FILETYPE_IMAGES), S_FILEFILTERS_TEXTURE);
			_translation_cache.push_back(filter);
			p = obs_properties_add_path(pr, ST_KEY_MASK_IMAGE, D_TRANSLATE(ST_I18N_MASK_IMAGE), OBS_PATH_FILE,
										_translation_cache.back().c_str(), nullptr);
		}
		/// Source
		p = obs_properties_add_list(pr, ST_KEY_MASK_SOURCE, D_TRANSLATE(ST_I18N_MASK_SOURCE), OBS_COMBO_TYPE_LIST,
									OBS_COMBO_FORMAT_STRING);
		obs_property_list_add_string(p, "", "");
		obs::source_tracker::get()->enumerate(
			[&p](std::string name, obs_source_t*) {
				obs_property_list_add_string(p, std::string(name + " (Source)").c_str(), name.c_str());
				return false;
			},
			obs::source_tracker::filter_video_sources);
		obs::source_tracker::get()->enumerate(
			[&p](std::string name, obs_source_t*) {
				obs_property_list_add_string(p, std::string(name + " (Scene)").c_str(), name.c_str());
				return false;
			},
			obs::source_tracker::filter_scenes);

		/// Shared
		p = obs_properties_add_color(pr, ST_KEY_MASK_COLOR, D_TRANSLATE(ST_I18N_MASK_COLOR));
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_ALPHA, D_TRANSLATE(ST_I18N_MASK_ALPHA), 0.0, 100.0, 0.1);
		p = obs_properties_add_float_slider(pr, ST_KEY_MASK_MULTIPLIER, D_TRANSLATE(ST_I18N_MASK_MULTIPLIER), 0.0, 10.0,
											0.01);
	}

	return pr;
}

std::string blur_factory::translate_string(const char* format, ...)
{
	va_list vargs;
	va_start(vargs, format);
	std::vector<char> buffer(2048);
	std::size_t       len = static_cast<size_t>(vsnprintf(buffer.data(), buffer.size(), format, vargs));
	va_end(vargs);
	return std::string(buffer.data(), buffer.data() + len);
}

#ifdef ENABLE_FRONTEND
bool blur_factory::on_manual_open(obs_properties_t* props, obs_property_t* property, void* data)
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

std::shared_ptr<blur_factory> _filter_blur_factory_instance = nullptr;

void streamfx::filter::blur::blur_factory::initialize()
try {
	if (!_filter_blur_factory_instance)
		_filter_blur_factory_instance = std::make_shared<blur_factory>();
} catch (const std::exception& ex) {
	D_LOG_ERROR("Failed to initialize due to error: %s", ex.what());
} catch (...) {
	D_LOG_ERROR("Failed to initialize due to unknown error.", "");
}

void streamfx::filter::blur::blur_factory::finalize()
{
	_filter_blur_factory_instance.reset();
}

std::shared_ptr<blur_factory> streamfx::filter::blur::blur_factory::get()
{
	return _filter_blur_factory_instance;
}
