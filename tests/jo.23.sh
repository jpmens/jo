# disable creation of keys with empty, null, [] or {} values.

tmp_input=$(mktemp)

${JO:-jo} foo=
${JO:-jo} -n foo=

echo null > $tmp_input
${JO:-jo} foo=null
${JO:-jo} -n foo=null
${JO:-jo} -n foo=:$tmp_input
${JO:-jo} -n foo:=$tmp_input

echo "[]" > $tmp_input
${JO:-jo} foo=[]
${JO:-jo} -n foo=[]
${JO:-jo} -n foo=:$tmp_input
${JO:-jo} -n foo:=$tmp_input

echo "{}" > $tmp_input
${JO:-jo} foo={}
${JO:-jo} -n foo={}
${JO:-jo} -n foo=:$tmp_input
${JO:-jo} -n foo:=$tmp_input

echo '{"bar":null}' > $tmp_input
${JO:-jo} foo='{"bar":null}'
${JO:-jo} -n foo='{"bar":null}'
${JO:-jo} -n foo=:$tmp_input
${JO:-jo} -n foo:=$tmp_input

echo '{"bar":[]}' > $tmp_input
${JO:-jo} foo='{"bar":[]}'
${JO:-jo} -n foo='{"bar":[]}'
${JO:-jo} -n foo=:$tmp_input
${JO:-jo} -n foo:=$tmp_input

echo '{"bar":{}}' > $tmp_input
${JO:-jo} foo='{"bar":{}}'
${JO:-jo} -n foo='{"bar":{}}'
${JO:-jo} -n foo=:$tmp_input
${JO:-jo} -n foo:=$tmp_input

${JO:-jo} foo=1 bar= baz=3
${JO:-jo} -n foo=1 bar= baz=3
nothing=
${JO:-jo} list[]=1 list[]=$nothing
${JO:-jo} -n list[]=1 list[]=$nothing

rm -f $tmp_input
