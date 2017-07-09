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

#include "psp2shell_k.h"

SceUID p2s_get_running_app_pid() {

    SceUID pid = -1;
    SceUID ids[20];

    int count = sceAppMgrGetRunningAppIdListForShell(ids, 20);
    if (count > 0) {
        pid = sceAppMgrGetProcessIdByAppIdForShell(ids[0]);
    }

    return pid;
}

SceUID p2s_get_running_app_id() {

    SceUID ids[20];

    int count = sceAppMgrGetRunningAppIdListForShell(ids, 20);
    if (count > 0) {
        return ids[0];
    }

    return 0;
}

int p2s_get_running_app_name(char *name) {

    int ret = -1;

    SceUID pid = p2s_get_running_app_pid();
    if (pid > 0) {
        ret = sceAppMgrAppParamGetString(pid, 9, name, 256);
        return ret;
    }

    return ret;
}

int p2s_get_running_app_title_id(char *title_id) {

    int ret = -1;

    SceUID pid = p2s_get_running_app_pid();
    if (pid > 0) {
        ret = sceAppMgrAppParamGetString(pid, 12, title_id, 256);
        return ret;
    }

    return ret;
}

int p2s_launch_app_by_uri(const char *tid) {

    char uri[32];

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    snprintf(uri, 32, "psgm:play?titleid=%s", tid);

    for (int i = 0; i < 40; i++) {
        if (sceAppMgrLaunchAppByUri(0xFFFFF, uri) != 0) {
            break;
        }
        sceKernelDelayThread(10000);
    }

    return 0;
}

int p2s_reset_running_app() {

    char name[256];
    char id[16];
    char uri[32];

    if (p2s_get_running_app_title_id(id) != 0) {
        return -1;
    }

    if (p2s_get_running_app_name(name) != 0) {
        return -1;
    }

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    snprintf(uri, 32, "psgm:play?titleid=%s", id);

    for (int i = 0; i < 40; i++) {
        sceAppMgrLaunchAppByUri(0xFFFFF, uri);
        sceKernelDelayThread(10000);
    }

    return 0;
}
