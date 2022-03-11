# treat empty file as empty value
${JO:-jo} name=:/dev/null
${JO:-jo} name:=/dev/null
${JO:-jo} -n name=:/dev/null
${JO:-jo} -n name:=/dev/null
