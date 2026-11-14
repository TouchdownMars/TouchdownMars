// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDDonkiClient.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/DataTable.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TouchdownMars.h"

namespace
{
	int32 ClassRank(const FString& Class)
	{
		static const FString Letters = TEXT("ABCMX");
		int32 Letter = INDEX_NONE;
		if (Class.IsEmpty() || !Letters.FindChar(Class[0], Letter))
		{
			return -1;
		}
		return Letter * 1000 + FMath::RoundToInt(FCString::Atof(*Class.Mid(1)) * 10.f);
	}
}

FString UTDDonkiClient::StrongerClass(const FString& A, const FString& B)
{
	return ClassRank(A) >= ClassRank(B) ? A : B;
}

void UTDDonkiClient::Fetch(const UDataTable* ArchiveTable, float TimeoutSeconds, int32 RangeDays, FOnDone InOnDone)
{
	Archive = ArchiveTable;
	OnDone = MoveTemp(InOnDone);
	Result = FTDSpaceWeather();
	Result.Status = ETDDataStatus::Live;
	bFailed = false;
	bFinished = false;

	const FDateTime End = FDateTime::UtcNow();
	const FDateTime Start = End - FTimespan::FromDays(FMath::Clamp(RangeDays, 1, 60));
	const FString Range = FString::Printf(TEXT("?startDate=%s&endDate=%s"), *Start.ToString(TEXT("%Y-%m-%d")), *End.ToString(TEXT("%Y-%m-%d")));

	const TArray<FString> Types = { TEXT("FLR"), TEXT("CME"), TEXT("GST") };
	Pending = Types.Num();
	for (const FString& Type : Types)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(BaseUrl + Type + Range);
		Request->SetVerb(TEXT("GET"));
		Request->SetTimeout(TimeoutSeconds);
		Request->OnProcessRequestComplete().BindUObject(this, &UTDDonkiClient::HandleResponse, Type);
		if (!Request->ProcessRequest())
		{
			bFailed = true;
			--Pending;
		}
	}
	if (Pending == 0)
	{
		FinishWithArchive();
	}
}

void UTDDonkiClient::HandleResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected, FString Type)
{
	if (bFinished)
	{
		return;
	}
	TArray<TSharedPtr<FJsonValue>> Items;
	const bool bOk = bConnected && Response.IsValid() && Response->GetResponseCode() == 200
		&& (Response->GetContentAsString().TrimStartAndEnd().IsEmpty()
			|| FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Items));
	if (!bOk)
	{
		bFailed = true;
	}
	else
	{
		for (const TSharedPtr<FJsonValue>& Item : Items)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Item.IsValid() || !Item->TryGetObject(Object))
			{
				continue;
			}
			FTDDonkiRow Row;
			Row.EventType = Type;
			Row.SourceURL = (*Object)->GetStringField(TEXT("link"));
			if (Type == TEXT("FLR"))
			{
				++Result.FlareCount;
				Row.ActivityId = (*Object)->GetStringField(TEXT("flrID"));
				Row.TimeUTC = (*Object)->GetStringField(TEXT("peakTime"));
				Row.ClassType = (*Object)->GetStringField(TEXT("classType"));
				Result.StrongestClass = StrongerClass(Result.StrongestClass, Row.ClassType);
			}
			else if (Type == TEXT("CME"))
			{
				++Result.CMECount;
				Row.ActivityId = (*Object)->GetStringField(TEXT("activityID"));
				Row.TimeUTC = (*Object)->GetStringField(TEXT("startTime"));
			}
			else
			{
				++Result.StormCount;
				Row.ActivityId = (*Object)->GetStringField(TEXT("gstID"));
				Row.TimeUTC = (*Object)->GetStringField(TEXT("startTime"));
			}
			Result.Rows.Add(Row);
		}
	}

	if (--Pending > 0)
	{
		return;
	}
	if (bFailed)
	{
		FinishWithArchive();
		return;
	}
	bFinished = true;
	Result.FetchedAtUTC = FDateTime::UtcNow();
	UE_LOG(LogTouchdownMars, Log, TEXT("DONKI LIVE: %d flares (strongest %s), %d CMEs, %d storms"), Result.FlareCount, *Result.StrongestClass, Result.CMECount, Result.StormCount);
	OnDone.ExecuteIfBound(Result);
}

void UTDDonkiClient::FinishWithArchive()
{
	bFinished = true;
	Result = FTDSpaceWeather();
	Result.Status = ETDDataStatus::Archive;
	if (Archive)
	{
		for (const TPair<FName, uint8*>& Pair : Archive->GetRowMap())
		{
			const FTDDonkiRow& Row = *reinterpret_cast<const FTDDonkiRow*>(Pair.Value);
			Result.Rows.Add(Row);
			if (Row.EventType == TEXT("FLR"))
			{
				++Result.FlareCount;
				Result.StrongestClass = StrongerClass(Result.StrongestClass, Row.ClassType);
			}
			else if (Row.EventType == TEXT("CME"))
			{
				++Result.CMECount;
			}
			else
			{
				++Result.StormCount;
			}
		}
	}
	UE_LOG(LogTouchdownMars, Log, TEXT("DONKI ARCHIVE: %d rows"), Result.Rows.Num());
	OnDone.ExecuteIfBound(Result);
}
