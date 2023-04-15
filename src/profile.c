#include <stdio.h>
#include "libjaguar.h"
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <linux/joystick.h>  
#include <pthread.h>
#include <signal.h>
#include <assert.h>

int main(int argc, char const* argv[]) {
    if (argc < 1 + 5) {
        printf("Error: arguments must be specified as canX, dvc#, outfile, gradation, pausems\n");
        exit(1);
    }

    CANConnection conn;
    uint8_t dvc = atoi(argv[2]);
    int err = open_can_connection(&conn, argv[1]);
    int grad = atoi(argv[4]);
    int pms = atoi(argv[5]);

    voltage_enable(&conn, dvc);
    speed_set_ref(&conn, dvc, 3);

    assert(0 == config_encoder_lines(&conn, dvc, 2048));
    assert(0 == position_ref_encoder(&conn, dvc));

    printf("CSV output is in following format:\noutput,speed,position");
    FILE* fcsv = fopen(argv[3], "w");

    for (int s = 0; s < (1 << 16); s += grad) {
        uint16_t throt = (1 << 15) - abs(s - (1 << 15));
        sys_heartbeat(&conn, dvc);
        voltage_set(&conn, dvc, throt);

        usleep(pms);

        uint32_t pos, spd;
        status_speed(&conn, dvc, &spd);
        status_position(&conn, dvc, &pos);
        float v = fixed32_to_float(spd);
        float p = fixed32_to_float(pos);
        printf("%d\n", throt);
        if (s <= (1 << 15))
            fprintf(fcsv, "%d,%f,%f\n", throt, v, p);
    }

    fflush(fcsv);
    fclose(fcsv);
    close_can_connection(&conn);
    return 0;
}
