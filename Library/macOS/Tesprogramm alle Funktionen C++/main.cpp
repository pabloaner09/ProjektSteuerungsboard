#include "steuerboard_c_api.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>

using namespace std::chrono_literals;

static void print_help() {
    std::cout <<
R"(Befehle:
  help                   – diese Hilfe
  ls                     – alle bekannten Controls auflisten
  snap                   – Snapshot aller aktuellen Werte ausgeben
  val <NAME>             – Einzelwert lesen (z.B. Ri1_1)
  watch <PATTERN>        – Events abonnieren (z.B. "*", "Ri*")
  unwatch                – Event-Abo deaktivieren
  refresh                – Snapshot vom STM anfordern (GET_VALUES)
  modules                – aktive Modulliste (raw)
  module_ids             – Liste der Modul-IDs
  modules_info           – Module + Controls
  json                   – JSON-Snapshot der aktuellen Werte
  quit / exit            – Programm beenden
)" << std::flush;
}

// --- Callback für Events aus sb_subscribe ---
static void event_cb(const char* name, int value, void* user) {
    std::mutex* m = static_cast<std::mutex*>(user);
    std::lock_guard<std::mutex> lk(*m);
    std::cout << "[EVT] " << name << " -> " << value << std::endl;
}

int main(int argc, char** argv) {
    std::string device = (argc > 1) ? argv[1] : "/dev/tty.usbmodem6D71416A55751";

    SB_Handle* h = sb_new(device.c_str(), 115200);
    if (!h) {
        std::cerr << "Fehler beim Öffnen: " << sb_last_error() << "\n";
        return 1;
    }

    std::cout << "Verbunden mit: " << device << "\n";

    std::mutex outMtx;
    std::atomic<bool> running{true};

    // Events abonnieren (Default: alles)
    unsigned long long subId = sb_subscribe(h, "*", event_cb, &outMtx);

    print_help();

    std::string line;
    while (true) {
        {
            std::lock_guard<std::mutex> lk(outMtx);
            std::cout << "\n> " << std::flush;
        }
        if (!std::getline(std::cin, line)) break;

        auto trim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
        };
        trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd; iss >> cmd;

        if (cmd == "quit" || cmd == "exit") break;
        else if (cmd == "help") print_help();

        else if (cmd == "ls") {
            size_t count = 0;
            char** arr = sb_names(h, &count);
            if (!arr) {
                std::cout << "Fehler: " << sb_last_error() << "\n";
            } else if (count == 0) {
                std::cout << "(keine Controls)\n";
            } else {
                for (size_t i = 0; i < count; ++i) {
                    std::cout << arr[i] << "\n";
                }
            }
            sb_free_str_array(arr, count);
        }

        else if (cmd == "snap") {
            if (!sb_refresh(h, 500)) {
                std::cout << "Refresh fehlgeschlagen: " << sb_last_error() << "\n";
                continue;
            }
            size_t count = 0;
            sb_kv_t* vals = sb_values(h, "*", &count);
            if (!vals) {
                std::cout << "Fehler: " << sb_last_error() << "\n";
            } else {
                for (size_t i = 0; i < count; ++i) {
                    std::cout << vals[i].name << ": " << vals[i].value << "\n";
                }
            }
            sb_free_kv_array(vals, count);
        }

        else if (cmd == "val") {
            std::string name; iss >> name;
            if (name.empty()) {
                std::cout << "Nutzung: val <NAME>\n"; continue;
            }
            int v = sb_value(h, name.c_str());
            std::cout << name << " = " << v << "\n";
        }

        else if (cmd == "watch") {
            std::string pat; iss >> pat;
            if (pat.empty()) pat = "*";
            sb_unsubscribe(h, subId);
            subId = sb_subscribe(h, pat.c_str(), event_cb, &outMtx);
            std::cout << "Abonniert: " << pat << "\n";
        }

        else if (cmd == "unwatch") {
            sb_unsubscribe(h, subId);
            subId = 0;
            std::cout << "Events pausiert.\n";
        }

        else if (cmd == "refresh") {
            if (!sb_refresh(h, 500))
                std::cout << "Refresh fehlgeschlagen: " << sb_last_error() << "\n";
            else
                std::cout << "Snapshot aktualisiert.\n";
        }

        else if (cmd == "modules") {
            if (!sb_refresh_modules(h, 1500)) {
                std::cout << "GET_MODULES Timeout: " << sb_last_error() << "\n";
                continue;
            }
            size_t count = 0;
            char** arr = sb_module_list_raw(h, &count);
            if (!arr) {
                std::cout << "Fehler: " << sb_last_error() << "\n";
            } else {
                for (size_t i = 0; i < count; ++i) {
                    std::cout << arr[i] << "\n";
                }
            }
            sb_free_str_array(arr, count);
        }

        else if (cmd == "module_ids") {
            size_t count = 0;
            char** arr = sb_module_ids(h, &count);
            if (!arr) {
                std::cout << "Fehler: " << sb_last_error() << "\n";
            } else {
                for (size_t i = 0; i < count; ++i) {
                    std::cout << arr[i] << "\n";
                }
            }
            sb_free_str_array(arr, count);
        }

        else if (cmd == "modules_info") {
            size_t count = 0;
            sb_module_info_t* infos = sb_modules_info(h, &count);
            if (!infos) {
                std::cout << "Fehler: " << sb_last_error() << "\n";
            } else {
                for (size_t i = 0; i < count; ++i) {
                    std::cout << "Modul: " << infos[i].module_id << "\n";
                    for (size_t j = 0; j < infos[i].ncontrols; ++j) {
                        std::cout << "   " << infos[i].controls[j] << "\n";
                    }
                }
            }
            sb_free_modules_info(infos, count);
        }

        else if (cmd == "json") {
            char* js = sb_snapshot_json(h, true);
            if (!js) {
                std::cout << "Fehler: " << sb_last_error() << "\n";
            } else {
                std::cout << js << "\n";
            }
            sb_free_string(js);
        }

        else {
            std::cout << "Unbekannter Befehl: " << cmd << "\n";
            print_help();
        }
    }

    running = false;
    if (subId) sb_unsubscribe(h, subId);
    sb_delete(h);
    std::cout << "Tschüss.\n";
    return 0;
}
