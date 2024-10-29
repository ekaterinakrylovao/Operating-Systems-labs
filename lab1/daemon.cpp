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

    Daemon() : interval(60) {}  // Интервал по умолчанию — 60 секунд

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
        // Удаление PID-файла при завершении
        unlink("/tmp/daemon.pid");
        closelog();
        exit(0);
    }
}

int main() {
    // Проверка существующего PID-файла
    std::ifstream pid_file_in("/tmp/daemon.pid");
    if (pid_file_in.is_open()) {
        pid_t existing_pid;
        pid_file_in >> existing_pid;
        pid_file_in.close();

        // Проверка, что процесс с таким PID работает
        if (kill(existing_pid, 0) == 0) {
            std::cerr << "Демон уже запущен. Завершается." << std::endl;
            return EXIT_FAILURE;
        } else {
            unlink("/tmp/daemon.pid");  // Если процесс не существует, удаляем PID-файл
        }
    }

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

    // Запись PID в файл
    std::ofstream pid_file_out("/tmp/daemon.pid");
    if (pid_file_out.is_open()) {
        pid_file_out << getpid();
        pid_file_out.close();
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
