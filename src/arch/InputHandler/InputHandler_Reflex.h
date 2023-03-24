#ifndef INPUTHANDLER_REFLEX_H
#define INPUTHANDLER_REFLEX_H
#include "InputHandler.h"
#include "RageThreads.h"

#include "hidapi.h"
#include <stdio.h>
//#include <cstdint>

#define NUM_BUTTONS 4
#define MAX_PLAYERS 4

using namespace std;

class RageMutex;

struct lua_State;

class InputHandler_Reflex : public InputHandler
{
public:
	InputHandler_Reflex();
	~InputHandler_Reflex();
	void GetDevicesAndDescriptions( std::vector<InputDeviceInfo>& vDevicesOut );

	// Lua
	void PushSelf( lua_State *L );

	void StartThread(int pn);
	void ShutdownThread(int pn);
	void Connect();
	void Disconnect();

	vector<bool> m_bReset;

	float m_fSongBeat;

	void ReadOne();
	void GetData(int pn);
	void TestLights(int pn);
	void SendLights(int pn);
	void SetLightData(int pn, int panel, int led, uint8_t r, uint8_t g, uint8_t b);
	void SetLightWholePanel(int pn, int panel, uint8_t r, uint8_t g, uint8_t b);
	void SetLightArrow(int pn, int panel, uint8_t r, uint8_t g, uint8_t b);

	vector<hid_device*> handle;
	vector<hid_device*> testHandle;

	vector<vector<uint16_t>> sensor_data;
	vector<vector<vector<uint16_t>>> light_data;

	vector<vector<uint16_t>> sensor_baseline;

	vector<vector<uint16_t>> panel_data; //faster access

	vector<vector<uint16_t>> panel_baseline;
	vector<vector<uint16_t>> panel_threshold;
	vector<vector<uint16_t>> panel_cooldown;

	vector<vector<bool>> LastInputs;
	vector<vector<bool>> IsPressed;

	vector<bool> baseline_set;
	vector<int> sensor_updates;
	
	vector<bool> m_bDeviceConnected;

	void SetBaselines(int pn);
	void SetPanelThresholds(uint16_t l, uint16_t d, uint16_t u, uint16_t r, int pn);

	vector<InputDevice> id;
	vector<wchar_t*> serial_number_list;

private:

	static const int POSITION_LUT[];
	static const int ARROW_LUT[][84];

	vector<uint8_t> light_seg;
	vector<uint8_t> light_frame;

	static int InputThread_Start0(void *p);
	static int InputThread_Start1(void *p);
	static int InputThread_Start2(void *p);
	static int InputThread_Start3(void *p);
	void InputThread(int pn);

	vector<RageThread> m_InputThread;
	vector<bool> m_bShutdown;

	RageMutex *ReflexIOMutex;

};

extern InputHandler_Reflex*	REFLEX;	// global and accessable from anywhere in our program

#endif
