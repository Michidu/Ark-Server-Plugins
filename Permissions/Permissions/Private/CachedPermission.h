#pragma once
#ifdef PERMISSIONS_ARK
#include "../Public/ArkPermissions.h"
#else
#include "../Public/AtlasPermissions.h"
#endif
struct TimedGroup {
	FString GroupName;
	long long DelayUntilTime, ExpireAtTime;
};
class CachedPermission {
public:
	explicit CachedPermission() {}
	CachedPermission(FString Permissions, FString TimedPermissions) {
		Permissions.ParseIntoArray(Groups, L",", true);
		TArray<FString> TimedGroupStrs;
		TimedPermissions.ParseIntoArray(TimedGroupStrs, L",", true);
		for (auto GroupStr : TimedGroupStrs) {
			TArray<FString> GroupParts;
			GroupStr.ParseIntoArray(GroupParts, L";", true);
			TimedGroup Group;
			Group.GroupName = GroupParts[2];
			try
			{
				Group.DelayUntilTime = std::stoull(*GroupParts[0]);
				Group.ExpireAtTime = std::stoull(*GroupParts[1]);
			}
			catch (const std::exception& exception)
			{
				Log::GetLog()->error("({} {}) Parsing error {}", __FILE__, __FUNCTION__, exception.what());
			}
			TimedGroups.Add(Group);
		}
	}
	TArray<FString> Groups;
	TArray<TimedGroup> TimedGroups;
	TArray<FString> CallbackGroups;
	bool hasCheckedCallbacks;

	TArray<FString> getGroups(long long now) 
	{
		TArray<FString> result{ "Default" };
		for (auto group : Groups) result.AddUnique(group);
		for (auto group : TimedGroups) {
			if (group.DelayUntilTime > 0 && now < group.DelayUntilTime) {
				continue;
			}
			if (group.ExpireAtTime > 0 && now < group.ExpireAtTime) {
				result.AddUnique(group.GroupName);
			}
		}

		return result;
	}

	FString getGroupsStr(long long now) 
	{
		FString result;
		auto groups = getGroups(now);
		for (auto group : groups) {
			if (!result.IsEmpty())
				result += ",";
			result += group;
		}
		return result;
	}
};