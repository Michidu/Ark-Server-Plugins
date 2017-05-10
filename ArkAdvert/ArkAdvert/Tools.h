#pragma once

#include <functional>
#include <thread>
#include "API/Base.h"

namespace Tools
{
	class Timer
	{
	public:
		template <class callable, class... arguments>
		Timer(int after, bool async, callable&& f, arguments&&... args)
		{
			std::function<typename std::result_of<callable(arguments ...)>::type()> task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

			if (async)
			{
				std::thread([after, task]()
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(after));
						task();
					}).detach();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(after));
				task();
			}
		}
	};

	template <typename... Args>
	void SendColoredMessageToAll(const wchar_t* msg, FLinearColor color, Args&&... args)
	{
		size_t size = swprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;

		wchar_t* buffer = new wchar_t[size];
		_snwprintf_s(buffer, size, _TRUNCATE, msg, std::forward<Args>(args)...);

		FString cmd(buffer);

		auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
		for (uint32_t i = 0; i < playerControllers.Num(); ++i)
		{
			auto playerController = playerControllers[i];

			AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

			aShooterPC->ClientServerChatDirectMessage(&cmd, color, false);
		}

		delete[] buffer;
	}

	template <typename... Args>
	void SendColoredMessage(AShooterPlayerController* playerController, const wchar_t* msg, FLinearColor color, Args&&... args)
	{
		size_t size = swprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;

		wchar_t* buffer = new wchar_t[size];
		_snwprintf_s(buffer, size, _TRUNCATE, msg, std::forward<Args>(args)...);

		FString cmd(buffer);

		playerController->ClientServerChatDirectMessage(&cmd, color, false);

		delete[] buffer;
	}

	template <typename... Args>
	void SendChatMessageToAll(const FString& senderName, const wchar_t* msg, Args&&... args)
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
			FChatMessage* chat = new(mem) FChatMessage(chatMessage);

			auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
			for (uint32_t i = 0; i < playerControllers.Num(); ++i)
			{
				auto playerController = playerControllers[i];

				AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

				aShooterPC->ClientChatMessage(chat);
			}

			chat->~FChatMessage();
			free(mem);

			free(chatMessage);
		}

		delete[] buffer;
	}

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

	int GetRandomNumber(int min, int max);
	std::string GetCurrentDir();
}
