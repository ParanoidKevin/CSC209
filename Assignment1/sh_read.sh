result1=""
result2=""
while read from to
do
	result1="$result1$from"
	result2="$result2$to"	
done<$1
echo "$result1 $result2"
