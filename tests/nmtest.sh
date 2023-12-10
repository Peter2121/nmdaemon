#!/usr/local/bin/zsh

JQ="/usr/local/bin/jq"
if [ ! -e "$JQ" ]; then
	echo "JSON processing tool is not installed"
	echo "It can be installed from port or package 'jq'"
	exit 1
fi

BUFSIZE=8192
SOCKET=/var/run/nmd.socket
TIMEOUT=2

for test in *.json;
	do
		export JQ_COLORS="1;30:0;39:0;39:0;39:0;37:1;39:1;39"
		cat $test | $JQ .CMD;
		export JQ_COLORS="1;30:0;39:0;39:0;39:0;36:1;39:1;39"
		cat $test | nc -w $TIMEOUT -O $BUFSIZE -I $BUFSIZE -U $SOCKET | $JQ .RESULT;
		export JQ_COLORS="1;30:0;39:0;39:0;39:0;32:1;39:1;39"
	done
