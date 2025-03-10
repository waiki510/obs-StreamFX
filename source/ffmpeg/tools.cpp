// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (c) 2019 Michael Fabian Dirks <info@xaymar.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "tools.hpp"
#include <list>
#include <sstream>
#include "plugin.hpp"

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#pragma warning(pop)
}

using namespace streamfx::ffmpeg;

const char* tools::get_pixel_format_name(AVPixelFormat v)
{
	return av_get_pix_fmt_name(v);
}

const char* tools::get_color_space_name(AVColorSpace v)
{
	switch (v) {
	case AVCOL_SPC_RGB:
		return "RGB";
	case AVCOL_SPC_BT709:
		return "BT.709";
	case AVCOL_SPC_FCC:
		return "FCC Title 47 CoFR 73.682 (a)(20)";
	case AVCOL_SPC_BT470BG:
		return "BT.601 625";
	case AVCOL_SPC_SMPTE170M:
	case AVCOL_SPC_SMPTE240M:
		return "BT.601 525";
	case AVCOL_SPC_YCGCO:
		return "ITU-T SG16";
	case AVCOL_SPC_BT2020_NCL:
		return "BT.2020 NCL";
	case AVCOL_SPC_BT2020_CL:
		return "BT.2020 CL";
	case AVCOL_SPC_SMPTE2085:
		return "SMPTE 2085";
	case AVCOL_SPC_CHROMA_DERIVED_NCL:
		return "Chroma NCL";
	case AVCOL_SPC_CHROMA_DERIVED_CL:
		return "Chroma CL";
	case AVCOL_SPC_ICTCP:
		return "BT.2100";
	case AVCOL_SPC_NB:
		return "Not Part of ABI";
	default:
		return "Unknown";
	}
}

const char* tools::get_error_description(int error)
{
	thread_local char error_buf[AV_ERROR_MAX_STRING_SIZE + 1];
	if (av_strerror(error, error_buf, AV_ERROR_MAX_STRING_SIZE) < 0) {
		snprintf(error_buf, AV_ERROR_MAX_STRING_SIZE, "Unknown Error (%i)", error);
	}
	return error_buf;
}

static std::map<video_format, AVPixelFormat> const obs_to_av_format_map = {
	{VIDEO_FORMAT_I420, AV_PIX_FMT_YUV420P},  // YUV 4:2:0
	{VIDEO_FORMAT_NV12, AV_PIX_FMT_NV12},     // NV12 Packed YUV
	{VIDEO_FORMAT_YVYU, AV_PIX_FMT_YVYU422},  // YVYU Packed YUV
	{VIDEO_FORMAT_YUY2, AV_PIX_FMT_YUYV422},  // YUYV Packed YUV
	{VIDEO_FORMAT_UYVY, AV_PIX_FMT_UYVY422},  // UYVY Packed YUV
	{VIDEO_FORMAT_RGBA, AV_PIX_FMT_RGBA},     //
	{VIDEO_FORMAT_BGRA, AV_PIX_FMT_BGRA},     //
	{VIDEO_FORMAT_BGRX, AV_PIX_FMT_BGR0},     //
	{VIDEO_FORMAT_Y800, AV_PIX_FMT_GRAY8},    //
	{VIDEO_FORMAT_I444, AV_PIX_FMT_YUV444P},  //
	{VIDEO_FORMAT_BGR3, AV_PIX_FMT_BGR24},    //
	{VIDEO_FORMAT_I422, AV_PIX_FMT_YUV422P},  //
	{VIDEO_FORMAT_I40A, AV_PIX_FMT_YUVA420P}, //
	{VIDEO_FORMAT_I42A, AV_PIX_FMT_YUVA422P}, //
	{VIDEO_FORMAT_YUVA, AV_PIX_FMT_YUVA444P}, //
											  //{VIDEO_FORMAT_AYUV, AV_PIX_FMT_AYUV444P}, //
};

AVPixelFormat tools::obs_videoformat_to_avpixelformat(video_format v)
{
	auto found = obs_to_av_format_map.find(v);
	if (found != obs_to_av_format_map.end()) {
		return found->second;
	}
	return AV_PIX_FMT_NONE;
}

video_format tools::avpixelformat_to_obs_videoformat(AVPixelFormat v)
{
	for (const auto& kv : obs_to_av_format_map) {
		if (kv.second == v)
			return kv.first;
	}
	return VIDEO_FORMAT_NONE;
}

AVPixelFormat tools::get_least_lossy_format(const AVPixelFormat* haystack, AVPixelFormat needle)
{
	int data_loss = 0;
	return avcodec_find_best_pix_fmt_of_list(haystack, needle, 0, &data_loss);
}

AVColorRange tools::obs_to_av_color_range(video_range_type v)
{
	switch (v) {
	case VIDEO_RANGE_DEFAULT:
	case VIDEO_RANGE_PARTIAL:
		return AVCOL_RANGE_MPEG;
	case VIDEO_RANGE_FULL:
		return AVCOL_RANGE_JPEG;
	}
	throw std::invalid_argument("Unknown Color Range");
}

AVColorSpace tools::obs_to_av_color_space(video_colorspace v)
{
	switch (v) {
	case VIDEO_CS_601: // BT.601
		return AVCOL_SPC_SMPTE170M;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:  // BT.709
	case VIDEO_CS_SRGB: // sRGB
		return AVCOL_SPC_BT709;
	default:
		throw std::invalid_argument("Unknown Color Space");
	}
}

AVColorPrimaries streamfx::ffmpeg::tools::obs_to_av_color_primary(video_colorspace v)
{
	switch (v) {
	case VIDEO_CS_601: // BT.601
		return AVCOL_PRI_SMPTE170M;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:  // BT.709
	case VIDEO_CS_SRGB: // sRGB
		return AVCOL_PRI_BT709;
	default:
		throw std::invalid_argument("Unknown Color Primaries");
	}
}

AVColorTransferCharacteristic streamfx::ffmpeg::tools::obs_to_av_color_transfer_characteristics(video_colorspace v)
{
	switch (v) {
	case VIDEO_CS_601: // BT.601
		return AVCOL_TRC_SMPTE170M;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709: // BT.709
		return AVCOL_TRC_BT709;
	case VIDEO_CS_SRGB: // sRGB with IEC 61966-2-1
		return AVCOL_TRC_IEC61966_2_1;
	default:
		throw std::invalid_argument("Unknown Color Transfer Characteristics");
	}
}

const char* tools::avoption_name_from_unit_value(const void* obj, std::string_view unit, int64_t value)
{
	for (const AVOption* opt = nullptr; (opt = av_opt_next(obj, opt)) != nullptr;) {
		// Skip all irrelevant options.
		if (!opt->unit)
			continue;
		if (opt->unit != unit)
			continue;
		if (opt->name == unit)
			continue;

		if (opt->default_val.i64 == value)
			return opt->name;
	}
	return nullptr;
}

bool tools::avoption_exists(const void* obj, std::string_view name)
{
	for (const AVOption* opt = nullptr; (opt = av_opt_next(obj, opt)) != nullptr;) {
		if (name == opt->name)
			return true;
	}
	return false;
}

void tools::avoption_list_add_entries_unnamed(const void* obj, std::string_view unit, obs_property_t* prop,
											  std::function<bool(const AVOption*)> filter)
{
	for (const AVOption* opt = nullptr; (opt = av_opt_next(obj, opt)) != nullptr;) {
		// Skip all irrelevant options.
		if (!opt->unit)
			continue;
		if (opt->unit != unit)
			continue;
		if (opt->name == unit)
			continue;

		// Skip any deprecated options.
		if (opt->flags & AV_OPT_FLAG_DEPRECATED)
			continue;

		if (filter && filter(opt))
			continue;

		// Generate name and add to list.
		obs_property_list_add_int(prop, opt->name, opt->default_val.i64);
	}
}

void tools::avoption_list_add_entries(const void* obj, std::string_view unit, obs_property_t* prop,
									  std::string_view prefix, std::function<bool(const AVOption*)> filter)
{
	for (const AVOption* opt = nullptr; (opt = av_opt_next(obj, opt)) != nullptr;) {
		// Skip all irrelevant options.
		if (!opt->unit)
			continue;
		if (opt->unit != unit)
			continue;
		if (opt->name == unit)
			continue;

		// Skip any deprecated options.
		if (opt->flags & AV_OPT_FLAG_DEPRECATED)
			continue;

		// Skip based on filter function.
		if (filter && filter(opt))
			continue;

		// Generate name and add to list.
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "%s.%s\0", prefix.data(), opt->name);
		obs_property_list_add_int(prop, D_TRANSLATE(buffer), opt->default_val.i64);
	}
}

bool tools::can_hardware_encode(const AVCodec* codec)
{
	AVPixelFormat hardware_formats[] = {AV_PIX_FMT_D3D11};

	for (const AVPixelFormat* fmt = codec->pix_fmts; (fmt != nullptr) && (*fmt != AV_PIX_FMT_NONE); fmt++) {
		for (auto cmp : hardware_formats) {
			if (*fmt == cmp) {
				return true;
			}
		}
	}
	return false;
}

std::vector<AVPixelFormat> tools::get_software_formats(const AVPixelFormat* list)
{
	constexpr AVPixelFormat hardware_formats[] = {
#if FF_API_VAAPI
		AV_PIX_FMT_VAAPI_MOCO,
		AV_PIX_FMT_VAAPI_IDCT,
#endif
		AV_PIX_FMT_VAAPI,
		AV_PIX_FMT_DXVA2_VLD,
		AV_PIX_FMT_VDPAU,
		AV_PIX_FMT_QSV,
		AV_PIX_FMT_MMAL,
		AV_PIX_FMT_D3D11VA_VLD,
		AV_PIX_FMT_CUDA,
		AV_PIX_FMT_XVMC,
		AV_PIX_FMT_VIDEOTOOLBOX,
		AV_PIX_FMT_MEDIACODEC,
		AV_PIX_FMT_D3D11,
	};

	std::vector<AVPixelFormat> fmts;
	for (auto fmt = list; fmt && (*fmt != AV_PIX_FMT_NONE); fmt++) {
		bool is_blacklisted = false;
		for (auto blacklisted : hardware_formats) {
			if (*fmt == blacklisted)
				is_blacklisted = true;
		}
		if (!is_blacklisted)
			fmts.push_back(*fmt);
	}

	fmts.push_back(AV_PIX_FMT_NONE);

	return std::move(fmts);
}

void tools::context_setup_from_obs(const video_output_info* voi, AVCodecContext* context)
{
	// Resolution
	context->width  = static_cast<int>(voi->width);
	context->height = static_cast<int>(voi->height);

	// Framerate
	context->ticks_per_frame = 1;
	context->framerate.num = context->time_base.den = static_cast<int>(voi->fps_num);
	context->framerate.den = context->time_base.num = static_cast<int>(voi->fps_den);

	// Aspect Ratio, Progressive
	context->sample_aspect_ratio.num = 1;
	context->sample_aspect_ratio.den = 1;
	context->field_order             = AV_FIELD_PROGRESSIVE;

	// Decipher Pixel information
	context->pix_fmt         = obs_videoformat_to_avpixelformat(voi->format);
	context->color_range     = obs_to_av_color_range(voi->range);
	context->colorspace      = obs_to_av_color_space(voi->colorspace);
	context->color_primaries = obs_to_av_color_primary(voi->colorspace);
	context->color_trc       = obs_to_av_color_transfer_characteristics(voi->colorspace);

	// Chroma Location
	switch (context->pix_fmt) {
	case AV_PIX_FMT_NV12:
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVA420P:
	case AV_PIX_FMT_YUV422P:
	case AV_PIX_FMT_YUVA422P:
	case AV_PIX_FMT_YVYU422:
	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_UYVY422:
		// libOBS merges Chroma at "Top", see H.264 specification.
		context->chroma_sample_location = AVCHROMA_LOC_TOP;
		break;
	default:
		// All other cases are unspecified.
		context->chroma_sample_location = AVCHROMA_LOC_UNSPECIFIED;
		break;
	}
}

const char* tools::get_std_compliance_name(int compliance)
{
	switch (compliance) {
	case FF_COMPLIANCE_VERY_STRICT:
		return "Very Strict";
	case FF_COMPLIANCE_STRICT:
		return "Strict";
	case FF_COMPLIANCE_NORMAL:
		return "Normal";
	case FF_COMPLIANCE_UNOFFICIAL:
		return "Unofficial";
	case FF_COMPLIANCE_EXPERIMENTAL:
		return "Experimental";
	}
	return "Invalid";
}

const char* tools::get_thread_type_name(int thread_type)
{
	switch (thread_type) {
	case FF_THREAD_FRAME | FF_THREAD_SLICE:
		return "Slice & Frame";
	case FF_THREAD_FRAME:
		return "Frame";
	case FF_THREAD_SLICE:
		return "Slice";
	default:
		return "None";
	}
}

void tools::print_av_option_bool(AVCodecContext* ctx_codec, const char* option, std::string text, bool inverse)
{
	print_av_option_bool(ctx_codec, ctx_codec, option, text, inverse);
}

void tools::print_av_option_bool(AVCodecContext* ctx_codec, void* ctx_option, const char* option, std::string text,
								 bool inverse)
{
	int64_t v = 0;
	if (int err = av_opt_get_int(ctx_option, option, AV_OPT_SEARCH_CHILDREN, &v); err != 0) {
		DLOG_INFO("[%s] %s: <Error: %s>", ctx_codec->codec->name, text.c_str(),
				  streamfx::ffmpeg::tools::get_error_description(err));
	} else {
		DLOG_INFO("[%s] %s: %s%s", ctx_codec->codec->name, text.c_str(),
				  (inverse ? v != 0 : v == 0) ? "Disabled" : "Enabled",
				  av_opt_is_set_to_default_by_name(ctx_option, option, AV_OPT_SEARCH_CHILDREN) > 0 ? " <Default>" : "");
	}
}

void tools::print_av_option_int(AVCodecContext* ctx_codec, const char* option, std::string text, std::string suffix)
{
	print_av_option_int(ctx_codec, ctx_codec, option, text, suffix);
}

void tools::print_av_option_int(AVCodecContext* ctx_codec, void* ctx_option, const char* option, std::string text,
								std::string suffix)
{
	int64_t v          = 0;
	bool    is_default = av_opt_is_set_to_default_by_name(ctx_option, option, AV_OPT_SEARCH_CHILDREN) > 0;
	if (int err = av_opt_get_int(ctx_option, option, AV_OPT_SEARCH_CHILDREN, &v); err != 0) {
		if (is_default) {
			DLOG_INFO("[%s] %s: <Default>", ctx_codec->codec->name, text.c_str());
		} else {
			DLOG_INFO("[%s] %s: <Error: %s>", ctx_codec->codec->name, text.c_str(),
					  streamfx::ffmpeg::tools::get_error_description(err));
		}
	} else {
		DLOG_INFO("[%s] %s: %" PRId64 " %s%s", ctx_codec->codec->name, text.c_str(), v, suffix.c_str(),
				  is_default ? " <Default>" : "");
	}
}

void tools::print_av_option_string(AVCodecContext* ctx_codec, const char* option, std::string text,
								   std::function<std::string(int64_t)> decoder)
{
	print_av_option_string(ctx_codec, ctx_codec, option, text, decoder);
}

void tools::print_av_option_string(AVCodecContext* ctx_codec, void* ctx_option, const char* option, std::string text,
								   std::function<std::string(int64_t)> decoder)
{
	int64_t v = 0;
	if (int err = av_opt_get_int(ctx_option, option, AV_OPT_SEARCH_CHILDREN, &v); err != 0) {
		DLOG_INFO("[%s] %s: <Error: %s>", ctx_codec->codec->name, text.c_str(),
				  streamfx::ffmpeg::tools::get_error_description(err));
	} else {
		std::string name = "<Unknown>";
		if (decoder)
			name = decoder(v);
		DLOG_INFO("[%s] %s: %s%s", ctx_codec->codec->name, text.c_str(), name.c_str(),
				  av_opt_is_set_to_default_by_name(ctx_option, option, AV_OPT_SEARCH_CHILDREN) > 0 ? " <Default>" : "");
	}
}

void tools::print_av_option_string2(AVCodecContext* ctx_codec, std::string_view option, std::string_view text,
									std::function<std::string(int64_t, std::string_view)> decoder)
{
	print_av_option_string2(ctx_codec, ctx_codec, option, text, decoder);
}

void tools::print_av_option_string2(AVCodecContext* ctx_codec, void* ctx_option, std::string_view option,
									std::string_view                                      text,
									std::function<std::string(int64_t, std::string_view)> decoder)
{
	int64_t v = 0;
	if (int err = av_opt_get_int(ctx_option, option.data(), AV_OPT_SEARCH_CHILDREN, &v); err != 0) {
		DLOG_INFO("[%s] %s: <Error: %s>", ctx_codec->codec->name, text.data(), tools::get_error_description(err));
	} else {
		std::string name = "<Unknown>";

		// Find the unit for the option.
		auto* opt = av_opt_find(ctx_option, option.data(), nullptr, 0, AV_OPT_SEARCH_CHILDREN);
		if (opt && opt->unit) {
			for (auto* opt_test = opt; (opt_test = av_opt_next(ctx_option, opt_test)) != nullptr;) {
				// Skip this entry if the unit doesn't match.
				if ((opt_test->unit == nullptr) || (strcmp(opt_test->unit, opt->unit) != 0)) {
					continue;
				}

				// Assign correct name if we found one.
				if (opt_test->default_val.i64 == v) {
					name = opt_test->name;
					break;
				}
			}

			if (decoder) {
				name = decoder(v, name);
			}
			DLOG_INFO("[%s] %s: %s%s", ctx_codec->codec->name, text.data(), name.c_str(),
					  av_opt_is_set_to_default_by_name(ctx_option, option.data(), AV_OPT_SEARCH_CHILDREN) > 0
						  ? " <Default>"
						  : "");
		} else {
			DLOG_INFO("[%s] %s: %" PRId64 "%s", ctx_codec->codec->name, text.data(), v,
					  av_opt_is_set_to_default_by_name(ctx_option, option.data(), AV_OPT_SEARCH_CHILDREN) > 0
						  ? " <Default>"
						  : "");
		}
	}
}
