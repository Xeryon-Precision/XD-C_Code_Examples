#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

#define SERIAL_PORT "/dev/ttyAMA0"
#define BAUDRATE    B9600
#define BUF_SIZE    256

static int fd = -1;

/* ---------- Tijd-helper ---------- */
static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* ---------- Seriële poort openen en configureren ---------- */
int open_serial(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open serial port");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    cfsetispeed(&tty, BAUDRATE);
    cfsetospeed(&tty, BAUDRATE);

    tty.c_cflag &= ~PARENB;      // geen parity (PARITY_NONE)
    tty.c_cflag &= ~CSTOPB;      // 1 stopbit (STOPBITS_ONE)
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;          // 8 databits (EIGHTBITS)
    tty.c_cflag &= ~CRTSCTS;     // geen hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;      // raw input
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // geen software flow control
    tty.c_iflag &= ~(ICRNL | INLCR);

    tty.c_oflag &= ~OPOST;       // raw output

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;        // 1.0s timeout, zoals timeout=1 in pyserial

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return -1;
    }

    return fd;
}

/* ---------- Basisfuncties, equivalent aan de Python-versies ---------- */

/* Stuurt commando + \r\n, zonder te wachten op antwoord (sendData) */
void sendData(const char *cmd) {
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "%s\r\n", cmd);
    write(fd, buf, strlen(buf));
    tcdrain(fd);   // equivalent van ser.flush()
}

/* Leest één regel van de seriële poort (equivalent van ser.readline()),
   met een timeout in milliseconden. Retourneert lengte, of 0 bij timeout. */
int read_line(char *out, size_t out_size, int timeout_ms) {
    size_t idx = 0;
    long start = now_ms();

    while (now_ms() - start < timeout_ms && idx < out_size - 1) {
        char c;
        int n = read(fd, &c, 1);
        if (n > 0) {
            if (c == '\n') break;
            if (c != '\r') out[idx++] = c;
        }
    }
    out[idx] = '\0';
    return (int)idx;
}

/* Leegt de input-buffer, equivalent van ser.reset_input_buffer() */
void reset_input_buffer(void) {
    tcflush(fd, TCIFLUSH);
}

/* Stuurt commando en wacht op antwoord (recvData) */
void recvData(const char *cmd, char *response, size_t response_size) {
    reset_input_buffer();
    sendData(cmd);
    read_line(response, response_size, 1000); // timeout=1s zoals in Python
}

/* Vraagt STAT op en parsed het als integer, equivalent van get_stat() */
int get_stat(int *value_out) {
    char response[BUF_SIZE];

    sendData("STAT=?");
    read_line(response, sizeof(response), 1000);

    if (strncmp(response, "STAT=", 5) == 0) {
        *value_out = atoi(response + 5);
        return 1;  // gevonden
    }

    return 0;  // niet gevonden (equivalent van None)
}

/* Wacht tot bit 8 van STAT gezet is, equivalent van wait_for_bit8() */
int wait_for_bit8(int timeout_s) {
    long start = now_ms();
    long timeout_ms = (long)timeout_s * 1000;

    while (now_ms() - start < timeout_ms) {
        int stat;
        if (get_stat(&stat)) {
            if (stat & (1 << 8)) {
                usleep(2 * 1000000); // time.sleep(2)
                return 1;
            }
        }
        usleep(10000); // time.sleep(0.01)
    }

    return 0;
}

/* ---------- Main ---------- */

int main(void) {
    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        return 1;
    }

    usleep(2 * 1000000); // time.sleep(2), tijd geven aan de verbinding om te stabiliseren

    // Index
    sendData("INDX=1");
    if (wait_for_bit8(10)) {
        printf("Index found\n");
    } else {
        printf("Timeout\n");
    }

    // Scan full stroke
    sendData("SCAN=-1");
    usleep(2 * 1000000);
    sendData("SCAN=1");
    usleep(2 * 1000000);

    sendData("SSPD=1000");

    // Scan with timeout
    sendData("SCAN=-1");
    usleep(2 * 1000000);
    sendData("STOP=1");

    sendData("SSPD=10000");

    // DPOS
    sendData("DPOS=0");
    usleep(2 * 1000000);
    sendData("DPOS=10000");
    usleep(2 * 1000000);
    sendData("DPOS=-10000");
    usleep(2 * 1000000);
    sendData("DPOS=0");
    usleep(2 * 1000000);

    // Stepping
    for (int i = 0; i < 10; i++) {
        sendData("STEP=3200");
        usleep(500000); // time.sleep(0.5)
    }

    usleep(1000000); // time.sleep(1)
    sendData("RSET=1");
    usleep(1000000); // time.sleep(1)

    close(fd);
    return 0;
}