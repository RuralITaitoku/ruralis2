set -x

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
else
	clear
	make all
fi
