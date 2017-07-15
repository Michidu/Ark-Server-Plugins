#pragma once

#include "ArkShop.h"

namespace Tools
{
	template <typename... Args>
	void SendDirectMessage(AShooterPlayerController* playerController, const wchar_t* msg, Args&&... args)
	{
		size_t size = swprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;

		wchar_t* buffer = new wchar_t[size];
		_snwprintf_s(buffer, size, _TRUNCATE, msg, std::forward<Args>(args)...);

		FString cmd(buffer);

		FLinearColor msgColor = {1,1,1,1};
		playerController->ClientServerChatDirectMessage(&cmd, msgColor, false);

		delete[] buffer;
	}

	template <typename... Args>
	void SendChatMessage(AShooterPlayerController* playerController, const FString& senderName, const wchar_t* msg, Args&&... args)
	{
		size_t size = swprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;

		wchar_t* buffer = new wchar_t[size];
		_snwprintf_s(buffer, size, _TRUNCATE, msg, std::forward<Args>(args)...);

		FString cmd(buffer);

		FChatMessage* chatMessage = static_cast<FChatMessage*>(malloc(sizeof(FChatMessage)));
		if (chatMessage)
		{
			chatMessage->SenderName = senderName;
			chatMessage->SenderSteamName = L"";
			chatMessage->SenderTribeName = L"";
			chatMessage->SenderId = 0;
			chatMessage->Message = cmd;
			chatMessage->Receiver = L"";
			chatMessage->SenderTeamIndex = 0;
			chatMessage->ReceivedTime = -1;
			chatMessage->SendMode = EChatSendMode::GlobalChat;
			chatMessage->RadioFrequency = 0;
			chatMessage->ChatType = EChatType::GlobalChat;
			chatMessage->SenderIcon = 0;
			chatMessage->UserId = L"";

			void* mem = malloc(sizeof(FChatMessage));
			if (mem)
			{
				FChatMessage* chat = new(mem) FChatMessage(chatMessage);

				playerController->ClientChatMessage(chat);

				chat->~FChatMessage();
				free(mem);
			}

			free(chatMessage);
		}

		delete[] buffer;
	}

	template <typename... Args>
	void SendNotification(AShooterPlayerController* playerController, const wchar_t* msg, FLinearColor color, float displayScale, float displayTime, UTexture2D* icon, Args&&... args)
	{
		size_t size = swprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;

		wchar_t* buffer = new wchar_t[size];
		_snwprintf_s(buffer, size, _TRUNCATE, msg, std::forward<Args>(args)...);

		FString cmd(buffer);

		playerController->ClientServerSOTFNotificationCustom(&cmd, color, displayScale, displayTime, icon, nullptr);

		delete[] buffer;
	}

	__int64 GetSteamId(AShooterPlayerController* playerController);
	AShooterPlayerController* FindPlayerFromName(const std::string& steamName);
	AShooterPlayerController* FindPlayerFromSteamId(unsigned __int64 steamId);
	int GetRandomNumber(int min, int max);
	void Log(const std::string& text);
	std::string GetCurrentDir();
	std::wstring ConvertToWideStr(const std::string& text);
}
