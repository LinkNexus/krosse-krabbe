#ifdef __linux__
#include <unistd.h>
#include <linux/joystick.h>
#endif
#include <iostream>
#include <fcntl.h>
#include <Button.h>

/// <summary>
/// Besitzt Funktionen zum auslesen und Interpretieren der Inputs eines Controllers.
/// </summary>
class ControllerInput
{
public:
    static bool isConnected(); // Überprüft, ob die ausgewählte ControllerID mit dem Raspberry Pi verbunden ist.

    #ifdef __linux__
    static bool isMappedButton(unsigned char buttonNumber);
    static Button update(js_event event); // Erkennt den gedrückten Button über das Event und gibt den gedrückten Befehl aus.
    static void testButtonValues(js_event event); // Zum Testen welcher Button welche "ID" hat, damit darüber dann die richtigen Befehle zugewiesen werden können.
    #else
    static Button update();
    static void testButtonValues();  
    #endif

private:
    static constexpr unsigned char A_BUTTON = 0;
    static constexpr unsigned char B_BUTTON = 1;
    static constexpr unsigned char X_BUTTON = 3;
    static constexpr unsigned char Y_BUTTON = 4;
    static constexpr unsigned char LB_BUTTON = 6;
    static constexpr unsigned char RB_BUTTON = 7;
};
