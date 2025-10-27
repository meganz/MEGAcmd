
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
		if  [[ $a =~ ^.*([ \\]).*$ ]] && [[ $a != "\""* ]] && [[ $a != "'"* ]]; then
			lastcharina="${a: -1}"
			#add trailing space if ended in \ (it would have been strimmed)
			#and unescape symbols that dont need scaping between single quotes
			linetoexec=$linetoexec" '"$(echo $a | sed 's#\([^\\]\)\\$#\1\\ #g' | sed "s#\\\\\([ \<\>\|\`;:\"\!]\)#\1#g")"'"
		else
			if [[ ${a} == '=' ]] || [[ ${lasta} == '=' ]] || [[ ${a} == ':' ]] || [[ ${lasta} == ':' ]]; then
				linetoexec=$linetoexec$a
			else
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

	opts=$(echo "${opts/\`/\\\`}")
	opts=$(echo "${opts/\|/\\\|}")

	declare -a "aOPTS=(${opts})" || declare -a 'aOPTS=(${opts})'

	#escape characters that need to be scaped
	for a in `seq 0 $(( ${#aOPTS[@]} -1 ))`; do
		if [[ $lasta != "\""* ]] && [[ $lasta != "'"* ]]; then
			COMPREPLY[$a]=$( echo ${aOPTS[$a]} | sed "s#\([ \!;:\|\`\(\)\<\>\"\'\\]\)#\\\\\1#g") #OK
		else
			COMPREPLY[$a]="${aOPTS[$a]}"
		fi
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
for i in $(compgen -ca mega-); do
	IFS=" " complete -o default -F _megacmd $i
done
