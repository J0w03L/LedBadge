#include "Commands.h"

#include "Serial.h"
#include "Eeprom.h"
#include "Display.h"
#include "Buttons.h"

#if defined(__AVR_ATmega88PA__)
#define F_CPU 12000000UL
#elif defined(__AVR_ATmega8A__)
#define F_CPU 8000000UL
#endif
#include <util/delay.h>

// Command/Animation state machine values
CommandState g_CommandReg = {};

typedef bool (*CommandHandler)(unsigned char header, FetchByte fetch);

bool PingCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char cookie = fetch(false);
	g_CommandReg.LastCookie = cookie;
	if(header & 0x08)
	{
		WriteSerialData((ResponseCodes::Ack << 4) | (1 << 3));
		WriteSerialData(cookie);
	}
	return true;
}

bool QuerySettingCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char setting = header & 0xF;
	if(setting >= Settings::Count)
	{
		return false;
	}
	
	WriteSerialData((ResponseCodes::Setting << 4) | setting);
	switch(setting)
	{
		case Settings::Brightness:
		{
			WriteSerialData(GetBrightness());
			break;
		}
		case Settings::HoldTimings:
		{
			unsigned char a, b, c;
			GetHoldTimings(&a, &b, &c);
			WriteSerialData(((a & 0xF) << 4) | (b & 0xF));
			WriteSerialData(((c & 0xF) << 4));
			break;
		}
		case Settings::IdleTimeout:
		{
			bool fade;
			unsigned char timeout;
			EndOfFadeAction::Enum endFadeAction;
			GetIdleTimeout(&fade, &endFadeAction, &timeout);
			WriteSerialData(timeout);
			WriteSerialData(((fade & 0x1) << 7) | ((endFadeAction & 0x3) << 5));
			break;
		}
		case Settings::FadeValue:
		{
			unsigned char counter;
			FadingAction::Enum action;
			GetFadeState(&counter, &action);
			WriteSerialData(counter);
			WriteSerialData((action & 0x3) << 6);
			break;
		}
		case Settings::AnimBookmarkPos:
		{
			WriteSerialData(g_CommandReg.AnimBookmark & 0xFF);
			WriteSerialData((g_CommandReg.AnimBookmark >> 8) & 0xFF);
			break;
		}
		case Settings::AnimPlayState:
		{
			WriteSerialData(g_CommandReg.AnimPlaying & 0xFF);
			WriteSerialData((((g_CommandReg.AnimPlaying >> 8) & 0x3F) << 2) | (g_CommandReg.AnimPlaying & 0x3));
			break;
		}
		case Settings::ButtonState:
		{
			WriteSerialData((CheckButton1() << 1) | CheckButton0());
			break;
		}
		case Settings::BufferFullness:
		{
			WriteSerialData(GetPendingSerialDataSize());
			break;
		}
		case Settings::Caps:
		{
			WriteSerialData(VERSION);
			WriteSerialData(BufferWidth);
			WriteSerialData((BufferHeight << 4) | 2 /* bit depth */);
			WriteSerialData(
			#if defined(__AVR_ATmega88PA__)
				SupportedFeatures::HardwareBrightness
			#else
				0
			#endif
			);
			break;
		}
	}
	return fetch(false) == 0; // discard dummy byte
}

bool UpdateSettingCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char setting = header & 0xF;
	if(setting >= Settings::Count)
	{
		return false;
	}

	switch(setting)
	{
		case Settings::Brightness:
		{
			SetBrightness(fetch(false));
			break;
		}
		case Settings::HoldTimings:
		{
			unsigned char a_b = fetch(true);
			unsigned char c_x = fetch(false);
			SetHoldTimings((a_b >> 4) & 0xf, a_b & 0xf, (c_x >> 4) & 0xf);
			break;
		}
		case Settings::IdleTimeout:
		{
			unsigned char timeout = fetch(true);
			unsigned char fade_endFadeAction_x = fetch(false);
			SetIdleTimeout((bool)((fade_endFadeAction_x >> 7) & 0x1), static_cast<EndOfFadeAction::Enum>((fade_endFadeAction_x >> 5) & 0x3), timeout);
			break;
		}
		case Settings::FadeValue:
		{
			unsigned char counter = fetch(true);
			FadingAction::Enum action = static_cast<FadingAction::Enum>((fetch(false) >> 6) & 0x3);
			SetFadeState(counter, action);
			break;
		}
		case Settings::AnimBookmarkPos:
		{
			g_CommandReg.AnimBookmark = fetch(true);
			g_CommandReg.AnimBookmark = (g_CommandReg.AnimBookmark << 8) | fetch(false);
			break;
		}
	}
	return true;
}

bool SwapCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char holdFrames = fetch(false);
	SwapBuffers();

	while(holdFrames--)
	{
		PumpAck();
		_delay_ms(16.6);
	}

	return true;
}

bool ReadRectCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char srcX_srcY = fetch(true);
	unsigned char width_height = fetch(false);
	unsigned char target = (header >> 2) & 0x3;
	PixelFormat::Enum format = static_cast<PixelFormat::Enum>(header & 0x3);
	WriteSerialData((ResponseCodes::Pixels << 4) | (format & 0x3));
	WriteSerialData(width_height);
	ReadRect((srcX_srcY >> 4) & 0xF, srcX_srcY & 0xF, (width_height >> 4) & 0xF, width_height & 0xF, format, 
		target == BufferTarget::BackBuffer ? g_DisplayReg.BackBuffer : g_DisplayReg.FrontBuffer);
	return true;
}

bool WriteRectCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char dstX_dstY = fetch(true);
	unsigned char width_height = fetch(true);
	unsigned char target = (header >> 2) & 0x3;
	PixelFormat::Enum format = static_cast<PixelFormat::Enum>(header & 0x3);
	Fill((dstX_dstY >> 4) & 0xF, dstX_dstY & 0xF, (width_height >> 4) & 0xF, width_height & 0xF, format, fetch,
		target == BufferTarget::BackBuffer ? g_DisplayReg.BackBuffer : g_DisplayReg.FrontBuffer);
	return true;
}

bool CopyRectCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char srcX_srcY = fetch(true);
	unsigned char dstX_dstY = fetch(true);
	unsigned char width_height = fetch(false);

	unsigned char *srcBuffer = ((header >> 2) & 0x3) == BufferTarget::BackBuffer ? g_DisplayReg.BackBuffer : g_DisplayReg.FrontBuffer;
	unsigned char *dstBuffer = (header & 0x3) == BufferTarget::BackBuffer ? g_DisplayReg.BackBuffer : g_DisplayReg.FrontBuffer;
	/*if(srcX_srcY == 0 && dstX_dstY == 0 && ((width_height >> 4) & 0xF) == BufferBitPlaneStride && (width_height & 0xF) == BufferHeight)
	{
		CopyWholeBuffer(srcBuffer, dstBuffer);
	}
	else*/
	{
		Copy((srcX_srcY >> 4) & 0xF, srcX_srcY & 0xF, 
			(dstX_dstY >> 4) & 0xF, dstX_dstY & 0xF, 
			(width_height >> 4) & 0xF, width_height & 0xF, srcBuffer, dstBuffer);
	}
	return true;
}

bool FillRectCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned char dstX_dstY = fetch(true);
	unsigned char width_height = fetch(true);
	Pix2x8 color = fetch(true);
	color = (color << 8) | fetch(false);

	unsigned char *buffer = ((header >> 2) & 0x3) == BufferTarget::BackBuffer ? g_DisplayReg.BackBuffer : g_DisplayReg.FrontBuffer;
	/*if(color == 0 && dstX_dstY == 0 && ((width_height >> 4) & 0xF) == BufferBitPlaneStride && (width_height & 0xF) == BufferHeight)
	{
		ClearBuffer(buffer);
	}
	else*/
	{
		SolidFill((dstX_dstY >> 4) & 0xF, dstX_dstY & 0xF, (width_height >> 4) & 0xF, width_height & 0xF, color, buffer);
	}
	return true;
}

bool ReadMemoryCommandHandler(unsigned char header, FetchByte fetch)
{
	unsigned int address = fetch(true);
	address = (address << 8) | fetch(false);

	unsigned char dwordCount = header & 0xF;

	WriteSerialData((ResponseCodes::Memory << 4) | dwordCount);
	WriteSerialData((address >> 8) & 0xFF);
	WriteSerialData(address & 0xFF);

	dwordCount += 1; // put in 1-16 range
	bool external = (address & RomTarget::TypeMask) != RomTarget::TypeInternal;
	if(external)
	{
		BeginReadExternalEEPROM(address & RomTarget::ExternalMask);
		while(dwordCount--)
		{
			WriteSerialData(ReadNextByteFromExternalEEPROM(true));
			WriteSerialData(ReadNextByteFromExternalEEPROM(true));
			WriteSerialData(ReadNextByteFromExternalEEPROM(true));
			WriteSerialData(ReadNextByteFromExternalEEPROM(dwordCount > 0));
		}
	}
	else
	{
		address &= RomTarget::InternalMask;
		while(dwordCount--)
		{
			WriteSerialData(ReadInternalEEPROM(address++));
			WriteSerialData(ReadInternalEEPROM(address++));
			WriteSerialData(ReadInternalEEPROM(address++));
			WriteSerialData(ReadInternalEEPROM(address++));
		}
	}
	return true;
}

bool WriteMemoryCommandHandler_SerialOnly(unsigned char header, FetchByte fetch)
{
	unsigned int address = fetch(true);
	address = (address << 8) | fetch(true);

	unsigned char dwordCount = header & 0xF;

	dwordCount += 1; // put in 1-16 range
	bool external = (address & RomTarget::TypeMask) != RomTarget::TypeInternal;
	if(external)
	{
		BeginWriteExternalEEPROM(address & RomTarget::ExternalMask);
		while(dwordCount--)
		{
			WriteNextByteToExternalEEPROM(fetch(true), true);
			WriteNextByteToExternalEEPROM(fetch(true), true);
			WriteNextByteToExternalEEPROM(fetch(true), true);
			WriteNextByteToExternalEEPROM(fetch(dwordCount > 0), dwordCount > 0);
		}
	}
	else
	{
		address &= RomTarget::InternalMask;
		while(dwordCount--)
		{
			WriteInternalEEPROM(address++, fetch(true));
			WriteInternalEEPROM(address++, fetch(true));
			WriteInternalEEPROM(address++, fetch(true));
			WriteInternalEEPROM(address++, fetch(dwordCount > 0));
		}
	}
	return true;
}

bool AnimControlCommandHandler(unsigned char header, FetchByte fetch)
{
	// TODO: 
}

unsigned char FetchSerial(bool moreBytes)
{
	return ReadSerialData();
}

unsigned char FetchInternalEEPROM(bool moreBytes)
{
	int readAddr = g_CommandReg.AnimReadPosition;
	g_CommandReg.AnimReadPosition = ((g_CommandReg.AnimReadPosition + 1) & (EepromInternalSize - 1)) | RomTarget::TypeInternal;
	return ReadInternalEEPROM(readAddr);
}

unsigned char FetchExternalEEPROM(bool moreBytes)
{
	g_CommandReg.AnimReadPosition = ((g_CommandReg.AnimReadPosition + 1) & (EepromExternalSize - 1)) | RomTarget::TypeExternal;
	return ReadNextByteFromExternalEEPROM(moreBytes);
}

// Helper for re-syncing after an invalid command is processed
// Similar to the buffer overflow re-sync, but this can be called from outside of an interrupt handler
void BadCommandPanic()
{
	// notify of panic state
	WriteSerialData((ResponseCodes::Error << 4) | ErrorCodes::BadSerialCommand);
	WriteSerialData(g_CommandReg.LastCookie);
}

void BadAnimPanic()
{
	// notify of panic state
	WriteSerialData((ResponseCodes::Error << 4) | ErrorCodes::BadAnimCommand);
	WriteSerialData(g_CommandReg.LastCookie);

	g_CommandReg.AnimPlaying = false;
}

static const CommandHandler s_SerialHandlers[SerialCommands::Count] = 
{
	PingCommandHandler,
	QuerySettingCommandHandler,
	UpdateSettingCommandHandler,
	SwapCommandHandler,
	ReadRectCommandHandler,
	WriteRectCommandHandler,
	CopyRectCommandHandler,
	FillRectCommandHandler,
	ReadMemoryCommandHandler,
	WriteMemoryCommandHandler_SerialOnly,
	AnimControlCommandHandler
};

void DispatchSerialCommand()
{
	unsigned char commandHeader = ReadSerialData();
	unsigned char command = (commandHeader >> 4) & 0xF;
	
	if((command >= SerialCommands::Count) || !s_SerialHandlers[command](commandHeader, FetchSerial))
	{
		BadCommandPanic();
	}
}

static const CommandHandler s_AnimHandlers[AnimCommands::Count] =
{
	PingCommandHandler,
	UpdateSettingCommandHandler,
	SwapCommandHandler,
	WriteRectCommandHandler,
	CopyRectCommandHandler,
	FillRectCommandHandler,
	AnimControlCommandHandler
};

void DispatchAnimCommand()
{
	bool external = (g_CommandReg.AnimReadPosition & RomTarget::TypeMask) != RomTarget::TypeInternal;

	if(external)
	{
		BeginReadExternalEEPROM(g_CommandReg.AnimReadPosition);
		unsigned char commandHeader = FetchExternalEEPROM(true);
		unsigned char command = (commandHeader >> 4) & 0xF;
		
		if((command >= SerialCommands::Count) || !s_SerialHandlers[command](commandHeader, FetchExternalEEPROM))
		{
			ReadNextByteFromExternalEEPROM(false); // discard junk byte and close connection, but don't call FetchExternalEEPROM/update the read pointer
			BadAnimPanic();
		}
	}
	else
	{
		unsigned char commandHeader = FetchInternalEEPROM(false);
		unsigned char command = (commandHeader >> 4) & 0xF;
		
		if((command >= SerialCommands::Count) || !s_SerialHandlers[command](commandHeader, FetchInternalEEPROM))
		{
			BadAnimPanic();
		}
	}
}

void InitAnim()
{
	ClearBuffer(g_DisplayReg.FrontBuffer);
	g_CommandReg.AnimReadPosition = 0;
	g_CommandReg.AnimPlaying = 0;

	static const unsigned char Magic[] = { '\0', 'H', '\0', 'i' };

	bool internal =
		ReadInternalEEPROM(0) == Magic[0] &&
		ReadInternalEEPROM(1) == Magic[1] &&
		ReadInternalEEPROM(2) == Magic[2] &&
		ReadInternalEEPROM(3) == Magic[3];
	if(internal)
	{
		g_CommandReg.AnimReadPosition = RomTarget::TypeInternal;
		g_CommandReg.AnimPlaying = 1;
	}
	else
	{
		BeginReadExternalEEPROM(0);
		bool external =
			ReadNextByteFromExternalEEPROM(true)  == Magic[0] &&
			ReadNextByteFromExternalEEPROM(true)  == Magic[1] &&
			ReadNextByteFromExternalEEPROM(true)  == Magic[2] &&
			ReadNextByteFromExternalEEPROM(false) == Magic[3];
		if(external)
		{
			g_CommandReg.AnimReadPosition = RomTarget::TypeExternal;
			g_CommandReg.AnimPlaying = 1;
		}
	}
}
