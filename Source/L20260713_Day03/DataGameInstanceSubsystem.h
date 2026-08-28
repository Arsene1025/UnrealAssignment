// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API UDataGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString UserID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString Password;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString ServerIP;

	/** 로그인 응답으로 받은 실제 언리얼 게임 서버 주소(IP:Port). */
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString GameServerAddress;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	bool bLoggedIn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	int32 Idx = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	int32 Level = 0;

};
