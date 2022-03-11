# output nothing for empty arrays upon -n
echo -n | ${JO:-jo} -n -a
