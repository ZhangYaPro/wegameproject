#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkiGameAudio.generated.h"
class UAudioComponent;
class USoundWave;
class ASkiRunnerCharacter;

UCLASS()
class SKI25D_API ASkiGameAudio : public AActor
{
    GENERATED_BODY()
public:
    ASkiGameAudio();
    virtual void Tick(float Dt) override;
    static void Event(const UObject* WorldContext,FName Name,float Volume=1);
    float MusicGain(int32 Index)const{return Gains[Index];}
    int32 EventCount(FName Name)const{return Counts.FindRef(Name);}
    float GlideGain()const{return SnowGain+WoodGain;}
    bool AssetsReady()const;
private:
    UPROPERTY() TArray<UAudioComponent*> Music;
    UPROPERTY() UAudioComponent* Snow;
    UPROPERTY() UAudioComponent* Wood;
    UPROPERTY() TMap<FName,USoundWave*> Sounds;
    UPROPERTY() ASkiRunnerCharacter* Runner;
    TArray<TWeakObjectPtr<UAudioComponent>> ActiveEffects;
    TMap<FName,double> LastEvent;
    TMap<FName,int32> Counts;
    float Gains[3]={0,0,0};
    float SnowGain=0,WoodGain=0;
    void Play(FName Name,float Volume);
};
