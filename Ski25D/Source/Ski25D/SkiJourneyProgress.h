#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "SkiJourneyProgress.generated.h"

USTRUCT()
struct FSkiHistoryEntry
{
    GENERATED_BODY()
    UPROPERTY() int32 Chapter=0;
    UPROPERTY() int32 Score=0;
    UPROPERTY() int32 Flips=0;
    UPROPERTY() int32 Hearts=0;
    UPROPERTY() float Seconds=0;
    UPROPERTY() FString Date;
};

UCLASS()
class SKI25D_API USkiJourneySave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Version=1;
    UPROPERTY() TArray<FSkiHistoryEntry> Entries;
    UPROPERTY() TArray<int32> BestScores;
};

UCLASS()
class SKI25D_API USkiJourneyProgress : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    int32 ChapterScores[3]={0,0,0};
    bool Completed[3]={false,false,false};
    TArray<FSkiHistoryEntry> History;
    bool bStartOnArrival=false;
    bool bSaveFailed=false;
    bool bAudioMuted=false;
    void Record(int32 Chapter,int32 Score,float Seconds,int32 Flips,int32 Hearts);
    static bool IsTestRun();
    int32 Total()const{return ChapterScores[0]+ChapterScores[1]+ChapterScores[2];}
};
