<img width="633" height="496" alt="Снимок экрана 2026-11-09 163416" src="https://github.com/user-attachments/assets/8c16e490-4d23-4002-a0e9-fa4b977b49ad" />

# Сервисное меню БУИ Авикон-31

Приложение для тестирования электронных блоков Авикон-31 после сборки на заводе и первой загрузке.

## Проверяемые компоненты блока
- Тестирование и калибровка тачскрина и дисплея.
- Проверка периферийных компонентов: датчиков температуры, GPS, звука, камеры, Wi-Fi, Bluetooth, USB, COM, Ethernet, питания.

## Сборка проекта
- требуется закрытый основной проект `avicon`
- требуется нахождение на одном уровне проекта **avicon**
- используется утилита из проекта setup-utils(/avicon/setup-utils): **lcdtestfb**.
- на целевом устройстве исполняемый файл должен быть по пути: /usr/local/avicon-31/test_avicon/**test_avicon**. 
- следующие скрипты из каталога /scripts: 
[**test_avicon.sh**](https://bitbucket.org/avionika/cdu_test/downloads/test_avicon.sh), 
[**ts_calibration.sh**](https://bitbucket.org/avionika/cdu_test/downloads/ts_calibration.sh), 
[**ts_test.sh**](https://bitbucket.org/avionika/cdu_test/downloads/ts_test.sh) 
надо положить по пути: **/usr/local/avicon-31/test_avicon/**.
- для запуска сервисного меню необходимо измененный скрипт [**run**](https://bitbucket.org/avionika/cdu_test/downloads/run) положить по пути: /etc/rc.d/init.d/**run**.
