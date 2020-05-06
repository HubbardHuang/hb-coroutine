function color_echo() {
    echo -e "\033[$1m\033[01m\033[05m$2\033[0m"
}

function blue_echo() {
    color_echo 36 "$1"
}

function green_echo() {
    color_echo 32 "$1"
}

function red_echo() {
    color_echo 31 "$1"
}

function yellow_echo() {
    color_echo 33 "$1"
}

# echo -e "\033[42;30m\033[05m 蓝底白字 \033[0m"