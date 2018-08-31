#!/bin/sh

echo U:640x480p-57 > /sys/class/graphics/fb0/mode

export TSLIB_TSDEVICE=/dev/input/ts0
export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONFILE=/usr/etc/ts.conf
export TSLIB_PLUGINDIR=/usr/lib/ts
export TSLIB_FBDEVICE=/dev/fb0
export TSLIB_CONSOLEDEVICE=none

WORKDIR=/usr/local/avicon-31
. ${WORKDIR}/setup-utils//setup-vars

dd if=/dev/zero of=/dev/fb0
/usr/bin/ts_test > /dev/null 
/usr/local/avicon-31/test_avicon/test_avicon -platform linuxfb:virtual=-100x0x100x480:virtual=640x0x100x480:virtual=-100x480x840x100 -plugin Tslib:/dev/input/ts0 -plugin EvdevKeyboard -plugin EvdevMouse 

# hideCursor() {
#     echo -e '\033[?25l' > /dev/console
# }
# 
# showCursor() {
#     echo -e '\033[?25h' > /dev/console
# }
# 
# hideCursor


# WORKDIR=/usr/local/avicon-31
# . ${WORKDIR}/setup-utils//setup-vars
# 
# while true ; do
#     $DIALOG --title "$ts_menu" \
#             --begin 3 3 \
#             --clear \
#             --backtitle "$ts_backtitle" \
#             --cancel-label "$ts_exit" \
#             --menu "$ts_select_menu_item" 24 40 17 \
#              "1" "$ts_lcd_screen_test" \
#              "2" "$ts_temperature_sensor" \
#              "3" "$ts_touchscreen_calibrate" \
#              "4" "$ts_touchscreen_test" \
#              "5" "$ts_gps" \
#              "6" "$ts_gps_raw_data" \
#              "7" "$ts_power" \
#              "8" "$ts_sound" \
#              "9" "$ts_lcdbrightness" \
#             "10" "$ts_externalkeyboard" \
#             "11" "$ts_settings" \
#             "12" "$ts_fsck" \
#             "13" "$ts_reboot" \
#             "14" "$ts_power_off" \
#             "15" "$ts_command_line" \
#             "16" "$ts_avicon31" \
#             "17" "$ts_wifiDeviceDetect" \
#             2> $stderrTempfile
# 
#     retval=$?
#     choice=`cat $stderrTempfile`
# 
#     case $retval in
#         0)
#             case $choice in
#                 1)
#                     ${WORKDIR}/setup-utils/lcdtest.sh
#                     ;;
#                 2)
#                     retval=255
#                     while [ $retval -ne 0 ]
#                     do
#                         ${WORKDIR}/setup-utils/temperature > $stdoutTempfile 2> $stderrTempfile
#                         retval=$?
#                         if [ $retval -eq 0 ] ; then
#                             temperature=`cat $stdoutTempfile`
#                         fi
#                         errors=`cat $stderrTempfile`
#                         dialog --title $ts_temperature \
#                             --backtitle $ts_backtitle \
#                             --begin 11 3 --infobox "$ts_errors:\n$errors" 10 60 \
#                             --and-widget \
#                             --begin 3 3 --timeout 1 --msgbox "$temperature C" 6 30 \
#                             2>/dev/null
#                         retval=$?
#                     done
#                     ;;
#                 3)
#                     /usr/bin/ts_calibrate > /dev/null
#                     ;;
#                 4)
#                     /usr/bin/ts_test > /dev/null
#                     ;;
#                 5)
#                     rm /var/lock/LCK..ttymxc2 # crutch
#                     ${WORKDIR}/setup-utils/geoposition
#                     rm /var/lock/LCK..ttymxc2 # crutch
#                     ;;
#                 6)
#                     rm /var/lock/LCK..ttymxc2 # crutch
#                     /usr/bin/minicom -D /dev/ttymxc2 -8
#                     rm /var/lock/LCK..ttymxc2 # crutch
#                     ;;
#                 7)
#                     retval=255
#                     while [ $retval -ne 0 ]
#                     do
#                         ${WORKDIR}/setup-utils/battery > $stdoutTempfile 2> $stderrTempfile
#                         retval=$?
#                         if [ $retval -eq 0 ] ; then
#                             battery=`cat $stdoutTempfile`
#                         fi
#                         errors=`cat $stderrTempfile`
#                         dialog --title $ts_power \
#                             --backtitle $ts_backtitle \
#                             --begin 11 3 --infobox "$ts_errors:\n$errors" 10 60 \
#                             --and-widget \
#                             --begin 3 3 --timeout 1 --msgbox "$battery V" 6 30 \
#                             2>/dev/null
#                         retval=$?
#                     done
#                     ;;
#                 8)
#                     ${WORKDIR}/setup-utils/sound
#                     ;;
#                 9)
#                     ${WORKDIR}/setup-utils/lcdbrightness
#                     ;;
#                 10)
#                     ${WORKDIR}/setup-utils/externalkeyboard
#                     ;;
#                 11)
#                     ${WORKDIR}/setup-utils/settings.sh
#                     ;;
#                 12)
#                     /sbin/fsck -f -y /dev/mmcblk0p1
#                     ;;
#                 13)
#                     /sbin/reboot
#                     ;;
#                 14)
#                     /sbin/poweroff
#                     ;;
#                 15)
#                     showCursor
#                     /sbin/getty 115200 /dev/console
#                     hideCursor
#                     ;;
#                 16)
#                     ${WORKDIR}/avicon31 -platform linuxfb:virtual=-100x0x100x480:virtual=640x0x100x480:virtual=-100x480x840x100 -plugin Tslib:/dev/input/ts0 -plugin EvdevKeyboard -plugin EvdevMouse
#                     ;;
#                 17)
#                     ${WORKDIR}/setup-utils/wifiDeviceDetect.sh
#                     ;;
#             esac
#             ;;
#         1)
#             showCursor
#             exit 0
#             ;;
#         255)
#             showCursor
#             exit 0
#             ;;
#     esac
# done
