# coding=utf-8
#
# Original extra_scripts.py
# Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
#
# ldscripts, lwip patching, updated postmortem flags and git support
# Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from .checks import check_cppcheck, check_printsize
from .float_support import remove_float_support
from .ldscripts import ldscripts_inject_libpath
from .lwip import lwip_inject_patcher
from .postmortem import dummy_ets_printf
from .git import app_inject_revision
from .release import copy_release
from .flags import app_inject_flags

__all__ = [
    "check_cppcheck",
    "check_printsize",
    "remove_float_support",
    "ldscripts_inject_libpath",
    "lwip_inject_patcher",
    "dummy_ets_printf",
    "app_inject_revision",
    "app_inject_flags",
    "copy_release",
]
