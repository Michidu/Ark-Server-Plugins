#pragma once
struct FInternetAddrBSD : FInternetAddr
{
	sockaddr_in& AddrField() { return *GetNativePointerField<sockaddr_in*>(this, "FInternetAddrBSD.Addr"); }

	// Functions

	void SetIp(unsigned int InAddr) { return NativeCall<void, unsigned int>(this, "FInternetAddrBSD.SetIp", InAddr); }
	void SetIp(const wchar_t * InAddr, bool * bIsValid) { return NativeCall<void, const wchar_t *, bool *>(this, "FInternetAddrBSD.SetIp", InAddr, bIsValid); }
	void GetIp(unsigned int * OutAddr) { return NativeCall<void, unsigned int *>(this, "FInternetAddrBSD.GetIp", OutAddr); }
	void SetPorta(int InPort) { return NativeCall<void, int>(this, "FInternetAddrBSD.SetPort", InPort); }
	void GetPort(int * OutPort) { return NativeCall<void, int *>(this, "FInternetAddrBSD.GetPort", OutPort); }
	int GetPort() { return NativeCall<int>(this, "FInternetAddrBSD.GetPort"); }
	void SetAnyAddress() { return NativeCall<void>(this, "FInternetAddrBSD.SetAnyAddress"); }
	void SetBroadcastAddress() { return NativeCall<void>(this, "FInternetAddrBSD.SetBroadcastAddress"); }
	FString * ToString(FString * result, bool bAppendPort) { return NativeCall<FString *, FString *, bool>(this, "FInternetAddrBSD.ToString", result, bAppendPort); }
	bool operator==(FInternetAddr * Other) { return NativeCall<bool, FInternetAddr *>(this, "FInternetAddrBSD.operator==", Other); }
	bool IsValid() { return NativeCall<bool>(this, "FInternetAddrBSD.IsValid"); }
};

struct URCONServer
{
	FSocket * SocketField() { return *GetNativePointerField<FSocket **>(this, "URCONServer.Socket"); }
	TSharedPtr<FInternetAddr, 0>& ListenAddrField() { return *GetNativePointerField<TSharedPtr<FInternetAddr, 0>*>(this, "URCONServer.ListenAddr"); }
	TArray<RCONClientConnection *> ConnectionsField() { return *GetNativePointerField<TArray<RCONClientConnection *>*>(this, "URCONServer.Connections"); }
	UShooterCheatManager * CheatManagerField() { return *GetNativePointerField<UShooterCheatManager **>(this, "URCONServer.CheatManager"); }
	FString& ServerPasswordField() { return *GetNativePointerField<FString*>(this, "URCONServer.ServerPassword"); }

	// Functions

	static void SetClientMessage(FString Message) { return NativeCall<void, FString>(nullptr, "URCONServer.SetClientMessage", Message); }
	static void AddToChatBuffer(FString Message) { return NativeCall<void, FString>(nullptr, "URCONServer.AddToChatBuffer", Message); }
	static FString*GetChatBuffer(FString * result, bool bClearChatBuffer) { return NativeCall<FString*, FString *, bool>(nullptr, "URCONServer.GetChatBuffer", result, bClearChatBuffer); }
	static FString* GetGameLog(FString * result, TArray<FString> * GameLogStrings) { return NativeCall<FString *, FString *, TArray<FString> *>(nullptr, "URCONServer.GetGameLog", result, GameLogStrings); }
	bool Init(FString Password, int InPort, UShooterCheatManager * CSheatManager) { return NativeCall<bool, FString, int, UShooterCheatManager *>(this, "URCONServer.Init", Password, InPort, CSheatManager); }
	void Tick(float UnusedTime, UWorld * InWorld) { return NativeCall<void, float, UWorld *>(this, "URCONServer.Tick", UnusedTime, InWorld); }
	void Close() { return NativeCall<void>(this, "URCONServer.Close"); }
	void BeginDestroy() { return NativeCall<void>(this, "URCONServer.BeginDestroy"); }
	static UClass * StaticClass() { return NativeCall<UClass *>(nullptr, "URCONServer.StaticClass"); }
};