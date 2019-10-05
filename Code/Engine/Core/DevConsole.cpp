//------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystems.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Rgba.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <algorithm>
#include "Engine/Core/MemTracking.hpp"
#include "Engine/Allocators/TemplatedUntrackedAllocator.hpp"

DevConsole* g_devConsole = nullptr;

//------------------------------------------------------------------------------------------------------------------------------
const STATIC Rgba DevConsole::CONSOLE_INFO			=	Rgba(1.0f, 1.0f, 0.0f, 1.0f);
const STATIC Rgba DevConsole::CONSOLE_BG_COLOR		=	Rgba(0.0f, 0.0f, 0.0f, 0.75f);
const STATIC Rgba DevConsole::CONSOLE_ERROR   		=	Rgba(1.0f, 0.0f, 0.0f, 1.0f);
const STATIC Rgba DevConsole::CONSOLE_ERROR_DESC	=	Rgba(1.0f, 0.5f, 0.3f, 1.0f);
const STATIC Rgba DevConsole::CONSOLE_INPUT			=	Rgba(1.0f, 1.0f, 1.0f, 1.0f);

extern std::mutex gTrackerLock;
extern std::map<void*, MemTrackInfo_T, std::less<void*>, TemplatedUntrackedAllocator<std::pair<void* const, MemTrackInfo_T>>> gMemTrackers;

//------------------------------------------------------------------------------------------------------------------------------
void LogHookForDevConsole(const LogObject_T* logObj)
{
	char filterString[1024];
	char lineString[1024];

	strcpy_s(filterString, 1024, logObj->filter);
	strcpy_s(lineString, 1024, logObj->line);

	g_devConsole->PrintString(Rgba::WHITE, filterString);
	g_devConsole->PrintString(Rgba::WHITE, lineString);
}

//------------------------------------------------------------------------------------------------------------------------------
bool DevConsole::ExecuteCommandLine( const std::string& commandLine )
{
	//Split the string to sensible key value pairs
	std::vector<std::string> splitStrings = SplitStringOnDelimiter(commandLine, ' ');
	
	if(splitStrings.size() == 0)
	{
		return false;
	}
	else
	{
		g_devConsole->PrintString(CONSOLE_INFO, "Data Received:");
		std::string printS = "> Exec ";
		for(int i =0; i < static_cast<int>(splitStrings.size()); i++)
		{
			printS += " " + splitStrings[i];
		}
		g_devConsole->PrintString(CONSOLE_INFO, printS);

		EventArgs args;

		for(int stringIndex = 1; stringIndex < static_cast<int>(splitStrings.size()); stringIndex++)
		{
			//split on =
			std::vector<std::string> KeyValSplit = SplitStringOnDelimiter(splitStrings[stringIndex], '=');
			if(KeyValSplit.size() != 2)
			{
				g_devConsole->PrintString(CONSOLE_ERROR ," ! The number of arguments read are not valid");
				g_devConsole->PrintString(CONSOLE_ERROR_DESC, "    Execute requires 2 arguments. A key and value pair split by =");
			}
			else
			{
				//Print the data we read
				printS = " Action: " + KeyValSplit[0] + " = " + KeyValSplit[1];
				g_devConsole->PrintString(CONSOLE_ECHO_COLOR, printS);

				args.SetValue(KeyValSplit[0], KeyValSplit[1]);
			}
		}

		bool result = g_eventSystem->FireEvent(splitStrings[0], args);
		return result;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::HandleKeyUp( unsigned char vkKeyCode )
{
	UNUSED(vkKeyCode);
	GUARANTEE_RECOVERABLE(true, "Nothing to handle");
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::ShowLastCommand()
{
	if(m_lastCommandIndex == 0)
	{
		int cmdSize = static_cast<int>(m_commandLog.size());
		if(cmdSize > 0)
		{
			m_lastCommandIndex = static_cast<int>(m_commandLog.size()) - 1;
			m_currentInput = m_commandLog[m_lastCommandIndex];
		}
	}
	else
	{
		if(m_lastCommandIndex > 0)
		{
			m_lastCommandIndex--;
			m_currentInput = m_commandLog[m_lastCommandIndex];
		}
	}

	m_carotPosition = static_cast<int>(m_currentInput.size());
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::ShowNextCommand()
{
	if(m_lastCommandIndex == static_cast<unsigned int>(m_commandLog.size()) - 1)
	{
		m_lastCommandIndex = 0;
		m_currentInput = m_commandLog[m_lastCommandIndex];
	}
	else
	{
		m_lastCommandIndex++;
		m_currentInput = m_commandLog[m_lastCommandIndex];
	}

	m_carotPosition = static_cast<int>(m_currentInput.size());
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::HandleKeyDown( unsigned char vkKeyCode )
{
	switch( vkKeyCode )
	{
	case UP_ARROW:
	{
		//Show the last command executed
		ShowLastCommand();
	}
	break;
	case DOWN_ARROW:
	{
		ShowNextCommand();
	}
	break;
	case LEFT_ARROW:
	{
		if(m_carotPosition > 0)
		{
			m_carotPosition -= 1;
		}
	}
	break;
	case RIGHT_ARROW:
	{
		if(m_carotPosition < static_cast<int>(m_currentInput.size()))
		{
			m_carotPosition += 1;
		}
	}
	break;
	case DEL_KEY:
	{
		//Delete the char after carot
		if(m_carotPosition < static_cast<int>(m_currentInput.size()))
		{
			std::string firstSubString = m_currentInput.substr(0, m_carotPosition);
			std::string secondSubString = m_currentInput.substr(m_carotPosition + 1, m_currentInput.size());

			m_currentInput = firstSubString;
			m_currentInput += secondSubString;
		}

		ResetIndexValues();
	}
	break;
	case BACK_SPACE:
	{
		if(m_carotPosition > 0)
		{
			std::string firstSubString = m_currentInput.substr(0, m_carotPosition - 1);
			std::string secondSubString = m_currentInput.substr(m_carotPosition, m_currentInput.size());

			m_currentInput = firstSubString;
			m_currentInput += secondSubString;

			m_carotPosition--;
		}

		ResetIndexValues();
	}
	break;
	case KEY_ESC:
	{
		if(m_currentInput.size() > 0)
		{
			//Clear the input string
			m_currentInput.clear();
			m_carotPosition = 0;
		}
		else
		{
			//shut the console
			m_isOpen = false;
		}
	}
	break;
	case ENTER_KEY:
	{
		//Run inputString as command only if not empty
		if(m_currentInput.size() == 0)
		{
			return;
		}

		//bool result = g_eventSystem->FireEvent(m_currentInput);

		bool result = ExecuteCommandLine(m_currentInput);

		if(!result)
		{
			PrintString(CONSOLE_ERROR, m_currentInput + ": Command was not found");
		}
		
		//Store the last command that was executed but remove copies

		// order of unique elements is preserved
		auto end_unique = std::end(m_commandLog) ;
		for( auto iter = std::begin(m_commandLog) ; iter != end_unique ; ++iter )
		{
			end_unique = std::remove( iter+1, end_unique, *iter ) ;
		}
		m_commandLog.erase( end_unique, std::end(m_commandLog) ) ;

		
		auto findItr = std::find(m_commandLog.begin(), m_commandLog.end(), m_currentInput);
		if(findItr != m_commandLog.end())
		{
			m_commandLog.erase(findItr);
		}

		m_commandLog.push_back(m_currentInput);

		m_currentInput.clear();
		m_carotPosition = 0;
		m_lastCommandIndex = 0;
	}
	break;
	default:
	break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::ResetIndexValues()
{
	m_lastCommandIndex = 0;
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::HandleCharacter( unsigned char charCode )
{
	if(m_carotPosition == m_currentInput.size())
	{
		m_currentInput += charCode;
		m_carotPosition += 1;
	}
	else
	{
		//carot is not at the end so add at the correct pos
		std::string m_firstSubString = m_currentInput.substr(0, m_carotPosition);
		std::string m_secondSubString = m_currentInput.substr(m_carotPosition, m_currentInput.size());

		m_currentInput = m_firstSubString;
		m_currentInput += charCode;
		m_currentInput += m_secondSubString;

		m_carotPosition++;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
const STATIC Rgba DevConsole::CONSOLE_ECHO_COLOR		=	Rgba(0.255f, 0.450f, 1.0f, 1.0f);

//------------------------------------------------------------------------------------------------------------------------------
DevConsole::DevConsole()
{
	m_timeAtStart = static_cast<float>(GetCurrentTimeSeconds());
}

//------------------------------------------------------------------------------------------------------------------------------
DevConsole::~DevConsole()
{

}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::Startup()
{
	g_eventSystem->SubscribeEventCallBackFn( "Test", Command_Test );
	g_eventSystem->SubscribeEventCallBackFn( "Help", Command_Help );
	g_eventSystem->SubscribeEventCallBackFn( "Clear", Command_Clear );
	g_eventSystem->SubscribeEventCallBackFn( "TrackMemory", Command_MemTracking);
	g_eventSystem->SubscribeEventCallBackFn( "LogMemory", Command_MemLog);

	g_eventSystem->SubscribeEventCallBackFn("EnableAllLogs", Command_EnableAllLogFilters);
	g_eventSystem->SubscribeEventCallBackFn("DisableAllLogs", Command_DisableAllLogfilters);
	g_eventSystem->SubscribeEventCallBackFn("EnableLogFilter", Command_EnableLogFilter);
	g_eventSystem->SubscribeEventCallBackFn("DisableLogFilter", Command_DisableLogFilter);
	g_eventSystem->SubscribeEventCallBackFn("FlushLog", Command_FlushLogSystem);
	g_eventSystem->SubscribeEventCallBackFn("Logf", Command_Logf);

	m_currentInput.clear();
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::BeginFrame()
{

}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::UpdateConsole(float deltaTime)
{
	m_carotTimeDiff += deltaTime;

	if(m_carotTimeDiff > 0.5f)
	{
		m_carotActive = !m_carotActive;
		m_carotTimeDiff = 0.f;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::EndFrame()
{
	m_frameCount++;
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::Shutdown()
{

}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::SetBitmapFont( BitmapFont& bitmapFont )
{
	m_consoleFont = &bitmapFont;
}

//------------------------------------------------------------------------------------------------------------------------------
ConsoleEntry::ConsoleEntry(const Rgba& textColor, const std::string& printString, unsigned int frameNum, float frameTime)
	: m_printColor (textColor),
	  m_printString (printString),
	  m_frameNum (frameNum),
	  m_calledTime (frameTime)
{

}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::PrintString( const Rgba& textColor, const std::string& devConsolePrintString )
{
	g_devConsole->m_mutexLock.lock();
	float time = static_cast<float>(GetCurrentTimeSeconds());
	time -= m_timeAtStart;
	ConsoleEntry consoleEntry = ConsoleEntry(textColor, devConsolePrintString, m_frameCount, time);
	
	// #ToDo: make this thread-safe
	m_printLog.push_back(consoleEntry);
	g_devConsole->m_mutexLock.unlock();
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::Render( RenderContext& renderer, Camera& camera, float lineHeight ) const
{
	g_devConsole->m_mutexLock.lock();
	camera.SetModelMatrix(Matrix44::IDENTITY);
	camera.UpdateUniformBuffer(&renderer);

	renderer.BeginCamera(camera);

	renderer.BindTextureViewWithSampler( 0U, m_consoleFont->GetTexture()); 

	//Draw a black box over the entire screen
	AABB2 blackBox = AABB2(camera.GetOrthoBottomLeft(), camera.GetOrthoTopRight());
	std::vector<Vertex_PCU> boxVerts;
	AddVertsForAABB2D(boxVerts, blackBox, CONSOLE_BG_COLOR);

	//Set the text based on Camera size
	Vec2 leftBot = camera.GetOrthoBottomLeft();
	Vec2 rightTop = camera.GetOrthoTopRight();

	float screenHeight = rightTop.y - leftBot.y;
	float screenWidth = rightTop.x - leftBot.x;

	int numLines = static_cast<int>(screenHeight / lineHeight);
	int numStrings = static_cast<int>(m_printLog.size());

	std::vector<ConsoleEntry> printLogVector;

	Vec2 boxBottomLeft = leftBot + Vec2(lineHeight, lineHeight * 2);
	Vec2 boxTopRight = boxBottomLeft;

	//Get the last string in the map and work your way back
	std::vector<ConsoleEntry>::const_iterator vecIterator = m_printLog.end();

	std::vector<Vertex_PCU> textVerts;
	renderer.BindTextureView(0U, m_consoleFont->GetTexture());

	//Setup your loop end condition
	int endCondition = 0;
	if(numLines < numStrings)
	{
		endCondition = numLines;
	}
	else
	{
		endCondition = numStrings;
	}

	for(int lineIndex = 0; lineIndex < endCondition; lineIndex++)
	{
		//Decrement the iterator
		vecIterator--;

		textVerts.clear();

		//Get length of string
		std::string printString = "[ T:" + Stringf("%.3f", vecIterator->m_calledTime) + " Frame:" + std::to_string(vecIterator->m_frameNum) + " ] " + vecIterator->m_printString;
		int numChars = static_cast<int>(printString.length());

		//Create the required box
		float glyphWidth = lineHeight * m_consoleFont->GetGlyphAspect(0);
		boxTopRight = boxBottomLeft;
		boxTopRight += Vec2(numChars * glyphWidth , lineHeight);
		AABB2 printBox = AABB2(boxBottomLeft, boxTopRight);

		//Print the text
		m_consoleFont->AddVertsForTextInBox2D(textVerts, printBox, lineHeight, printString, vecIterator->m_printColor, m_consoleFont->GetGlyphAspect(0), Vec2::ALIGN_LEFT_CENTERED);
		renderer.DrawVertexArray(textVerts);

		textVerts.clear();

		//Change the boxBottomLeft based on line height
		boxBottomLeft.y += lineHeight;

		//Add a padding if desired
		//boxBottomLeft.y += CONSOLE_LINE_SPACE;
	}

	//Draw Text Input box
	Vec2 textBoxBottomLeft = leftBot + Vec2(lineHeight, lineHeight * 0.25f);
	Vec2 textBoxTopRight = textBoxBottomLeft + Vec2(screenWidth - lineHeight * 2, lineHeight * 1.25f);

	AABB2 inputBox = AABB2(textBoxBottomLeft, textBoxTopRight);

	renderer.BindTextureView(0U, nullptr);
	
	std::vector<Vertex_PCU> inputBoxVerts;
	AddVertsForBoundingBox(inputBoxVerts, inputBox, CONSOLE_INFO, lineHeight * 0.1f);
	renderer.DrawVertexArray(inputBoxVerts);

	//Draw current string
	renderer.BindTextureView(0U, m_consoleFont->GetTexture());

	std::vector<Vertex_PCU> inputStringVerts;
	m_consoleFont->AddVertsForTextInBox2D(inputStringVerts, inputBox, lineHeight, m_currentInput, CONSOLE_INPUT, 1.f, Vec2::ALIGN_LEFT_CENTERED);
	renderer.DrawVertexArray(inputStringVerts);

	if(m_carotActive)
	{
		//Draw carot
		renderer.BindTextureView(0U, nullptr);

		Vec2 carotStart = inputBox.m_minBounds;
		carotStart.x += m_carotPosition * lineHeight;
		Vec2 carotEnd = carotStart + Vec2(0.f, lineHeight * 1.25f);

		std::vector<Vertex_PCU> carotVerts;
		AddVertsForLine2D(carotVerts, carotStart, carotEnd, lineHeight * 0.1f, CONSOLE_INPUT);
		renderer.DrawVertexArray(carotVerts);
	}

	if (m_memTrackingEnabled)
	{
		RenderMemTrackingInfo(renderer, camera, lineHeight);
	}

	renderer.EndCamera();
	g_devConsole->m_mutexLock.unlock();
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::RenderMemTrackingInfo(RenderContext& renderer, Camera& camera, float lineHeight) const
{
	renderer.BindTextureView(0U, nullptr);

	//Draw a black box over the entire screen
	Vec2 boxTopRight = camera.GetOrthoTopRight();
	Vec2 boxBottomLeft = boxTopRight - m_memTrackingBoxSize;

	AABB2 blackBox = AABB2(boxBottomLeft, boxTopRight);
	std::vector<Vertex_PCU> boxVerts;
	AddVertsForAABB2D(boxVerts, blackBox, Rgba::DARK_GREY);

	renderer.DrawVertexArray(boxVerts);
 	renderer.BindTextureView(0U, m_consoleFont->GetTexture());

	Vec2 textBoxBottomLeft = boxBottomLeft;
	Vec2 textBoxTopRight = textBoxBottomLeft + Vec2(m_memTrackingBoxSize.x, lineHeight);

	AABB2 inputBox = AABB2(textBoxBottomLeft, textBoxTopRight);

	std::vector<Vertex_PCU> textVerts;

	GetVertsForDevConsoleMemTracker(textVerts, inputBox, lineHeight);

	renderer.DrawVertexArray(textVerts);

}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::GetVertsForDevConsoleMemTracker(std::vector<Vertex_PCU>& textVerts, AABB2& memTrackingBox, float lineHeight) const
{
	std::string textString;
	std::string numAllocationsText;
	std::string totalBytesAllocatedText;

#if defined(MEM_TRACKING)
	#if (MEM_TRACKING == MEM_TRACK_ALLOC_COUNT)
		textString = "Tracking Mode: Alloc Count";
		uint numAllocations = (uint)gTotalAllocations;
		numAllocationsText = Stringf("Total Allocation count: %u", numAllocations);

		m_consoleFont->AddVertsForTextInBox2D(textVerts, memTrackingBox, lineHeight, numAllocationsText, Rgba::GREEN, 1.f, Vec2::ALIGN_LEFT_BOTTOM);
		
		memTrackingBox.m_minBounds += Vec2(0.f, lineHeight);
		memTrackingBox.m_maxBounds += Vec2(0.f, lineHeight);
		m_consoleFont->AddVertsForTextInBox2D(textVerts, memTrackingBox, lineHeight, textString, Rgba::GREEN, 1.f, Vec2::ALIGN_LEFT_BOTTOM);
	#elif (MEM_TRACKING == MEM_TRACK_VERBOSE)
		textString = "Tracking Mode: Verbose";
		uint numAllocations = (uint)gTotalAllocations;
		numAllocationsText = Stringf("Total Allocation count: %u", numAllocations);
		totalBytesAllocatedText = GetSizeString(gTotalBytesAllocated);

		m_consoleFont->AddVertsForTextInBox2D(textVerts, memTrackingBox, lineHeight, totalBytesAllocatedText, Rgba::GREEN, 1.f, Vec2::ALIGN_LEFT_BOTTOM);
		
		memTrackingBox.m_minBounds += Vec2(0.f, lineHeight);
		memTrackingBox.m_maxBounds += Vec2(0.f, lineHeight);
		m_consoleFont->AddVertsForTextInBox2D(textVerts, memTrackingBox, lineHeight, numAllocationsText, Rgba::GREEN, 1.f, Vec2::ALIGN_LEFT_BOTTOM);

		memTrackingBox.m_minBounds += Vec2(0.f, lineHeight);
		memTrackingBox.m_maxBounds += Vec2(0.f, lineHeight);
		m_consoleFont->AddVertsForTextInBox2D(textVerts, memTrackingBox, lineHeight, textString, Rgba::GREEN, 1.f, Vec2::ALIGN_LEFT_BOTTOM);

	#endif
#else
	textString = "Tracking is Off";
	m_consoleFont->AddVertsForTextInBox2D(textVerts, memTrackingBox, lineHeight, textString, Rgba::GREEN, 1.f, Vec2::ALIGN_LEFT_BOTTOM);
#endif
}

//------------------------------------------------------------------------------------------------------------------------------
void DevConsole::ToggleOpenFull()
{
	if(m_isOpen)
	{
		m_isOpen = false;
	}
	else
	{
		m_isOpen = true;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool DevConsole::IsOpen() const
{
	return m_isOpen;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_Test( EventArgs& args )
{
	UNUSED( args );
	g_devConsole->PrintString( CONSOLE_INFO, Stringf( "Test command received" ) );
	//args.DebugPrintContents();
	return true; // Does not consume event; continue to call other subscribers� callback functions
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool	DevConsole::Command_Help( EventArgs& args )
{
	UNUSED(args);
	g_devConsole->PrintString( CONSOLE_INFO, Stringf( "> Help Commands available to user" ) );
	std::vector<std::string> eventsAvailable;
	g_eventSystem->GetSubscribedEventsList(eventsAvailable);
	for(int eventIndex = 0; eventIndex < static_cast<int>(eventsAvailable.size()); eventIndex++)
	{
		std::string printString =  "   " + eventsAvailable[eventIndex];
		g_devConsole->PrintString( CONSOLE_ECHO_COLOR, printString );
	}
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_Clear(EventArgs& args)
{
	UNUSED(args);
	g_devConsole->m_printLog.clear();
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_MemTracking(EventArgs& args)
{
	UNUSED(args);
	g_devConsole->m_memTrackingEnabled = !g_devConsole->m_memTrackingEnabled;
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_MemLog(EventArgs& args)
{
	UNUSED(args);
	MemTrackLogLiveAllocations();
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_EnableAllLogFilters(EventArgs& args)
{
	UNUSED(args);
	g_LogSystem->LogEnableAll();
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_DisableAllLogfilters(EventArgs& args)
{
	UNUSED(args);
	g_LogSystem->LogDisableAll();
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_EnableLogFilter(EventArgs& args)
{
	std::string filterName;
	filterName = args.GetValue("Filter", filterName);
	g_LogSystem->LogEnable(filterName.c_str());
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_DisableLogFilter(EventArgs& args)
{
	std::string filterName;
	filterName = args.GetValue("Filter", filterName);
	g_LogSystem->LogDisable(filterName.c_str());
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_FlushLogSystem(EventArgs& args)
{
	UNUSED(args);
	g_LogSystem->LogFlush();
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC bool DevConsole::Command_Logf(EventArgs& args)
{
	std::string filterText;
	std::string messageText;
	filterText = args.GetValue("Filter", filterText);
	messageText = args.GetValue("Message", messageText);
	g_LogSystem->Logf(filterText.c_str(), messageText.c_str());
	return true;
}
