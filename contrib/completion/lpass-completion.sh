# This file is part of LockPassword
# Copyright (C) 2020-2021 Aleksandr D. Goncharov (Joursoir) <chat@joursoir.net>

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

lockpass_dir=~/.lock-password

__lpass_comp_file()
{
	local cur=$1 tmp
	local counter=0
	local path
	compopt -o filenames

	for path in $(compgen -f "$lockpass_dir/$cur" -- $cur)
	do
		tmp="${path#$lockpass_dir/}"
		[ -d "$path" ] && tmp+=/ && compopt -o nospace
		[ "${tmp:0:1}" = "." ] && continue

		COMPREPLY[counter++]=$tmp
	done
}

__lpass_main()
{
	local cur prev subcommands
	local i counter=1 command cmd_func
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	subcommands="
		init insert show edit generate
		rm mv help version
		"

	while [ $counter -lt $COMP_CWORD ]; do
		i="${COMP_WORDS[counter]}"
		case "$i" in
			-*) ;;
			*) command="$i"; break ;;
		esac
		(( counter++ ))
	done

	case "$command" in
		insert|show|edit|generate|rm|mv)
			__lpass_comp_file $cur
			;;
		"")
			COMPREPLY=( $(compgen -W "$subcommands" -- $cur) )
			;;
	esac
}

complete -F __lpass_main lpass
