#pragma once

#include "windows.h"
#include "Timer.h"

namespace SafeZones::Tools
{
	class Timer
	{
	public:
		Timer()
		{
			ArkApi::GetCommands().AddOnTimerCallback("SZ.HelperTimer.Update", std::bind(&Timer::Update, this));
		}

		~Timer()
		{
			ArkApi::GetCommands().RemoveOnTimerCallback("SZ.HelperTimer.Update");
		}

		template <class callable, class... arguments>
		void DelayExec(callable&& f, int after, bool async, arguments&&... args)
		{
			if (!async)
			{
				tasks_.push_back(std::make_shared<Task>(std::bind(f, std::forward<arguments>(args)...), after));
			}
			else
			{
				auto func = std::bind(f, std::forward<arguments>(args)...);
				std::thread([&func, after]()
					{
						std::this_thread::sleep_for(std::chrono::seconds(after));
						func();
					}
				).detach();
			}
		}

		void Update()
		{
			const time_t now = std::time(nullptr);
			for (auto& func : tasks_)
			{
				if (func
					&& func->execTime <= now
					&& func->callback)
				{
					func->callback();

					try
					{
						auto it = std::find(tasks_.begin(), tasks_.end(), func);

						if (it != tasks_.end())
							tasks_.erase(it);
					}
					catch (std::exception& ex)
					{
						Log::GetLog()->error(ex.what());
					}
				}
			}
		}

		static Timer& Get()
		{
			static Timer instance;
			return instance;
		}

	private:
		struct Task
		{
			Task(std::function<void()> cbk, int after)
				: callback(std::move(cbk))
			{
				execTime = std::time(nullptr) + after;
			}

			std::function<void()> callback;
			time_t execTime;
		};

		std::vector<std::shared_ptr<Task>> tasks_;
	};

	inline int GetRandomNumber(int min, int max)
	{
		const int n = max - min + 1;
		const int remainder = RAND_MAX % n;
		int x;

		do
		{
			x = rand();
		}
		while (x >= RAND_MAX - remainder);

		return min + x % n;
	}

	inline FCustomItemData GetDinoCustomItemData(APrimalDinoCharacter* dino, UPrimalItem* saddle)
	{
		FCustomItemData customItemData;

		FARKDinoData dinoData;
		dino->GetDinoData(&dinoData);

		customItemData.CustomDataName = FName("Dino", EFindName::FNAME_Add);
		customItemData.CustomDataNames.Add(FName("MissionTemporary", EFindName::FNAME_Add));
		customItemData.CustomDataNames.Add(FName("None", EFindName::FNAME_Find));

		customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->CurrentStatusValuesField()()[EPrimalCharacterStatusValue::Health]);
		customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->CurrentStatusValuesField()()[EPrimalCharacterStatusValue::Stamina]);
		customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->CurrentStatusValuesField()()[EPrimalCharacterStatusValue::Torpidity]);
		customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->MaxStatusValuesField()()[EPrimalCharacterStatusValue::Health]);
		customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->MaxStatusValuesField()()[EPrimalCharacterStatusValue::Stamina]);
		customItemData.CustomDataFloats.Add(dino->MyCharacterStatusComponentField()->MaxStatusValuesField()()[EPrimalCharacterStatusValue::Torpidity]);
		customItemData.CustomDataFloats.Add(dino->bIsFemale()());

		customItemData.CustomDataStrings.Add(dinoData.DinoNameInMap);
		customItemData.CustomDataStrings.Add(dinoData.DinoName);
		customItemData.CustomDataClasses.Add(dinoData.DinoClass);

		FCustomItemByteArray dinoBytes, saddlebytes;
		dinoBytes.Bytes = dinoData.DinoData;
		customItemData.CustomDataBytes.ByteArrays.Add(dinoBytes);
		if (saddle)
		{
			saddle->GetItemBytes(&saddlebytes.Bytes);
			customItemData.CustomDataBytes.ByteArrays.Add(saddlebytes);
		}

		return customItemData;
	}

	//Spawns dino or gives in cryopod
	inline bool GiveDino(AShooterPlayerController* player_controller, int level, bool neutered, std::string blueprint, std::string saddleblueprint)
	{
		bool success = false;
		const FString fblueprint(blueprint.c_str());
		APrimalDinoCharacter* dino = ArkApi::GetApiUtils().SpawnDino(player_controller, fblueprint, nullptr, level, true, neutered);
		if (dino)
		{
			FString cryo = FString(config.value("CryoItemPath", "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'"));
			UClass* Class = UVictoryCore::BPLoadClass(&cryo);
			UPrimalItem* item = UPrimalItem::AddNewItem(Class, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0);
			if (item)
			{
				UPrimalItem* saddle = nullptr;
				if (saddleblueprint.size() > 0)
				{
					FString fblueprint(saddleblueprint.c_str());
					UClass* Class = UVictoryCore::BPLoadClass(&fblueprint);
					saddle = UPrimalItem::AddNewItem(Class, nullptr, false, false, 0, false, 0, false, 0, false, nullptr, 0);
				}

				FCustomItemData customItemData = GetDinoCustomItemData(dino, saddle);
				item->SetCustomItemData(&customItemData);
				item->UpdatedItem(true);

				if (player_controller->GetPlayerInventoryComponent())
				{
					UPrimalItem* item2 = player_controller->GetPlayerInventoryComponent()->AddItemObject(item);

					if (item2)
						success = true;
				}
			}

			dino->Destroy(true, false);
		}
		else if (dino)
			success = true;

		return success;
	}

	inline bool ShouldSendNotification(AActor* player, FVector& zone_pos, int radius)
	{
		if (player->ActorHasTag(FName("IgnoreSzNotification", EFindName::FNAME_Add)))
			return false;

		FVector player_pos = player->RootComponentField()->RelativeLocationField();
		const float dist = FVector::Distance(player_pos, zone_pos);

		// Don't show notif if it's away from zone border

		const float min_dist = (float)radius - 100.f;

		if (dist >= min_dist)
			return true;
		else
			return false;
	}
}
