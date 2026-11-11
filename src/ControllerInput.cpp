#include <ControllerInput.h>

	#ifdef __linux__
	/// <summary>
	/// Überprüft, ob die ausgewählte ControllerID mit dem Arduino verbunden ist.
	/// </summary>
	/// <returns> Wenn der Controller Verbunden ist, wird true zurückgegeben. </returns>
	bool ControllerInput::isConnected()
	{
		int controller = open("/dev/input/controllerID", O_RDONLY); // Braucht die richtige ControllerID.
		if (controller < 0)
		{
			std::cerr << "Controller nicht gefunden\n";
			return false;
		}
		return true;
	}

	/// <summary>
	/// Erkennt den gedrückten Button über das Event und gibt den gedrückten Befehl aus.
	/// </summary>
	/// <param name="event"> Joystick Event, welches das Drücken von Buttons erkennt. </param>
	/// <returns> Einen ControllerButton Befehl, der in der Steuerung verwendet werden kann, um Befehle auszuführen. Z.B. der Return-Wert "Forward" zum initiieren einer Vorwärtsbewegung. </returns>
	bool ControllerInput::isMappedButton(unsigned char buttonNumber)
	{
		return buttonNumber == A_BUTTON || buttonNumber == B_BUTTON ||
			buttonNumber == X_BUTTON || buttonNumber == Y_BUTTON ||
			buttonNumber == LB_BUTTON || buttonNumber == RB_BUTTON;
	}

	Button ControllerInput::update(js_event event)
	{
		if ((event.type & JS_EVENT_BUTTON) == 0)
			return Button::None;

		if (event.value == 0)
        return Button::None;

		switch (event.number)
		{
		case Y_BUTTON:
			return Button::Forward;

		case B_BUTTON:
			return Button::RotateRight;

		case A_BUTTON:
			return Button::Backward;

		case X_BUTTON:
			return Button::RotateLeft;

		case RB_BUTTON:
			return Button::TrackingMode;

		case LB_BUTTON:
			return Button::DanceMode;

		default:
			return Button::None;
		}
	}
/// <summary>
/// Zum Testen welcher Button welche "ID" hat, damit darüber dann die richtigen Befehle zugewiesen werden können.
/// </summary>
/// <param name="event"> Joystick Event, welches das Drücken von Buttons erkennt. </param>
void ControllerInput::testButtonValues(js_event event)
{
	if ((event.type & JS_EVENT_BUTTON) == 0)
		return;

	std::cout
		<< "Type: " 
		<< event.type
		<< "Button "
		<< event.number
		<< " Wert "
		<< event.value
		<< std::endl;
}
	#else
	bool ControllerInput::isConnected()
	{
		std::cout << "Testfunktion für Controller-Connection funktioniert nur in Linux" << std::endl;
		return false;
	}

	Button ControllerInput::update()
	{
		return Button::None;
	}

	void ControllerInput::testButtonValues()
	{
		std::cout << "Testfunktion für Controller-Button-Werte funktioniert nur in Linux" << std::endl;
	}
	#endif
