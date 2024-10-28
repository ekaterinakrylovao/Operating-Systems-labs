#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <fcntl.h>

class Daemon {
private:
    static Daemon* instance;
    std::string config_path;
    std::string directory;
    int interval;

    Daemon() : interval(60) {}

public:
    static Daemon* getInstance() {
        if (!instance) {
            instance = new Daemon();
        }
        return instance;
    }

    bool readConfig(const std::string& path) {
        config_path = path;
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            syslog(LOG_ERR, "Не удалось открыть конфигурационный файл");
            return false;
        }
        config_file >> directory >> interval;
        config_file.close();
        syslog(LOG_INFO, "Конфигурационный файл успешно загружен");
        return true;
    }

    void run() {
        while (true) {
            cleanDirectory();
            sleep(interval);
        }
    }

    void cleanDirectory() {
        syslog(LOG_INFO, "Очистка папки %s", directory.c_str());
        std::string command = "rm -f " + directory + "/*.tmp";
        system(command.c_str());
    }

    void reloadConfig() {
        syslog(LOG_INFO, "Перезагрузка конфигурационного файла");
        readConfig(config_path);
    }
};

// Инициализация статического экземпляра
Daemon* Daemon::instance = nullptr;

// Обработчики сигналов
void handleSignal(int signal) {
    Daemon* daemon = Daemon::getInstance();
    if (signal == SIGHUP) {
        daemon->reloadConfig();
    } else if (signal == SIGTERM) {
        syslog(LOG_INFO, "Демон завершает работу");
        closelog();
        exit(0);
    }
}

int main() {
    // Демонизация процесса
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    setsid();
    if (chdir("/") < 0) exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Запись pid в файл
    std::ofstream pid_file("/tmp/daemon.pid");
    if (pid_file.is_open()) {
        pid_file << getpid();
        pid_file.close();
    } else {
        exit(EXIT_FAILURE);
    }

    openlog("daemon_example", LOG_PID, LOG_DAEMON);

    Daemon* daemon = Daemon::getInstance();

    // Требует изменений под полный абсолютный путь к config.txt пользователя
    daemon->readConfig("/path/to/daemon_project/config.txt");
    // Мой путь
    // daemon->readConfig("/home/vboxuser/Downloads/projects/lab1/config.txt");

    // Установка обработчиков сигналов
    signal(SIGHUP, handleSignal);
    signal(SIGTERM, handleSignal);

    syslog(LOG_INFO, "Демон запущен");
    daemon->run();

    return 0;
}
