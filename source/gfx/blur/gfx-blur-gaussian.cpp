// Modern effects for a modern Streamer
// Copyright (C) 2019 Michael Fabian Dirks
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#include "gfx-blur-gaussian.hpp"
#include <algorithm>
#include <stdexcept>
#include "obs/gs/gs-helper.hpp"
#include "plugin.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <obs.h>
#include <obs-module.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// TODO: It may be possible to optimize to run much faster: https://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

#define ST_KERNEL_SIZE 128u
#define ST_OVERSAMPLE_MULTIPLIER 2
#define ST_MAX_BLUR_SIZE ST_KERNEL_SIZE / ST_OVERSAMPLE_MULTIPLIER

streamfx::gfx::blur::gaussian_data::gaussian_data()
{
	using namespace streamfx::util;

	std::vector<double> kernel_dbl(ST_KERNEL_SIZE);
	std::vector<float>  kernel(ST_KERNEL_SIZE);

	{
		auto gctx = streamfx::obs::gs::context();

		{
			auto file = streamfx::data_file_path("effects/blur/gaussian.effect");
			try {
				_effect = streamfx::obs::gs::effect::create(file);
			} catch (const std::exception& ex) {
				DLOG_ERROR("Error loading '%s': %s", file.generic_u8string().c_str(), ex.what());
			}
		}
	}

	//#define ST_USE_PASCAL_TRIANGLE

	// Pre-calculate Kernel Information for all Kernel sizes
	for (size_t size = 1; size <= ST_MAX_BLUR_SIZE; size++) {
#ifdef ST_USE_PASCAL_TRIANGLE
		// The Pascal Triangle can be used to generate Gaussian Kernels, which is
		// significantly faster than doing the same task with searching. It is also
		// much more accurate at the same time, so it is a 2-in-1 solution.

		// Generate the required row and sum.
		size_t offset   = size;
		size_t row      = size * 2;
		auto   triangle = math::pascal_triangle<double>(row);
		double sum      = pow(2, row);

		// Convert all integers to floats.
		double accum = 0.;
		for (size_t idx = offset; idx < std::min<size_t>(triangle.size(), ST_KERNEL_SIZE); idx++) {
			double v                 = static_cast<double>(triangle[idx]) / sum;
			kernel_dbl[idx - offset] = v;
			// Accumulator needed as we end up with float inaccuracies above a certain threshold.
			accum += v * (idx > offset ? 2 : 1);
		}

		// Rescale all values back into useful ranges.
		accum = 1. / accum;
		for (size_t idx = offset; idx < ST_KERNEL_SIZE; idx++) {
			kernel[idx - offset] = kernel_dbl[idx - offset] * accum;
		}
#else
		size_t oversample = size * ST_OVERSAMPLE_MULTIPLIER;

		// Generate initial weights and calculate a total from them.
		double total = 0.;
		for (size_t idx = 0; (idx < oversample) && (idx < ST_KERNEL_SIZE); idx++) {
			kernel_dbl[idx] = math::gaussian<double>(static_cast<double>(idx), static_cast<double>(size));
			total += kernel_dbl[idx] * (idx > 0 ? 2 : 1);
		}

		// Scale the weights according to the total gathered, and convert to float.
		for (size_t idx = 0; (idx < oversample) && (idx < ST_KERNEL_SIZE); idx++) {
			kernel_dbl[idx] /= total;
			kernel[idx] = static_cast<float>(kernel_dbl[idx]);
		}

#endif

		// Store Kernel
		_kernels.insert_or_assign(size, kernel);
	}
}

streamfx::gfx::blur::gaussian_data::~gaussian_data()
{
	auto gctx = streamfx::obs::gs::context();
	_effect.reset();
}

streamfx::obs::gs::effect streamfx::gfx::blur::gaussian_data::get_effect()
{
	return _effect;
}

std::vector<float_t> const& streamfx::gfx::blur::gaussian_data::get_kernel(std::size_t width)
{
	width = std::clamp<size_t>(width, 1, ST_MAX_BLUR_SIZE);
	return _kernels.at(width);
}

streamfx::gfx::blur::gaussian_factory::gaussian_factory() {}

streamfx::gfx::blur::gaussian_factory::~gaussian_factory() {}

bool streamfx::gfx::blur::gaussian_factory::is_type_supported(::streamfx::gfx::blur::type v)
{
	switch (v) {
	case ::streamfx::gfx::blur::type::Area:
		return true;
	case ::streamfx::gfx::blur::type::Directional:
		return true;
	case ::streamfx::gfx::blur::type::Rotational:
		return true;
	case ::streamfx::gfx::blur::type::Zoom:
		return true;
	default:
		return false;
	}
}

std::shared_ptr<::streamfx::gfx::blur::base>
	streamfx::gfx::blur::gaussian_factory::create(::streamfx::gfx::blur::type v)
{
	switch (v) {
	case ::streamfx::gfx::blur::type::Area:
		return std::make_shared<::streamfx::gfx::blur::gaussian>();
	case ::streamfx::gfx::blur::type::Directional:
		return std::static_pointer_cast<::streamfx::gfx::blur::gaussian>(
			std::make_shared<::streamfx::gfx::blur::gaussian_directional>());
	case ::streamfx::gfx::blur::type::Rotational:
		return std::make_shared<::streamfx::gfx::blur::gaussian_rotational>();
	case ::streamfx::gfx::blur::type::Zoom:
		return std::make_shared<::streamfx::gfx::blur::gaussian_zoom>();
	default:
		throw std::runtime_error("Invalid type.");
	}
}

double_t streamfx::gfx::blur::gaussian_factory::get_min_size(::streamfx::gfx::blur::type)
{
	return double_t(1.0);
}

double_t streamfx::gfx::blur::gaussian_factory::get_step_size(::streamfx::gfx::blur::type)
{
	return double_t(1.0);
}

double_t streamfx::gfx::blur::gaussian_factory::get_max_size(::streamfx::gfx::blur::type)
{
	return double_t(ST_MAX_BLUR_SIZE);
}

double_t streamfx::gfx::blur::gaussian_factory::get_min_angle(::streamfx::gfx::blur::type v)
{
	switch (v) {
	case ::streamfx::gfx::blur::type::Directional:
	case ::streamfx::gfx::blur::type::Rotational:
		return -180.0;
	default:
		return 0;
	}
}

double_t streamfx::gfx::blur::gaussian_factory::get_step_angle(::streamfx::gfx::blur::type)
{
	return double_t(0.01);
}

double_t streamfx::gfx::blur::gaussian_factory::get_max_angle(::streamfx::gfx::blur::type v)
{
	switch (v) {
	case ::streamfx::gfx::blur::type::Directional:
	case ::streamfx::gfx::blur::type::Rotational:
		return 180.0;
	default:
		return 0;
	}
}

bool streamfx::gfx::blur::gaussian_factory::is_step_scale_supported(::streamfx::gfx::blur::type v)
{
	switch (v) {
	case ::streamfx::gfx::blur::type::Area:
	case ::streamfx::gfx::blur::type::Zoom:
	case ::streamfx::gfx::blur::type::Directional:
		return true;
	default:
		return false;
	}
}

double_t streamfx::gfx::blur::gaussian_factory::get_min_step_scale_x(::streamfx::gfx::blur::type)
{
	return double_t(0.01);
}

double_t streamfx::gfx::blur::gaussian_factory::get_step_step_scale_x(::streamfx::gfx::blur::type)
{
	return double_t(0.01);
}

double_t streamfx::gfx::blur::gaussian_factory::get_max_step_scale_x(::streamfx::gfx::blur::type)
{
	return double_t(1000.0);
}

double_t streamfx::gfx::blur::gaussian_factory::get_min_step_scale_y(::streamfx::gfx::blur::type)
{
	return double_t(0.01);
}

double_t streamfx::gfx::blur::gaussian_factory::get_step_step_scale_y(::streamfx::gfx::blur::type)
{
	return double_t(0.01);
}

double_t streamfx::gfx::blur::gaussian_factory::get_max_step_scale_y(::streamfx::gfx::blur::type)
{
	return double_t(1000.0);
}

std::shared_ptr<::streamfx::gfx::blur::gaussian_data> streamfx::gfx::blur::gaussian_factory::data()
{
	std::unique_lock<std::mutex>                          ulock(_data_lock);
	std::shared_ptr<::streamfx::gfx::blur::gaussian_data> data = _data.lock();
	if (!data) {
		data  = std::make_shared<::streamfx::gfx::blur::gaussian_data>();
		_data = data;
	}
	return data;
}

::streamfx::gfx::blur::gaussian_factory& streamfx::gfx::blur::gaussian_factory::get()
{
	static ::streamfx::gfx::blur::gaussian_factory instance;
	return instance;
}

streamfx::gfx::blur::gaussian::gaussian()
	: _data(::streamfx::gfx::blur::gaussian_factory::get().data()), _size(1.), _step_scale({1., 1.})
{
	auto gctx      = streamfx::obs::gs::context();
	_rendertarget  = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
	_rendertarget2 = std::make_shared<streamfx::obs::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
}

streamfx::gfx::blur::gaussian::~gaussian() {}

void streamfx::gfx::blur::gaussian::set_input(std::shared_ptr<::streamfx::obs::gs::texture> texture)
{
	_input_texture = texture;
}

::streamfx::gfx::blur::type streamfx::gfx::blur::gaussian::get_type()
{
	return ::streamfx::gfx::blur::type::Area;
}

double_t streamfx::gfx::blur::gaussian::get_size()
{
	return _size;
}

void streamfx::gfx::blur::gaussian::set_size(double_t width)
{
	if (width < 1.)
		width = 1.;
	if (width > ST_MAX_BLUR_SIZE)
		width = ST_MAX_BLUR_SIZE;
	_size = width;
}

void streamfx::gfx::blur::gaussian::set_step_scale(double_t x, double_t y)
{
	_step_scale.first  = x;
	_step_scale.second = y;
}

void streamfx::gfx::blur::gaussian::get_step_scale(double_t& x, double_t& y)
{
	x = _step_scale.first;
	y = _step_scale.second;
}

double_t streamfx::gfx::blur::gaussian::get_step_scale_x()
{
	return _step_scale.first;
}

double_t streamfx::gfx::blur::gaussian::get_step_scale_y()
{
	return _step_scale.second;
}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::gaussian::render()
{
	auto gctx = streamfx::obs::gs::context();

#ifdef ENABLE_PROFILING
	auto gdmp = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Gaussian Blur");
#endif

	streamfx::obs::gs::effect effect = _data->get_effect();

	if (!effect || ((_step_scale.first + _step_scale.second) < std::numeric_limits<double_t>::epsilon())) {
		return _input_texture;
	}

	auto    kernel = _data->get_kernel(size_t(_size));
	float_t width  = float_t(_input_texture->get_width());
	float_t height = float_t(_input_texture->get_height());

	// Setup
	gs_set_cull_mode(GS_NEITHER);
	gs_enable_color(true, true, true, true);
	gs_enable_depth_test(false);
	gs_depth_function(GS_ALWAYS);
	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_blending(false);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
	gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

	effect.get_parameter("pStepScale").set_float2(float_t(_step_scale.first), float_t(_step_scale.second));
	effect.get_parameter("pSize").set_float(float_t(_size * ST_OVERSAMPLE_MULTIPLIER));
	effect.get_parameter("pKernel").set_value(kernel.data(), ST_KERNEL_SIZE);

	// First Pass
	if (_step_scale.first > std::numeric_limits<double_t>::epsilon()) {
		effect.get_parameter("pImage").set_texture(_input_texture);
		effect.get_parameter("pImageTexel").set_float2(float_t(1.f / width), 0.f);

		{
#ifdef ENABLE_PROFILING
			auto gdm = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Horizontal");
#endif

			auto op = _rendertarget2->render(uint32_t(width), uint32_t(height));
			gs_ortho(0, 1., 0, 1., 0, 1.);
			while (gs_effect_loop(effect.get_object(), "Draw")) {
				streamfx::gs_draw_fullscreen_tri();
			}
		}

		std::swap(_rendertarget, _rendertarget2);
	}

	// Second Pass
	if (_step_scale.second > std::numeric_limits<double_t>::epsilon()) {
		effect.get_parameter("pImage").set_texture(_rendertarget->get_texture());
		effect.get_parameter("pImageTexel").set_float2(0.f, float_t(1.f / height));

		{
#ifdef ENABLE_PROFILING
			auto gdm = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Vertical");
#endif

			auto op = _rendertarget2->render(uint32_t(width), uint32_t(height));
			gs_ortho(0, 1., 0, 1., 0, 1.);
			while (gs_effect_loop(effect.get_object(), "Draw")) {
				streamfx::gs_draw_fullscreen_tri();
			}
		}

		std::swap(_rendertarget, _rendertarget2);
	}

	gs_blend_state_pop();

	return this->get();
}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::gaussian::get()
{
	return _rendertarget->get_texture();
}

streamfx::gfx::blur::gaussian_directional::gaussian_directional() : m_angle(0.) {}

streamfx::gfx::blur::gaussian_directional::~gaussian_directional() {}

::streamfx::gfx::blur::type streamfx::gfx::blur::gaussian_directional::get_type()
{
	return ::streamfx::gfx::blur::type::Directional;
}

double_t streamfx::gfx::blur::gaussian_directional::get_angle()
{
	return D_RAD_TO_DEG(m_angle);
}

void streamfx::gfx::blur::gaussian_directional::set_angle(double_t angle)
{
	m_angle = D_DEG_TO_RAD(angle);
}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::gaussian_directional::render()
{
	auto gctx = streamfx::obs::gs::context();

#ifdef ENABLE_PROFILING
	auto gdmp =
		streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Gaussian Directional Blur");
#endif

	streamfx::obs::gs::effect effect = _data->get_effect();

	if (!effect || ((_step_scale.first + _step_scale.second) < std::numeric_limits<double_t>::epsilon())) {
		return _input_texture;
	}

	auto    kernel = _data->get_kernel(size_t(_size));
	float_t width  = float_t(_input_texture->get_width());
	float_t height = float_t(_input_texture->get_height());

	// Setup
	gs_set_cull_mode(GS_NEITHER);
	gs_enable_color(true, true, true, true);
	gs_enable_depth_test(false);
	gs_depth_function(GS_ALWAYS);
	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_blending(false);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
	gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

	effect.get_parameter("pImage").set_texture(_input_texture);
	effect.get_parameter("pImageTexel")
		.set_float2(float_t(1.f / width * cos(m_angle)), float_t(1.f / height * sin(m_angle)));
	effect.get_parameter("pStepScale").set_float2(float_t(_step_scale.first), float_t(_step_scale.second));
	effect.get_parameter("pSize").set_float(float_t(_size * ST_OVERSAMPLE_MULTIPLIER));
	effect.get_parameter("pKernel").set_value(kernel.data(), ST_KERNEL_SIZE);

	{
		auto op = _rendertarget->render(uint32_t(width), uint32_t(height));
		gs_ortho(0, 1., 0, 1., 0, 1.);
		while (gs_effect_loop(effect.get_object(), "Draw")) {
			streamfx::gs_draw_fullscreen_tri();
		}
	}

	gs_blend_state_pop();

	return this->get();
}

::streamfx::gfx::blur::type streamfx::gfx::blur::gaussian_rotational::get_type()
{
	return ::streamfx::gfx::blur::type::Rotational;
}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::gaussian_rotational::render()
{
	auto gctx = streamfx::obs::gs::context();

#ifdef ENABLE_PROFILING
	auto gdmp =
		streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Gaussian Rotational Blur");
#endif

	streamfx::obs::gs::effect effect = _data->get_effect();

	if (!effect || ((_step_scale.first + _step_scale.second) < std::numeric_limits<double_t>::epsilon())) {
		return _input_texture;
	}

	auto    kernel = _data->get_kernel(size_t(_size));
	float_t width  = float_t(_input_texture->get_width());
	float_t height = float_t(_input_texture->get_height());

	// Setup
	gs_set_cull_mode(GS_NEITHER);
	gs_enable_color(true, true, true, true);
	gs_enable_depth_test(false);
	gs_depth_function(GS_ALWAYS);
	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_blending(false);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
	gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

	effect.get_parameter("pImage").set_texture(_input_texture);
	effect.get_parameter("pImageTexel").set_float2(float_t(1.f / width), float_t(1.f / height));
	effect.get_parameter("pStepScale").set_float2(float_t(_step_scale.first), float_t(_step_scale.second));
	effect.get_parameter("pSize").set_float(float_t(_size * ST_OVERSAMPLE_MULTIPLIER));
	effect.get_parameter("pAngle").set_float(float_t(m_angle / _size));
	effect.get_parameter("pCenter").set_float2(float_t(m_center.first), float_t(m_center.second));
	effect.get_parameter("pKernel").set_value(kernel.data(), ST_KERNEL_SIZE);

	// First Pass
	{
		auto op = _rendertarget->render(uint32_t(width), uint32_t(height));
		gs_ortho(0, 1., 0, 1., 0, 1.);
		while (gs_effect_loop(effect.get_object(), "Rotate")) {
			streamfx::gs_draw_fullscreen_tri();
		}
	}

	gs_blend_state_pop();

	return this->get();
}

void streamfx::gfx::blur::gaussian_rotational::set_center(double_t x, double_t y)
{
	m_center.first  = x;
	m_center.second = y;
}

void streamfx::gfx::blur::gaussian_rotational::get_center(double_t& x, double_t& y)
{
	x = m_center.first;
	y = m_center.second;
}

double_t streamfx::gfx::blur::gaussian_rotational::get_angle()
{
	return double_t(D_RAD_TO_DEG(m_angle));
}

void streamfx::gfx::blur::gaussian_rotational::set_angle(double_t angle)
{
	m_angle = D_DEG_TO_RAD(angle);
}

::streamfx::gfx::blur::type streamfx::gfx::blur::gaussian_zoom::get_type()
{
	return ::streamfx::gfx::blur::type::Zoom;
}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::gaussian_zoom::render()
{
	auto gctx = streamfx::obs::gs::context();

#ifdef ENABLE_PROFILING
	auto gdmp = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Gaussian Zoom Blur");
#endif

	streamfx::obs::gs::effect effect = _data->get_effect();
	auto                      kernel = _data->get_kernel(size_t(_size));

	if (!effect || ((_step_scale.first + _step_scale.second) < std::numeric_limits<double_t>::epsilon())) {
		return _input_texture;
	}

	float_t width  = float_t(_input_texture->get_width());
	float_t height = float_t(_input_texture->get_height());

	// Setup
	gs_set_cull_mode(GS_NEITHER);
	gs_enable_color(true, true, true, true);
	gs_enable_depth_test(false);
	gs_depth_function(GS_ALWAYS);
	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_blending(false);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
	gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

	effect.get_parameter("pImage").set_texture(_input_texture);
	effect.get_parameter("pImageTexel").set_float2(float_t(1.f / width), float_t(1.f / height));
	effect.get_parameter("pStepScale").set_float2(float_t(_step_scale.first), float_t(_step_scale.second));
	effect.get_parameter("pSize").set_float(float_t(_size));
	effect.get_parameter("pCenter").set_float2(float_t(m_center.first), float_t(m_center.second));
	effect.get_parameter("pKernel").set_value(kernel.data(), ST_KERNEL_SIZE);

	// First Pass
	{
		auto op = _rendertarget->render(uint32_t(width), uint32_t(height));
		gs_ortho(0, 1., 0, 1., 0, 1.);
		while (gs_effect_loop(effect.get_object(), "Zoom")) {
			streamfx::gs_draw_fullscreen_tri();
		}
	}

	gs_blend_state_pop();

	return this->get();
}

void streamfx::gfx::blur::gaussian_zoom::set_center(double_t x, double_t y)
{
	m_center.first  = x;
	m_center.second = y;
}

void streamfx::gfx::blur::gaussian_zoom::get_center(double_t& x, double_t& y)
{
	x = m_center.first;
	y = m_center.second;
}
