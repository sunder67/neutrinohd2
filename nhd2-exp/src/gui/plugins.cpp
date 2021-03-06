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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/plugins.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <dirent.h>
#include <dlfcn.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <global.h>
#include <neutrino.h>

/*zapit includes*/
#include <client/zapittools.h>

#include <daemonc/remotecontrol.h>
#include <system/safe_system.h>
#include <system/debug.h>
#include <system/helpers.h>

#include <gui/widget/hintbox.h>


extern CPlugins * g_PluginList;    /* neutrino.cpp */

bool CPlugins::plugin_exists(const std::string & filename)
{
	return (find_plugin(filename) >= 0);
}

int CPlugins::find_plugin(const std::string & filename)
{
	for (int i = 0; i <  (int) plugin_list.size(); i++)
	{
		if ( (filename.compare(plugin_list[i].filename) == 0) || (filename.compare(plugin_list[i].filename + ".cfg") == 0) )
			return i;
	}

	return -1;
}

bool CPlugins::pluginfile_exists(const std::string & filename)
{
	FILE *file = fopen(filename.c_str(), "r");
	if (file != NULL)
	{
		fclose(file);
		return true;
	} 
	else
	{
		return false;
	}
}

void CPlugins::addPlugin(const char * dir)
{
	PluginInit initPlugin;
	void *handle = NULL;
	char *error;
	
	struct dirent **namelist;
	std::string fname;

	int number_of_files = scandir(dir, &namelist, 0, alphasort);

	for (int i = 0; i < number_of_files; i++)
	{
		std::string filename;

		filename = namelist[i]->d_name;
		
		int pos = filename.find(".cfg");
		if (pos > -1)
		{
			plugin new_plugin;
			new_plugin.filename = filename.substr(0, pos);
			fname = dir;
			fname += '/';
			new_plugin.cfgfile = fname.append(new_plugin.filename);
			new_plugin.cfgfile.append(".cfg");
			parseCfg(&new_plugin);
			bool plugin_ok = parseCfg(&new_plugin);

			if (plugin_ok) 
			{
				new_plugin.pluginfile = fname;
				if (new_plugin.type == CPlugins::P_TYPE_SCRIPT)
				{
					new_plugin.pluginfile.append(".sh");
				} 
				else if (new_plugin.type == CPlugins::P_TYPE_GAME)
				{
					new_plugin.pluginfile.append(".so");
				}
				else if (new_plugin.type == CPlugins::P_TYPE_TOOL)
				{
					new_plugin.pluginfile.append(".so");
				}
				else if (new_plugin.type == CPlugins::P_TYPE_NEUTRINO)
				{
					new_plugin.pluginfile.append(".so");
					
					// initPlugin
					handle = dlopen ( new_plugin.pluginfile.c_str(), RTLD_NOW);
					if (!handle)
					{
						fputs (dlerror(), stderr);
					} 
					else
					{
						initPlugin = (PluginInit) dlsym(handle, "plugin_init");
						if ((error = dlerror()) != NULL)
						{
							fputs(error, stderr);
							dlclose(handle);
						} 
						else 
						{
							dprintf(DEBUG_DEBUG, "[CPlugins] try init...\n");				
								
							initPlugin();
							dlclose(handle);
							dprintf(DEBUG_DEBUG, "[CPlugins] init done...\n");
						}
					}
				}
				
				//
				if (!plugin_exists(new_plugin.filename))
				{
					plugin_list.push_back(new_plugin);
					number_of_plugins++;
				}
			}
		}
	}
}

void CPlugins::loadPlugins()
{
	dprintf(DEBUG_NORMAL, "CPlugins::loadPlugins\n");
	
	frameBuffer = CFrameBuffer::getInstance();
	number_of_plugins = 0;
	plugin_list.clear();
	
	//
	struct dirent **namelist;
	int i = 0;
	std::string pluginPath;

	i = scandir(PLUGINDIR, &namelist, 0, 0);
	if(i < 0)
	{
		return;
	}

	while(i--)
	{
		if(namelist[i]->d_type == DT_DIR && !strstr(namelist[i]->d_name, ".") && !strstr(namelist[i]->d_name, ".."))
		{
			pluginPath += PLUGINDIR;
			pluginPath += "/";
			pluginPath += namelist[i]->d_name;
			
			addPlugin(pluginPath.c_str());
			pluginPath.clear();			
		}
		free(namelist[i]);
	}
	free(namelist);
	
	sort(plugin_list.begin(), plugin_list.end());
}

CPlugins::~CPlugins()
{
	plugin_list.clear();
}

bool CPlugins::parseCfg(plugin *plugin_data)
{
	std::ifstream inFile;
	std::string line[20];
	int linecount = 0;
	bool reject = false;

	inFile.open(plugin_data->cfgfile.c_str());

	while (linecount < 20 && getline(inFile, line[linecount++]))
	{};

	plugin_data->type = CPlugins::P_TYPE_DISABLED;
	plugin_data->hide = false;

	for (int i = 0; i < linecount; i++)
	{
		std::istringstream iss(line[i]);
		std::string cmd;
		std::string parm;

		getline(iss, cmd, '=');
		getline(iss, parm, '=');

		if (cmd == "name")
		{
			plugin_data->name = parm;
		}
		else if (cmd == "desc")
		{
			plugin_data->description = parm;
		}
		else if (cmd == "version")
		{
			plugin_data->version = parm;
		}
		else if (cmd == "type")
		{
			plugin_data->type = getPluginType(atoi(parm.c_str()));
		}
		else if (cmd == "icon")
		{
			plugin_data->icon = parm;
		}
		else if (cmd == "hide")
		{
			plugin_data->hide = ((parm == "1")?true:false);
		}
	}

	inFile.close();
	return !reject;
}

void CPlugins::start_plugin_by_name(const std::string & filename)
{
	for (int i = 0; i <  (int) plugin_list.size(); i++)
	{
		if (filename.compare(g_PluginList->getName(i))==0)
		{
			startPlugin(i);
			return;
		}
	}
}

void CPlugins::startPlugin(const char * const name)
{
	int pluginnr = find_plugin(name);
	if (pluginnr > -1)
		startPlugin(pluginnr);
	else
	{
		dprintf(DEBUG_NORMAL, "[CPlugins] could not find %s\n", name);
		
		std::string hint = name;
		hint += " ";
		hint += g_Locale->getText(LOCALE_PLUGINS_NOT_INSTALLED);
		
		HintBox(LOCALE_MESSAGEBOX_INFO, hint.c_str());
	}

}

void CPlugins::startScriptPlugin(int number)
{
	const char * script = plugin_list[number].pluginfile.c_str();
	
	dprintf(DEBUG_NORMAL, "[CPlugins] executing script %s\n", script);
	
	if (!pluginfile_exists(plugin_list[number].pluginfile))
	{
		dprintf(DEBUG_NORMAL, "[CPlugins] could not find %s,\nperhaps wrong plugin type in %s\n", script, plugin_list[number].cfgfile.c_str());
		return;
	}
	
	if( !safe_system(script) )
	{
		dprintf(DEBUG_NORMAL, "CPlugins::startScriptPlugin: script %s successfull started\n", script);
	} 
	else 
	{	
		dprintf(DEBUG_NORMAL, "[CPlugins] can't execute %s\n",script);
	}
}

void CPlugins::startPlugin(int number)
{
	dprintf(DEBUG_NORMAL, "CPlugins::startPlugin: %s type:%d\n", plugin_list[number].pluginfile.c_str(), plugin_list[number].type);
	
	// export neutrino settings to the environment
	char tmp[32];
	sprintf(tmp, "%d", g_settings.screen_StartX);
	setenv("SCREEN_OFF_X", tmp, 1);
	sprintf(tmp, "%d", g_settings.screen_StartY);
	setenv("SCREEN_OFF_Y", tmp, 1);
	sprintf(tmp, "%d", g_settings.screen_EndX);
	setenv("SCREEN_END_X", tmp, 1);
	sprintf(tmp, "%d", g_settings.screen_EndY);
	setenv("SCREEN_END_Y", tmp, 1);
	
	// script type
	if (plugin_list[number].type == CPlugins::P_TYPE_SCRIPT)
	{
		g_RCInput->clearRCMsg();
		
		g_RCInput->stopInput();

		startScriptPlugin(number);
		
		frameBuffer->paintBackground();

		frameBuffer->blit();	
		
		g_RCInput->restartInput();
		g_RCInput->clearRCMsg();

		return;
	}

	// neutrinoHD plugins (standalone)
	if ( (plugin_list[number].type == CPlugins::P_TYPE_TOOL) || (plugin_list[number].type == CPlugins::P_TYPE_GAME) )
	{
		/* stop rc input */
		g_RCInput->stopInput();
		
		safe_system((char *) plugin_list[number].pluginfile.c_str());
		
		frameBuffer->paintBackground();

		frameBuffer->blit();
	
		g_RCInput->restartInput();
		g_RCInput->clearRCMsg();
		
		return;
	}
	else if (plugin_list[number].type == CPlugins::P_TYPE_NEUTRINO)
	{
		PluginExec execPlugin;
		void *handle;
		char * error;

		g_RCInput->clearRCMsg();
	
		// load
		handle = dlopen ( plugin_list[number].pluginfile.c_str(), RTLD_NOW);
		if (!handle)
		{
			fputs (dlerror(), stderr);
		} 
		else 
		{
			execPlugin = (PluginExec) dlsym(handle, "plugin_exec");
			if ((error = dlerror()) != NULL)
			{
				fputs(error, stderr);
				dlclose(handle);
			} 
			else 
			{
				dprintf(DEBUG_NORMAL, "[CPlugins] try exec...\n");
				
				frameBuffer->paintBackground();

				frameBuffer->blit();				
					
				execPlugin();
				dlclose(handle);
				dprintf(DEBUG_NORMAL, "[CPlugins] exec done...\n");
			}
		}
			
		g_RCInput->clearRCMsg();
	}
}

void CPlugins::removePlugin(int number)
{
	PluginDel delPlugin;
	void *handle = NULL;
	char *error;
	
	dprintf(DEBUG_NORMAL, "CPlugins::removePlugin: %s type:%d\n", plugin_list[number].pluginfile.c_str(), plugin_list[number].type);
	
	// unload plugin
	// load
	handle = dlopen ( plugin_list[number].pluginfile.c_str(), RTLD_NOW);
	if (!handle)
	{
		fputs (dlerror(), stderr);
	} 
	else 
	{
		delPlugin = (PluginDel) dlsym(handle, "plugin_del");
		if ((error = dlerror()) != NULL)
		{
			fputs(error, stderr);
			dlclose(handle);
		} 
		else 
		{
			dprintf(DEBUG_NORMAL, "[CPlugins] try del...\n");			
					
			delPlugin();
			dlclose(handle);
			dprintf(DEBUG_NORMAL, "[CPlugins] del done...\n");
		}
	}
	
	// remove plugin
	std::string pluginPath;
	pluginPath += PLUGINDIR;
	pluginPath += "/";
	pluginPath += plugin_list[number].filename.c_str();
	
	CFileHelpers::getInstance()->removeDir(pluginPath.c_str());
	
	
	//erase from pluginlist
	plugin_list.erase(plugin_list.begin() + number);
}

bool CPlugins::hasPlugin(CPlugins::p_type_t type)
{
	for (std::vector<plugin>::iterator it = plugin_list.begin(); it!=plugin_list.end(); it++)
	{
		if (it->type == type && !it->hide)
			return true;
	}
	return false;
}

CPlugins::p_type_t CPlugins::getPluginType(int type)
{
	switch (type)
	{
		case PLUGIN_TYPE_DISABLED:
			return P_TYPE_DISABLED;
			break;
			
		case PLUGIN_TYPE_GAME:
			return P_TYPE_GAME;
			break;
			
		case PLUGIN_TYPE_TOOL:
			return P_TYPE_TOOL;
			break;
			
		case PLUGIN_TYPE_SCRIPT:
			return P_TYPE_SCRIPT;
			break;
			
		case PLUGIN_TYPE_NEUTRINO:
			return P_TYPE_NEUTRINO;
			break;
			
		default:
			return P_TYPE_DISABLED;
	}
}


