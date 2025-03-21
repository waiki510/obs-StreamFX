/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2020 Michael Fabian Dirks
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

#include "ui.hpp"
#include "common.hpp"
#include "strings.hpp"
#include <string_view>
#include "configuration.hpp"
#include "obs/obs-tools.hpp"
#include "plugin.hpp"

#include <obs-frontend-api.h>

// Translation Keys
constexpr std::string_view _i18n_prefix       = "StreamFX::";
constexpr std::string_view _i18n_menu         = "UI.Menu";
constexpr std::string_view _i18n_menu_support = "UI.Menu.Support";
constexpr std::string_view _i18n_menu_wiki    = "UI.Menu.Wiki";
constexpr std::string_view _i18n_menu_website = "UI.Menu.Website";
constexpr std::string_view _i18n_menu_discord = "UI.Menu.Discord";
constexpr std::string_view _i18n_menu_youtube = "UI.Menu.YouTube";
constexpr std::string_view _i18n_menu_twitter = "UI.Menu.Twitter";
constexpr std::string_view _i18n_menu_github  = "UI.Menu.Github";
constexpr std::string_view _i18n_menu_about   = "UI.Menu.About";

// Configuration
constexpr std::string_view _cfg_have_shown_about = "UI.HaveShownAboutStreamFX";

// URLs
constexpr std::string_view _url_support = "https://s.xaymar.com/streamfx-dc-support";
constexpr std::string_view _url_wiki    = "https://github.com/Xaymar/obs-StreamFX/wiki";
constexpr std::string_view _url_website = "https://streamfx.xaymar.com";
constexpr std::string_view _url_discord = "https://s.xaymar.com/streamfx-dc";
constexpr std::string_view _url_twitter = "https://s.xaymar.com/streamfx-tw";
constexpr std::string_view _url_youtube = "https://s.xaymar.com/streamfx-yt";

inline void qt_init_resource()
{
	Q_INIT_RESOURCE(streamfx);
}

inline void qt_cleanup_resource()
{
	Q_CLEANUP_RESOURCE(streamfx);
}

bool streamfx::ui::handler::have_shown_about_streamfx(bool shown)
{
	auto config = streamfx::configuration::instance();
	auto data   = config->get();
	if (shown) {
		obs_data_set_bool(data.get(), _cfg_have_shown_about.data(), true);
	}
	if (config->is_different_version()) {
		return false;
	} else {
		return obs_data_get_bool(data.get(), _cfg_have_shown_about.data());
	}
}

streamfx::ui::handler::handler()
	: QObject(), _menu_action(), _menu(),

	  _action_support(), _action_wiki(), _action_website(), _action_discord(), _action_twitter(), _action_youtube(),

	  _about_action(), _about_dialog(),

	  _translator()
#ifdef ENABLE_UPDATER
	  ,
	  _updater()
#endif
{
	obs_frontend_add_event_callback(frontend_event_handler, this);
}

streamfx::ui::handler::~handler()
{
	obs_frontend_remove_event_callback(frontend_event_handler, this);
}

void streamfx::ui::handler::frontend_event_handler(obs_frontend_event event, void* private_data)
{
	streamfx::ui::handler* ptr = reinterpret_cast<streamfx::ui::handler*>(private_data);
	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		ptr->on_obs_loaded();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		ptr->on_obs_exit();
		break;
	default:
		break;
	}
}

void streamfx::ui::handler::on_obs_loaded()
{
	// Initialize the required Qt resources.
	qt_init_resource();

	// Add our own translation helper to the Qt Application.
	_translator = new streamfx::ui::translator(this);
	QCoreApplication::installTranslator(_translator);

	// Create the 'About StreamFX' dialog.
	_about_dialog = new streamfx::ui::about();

	{ // Create and build the StreamFX menu
		_menu = new QMenu(reinterpret_cast<QWidget*>(obs_frontend_get_main_window()));

		// Wiki
		// Help & Support
		// ---
		// Website
		// Discord
		// Twitter
		// YouTube
		// <--->
		// <Updater>
		// ---
		// About StreamFX

		{
			// Wiki
			_action_wiki = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_wiki.data())));
			_action_wiki->setMenuRole(QAction::NoRole);
			connect(_action_wiki, &QAction::triggered, this, &streamfx::ui::handler::on_action_wiki);

			// Help & Support
			_action_support = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_support.data())));
			_action_support->setMenuRole(QAction::NoRole);
			connect(_action_support, &QAction::triggered, this, &streamfx::ui::handler::on_action_support);
		}

		_menu->addSeparator();
		{
			_action_website = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_website.data())));
			_action_website->setMenuRole(QAction::NoRole);
			connect(_action_website, &QAction::triggered, this, &streamfx::ui::handler::on_action_website);

			_action_discord = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_discord.data())));
			_action_discord->setMenuRole(QAction::NoRole);
			connect(_action_discord, &QAction::triggered, this, &streamfx::ui::handler::on_action_discord);

			_action_twitter = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_twitter.data())));
			_action_twitter->setMenuRole(QAction::NoRole);
			connect(_action_twitter, &QAction::triggered, this, &streamfx::ui::handler::on_action_twitter);

			_action_youtube = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_youtube.data())));
			_action_youtube->setMenuRole(QAction::NoRole);
			connect(_action_youtube, &QAction::triggered, this, &streamfx::ui::handler::on_action_youtube);
		}

		// Create the updater.
#ifdef ENABLE_UPDATER
		_updater = streamfx::ui::updater::instance(_menu);
#endif

		_menu->addSeparator();

		// About
		_about_action = _menu->addAction(QString::fromUtf8(D_TRANSLATE(_i18n_menu_about.data())));
		_about_action->setMenuRole(QAction::NoRole);
		connect(_about_action, &QAction::triggered, this, &streamfx::ui::handler::on_action_about);
	}

	{ // Add an actual Menu entry.
		QMainWindow* main_widget = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
		_menu_action             = new QAction(main_widget);
		_menu_action->setMenuRole(QAction::NoRole);
		_menu_action->setMenu(_menu);
		_menu_action->setText(QString::fromUtf8(D_TRANSLATE(_i18n_menu.data())));

		// For unknown reasons, the "About StreamFX" menu replaces the OBS about menu.
		QList<QMenu*> obs_menus = main_widget->menuBar()->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
		if (QMenu* help_menu = obs_menus.at(1); help_menu) {
			main_widget->menuBar()->insertAction(help_menu->menuAction(), _menu_action);
		} else {
			main_widget->menuBar()->addAction(_menu_action);
		}
	}

	// Show the 'About StreamFX' dialog if that has not happened yet.
	if (!have_shown_about_streamfx()) {
		// Automatically show it if it has not yet been shown.
		_about_dialog->show();
		have_shown_about_streamfx(true);
	}

	// Let the Updater start its work.

#ifdef ENABLE_UPDATER
	this->_updater->obs_ready();
#endif
}

void streamfx::ui::handler::on_obs_exit()
{
	// Remove translator.
	QCoreApplication::removeTranslator(_translator);

	// Clean up any Qt resources we added.
	qt_cleanup_resource();
}

void streamfx::ui::handler::on_action_support(bool)
{
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(_url_support.data())));
}

void streamfx::ui::handler::on_action_wiki(bool)
{
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(_url_wiki.data())));
}

void streamfx::ui::handler::on_action_website(bool)
{
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(_url_website.data())));
}

void streamfx::ui::handler::on_action_discord(bool)
{
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(_url_discord.data())));
}

void streamfx::ui::handler::on_action_twitter(bool)
{
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(_url_twitter.data())));
}

void streamfx::ui::handler::on_action_youtube(bool)
{
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(_url_youtube.data())));
}

void streamfx::ui::handler::on_action_about(bool checked)
{
	_about_dialog->show();
}

static std::shared_ptr<streamfx::ui::handler> _handler_singleton;

void streamfx::ui::handler::initialize()
{
	_handler_singleton = std::make_shared<streamfx::ui::handler>();
}

void streamfx::ui::handler::finalize()
{
	_handler_singleton.reset();
}

std::shared_ptr<streamfx::ui::handler> streamfx::ui::handler::get()
{
	return _handler_singleton;
}

streamfx::ui::translator::translator(QObject* parent) {}

streamfx::ui::translator::~translator() {}

QString streamfx::ui::translator::translate(const char* context, const char* sourceText, const char* disambiguation,
											int n) const
{
	if (sourceText) {
		std::string_view sourceView{sourceText};
		if (sourceView.substr(0, _i18n_prefix.length()) == _i18n_prefix) {
			return QString::fromUtf8(D_TRANSLATE(sourceView.substr(_i18n_prefix.length()).data()));
		}
	}
	if (disambiguation) {
		std::string_view disambiguationView{disambiguation};
		if (disambiguationView.substr(0, _i18n_prefix.length()) == _i18n_prefix) {
			return QString::fromUtf8(D_TRANSLATE(disambiguationView.substr(_i18n_prefix.length()).data()));
		}
	}
	return QString();
}
