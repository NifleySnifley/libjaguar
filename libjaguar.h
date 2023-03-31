#ifndef LIBJAGUAR_H
#define LIBJAGUAR_H

#include "can.h"
#include "canutil.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#ifdef CANDRIVER_SERIAL

typedef struct CANConnection {
    int serial_fd;
    bool is_connected;
    const char *serial_port;
    struct termios *saved_settings;
} CANConnection;

#elif CANDRIVER_SOCKETCAN

#endif

int open_can_connection(CANConnection *conn, const char *serial_port);
int close_can_connection(CANConnection *conn);

int send_can_message(CANConnection *conn, CANMessage *message);
int recieve_can_message(CANConnection *conn, CANMessage *message);

int init_sys_message(CANMessage *message, uint8_t api_index);
int init_jaguar_message(CANMessage *message, uint8_t api_class, uint8_t api_index);

bool valid_sys_reply(CANMessage *message, CANMessage *reply);
bool valid_jaguar_reply(CANMessage *message, CANMessage *reply);
bool valid_ack(CANMessage *message, CANMessage *ack);

int sys_heartbeat(CANConnection *conn, uint8_t device);
int sys_halt(CANConnection *conn, uint8_t device);
int sys_reset(CANConnection *conn, uint8_t device);
int sys_resume(CANConnection *conn, uint8_t device);
int sys_sync_update(CANConnection *conn, uint8_t group);

int voltage_enable(CANConnection *conn, uint8_t device);
int voltage_disable(CANConnection *conn, uint8_t device);
int voltage_set(CANConnection *conn, uint8_t device, int16_t voltage);
int voltage_set_sync(CANConnection *conn, uint8_t device, int16_t voltage, 
        uint8_t group);
int voltage_get(CANConnection *conn, uint8_t device, int16_t *voltage);
int voltage_ramp(CANConnection *conn, uint8_t device, uint16_t ramp);

int position_enable(CANConnection *conn, uint8_t device, 
        int32_t position);
int position_disable(CANConnection *conn, uint8_t device);
int position_set(CANConnection *conn, uint8_t device, 
        int32_t position);
int position_set_sync(CANConnection *conn, uint8_t device, int32_t position,
        uint8_t group);
int position_get(CANConnection *conn, uint8_t device, int32_t *position);
int position_p(CANConnection *conn, uint8_t device, int32_t p);
int position_i(CANConnection *conn, uint8_t device, int32_t i);
int position_d(CANConnection *conn, uint8_t device, int32_t d);
int position_pid(CANConnection *conn, uint8_t device, int32_t p, int32_t i, 
        int32_t d);
int position_ref_encoder(CANConnection *conn, uint8_t device);

int status_output_percent(CANConnection *conn, uint8_t device, 
        int16_t *output_percent);
int status_temperature(CANConnection *conn, uint8_t device, 
        uint16_t *temperature);
int status_position(CANConnection *conn, uint8_t device, uint32_t *position);
int status_mode(CANConnection *conn, uint8_t device, uint8_t *mode);

int config_encoder_lines(CANConnection *conn, uint8_t device, uint16_t lines);
int get_encoder_lines(CANConnection *conn, uint8_t device, uint16_t *lines);

#endif
