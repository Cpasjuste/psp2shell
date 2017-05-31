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

int p2s_moduleInfo(SceUID uid);

int p2s_moduleList();

SceUID p2s_moduleLoad(char *modulePath);

int p2s_moduleStart(SceUID uid);

SceUID p2s_moduleLoadStart(char *modulePath);

int p2s_moduleStop(SceUID uid);

int p2s_moduleUnload(SceUID uid);

int p2s_moduleStopUnload(SceUID uid);

#endif //_MODULE_H
