#include "WebServer.h"

#include <stdexcept>
#include <thread>
#include <unordered_map>

WebServer::~WebServer()
{
    server.stop();
    if(serverThread.joinable())
    {
        serverThread.join();
    }
}

/// <summary>
/// Initialisiert einen Webserver, der auf Anfragen über eine html-Seite reagiert und über die IP-Adresse des Raspberry Pi erreichbar ist.
/// </summary>
void WebServer::initializeServer() 
{
    server.Get("/", [](const httplib::Request&, httplib::Response& res) // Handler für die Startseite
    {
        res.set_content(loadHtmlFile(), "text/html");
    });

    server.Get("/style.css", [](const httplib::Request&, httplib::Response& res)
    {
        for(const char* path : {"src/Webserver/style.css", "Webserver/style.css"})
        {
            std::ifstream file(path);
            if(file)
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.set_content(buffer.str(), "text/css");
                return;
            }
        }

        res.status = 404;
        res.set_content("style.css nicht gefunden", "text/plain");
    });

    server.Get("/status", [this](const httplib::Request&, httplib::Response& res) // Handler für die Statusanzeige vom aktiven Modus
    {
        res.set_content(getModeString(getActiveMode()), "text/plain");
    });

    server.Post("/command", [this](const httplib::Request& req, httplib::Response& res) // Handler für die Verarbeitung von Befehlen
        {
            Button cmd = parseCommand(req.body);
            this->setPressedButton(cmd); // Setzt den gedrückten Button

            res.set_content("OK", "text/plain");
        });
    if(!server.bind_to_port("0.0.0.0", 8080))
    {
        throw std::runtime_error("Webserver konnte Port 8080 nicht binden");
    }

    serverThread = std::thread([this]() {
        server.listen_after_bind();
    });
}

/// <summary>
/// Lädt den Inhalt der web.html-Datei und gibt ihn als String zurück.
/// </summary>
std::string WebServer::loadHtmlFile()
{
    for(const char* path : {"src/Webserver/web.html", "Webserver/web.html"})
    {
        std::ifstream file(path);
        if(file)
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    return "<!doctype html><html><body><h1>web.html nicht gefunden</h1></body></html>";
}

/// <summary>
/// Parst den übergebenen Text vom Klicken eines Buttons auf der Website und gibt den entsprechenden Button zurück.
/// </summary> 
Button WebServer::parseCommand(const std::string& text)
{
    // Mapping von Text zu Button
    static const std::unordered_map<std::string, Button> lookup = {
        {"None", Button::None},
        {"Forward", Button::Forward},
        {"Backward", Button::Backward},
        {"RotateLeft", Button::RotateLeft},
        {"RotateRight", Button::RotateRight},
        {"DanceMode", Button::DanceMode},
        {"TrackingMode", Button::TrackingMode}
    };
    
    // Suche nach dem Button im Mapping
    auto it = lookup.find(text);
    if(it != lookup.end()) return it->second;

    return Button::None;
}

/// <summary>
/// Gibt den Modus als String zurück, basierend auf dem übergebenen Button.
/// </summary> 
std::string WebServer::getModeString(Button button)
{
    if (button == Button::DanceMode) return "Dance";
    if (button == Button::TrackingMode) return "Tracking";
    if (button == Button::Forward || button == Button::Backward || 
        button == Button::RotateLeft || button == Button::RotateRight) return "Manual";
    return "Idle";
}
