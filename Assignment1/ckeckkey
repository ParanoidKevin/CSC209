if test $# -ne 1
then
        echo usage: format checkkey
else
        echo Letters repeated in the from list:
        cat $1 | sed 's/ .*//' | sort | uniq -d
        echo Letters missing from the from list:
	sed 's/ .*//' $1 atoz | sort | uniq -u
        echo Letters repeated in the to list:
        cat $1 | sed 's/[\ ][\ ]*/ /g' | cut -d' ' -f2 | sort | uniq -d
fi

