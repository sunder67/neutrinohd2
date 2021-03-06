/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __plugins__
#define __plugins__

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>
#include <system/localize.h>

#include <string>
#include <vector>


typedef void (*PluginExec)(void);
typedef void (*PluginInit)(void);
typedef void (*PluginDel)(void);

typedef enum plugin_type
{
	PLUGIN_TYPE_DISABLED = 0,
	PLUGIN_TYPE_GAME     = 1,
	PLUGIN_TYPE_TOOL     = 2,
	PLUGIN_TYPE_SCRIPT   = 3,
	PLUGIN_TYPE_NEUTRINO = 4
}
plugin_type_t;


class CPlugins
{
	public:
		typedef enum p_type
		{
			P_TYPE_DISABLED = 0x1,
			P_TYPE_GAME     = 0x2,
			P_TYPE_TOOL     = 0x4,
			P_TYPE_SCRIPT   = 0x8,
			P_TYPE_NEUTRINO = 0x10
		}
		p_type_t;

	private:

		CFrameBuffer	*frameBuffer;

		struct plugin
		{
			std::string filename;
			std::string cfgfile;
			std::string pluginfile;
			std::string name;               // UTF-8 encoded
			std::string description;        // UTF-8 encoded
			std::string version;
			CPlugins::p_type_t type;
			std::string icon;		// Icon
			bool hide;
			
			bool operator< (const plugin& a) const
			{
				return this->filename < a.filename ;
			}
		};

		int number_of_plugins;

		std::vector<plugin> plugin_list;
		std::string plugin_dir;

		bool parseCfg(plugin *plugin_data);
		void addPlugin(const char *dir);
		int find_plugin(const std::string & filename);
		CPlugins::p_type_t getPluginType(int type);
	public:

		~CPlugins();
		
		bool pluginfile_exists(const std::string & filename);
		bool plugin_exists(const std::string & filename);

		void loadPlugins();

		void setPluginDir(const std::string & dir) { plugin_dir = dir; }

		inline int getNumberOfPlugins(void) const { return plugin_list.size(); }
		inline const char * getName(const int number) const { return plugin_list[number].name.c_str(); }
		inline const char * getPluginFile(const int number) const { return plugin_list[number].pluginfile.c_str(); }
		inline const char * getFileName(const int number) const { return plugin_list[number].filename.c_str(); }
		inline const std::string & getDescription(const int number) const { return plugin_list[number].description; }
		inline const std::string & getVersion(const int number) const { return plugin_list[number].version; }
		inline int getType(const int number) const { return plugin_list[number].type; }
		inline const char * getIcon(const int number) const { return plugin_list[number].icon.c_str(); }
		inline bool isHidden(const int number) const { return plugin_list[number].hide; }

		void startPlugin(int number);
		void start_plugin_by_name(const std::string & filename);	// start plugins by "name=" in .cfg
		void startScriptPlugin(int number);

		void startPlugin(const char * const filename); 			// start plugins also by name
		bool hasPlugin(CPlugins::p_type_t type);
		
		void removePlugin(int number);
};

#endif
