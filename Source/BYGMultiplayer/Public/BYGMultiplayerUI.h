#pragma once

#include "CoreMinimal.h"
#include "ImGuiCommon.h"
#include "BYGMultiplayerUI.generated.h"

UCLASS()
class UBYGMultiplayerUI : public UObject
{
public:
	GENERATED_BODY()
public:
#if WITH_IMGUI

	void Init();
	void DrawDebug(bool* bIsOpen);
	
	bool bHostLAN = false;
	char HostGameName[64] = "Test Game";
	TSharedPtr<FUniqueNetId> SessionId;
	bool bIsHosting = false;
	bool bIsEndingHosting = false;

	char LobbyMap[128] = "MP_Lobby";
	char MapArguments[128] = "listen";
	int32 HostSelectedMapIndex = 0;
	TArray<FString> HostMaps;
	

	bool bIsJoinedSession = false;
	bool bFindLAN = false;
	bool bFindViaPresence = true;
	int32 FindMaxResults = 10;
	int32 FindTimeout = 10;
	int32 FindCurrentItem = 0;

	bool bIsLoggedIn = false;
	FString PlayerNickname = "(Unknown)";

	int32 OnlineSubsystemIndex = 0;
	const uint32 STEAM_INDEX = 1;
	const char* OnlineSubsystems[2] = {
		"NULL", // null must always be 0
		"STEAM"
	};

	void ShowSessionInfo(FName SessionName);
	void ShowRegisterButton();
#endif
	
protected:
	class UBYGMultiplayerSubsystem* GetMultiplayerSubsystem() const;
};

