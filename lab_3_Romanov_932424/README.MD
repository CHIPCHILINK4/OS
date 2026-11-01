Модуль ядра Linux и работа с /proc
Требуется подготовить модуль для загрузки в ядро Linux. Модуль в базовом виде должен:

    сопровождаться Makefile для сборки;
    оформляться в виде файла *.ko;
    загружаться командой insmod и выгружаться командой rmmod;
    при загрузке выводить в dmesg строку «Welcome to the Tomsk State University»;
    при выгрузке выводить в dmesg строку «Tomsk State University forever!»;
    должен создавать файл tsulab в каталоге /proc;
    созданный файл должен содержать информацию о каком-нибудь системном параметре или какое-нибудь вычислимое значение (например, количество секунд, оставшихся до начала сессии).

make,
sudo insmod tsulab.ko,
sudo dmesg | tail -n 20,
cat /proc/tsulab,
sudo rmmod tsulab,
sudo   dmesg | tail -n 20,
make clean.

