#include "libjaguar.h"
#include <stdio.h>
#include <assert.h>

#ifdef CANDRIVER_WIN32
#include <WinBase.h>

int open_can_connection(CANConnection* conn, const char* serial_port) {

    conn->hSerial = CreateFile(serial_port,            //port name 
        GENERIC_READ | GENERIC_WRITE, //Read/Write   				 
        0,            // No Sharing                               
        NULL,         // No Security                              
        OPEN_EXISTING,// Open existing port only                     
        0,            // Non Overlapped I/O                           
        NULL);        // Null for Comm Devices

    if (conn->hSerial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return 1;
        }
        return 2;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(conn->hSerial, &dcbSerialParams)) {
        return 3;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(conn->hSerial, &dcbSerialParams)) {
        return 4;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(conn->hSerial, &timeouts)) {
        return 5;
    }

    PurgeComm(conn->hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);

    return 0;
}

int close_can_connection(CANConnection* conn) {
    CloseHandle(conn->hSerial);
    return 0;
}

int send_can_message(CANConnection* conn, CANMessage* message) {
    CANEncodedMsg encoded_message;

    encode_can_message(message, &encoded_message);

    // write(conn->serial_fd, encoded_message.data, encoded_message.size);
    // Sleep to allow message to send before proceding

    if (!WriteFile(conn->hSerial, encoded_message.data, encoded_message.size, NULL, NULL)) {
        return 1;
    }

    // Sleep(2);
    return 0;
}

int recieve_can_message(CANConnection* conn, CANMessage* message) {
    // int fd;
    int bytes_read;
    int extra_bytes;
    uint8_t read_byte;
    uint8_t size;
    uint8_t* data_ptr;
    CANEncodedMsg encoded_message;

    // fd = conn->serial_fd;

    // printf("Starting read\n");
    // discard bytes or wait until start of frame read
    // read(fd, &read_byte, 1);

    // while
    while (!ReadFile(conn->hSerial, &read_byte, 1, NULL, NULL)) {
        // return 1;
        Sleep(1);
    }
    // printf("read: %x\n", read_byte);


    while (read_byte != START_OF_FRAME) {
        if (!ReadFile(conn->hSerial, &read_byte, 1, NULL, NULL)) {
            return 1;
        }
        // printf("read: %x\n", read_byte);
    }

    bytes_read = 0;
    encoded_message.data[0] = START_OF_FRAME;

    if (!ReadFile(conn->hSerial, &size, 1, NULL, NULL)) {
        return 2;
    }
    encoded_message.data[1] = size;
    assert(size <= 8);
    // printf("Size: %d\n", size);

    // read bytes into encoded message buffer
    data_ptr = &(encoded_message.data[2]);
    for (bytes_read = 0; bytes_read < size; bytes_read++) {
        if (!ReadFile(conn->hSerial, data_ptr, 1, NULL, NULL)) {
            return 3;
        }
        // printf("read: %x\n", *data_ptr);
        if (*data_ptr == ENCODE_BYTE_A) {
            // read the second encoded byte
            data_ptr += 1;
            if (!ReadFile(conn->hSerial, data_ptr, 1, NULL, NULL)) {
                return 4;
            }
            // printf("read: %x\n", *data_ptr);
            extra_bytes += 1;
        }
        data_ptr += 1;
    }

    // FLUSH!!!

    encoded_message.size = 2 + size + extra_bytes;

    decode_can_message(&encoded_message, message);

    // Sleep(2);

    return 0;
}

#elif CANDRIVER_SERIAL

int open_can_connection(CANConnection* conn, const char* serial_port) {
    int fd;
    struct termios settings;
    conn->is_connected = false;
    conn->serial_port = serial_port;

    // open serial port
    // TODO: Fix nonblocking IO for more speeeed!
    fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        // could not open serial port
        return 1;
    }
    conn->serial_fd = fd;
    conn->is_connected = true;

    conn->saved_settings = malloc(sizeof(struct termios));

    // save existing serial settings
    tcgetattr(fd, conn->saved_settings);

    // initialize new settings with existing settings
    tcgetattr(fd, &settings);

    // set baud rate to 115200
    cfsetispeed(&settings, B115200);
    cfsetospeed(&settings, B115200);

    // set port to raw mode
    cfmakeraw(&settings);

    // apply settings
    tcsetattr(fd, TCSANOW, &settings);

    // flush buffers
    tcflush(fd, TCIOFLUSH);

    return 0;
}

int close_can_connection(CANConnection* conn) {
    int fd;
    conn->is_connected = false;

    fd = conn->serial_fd;

    // flush buffers
    tcflush(fd, TCIOFLUSH);

    // apply saved settings
    tcsetattr(fd, TCSANOW, conn->saved_settings);

    // free memory for saved settings
    free(conn->saved_settings);

    // close serial port
    close(fd);

    return 0;
}

int send_can_message(CANConnection* conn, CANMessage* message) {
    CANEncodedMsg encoded_message;

    encode_can_message(message, &encoded_message);
    write(conn->serial_fd, encoded_message.data, encoded_message.size);
    // Sleep to allow message to send before proceding
    usleep(1);

    return 0;
}

int recieve_can_message(CANConnection* conn, CANMessage* message) {
    int fd;
    int bytes_read;
    int extra_bytes;
    uint8_t read_byte;
    uint8_t size;
    uint8_t* data_ptr;
    CANEncodedMsg encoded_message;

    fd = conn->serial_fd;

    // printf("Starting read\n");
    // discard bytes or wait until start of frame read
    read(fd, &read_byte, 1);
    // printf("read: %x\n", read_byte);
    while (read_byte != START_OF_FRAME) {
        read(fd, &read_byte, 1);
        // printf("read: %x\n", read_byte);
    }

    bytes_read = 0;
    encoded_message.data[0] = START_OF_FRAME;

    read(fd, &size, 1);
    encoded_message.data[1] = size;
    assert(size <= 8);
    // printf("Size: %d\n", size);

    // read bytes into encoded message buffer
    data_ptr = &(encoded_message.data[2]);
    for (bytes_read = 0; bytes_read < size; bytes_read++) {
        read(fd, data_ptr, 1);
        // printf("read: %x\n", *data_ptr);
        if (*data_ptr == ENCODE_BYTE_A) {
            // read the second encoded byte
            data_ptr += 1;
            read(fd, data_ptr, 1);
            // printf("read: %x\n", *data_ptr);
            extra_bytes += 1;
        }
        data_ptr += 1;
    }

    encoded_message.size = 2 + size + extra_bytes;

    decode_can_message(&encoded_message, message);

    usleep(1);

    return 0;
}

#elif defined(CANDRIVER_SOCKETCAN)



int open_can_connection(CANConnection* conn, const char* serial_port) {
    struct sockaddr_can addr;
    struct ifreq ifr;

    conn->socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    assert(conn->socket_fd > 0);


    strcpy(ifr.ifr_name, "can0");
    ioctl(conn->socket_fd, SIOCGIFINDEX, &ifr);
    int bufsize = 128;
    setsockopt(conn->socket_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;


    int berr = bind(conn->socket_fd, (struct sockaddr*)&addr, sizeof(addr));
    assert(berr >= 0);

    conn->is_connected = true;
    return 0;
}

int close_can_connection(CANConnection* conn) {
    close(conn->socket_fd);
    return 0;
}

int send_can_message(CANConnection* conn, CANMessage* message) {
    struct can_frame frame;

    uint32_t canid = 0;
    canid |= message->device;
    canid |= message->api_index << 6;
    canid |= message->api_class << 10;
    canid |= message->manufacturer << 16;
    canid |= message->device_type << 24;
    // class, index = 5,3
    // canid = reverse_bytes(canid);

    frame.can_id = canid | CAN_EFF_FLAG;
    // printf("Canid: %x\n", frame.can_id);
    memcpy(frame.data, message->data, MAX_DATA_BYTES);

    frame.can_dlc = message->data_size;


    int nbytes = write(conn->socket_fd, &frame, sizeof(struct can_frame));

    return nbytes > 0;
}

int recieve_can_message(CANConnection* conn, CANMessage* message) {
    struct can_frame frame;

    // printf("srx\n");

    int nbytes = read(conn->socket_fd, &frame, sizeof(struct can_frame));

    if (nbytes < sizeof(struct can_frame)) {
        perror("can raw socket read");
        return 1;
    }
    // printf("Recv message! ID = %x\n", frame.can_id);

    uint32_t cid = frame.can_id;
    message->device = cid & 0x3F;
    cid >>= 6;
    message->api_index = cid & 0x0F;
    cid >>= 4;
    message->api_class = cid & 0x3F;
    cid >>= 6;
    message->manufacturer = cid & 0xFF;
    cid >>= 8;
    message->device_type = cid & 0x1F;

    memcpy(message->data, frame.data, frame.can_dlc);

    // printf("succ recv\n");
    return 0;
}

#endif


int init_sys_message(CANMessage* message, uint8_t api_index) {
    message->manufacturer = MANUFACTURER_SYS;
    message->device_type = DEVTYPE_SYS;
    message->api_class = API_SYS;
    message->api_index = api_index;
    return 0;
}

int init_jaguar_message(CANMessage* message, uint8_t api_class, uint8_t api_index) {
    message->manufacturer = MANUFACTURER_TI;
    message->device_type = DEVTYPE_MOTORCTRL;
    message->api_class = api_class;
    message->api_index = api_index;
    return 0;
}

// bool valid_sys_reply(CANMessage *message, CANMessage *reply)
// {
//     return reply->manufacturer == MANUFACTURER_SYS
//             && reply->device_type == DEVTYPE_SYS
//             && reply->api_class == API_SYS
//             && reply->api_index == message->api_index
//             && reply->device == message->device;
// }

bool valid_jaguar_reply(CANMessage* message, CANMessage* reply) {
    bool v = reply->manufacturer == MANUFACTURER_TI
        && reply->device_type == DEVTYPE_MOTORCTRL
        && reply->api_class == message->api_class
        && reply->api_index == message->api_index
        && reply->device == message->device;
    // printf("Jaguar reply valid? %d\n", v);
    return v;
}

bool valid_ack(CANMessage* message, CANMessage* ack) {
    return ack->manufacturer == MANUFACTURER_TI
        && ack->device_type == DEVTYPE_MOTORCTRL
        && ack->api_class == API_ACK
        && ack->device == message->device;
}

int sys_heartbeat(CANConnection* conn, uint8_t device) {
    CANMessage message;
    init_sys_message(&message, SYS_HEARTBEAT);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);

    return 0;
}

int sys_sync_update(CANConnection* conn, uint8_t mask) {
    CANMessage message;
    init_sys_message(&message, SYS_SYNC_UPDATE);
    message.device = 0;
    message.data_size = 1;
    message.data[0] = mask;
    send_can_message(conn, &message);

    return 0;
}

int sys_halt(CANConnection* conn, uint8_t device) {
    CANMessage message;
    init_sys_message(&message, SYS_HALT);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);

    return 0;
}

int sys_reset(CANConnection* conn, uint8_t device) {
    CANMessage message;
    init_sys_message(&message, SYS_RESET);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);

    return 0;
}

int sys_resume(CANConnection* conn, uint8_t device) {
    CANMessage message;
    init_sys_message(&message, SYS_RESUME);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);

    return 0;
}

int status_output_percent(CANConnection* conn, uint8_t device,
    int16_t* output_percent) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_OUTPUT_PERCENT);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *output_percent = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}

int status_current(CANConnection* conn, uint8_t device, uint16_t* current) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_CURRENT);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *current = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}

int status_bus_voltage(CANConnection* conn, uint8_t device, uint16_t* voltage) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_BUS_VOLTAGE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *voltage = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}

int status_temperature(CANConnection* conn, uint8_t device,
    uint16_t* temperature) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_TEMPERATURE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    // usleep(10);


/* WANTED:
can0  020214C3   [0]
can0  020214C3   [2]  E1 11
can0  02022003   [0]

GOT:
can0  020214C3   [0]
can0  020214C3   [2]  EC 10
can0  02022003   [0]
*/

    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);


    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *temperature = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}

int status_position(CANConnection* conn, uint8_t device, uint32_t* position) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_POSITION);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *position = reply.data[0] | reply.data[1] << 8 | reply.data[2] << 16
            | reply.data[3] << 24;
        return 0;
    } else {
        return 1;
    }
}

int status_speed(CANConnection* conn, uint8_t device, uint32_t* speed) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, 5);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);
    // recieve_can_message(conn, &ack);

    if ((valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack))) {
        *speed = reply.data[0] | reply.data[1] << 8 | reply.data[2] << 16
            | reply.data[3] << 24;
        return 0;
    } else {
        return 1;
    }
}

void print_can_message(CANMessage* msg) {
    printf("CAN Message: \n\tapi_class: %d\n\tapi_id: %d\n\tdvc_type: %d\n\tdvc_num: %d\n\tmfg: %d\n", msg->api_class, msg->api_index, msg->device_type, msg->device, msg->manufacturer);
    printf("Data (%d):\n", msg->data_size);
    for (int i = 0; i < msg->data_size; ++i)
        printf("\t%x\n", msg->data[i]);
}

int status_mode(CANConnection* conn, uint8_t device, uint8_t* mode) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_MODE);
    message.device = device;
    message.data_size = 0;
    // print_can_message(&message);

    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    // print_can_message(&reply);
    // print_can_message(&ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *mode = reply.data[0];
        return 0;
    } else {
        return 1;
    }
}

int status_limits(CANConnection* conn, uint8_t device, uint8_t* lims) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_STATUS, STATUS_MODE);
    message.device = device;
    message.data_size = 0;
    // print_can_message(&message);

    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    // print_can_message(&reply);
    // print_can_message(&ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *lims = reply.data[0];
        return 0;
    } else {
        return 1;
    }
}

int voltage_enable(CANConnection* conn, uint8_t device) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTAGE, VOLTAGE_ENABLE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);

    // printf("ven\n");
    recieve_can_message(conn, &ack);
    // print_can_message(&ack);


    return 0;
}

int voltage_disable(CANConnection* conn, uint8_t device) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTAGE, VOLTAGE_DISABLE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int voltage_set(CANConnection* conn, uint8_t device, int16_t voltage) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTAGE, VOLTAGE_SET);
    message.device = device;
    message.data_size = 2;
    message.data[0] = (uint8_t)(voltage & 0x00ff);
    message.data[1] = (uint8_t)(voltage >> 8);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    // return 0;
    return 1 - valid_ack(&message, &ack);
}

int voltage_set_sync(CANConnection* conn, uint8_t device, int16_t voltage,
    uint8_t group) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTAGE, VOLTAGE_SET);
    message.device = device;
    message.data_size = 3;
    message.data[0] = (uint8_t)(voltage & 0x00ff);
    message.data[1] = (uint8_t)(voltage >> 8);
    message.data[2] = group;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int voltage_get(CANConnection* conn, uint8_t device, int16_t* voltage) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTAGE, VOLTAGE_SET);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *voltage = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}

int voltage_ramp(CANConnection* conn, uint8_t device, uint16_t ramp) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTAGE, VOLTAGE_RAMP);
    message.device = device;
    message.data_size = 2;
    message.data[0] = (uint8_t)(ramp & 0x00ff);
    message.data[1] = (uint8_t)(ramp >> 8);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_enable(CANConnection* conn, uint8_t device, int32_t position) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_ENABLE);
    message.device = device;
    message.data_size = 4;
    message.data[0] = (uint8_t)(position & 0x000000ff);
    message.data[1] = (uint8_t)(position >> 8 & 0x000000ff);
    message.data[2] = (uint8_t)(position >> 16 & 0x000000ff);
    message.data[3] = (uint8_t)(position >> 24);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_disable(CANConnection* conn, uint8_t device) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_DISABLE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_set(CANConnection* conn, uint8_t device, int32_t position) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_SET);
    message.device = device;
    message.data_size = 4;
    message.data[0] = (uint8_t)(position & 0x000000ff);
    message.data[1] = (uint8_t)(position >> 8 & 0x000000ff);
    message.data[2] = (uint8_t)(position >> 16 & 0x000000ff);
    message.data[3] = (uint8_t)(position >> 24);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_set_sync(CANConnection* conn, uint8_t device, int32_t position,
    uint8_t group) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_SET);
    message.device = device;
    message.data_size = 5;
    message.data[0] = (uint8_t)(position & 0x000000ff);
    message.data[1] = (uint8_t)(position >> 8 & 0x000000ff);
    message.data[2] = (uint8_t)(position >> 16 & 0x000000ff);
    message.data[3] = (uint8_t)(position >> 24);
    message.data[4] = group;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_get(CANConnection* conn, uint8_t device, int32_t* position) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_SET);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    // printf("Ack: cls=%d, idx=%d\n", ack.api_class, ack.api_index);
    // printf("Rep: cls=%d, idx=%d\n", reply.api_class, reply.api_index);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *position = reply.data[0] | reply.data[1] << 8 | reply.data[2] << 16
            | reply.data[3] << 24;
        return 0;
    } else {
        return 1;
    }
}

int position_ref_encoder(CANConnection* conn, uint8_t device) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_REF);
    message.device = device;
    message.data_size = 1;
    message.data[0] = (uint8_t)POSITION_ENCODER;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_set_ref(CANConnection* conn, uint8_t device, uint8_t ref) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_REF);
    message.device = device;
    message.data_size = 1;
    message.data[0] = ref;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_p(CANConnection* conn, uint8_t device, int32_t p) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_P);
    message.device = device;
    message.data_size = 4;
    message.data[0] = (uint8_t)(p & 0x000000ff);
    message.data[1] = (uint8_t)(p >> 8 & 0x000000ff);
    message.data[2] = (uint8_t)(p >> 16 & 0x000000ff);
    message.data[3] = (uint8_t)(p >> 24);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_i(CANConnection* conn, uint8_t device, int32_t i) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_I);
    message.device = device;
    message.data_size = 4;
    message.data[0] = (uint8_t)(i & 0x000000ff);
    message.data[1] = (uint8_t)(i >> 8 & 0x000000ff);
    message.data[2] = (uint8_t)(i >> 16 & 0x000000ff);
    message.data[3] = (uint8_t)(i >> 24);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_d(CANConnection* conn, uint8_t device, int32_t d) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_POSITION, POSITION_D);
    message.device = device;
    message.data_size = 4;
    message.data[0] = (uint8_t)(d & 0x000000ff);
    message.data[1] = (uint8_t)(d >> 8 & 0x000000ff);
    message.data[2] = (uint8_t)(d >> 16 & 0x000000ff);
    message.data[3] = (uint8_t)(d >> 24);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int position_pid(CANConnection* conn, uint8_t device, int32_t p, int32_t i,
    int32_t d) {
    return position_p(conn, device, p) | position_i(conn, device, i) |
        position_d(conn, device, d);
}

int speed_set_ref(CANConnection* conn, uint8_t device, uint8_t ref) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_SPEED, SPEED_REF);
    message.device = device;
    message.data_size = 1;
    message.data[0] = ref;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    // return 0;
    return 1 - valid_ack(&message, &ack);
}


int config_encoder_lines(CANConnection* conn, uint8_t device, uint16_t lines) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_CONFIG, CONFIG_ENCODER_LINES);
    message.device = device;
    message.data_size = 2;
    message.data[0] = (uint8_t)(lines & 0x00ff);
    message.data[1] = (uint8_t)(lines >> 8);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int get_encoder_lines(CANConnection* conn, uint8_t device, uint16_t* lines) {
    CANMessage message;
    CANMessage reply;
    init_jaguar_message(&message, API_CONFIG, CONFIG_ENCODER_LINES);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);

    if (valid_jaguar_reply(&message, &reply)) {
        *lines = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}



int voltcomp_enable(CANConnection* conn, uint8_t device) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_ENABLE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);

    // printf("ven\n");
    recieve_can_message(conn, &ack);
    // print_can_message(&ack);


    return 0;
}

int voltcomp_disable(CANConnection* conn, uint8_t device) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_DISABLE);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int voltcomp_set(CANConnection* conn, uint8_t device, int16_t voltage) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_SET);
    message.device = device;
    message.data_size = 2;
    message.data[0] = (uint8_t)(voltage & 0x00ff);
    message.data[1] = (uint8_t)(voltage >> 8);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    // return 0;
    return 1 - valid_ack(&message, &ack);
}

// int voltcomp_set_sync(CANConnection* conn, uint8_t device, int16_t voltage,
//     uint8_t group) {
//     CANMessage message;
//     CANMessage ack;
//     init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_S);
//     message.device = device;
//     message.data_size = 3;
//     message.data[0] = (uint8_t)(voltage & 0x00ff);
//     message.data[1] = (uint8_t)(voltage >> 8);
//     message.data[2] = group;
//     send_can_message(conn, &message);
//     recieve_can_message(conn, &ack);

//     return 1 - valid_ack(&message, &ack);
// }

int voltcomp_get(CANConnection* conn, uint8_t device, int16_t* voltage) {
    CANMessage message;
    CANMessage reply;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_SET);
    message.device = device;
    message.data_size = 0;
    send_can_message(conn, &message);
    recieve_can_message(conn, &reply);
    recieve_can_message(conn, &ack);

    if (valid_jaguar_reply(&message, &reply) && valid_ack(&message, &ack)) {
        *voltage = reply.data[0] | reply.data[1] << 8;
        return 0;
    } else {
        return 1;
    }
}

int voltcomp_ramp(CANConnection* conn, uint8_t device, uint16_t ramp) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_RAMP);
    message.device = device;
    message.data_size = 2;
    message.data[0] = (uint8_t)(ramp & 0x00ff);
    message.data[1] = (uint8_t)(ramp >> 8);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}

int voltcomp_comp_ramp(CANConnection* conn, uint8_t device, uint16_t ramp) {
    CANMessage message;
    CANMessage ack;
    init_jaguar_message(&message, API_VOLTCOMP, VOLTCOMP_RATE);
    message.device = device;
    message.data_size = 2;
    message.data[0] = (uint8_t)(ramp & 0x00ff);
    message.data[1] = (uint8_t)(ramp >> 8);
    send_can_message(conn, &message);
    recieve_can_message(conn, &ack);

    return 1 - valid_ack(&message, &ack);
}