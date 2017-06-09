/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MODULE_H
#define _MODULE_H

int p2s_moduleList();
int p2s_moduleListForPid(SceUID pid);

int p2s_moduleInfo(SceUID uid);
int p2s_moduleInfoForPid(SceUID pid, SceUID uid);

SceUID p2s_moduleLoadStart(char *modulePath);
SceUID p2s_moduleLoadStartForPid(SceUID pid, char *modulePath);

int p2s_moduleStopUnload(SceUID uid);
int p2s_moduleStopUnloadForPid(SceUID pid, SceUID uid);


SceUID p2s_kmoduleLoadStart(char *modulePath);
int p2s_kmoduleStopUnload(SceUID uid);

#endif //_MODULE_H
