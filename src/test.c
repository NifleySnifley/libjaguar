#include <stdio.h>
#include "libjaguar.h"
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <windows.h>
#include <math.h>
#include <sys\timeb.h> 

bool running = true;

BOOL WINAPI consoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
        running = false;

    return TRUE;
}

uint16_t float2fixed16(float f) {
    uint8_t frac = fmodf(f, 1.0) * 256.0f;
    int8_t whole = ((int8_t)floor(f)) & 0xFF;
    // printf("frac: %d, whole: %d\n", frac, whole);
    return (((uint16_t)whole) << 8) | frac;
}

int main(int argc, char const* argv[]) {
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        printf("\nERROR: Could not set control handler");
        return 1;
    }

    if (argc < 2) {
        printf("Error, COM port not specified");
        return 1;
    }
    float cmd = 0.1;
    if (argc >= 3) {
        cmd = atof(argv[2]);
        printf("Cmd: %f\n", cmd);
    }

    CANConnection conn;
    uint8_t dvc = 1;
    int err = open_can_connection(&conn, argv[1]);
    assert(err == 0);

    printf("Successfully opened port\n");

    assert(0 == voltage_enable(&conn, dvc));
    assert(0 == config_encoder_lines(&conn, dvc, 2048));
    assert(0 == position_ref_encoder(&conn, dvc));
    assert(0 == speed_set_ref(&conn, dvc, SPEED_ENCODER_QUAD));
    voltage_ramp(&conn, dvc, float2fixed16(0.1));

    struct timeb lastt, curt;
    ftime(&lastt);
    while (running) {

        sys_heartbeat(&conn, dvc);

        assert(0 == voltage_set(&conn, dvc, percent_to_fixed16(cmd)));

        uint32_t tmp;
        assert(0 == status_speed(&conn, dvc, &tmp));
        // printf("Speed (RPS): %f\n", fixed32_to_float(tmp) / 60.0);
        ftime(&curt);
        int loopms = (int)(1000.0 * (curt.time - lastt.time)
            + (curt.millitm - lastt.millitm));
        printf("Ms: %d Speed (RPS):        \r", loopms);
        ftime(&lastt);
    }



    // err = speed_set_ref(&conn, dvc, 3);
    // //err = voltage_enable(&conn, 3);

    // uint16_t tmp;
    // err = status_temperature(&conn, dvc, &tmp);
    // // chkerr(err, 3);
    // printf("Device temperature: %f\n", fixed16_to_float(tmp));

    // //assert(pthread_create(&js_thread, NULL, js_reader, NULL) == 0);

    // while (1) {
    //     sys_heartbeat(&conn, dvc);

    //     uint32_t tmp;
    //     assert(0 == status_speed(&conn, dvc, &tmp));
    //     float tv = fixed32_to_float(tmp);
    //     printf("Speed: %f\r", tv);
    // }

    close_can_connection(&conn);
    return 0;
}
