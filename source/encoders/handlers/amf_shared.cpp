// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (c) 2020 Michael Fabian Dirks <info@xaymar.com>
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

#include "amf_shared.hpp"
#include "ffmpeg/tools.hpp"

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavutil/opt.h>
#pragma warning(pop)
}

// Translation
#define ST_I18N "Encoder.FFmpeg.AMF"
#define ST_I18N_PRESET ST_I18N ".Preset"
#define ST_I18N_PRESET_(x) ST_I18N_PRESET "." x
#define ST_I18N_RATECONTROL "Encoder.FFmpeg.AMF.RateControl"
#define ST_I18N_RATECONTROL_MODE ST_I18N_RATECONTROL ".Mode"
#define ST_I18N_RATECONTROL_MODE_(x) ST_I18N_RATECONTROL_MODE "." x
#define ST_I18N_RATECONTROL_LOOKAHEAD ST_I18N_RATECONTROL ".LookAhead"
#define ST_I18N_RATECONTROL_FRAMESKIPPING ST_I18N_RATECONTROL ".FrameSkipping"
#define ST_I18N_RATECONTROL_LIMITS ST_I18N_RATECONTROL ".Limits"
#define ST_I18N_RATECONTROL_LIMITS_BUFFERSIZE ST_I18N_RATECONTROL_LIMITS ".BufferSize"
#define ST_I18N_RATECONTROL_LIMITS_BITRATE ST_I18N_RATECONTROL_LIMITS ".Bitrate"
#define ST_I18N_RATECONTROL_LIMITS_BITRATE_TARGET ST_I18N_RATECONTROL_LIMITS_BITRATE ".Target"
#define ST_I18N_RATECONTROL_LIMITS_BITRATE_MAXIMUM ST_I18N_RATECONTROL_LIMITS_BITRATE ".Maximum"
#define ST_I18N_RATECONTROL_QP ST_I18N_RATECONTROL ".QP"
#define ST_I18N_RATECONTROL_QP_I ST_I18N_RATECONTROL_QP ".I"
#define ST_I18N_RATECONTROL_QP_P ST_I18N_RATECONTROL_QP ".P"
#define ST_I18N_RATECONTROL_QP_B ST_I18N_RATECONTROL_QP ".B"
#define ST_I18N_OTHER ST_I18N ".Other"
#define ST_I18N_OTHER_BFRAMES ST_I18N_OTHER ".BFrames"
#define ST_I18N_OTHER_BFRAMEREFERENCES ST_I18N_OTHER ".BFrameReferences"
#define ST_I18N_OTHER_REFERENCEFRAMES ST_I18N_OTHER ".ReferenceFrames"
#define ST_I18N_OTHER_ENFORCEHRD ST_I18N_OTHER ".EnforceHRD"
#define ST_I18N_OTHER_VBAQ ST_I18N_OTHER ".VBAQ"
#define ST_I18N_OTHER_ACCESSUNITDELIMITER ST_I18N_OTHER ".AccessUnitDelimiter"

// Settings
#define ST_KEY_PRESET "Preset"
#define ST_KEY_RATECONTROL_MODE "RateControl.Mode"
#define ST_KEY_RATECONTROL_LOOKAHEAD "RateControl.LookAhead"
#define ST_KEY_RATECONTROL_FRAMESKIPPING "RateControl.FrameSkipping"
#define ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE "RateControl.Limits.BufferSize"
#define ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET "RateControl.Limits.Bitrate.Target"
#define ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM "RateControl.Limits.Bitrate.Maximum"
#define ST_KEY_RATECONTROL_QP_I "RateControl.QP.I"
#define ST_KEY_RATECONTROL_QP_P "RateControl.QP.P"
#define ST_KEY_RATECONTROL_QP_B "RateControl.QP.B"
#define ST_KEY_OTHER_BFRAMES "Other.BFrames"
#define ST_KEY_OTHER_BFRAMEREFERENCES "Other.BFrameReferences"
#define ST_KEY_OTHER_REFERENCEFRAMES "Other.ReferenceFrames"
#define ST_KEY_OTHER_ENFORCEHRD "Other.EnforceHRD"
#define ST_KEY_OTHER_VBAQ "Other.VBAQ"
#define ST_KEY_OTHER_ACCESSUNITDELIMITER "Other.AccessUnitDelimiter"

using namespace streamfx::encoder::ffmpeg::handler;

std::map<amf::preset, std::string> amf::presets{
	{amf::preset::SPEED, ST_I18N_PRESET_("Speed")},
	{amf::preset::BALANCED, ST_I18N_PRESET_("Balanced")},
	{amf::preset::QUALITY, ST_I18N_PRESET_("Quality")},
};

std::map<amf::preset, std::string> amf::preset_to_opt{
	{amf::preset::SPEED, "speed"},
	{amf::preset::BALANCED, "balanced"},
	{amf::preset::QUALITY, "quality"},
};

std::map<amf::ratecontrolmode, std::string> amf::ratecontrolmodes{
	{amf::ratecontrolmode::CQP, ST_I18N_RATECONTROL_MODE_("CQP")},
	{amf::ratecontrolmode::CBR, ST_I18N_RATECONTROL_MODE_("CBR")},
	{amf::ratecontrolmode::VBR_PEAK, ST_I18N_RATECONTROL_MODE_("VBR_PEAK")},
	{amf::ratecontrolmode::VBR_LATENCY, ST_I18N_RATECONTROL_MODE_("VBR_LATENCY")},
};

std::map<amf::ratecontrolmode, std::string> amf::ratecontrolmode_to_opt{
	{amf::ratecontrolmode::CQP, "cqp"},
	{amf::ratecontrolmode::CBR, "cbr"},
	{amf::ratecontrolmode::VBR_PEAK, "vbr_peak"},
	{amf::ratecontrolmode::VBR_LATENCY, "vbr_latency"},
};

bool streamfx::encoder::ffmpeg::handler::amf::is_available()
{
#if defined(D_PLATFORM_WINDOWS)
#if defined(D_PLATFORM_64BIT)
	std::filesystem::path lib_name = std::filesystem::u8path("amfrt64.dll");
#else
	std::filesystem::path lib_name = std::filesystem::u8path("amfrt32.dll");
#endif
#elif defined(D_PLATFORM_LINUX)
#if defined(D_PLATFORM_64BIT)
	std::filesystem::path lib_name = std::filesystem::u8path("libamfrt64.so.1");
#else
	std::filesystem::path lib_name = std::filesystem::u8path("libamfrt32.so.1");
#endif
#endif
	try {
		streamfx::util::library::load(lib_name);
		return true;
	} catch (...) {
		return false;
	}
}

void amf::get_defaults(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context)
{
	obs_data_set_default_int(settings, ST_KEY_PRESET, static_cast<int64_t>(amf::preset::BALANCED));

	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_MODE, static_cast<int64_t>(ratecontrolmode::CBR));
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LOOKAHEAD, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_FRAMESKIPPING, -1);
	//ob
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET, 6000);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM, 0);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE, 12000);

	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_I, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_P, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_B, -1);

	obs_data_set_default_int(settings, ST_KEY_OTHER_BFRAMES, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_BFRAMEREFERENCES, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_REFERENCEFRAMES, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_ENFORCEHRD, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_VBAQ, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_ACCESSUNITDELIMITER, -1);

	// Replay Buffer
	obs_data_set_default_int(settings, "bitrate", 0);
}

static bool modified_ratecontrol(obs_properties_t* props, obs_property_t*, obs_data_t* settings) noexcept
{
	bool have_bitrate       = false;
	bool have_bitrate_range = false;
	bool have_qp            = false;

	amf::ratecontrolmode rc = static_cast<amf::ratecontrolmode>(obs_data_get_int(settings, ST_KEY_RATECONTROL_MODE));
	switch (rc) {
	case amf::ratecontrolmode::CQP:
		have_qp = true;
		break;
	case amf::ratecontrolmode::INVALID:
	case amf::ratecontrolmode::CBR:
		have_bitrate = true;
		break;
	case amf::ratecontrolmode::VBR_PEAK:
	case amf::ratecontrolmode::VBR_LATENCY:
		have_bitrate       = true;
		have_bitrate_range = true;
		break;
	}

	obs_property_set_visible(obs_properties_get(props, ST_I18N_RATECONTROL_LIMITS), have_bitrate);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE), have_bitrate);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET), have_bitrate);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM), have_bitrate_range);

	obs_property_set_visible(obs_properties_get(props, ST_I18N_RATECONTROL_QP), have_qp);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_I), have_qp);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_P), have_qp);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_B), have_qp);

	return true;
}

void amf::get_properties_pre(obs_properties_t* props, const AVCodec* codec)
{
	auto p = obs_properties_add_list(props, ST_KEY_PRESET, D_TRANSLATE(ST_I18N_PRESET), OBS_COMBO_TYPE_LIST,
									 OBS_COMBO_FORMAT_INT);
	for (auto kv : presets) {
		obs_property_list_add_int(p, D_TRANSLATE(kv.second.c_str()), static_cast<int64_t>(kv.first));
	}
}

void amf::get_properties_post(obs_properties_t* props, const AVCodec* codec)
{
	{ // Rate Control
		obs_properties_t* grp = obs_properties_create();
		obs_properties_add_group(props, ST_I18N_RATECONTROL, D_TRANSLATE(ST_I18N_RATECONTROL), OBS_GROUP_NORMAL, grp);

		{
			auto p = obs_properties_add_list(grp, ST_KEY_RATECONTROL_MODE, D_TRANSLATE(ST_I18N_RATECONTROL_MODE),
											 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_modified_callback(p, modified_ratecontrol);
			for (auto kv : ratecontrolmodes) {
				obs_property_list_add_int(p, D_TRANSLATE(kv.second.c_str()), static_cast<int64_t>(kv.first));
			}
		}

		streamfx::util::obs_properties_add_tristate(grp, ST_KEY_RATECONTROL_LOOKAHEAD,
													D_TRANSLATE(ST_I18N_RATECONTROL_LOOKAHEAD));
		streamfx::util::obs_properties_add_tristate(grp, ST_KEY_RATECONTROL_FRAMESKIPPING,
													D_TRANSLATE(ST_I18N_RATECONTROL_FRAMESKIPPING));
	}

	{
		obs_properties_t* grp = obs_properties_create();
		obs_properties_add_group(props, ST_I18N_RATECONTROL_LIMITS, D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS),
								 OBS_GROUP_NORMAL, grp);

		{
			auto p = obs_properties_add_int(grp, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET,
											D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_BITRATE_TARGET), -1,
											std::numeric_limits<std::int32_t>::max(), 1);
			obs_property_int_set_suffix(p, " kbit/s");
		}

		{
			auto p = obs_properties_add_int(grp, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM,
											D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_BITRATE_MAXIMUM), -1,
											std::numeric_limits<std::int32_t>::max(), 1);
			obs_property_int_set_suffix(p, " kbit/s");
		}

		{
			auto p = obs_properties_add_int(grp, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE,
											D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_BUFFERSIZE), 0,
											std::numeric_limits<std::int32_t>::max(), 1);
			obs_property_int_set_suffix(p, " kbit");
		}
	}

	{
		obs_properties_t* grp = obs_properties_create();
		obs_properties_add_group(props, ST_I18N_RATECONTROL_QP, D_TRANSLATE(ST_I18N_RATECONTROL_QP), OBS_GROUP_NORMAL,
								 grp);

		obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_I, D_TRANSLATE(ST_I18N_RATECONTROL_QP_I), -1, 51, 1);
		obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_P, D_TRANSLATE(ST_I18N_RATECONTROL_QP_P), -1, 51, 1);

		if (std::string_view("amf_h264") == codec->name) {
			obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_B, D_TRANSLATE(ST_I18N_RATECONTROL_QP_B), -1, 51,
										  1);
		}
	}

	{
		obs_properties_t* grp = obs_properties_create();
		obs_properties_add_group(props, ST_I18N_OTHER, D_TRANSLATE(ST_I18N_OTHER), OBS_GROUP_NORMAL, grp);

		{
			auto p =
				obs_properties_add_int_slider(grp, ST_KEY_OTHER_BFRAMES, D_TRANSLATE(ST_I18N_OTHER_BFRAMES), -1, 4, 1);
			obs_property_int_set_suffix(p, " frames");
		}

		streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_BFRAMEREFERENCES,
													D_TRANSLATE(ST_I18N_OTHER_BFRAMEREFERENCES));
		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_OTHER_REFERENCEFRAMES,
												   D_TRANSLATE(ST_I18N_OTHER_REFERENCEFRAMES), -1, 16, 1);
			obs_property_int_set_suffix(p, " frames");
		}
		streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_ENFORCEHRD,
													D_TRANSLATE(ST_I18N_OTHER_ENFORCEHRD));
		streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_VBAQ, D_TRANSLATE(ST_I18N_OTHER_VBAQ));
		streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_ACCESSUNITDELIMITER,
													D_TRANSLATE(ST_I18N_OTHER_ACCESSUNITDELIMITER));
	}
}

void amf::update(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context)
{
	// Alway enable loop filter.
	context->flags |= AV_CODEC_FLAG_LOOP_FILTER;

	// Always transcoding. Other usage options cause problems.
	av_opt_set(context->priv_data, "usage", "transcoding", AV_OPT_SEARCH_CHILDREN);

	{ // Presets
		preset c_preset = static_cast<preset>(obs_data_get_int(settings, ST_KEY_PRESET));
		auto   found    = preset_to_opt.find(c_preset);
		if (found != preset_to_opt.end()) {
			av_opt_set(context->priv_data, "quality", found->second.c_str(), AV_OPT_SEARCH_CHILDREN);
		} else {
			av_opt_set(context->priv_data, "quality", nullptr, AV_OPT_SEARCH_CHILDREN);
		}
	}

	{ // Rate Control
		bool have_bitrate       = false;
		bool have_bitrate_range = false;
		bool have_qp            = false;

		ratecontrolmode rc    = static_cast<ratecontrolmode>(obs_data_get_int(settings, ST_KEY_RATECONTROL_MODE));
		auto            rcopt = ratecontrolmode_to_opt.find(rc);
		if (rcopt != ratecontrolmode_to_opt.end()) {
			av_opt_set(context->priv_data, "rc", rcopt->second.c_str(), AV_OPT_SEARCH_CHILDREN);
		} else {
			have_bitrate = true;
			av_opt_set(context->priv_data, "rc", "cbr", AV_OPT_SEARCH_CHILDREN);
		}

		av_opt_set_int(context->priv_data, "filler_data", 0, AV_OPT_SEARCH_CHILDREN);
		switch (rc) {
		case ratecontrolmode::CQP:
			have_qp = true;
			break;
		case ratecontrolmode::INVALID:
		case ratecontrolmode::CBR:
			have_bitrate = true;
			av_opt_set_int(context->priv_data, "filler_data", 1, AV_OPT_SEARCH_CHILDREN);
			break;
		case ratecontrolmode::VBR_PEAK:
		case ratecontrolmode::VBR_LATENCY:
			have_bitrate_range = true;
			have_bitrate       = true;
			break;
		}

		// Look Ahead (Pre-analysis, single frame lookahead)
		if (int la = static_cast<int>(obs_data_get_int(settings, ST_KEY_RATECONTROL_LOOKAHEAD));
			!streamfx::util::is_tristate_default(la)) {
			av_opt_set_int(context->priv_data, "preanalysis", la, AV_OPT_SEARCH_CHILDREN);
		}

		// Frame Skipping (Drop frames to maintain bitrate limits)
		if (int la = static_cast<int>(obs_data_get_int(settings, ST_KEY_RATECONTROL_FRAMESKIPPING));
			!streamfx::util::is_tristate_default(la)) {
			if (std::string_view("amf_h264") == codec->name) {
				av_opt_set_int(context->priv_data, "frame_skipping", la, AV_OPT_SEARCH_CHILDREN);
			} else {
				av_opt_set_int(context->priv_data, "skip_frame", la, AV_OPT_SEARCH_CHILDREN);
			}
		}

		if (have_bitrate) {
			int64_t v = obs_data_get_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET);
			if (v > -1) {
				context->bit_rate    = static_cast<int>(v * 1000);
				context->rc_max_rate = context->bit_rate;

				// Support for Replay Buffer
				obs_data_set_int(settings, "bitrate", v);
			} else {
				obs_data_set_int(settings, "bitrate", context->bit_rate);
			}
		} else {
			context->bit_rate = 0;
		}
		if (have_bitrate_range) {
			if (int64_t max = obs_data_get_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM); max > -1)
				context->rc_max_rate = static_cast<int>(max * 1000);
		} else {
			context->rc_max_rate = 0;
		}

		// Buffer Size
		if (have_bitrate || have_bitrate_range) {
			if (int64_t v = obs_data_get_int(settings, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE); v > -1)
				context->rc_buffer_size = static_cast<int>(v * 1000);
		} else {
			context->rc_buffer_size = 0;
		}

		// QP Settings
		if (have_qp) {
			if (int64_t qp = obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_I); qp > -1) {
				av_opt_set_int(context->priv_data, "qp_i", static_cast<int>(qp), AV_OPT_SEARCH_CHILDREN);
			}
			if (int64_t qp = obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_P); qp > -1) {
				av_opt_set_int(context->priv_data, "qp_p", static_cast<int>(qp), AV_OPT_SEARCH_CHILDREN);
			}
			if (std::string_view("amf_h264") == codec->name) {
				if (int64_t qp = obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_B); qp > -1) {
					av_opt_set_int(context->priv_data, "qp_b", static_cast<int>(qp), AV_OPT_SEARCH_CHILDREN);
				}
			}
		}
	}

	{ // Other
		if (std::string_view("amf_h264") == codec->name) {
			if (int64_t bf = obs_data_get_int(settings, ST_KEY_OTHER_BFRAMES); bf > -1) {
				context->max_b_frames = static_cast<int>(bf);
			}
			if (int64_t zl = obs_data_get_int(settings, ST_KEY_OTHER_BFRAMEREFERENCES);
				!streamfx::util::is_tristate_default(zl)) {
				av_opt_set_int(context->priv_data, "bf_ref", zl, AV_OPT_SEARCH_CHILDREN);
			}
		}

		if (int64_t refs = obs_data_get_int(settings, ST_KEY_OTHER_REFERENCEFRAMES); refs > -1) {
			context->refs = static_cast<int>(refs);
		}

		if (int64_t v = obs_data_get_int(settings, ST_KEY_OTHER_ENFORCEHRD); !streamfx::util::is_tristate_default(v)) {
			av_opt_set_int(context->priv_data, "enforce_hrd", v, AV_OPT_SEARCH_CHILDREN);
		}

		if (int64_t v = obs_data_get_int(settings, ST_KEY_OTHER_VBAQ); !streamfx::util::is_tristate_default(v)) {
			av_opt_set_int(context->priv_data, "vbaq", v, AV_OPT_SEARCH_CHILDREN);
		}

		if (int64_t v = obs_data_get_int(settings, ST_KEY_OTHER_ACCESSUNITDELIMITER);
			!streamfx::util::is_tristate_default(v)) {
			av_opt_set_int(context->priv_data, "aud", v, AV_OPT_SEARCH_CHILDREN);
		}

		av_opt_set_int(context->priv_data, "me_half_pel", 1, AV_OPT_SEARCH_CHILDREN);
		av_opt_set_int(context->priv_data, "me_quarter_pel", 1, AV_OPT_SEARCH_CHILDREN);
	}
}

void amf::log_options(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context)
{
	using namespace ::streamfx::ffmpeg;

	DLOG_INFO("[%s]   AMD AMF:", codec->name);
	tools::print_av_option_string2(context, "usage", "    Usage",
								   [](int64_t v, std::string_view o) { return std::string(o); });
	tools::print_av_option_string2(context, "quality", "    Preset",
								   [](int64_t v, std::string_view o) { return std::string(o); });
	tools::print_av_option_string2(context, "rc", "    Rate Control",
								   [](int64_t v, std::string_view o) { return std::string(o); });
	tools::print_av_option_bool(context, "preanalysis", "      Look-Ahead");
	if (std::string_view("amf_h264") == codec->name) {
		tools::print_av_option_bool(context, "frame_skipping", "      Frame Skipping");
	} else {
		tools::print_av_option_bool(context, "skip_frame", "      Frame Skipping");
	}
	tools::print_av_option_bool(context, "filler_data", "      Filler Data");

	DLOG_INFO("[%s]       Bitrate:", codec->name);
	tools::print_av_option_int(context, "b", "        Target", "bits/sec");
	tools::print_av_option_int(context, "maxrate", "        Maximum", "bits/sec");
	tools::print_av_option_int(context, "bufsize", "        Buffer", "bits");
	DLOG_INFO("[%s]       Quantization Parameters:", codec->name);
	tools::print_av_option_int(context, "qp_i", "        I-Frame", "");
	tools::print_av_option_int(context, "qp_p", "        P-Frame", "");
	if (std::string_view("amf_h264") == codec->name) { // B-Frames
		tools::print_av_option_int(context, "qp_b", "        B-Frame", "");

		tools::print_av_option_int(context, "bf", "    B-Frames", "Frames");
		tools::print_av_option_int(context, "bf_delta_qp", "      Delta QP", "");
		tools::print_av_option_bool(context, "bf_ref", "      References");
		tools::print_av_option_int(context, "bf_ref_delta_qp", "        Delta QP", "");
	}

	DLOG_INFO("[%s]     Other:", codec->name);
	tools::print_av_option_int(context, "refs", "      Reference Frames", "Frames");
	tools::print_av_option_bool(context, "enforce_hrd", "      Enforce HRD");
	tools::print_av_option_bool(context, "vbaq", "      VBAQ");
	tools::print_av_option_bool(context, "aud", "      Access Unit Delimiter");
	tools::print_av_option_int(context, "max_au_size", "        Maximum Size", "");
	tools::print_av_option_bool(context, "me_half_pel", "      Half-Pel Motion Estimation");
	tools::print_av_option_bool(context, "me_quarter_pel", "      Quarter-Pel Motion Estimation");
}

void streamfx::encoder::ffmpeg::handler::amf::get_runtime_properties(obs_properties_t* props, const AVCodec* codec,
																	 AVCodecContext* context)
{}

void streamfx::encoder::ffmpeg::handler::amf::migrate(obs_data_t* settings, uint64_t version, const AVCodec* codec,
													  AVCodecContext* context)
{}

void streamfx::encoder::ffmpeg::handler::amf::override_update(ffmpeg_instance* instance, obs_data_t* settings) {}
