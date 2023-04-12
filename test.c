#define CANDRIVER_SOCKETCAN
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

void chkerr(int err, int f) {
    if (err) {
        printf("error! %d\n", f);
        exit(1);
    }
}

int read_event(int fd, struct js_event *event) {
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

static int16_t axes[16];
static pthread_t js_thread;

void js_reader(void* args) {
    const char* js_fname="/dev/input/js0";
    int js_fd;
    js_fd = open(js_fname, O_RDONLY);

    if (js_fd == -1)
        perror("Could not open joystick");

    struct js_event ev;
        
    while (read_event(js_fd, &ev) == 0) {
        switch (ev.type)
        {
            case JS_EVENT_BUTTON:
                printf("Button %u %s\n", ev.number, ev.value ? "pressed" : "released");
                break;
            case JS_EVENT_AXIS:
                axes[ev.number] = ev.value;
                break;
            default:
                /* Ignore init events. */
                break;
        }
        
        fflush(stdout);
    }
}
 
void int_handler() {
    pthread_cancel(js_thread);
    exit(0);
}

int main(int argc, char const *argv[]) {
    signal(SIGINT, int_handler); 

    CANConnection conn;
    uint8_t dvc = 2;
    int err = open_can_connection(&conn, "can0");
    chkerr(err, 0);


    err = voltage_enable(&conn, dvc);
    err=speed_set_ref(&conn, dvc, 3);
    //err = voltage_enable(&conn, 3);
    chkerr(err, 1); 

    assert(0==config_encoder_lines(&conn, dvc, 2048));
    assert(0==position_ref_encoder(&conn, dvc));

    uint16_t tmp;
    err = status_temperature(&conn, dvc, &tmp);
    chkerr(err, 3);
    printf("Device temperature: %f\n", fixed16_to_float(tmp));

    //assert(pthread_create(&js_thread, NULL, js_reader, NULL) == 0);

    while (1) {
        sys_heartbeat(&conn, dvc);
        //voltage_set(&conn, dvc, axes[4]);
        //voltage_set(&conn, 3, axes[1]);
        uint32_t tmp;
        assert(0==status_speed(&conn, dvc, &tmp));
        float tv = fixed32_to_float(tmp);
        printf("Speed: %f\r", tv);
    }

    close_can_connection(&conn);
    return 0;
}
