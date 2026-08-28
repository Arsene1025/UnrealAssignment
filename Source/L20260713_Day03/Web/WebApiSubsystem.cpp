// Fill out your copyright notice in the Description page of Project Settings.


#include "WebApiSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "../DataGameInstanceSubsystem.h"

namespace
{
	constexpr int32 WebServerPort = 8080;
}

void UWebApiSubsystem::RequestLogin(const FString& InServerIP, const FString& InUserID, const FString& InPassword)
{
	SendAuthRequest(InServerIP, TEXT("/login"), InUserID, InPassword, OnLoginResult, true);
}

void UWebApiSubsystem::RequestSignUp(const FString& InServerIP, const FString& InUserID, const FString& InPassword)
{
	SendAuthRequest(InServerIP, TEXT("/signup"), InUserID, InPassword, OnSignUpResult, false);
}

void UWebApiSubsystem::RequestGameServerRegistration()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("server_address"), GetAdvertisedGameServerAddress());

	FString RegistrationToken;
	GConfig->GetString(TEXT("WebApi"), TEXT("RegistrationToken"), RegistrationToken, GGameIni);
	FParse::Value(FCommandLine::Get(), TEXT("RegistrationToken="), RegistrationToken);
	JsonObject->SetStringField(TEXT("registration_token"), RegistrationToken);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject, Writer);

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(GetConfiguredWebApiBaseUrl() + TEXT("/servers/register"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!bInConnectedSuccessfully || !InResponse.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("게임 서버 주소를 웹 서버에 등록하지 못했습니다"));
				return;
			}

			if (InResponse->GetResponseCode() != 200)
			{
				UE_LOG(LogTemp, Error, TEXT("게임 서버 등록 실패 (HTTP %d): %s"),
					InResponse->GetResponseCode(), *InResponse->GetContentAsString());
				return;
			}

			UE_LOG(LogTemp, Display, TEXT("게임 서버 주소 등록 완료: %s"),
				*WeakThis->GetAdvertisedGameServerAddress());
		});

	Request->ProcessRequest();
}

void UWebApiSubsystem::SendAuthRequest(const FString& InServerIP, const FString& InPath,
	const FString& InUserID, const FString& InPassword,
	FWebApiResultSignature& InDelegate, const bool bInIsLogin)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("user_id"), InUserID);
	JsonObject->SetStringField(TEXT("passwd"), InPassword);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject, Writer);

	const FString Url = FString::Printf(TEXT("http://%s:%d%s"), *InServerIP, WebServerPort, *InPath);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Url);

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	// 응답이 도착하기 전에 GameInstance가 정리될 수 있으므로 약참조로 잡는다.
	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);
	FWebApiResultSignature* DelegatePtr = &InDelegate;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, DelegatePtr, bInIsLogin](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->HandleAuthResponse(InResponse, bInConnectedSuccessfully, *DelegatePtr, bInIsLogin);
		});

	Request->ProcessRequest();
}

void UWebApiSubsystem::HandleAuthResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
	FWebApiResultSignature& InDelegate, const bool bInIsLogin)
{
	if (!bInConnectedSuccessfully || !InResponse.IsValid())
	{
		InDelegate.Broadcast(false, TEXT("서버에 연결할 수 없습니다"));
		return;
	}

	const int32 ResponseCode = InResponse->GetResponseCode();
	if (ResponseCode != 200)
	{
		InDelegate.Broadcast(false, FString::Printf(TEXT("요청을 처리할 수 없습니다 (코드 %d)"), ResponseCode));
		return;
	}

	const FString ResponseBody = InResponse->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		InDelegate.Broadcast(false, TEXT("응답을 해석할 수 없습니다"));
		return;
	}

	if (!JsonObject->GetBoolField(TEXT("result")))
	{
		InDelegate.Broadcast(false, JsonObject->GetStringField(TEXT("message")));
		return;
	}

	if (bInIsLogin)
	{
		UDataGameInstanceSubsystem* Data = GetGameInstance()->GetSubsystem<UDataGameInstanceSubsystem>();
		if (Data)
		{
			Data->Idx = JsonObject->GetIntegerField(TEXT("idx"));
			Data->Nickname = JsonObject->GetStringField(TEXT("nickname"));
			Data->Level = JsonObject->GetIntegerField(TEXT("level"));
			JsonObject->TryGetStringField(TEXT("server_address"), Data->GameServerAddress);
			Data->bLoggedIn = true;
		}
	}

	InDelegate.Broadcast(true, TEXT(""));
}

FString UWebApiSubsystem::GetConfiguredWebApiBaseUrl() const
{
	FString BaseUrl;
	GConfig->GetString(TEXT("WebApi"), TEXT("BaseUrl"), BaseUrl, GGameIni);
	FParse::Value(FCommandLine::Get(), TEXT("WebApiBaseUrl="), BaseUrl);

	BaseUrl.RemoveFromEnd(TEXT("/"));
	return BaseUrl.IsEmpty() ? TEXT("http://127.0.0.1:8080") : BaseUrl;
}

FString UWebApiSubsystem::GetAdvertisedGameServerAddress() const
{
	FString ServerAddress;
	GConfig->GetString(TEXT("WebApi"), TEXT("GameServerAddress"), ServerAddress, GGameIni);
	FParse::Value(FCommandLine::Get(), TEXT("PublicServerAddress="), ServerAddress);

	return ServerAddress.IsEmpty() ? TEXT("127.0.0.1:7777") : ServerAddress;
}
