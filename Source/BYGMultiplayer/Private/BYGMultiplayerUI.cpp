// Copyright Brace Yourself Games. All Rights Reserved.

#include "BYGMultiplayerUI.h"
#include "imgui_helpers.h"
#include "BYGMultiplayerSubsystem.h"
#include "ImGuiCommon.h"
#include "OnlineSubsystem.h"

void UBYGMultiplayerUI::Init()
{
	if (ensure(GEngine))
	{
		//GEngine->OnTravelFailure().AddUObject(this, &UBYGMultiplayerUI::HandleTravelFailure);
		//GEngine->OnNetworkFailure().AddUObject(this, &UBYGMultiplayerUI::HandleNetworkFailure);
	}
}

#if WITH_IMGUI
void UBYGMultiplayerUI::ShowRegisterButton()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(OnlineSubsystems[OnlineSubsystemIndex]);
	if (!Subsystem)
		return;
	IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
	if (IdentityInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> PlayerId = IdentityInterface->GetUniquePlayerId(0);
		IOnlineSessionPtr SessionInterface = GetMultiplayerSubsystem()->GetSession();
		FNamedOnlineSession* FoundSession = SessionInterface->GetNamedSession(NAME_GameSession);
		bool bIsRegistered = false;
		if (FoundSession)
		{
			for (const TSharedRef<const FUniqueNetId>& RegisteredPlayer : FoundSession->RegisteredPlayers)
			{
				if (RegisteredPlayer->GetHexEncodedString() == PlayerId->GetHexEncodedString())
				{
					bIsRegistered = true;
					break;
				}
			}
		}

		if (bIsRegistered)
			ImGui::PushDisabled();
		if (ImGui::Button(bIsRegistered ? "Registered" : "Register"))
		{
			if (SessionInterface->RegisterPlayers(NAME_GameSession, {PlayerId.ToSharedRef()}, false))
			{
				UE_LOG(LogBYGMultiplayer, Log, TEXT("Registered player!"));
			}
			else
			{
				UE_LOG(LogBYGMultiplayer, Error, TEXT("Failed to register player!"));
			}
		}
		if (bIsRegistered)
			ImGui::PopDisabled();
	}
}

void UBYGMultiplayerUI::ShowSessionInfo(FName SessionName)
{
	IOnlineSessionPtr SessionInterface = GetMultiplayerSubsystem()->GetSession();
	if (SessionInterface.IsValid())
	{
		FNamedOnlineSession* FoundSession = SessionInterface->GetNamedSession(SessionName);
		if (FoundSession)
		{
			ImGui::PushDisabled();
			FString SessionIdStr = FoundSession->GetSessionIdStr();
			ImGui::InputText("Session ID", &SessionIdStr);
			ImGui::Checkbox("Is host", &FoundSession->bHosting);
			FString State;
			switch (FoundSession->SessionState)
			{
			case EOnlineSessionState::NoSession:
				State = "No session";
				break;
			case EOnlineSessionState::Creating:
				State = "Creating";
				break;
			case EOnlineSessionState::Pending:
				State = "Pending";
				break;
			case EOnlineSessionState::Starting:
				State = "Starting";
				break;
			case EOnlineSessionState::InProgress:
				State = "In progress";
				break;
			case EOnlineSessionState::Ending:
				State = "Ending";
				break;
			case EOnlineSessionState::Ended:
				State = "Ended";
				break;
			case EOnlineSessionState::Destroying:
				State = "Destroying";
				break;
			}
			ImGui::InputText("Session state", &State);
			FString SessionNameStr = FoundSession->SessionName.ToString();
			ImGui::InputText("Session name", &SessionNameStr);
			ImGui::InputInt("Open public connections", &FoundSession->NumOpenPublicConnections);
			ImGui::InputInt("Open private connections", &FoundSession->NumOpenPrivateConnections);
			ImGui::InputInt("Hosting player num", &FoundSession->HostingPlayerNum);
			FString LocalOwnerId = "null";
			if (FoundSession->LocalOwnerId.IsValid())
			{
				LocalOwnerId = FoundSession->LocalOwnerId->ToString();
			}
			ImGui::InputText("Local owner ID", &LocalOwnerId);

			ImGui::PopDisabled();

			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
			ImGui::Columns(2, "HostColumns");
			ImGui::Separator();
			ImGui::Text("Index");
			ImGui::NextColumn();
			ImGui::Text("User");
			ImGui::NextColumn();
			ImGui::Separator();
			for (int32 j = 0; j < FoundSession->RegisteredPlayers.Num(); ++j)
			{
				ImGui::Text("%d", j);
				ImGui::NextColumn();
				ImGui::Text(TCHAR_TO_ANSI(*FoundSession->RegisteredPlayers[j]->ToString()));
				ImGui::NextColumn();
			}
		}
		else
		{
			ImGui::PushDisabled();
			ImGui::Text("No local session exists with ID '%s'. Try hosting or joining a game.", TCHAR_TO_ANSI(*FName(NAME_GameSession).ToString()));
			ImGui::PopDisabled();
		}
	}
}

void UBYGMultiplayerUI::DrawDebug(bool* bIsOpen)
{
	
	ImGui::Begin("Multiplayer", bIsOpen);
	if (ImGui::Combo("Online subsystem", &OnlineSubsystemIndex, OnlineSubsystems, IM_ARRAYSIZE(OnlineSubsystems)))
	{
		const bool bWasSuccess = GetMultiplayerSubsystem()->TryChangeOnlineSubsystem(FName(OnlineSubsystems[OnlineSubsystemIndex]));
	}

	{
		IOnlineSessionPtr SessionInterface = GetMultiplayerSubsystem()->GetSession();
		ImGui::Text("Current sessions: %d", SessionInterface->GetNumSessions());
	}

	if (ImGui::BeginTabBar("MultiplayerTabBar"))
	{
		if (ImGui::BeginTabItem("Info"))
		{
			if (OnlineSubsystemIndex == STEAM_INDEX)
			{
				ImGui::Text("Steam information");
				ImGui::PushDisabled();
				ImGui::Checkbox("Is logged in?", &bIsLoggedIn);
				ImGui::PopDisabled();
				ImGui::Text("ID:");
				ImGui::SameLine();
				ImGui::Text("Display Name:");
				ImGui::SameLine();
				ImGui::Text(TCHAR_TO_ANSI(*PlayerNickname));

				FOnlineSubsystemSteam* SubsystemSteam = (FOnlineSubsystemSteam*)IOnlineSubsystem::Get(OnlineSubsystems[OnlineSubsystemIndex]);
				if (SubsystemSteam)
				{
					ImGui::PushDisabled();
					bool bClientAvailable = SubsystemSteam->IsSteamClientAvailable();
					ImGui::Checkbox("Client init?", &bClientAvailable);
					ImGui::PopDisabled();
					ImGui::SameLine();
					ImGui::HelpMarker("Has the Steamworks client been successfully initialized?");
					ImGui::SameLine();
					ImGui::PushDisabled();
					bool bServerAvailable = SubsystemSteam->IsSteamServerAvailable();
					ImGui::Checkbox("Server init?", &bServerAvailable);
					ImGui::PopDisabled();
					ImGui::SameLine();
					ImGui::HelpMarker("Has the Steamworks server been successfully initialized? If false we won't be able to host games. To enable this check DefaultEngine.ini, [OnlineSubsystemSteam] bInitServerOnClient=true ");

					bool bIsUsingSteamNetworking = SubsystemSteam->IsUsingSteamNetworking();
					ImGui::PushDisabled();
					ImGui::Checkbox("Steam networking?", &bIsUsingSteamNetworking);
					ImGui::SameLine();
					ImGui::PopDisabled();
					ImGui::HelpMarker("If this subsystem is using SteamNetworking functionality or another network layer like SteamSockets");
					ImGui::PushDisabled();
					int32 GamePort = SubsystemSteam->GetGameServerGamePort();
					ImGui::InputInt("Game port", &GamePort);
					int32 QueryPort = SubsystemSteam->GetGameServerQueryPort();
					ImGui::InputInt("Query port", &QueryPort);
					int32 SteamPort = SubsystemSteam->GetGameServerSteamPort();
					ImGui::InputInt("Steam port", &SteamPort);
					ImGui::PopDisabled();
				}
			}
			else
			{
				ImGui::Text("Default LAN-only subsystem");
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Host"))
		{
			UBYGMultiplayerSubsystem* Sys = GetMultiplayerSubsystem();
			
			ImGui::Text("Start a game as a host");
			if (bIsHosting || bIsEndingHosting)
			{
				ImGui::PushDisabled();
			}
			ImGui::InputText("Server name", HostGameName, IM_ARRAYSIZE(HostGameName));
			if (OnlineSubsystemIndex == STEAM_INDEX)
			{
				bHostLAN = false;
			}
			else
			{
				ImGui::Checkbox("LAN game", &bHostLAN);
				ImGui::SameLine();
				ImGui::HelpMarker("Local Area Network");
			}
			ImGui::InputInt("Public slots", &Sys->OnlineSessionSettings.NumPublicConnections);
			ImGui::SameLine();
			ImGui::HelpMarker("The number of people that can play on the server (including the host if it is a non-dedicated game).");
			if (ImGui::Combo("Map", &HostSelectedMapIndex, HostMaps, IM_ARRAYSIZE(HostMaps)))
			{
				// ??
			}
			if (bIsHosting || bIsEndingHosting)
			{
				ImGui::PopDisabled();
			}

			if (ImGui::CollapsingHeader("Advanced Host Game"))
			{
				if (bIsHosting || bIsEndingHosting)
				{
					ImGui::PushDisabled();
				}
				
				ImGui::InputInt("Private connections", &Sys->OnlineSessionSettings.NumPrivateConnections);
				ImGui::Checkbox("Is dedicated", &Sys->OnlineSessionSettings.bIsDedicated);
				ImGui::SameLine();
				ImGui::HelpMarker("Probably want this false");
				ImGui::Checkbox("Should advertise", &Sys->OnlineSessionSettings.bShouldAdvertise);
				ImGui::SameLine();
				ImGui::HelpMarker("Probably want this true");
				ImGui::Checkbox("Uses presence", &Sys->OnlineSessionSettings.bUsesPresence);
				ImGui::SameLine();
				ImGui::HelpMarker("If true on Steam, creating a host will start a lobby session rather than a regular multiplayer session.");
				ImGui::Checkbox("Anti cheat protected", &Sys->OnlineSessionSettings.bAntiCheatProtected);
				ImGui::Checkbox("Allow invites", &Sys->OnlineSessionSettings.bAllowInvites);
				ImGui::Checkbox("Allow join in progress", &Sys->OnlineSessionSettings.bAllowJoinInProgress);
				ImGui::Checkbox("Allow join via presence", &Sys->OnlineSessionSettings.bAllowJoinViaPresence);
				ImGui::Checkbox("Allow join via presence Friends only", &Sys->OnlineSessionSettings.bAllowJoinViaPresenceFriendsOnly);
				if (bIsHosting || bIsEndingHosting)
				{
					ImGui::PopDisabled();
				}
			}

			if (bIsEndingHosting)
			{
				ImGui::PushDisabled();
				ImGui::Button("Stopping Hosting");
				ImGui::PopDisabled();
			}
			else if (bIsHosting)
			{
				if (ImGui::Button("Stop Hosting"))
				{
					GetMultiplayerSubsystem()->CancelHostingGame();
				}

				ShowRegisterButton();

				IOnlineSessionPtr SessionInterface = GetMultiplayerSubsystem()->GetSession();
				FNamedOnlineSession* FoundSession = SessionInterface->GetNamedSession(NAME_GameSession);
				if (FoundSession && FoundSession->SessionState == EOnlineSessionState::Pending)
				{
					if (ImGui::Button("Start Session"))
					{
						SessionInterface->StartSession(NAME_GameSession);
					}
					ImGui::SameLine();
					ImGui::HelpMarker("We created the session, now it is in pending state. You need to manually Start the session to allow people to join");
				}

				UWorld* World = GetWorld();
				if (World)
				{
					ImGui::InputText("Lobby map", LobbyMap, IM_ARRAYSIZE(LobbyMap));
					ImGui::SameLine();
					ImGui::HelpMarker("Map name does not need full path, just the map name. Case-insensitive.");
					ImGui::InputText("Arguments", MapArguments, IM_ARRAYSIZE(MapArguments));
					ImGui::SameLine();
					ImGui::HelpMarker("Question-mark separated arguments. No starting question-mark needed. Should include 'listen'");
					ImGui::Checkbox("Travel absolute", &Sys->OnlineSessionSettings.bTravelAbsolute);
					if (ImGui::Button("Server travel"))
					{
						World->ServerTravel(FString(LobbyMap) + "?" + FString(MapArguments), Sys->OnlineSessionSettings.bTravelAbsolute);
					}
				}
				else
				{
					ImGui::Text("Cannot get world!!!");
				}
			}
			else
			{
				if (ImGui::Button("Host Game"))
				{
					// Set the props
					GetMultiplayerSubsystem()->OnlineSessionSettings.ServerName = HostGameName;
					GetMultiplayerSubsystem()->OnlineSessionSettings.TargetMapName = HostMaps[HostSelectedMapIndex];
					GetMultiplayerSubsystem()->OnlineSessionSettings.LobbyMapName = LobbyMap;
					GetMultiplayerSubsystem()->OnlineSessionSettings.MapArguments = { MapArguments };
					GetMultiplayerSubsystem()->HostGame();
				}
			}

			if (bIsHosting)
			{
				ImGui::Text("Current Hosted Session");

				ShowSessionInfo(NAME_GameSession);
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Join"))
		{
			ImGui::Text("Find and join games");
			if (OnlineSubsystemIndex == STEAM_INDEX)
			{
				bFindLAN = false;
			}
			else
			{
				ImGui::Checkbox("LAN game", &bFindLAN);
				ImGui::SameLine();
				ImGui::HelpMarker("Local Area Network");
			}
			if (ImGui::CollapsingHeader("Advanced Search"))
			{
				ImGui::Checkbox("Find via presence", &bFindViaPresence);
				ImGui::SameLine();
				ImGui::HelpMarker("You probably want this enabled. Very probably.");
				ImGui::InputInt("Max results", &FindMaxResults);
				ImGui::InputInt("Timeout", &FindTimeout);
				ImGui::SameLine();
				ImGui::HelpMarker("In seconds.");
			}
			if (ImGui::Button("Find games"))
			{
				GetMultiplayerSubsystem()->FindSessions();
			}

			ShowRegisterButton();

			TArray<FOnlineSessionSearchResult> Results;
			if (GetMultiplayerSubsystem()->SessionSearch.IsValid())
				Results = GetMultiplayerSubsystem()->SessionSearch->SearchResults;

			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
			ImGui::Columns(6, "ResultsColumns");
			ImGui::Separator();
			ImGui::Text("Server Name");
			ImGui::Separator();
			ImGui::Text("User");
			ImGui::NextColumn();
			ImGui::Text("Ping");
			ImGui::NextColumn();
			ImGui::Text("Players");
			ImGui::NextColumn();
			ImGui::Text("Max Players");
			ImGui::NextColumn();
			ImGui::Text("Join");
			ImGui::NextColumn();
			ImGui::Separator();
			if (Results.Num() > 0)
			{
				for (int32 i = 0; i < Results.Num(); ++i)
				{
					ImGui::PushID(i);
					const FOnlineSessionSetting* ServerNameProp = Results[i].Session.SessionSettings.Settings.Find(SETTING_SERVER_NAME);
					FString ServerNam;
					ServerNameProp->Data.GetValue(ServerName);
					ImGui::Text(TCHAR_TO_ANSI(*ServerName));
					ImGui::NextColumn();
					ImGui::Text(TCHAR_TO_ANSI(*Results[i].Session.OwningUserName));
					ImGui::NextColumn();
					ImGui::Text("%dms", Results[i].PingInMs);
					ImGui::NextColumn();
					ImGui::Text("%d", Results[i].Session.SessionSettings.NumPublicConnections - Results[i].Session.NumOpenPublicConnections);
					ImGui::NextColumn();
					ImGui::Text("%d", Results[i].Session.SessionSettings.NumPublicConnections);
					ImGui::NextColumn();
					if (ImGui::Button("Join Game"))
					{
						GetMultiplayerSubsystem()->JoinSession(i);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Session Details");
						ImGui::PushDisabled();
						ImGui::InputInt("Open public connections", &Results[i].Session.NumOpenPublicConnections);
						ImGui::InputInt("Open private connections", &Results[i].Session.NumOpenPrivateConnections);
						ImGui::InputInt("Num public connections", &Results[i].Session.SessionSettings.NumPublicConnections);
						ImGui::InputInt("Num private connections", &Results[i].Session.SessionSettings.NumPrivateConnections);
						ImGui::Checkbox("Is LAN match", &Results[i].Session.SessionSettings.bIsLANMatch);
						ImGui::Checkbox("Is dedicated", &Results[i].Session.SessionSettings.bIsDedicated);
						ImGui::Checkbox("Allow invites", &Results[i].Session.SessionSettings.bAllowInvites);
						ImGui::Checkbox("Uses presence", &Results[i].Session.SessionSettings.bUsesPresence);
						ImGui::Checkbox("Uses stats", &Results[i].Session.SessionSettings.bUsesStats);
						ImGui::Checkbox("Anti cheat protected", &Results[i].Session.SessionSettings.bAntiCheatProtected);
						ImGui::Checkbox("Allow join in progress", &Results[i].Session.SessionSettings.bAllowJoinInProgress);
						ImGui::Checkbox("Allow join via presence", &Results[i].Session.SessionSettings.bAllowJoinViaPresence);
						ImGui::Checkbox("Allow join via presence friends only", &Results[i].Session.SessionSettings.bAllowJoinViaPresenceFriendsOnly);
						ImGui::InputInt("Build unique ID", &Results[i].Session.SessionSettings.BuildUniqueId);
						ImGui::PopDisabled();
						ImGui::Columns(2, "session custom settings");
						ImGui::Separator();
						ImGui::Text("Key");
						ImGui::NextColumn();
						ImGui::Text("Value");
						ImGui::Separator();
						ImGui::NextColumn();
						for (const auto& Pair : Results[i].Session.SessionSettings.Settings)
						{
							ImGui::Text(TCHAR_TO_ANSI(*Pair.Key.ToString()));
							ImGui::NextColumn();
							ImGui::Text(TCHAR_TO_ANSI(*Pair.Value.Data.ToString()));
							ImGui::NextColumn();
						}
						ImGui::EndTooltip();
					}
					ImGui::NextColumn();
					ImGui::PopID();
				}
			}
			else
			{
				ImGui::Columns(1);
				ImGui::PushDisabled();
				if (GetMultiplayerSubsystem()->SessionSearch.IsValid())
				{
					switch (GetMultiplayerSubsystem()->SessionSearch->SearchState)
					{
					case EOnlineAsyncTaskState::InProgress:
						ImGui::TextCentered("In progress");
						break;
					case EOnlineAsyncTaskState::Failed:
						ImGui::TextCentered("Failed");
						break;
					case EOnlineAsyncTaskState::NotStarted:
						ImGui::TextCentered("Not started");
						break;
					case EOnlineAsyncTaskState::Done:
						ImGui::TextCentered("Done");
						break;
					}
				}
				else
				{
					ImGui::TextCentered("No results");
				}
				ImGui::PopDisabled();
			}
			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ShowSessionInfo(NAME_GameSession);

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::End();
}
#endif

#if 0
void UBYGMultiplayerUI::HandleTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (ImGui::BeginPopupModal("Travel failure", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Failed travelling with error message:");
		ImGui::Text(TCHAR_TO_ANSI(*ErrorString));
		ImGui::Text("Failure type:");
		ImGui::Text(TCHAR_TO_ANSI(ETravelFailure::ToString(FailureType)));
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void UBYGMultiplayerUI::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (ImGui::BeginPopupModal("Network failure", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Failed network with error message:");
		ImGui::Text(TCHAR_TO_ANSI(*ErrorString));
		ImGui::Text("Failure type:");
		ImGui::Text(TCHAR_TO_ANSI(ENetworkFailure::ToString(FailureType)));
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

#endif
	UBYGMultiplayerSubsystem* UBYGMultiplayerUI::GetMultiplayerSubsystem() const
	{
		return Cast<UBYGMultiplayerSubsystem>(GetOuter());
	}
