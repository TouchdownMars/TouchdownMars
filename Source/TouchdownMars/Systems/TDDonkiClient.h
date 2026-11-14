// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interfaces/IHttpRequest.h"
#include "Model/TDTypes.h"
#include "TDDonkiClient.generated.h"

class UDataTable;

/**
 * One fetch per session: FLR, CME and GST for the last DonkiRangeDays days from NASA CCMC DONKI (no API key).
 * Any failure or timeout gives the archive (DT_TD_DonkiArchive) with Status Archive. The panel never changes gameplay.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDDonkiClient : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_OneParam(FOnDone, const FTDSpaceWeather&);

	void Fetch(const UDataTable* ArchiveTable, float TimeoutSeconds, int32 RangeDays, FOnDone InOnDone);

	/** "X8.2" > "M3.5" > "C1.0"; returns the stronger class string. */
	static FString StrongerClass(const FString& A, const FString& B);

	/** Base URL; tests point it at a bad host to check the archive fallback. */
	FString BaseUrl = TEXT("https://ccmc.gsfc.nasa.gov/DONKI/WS/get/");

private:
	void HandleResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected, FString Type);
	void FinishWithArchive();

	FOnDone OnDone;
	FTDSpaceWeather Result;
	int32 Pending = 0;
	bool bFailed = false;
	bool bFinished = false;

	UPROPERTY()
	TObjectPtr<const UDataTable> Archive;
};
