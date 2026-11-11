#pragma once

#include <httplib.h>
#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <Button.h>

/// <summary>
/// Die Klasse WebServer stellt einen Webserver bereit, der auf Anfragen über eine html-Seite reagiert.
/// </summary>
class WebServer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct InputSnapshot
    {
        Button command;
        bool danceRequested;
    };

private:
    httplib::Server server;
    std::thread serverThread;
    Button pressedButton = Button::None;
    Button activeMode = Button::None;
    bool danceRequested = false;
    TimePoint lastCommandTime{};
    mutable std::mutex stateMutex;

public:
    ~WebServer();

    // Setzt den zuletzt gedrueckten Button und aktualisiert die Zeit des
    // letzten Befehls. Der optionale Zeitpunkt macht Timeout-Tests schnell.
    void setPressedButton(Button button, TimePoint now = Clock::now())
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        pressedButton = button;
        lastCommandTime = now;

        if(button == Button::DanceMode)
        {
            danceRequested = true;
        }
    }

    // Liest Level-Befehl und Tanz-Klick unter demselben Lock. Der Tanz-Klick
    // wird dabei genau einmal konsumiert und kann deshalb nicht zwischen zwei
    // getrennten Mainloop-Abfragen verloren gehen.
    InputSnapshot pollInput(TimePoint now = Clock::now())
    {
        std::lock_guard<std::mutex> lock(stateMutex);

        Button command = pressedButton;
        if(now - lastCommandTime > std::chrono::milliseconds(200))
        {
            command = Button::None;
        }

        InputSnapshot snapshot{command, danceRequested};
        danceRequested = false;
        return snapshot;
    }

    void setActiveMode(Button mode)
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        activeMode = mode;
    }

    Button getActiveMode() const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return activeMode;
    }

    void initializeServer(); // Initialisiert den Webserver und startet ihn auf der angegebenen IP-Adresse und Port 8080
    static std::string loadHtmlFile(); // Lädt den Inhalt der web.html-Datei und gibt ihn als String zurück
    Button parseCommand(const std::string& text); // Parst den übergebenen Text vom Klicken eines Buttons auf der Website und gibt den entsprechenden ControllerButton zurück
    static std::string getModeString(Button button); // Gibt den Modus als String zurück, basierend auf dem übergebenen Button
};
