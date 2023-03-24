#include "global.h"
#include "InputHandler_Reflex.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "MessageManager.h"
#include "Gamestate.h"
#include "PrefsManager.h"
#include "arch/ArchHooks/ArchHooks.h"

using namespace std;

REGISTER_INPUT_HANDLER_CLASS2(Reflex, Reflex);

InputHandler_Reflex*		REFLEX	= nullptr;		// globally accessable input device

/*
0,0,0,0,0,8,8,0,0,0,0,0,0,0,
0,0,0,0,8,2,2,8,0,0,0,0,0,0,
0,0,0,0,8,2,2,2,8,0,0,0,0,0,
0,0,0,0,0,8,2,2,2,8,0,0,0,0,
0,8,8,8,8,8,8,2,2,2,8,0,0,0,
8,2,2,2,2,2,2,2,2,2,2,8,0,0,
8,2,2,2,2,2,2,2,2,2,2,8,0,0,
0,8,8,8,8,8,8,2,2,2,8,0,0,0,
0,0,0,0,0,8,2,2,2,8,0,0,0,0,
0,0,0,0,8,2,2,2,8,0,0,0,0,0,
0,0,0,0,8,2,2,8,0,0,0,0,0,0,
0,0,0,0,0,8,8,0,0,0,0,0,0,0
*/

const int InputHandler_Reflex::ARROW_LUT[4][84] = {
	{
		8,8,
		8,1,1,8,
		8,1,1,1,8,0,
		8,1,1,1,8,0,0,0,
		8,1,1,1,8,8,8,8,8,8,
		8,1,1,1,1,1,1,1,1,1,1,8,
		8,1,1,1,1,1,1,1,1,1,1,8,
		8,1,1,1,8,8,8,8,8,8,
		8,1,1,1,8,0,0,0,
		8,1,1,1,8,0,
		8,1,1,8,
		8,8
	},
	{
		8,8,
		8,1,1,8,
		0,8,1,1,8,0,
		0,0,8,1,1,8,0,0,
		8,8,0,8,1,1,8,0,8,8,
		8,1,1,8,8,1,1,8,8,1,1,8,
		8,1,1,1,8,1,1,8,1,1,1,8,
		8,1,1,1,1,1,1,1,1,8,
		8,1,1,1,1,1,1,8,
		8,1,1,1,1,8,
		8,1,1,8,
		8,8
	},
	{
		8,8,
		8,1,1,8,
		8,1,1,1,1,8,
		8,1,1,1,1,1,1,8,
		8,1,1,1,1,1,1,1,1,8,
		8,1,1,1,8,1,1,8,1,1,1,8,
		8,1,1,8,8,1,1,8,8,1,1,8,
		8,8,0,8,1,1,8,0,8,8,
		0,0,8,1,1,8,0,0,
		0,8,1,1,8,0,
		8,1,1,8,
		8,8
	},
	{
		8,8,
		8,1,1,8,
		0,8,1,1,1,8,
		0,0,0,8,1,1,1,8,
		8,8,8,8,8,8,1,1,1,8,
		8,1,1,1,1,1,1,1,1,1,1,8,
		8,1,1,1,1,1,1,1,1,1,1,8,
		8,8,8,8,8,8,1,1,1,8,
		0,0,0,8,1,1,1,8,
		0,8,1,1,1,8,
		8,1,1,8,
		8,8
	}
};

const int InputHandler_Reflex::POSITION_LUT[84] = {
	 0, 21,  2,  1, 23, 22,  5,  4,  3, 26, 25, 24,
	 9,  8,  7,  6, 30, 29, 28, 27, 14, 13, 12, 11,
	10, 35, 34, 33, 32, 31, 20, 19, 18, 17, 16, 15,
	41, 40, 39, 38, 37, 36, 83, 82, 81, 80, 79, 78,
	62, 61, 60, 59, 58, 57, 77, 76, 75, 74, 73, 56,
	55, 54, 53, 52, 72, 71, 70, 69, 51, 50, 49, 48,
	68, 67, 66, 47, 46, 45, 65, 64, 44, 43, 63, 42
};

InputHandler_Reflex::InputHandler_Reflex()
{
	m_InputThread.resize(MAX_PLAYERS);
	
	sensor_data.resize(MAX_PLAYERS, std::vector<uint16_t>(16));
	sensor_baseline.resize(MAX_PLAYERS, std::vector<uint16_t>(16));
	
	panel_data.resize(MAX_PLAYERS, std::vector<uint16_t>(4));
	panel_baseline.resize(MAX_PLAYERS, std::vector<uint16_t>(4));
	panel_threshold.resize(MAX_PLAYERS, std::vector<uint16_t>(4));
	panel_cooldown.resize(MAX_PLAYERS, std::vector<uint16_t>(4));
	LastInputs.resize(MAX_PLAYERS, std::vector<bool>(4));
	IsPressed.resize(MAX_PLAYERS, std::vector<bool>(4));
	
	light_seg.resize(MAX_PLAYERS);
	light_frame.resize(MAX_PLAYERS);
	baseline_set.resize(MAX_PLAYERS,false);
	sensor_updates.resize(MAX_PLAYERS);
	
	handle.resize(MAX_PLAYERS);
	testHandle.resize(MAX_PLAYERS);
	
	m_bDeviceConnected.resize(MAX_PLAYERS, false);
	
	light_data.resize(MAX_PLAYERS, vector<vector<uint16_t>>(16, vector<uint16_t>(64)));

	m_bShutdown.resize(MAX_PLAYERS, false);
	m_fSongBeat = 0;
	
	LOG->Trace( "InputHandler_Reflex::InputHandler_Reflex()" );
	//StartThread();
	
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring( L, "REFLEX" );
		this->PushSelf( L );
		lua_settable( L, LUA_GLOBALSINDEX );
		LUA->Release( L );
	}

	ReflexIOMutex = new RageMutex("ReflexIOMutex");

}

void InputHandler_Reflex::Connect()
{
	for(int pn=0; pn<MAX_PLAYERS; pn++){
		if (m_bDeviceConnected[pn])
			ShutdownThread(pn);
	}
	
	baseline_set.clear();
	baseline_set.resize(MAX_PLAYERS,false);
	
	LOG->Trace("Reflex thread step 0 - hid_init()");

	uint16_t vendor_id = 0x0483;
	uint16_t product_id = 0x5750;

	hid_init();
	
	LOG->Trace("Reflex thread step 1 - hid_enumerate()");
	
	//OH BOY get the list of serial no.s
	struct hid_device_info *devs, *cur_dev;
	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	
	//vector<wchar_t*> serial_number_list;
	//vector<InputDevice> id;
	
	handle.clear();
	serial_number_list.clear();
	id.clear();
	
	LOG->Trace("Reflex thread step 2 - hid device list iterate");
	
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
			cur_dev->product_id == product_id) {
			
			serial_number_list.push_back(cur_dev->serial_number);
			
			//LOG->Trace("Device found, serial: %s", cur_dev->serial_number.c_str());
		}
		cur_dev = cur_dev->next;
	}
	
	LOG->Trace("Reflex thread step 3 - hid_open, set connected devices");

	for (int pn=0; pn<MAX_PLAYERS; pn++)
	{
		if( pn < serial_number_list.size())
		{
			//LOG->Trace("connecting device %s", serial_number_list[pn].c_str());
			handle.push_back(hid_open(vendor_id, product_id, serial_number_list[pn]));
			id.push_back(InputDevice(DEVICE_JOY1+pn));
			m_bDeviceConnected[pn] = true;
		}
	}
	
	LOG->Trace("We have %i handle(s)", handle.size());
	
	LOG->Trace("Reflex thread step 4 - start thread(s)");
	for (int pn=0; pn<handle.size(); pn++)
	{
		LOG->Trace("Reflex thread step 5.%i - start thread(%i)",pn,pn);
		if (m_bDeviceConnected[pn])
			StartThread(pn);
	}
}
void InputHandler_Reflex::Disconnect()
{
	for(int pn=0; pn<MAX_PLAYERS; pn++){
		if (m_bDeviceConnected[pn])
			ShutdownThread(pn);
	}
}
InputHandler_Reflex::~InputHandler_Reflex()
{
	for(int pn=0; pn<MAX_PLAYERS; pn++){
		if (m_bDeviceConnected[pn])
			ShutdownThread(pn);
	}
}

void InputHandler_Reflex::StartThread(int pn)
{
	//LOG->Trace("Start thread for reflex pad %i. Created: %i",pn,(int)m_InputThread[pn].IsCreated());
	if( m_InputThread[pn].IsCreated() )
		return;
	m_InputThread[pn].SetName( ssprintf("Reflex thread %i",pn) );
	LOG->Trace("Start thread for reflex pad %i",pn);
	if (pn==0) {
		m_InputThread[pn].Create( InputThread_Start0, this );
	}else if (pn==1) {
		m_InputThread[pn].Create( InputThread_Start1, this );
	}else if (pn==2) {
		m_InputThread[pn].Create( InputThread_Start2, this );
	}else if (pn==3) {
		m_InputThread[pn].Create( InputThread_Start3, this );
	}else{
		//nah
	}
}

void InputHandler_Reflex::ShutdownThread(int pn)
{
	if( !m_InputThread[pn].IsCreated() )
		return;

	m_bShutdown[pn] = true;
	LOG->Trace("Shutting down Reflex thread %i ...",pn);
	m_InputThread[pn].Wait();
	LOG->Trace("Reflex thread %i shut down.",pn);
	m_bShutdown[pn] = false;
}



int InputHandler_Reflex::InputThread_Start0(void *p)
{
	((InputHandler_Reflex *)p)->InputThread(0);
	return 0;
}
int InputHandler_Reflex::InputThread_Start1(void *p)
{
	((InputHandler_Reflex *)p)->InputThread(1);
	return 0;
}
int InputHandler_Reflex::InputThread_Start2(void *p)
{
	((InputHandler_Reflex *)p)->InputThread(2);
	return 0;
}
int InputHandler_Reflex::InputThread_Start3(void *p)
{
	((InputHandler_Reflex *)p)->InputThread(3);
	return 0;
}

void InputHandler_Reflex::SetBaselines(int pn)
{
	for(int i=0; i<4; i++)
	{
		panel_baseline[pn][i] = sensor_data[pn][i*4 + 0]+sensor_data[pn][i*4 + 1]+sensor_data[pn][i*4 + 2]+sensor_data[pn][i*4 + 3];
	}
	for(int i=0; i<16; i++)
	{
		sensor_baseline[pn][i] = sensor_data[pn][i];
	}
}

void InputHandler_Reflex::SetPanelThresholds(uint16_t l, uint16_t d, uint16_t u, uint16_t r, int pn)
{
	panel_threshold[pn][0] = l;
	panel_threshold[pn][1] = d;
	panel_threshold[pn][2] = u;
	panel_threshold[pn][3] = r;
	panel_cooldown[pn][0] = l/2;
	panel_cooldown[pn][1] = d/2;
	panel_cooldown[pn][2] = u/2;
	panel_cooldown[pn][3] = r/2;
}

void InputHandler_Reflex::GetData(int pn)
{
	//LOG->Trace("Reflex GetData step 0 - m_bDeviceConnected: %i",(int)m_bDeviceConnected[pn]);
	if (m_bDeviceConnected[pn])
	{
		uint8_t buf[65];
		
		//LOG->Trace("Reflex GetData step 1 - hid_read()");
		int read = hid_read(handle[pn], buf, 65);
		
		//LOG->Trace("Reflex GetData step 1.5 - clear panel_data");

		panel_data[pn][0] = 0;
		panel_data[pn][1] = 0;
		panel_data[pn][2] = 0;
		panel_data[pn][3] = 0;
		
		//LOG->Trace("Reflex GetData step 2 - update sensor data");

		for (int sensor = 0; sensor < 16; sensor++) {

			sensor_data[pn][sensor] = buf[sensor * 2] | (buf[sensor * 2 + 1] << 8);
			panel_data[pn][sensor/4] += buf[sensor * 2] | (buf[sensor * 2 + 1] << 8);

		}
		
		//LOG->Trace("Reflex GetData step 3 - set baselines");

		if (sensor_updates[pn] > 10 && !baseline_set[pn])
		{
			baseline_set[pn] = true;
			SetBaselines(pn);
		}
		
		//LOG->Trace("Reflex GetData step 4 - after setbaselines");

		if (!baseline_set[pn])
			sensor_updates[pn]++;
	}

	//printf( "\n" );
}

void InputHandler_Reflex::ReadOne()
{
	uint16_t USB_VID = 0x0483;
	uint16_t USB_PID = 0x5750;

	hid_init();
	testHandle[0] = hid_open(USB_VID, USB_PID, NULL);

	uint8_t buf[65];
    int read = hid_read(testHandle[0], buf, 65);
	printf( "read: %d \n", read );

	for(int i = 0; i < read; i++){
		if (i > 0 )
			if(i%4 == 0) printf("\n");
		else
			printf(":");
		printf("%02X", buf[i]);
	}

	printf( "\n" );

	hid_close(testHandle[0]);
}

void InputHandler_Reflex::SetLightData(int pn, int panel, int led, uint8_t r, uint8_t g, uint8_t b)
{

	if(led < 0 || led >= 84){
		return;
	}

	led = POSITION_LUT[led];

	int seg = panel*4 + led/21;
	int channel = led%21;

	light_data[pn][seg][channel*3+0] = g;
	light_data[pn][seg][channel*3+1] = r;
	light_data[pn][seg][channel*3+2] = b;

}

void InputHandler_Reflex::SetLightWholePanel(int pn, int panel, uint8_t r, uint8_t g, uint8_t b)
{
	int seg;
	int channel;

	for(int i = 0; i < 84; i++){
		seg = panel*4 + i/21;
		channel = i%21;
		light_data[pn][seg][channel*3+0] = g;
		light_data[pn][seg][channel*3+1] = r;
		light_data[pn][seg][channel*3+2] = b;
	}
}

void InputHandler_Reflex::SetLightArrow(int pn, int panel, uint8_t r, uint8_t g, uint8_t b)
{
	int seg;
	int channel;

	for(int i = 0; i < 84; i++){
		seg = panel*4 + POSITION_LUT[i]/21;
		channel = POSITION_LUT[i]%21;
		light_data[pn][seg][channel*3+0] = (g * ARROW_LUT[panel][i])/8;
		light_data[pn][seg][channel*3+1] = (r * ARROW_LUT[panel][i])/8;
		light_data[pn][seg][channel*3+2] = (b * ARROW_LUT[panel][i])/8;
	}
}

void InputHandler_Reflex::SendLights(int pn)
{
	if (m_bDeviceConnected[pn])
	{
		uint8_t lbuf[65];

		lbuf[0] = 0x0;
		lbuf[1] = (light_seg[pn] << 4 | light_frame[pn] );
		for(uint8_t i = 2; i < 65; i++){
			lbuf[i] = light_data[pn][light_seg[pn]][i-2];
		}
		hid_write(handle[pn], lbuf, 65);

		light_seg[pn]++;
		if( light_seg[pn] > 15 )
		{
			light_seg[pn] = 0;
			if( light_frame[pn] == 0 ){
				light_frame[pn] = 1;
			}else{
				light_frame[pn] = 0;
			}
		}
	}
}

void InputHandler_Reflex::TestLights(int pn)
{
	uint8_t lbuf[65];
    //buf[1-65] = your 64 bytes go here;

	lbuf[0] = 0x0;
	lbuf[1] = (light_seg[pn] << 4 | light_frame[pn] );
	for(uint8_t i = 2; i < 65; i++){
		lbuf[i] = 0xFF;
	}
	hid_write(handle[pn], lbuf, 65);

	light_seg[pn]++;
	if( light_seg[pn] > 15 )
	{
		light_seg[pn] = 0;
		if( light_frame[pn] == 0 ){
			light_frame[pn] = 1;
		}else{
			light_frame[pn] = 0;
		}
	}

}

void InputHandler_Reflex::InputThread(int pn)
{
	// HOOKS->BoostThreadPriority();
	
	if( true ){ //If Reflex Device Picked Up
		//if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
		//	LOG->Warn(werr_ssprintf(GetLastError(), "Failed to set DirectInput thread priority"));

		/* Enable priority boosting. */
		//SetThreadPriorityBoost( GetCurrentThread(), FALSE );
		

		RageTimer LastPoll;
		usleep(1000);
		while (!m_bShutdown[pn])
		{

			//LastPoll.Touch();
			RageTimer now;
			
			//LOG->Trace("Device %i connected: %i",pn,(int)m_bDeviceConnected[pn]);
			if (m_bDeviceConnected[pn]) {
				GetData(pn);
				SendLights(pn);
				
				for (int i = 0; i < 4; i++)
				{
					if (panel_baseline[pn][i] > 0 && panel_threshold[pn][i] > 0)
					{
						int pressure = panel_data[pn][i];
						if (!LastInputs[pn][i] && pressure > panel_baseline[pn][i] + panel_threshold[pn][i]){
							IsPressed[pn][i] = true;
						}else if (LastInputs[pn][i] && pressure < panel_baseline[pn][i] + panel_threshold[pn][i] - panel_cooldown[pn][i]){
							IsPressed[pn][i] = false;
						}
						if (IsPressed[pn][i] != LastInputs[pn][i]) {
							//m_fSongBeat = GAMESTATE->m_fSongBeat;
							// if (IsPressed[i]) {
								// LOG->Trace( "Button %i pressed", i );
							// } else {
								// LOG->Trace( "Button %i released", i );
							// }
							ButtonPressed(DeviceInput(id[pn], enum_add2(JOY_BUTTON_1, i), IsPressed[pn][i] ? 1.0 : 0.0, now));
						}
						LastInputs[pn][i] = IsPressed[pn][i];
					}
				}
			}
			
			//usleep(1000);

		};
		
		LOG->Trace("Reflex thread step 5 - hid_close()");
		
		if (m_bDeviceConnected[pn]) {
			hid_close(handle[pn]);
			m_bDeviceConnected[pn] = false;
		}
		
		InputHandler::UpdateTimer();
	}
	
	// HOOKS->UnBoostThreadPriority();
}

void InputHandler_Reflex::GetDevicesAndDescriptions( std::vector<InputDeviceInfo>& vDevicesOut )
{
	if( true ){
		vDevicesOut.push_back(InputDeviceInfo(DEVICE_REFLEX, "Reflex") );
	}
}

// lua start
#include "../../LuaBinding.h"

class LunaInputHandler_Reflex : public Luna<InputHandler_Reflex>
{
public:
	static int GetSensorBaseline( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->sensor_baseline[pn][IArg(1)]);
		return 1;
	}

	static int GetSensorValueAboveBaseline( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->sensor_data[pn][IArg(1)] - p->sensor_baseline[pn][IArg(1)]);
		return 1;
	}

	static int GetSensorValue( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->sensor_data[pn][IArg(1)]);
		return 1;
	}

	static int GetPanelBaseline( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->panel_baseline[pn][IArg(1)]);
		return 1;
	}

	static int GetPanelValueAboveBaseline( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->panel_data[pn][IArg(1)] - p->panel_baseline[pn][IArg(1)]);
		return 1;
	}

	static int GetPanelValue( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->panel_data[pn][IArg(1)]);
		return 1;
	}

	static int SetLightData( T* p, lua_State *L )
	{
		//REFLEX:SetLightData(pn,panel,led,r,g,b);
		p->SetLightData(IArg(1), IArg(2), IArg(3), IArg(4), IArg(5), IArg(6)); return 0;
		return 0;
	}

	static int SetLightWholePanel( T* p, lua_State *L )
	{
		p->SetLightWholePanel(IArg(1), IArg(2), IArg(3), IArg(4), IArg(5)); return 0;
		return 0;
	}

	static int SetLightArrow( T* p, lua_State *L )
	{
		p->SetLightArrow(IArg(1), IArg(2), IArg(3), IArg(4), IArg(5)); return 0;
		return 0;
	}

	static int SetPanelThresholds( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 5 && !lua_isnil(L,5) )
			pn = luaL_checkint( L, 5 );
		p->SetPanelThresholds(IArg(1), IArg(2), IArg(3), IArg(4), pn); return 0;
	}

	static int SetPanelCooldowns( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 5 && !lua_isnil(L,5) )
			pn = luaL_checkint( L, 5 );
		p->panel_cooldown[pn][0] = IArg(1);
		p->panel_cooldown[pn][1] = IArg(2);
		p->panel_cooldown[pn][2] = IArg(3);
		p->panel_cooldown[pn][3] = IArg(4);
		return 0;
	}

	static int GetPanelThreshold( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->panel_threshold[pn][IArg(1)]);
		return 1;
	}

	static int GetPanelCooldown( T* p, lua_State *L )
	{
		int pn = 0;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = luaL_checkint( L, 2 );
		lua_pushnumber(L, p->panel_cooldown[pn][IArg(1)]);
		return 1;
	}

	static int Test( T* p, lua_State *L )
	{
		p->ReadOne();
		return 0;
	}

	static int Connect( T* p, lua_State *L )
	{
		p->Connect();
		return 0;
	}
	static int Disconnect( T* p, lua_State *L )
	{
		p->Disconnect();
		return 0;
	}

	static int Reset( T* p, lua_State *L )
	{
		//REFLEX:Reset
		p->Connect();
		return 0;
	}

	static int GetTimeStamp( T* p, lua_State *L )
	{
		//HBT:GetTimeStamp()
		lua_pushnumber(L, p->m_fSongBeat);
		return 1;
	}

	LunaInputHandler_Reflex()
	{
		ADD_METHOD(GetSensorBaseline);
		ADD_METHOD(GetSensorValueAboveBaseline);
		ADD_METHOD(GetSensorValue);
		ADD_METHOD(GetPanelBaseline);
		ADD_METHOD(GetPanelValueAboveBaseline);
		ADD_METHOD(GetPanelValue);
		ADD_METHOD(SetLightData);
		ADD_METHOD(SetLightWholePanel);
		ADD_METHOD(SetLightArrow);

		ADD_METHOD(GetPanelThreshold);
		ADD_METHOD(SetPanelThresholds);
		ADD_METHOD(GetPanelCooldown);
		ADD_METHOD(SetPanelCooldowns);

		ADD_METHOD(Connect);
		ADD_METHOD(Disconnect);
		ADD_METHOD(Reset);

		ADD_METHOD(Test);
		
		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before
		// initializing the display.
		if( REFLEX )
		{
		  Lua *L = LUA->Get();
			lua_pushstring(L, "REFLEX");
			REFLEX->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( InputHandler_Reflex )
// lua end
