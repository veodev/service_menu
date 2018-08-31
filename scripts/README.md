Для сборки требуется нахождение на одном уровне проекта **avicon**.

При работе программы используется утилита из проекта setup-utils(/avicon/setup-utils): **lcdtestfb**.

На целевом устройстве исполняемый файл должен быть по пути: /usr/local/avicon-31/test_avicon/**test_avicon**. 

Следующие скрипты: 
[**test_avicon.sh**](https://bitbucket.org/avionika/cdu_test/downloads/test_avicon.sh), 
[**ts_calibration.sh**](https://bitbucket.org/avionika/cdu_test/downloads/ts_calibration.sh), 
[**ts_test.sh**](https://bitbucket.org/avionika/cdu_test/downloads/ts_test.sh) 
надо положить по пути: **/usr/local/avicon-31/test_avicon/**.

Для запуска сервисного меню необходимо измененный скрипт [**run**](https://bitbucket.org/avionika/cdu_test/downloads/run) положить по пути: /etc/rc.d/init.d/**run**.