#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdexcept>
#include <cstring>

class XDC {
private:
    int fd;

public:
    XDC(const char* port = "/dev/ttyAMA0") {
        fd = open(port, O_RDWR | O_NOCTTY);

        if (fd < 0) {
            throw std::runtime_error("Kan UART niet openen");
        }

        termios tty{};
        if (tcgetattr(fd, &tty) != 0) {
            throw std::runtime_error("Kan UART instellingen niet lezen");
        }

        cfsetispeed(&tty, B9600);
        cfsetospeed(&tty, B9600);

        tty.c_cflag &= ~PARENB;   // geen parity
        tty.c_cflag &= ~CSTOPB;   // 1 stopbit
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;       // 8 databits
        tty.c_cflag |= CLOCAL | CREAD;
        tty.c_cflag &= ~CRTSCTS;  // geen hardware flow control

        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_oflag &= ~OPOST;

        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 10;     // 1 sec timeout

        tcflush(fd, TCIOFLUSH);

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            throw std::runtime_error("Kan UART instellingen niet toepassen");
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    ~XDC() {
        if (fd >= 0) {
            close(fd);
        }
    }

    void sendData(const std::string& cmd) {
        std::string msg = cmd + "\r\n";
        write(fd, msg.c_str(), msg.size());
        tcdrain(fd);
    }

    std::string recvData(const std::string& cmd) {
        tcflush(fd, TCIFLUSH);

        sendData(cmd);

        std::string response;
        char c;

        while (true) {
            int n = read(fd, &c, 1);

            if (n > 0) {
                if (c == '\n') {
                    break;
                }

                if (c != '\r') {
                    response += c;
                }
            } else {
                break;
            }
        }

        return response;
    }

    int getStat() {
        sendData("STAT=?");

        std::string response;
        char c;

        while (true) {
            int n = read(fd, &c, 1);

            if (n > 0) {
                if (c == '\n') {
                    break;
                }

                if (c != '\r') {
                    response += c;
                }
            } else {
                break;
            }
        }

        if (response.rfind("STAT=", 0) == 0) {
            return std::stoi(response.substr(5));
        }

        return -1;
    }

    bool waitForBit8(int timeout_sec = 10) {
        auto start = std::chrono::steady_clock::now();

        while (true) {
            int stat = getStat();

            if (stat >= 0) {
                if (stat & (1 << 8)) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    return true;
                }
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(now - start);

            if (elapsed.count() >= timeout_sec) {
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

int main() {
    try {
        XDC motor("/dev/ttyAMA0");

        motor.sendData("INDX=1");

        if (motor.waitForBit8()) {
            std::cout << "Index found" << std::endl;
        } else {
            std::cout << "Timeout" << std::endl;
        }

        motor.sendData("SCAN=-1");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        motor.sendData("SCAN=1");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        motor.sendData("SSPD=1000");

        motor.sendData("SCAN=-1");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        motor.sendData("STOP=1");

        motor.sendData("SSPD=10000");

        motor.sendData("DPOS=0");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        motor.sendData("DPOS=10000");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        motor.sendData("DPOS=-10000");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        motor.sendData("DPOS=0");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        for (int i = 0; i < 10; i++) {
            motor.sendData("STEP=3200");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        motor.sendData("RSET=1");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (const std::exception& e) {
        std::cerr << "Fout: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}