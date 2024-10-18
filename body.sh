set -x
LIB_FILE_NAME=libruralis2.so
H_FILE_NAME=ruralis2.h


if [ "$1" = "run" ]; then
	sh stop.sh
	nohup ./ruralis2 > log.log 2>&1 &
	echo "kill $!" > stop.sh
	set +x
	echo ---------------------------------------
	tail -f log.log
elif [ "$1" = "clean" ]; then
	make clean
elif [ "$1" = "check" ]; then
    set +x
    echo ▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️ ruralisプロセスチェック ▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️
    set -x
    ps -ax | grep ruralis2
elif [ "$1" = "net" ]; then
    set +x
    echo ▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️ ネットワークチェック158 ▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️▪️
    set -x
    netstat -an | grep 1581
elif [ "$1" = "yuralib" ]; then
	mkdir -p ~/Library/yuralib/lib
	mkdir -p ~/Library/yuralib/include
	if [ "$2" = "push" ]; then
		cp $LIB_FILE_NAME ~/Library/yuralib/lib 	
		cp $H_FILE_NAME ~/Library/yuralib/include 	
	fi
else
	clear
	make all
fi
