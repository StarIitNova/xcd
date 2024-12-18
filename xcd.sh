fline=""

script_full_path=$(dirname "$BASH_SOURCE")

output="$($script_full_path/xcd-a $@)"

# echo $output

while read line ; do
    if [[ ${line:0:7} == "~\$\$;cd;" ]]; then
        fline=${line:7}
    else
        echo $line
    fi
done <<< $output

if [[ $fline != "" ]]; then
    cd "$fline"
fi
