#pragma once
//------------------------------------------------------------------------------------------------------------------------------
#include "ThirdParty/fmod/fmod.hpp"
#include <string>
#include <vector>
#include <map>

//------------------------------------------------------------------------------------------------------------------------------
typedef size_t SoundID;
typedef size_t SoundPlaybackID;
typedef size_t ChannelGroupID;
constexpr size_t MISSING_SOUND_ID = (size_t)(-1); // for bad SoundIDs and SoundPlaybackIDs

//------------------------------------------------------------------------------------------------------------------------------
class AudioSystem;
struct Vec3;

//------------------------------------------------------------------------------------------------------------------------------
class AudioSystem
{
public:
	AudioSystem();
	virtual ~AudioSystem();

public:
	virtual void				BeginFrame();
	virtual void				EndFrame();

	virtual SoundID				CreateOrGetSound( const std::string& soundFilePath );
	virtual SoundID				CreateOrGetSound3D(const std::string& soundFilePath);
	virtual SoundPlaybackID		PlaySound( SoundID soundID, bool isLooped=false, float volume=1.f, float balance=0.0f, float speed=1.0f, bool isPaused=false );
	virtual ChannelGroupID		CreateOrGetChannelGroup(const std::string& channelName);
	virtual SoundPlaybackID		Play3DSound( SoundID soundID, const Vec3& position, ChannelGroupID channelID, bool isLooped=false, float volume=1.f, float balance=0.0f, float speed=1.0f, bool isPaused=false );

	virtual void				StopSound( SoundPlaybackID soundPlaybackID );
	virtual void				SetSoundPlaybackVolume( SoundPlaybackID soundPlaybackID, float volume );	// volume is in [0,1]
	virtual void				SetSoundPlaybackBalance( SoundPlaybackID soundPlaybackID, float balance );	// balance is in [-1,1], where 0 is L/R centered
	virtual void				SetSoundPlaybackSpeed( SoundPlaybackID soundPlaybackID, float speed );		// speed is frequency multiplier (1.0 == normal)

	virtual void				SetAudioListener(const Vec3& position, const Vec3& forward, const Vec3& up);

	//FMOD Result validation
	virtual void				ValidateResult( FMOD_RESULT result );

protected:
	FMOD::System*						m_fmodSystem;
	//2D audio
	std::map< std::string, SoundID >	m_registeredSoundIDs;
	std::vector< FMOD::Sound* >			m_registeredSounds;
	//3D audio
	std::map< std::string, SoundID >	m_registered3DSoundIDs;
	std::vector< FMOD::Sound* >			m_registered3DSounds;
	//Channel Groups (Mixers)
	std::map< std::string, ChannelGroupID >		m_registeredChannelIDs;
	std::vector< FMOD::ChannelGroup* >			m_registeredChannels;

};