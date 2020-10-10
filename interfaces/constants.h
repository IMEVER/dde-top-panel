/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QtCore>

namespace Dock {

#define DOCK_PLUGIN_MIME    "dock/plugin"
#define DOCK_PLUGIN_API_VERSION    "1.2.2"

#define PLUGIN_BACKGROUND_MAX_SIZE 20
#define PLUGIN_BACKGROUND_MIN_SIZE 20

#define PLUGIN_ICON_MAX_SIZE 20

// 需求变更成插件图标始终保持20x20,但16x16的资源还在。所以暂时保留此宏
#define PLUGIN_ICON_MIN_SIZE 20

// 插件最小尺寸，图标采用深色
#define PLUGIN_MIN_ICON_NAME "-dark"

// dock最大尺寸
#define DOCK_MAX_SIZE 100

#define DEFAULT_HEIGHT 24

}

#endif // CONSTANTS_H
