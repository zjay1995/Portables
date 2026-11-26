#include <Arduino.h>

#include <vector>
#include <algorithm>
#include <functional>
#define SSD1306_DISPLAYOFF          0xAE

#include "Globals.h"
#include "pins.h"
#include <Wire.h>

#include "../BuildConfig.h"   // FEATURE_* switches (sketch root)

class ButtonPressListener
{
public:

	virtual void onButtonPressUp(int pinNum, unsigned long pressDuration);
	virtual void onButtonPressDown(int pinNum);
	// Called on every tick while the button stays pressed (heldMillis since
	// press-down). Default: ignore.
	virtual void onButtonHeld(int pinNum, unsigned long heldMillis) {}
};

class ButtonPressDetector
{

protected:

	int m_pin;
	std::vector<ButtonPressListener*> m_listeners;

	static std::vector<ButtonPressDetector*> s_ButtonPressDetectors;

public:
	ButtonPressDetector(int pin) : m_pin(pin)
	{

	}

	virtual ~ButtonPressDetector()
	{
		for(auto l : m_listeners)
			delete l;
	}

	virtual void tick()=0;

	void addListener(ButtonPressListener* l) { m_listeners.push_back(l); }

	void notifyPressDownListeners()
	{
		for(auto l : m_listeners)
			l->onButtonPressDown(m_pin);
	}

	void notifyPressUpListeners(unsigned int pressDuration)
	{
		for(auto l : m_listeners)
			l->onButtonPressUp(m_pin, pressDuration);
	}

	void notifyHeldListeners(unsigned long heldMillis)
	{
		for(auto l : m_listeners)
			l->onButtonHeld(m_pin, heldMillis);
	}

	int getPinNum() const { return m_pin; }

	static void handleTick()
	{
		for(auto d: s_ButtonPressDetectors)
			d->tick();
	}

};


class DigitalReadSingleButtonPressDetector : public ButtonPressDetector
{

	int 	m_currentPinState;
	int		m_activePinState;

	unsigned long m_pressDownStartMillis = 0;
	bool          m_isDown = false;   // a press-down was seen and not yet released

public:

	DigitalReadSingleButtonPressDetector(int pinNum, int pressDurationMilis = 150) : ButtonPressDetector(pinNum)
	{
		m_activePinState = 0;
		pinMode(pinNum, INPUT_PULLUP);
		m_currentPinState = digitalRead(m_pin);

		s_ButtonPressDetectors.push_back(this);
	}

	~DigitalReadSingleButtonPressDetector()
	{
		remove_if(s_ButtonPressDetectors.begin(), s_ButtonPressDetectors.end(), [&, this](ButtonPressDetector* d){return this == d;} );
	}

	void tick()
	{
		int pinState = digitalRead(m_pin);
 		if(pinState != m_currentPinState)
		{
			if(pinState == m_activePinState)
			{
				m_pressDownStartMillis = millis();
				m_isDown = true;
				notifyPressDownListeners();
			}
			else
			{
				unsigned long elapsedMillis = millis() - m_pressDownStartMillis;
				m_isDown = false;
				notifyPressUpListeners(elapsedMillis);
				m_pressDownStartMillis = 0;
			}
			m_currentPinState = pinState;
		}
		else if(m_isDown && pinState == m_activePinState)
		{
			notifyHeldListeners(millis() - m_pressDownStartMillis);
		}
	}
};

class Keyboard : ButtonPressListener
{

	unsigned long m_buttonDownTotalMillis;
	unsigned long m_buttonRightTotalMillis;
	unsigned long m_buttonSTotalMillis;
    unsigned long m_buttonOnTotalMillis;

	ButtonPressDetector* m_buttonDownDetector;
	ButtonPressDetector* m_buttonSDetector;
	ButtonPressDetector* m_buttonRightDetector;
    ButtonPressDetector* m_buttonOnDetector;

    std::function<void()> m_onCalibrationComboPressed;
	std::function<void()> m_onDownPressed;
	std::function<void()> m_onSPressed;
	std::function<void()> m_onRightPressed;
    std::function<void()> m_onOnPressed;

#if FEATURE_HOLD_TO_SCROLL
	// Hold-to-scroll on DOWN (see FEATURE_HOLD_TO_SCROLL in BuildConfig.h)
	// Enum constants (not static const members): usable in ?: under C++11
	// without needing an out-of-class definition at link time.
	enum : unsigned long {
		kHoldFirstRepeatMs = HOLD_SCROLL_FIRST_MS,      // tunables in BuildConfig.h
		kHoldRepeatMs      = HOLD_SCROLL_STEP_MS,
		kHoldFastAfterMs   = HOLD_SCROLL_FAST_AFTER_MS,
		kHoldFastRepeatMs  = HOLD_SCROLL_FAST_STEP_MS
	};
	std::function<bool()> m_downRepeatAllowed;            // true on screens that may scroll
	int           m_downRepeats = 0;                      // repeats fired in this press
	unsigned long m_downNextRepeatAt = kHoldFirstRepeatMs; // held-time of next repeat
#endif

public:

	Keyboard()
	{
		m_buttonDownDetector = new DigitalReadSingleButtonPressDetector(c_BUTTON_DOWN_PIN);
		m_buttonSDetector = new DigitalReadSingleButtonPressDetector(c_BUTTON_S_PIN);
		m_buttonRightDetector = new DigitalReadSingleButtonPressDetector(c_BUTTON_RIGHT_PIN);
        m_buttonOnDetector = new DigitalReadSingleButtonPressDetector(c_BUTTON_ON_PIN);

        m_buttonDownDetector->addListener(this);
		m_buttonSDetector->addListener(this);
		m_buttonRightDetector->addListener(this);
        m_buttonOnDetector->addListener(this);

        m_buttonDownTotalMillis = 0;
		m_buttonRightTotalMillis = 0;
		m_buttonSTotalMillis = 0;
        m_buttonOnTotalMillis = 0;

    }

	~Keyboard()=default;

	// Is a press of this length a single click on DOWN / S / RIGHT?
	static bool isNavClick(unsigned long pressDuration)
	{
#if FEATURE_WIDE_CLICK_WINDOW
		// 30 ms rejects contact bounce; 800 ms stays well below the
		// 1750 ms ON-button combo and lets slow presses register.
		return pressDuration > 30 && pressDuration < 800;
#else
		return pressDuration > c_BUTTON_SINGLE_CLICK_HOLD_DURATION &&
		       pressDuration < c_BUTTON_SINGLE_CLICK_HOLD_DURATION + 200;
#endif
	}

	void addOnDownPressedFctor(const std::function<void()>& f) { m_onDownPressed = f; }
	void addOnSPressedFctor(const std::function<void()>& f) { m_onSPressed = f; }
	void addOnRightPressedFctor(const std::function<void()>& f) { m_onRightPressed = f; }
    void addOnOnPressedFctor(const std::function<void()>& f) { m_onOnPressed = f; }

#if FEATURE_HOLD_TO_SCROLL
	// Decides, at each repeat, whether the current screen allows hold-to-scroll.
	void addDownRepeatAllowedFctor(const std::function<bool()>& f) { m_downRepeatAllowed = f; }

	void onButtonHeld(int pinNum, unsigned long heldMillis)
	{
		if(m_buttonDownDetector->getPinNum() != pinNum) return;
		if(!m_onDownPressed || !m_downRepeatAllowed || !m_downRepeatAllowed()) return;
		if(heldMillis < m_downNextRepeatAt) return;

		m_onDownPressed();
		m_downRepeats++;
		// Schedule from the previous due time, not from "now", so a slow main
		// loop (display refresh) doesn't stretch every interval. If we fell
		// more than a step behind, resync instead of firing a burst.
		m_downNextRepeatAt +=
			(heldMillis >= kHoldFastAfterMs ? kHoldFastRepeatMs : kHoldRepeatMs);
		if(m_downNextRepeatAt < heldMillis)
			m_downNextRepeatAt = heldMillis;
	}
#endif

    void addOnCalibrationComboPressedFctor(const std::function<void()>& f) { m_onCalibrationComboPressed = f; }

	void onButtonPressUp(int pinNum, unsigned long pressDuration)
	{
		String arr;
		if(m_buttonDownTotalMillis != 0) arr += "1"; else arr += "0";
		if(m_buttonSTotalMillis != 0) arr += "1"; else arr += "0";
		if(m_buttonRightTotalMillis != 0) arr += "1"; else arr += "0";
        if(m_buttonOnTotalMillis != 0) arr += "1"; else arr += "0";

        Serial.println("Press duration for pinNum:" + String(pinNum) + " duration: " + String(pressDuration) + " " + arr);




		if(m_buttonDownDetector->getPinNum() == pinNum)
		{
			m_buttonDownTotalMillis = pressDuration;
#if FEATURE_HOLD_TO_SCROLL
			// The hold already stepped the value; releasing must not add one more.
			bool scrolled = (m_downRepeats > 0);
			m_downRepeats = 0;
			m_downNextRepeatAt = kHoldFirstRepeatMs;
			if(scrolled)
			{
				m_buttonDownTotalMillis = 0;
				return;
			}
#endif
			if(
				isNavClick(pressDuration) &&
				m_buttonRightTotalMillis == 0 &&
				m_buttonSTotalMillis == 0
			)
			{
				m_onDownPressed();
				m_buttonDownTotalMillis = 0;
			}
		}
		else if(m_buttonRightDetector->getPinNum() == pinNum)
		{
			m_buttonRightTotalMillis = pressDuration;
			if(
				isNavClick(pressDuration) &&
				m_buttonSTotalMillis == 0 &&
				m_buttonDownTotalMillis == 0
			)
			{
				m_onRightPressed();
				m_buttonRightTotalMillis = 0;
			}
		}
		else if(m_buttonSDetector->getPinNum() == pinNum)
		{
			m_buttonSTotalMillis = pressDuration;

			if(
				isNavClick(pressDuration) &&
				m_buttonRightTotalMillis == 0 &&
				m_buttonDownTotalMillis == 0
			)
			{
				m_onSPressed();
			}

			m_buttonSTotalMillis = 0;
			return;
		}
        else if(m_buttonOnDetector->getPinNum() == pinNum)
        {
            m_buttonOnTotalMillis = pressDuration;

            if(
                    pressDuration > c_BUTTON_SINGLE_CLICK_HOLD_DURATION &&
                    pressDuration < c_BUTTON_SINGLE_CLICK_HOLD_DURATION + 200
                    )
            {
                m_onOnPressed();
            }else if(m_buttonOnTotalMillis > c_BUTTON_COMBO_CLICK_HOLD_DURATION && m_buttonOnTotalMillis < c_BUTTON_COMBO_CLICK_HOLD_DURATION + 2000)
            {
                m_onCalibrationComboPressed();
                Serial.println("Calibration combo pressed");
            }

            m_buttonOnTotalMillis = 0;
            return;
        }




        Serial.println("D:" + String(m_buttonDownTotalMillis) + " S: " + String(m_buttonSTotalMillis) + " R: " + String(m_buttonRightTotalMillis)+ " O: " + String(m_buttonOnTotalMillis));

	}

	void onButtonPressDown(int pinNum)
	{
		Serial.println("Press down for pinNum:" + String(pinNum));
#if FEATURE_HOLD_TO_SCROLL
		if(m_buttonDownDetector->getPinNum() == pinNum)
		{
			m_downRepeats = 0;
			m_downNextRepeatAt = kHoldFirstRepeatMs;
		}
#endif
		if(m_buttonDownDetector->getPinNum() == pinNum)
			m_buttonDownTotalMillis = 0;
        m_buttonRightTotalMillis = 0;

        if(m_buttonRightDetector->getPinNum() == pinNum)
			m_buttonRightTotalMillis = 0;
		if(m_buttonSDetector->getPinNum() == pinNum)
			m_buttonSTotalMillis = 0;
        if(m_buttonOnDetector->getPinNum() == pinNum)
            m_buttonOnTotalMillis = 0;
        m_buttonRightTotalMillis = 0;
        m_buttonDownTotalMillis = 0;


    }

};