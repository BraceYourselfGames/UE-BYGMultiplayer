// Copyright Brace Yourself Games. All Rights Reserved.

// This is a wrapper around the online subsystem, with all the callbacks in place to smooth things out.
// Feel free to use this, or base your own implementation on it, or just throw it into the sea.

// Hosting: Call HostGame();
// Joining: Call FindGames(), then JoinGame()

// Under the hood:
// Create Session => Join Session => Start Session

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSessionSettings.h"
#include "ImGuiCommon.h"
#include "BYGMultiplayerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBYGMultiplayer, Log, All);

#define SETTING_SERVER_NAME FName(TEXT("SERVER_NAME"))

// Creates sensible defaults and exposes some more functionality in a nicer way
struct FBYGOnlineSessionSettings : public FOnlineSessionSettings
{
public:
	FBYGOnlineSessionSettings()
	{
		NumPublicConnections = 4;
		bIsLANMatch = false;
		bShouldAdvertise = true;
		bIsDedicated = false;
		bUsesPresence = true;
		bAllowJoinInProgress = true;
		bAllowJoinViaPresence = true;
		NumPrivateConnections = true;
		bAllowInvites = true;
		bAntiCheatProtected = false;
		bAllowJoinViaPresenceFriendsOnly = false;
		TargetMapName = "MP_Dummy";
		TargetPublicFacingMapName = "Dummy Map";
		LobbyMapName = "MP_Lobby";
		MapArguments = {};
		bTravelAbsolute = true;
	}
	FOnlineSessionSettings GetSessionSettings() const
	{
		FOnlineSessionSettings BaseSettings;
		BaseSettings.bIsLANMatch = bIsLANMatch;
		BaseSettings.NumPublicConnections = NumPublicConnections;
		BaseSettings.NumPrivateConnections = NumPrivateConnections;
		BaseSettings.bAllowInvites = bAllowInvites;
		BaseSettings.bAntiCheatProtected = bAntiCheatProtected;
		BaseSettings.bAllowJoinViaPresenceFriendsOnly = bAllowJoinViaPresenceFriendsOnly;
		BaseSettings.bShouldAdvertise = bShouldAdvertise;
		BaseSettings.bIsDedicated = bIsDedicated;
		BaseSettings.bUsesPresence = bUsesPresence;
		BaseSettings.bAllowJoinInProgress = bAllowJoinInProgress;
		BaseSettings.bAllowJoinViaPresence = bAllowJoinViaPresence;
		BaseSettings.bUseLobbiesIfAvailable = true;
		BaseSettings.Set<FString>(SETTING_MAPNAME, TargetPublicFacingMapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		// Custom but we can standardize it
		BaseSettings.Set<FString>(SETTING_SERVER_NAME, ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		return BaseSettings;
	}
	FString ServerName;
	FString TargetPublicFacingMapName;
	FName TargetMapName;
	FName LobbyMapName;
	TArray<FString> MapArguments;
	bool bTravelAbsolute;
};

UCLASS()
class BYGMULTIPLAYER_API UBYGMultiplayerSubsystem : public UGameInstanceSubsystem
{
public:
	GENERATED_BODY()
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

#if WITH_IMGUI
	void DrawDebug(bool* bIsOpen);
#endif

	// Making this all public for now. be nice

	bool bHostLAN = false;
	TSharedPtr<FUniqueNetId> SessionId;
	FBYGOnlineSessionSettings OnlineSessionSettings;

	bool bIsHosting = false;
	bool bIsEndingHosting = false;

	bool bIsJoinedSession = false;
	bool bFindLAN = false;
	bool bFindViaPresence = true;
	int32 FindMaxResults = 10;
	int32 FindTimeout = 10;

	//bool bIsLoggedIn = false;
	//FString PlayerNickname = "(Unknown)";

	// Names are case-insensitive. Using strings because future-proofing?
	// You probably want "STEAM" or "NULL"
	// Calls InitializeOnlineSubsystem with the new index, but falls back to NULL on failure
	bool TryChangeOnlineSubsystem(const FName& SubsystemName);

	void HostGame();
	void CancelHostingGame();

	void FindSessions();

	// Index refers to an entry in SessionSearch. Call FindSessions first to populate it.
	void JoinSession(uint32 Index);

	IOnlineSessionPtr GetSession();
	void ResetState();

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;
	
	UPROPERTY()
	class UBYGMultiplayerUI* UI;

	FString GetPlayerNickname() const;
protected:
	FName CurrentSubsystemName = NAME_None;
	bool InitializeOnlineSubsystem(const FName& SubsystemName);

	FOnCreateSessionCompleteDelegate CreateCompleteDelegate;
	FDelegateHandle CreateCompleteDelegateHandle;
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	FOnStartSessionCompleteDelegate StartCompleteDelegate;
	FDelegateHandle StartCompleteDelegateHandle;
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

	FOnJoinSessionCompleteDelegate JoinCompleteDelegate;
	FDelegateHandle JoinCompleteDelegateHandle;
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	FOnLoginCompleteDelegate LoginCompleteDelegate;
	FDelegateHandle LoginCompleteDelegateHandle;
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	void OnFindSessionsComplete(bool bWasSuccessful);

	//void OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful);

	void DoEndSession(FName SessionName);

	FOnEndSessionCompleteDelegate EndSessionCompleteDelegate;
	FDelegateHandle EndSessionCompleteDelegateHandle;
	void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);
	//void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	//void OnPingSearchResultsComplete(bool bWasSuccessful);
	//void OnCancelFindSessionsComplete(bool bWasSuccessful);
	//void OnMatchmakingComplete(FName SessionName, bool bWasSuccessful);
	//void OnCancelMatchmakingComplete(FName SessionName, bool bWasSuccessful);
	//void OnRegisterPlayersComplete(FName SessionName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIDs, bool bWasSuccessful);
	//void OnUnregisterPlayersComplete(FName SessionName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIDs, bool bWasSuccessful);

	// See Engine.h for these originally
	virtual void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	virtual void HandleTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString);


	/** @return the current game world */
	virtual UWorld* GetWorld() const override;
};
