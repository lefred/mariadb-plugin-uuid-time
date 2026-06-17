# Copyright (c) 2019, MariaDB corporation. All rights reserved.
# Copyright (c) 2026, lefred (Frédéric Descamps). All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1335  USA

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/plugin/type_uuid
)

MYSQL_ADD_PLUGIN(uuid_time
                 plugin.cc item_uuidtime.cc common.cc
                 ${CMAKE_SOURCE_DIR}/plugin/type_uuid/item_uuidfunc.cc
                 RECOMPILE_FOR_EMBEDDED
                 MODULE_ONLY COMPONENT Test)
