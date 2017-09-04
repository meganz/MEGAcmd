
_megacmd()
{
	local cur opts
	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"

	if [[ ${cur} == '=' ]]; then
		cur=""
	fi
	
	COMP_WORDS[0]="${COMP_WORDS[0]/mega-/}"
	linetoexec=""
	lasta=""
	for a in "${COMP_WORDS[@]}"; do
		if [[ $a == *" "* ]] && [[ $a != "\""* ]] && [[ $a != "'"* ]]; then
			linetoexec=$linetoexec" \""$(echo $a | sed 's#\\$#\\ #g' | sed 's#\\\ # #g')"\""
		else
			if [[ ${a} == '=' ]] || [[ ${lasta} == '=' ]] || [[ ${a} == ':' ]] || [[ ${lasta} == ':' ]]; then
				linetoexec=$linetoexec$a
			else
#				linetoexec=$linetoexec" "$(echo $a | sed 's#\\$#\\ #g' | sed 's#\\\ # #g')
				linetoexec=$linetoexec" "$a
				if [[ $a == "\""* ]] && [[ $a != *"\"" ]];then
					linetoexec=$linetoexec"\""
				fi
				if [[ $a == "'"* ]] && [[ $a != *"'" ]];then
					linetoexec=$linetoexec"'"
				fi
			fi
		fi
		lasta=$a
	done

	if [[ "$linetoexec" == *" " ]]
	then
		linetoexec="$linetoexec\"\""
	fi
	
	COMPREPLY=""
	opts="$(mega-exec completion ${linetoexec/#mega-/} 2>/dev/null)"
	if [ $? -ne 0 ]; then
		COMPREPLY=""
		return $?
	fi

	declare -a "aOPTS=($opts)" || declare -a 'aOPTS=($opts)'

	for a in `seq 0 $(( ${#aOPTS[@]} -1 ))`; do
		COMPREPLY[$a]=$( echo ${aOPTS[$a]} | sed "s# #\\\ #g")
	done

	for i in "${COMPREPLY[@]}"; do
		if [[ ${i} == --*= ]] || [[ ${i} == */ ]]; then
			hash compopt 2>/dev/null >/dev/null && compopt -o nospace
		fi
	done
	
	if [[ $opts == "MEGACMD_USE_LOCAL_COMPLETION" ]]; then
		COMPREPLY=()
	fi
	
	if [[ $opts == "" ]]; then
		COMPREPLY=""
		compopt -o nospace
	fi
	
	return 0
}
for i in $(compgen -ca | grep mega-); do
	IFS=" " complete -o default -F _megacmd $i
done
