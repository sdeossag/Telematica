#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void generate_sensor_data(char *buffer) {
    srand((unsigned)time(NULL));
    float temp = 20 + rand() % 10;
    float hum = 40 + rand() % 30;
    float pres = 1000 + rand() % 20;
    float co2 = 400 + rand() % 50;
    sprintf(buffer, "DATA TEMP=%.1f HUM=%.1f PRES=%.1f CO2=%.1f\r\n",
            temp, hum, pres, co2);
}
