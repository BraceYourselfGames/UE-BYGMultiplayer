// Copyright Brace Yourself Games. All Rights Reserved.

#include "BYGMultiplayerSubsystem.h"
#include "Online.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystemSteam.h"
#include "BYGMultiplayerUI.h"


DEFINE_LOG_CATEGORY (LogBYGMultiplayer);

void UBYGMultiplayerSubsystem::ResetState()
{
	CurrentSubsystemName = NAME_None;
	OnlineSessionSettings = FBYGOnlineSessionSettings();

	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegateHandle);
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		SessionInterface->DestroySession(NAME_GameSession);
	}
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(CurrentSubsystemName);
	if (Subsystem)
	{
		IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteDelegateHandle);
		}
	}
}

void UBYGMultiplayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if WITH_IMGUI
	UI = NewObject<UBYGMultiplayerUI>(this);
	UI->Init();
#endif

	ResetState();

	// Register all of our delegates to call our functions when they are fired
	CreateCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete);
	StartCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete);
	JoinCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete);
	LoginCompleteDelegate = FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnLoginComplete);

	if (ensure(GEngine))
	{
		//GEngine->OnTravelFailure().AddUObject(this, &UBYGMultiplayerSubsystem::HandleTravelFailure);
		//GEngine->OnNetworkFailure().AddUObject(this, &UBYGMultiplayerSubsystem::HandleNetworkFailure);
	}
}

void UBYGMultiplayerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ResetState();
}

bool UBYGMultiplayerSubsystem::TryChangeOnlineSubsystem(const FName& SubsystemName)
{
	ResetState();

	const bool bWasSuccess = InitializeOnlineSubsystem(SubsystemName);
	if (bWasSuccess)
	{
		UE_LOG(LogBYGMultiplayer, Log, TEXT("Successfully initialized subsystem '%s'"), *SubsystemName.ToString());
		return true;
	}
	else
	{
		const FName FallbackName = "NULL";
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to initialize subsystem '%s'. Changing back to subsystem '%s'"), *SubsystemName.ToString(), *FallbackName.ToString());
		InitializeOnlineSubsystem(FallbackName);
		return false;
	}
}

bool UBYGMultiplayerSubsystem::InitializeOnlineSubsystem(const FName& SubsystemName)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(SubsystemName);
	if (Subsystem)
	{
		// NOTE: I used to bulk-register a tonne of delegates here, stuff like OnStartSessionCompleteDelegates.AddUObject
		// But I encountered situations where the delegates were not being fired.
		// Other examples register just before calling the function, and use the AddOnBlahBlahDelegate_Handle signature.

		if (SubsystemName != "Null")
		{
			const int32 LocalUserNum = 0;
			IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
			if (Identity.IsValid())
			{
				LoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteDelegate);
				if (Identity->AutoLogin(LocalUserNum))
				{
					UE_LOG(LogBYGMultiplayer, Log, TEXT("User logged in!"));
				}
				else
				{
					UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to log user into subsystem"));
				}
			}
			else
			{
				UE_LOG(LogBYGMultiplayer, Error, TEXT("Could not get identity interface for login"));
			}
		}
		return true;
	}
	UE_LOG(LogBYGMultiplayer, Error, TEXT("Could not initialize/get subsystem '%s'"), *SubsystemName.ToString());
	return false;
}

void UBYGMultiplayerSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(CurrentSubsystemName);
		if (Subsystem)
		{
			IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
			if (Identity)
			{
				UE_LOG(LogBYGMultiplayer, Log, TEXT("On ping search results complete : %s"), (bWasSuccessful ? TEXT("success") : TEXT("failure")));
				Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteDelegateHandle);
			}
		}
	}
}


void UBYGMultiplayerSubsystem::HostGame()
{
	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings = OnlineSessionSettings.GetSessionSettings();

		// This is how we can set custom variables
		// SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(CurrentSubsystemName);
		const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
		if (!IdentityInterface.IsValid())
		{
			UE_LOG(LogBYGMultiplayer, Error, TEXT("Could not get identity interface"));
			return;
		}

		TSharedPtr<const FUniqueNetId> PlayerId = IdentityInterface->GetUniquePlayerId(0);
		if (!PlayerId.IsValid())
		{
			UE_LOG(LogBYGMultiplayer, Error, TEXT("Player ID is invalid"));
			return;
		}

		CreateCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegate);

		bIsHosting = SessionInterface->CreateSession(*PlayerId, NAME_GameSession, SessionSettings);
		if (bIsHosting)
		{
			UE_LOG(LogBYGMultiplayer, Log, TEXT("HostGame succeeded"));
		}
		else
		{
			UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to host"));
		}
	}
	else
	{
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to get session interface"));
	}
}

void UBYGMultiplayerSubsystem::CancelHostingGame()
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("Start cancel hosting"));
	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface)
	{
		// end immediately or not
		if (SessionInterface->EndSession(NAME_GameSession))
		{
			//bIsEndingHosting = true;
		}
		else
		{
			SessionInterface->DestroySession(NAME_GameSession);
			//bIsHosting = false;
		}
	}
}

void UBYGMultiplayerSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On create session '%s' complete: %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("success") : TEXT("failure"));
	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegateHandle);
		if (bWasSuccessful)
		{
			UE_LOG(LogBYGMultiplayer, Log, TEXT("Automatically starting session"));
			StartCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartCompleteDelegate);
			SessionInterface->StartSession(SessionName);
		}
	}
}

void UBYGMultiplayerSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On start session '%s' complete: %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("success") : TEXT("failure"));
	if (bWasSuccessful)
	{
		UE_LOG(LogBYGMultiplayer, Log, TEXT("Travelling to target map: '%s'"), *OnlineSessionSettings.TargetMapName.ToString());

		IOnlineSessionPtr SessionInterface = GetSession();
		if (ensure(SessionInterface.IsValid()))
		{
			SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartCompleteDelegateHandle);
		}
		const FString Arguments = FString::Join(OnlineSessionSettings.MapArguments, TEXT("?"));
		UGameplayStatics::OpenLevel(GetWorld(), OnlineSessionSettings.TargetMapName, OnlineSessionSettings.bTravelAbsolute, Arguments);
	}
}

void UBYGMultiplayerSubsystem::FindSessions()
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("Finding sessions"));
	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface.IsValid())
	{
		SessionSearch = MakeShareable<FOnlineSessionSearch>(new FOnlineSessionSearch());
		if (SessionSearch.IsValid())
		{
			SessionSearch->bIsLanQuery = bFindLAN;
			SessionSearch->MaxSearchResults = FindMaxResults;
			SessionSearch->TimeoutInSeconds = FindTimeout;
			// NOTE: THIS IS INCREDIBLY IMPORTANT
			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, bFindViaPresence, EOnlineComparisonOp::Equals);
			// NOTE ^
			FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
			SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
		}
		else
		{
			UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to create new session search interface"));
		}
	}
	else
	{
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to get session interface"));
	}
}

void UBYGMultiplayerSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On find session complete: %s"), bWasSuccessful ? TEXT("success") : TEXT("failure"));

	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface.IsValid())
	{
		if (SessionSearch.IsValid())
		{
			UE_LOG(LogBYGMultiplayer, Log, TEXT("Found %d results"), SessionSearch->SearchResults.Num());
		}
	}
}

void UBYGMultiplayerSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On join session '%s' complete with result: %s"), *SessionName.ToString(), LexToString(Result));
	IOnlineSessionPtr SessionInterface = GetSession();
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteDelegateHandle);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		bIsJoinedSession = true;
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
		{
			UE_LOG(LogBYGMultiplayer, Log, TEXT("Connect string: %s"), *ConnectInfo);
			APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController(GetWorld());
			UE_LOG(LogBYGMultiplayer, Log, TEXT("Travelling client"));
			PC->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
		}
		else
		{
			UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to get connect string"));
		}
	}
	else
	{
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Join session complete returned non-success! %s"), LexToString(Result));
		bIsJoinedSession = false;
	}
}


void UBYGMultiplayerSubsystem::JoinSession(uint32 Index)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("Join session index '%d'"), Index);

	if (!SessionSearch.IsValid())
	{
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Called join session but session search instance is invalid"));
		return;
	}
	if (!SessionSearch->SearchResults.IsValidIndex(Index))
	{
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Called join session with index %d but session search results only have %d entries. Invalid index."), Index, SessionSearch->SearchResults.Num());
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(CurrentSubsystemName);
	IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
	TSharedPtr<const FUniqueNetId> PlayerId;
	if (IdentityInterface.IsValid())
	{
		PlayerId = IdentityInterface->GetUniquePlayerId(0);
	}
	IOnlineSessionPtr SessionInterface = GetSession();
	if (SessionInterface.IsValid())
	{
		JoinCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinCompleteDelegate);
		if (SessionInterface->JoinSession(*PlayerId, NAME_GameSession, SessionSearch->SearchResults[Index]))
		{
			UE_LOG(LogBYGMultiplayer, Log, TEXT("Join session started"));
		}
		else
		{
			UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to join session"));
		}
	}
	else
	{
		UE_LOG(LogBYGMultiplayer, Error, TEXT("Could not get session interface"));
	}
}

IOnlineSessionPtr UBYGMultiplayerSubsystem::GetSession()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(CurrentSubsystemName);
	if (Subsystem)
		return Subsystem->GetSessionInterface();
	return IOnlineSessionPtr();

	/* Another example gets it this way?
	UWorld* world = GetWorld();
	check(world != nullptr);
	return Online::GetSessionInterface(world);
	*/
}

UWorld* UBYGMultiplayerSubsystem::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}

#if WITH_IMGUI
void UBYGMultiplayerSubsystem::DrawDebug(bool* bIsOpen)
{
	if (UI)
	{
		UI->DrawDebug(bIsOpen);
	}
}
#endif


#if 0
void UBYGMultiplayerSubsystem::OnRegisterPlayersComplete(FName SessionName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIDs, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On register players complete: %d"), bWasSuccessful);
}

void UBYGMultiplayerSubsystem::OnUnregisterPlayersComplete(FName SessionName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIDs, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On unregister player complete"));
}
#endif

#if 0
// later
void UBYGMultiplayerSubsystem::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On update session complete"));
}

void UBYGMultiplayerSubsystem::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On end session complete"));
	bIsEndingHosting = false;
	bIsHosting = false;
	//CurrentHostedGameName = NAME_None;
}

void UBYGMultiplayerSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On destroy session complete"));
}
#endif

#if 0
void UBYGMultiplayerSubsystem::OnPingSearchResultsComplete(bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On ping search results complete : %s"), (bWasSuccessful ? "Success!" : "Failure"));
}

void UBYGMultiplayerSubsystem::OnMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On matchmaking complete '%s' complete with result: %d"), *SessionName.ToString(), bWasSuccessful);
}

void UBYGMultiplayerSubsystem::OnCancelMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On cancel matchmaking complete '%s' complete with result: %d"), *SessionName.ToString(), bWasSuccessful);
}

void UBYGMultiplayerSubsystem::OnCancelFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogBYGMultiplayer, Log, TEXT("On cancel find session complete: %d"), bWasSuccessful);
}
#endif

FString UBYGMultiplayerSubsystem::GetPlayerNickname() const
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(CurrentSubsystemName);
	if (Subsystem)
	{
		IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			return Identity->GetPlayerNickname(0);
		}
	}
	return "(Unknown)";
}
